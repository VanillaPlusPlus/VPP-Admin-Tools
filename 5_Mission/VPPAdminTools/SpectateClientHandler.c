/*
	Client-side spectate glue (singleton, owned by MissionGameplay).

	Receives RPC_SpectateClient traffic from the SpectateManager plugin, drives the
	engine player-camera hijack (CameraHandler returns VPPAT_SPECTATE_CAMERA while the
	flag is set — the ENGINE constructs/destroys VPPDayZPlayerCameraSpectate; state
	reaches it via the class statics push protocol), mirrors the body-freeze locally
	(scripted commands are per-machine — same pattern as the freecam's HandleFreeCam),
	and exposes the state + ScriptInvokers the menu/overlay bind to.

	NOTE: "menu" inputs are deliberately NOT excluded while spectating so the admin
	hud / ESC remain usable over the camera.
*/
class VPPSpectateClientHandler
{
	protected static ref VPPSpectateClientHandler s_Instance;

	protected bool            m_Active;
	protected string          m_TargetId;
	protected int             m_ViewPref;
	protected float           m_VitalsHP;
	protected float           m_VitalsBlood;
	protected int             m_DeathGraceSec; //seconds to linger on a dead target (0 = immediate)
	protected int             m_TargetSightMode; //EVPPSpectateSightMode (server-pushed, edge-triggered)

	protected const static string PROFILE_DEATH_GRACE = "vppat_spectate_death_grace";

	ref ScriptInvoker m_OnSpectateStarted;    //(string targetId)
	ref ScriptInvoker m_OnSpectateStopped;    //()
	ref ScriptInvoker m_OnTargetLost;         //(string targetId)
	ref ScriptInvoker m_OnPerspectiveChanged; //(bool firstPerson)
	ref ScriptInvoker m_OnVitals;             //(float hpPct, float blood)
	ref ScriptInvoker m_OnADSChanged;         //(bool adsActive)

	void VPPSpectateClientHandler()
	{
		m_Active   = false;
		m_ViewPref = EVPPSpectateView.THIRD_PERSON;

		//load the persisted death-grace preference (empty = default; "0" = immediate)
		m_DeathGraceSec = VPPSpectateConstants.DEATH_GRACE_DEFAULT;
		string savedGrace;
		GetGame().GetProfileString(PROFILE_DEATH_GRACE, savedGrace);
		if (savedGrace != "")
			m_DeathGraceSec = Math.Clamp(savedGrace.ToInt(), 0, VPPSpectateConstants.DEATH_GRACE_MAX);

		m_OnSpectateStarted    = new ScriptInvoker();
		m_OnSpectateStopped    = new ScriptInvoker();
		m_OnTargetLost         = new ScriptInvoker();
		m_OnPerspectiveChanged = new ScriptInvoker();
		m_OnVitals             = new ScriptInvoker();
		m_OnADSChanged         = new ScriptInvoker();

		m_TargetSightMode = EVPPSpectateSightMode.NONE;

		GetRPCManager().AddRPC( "RPC_SpectateClient", "StartSpectate", this, SingleplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_SpectateClient", "EndSpectate", this, SingleplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_SpectateClient", "TargetVitals", this, SingleplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_SpectateClient", "TargetADS", this, SingleplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_SpectateClient", "TargetLook", this, SingleplayerExecutionType.Client );
	}

	static VPPSpectateClientHandler GetInstance()
	{
		if (s_Instance == null)
			s_Instance = new VPPSpectateClientHandler();
		return s_Instance;
	}

	//---------------------------------------------------------------------------------
	// Public surface (menu / overlay / keybinds)
	//---------------------------------------------------------------------------------
	bool IsSpectating()
	{
		return m_Active;
	}

	string GetTargetId()
	{
		return m_TargetId;
	}

	bool IsFirstPerson()
	{
		return m_ViewPref == EVPPSpectateView.FIRST_PERSON;
	}

	bool IsTargetADS()
	{
		return m_TargetSightMode != EVPPSpectateSightMode.NONE;
	}

	int GetTargetSightMode()
	{
		return m_TargetSightMode;
	}

	protected void ResetSightMode()
	{
		if (m_TargetSightMode != EVPPSpectateSightMode.NONE)
		{
			m_TargetSightMode = EVPPSpectateSightMode.NONE;
			m_OnADSChanged.Invoke(false);
		}
	}

	float GetVitalsHP()
	{
		return m_VitalsHP;
	}

	float GetVitalsBlood()
	{
		return m_VitalsBlood;
	}

	int GetDeathGrace()
	{
		return m_DeathGraceSec;
	}

	//menu dropdown: how long to linger on a dead target before auto-exit
	void SetDeathGrace(int seconds)
	{
		m_DeathGraceSec = Math.Clamp(seconds, 0, VPPSpectateConstants.DEATH_GRACE_MAX);
		GetGame().SetProfileString(PROFILE_DEATH_GRACE, m_DeathGraceSec.ToString());
		GetGame().SaveProfile();
		SyncDeathGrace();
	}

	protected void SyncDeathGrace()
	{
		GetRPCManager().VSendRPC("RPC_SpectateManager", "SetDeathGrace", new Param1<int>(m_DeathGraceSec), true);
	}

