// Two-lane budgeted job queue (INTERFACES v2.1 section 3). All heavy XML Editor work runs as VPPXEJob steps inside
// a per-frame time budget: INTERACTIVE first, BACKGROUND guaranteed its share while both lanes are busy.
// The pump runs once per server frame (CALL_CATEGORY_SYSTEM call queue, ticked from DayZGame.OnUpdate) and assumes
// no frame rate. The slice of a frame is JobBudgetMs, raised by frame pacing to FRAME_SHARE_PCT of the measured pump
// interval (a slow-ticking server gives more per frame), and set to JobBoostMs while an admin waits (INTERACTIVE work
// queued or KeepUrgent) or nobody is connected. It never exceeds max(JobBudgetMs, JobBoostMs).

class VPPXEJob : Managed
{
	// Queue statistics: ticks spent inside Step, Step calls, game time of the first Enqueue (-1 = never queued).
	int StatTicks;
	int StatSteps;
	int StatQueuedMs = -1;

	// Bounded work: loop while VPPXEJobQueue.BudgetOk(); return true when finished (success or failure).
	bool Step()
	{
		return true;
	}

	// Called once after Step returned true (the job is already out of its lane).
	void OnFinished()
	{
	}

	// Called once when the queue detects that Step aborted with a VM error; request-serving jobs reply ERR_INTERNAL.
	void OnAborted()
	{
	}

	string GetLabel()
	{
		return "VPPXEJob";
	}
};

class VPPXEJobQueue : Managed
{
	const static int DEFAULT_BUDGET_MS = 4;
	const static int DEFAULT_BOOST_MS = 8;
	const static int DEFAULT_BACKGROUND_SHARE = 25;
	// Pre-calibration BudgetOk calls per frame. Deliberately 100, not 2000: one budget unit can be a splitter Step(128)
	// or a reader Step(64), and a frame over 500 ms also keeps restarting the calibration window.
	const static int FALLBACK_CALLS_PER_FRAME = 100;
	const static int CALIBRATE_MIN_MS = 150;
	const static int CALIBRATE_MAX_MS = 500;
	const static int PUMP_STALL_MS = 2000;
	// Frame pacing: the slice may grow to this share of the smoothed pump interval (clamped to budget..boost).
	const static int FRAME_SHARE_PCT = 25;
	// Pump intervals above this are hitches or restarts, not the frame rate; they are not averaged in.
	const static int MAX_GAP_MS = 1000;
	// How often the connected-identity count is refreshed.
	const static int PLAYER_CHECK_MS = 1000;
	// KeepUrgent hold used by services that poll their waiting requests (dist Tick runs every 250 ms).
	const static int URGENT_HOLD_MS = 1000;
	// Finished jobs that used less CPU than this are not logged.
	const static int STATS_MIN_CPU_MS = 50;

	static ref VPPXEJobQueue s_Instance;

	ref array<ref VPPXEJob> m_Interactive;
	ref array<ref VPPXEJob> m_Background;

	// Crash detection: the job whose Step is running (weak; the lane owns it) and its lane.
	VPPXEJob m_Running;
	int m_RunningLane;

	bool m_PumpActive;
	bool m_InPump;
	bool m_BothBusy;
	int m_LastPumpTime;
	int m_BudgetMs;
	int m_BoostMs;
	int m_BackgroundShare;

	// Slice of the current frame in ms (PickSliceMs) and the smoothed interval between pumps (includes our slice).
	float m_SliceMs;
	float m_FrameGapMs;
	int m_LastPumpTick;
	bool m_HaveLastPump;

	// Urgency: game time until which the boost applies (KeepUrgent), and the cached "nobody connected" state.
	int m_UrgentUntil;
	int m_NextPlayerCheck;
	bool m_NoPlayers;

	// Tick budget of the current frame (valid once calibrated).
	int m_FrameStart;
	int m_SliceEnd;

	// Call-count budget used until calibration succeeds.
	int m_CallCount;
	int m_CallSliceEnd;

	// Ticks-per-ms calibration over a 150..500 ms window of game time, sampled by its own light repeating call so it
	// never depends on job work or on the pump running.
	bool m_Calibrated;
	bool m_CalTickActive;
	float m_TicksPerMs;
	int m_CalStartTick;
	int m_CalStartTime;

