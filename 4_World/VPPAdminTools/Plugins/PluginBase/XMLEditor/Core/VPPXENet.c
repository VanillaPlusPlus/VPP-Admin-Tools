// XML Editor server networking and logging.
// VPPXENet is the ONLY server file that sends on RPC_MenuXMLEditor. Every entry point refuses a null
// identity, because VSendRPC with a null identity broadcasts to every connected client.

// A queued send holds the already-serialized packet, never the Param. A Param1<ref T> kept across frames does not
// keep its object alive reliably: the queued chunk was freed before the pump sent it (empty delivery), and holding
// a second ref to it corrupted the heap on release. The caller serializes while it still owns the object.
// Serializer.Write uses the DECLARED type of the variable: write the payload from a local declared as the concrete
// Param1<ref T>. Written through a plain Param variable it serializes nothing and the client read fails.
class VPPXENetItem : Managed
{
	string FnName;
	ref ScriptRPC Rpc;

	void VPPXENetItem(string fnName, ScriptRPC rpc)
	{
		FnName = fnName;
		Rpc = rpc;
	}
};

class VPPXENetProgressState : Managed
{
	int ReqId;
	int Stage;
	int Percent;
	int SentAt;
};

// Drives the static send pump from an object: the repeating CallLater is registered on this instance's Tick, the
// instance-bound repeating form VPPXEJobQueue.Pump already proves in game (the old static repeating registration
// was the only one of its kind, and the queued sends it drained never reached the client).
class VPPXENetPump : Managed
{
	void Tick()
	{
		VPPXENet.Pump();
	}
};

class VPPXENet
{
	protected static ref map<string, ref array<ref VPPXENetItem>> s_Queues;
	protected static ref map<string, int> s_Heads;
	protected static ref map<string, int> s_WindowStart;
	protected static ref map<string, int> s_WindowCount;
	protected static ref map<string, ref VPPXENetProgressState> s_Progress;
	protected static bool s_PumpActive;
	const static int PUMP_STALL_MS = 2000;
	// TEMPORARY delivery trace for the index/issues test; set false once delivery is confirmed.
	const static bool NET_TRACE = false;
	protected static ref VPPXENetPump s_PumpObj;
	protected static int s_LastPumpMs;

	protected static void EnsureInit()
	{
		if (!s_Queues)
		{
			s_Queues = new map<string, ref array<ref VPPXENetItem>>();
		}

		if (!s_Heads)
		{
			s_Heads = new map<string, int>();
		}

		if (!s_WindowStart)
		{
			s_WindowStart = new map<string, int>();
		}

		if (!s_WindowCount)
		{
			s_WindowCount = new map<string, int>();
		}

		if (!s_Progress)
		{
			s_Progress = new map<string, ref VPPXENetProgressState>();
		}
	}

	// Starts a queued packet in the layout VSendRPC uses from the server: Param2 module/function first. The caller
	// then writes its payload (from a concretely typed Param1<ref T> local) and hands the packet to Send.
	static ScriptRPC Begin(string fnName)
	{
		ScriptRPC rpc = new ScriptRPC();
		Param2<string, string> meta = new Param2<string, string>("RPC_MenuXMLEditor", fnName);
		rpc.Write(meta);
		return rpc;
	}

	// Queued, paced send (NetChunksPer100ms per admin per 100 ms window) of a packet built with Begin.
	static void Send(PlayerIdentity to, string fnName, ScriptRPC rpc)
	{
		if (!to)
		{
			return;
		}

		EnsureInit();
		string plainId = to.GetPlainId();
		array<ref VPPXENetItem> queue = s_Queues.Get(plainId);
		if (!queue)
		{
			queue = new array<ref VPPXENetItem>();
			s_Queues.Set(plainId, queue);
			s_Heads.Set(plainId, 0);
		}

		queue.Insert(new VPPXENetItem(fnName, rpc));
		StartPump();
	}

	// Immediate send; counts toward the admin's current window.
	static void SendNow(PlayerIdentity to, string fnName, Param p)
	{
		if (!to)
		{
			return;
		}

		EnsureInit();
		CountSend(to.GetPlainId());
		GetRPCManager().VSendRPC("RPC_MenuXMLEditor", fnName, p, true, to);
	}