	//menu "Spectate"/"Switch Target" button + kN/kB cycling all land here
	void RequestSpectate(string steam64)
	{
		//push the current grace pref first so the server has it before any death
		SyncDeathGrace();
		GetRPCManager().VSendRPC("RPC_SpectateManager", "RequestSpectate", new Param1<string>(steam64), true);
	}

	void RequestExit()
	{
		if (!m_Active)
			return;

		GetRPCManager().VSendRPC("RPC_SpectateManager", "RequestExitSpectate", NULL, true);
		//failsafe: if the server reply never arrives, tear down locally — never reconnect
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.FailsafeTeardown, VPPSpectateConstants.EXIT_FAILSAFE_SEC * 1000, false);
	}

	void SetPerspective(bool firstPerson)
	{
		if (firstPerson)
			m_ViewPref = EVPPSpectateView.FIRST_PERSON;
		else
			m_ViewPref = EVPPSpectateView.THIRD_PERSON;

		if (m_Active)
			VPPDayZPlayerCameraSpectate.PushView(m_ViewPref);

		m_OnPerspectiveChanged.Invoke(firstPerson);
	}

	void TogglePerspective()
	{
		SetPerspective(!IsFirstPerson());
	}

	//---------------------------------------------------------------------------------
	// RPC handlers (server -> client)
	//---------------------------------------------------------------------------------
	void StartSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param2<Object, string> data;
		if (!ctx.Read(data))
			return;

		PlayerBase targetPb = PlayerBase.Cast(data.param1);
		if (targetPb == null)
		{
			//bubble race: target not streamed in yet — ask the server to resend
			GetRPCManager().VSendRPC("RPC_SpectateManager", "RequestInitRetry", NULL, true);
			return;
		}

		m_TargetId = data.param2;

		if (m_Active)
		{
			//target SWITCH: retarget only, no teardown (camera instance survives —
			//CameraHandler keeps returning the spectate id, no recreation)
			ResetSightMode();

			//re-link the local body to the new target (lag-free camera base)
			PlayerBase switchPb = PlayerBase.Cast(GetGame().GetPlayer());
			if (switchPb != null)
			{
				switchPb.VPPATDetachFromSpectateTarget();
				switchPb.VPPATAttachToSpectateTarget(targetPb);
			}

			VPPDayZPlayerCameraSpectate.PushTarget(targetPb);
			m_OnSpectateStarted.Invoke(m_TargetId);
			GetRPCManager().VSendRPC("RPC_SpectateManager", "ClientReadyAck", NULL, true);
			return;
		}

		//fresh enter
		DayZPlayerImplement player = DayZPlayerImplement.Cast(GetGame().GetPlayer());
		if (player == null)
			return;

		//mutual exclusion: leaving freecam's camera on would fight the spectate cam
		if (player.IsFreeCamActive())
			player.SetFreeCamActive(false);

		//NO scripted freeze command for spectate (VPPBR precedent: linked spectators
		//keep the ordinary Move command). The body LINK corrupts HumanCommandScript_
		//VPPCam (double-free in server logs) and exits could never cleanly hand
		//locomotion back. Input is held off directly instead; the link holds the body.
		HumanInputController hicEnter = player.GetInputController();
		if (hicEnter)
			hicEnter.SetDisabled(true);

		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetMission().AddActiveInputExcludes({"movement", "aiming"});
		//"menu" stays usable over the camera (admin hud / ESC); the exclude could
		//linger from a prior freecam session — clear it now and a tick later
		RemoveMenuExclude();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.RemoveMenuExclude, 100, false);

		//client-LOCAL body link to the target's Head bone: the engine composes our
		//transform in the target's animation phase, giving the camera a lag-free
		//base to ride (fixes run-jitter + 1PP phase-through). Server body untouched.
		PlayerBase adminPb = PlayerBase.Cast(player);
		if (adminPb != null)
			adminPb.VPPATAttachToSpectateTarget(targetPb);

		//engine player-camera hijack: register the creator lazily on EVERY enter
		//(freecam precedent — also wins any camera-id registration fight), park the
		//state in the class statics, THEN flip the CameraHandler flag; the engine
		//constructs the camera instance and its ctor consumes the pending state
		player.GetDayZPlayerType().RegisterCameraCreator(DayZPlayerCameras.VPPAT_SPECTATE_CAMERA, VPPDayZPlayerCameraSpectate);

		ResetSightMode();
		VPPDayZPlayerCameraSpectate.PushTarget(targetPb);
		VPPDayZPlayerCameraSpectate.PushView(m_ViewPref);
		VPPDayZPlayerCameraSpectate.PushSightMode(EVPPSpectateSightMode.NONE);
		player.SetVPPSpectateCamActive(true);

		g_Game.SetSpectateMode(true);
		m_Active = true;

		m_OnSpectateStarted.Invoke(m_TargetId);
		GetRPCManager().VSendRPC("RPC_SpectateManager", "ClientReadyAck", NULL, true);
	}

	void EndSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<int> data;
		if (!ctx.Read(data))
			return;

		TeardownLocal(data.param1);
	}

	void TargetVitals(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param2<float, float> data;
		if (!ctx.Read(data))
			return;

		m_VitalsHP    = data.param1;
		m_VitalsBlood = data.param2;
		m_OnVitals.Invoke(m_VitalsHP, m_VitalsBlood);
	}

	//edge-triggered sight-mode push from the server (EVPPSpectateSightMode)
	void TargetADS(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<int> data;
		if (!ctx.Read(data))
			return;

		if (!m_Active)
			return;

		if (VPPSpectateConstants.ADS_DEBUG)
			Print("[VPPADS] handler: TargetADS edge received mode=" + data.param1.ToString());

		m_TargetSightMode = data.param1;
		VPPDayZPlayerCameraSpectate.PushSightMode(m_TargetSightMode);

		m_OnADSChanged.Invoke(m_TargetSightMode != EVPPSpectateSightMode.NONE);
	}

	//20Hz true-look relay from the server (the target's real camera direction)
	void TargetLook(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param2<float, float> data;
		if (!ctx.Read(data))
			return;

		if (!m_Active)
			return;

		VPPDayZPlayerCameraSpectate.PushLook(data.param1, data.param2);
	}

	//---------------------------------------------------------------------------------
	// Teardown
	//---------------------------------------------------------------------------------
	protected void RemoveMenuExclude()
	{
		GetGame().GetMission().RemoveActiveInputExcludes({"menu"}, true);
		GetGame().GetMission().RefreshExcludes();
	}

	//deferred post-teardown kick: the link-corrupted freeze command cannot be
	//trusted to hand back locomotion — force a fresh Move command (vanilla uses
	//StartCommand_Move as the universal locomotion recovery)
	protected void ForceRestoreLocomotion()
	{
		DayZPlayerImplement player = DayZPlayerImplement.Cast(GetGame().GetPlayer());
		if (player == null)
			return;
		if (!player.IsAlive())
			return;

		HumanInputController hic = player.GetInputController();
		if (hic)
			hic.SetDisabled(false);

		if (player.GetCommand_Vehicle() == null)
			player.StartCommand_Move();
	}

	protected void FailsafeTeardown()
	{
		//the server never answered RequestExitSpectate: re-send it unguarded so the
		//server session (still parking the body under the target) also ends, THEN
		//clean up locally.
		GetRPCManager().VSendRPC("RPC_SpectateManager", "RequestExitSpectate", NULL, true);
		TeardownLocal(EVPPSpectateEndReason.ADMIN_REQUEST);
	}

	protected void TeardownLocal(int reason)
	{
		if (!m_Active)
			return;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.FailsafeTeardown);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.RemoveMenuExclude);

		m_Active = false;
		ResetSightMode();

		//clear the camera's optic visuals SYNCHRONOUSLY before the camera flag flips
		//so no frame renders scope PPE over the returning vanilla camera; the engine
		//destroys the camera instance itself once CameraHandler returns a vanilla id
		VPPDayZPlayerCameraSpectate.OnTeardown();

		//unlink the local body from the target BEFORE the vanilla camera resumes
		//(a still-linked body would render the returning camera at the target)
		PlayerBase ownPb = PlayerBase.Cast(GetGame().GetPlayer());
		if (ownPb != null)
			ownPb.VPPATDetachFromSpectateTarget();

		//finish the local freeze command (its OnDeactivate restores controls/excludes/cursor)
		//and drop the camera flag — the vanilla player camera resumes natively (freecam
		//exit precedent; no SelectPlayer hand-back needed in the player-camera pipeline)
		DayZPlayerImplement player = DayZPlayerImplement.Cast(GetGame().GetPlayer());
		if (player != null)
		{
			player.SetVPPSpectateCamActive(false);

			//safety only: spectate no longer starts a scripted command, but finish
			//any leftover (e.g. a lingering freecam command) so it can't hold input
			HumanCommandScript_VPPCam hcs = HumanCommandScript_VPPCam.Cast(player.GetCommand_Script());
			if (hcs)
			{
				hcs.SetFlagFinished(true);
				hcs.m_bNeedFinish = true;
			}

			HumanInputController hic = player.GetInputController();
			if (hic)
				hic.SetDisabled(false);
			//belt-and-suspenders locomotion kick (idempotent — Move ran all along)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ForceRestoreLocomotion, 300, false);
		}

		GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().RemoveActiveInputExcludes({"movement", "aiming"}, true);
		GetGame().GetMission().RefreshExcludes();

		g_Game.SetSpectateMode(false);

		string lostId = m_TargetId;
		m_TargetId = "";

		if (reason == EVPPSpectateEndReason.TARGET_DISCONNECTED || reason == EVPPSpectateEndReason.TARGET_DIED)
		{
			m_OnTargetLost.Invoke(lostId);
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_TARGET_LOST");
		}
		else
		{
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_STOPPED");
		}

		m_OnSpectateStopped.Invoke();
	}
};

/* Global getter (menu / overlay / keybind handlers) */
VPPSpectateClientHandler GetSpectateClient()
{
	return VPPSpectateClientHandler.GetInstance();
}