	void VPPXEJobQueue()
	{
		m_Interactive = new array<ref VPPXEJob>;
		m_Background = new array<ref VPPXEJob>;
		m_RunningLane = -1;
		m_BudgetMs = DEFAULT_BUDGET_MS;
		m_BoostMs = DEFAULT_BOOST_MS;
		m_BackgroundShare = DEFAULT_BACKGROUND_SHARE;
		m_SliceMs = DEFAULT_BUDGET_MS;
		m_FrameGapMs = 0;
		m_HaveLastPump = false;
		m_UrgentUntil = 0;
		m_NextPlayerCheck = 0;
		m_NoPlayers = false;
		m_CalStartTime = -1;
		m_TicksPerMs = 0;
		EnsureCalibration();
	}

	void ~VPPXEJobQueue()
	{
		if (m_PumpActive && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Pump);
		}

		if (m_CalTickActive && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CalibrateTick);
		}
	}

	static VPPXEJobQueue Get()
	{
		if (!s_Instance)
		{
			s_Instance = new VPPXEJobQueue();
		}

		return s_Instance;
	}

	// True while the current lane slice has time left. Outside a pump there is no slice and it returns true, so only
	// jobs (driven by the pump) and their inner jobs may loop on it; unbudgeted wrappers never rely on it.
	static bool BudgetOk()
	{
		if (!s_Instance)
		{
			return true;
		}

		return s_Instance.SliceHasTime();
	}

	// FIFO per lane (VPPXELane). A job already queued is not added twice.
	void Enqueue(VPPXEJob job, int lane)
	{
		if (!job)
		{
			return;
		}

		if (job.StatQueuedMs < 0)
		{
			job.StatQueuedMs = CurrentTimeMs();
		}

		array<ref VPPXEJob> jobs = LaneArray(lane);
		if (jobs.Find(job) < 0)
		{
			jobs.Insert(job);
		}

		EnsurePump();
	}

	// Removes a queued job without OnFinished (clears the running marker when it matches).
	void Cancel(VPPXEJob job)
	{
		if (!job)
		{
			return;
		}

		if (m_Running == job)
		{
			m_Running = null;
			m_RunningLane = -1;
		}

		int at = m_Interactive.Find(job);
		if (at >= 0)
		{
			m_Interactive.RemoveOrdered(at);
		}

		at = m_Background.Find(job);
		if (at >= 0)
		{
			m_Background.RemoveOrdered(at);
		}
	}

	bool IsIdle()
	{
		return m_Interactive.Count() == 0 && m_Background.Count() == 0;
	}

	bool IsLaneIdle(int lane)
	{
		array<ref VPPXEJob> jobs = LaneArray(lane);
		return jobs.Count() == 0;
	}

	void SetBudgetMs(int ms)
	{
		m_BudgetMs = ClampMs(ms);
	}

	void SetBoostMs(int ms)
	{
		m_BoostMs = ClampMs(ms);
	}

	protected int ClampMs(int ms)
	{
		int clamped = ms;
		if (clamped < 1)
		{
			clamped = 1;
		}

		if (clamped > 20)
		{
			clamped = 20;
		}

		return clamped;
	}

	void SetBackgroundShare(int pct)
	{
		int clamped = pct;
		if (clamped < 10)
		{
			clamped = 10;
		}

		if (clamped > 90)
		{
			clamped = 90;
		}

		m_BackgroundShare = clamped;
	}

	// Called while an admin waits for queued work (for example the dist Tick while requests are pending): the next
	// frames use the boost slice for holdMs.
	void KeepUrgent(int holdMs)
	{
		int holdEnd = CurrentTimeMs() + holdMs;
		if (holdEnd > m_UrgentUntil)
		{
			m_UrgentUntil = holdEnd;
		}
	}

	bool IsUrgent()
	{
		if (m_Interactive.Count() > 0)
		{
			return true;
		}

		return CurrentTimeMs() < m_UrgentUntil;
	}

	array<ref VPPXEJob> LaneArray(int lane)
	{
		if (lane == VPPXELane.BACKGROUND)
		{
			return m_Background;
		}

		return m_Interactive;
	}

	int CurrentTimeMs()
	{
		if (!GetGame())
		{
			return 0;
		}

		return GetGame().GetTime();
	}

	// Starts the repeating pump when it is not running. A pump that has not run for PUMP_STALL_MS is assumed to have
	// been dropped after a VM error and is registered again (Remove first, so it can never run twice per frame).
	void EnsurePump()
	{
		if (!GetGame())
		{
			return;
		}

		EnsureCalibration();
		int now = CurrentTimeMs();
		if (m_PumpActive)
		{
			if (now - m_LastPumpTime < PUMP_STALL_MS)
			{
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Pump);
			Print("[XMLEditor][Jobs] pump stalled, restarting it");
		}

		m_PumpActive = true;
		m_HaveLastPump = false;
		m_LastPumpTime = now;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Pump, 0, true);
	}

	void StopPump()
	{
		if (m_PumpActive && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Pump);
		}

		m_PumpActive = false;
		m_HaveLastPump = false;
	}

	// One frame of work: recover a crashed job, then run INTERACTIVE and BACKGROUND in their slices.
	void Pump()
	{
		m_LastPumpTime = CurrentTimeMs();
		m_InPump = false;
		RecoverAbortedJob();
		if (IsIdle())
		{
			StopPump();
			return;
		}

		m_BothBusy = m_Interactive.Count() > 0 && m_Background.Count() > 0;
		MeasureFrameGap();
		m_SliceMs = PickSliceMs();
		m_CallCount = 0;
		m_InPump = true;
		RunLane(VPPXELane.INTERACTIVE);
		RunLane(VPPXELane.BACKGROUND);
		m_InPump = false;
		if (IsIdle())
		{
			StopPump();
		}
	}

	// Sets m_FrameStart and folds the interval since the previous pump into m_FrameGapMs.
	protected void MeasureFrameGap()
	{
		int gapTicks = 0;
		bool haveGap = m_HaveLastPump;
		if (haveGap)
		{
			gapTicks = TickCount(m_LastPumpTick);
		}

		m_FrameStart = TickCount(0);
		m_LastPumpTick = m_FrameStart;
		m_HaveLastPump = true;
		if (!haveGap || !m_Calibrated || gapTicks <= 0)
		{
			return;
		}

		float gapMs = gapTicks;
		gapMs = gapMs / m_TicksPerMs;
		if (gapMs > MAX_GAP_MS)
		{
			return;
		}

		if (m_FrameGapMs <= 0)
		{
			m_FrameGapMs = gapMs;
			return;
		}

		m_FrameGapMs = m_FrameGapMs * 0.8 + gapMs * 0.2;
	}

	// JobBudgetMs raised by frame pacing; the boost while an admin waits or nobody is connected; never above the boost.
	protected float PickSliceMs()
	{
		float ceiling = m_BoostMs;
		if (ceiling < m_BudgetMs)
		{
			ceiling = m_BudgetMs;
		}

		if (IsUrgent() || NoPlayersOnline())
		{
			return ceiling;
		}

		float slice = m_BudgetMs;
		float paced = m_FrameGapMs * FRAME_SHARE_PCT;
		paced = paced / 100.0;
		if (paced > slice)
		{
			slice = paced;
		}

		if (slice > ceiling)
		{
			slice = ceiling;
		}

		return slice;
	}

	// Connected identities, refreshed at most every PLAYER_CHECK_MS (an empty server has nobody to notice a longer slice).
	protected bool NoPlayersOnline()
	{
		int now = CurrentTimeMs();
		if (now >= m_NextPlayerCheck)
		{
			m_NextPlayerCheck = now + PLAYER_CHECK_MS;
			array<PlayerIdentity> identities = new array<PlayerIdentity>();
			GetGame().GetPlayerIndentities(identities);
			m_NoPlayers = identities.Count() == 0;
		}

		return m_NoPlayers;
	}

	// A running marker that survived to the next pump means the previous Step aborted with a VM error.
	void RecoverAbortedJob()
	{
		if (!m_Running)
		{
			m_RunningLane = -1;
			return;
		}

		VPPXEJob crashed = m_Running;
		m_Running = null;
		m_RunningLane = -1;
		int at = m_Interactive.Find(crashed);
		if (at >= 0)
		{
			m_Interactive.RemoveOrdered(at);
		}

		at = m_Background.Find(crashed);
		if (at >= 0)
		{
			m_Background.RemoveOrdered(at);
		}

		string label = crashed.GetLabel();
		Print("[XMLEditor][Jobs] job aborted with a script error and was dropped: " + label);
		crashed.OnAborted();
	}

	void RunLane(int lane)
	{
		array<ref VPPXEJob> jobs = LaneArray(lane);
		if (jobs.Count() == 0)
		{
			return;
		}

		BeginSlice(lane);
		while (jobs.Count() > 0 && BudgetOk())
		{
			VPPXEJob head = jobs[0];
			m_Running = head;
			m_RunningLane = lane;
			int stepStart = TickCount(0);
			bool finished = head.Step();
			head.StatTicks = head.StatTicks + TickCount(stepStart);
			head.StatSteps = head.StatSteps + 1;
			m_Running = null;
			m_RunningLane = -1;
			if (!finished)
			{
				continue;
			}

			int at = jobs.Find(head);
			if (at < 0)
			{
				continue;
			}

			jobs.RemoveOrdered(at);
			LogJobStats(head);
			head.OnFinished();
		}
	}

	// One line per finished job that used at least STATS_MIN_CPU_MS: CPU inside Step, Step calls (about one per frame),
	// wall time since the first Enqueue, the smoothed frame interval and the last slice.
	protected void LogJobStats(VPPXEJob job)
	{
		if (!m_Calibrated || m_TicksPerMs <= 0)
		{
			return;
		}

		float cpuMs = job.StatTicks;
		cpuMs = cpuMs / m_TicksPerMs;
		if (cpuMs < STATS_MIN_CPU_MS)
		{
			return;
		}

		int wallMs = CurrentTimeMs() - job.StatQueuedMs;
		string text = "[XMLEditor][Jobs] " + job.GetLabel() + " done: cpu " + cpuMs.ToString() + " ms in " + job.StatSteps.ToString() + " steps, wall " + wallMs.ToString() + " ms, frame " + m_FrameGapMs.ToString() + " ms, slice " + m_SliceMs.ToString() + " ms";
		Print(text);
	}

	// INTERACTIVE ends at total * (100 - share) / 100 when both lanes had work at frame start; BACKGROUND (or a lone
	// busy lane) ends at total. Float math avoids int overflow of ticks * percent.
	void BeginSlice(int lane)
	{
		float fraction = 1.0;
		if (m_BothBusy && lane == VPPXELane.INTERACTIVE)
		{
			float keepPct = 100 - m_BackgroundShare;
			fraction = keepPct / 100.0;
		}

		if (m_Calibrated)
		{
			float sliceTicks = m_TicksPerMs * m_SliceMs;
			sliceTicks = sliceTicks * fraction;
			m_SliceEnd = sliceTicks;
			return;
		}

		float sliceCalls = fraction * FALLBACK_CALLS_PER_FRAME;
		m_CallSliceEnd = sliceCalls;
	}

	bool SliceHasTime()
	{
		if (!m_InPump)
		{
			return true;
		}

		if (m_Calibrated)
		{
			return TickCount(m_FrameStart) < m_SliceEnd;
		}

		m_CallCount++;
		return m_CallCount <= m_CallSliceEnd;
	}

	// Starts the calibration sampler once (it removes itself when calibrated). Not tied to the pump or its idle stop.
	void EnsureCalibration()
	{
		if (m_Calibrated || m_CalTickActive || !GetGame())
		{
			return;
		}

		m_CalTickActive = true;
		m_CalStartTime = -1;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.CalibrateTick, 0, true);
	}

	// Ticks per ms from TickCount over a short game-time window (150..500 ms) so the int tick delta cannot overflow.
	// Only samples TickCount and GetTime; a window that overran (a long frame) or went backwards simply restarts, with no
	// attempt limit.
	void CalibrateTick()
	{
		if (m_Calibrated)
		{
			StopCalibration();
			return;
		}

		int now = CurrentTimeMs();
		if (m_CalStartTime < 0)
		{
			m_CalStartTime = now;
			m_CalStartTick = TickCount(0);
			return;
		}

		int elapsed = now - m_CalStartTime;
		if (elapsed < CALIBRATE_MIN_MS && elapsed >= 0)
		{
			return;
		}

		if (elapsed >= CALIBRATE_MIN_MS && elapsed < CALIBRATE_MAX_MS)
		{
			int ticks = TickCount(m_CalStartTick);
			float perMs = ticks;
			perMs = perMs / elapsed;
			if (perMs > 0)
			{
				m_TicksPerMs = perMs;
				m_Calibrated = true;
				Print("[XMLEditor][Jobs] budget calibrated: " + perMs.ToString() + " ticks per ms");
				StopCalibration();
				return;
			}
		}

		m_CalStartTime = now;
		m_CalStartTick = TickCount(0);
	}

	void StopCalibration()
	{
		if (m_CalTickActive && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CalibrateTick);
		}

		m_CalTickActive = false;
	}
};