	static void Result(PlayerIdentity to, int reqId, bool ok, string key, string arg)
	{
		if (!to)
		{
			return;
		}

		EnsureInit();
		CountSend(to.GetPlainId());
		GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "XE_OnActionResult", new Param4<int, bool, string, string>(reqId, ok, key, arg), true, to);
	}

	// Throttled: sends only when (reqId, stage) changed, the percent reached 0 or 100, moved by at least
	// PROGRESS_STEP_PCT, or PROGRESS_INTERVAL_MS passed since the last progress to that admin.
	static void Progress(PlayerIdentity to, int reqId, int stage, int percent)
	{
		if (!to)
		{
			return;
		}

		EnsureInit();
		int pct = percent;
		if (pct < 0)
		{
			pct = 0;
		}

		if (pct > 100)
		{
			pct = 100;
		}

		string plainId = to.GetPlainId();
		int now = GetGame().GetTime();
		bool doSend = false;
		VPPXENetProgressState st = s_Progress.Get(plainId);
		if (!st)
		{
			st = new VPPXENetProgressState();
			s_Progress.Set(plainId, st);
			doSend = true;
		}
		else if (st.ReqId != reqId || st.Stage != stage)
		{
			doSend = true;
		}
		else if ((pct == 0 || pct == 100) && pct != st.Percent)
		{
			doSend = true;
		}
		else if (Math.AbsInt(pct - st.Percent) >= VPPXEConst.PROGRESS_STEP_PCT)
		{
			doSend = true;
		}
		else if (now - st.SentAt >= VPPXEConst.PROGRESS_INTERVAL_MS)
		{
			doSend = true;
		}

		if (!doSend)
		{
			return;
		}

		st.ReqId = reqId;
		st.Stage = stage;
		st.Percent = pct;
		st.SentAt = now;

		VPPXEProgress prog = new VPPXEProgress();
		prog.ReqId = reqId;
		prog.Stage = stage;
		prog.Percent = pct;
		CountSend(plainId);
		GetRPCManager().VSendRPC("RPC_MenuXMLEditor", "XE_OnProgress", new Param1<ref VPPXEProgress>(prog), true, to);
	}

	static void Cancel(string plainId)
	{
		EnsureInit();
		s_Queues.Remove(plainId);
		s_Heads.Remove(plainId);
		s_WindowStart.Remove(plainId);
		s_WindowCount.Remove(plainId);
		s_Progress.Remove(plainId);
		if (s_Queues.Count() == 0)
		{
			StopPump();
		}
	}

	static bool HasQueued(string plainId)
	{
		EnsureInit();
		return s_Queues.Contains(plainId);
	}

	protected static void CountSend(string plainId)
	{
		int now = GetGame().GetTime();
		RollWindow(plainId, now);
		int used = s_WindowCount.Get(plainId);
		s_WindowCount.Set(plainId, used + 1);
	}

	protected static void RollWindow(string plainId, int now)
	{
		if (!s_WindowStart.Contains(plainId))
		{
			s_WindowStart.Set(plainId, now);
			s_WindowCount.Set(plainId, 0);
			return;
		}

		int start = s_WindowStart.Get(plainId);
		if (now - start >= 100 || now < start)
		{
			s_WindowStart.Set(plainId, now);
			s_WindowCount.Set(plainId, 0);
		}
	}

	protected static int GetLimit()
	{
		XMLEditor editor = GetXMLEditor();
		if (editor && editor.GetSettings())
		{
			return editor.GetSettings().NetChunksPer100ms;
		}

		return 3;
	}

	// The connected identity for a plain (Steam64) id, or null when that player is not connected. Reads the
	// engine identity list, so an admin without a live body (death or respawn screen) still resolves.
	static PlayerIdentity ResolveIdentity(string plainId)
	{
		if (plainId == "")
		{
			return null;
		}

		array<PlayerIdentity> identities = new array<PlayerIdentity>();
		GetGame().GetPlayerIndentities(identities);
		foreach (PlayerIdentity identity : identities)
		{
			if (identity && identity.GetPlainId() == plainId)
			{
				return identity;
			}
		}

		return null;
	}

	// Starts the repeating send pump (bound to s_PumpObj) when it is not running. The watchdog mirrors
	// VPPXEJobQueue.EnsurePump: a pump that has not run for PUMP_STALL_MS is registered again (Remove first, so it
	// can never run twice per frame). Every Send calls this, so a dropped pump is revived by the next queued item.
	protected static void StartPump()
	{
		if (!GetGame())
		{
			return;
		}

		if (!s_PumpObj)
		{
			s_PumpObj = new VPPXENetPump();
		}

		int now = GetGame().GetTime();
		if (s_PumpActive)
		{
			if (now - s_LastPumpMs < PUMP_STALL_MS)
			{
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(s_PumpObj.Tick);
			Print("[XMLEditor][Net] send pump stalled, restarting it");
		}

		s_PumpActive = true;
		s_LastPumpMs = now;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(s_PumpObj.Tick, 0, true);
	}

	protected static void StopPump()
	{
		if (!s_PumpActive)
		{
			return;
		}

		s_PumpActive = false;
		if (s_PumpObj && GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(s_PumpObj.Tick);
		}
	}

	static void Pump()
	{
		EnsureInit();
		int limit = GetLimit();
		int now = GetGame().GetTime();
		s_LastPumpMs = now;
		array<string> dropKeys = new array<string>();
		map<string, PlayerIdentity> connected = new map<string, PlayerIdentity>();
		array<PlayerIdentity> identities = new array<PlayerIdentity>();
		GetGame().GetPlayerIndentities(identities);
		foreach (PlayerIdentity connIdentity : identities)
		{
			if (connIdentity)
			{
				connected.Set(connIdentity.GetPlainId(), connIdentity);
			}
		}

		for (int i = 0; i < s_Queues.Count(); i++)
		{
			string plainId = s_Queues.GetKey(i);
			array<ref VPPXENetItem> queue = s_Queues.GetElement(i);
			int head = s_Heads.Get(plainId);
			if (!queue || head >= queue.Count())
			{
				dropKeys.Insert(plainId);
				continue;
			}

			PlayerIdentity to = connected.Get(plainId);
			if (!to)
			{
				dropKeys.Insert(plainId);
				s_Progress.Remove(plainId);
				continue;
			}

			RollWindow(plainId, now);
			int used = s_WindowCount.Get(plainId);
			while (used < limit && head < queue.Count())
			{
				VPPXENetItem item = queue.Get(head);
				head++;
				if (item && item.Rpc)
				{
					item.Rpc.Send(null, RPCManager.VPPAT_FRAMEWORK_RPC_ID, true, to);
					used++;
					if (NET_TRACE)
					{
						int left = queue.Count() - head;
						string sentLine = "[XMLEditor][Net] sent " + item.FnName + " to " + plainId + " (" + left.ToString() + " left)";
						Print(sentLine);
					}
				}

				queue.Set(head - 1, null);
			}

			s_WindowCount.Set(plainId, used);
			s_Heads.Set(plainId, head);
			if (head >= queue.Count())
			{
				dropKeys.Insert(plainId);
			}
		}

		foreach (string dropId : dropKeys)
		{
			s_Queues.Remove(dropId);
			s_Heads.Remove(dropId);
		}

		if (s_Queues.Count() == 0)
		{
			StopPump();
		}
	}
};

class VPPXELog
{
	static void Action(PlayerIdentity who, string text, bool webhook)
	{
		string adminName = "System";
		string adminId = "";
		if (who)
		{
			adminName = who.GetName();
			adminId = who.GetPlainId();
		}

		ActionBy(adminName, adminId, text, webhook);
	}

	// Same as Action for callers that captured the admin name and id earlier (the admin may have left).
	static void ActionBy(string adminName, string adminId, string text, bool webhook)
	{
		string line = string.Format("[XMLEditor] \"%1\" (steamid=%2) %3", adminName, adminId, text);
		VPPLogManager logger = GetSimpleLogger();
		if (logger)
		{
			logger.Log(line);
		}

		if (!webhook)
		{
			return;
		}

		WebHooksManager hooks = GetWebHooksManager();
		if (hooks)
		{
			hooks.PostData(AdminActivityMessage, new AdminActivityMessage(adminId, adminName, "[XMLEditor] " + text));
		}
	}

	static void Info(string text)
	{
		Print("[XMLEditor] " + text);
	}

	// Loud: script log plus the VPP log file.
	static void Warn(string text)
	{
		string line = "[XMLEditor] WARNING: " + text;
		Print(line);
		VPPLogManager logger = GetSimpleLogger();
		if (logger)
		{
			logger.Log(line);
		}
	}
};
