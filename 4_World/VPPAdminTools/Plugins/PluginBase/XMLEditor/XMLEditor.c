// XML Editor server plugin (registered server-only by PluginManager).
// 15 client-to-server RPCs on RPC_XMLEditor, permission checks, sessions, the pending-request rule (a TYPES
// file is served only when its index is current), idle freeing, read-only mode and delegation to the
// registry, types, backups and distribution services. Replies go out only through VPPXENet.

class VPPXEPendingReq : Managed
{
	int Kind;
	string PlainId;
	int ReqId;
	string Key;
};

class XMLEditor extends PluginBase
{
	const static int PENDING_INDEX = 0;
	const static int PENDING_DETAILS = 1;
	const static int PENDING_ISSUES = 2;
	const static int UPLOAD_EXPIRY_MS = 60000;
	const static int IDLE_TICK_MS = 60000;

	protected ref VPPXESettings m_Settings;
	protected ref VPPXEFileRegistry m_Registry;
	protected ref VPPXETypesService m_Types;
	protected ref VPPXEBackupManager m_Backups;
	protected ref VPPXEDistService m_Dist;
	protected ref map<string, int> m_LastSeen;
	protected ref map<string, bool> m_OpenSessions;
	protected ref array<ref VPPXEPendingReq> m_Pending;
	protected ref map<string, ref VPPXEEditBatch> m_Uploads;
	protected ref map<string, ref VPPXEMessagesSave> m_MsgUploads;
	protected ref map<string, ref VPPXEEventsSave> m_EvtUploads;
	protected ref map<string, ref VPPXESpawnsSave> m_SpawnUploads;
	protected ref map<string, ref VPPXEGroupsSave> m_GroupUploads;
	protected ref map<string, ref VPPXESpawnablesSave> m_SpwUploads;
	// last part time of the MESSAGES and EVENTS uploads (same keys as their maps), for ExpireUploads
	protected ref map<string, int> m_SideUploadTimes;
	protected ref map<string, int> m_UploadTimes;
	protected ref map<string, int> m_UploadParts;
	protected ref map<string, int> m_RejectedUploads;
	protected ref map<string, bool> m_PushDeferred;
	protected bool m_ReadOnlyMode;
	protected bool m_Booted;
	protected int m_LastActivity;
	protected bool m_CachesFreed;
	protected bool m_Evaluating;
	protected bool m_EvalAgain;
	protected bool m_InReindexEval;
	protected bool m_TypesFailed;
	protected int m_FailedStageMask;
	protected int m_LastTypesProgPct;
	protected int m_LastTypesProgMs;

	void XMLEditor()
	{
		MakeDirectory("$profile:VPPAdminTools");
		MakeDirectory("$profile:VPPAdminTools/XMLEditor");
		MakeDirectory("$profile:VPPAdminTools/XMLEditor/Backups");
		MakeDirectory("$profile:VPPAdminTools/XMLEditor/Cache");

		m_Settings = VPPXESettings.LoadOrCreate();
		VPPXEJobQueue.Get().SetBudgetMs(m_Settings.JobBudgetMs);
		VPPXEJobQueue.Get().SetBackgroundShare(m_Settings.BackgroundSharePct);
		VPPXEJobQueue.Get().SetBoostMs(m_Settings.JobBoostMs);

		m_LastSeen = new map<string, int>();
		m_OpenSessions = new map<string, bool>();
		m_Pending = new array<ref VPPXEPendingReq>();
		m_Uploads = new map<string, ref VPPXEEditBatch>();
		m_MsgUploads = new map<string, ref VPPXEMessagesSave>();
		m_EvtUploads = new map<string, ref VPPXEEventsSave>();
		m_SpawnUploads = new map<string, ref VPPXESpawnsSave>();
		m_GroupUploads = new map<string, ref VPPXEGroupsSave>();
		m_SpwUploads = new map<string, ref VPPXESpawnablesSave>();
		m_SideUploadTimes = new map<string, int>();
		m_UploadTimes = new map<string, int>();
		m_UploadParts = new map<string, int>();
		m_RejectedUploads = new map<string, int>();
		m_PushDeferred = new map<string, bool>();
		m_LastTypesProgPct = -1;

		m_Registry = new VPPXEFileRegistry();
		m_Types = new VPPXETypesService();
		m_Backups = new VPPXEBackupManager(this);
		m_Dist = new VPPXEDistService(this);

		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_OpenSession", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_CloseSession", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetTypeIndex", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetTypeDetails", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveTypeEdits", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetIssues", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetBackups", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetBackupDiff", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_RestoreBackup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_DeleteBackups", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_PinBackup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_CreateSnapshot", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetDistribution", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_LiveScan", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_TeleportToEntity", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_DeleteLiveObjects", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_CreateTypesFile", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetMessages", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveMessages", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_RemoveCeFile", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetEvents", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveEvents", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveEventSpawns", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveEventGroups", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_GetSpawnables", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_SaveSpawnables", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_XMLEditor", "XE_TestSpawnable", this, SingleplayerExecutionType.Server);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Boot, 5000);
	}

	void ~XMLEditor()
	{
		if (!GetGame())
		{
			return;
		}

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Boot);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(IdleTick);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(PrewarmTypes);
	}

	// ---------------------------------------------------------------- accessors

	VPPXEFileRegistry GetRegistry()
	{
		return m_Registry;
	}

	VPPXETypesService GetTypes()
	{
		return m_Types;
	}

	VPPXESettings GetSettings()
	{
		return m_Settings;
	}

	VPPXEBackupManager GetBackups()
	{
		return m_Backups;
	}

	VPPXEDistService GetDist()
	{
		return m_Dist;
	}

	bool IsReadOnlyMode()
	{
		return m_ReadOnlyMode;
	}

	// ---------------------------------------------------------------- boot and idle

	void Boot()
	{
		string failure;
		if (VPPXmlDocument.SelfTest(failure))
		{
			VPPXELog.Info("SelfTest PASS");
		}
		else
		{
			m_ReadOnlyMode = true;
			VPPXELog.Warn("SelfTest FAILED (" + failure + "): type editing is disabled (no types file is EDITABLE); restoring backups and snapshots still work");
		}

		VPPXESafeWrite.ProbeEol();
		m_Registry.Build();
		VPPXESafeWrite.CheckJournal(m_Registry);
		m_Booted = true;
		m_LastActivity = GetGame().GetTime();
		if (m_Settings.PrewarmTypes)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(PrewarmTypes, 5000);
		}

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(IdleTick, IDLE_TICK_MS, true);
		if (m_OpenSessions.Count() > 0)
		{
			m_Types.EnsureIndex();
			PushSessions();
		}
	}

	void PrewarmTypes()
	{
		if (m_Types)
		{
			m_Types.Prewarm();
		}
	}

	// Every 60 s: drops expired partial uploads and, after CacheIdleMinutes without editor traffic, frees
	// the dist and types caches (all or nothing).
	void IdleTick()
	{
		int now = GetGame().GetTime();
		ExpireUploads(now);
		if (m_CachesFreed || m_Pending.Count() > 0)
		{
			return;
		}

		int idleMs = m_Settings.CacheIdleMinutes * 60000;
		if (now - m_LastActivity < idleMs)
		{
			return;
		}

		if (!VPPXEJobQueue.Get().IsIdle())
		{
			return;
		}

		m_Dist.FreeCaches();
		m_Types.FreeCaches();
		m_CachesFreed = true;
	}

	protected void ExpireUploads(int now)
	{
		array<string> expired = new array<string>();
		int count = m_UploadTimes.Count();
		for (int i = 0; i < count; i++)
		{
			int last = m_UploadTimes.GetElement(i);
			if (now - last > UPLOAD_EXPIRY_MS || now < last)
			{
				expired.Insert(m_UploadTimes.GetKey(i));
			}
		}

		foreach (string uploadKey : expired)
		{
			DropUpload(uploadKey);
			VPPXELog.Info("Dropped an incomplete edit upload (" + uploadKey + ")");
		}

		array<string> expiredSide = new array<string>();
		int sideCount = m_SideUploadTimes.Count();
		for (int s = 0; s < sideCount; s++)
		{
			int sideLast = m_SideUploadTimes.GetElement(s);
			if (now - sideLast > UPLOAD_EXPIRY_MS || now < sideLast)
			{
				expiredSide.Insert(m_SideUploadTimes.GetKey(s));
			}
		}

		foreach (string sideKey : expiredSide)
		{
			DropSideUpload(sideKey);
			VPPXELog.Info("Dropped an incomplete messages or events upload (" + sideKey + ")");
		}

		array<string> expiredRejected = new array<string>();
		int rejectedCount = m_RejectedUploads.Count();
		for (int j = 0; j < rejectedCount; j++)
		{
			int stamp = m_RejectedUploads.GetElement(j);
			if (now - stamp > UPLOAD_EXPIRY_MS || now < stamp)
			{
				expiredRejected.Insert(m_RejectedUploads.GetKey(j));
			}
		}

		foreach (string rejectedKey : expiredRejected)
		{
			m_RejectedUploads.Remove(rejectedKey);
		}
	}

	protected void DropUpload(string uploadKey)
	{
		m_Uploads.Remove(uploadKey);
		m_UploadTimes.Remove(uploadKey);
		m_UploadParts.Remove(uploadKey);
	}

	// A rejected multi-part upload: later parts of the same request are ignored silently (one error reply).
	protected void RejectUpload(string uploadKey)
	{
		DropUpload(uploadKey);
		m_RejectedUploads.Set(uploadKey, GetGame().GetTime());
	}

	protected void DropRejectedOf(string plainId)
	{
		string prefix = plainId + "|";
		array<string> ownKeys = new array<string>();
		int count = m_RejectedUploads.Count();
		for (int i = 0; i < count; i++)
		{
			string rejectedKey = m_RejectedUploads.GetKey(i);
			if (rejectedKey.IndexOf(prefix) == 0)
			{
				ownKeys.Insert(rejectedKey);
			}
		}

		foreach (string ownKey : ownKeys)
		{
			m_RejectedUploads.Remove(ownKey);
		}
	}

	protected void DropUploadsOf(string plainId)
	{
		string prefix = plainId + "|";
		array<string> ownKeys = new array<string>();
		int count = m_Uploads.Count();
		for (int i = 0; i < count; i++)
		{
			string uploadKey = m_Uploads.GetKey(i);
			if (uploadKey.IndexOf(prefix) == 0)
			{
				ownKeys.Insert(uploadKey);
			}
		}

		foreach (string ownKey : ownKeys)
		{
			DropUpload(ownKey);
		}
	}

	protected void MarkActivity()
	{
		m_LastActivity = GetGame().GetTime();
		m_CachesFreed = false;
	}

	// ---------------------------------------------------------------- permissions

	protected bool CheckPerm(PlayerIdentity sender, string perm, int reqId)
	{
		string plainId = sender.GetPlainId();
		PermissionManager pm = GetPermissionManager();
		bool ok = false;
		if (pm)
		{
			ok = pm.VerifyPermission(plainId, "MenuXMLEditor", "", false);
			if (ok && perm != "")
			{
				ok = pm.VerifyPermission(plainId, perm, "", false);
			}
		}

		if (!ok)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_PERMISSION", "");
			string denied = "MenuXMLEditor";
			if (perm != "")
			{
				denied = perm;
			}

			VPPXELog.Action(sender, "was denied " + denied, false);
			return false;
		}

		m_LastSeen.Set(plainId, GetGame().GetTime());
		MarkActivity();
		return true;
	}

	// Silent view check for server-initiated pushes: a revoked admin stops receiving session data at once.
	protected bool HasViewPermission(string plainId)
	{
		PermissionManager pm = GetPermissionManager();
		if (!pm)
		{
			return false;
		}

		return pm.VerifyPermission(plainId, "MenuXMLEditor", "", false);
	}

	protected bool HasPerm(PermissionManager pm, string plainId, string perm)
	{
		if (!pm)
		{
			return false;
		}

		return pm.VerifyPermission(plainId, perm, "", false);
	}

	protected int BuildPermMask(string plainId)
	{
		PermissionManager pm = GetPermissionManager();
		int mask = 0;
		if (HasPerm(pm, plainId, "MenuXMLEditor:EditTypes"))
		{
			mask = mask | VPPXEPerm.EDIT_TYPES;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:AddRemoveTypes"))
		{
			mask = mask | VPPXEPerm.ADD_REMOVE;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:DistributionMap"))
		{
			mask = mask | VPPXEPerm.DIST_MAP;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:ViewBackups"))
		{
			mask = mask | VPPXEPerm.VIEW_BACKUPS;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:RestoreBackup"))
		{
			mask = mask | VPPXEPerm.RESTORE;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:DeleteBackup"))
		{
			mask = mask | VPPXEPerm.DELETE_BACKUPS;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:LiveScan"))
		{
			mask = mask | VPPXEPerm.LIVE_SCAN;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:DeleteObjects"))
		{
			mask = mask | VPPXEPerm.DELETE_OBJECTS;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:EditMessages"))
		{
			mask = mask | VPPXEPerm.EDIT_MESSAGES;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:EditEvents"))
		{
			mask = mask | VPPXEPerm.EDIT_EVENTS;
		}

		if (HasPerm(pm, plainId, "MenuXMLEditor:EditSpawnables"))
		{
			mask = mask | VPPXEPerm.EDIT_SPAWNABLES;
		}

		if (HasPerm(pm, plainId, "TeleportManager:TPPlayers") && HasPerm(pm, plainId, "TeleportManager:TPSelf"))
		{
			mask = mask | VPPXEPerm.TELEPORT;
		}

		return mask;
	}

	protected PlayerIdentity ResolveIdentity(string plainId)
	{
		return VPPXENet.ResolveIdentity(plainId);
	}

	// Handlers other than open/close wait for the boot (registry) to finish.
	protected bool CheckBooted(PlayerIdentity sender, int reqId)
	{
		if (m_Booted)
		{
			return true;
		}

		VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
		return false;
	}

	// ---------------------------------------------------------------- sessions

	protected VPPXESessionInfo BuildSessionInfo(string plainId)
	{
		VPPXESessionInfo info = new VPPXESessionInfo();
		info.Protocol = VPPXEConst.PROTOCOL;
		info.PermMask = BuildPermMask(plainId);
		info.RegistryRev = m_Registry.GetRev();
		info.WorldSize = GetGame().GetWorld().GetWorldSize();
		info.WorldName = GetGame().GetWorldName();
		info.BackupMaxPerFile = m_Settings.BackupMaxPerFile;
		info.BackupMaxAgeDays = m_Settings.BackupMaxAgeDays;
		info.BackupMaxTotalMB = m_Settings.BackupMaxTotalMB;
		info.IndexReady = m_Types.IsReady();
		info.EolMode = VPPXESafeWrite.GetEolMode();
		int count = m_Registry.Count();
		for (int i = 0; i < count; i++)
		{
			VPPXEFileEntry entry = m_Registry.At(i);
			VPPXEFileInfo fileInfo = new VPPXEFileInfo();
			fileInfo.Key = entry.Key;
			fileInfo.Label = "";
			fileInfo.Kind = entry.Kind;
			fileInfo.LoadOrder = entry.LoadOrder;
			fileInfo.Flags = entry.Flags;
			fileInfo.Revision = entry.Revision;
			fileInfo.EntryCount = 0;
			if (entry.Kind == VPPXEFileKind.TYPES)
			{
				fileInfo.EntryCount = m_Types.GetEntryCount(entry.Key);
			}

			fileInfo.IssueCount = m_Registry.GetIssueCount(entry.Key) + m_Types.GetIssueCount(entry.Key);
			info.Files.Insert(fileInfo);
		}

		VPPXELimits limits = m_Types.GetLimits();
		if (limits)
		{
			info.Limits = limits;
		}

		return info;
	}

	// XE_OnSession to every open session seen within CacheIdleMinutes (cached data only).
	void PushSessions()
	{
		int now = GetGame().GetTime();
		int idleMs = m_Settings.CacheIdleMinutes * 60000;
		int count = m_OpenSessions.Count();
		for (int i = 0; i < count; i++)
		{
			string plainId = m_OpenSessions.GetKey(i);
			if (m_PushDeferred.Contains(plainId) || !m_LastSeen.Contains(plainId))
			{
				continue;
			}

			int seen = m_LastSeen.Get(plainId);
			if (now - seen > idleMs)
			{
				continue;
			}

			if (!HasViewPermission(plainId))
			{
				continue;
			}

			PlayerIdentity identity = ResolveIdentity(plainId);
			if (!identity)
			{
				continue;
			}

			VPPXESessionInfo info = BuildSessionInfo(plainId);
			VPPXENet.SendNow(identity, "XE_OnSession", new Param1<ref VPPXESessionInfo>(info));
		}
	}

	// Session push queued BEHIND the index chunks of a served request, so the client applies the rows first.
	void QueueSessionPush(string plainId)
	{
		PlayerIdentity identity = ResolveIdentity(plainId);
		if (!identity || !m_OpenSessions.Contains(plainId))
		{
			return;
		}

		if (!HasViewPermission(plainId))
		{
			return;
		}

		VPPXESessionInfo info = BuildSessionInfo(plainId);
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnSession");
		Param1<ref VPPXESessionInfo> netPayload = new Param1<ref VPPXESessionInfo>(info);
		netRpc.Write(netPayload);
		VPPXENet.Send(identity, "XE_OnSession", netRpc);
	}

	// ---------------------------------------------------------------- service callbacks

	// VPPXEDistService: a LIGHT/MAP/CLUSTERS stage finished.
	void OnIndexStageReady(int stage, bool ok)
	{
		if (!ok)
		{
			m_FailedStageMask = m_FailedStageMask | (1 << stage);
		}

		EvaluatePending();

		// every open session re-checks its waits (index needs, timed-out tabs) now that a stage is ready
		if (ok)
		{
			PushSessions();
		}
	}

	// VPPXETypesService: an index job ended (ok or failed); pending requests first, then the session push.
	void OnTypesReindexed()
	{
		if (m_Types.ConsumeJobFailed())
		{
			m_TypesFailed = true;
		}

		m_InReindexEval = true;
		EvaluatePending();
		m_InReindexEval = false;
		PushSessions();
		m_PushDeferred.Clear();
	}

	// After our own write: registry refresh, dist invalidation; TYPES files are reindexed and the session push
	// follows the reindex (OnTypesReindexed); any other kind pushes now.
	void OnFileWritten(string fileKey, PlayerIdentity who)
	{
		// A verified write already noted the new revision, so Refresh sees no change for our own write of
		// cfgeconomycore.xml or cfgenvironment.xml: the file list is rebuilt here (CREATE tab, restores).
		VPPXEFileEntry written = m_Registry.Find(fileKey);
		if (written && VPPXEFileRegistry.NeedsRebuild(written.Kind))
		{
			m_Registry.Rebuild();
		}
		else
		{
			m_Registry.Refresh(fileKey);
		}

		m_Dist.OnFilesChanged();
		VPPXEFileEntry entry = m_Registry.Find(fileKey);
		if (entry && entry.Kind == VPPXEFileKind.TYPES)
		{
			m_Types.ReindexFile(fileKey);
			return;
		}

		PushSessions();
	}

	// VPPXEFileRegistry: the file list was rebuilt (file indexes changed): the types index starts over.
	void OnRegistryRebuilt()
	{
		m_Types.FreeCaches();
		m_Types.EnsureIndex();
		m_Dist.OnFilesChanged();
	}

	// Progress to every request waiting for the types index, XMLEditor and distribution queries alike. The index job
	// calls this every frame it yields; the per-admin throttle in VPPXENet breaks when several ReqIds alternate, so
	// it is throttled here first: a step of PROGRESS_STEP_PCT, 100, a restart, or once a second.
	void NotifyTypesProgress(int pct)
	{
		int now = GetGame().GetTime();
		int progDelta = pct - m_LastTypesProgPct;
		bool bigStep = m_LastTypesProgPct < 0 || (pct == 100 && m_LastTypesProgPct != 100) || pct < m_LastTypesProgPct || progDelta >= VPPXEConst.PROGRESS_STEP_PCT;
		if (!bigStep && now - m_LastTypesProgMs < 1000)
		{
			return;
		}

		m_LastTypesProgPct = pct;
		m_LastTypesProgMs = now;
		foreach (VPPXEPendingReq req : m_Pending)
		{
			if (TypesReadyFor(req))
			{
				continue;
			}

			PlayerIdentity identity = ResolveIdentity(req.PlainId);
			if (identity)
			{
				VPPXENet.Progress(identity, req.ReqId, VPPXEStage.TYPES, pct);
			}
		}

		if (m_Dist)
		{
			m_Dist.NotifyTypesProgress(pct);
		}
	}

	// ---------------------------------------------------------------- pending requests

	protected void AddPending(int kind, PlayerIdentity sender, int reqId, string key)
	{
		string plainId = sender.GetPlainId();
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			VPPXEPendingReq prevReq = m_Pending.Get(i);
			if (prevReq.PlainId == plainId && prevReq.Kind == kind && (kind != PENDING_INDEX || prevReq.Key == key))
			{
				m_Pending.RemoveOrdered(i);
			}
		}

		VPPXEPendingReq req = new VPPXEPendingReq();
		req.Kind = kind;
		req.PlainId = plainId;
		req.ReqId = reqId;
		req.Key = key;
		m_Pending.Insert(req);
		EvaluatePending();
	}

	protected void DropPendingOf(string plainId)
	{
		for (int i = m_Pending.Count() - 1; i >= 0; i--)
		{
			if (m_Pending.Get(i).PlainId == plainId)
			{
				m_Pending.RemoveOrdered(i);
			}
		}
	}

	// Serves every entry whose preconditions hold, fails entries whose stage or reindex failed, drops entries
	// whose sender left, and kicks the missing work for the rest. Re-entrant calls are folded into a re-run.
	protected void EvaluatePending()
	{
		if (m_Evaluating)
		{
			m_EvalAgain = true;
			return;
		}

		m_Evaluating = true;
		m_EvalAgain = true;
		int guard = 0;
		while (m_EvalAgain && guard < 8)
		{
			m_EvalAgain = false;
			guard++;
			bool typesFailed = m_TypesFailed;
			int failedMask = m_FailedStageMask;
			m_TypesFailed = false;
			m_FailedStageMask = 0;
			array<ref VPPXEPendingReq> current = m_Pending;
			m_Pending = new array<ref VPPXEPendingReq>();
			foreach (VPPXEPendingReq req : current)
			{
				PlayerIdentity identity = ResolveIdentity(req.PlainId);
				if (!identity)
				{
					continue;
				}

				if (FailIfNeeded(req, identity, typesFailed, failedMask))
				{
					continue;
				}

				if (TryServe(req, identity))
				{
					continue;
				}

				m_Pending.Insert(req);
			}
		}

		m_Evaluating = false;
	}

	protected bool TypesReadyFor(VPPXEPendingReq req)
	{
		if (!m_Types.IsReady())
		{
			return false;
		}

		if (req.Kind == PENDING_INDEX && req.Key != "")
		{
			return m_Types.IsFileCurrent(req.Key);
		}

		return m_Types.AllFilesCurrent();
	}

	protected bool FailIfNeeded(VPPXEPendingReq req, PlayerIdentity identity, bool typesFailed, int failedMask)
	{
		bool fail = false;
		if (typesFailed && !TypesReadyFor(req))
		{
			fail = true;
		}

		int lightBit = 1 << VPPXEIndexStage.LIGHT;
		int mapBit = 1 << VPPXEIndexStage.MAP;
		if (req.Kind != PENDING_INDEX && (failedMask & lightBit) != 0 && !m_Dist.IsReady(VPPXEIndexStage.LIGHT))
		{
			fail = true;
		}

		if (req.Kind == PENDING_ISSUES && (failedMask & mapBit) != 0 && !m_Dist.IsReady(VPPXEIndexStage.MAP))
		{
			fail = true;
		}

		if (fail)
		{
			VPPXENet.Result(identity, req.ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
		}

		return fail;
	}

	// True when the entry was served (it leaves the list); otherwise the missing work is started.
	protected bool TryServe(VPPXEPendingReq req, PlayerIdentity identity)
	{
		bool typesOk = TypesReadyFor(req);
		bool lightOk = true;
		bool mapOk = true;
		if (req.Kind != PENDING_INDEX)
		{
			lightOk = m_Dist.IsReady(VPPXEIndexStage.LIGHT);
		}

		if (req.Kind == PENDING_ISSUES)
		{
			mapOk = m_Dist.IsReady(VPPXEIndexStage.MAP);
		}

		if (typesOk && lightOk && mapOk)
		{
			Serve(req, identity);
			return true;
		}

		if (!typesOk)
		{
			KickTypes(req);
			VPPXENet.Progress(identity, req.ReqId, VPPXEStage.TYPES, m_Types.GetProgressPct());
		}

		if (!lightOk)
		{
			m_Dist.EnsureStage(VPPXEIndexStage.LIGHT, identity, req.ReqId);
			if (typesOk)
			{
				VPPXENet.Progress(identity, req.ReqId, VPPXEStage.SPAWN_LIGHT, 0);
			}
		}
		else if (!mapOk)
		{
			m_Dist.EnsureStage(VPPXEIndexStage.MAP, identity, req.ReqId);
			if (typesOk)
			{
				VPPXENet.Progress(identity, req.ReqId, VPPXEStage.PROTO, 0);
			}
		}

		return false;
	}

	// Not ready: build the index. Ready but not current: reindex (coalesced) every stale TYPES file needed.
	protected void KickTypes(VPPXEPendingReq req)
	{
		if (!m_Types.IsReady())
		{
			m_Types.EnsureIndex();
			return;
		}

		if (req.Kind == PENDING_INDEX && req.Key != "")
		{
			if (m_Types.IsFileStale(req.Key))
			{
				m_Types.ReindexFile(req.Key);
			}

			return;
		}

		array<VPPXEFileEntry> typesFiles = new array<VPPXEFileEntry>();
		m_Registry.GetByKind(VPPXEFileKind.TYPES, typesFiles);
		foreach (VPPXEFileEntry entry : typesFiles)
		{
			if (m_Types.IsFileStale(entry.Key))
			{
				m_Types.ReindexFile(entry.Key);
			}
		}
	}

	protected void Serve(VPPXEPendingReq req, PlayerIdentity identity)
	{
		if (req.Kind == PENDING_INDEX)
		{
			VPPXEIndexSendJob sendJob = new VPPXEIndexSendJob(this, req.PlainId, req.ReqId, req.Key);
			if (m_InReindexEval)
			{
				sendJob.SetPushSessionAfter(true);
				m_PushDeferred.Set(req.PlainId, true);
			}

			VPPXEJobQueue.Get().Enqueue(sendJob, VPPXELane.INTERACTIVE);
			return;
		}

		if (req.Kind == PENDING_DETAILS)
		{
			SendDetails(identity, req.ReqId, req.Key);
			return;
		}

		VPPXEIssuesJob issuesJob = new VPPXEIssuesJob(this, req.PlainId, req.ReqId, req.Key);
		VPPXEJobQueue.Get().Enqueue(issuesJob, VPPXELane.INTERACTIVE);
	}

	// O(defs of the name); every optional list stops growing at CHUNK_BYTES of estimated payload.
	protected void SendDetails(PlayerIdentity identity, int reqId, string typeName)
	{
		VPPXETypeDetails d = new VPPXETypeDetails();
		d.ReqId = reqId;
		d.Name = typeName;
		d.ConfigState = m_Types.ConfigStateOf(typeName);
		d.Ignored = m_Types.IsIgnored(typeName);
		int budget = VPPXEConst.CHUNK_BYTES;
		int est = 64 + typeName.Length();
		est = m_Types.FillDetailsCore(d, typeName, est);
		est = m_Types.FillDetailsIssues(d, typeName, est, budget);
		est = m_Types.FillDetailsUnknownRefs(d, typeName, est, budget);
		est = FillSpawnable(d, typeName, est, budget);
		est = m_Types.FillDetailsRawBlock(d, typeName, est, budget);
		VPPXENet.SendNow(identity, "XE_OnTypeDetails", new Param1<ref VPPXETypeDetails>(d));
	}

	protected int FillSpawnable(VPPXETypeDetails d, string typeName, int est, int budget)
	{
		VPPXESpawnableInfo info = new VPPXESpawnableInfo();
		m_Dist.FillSpawnable(typeName, info);
		VPPXESpawnableInfo outInfo = d.Spawnable;
		if (!outInfo)
		{
			outInfo = new VPPXESpawnableInfo();
			d.Spawnable = outInfo;
		}

		outInfo.Found = info.Found;
		outInfo.Hoarder = info.Hoarder;
		outInfo.HasDamage = info.HasDamage;
		outInfo.DamageMin = info.DamageMin;
		outInfo.DamageMax = info.DamageMax;
		int total = est + 24;
		if (info.Lines)
		{
			foreach (string spawnLine : info.Lines)
			{
				string clipped = VPPXmlText.Clip(spawnLine, VPPXEConst.MAX_CELL_CHARS);
				int lineSize = clipped.Length() + 4;
				if (total + lineSize > budget)
				{
					break;
				}

				outInfo.Lines.Insert(clipped);
				total += lineSize;
			}
		}

		if (info.Parents)
		{
			foreach (string parentName : info.Parents)
			{
				int parentSize = parentName.Length() + 4;
				if (outInfo.Parents.Count() >= 50 || total + parentSize > budget)
				{
					break;
				}

				outInfo.Parents.Insert(parentName);
				total += parentSize;
			}
		}

		return total;
	}

	// ---------------------------------------------------------------- RPC handlers (client -> server)

	void XE_OpenSession(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		if (!CheckPerm(sender, "", 0))
		{
			return;
		}

		if (data.param1 != VPPXEConst.PROTOCOL)
		{
			VPPXENet.Result(sender, 0, false, "#VSTR_XMLE_ERR_PROTOCOL", "");
			return;
		}

		string plainId = sender.GetPlainId();
		m_OpenSessions.Set(plainId, true);
		DropRejectedOf(plainId);
		VPPXESessionInfo info = BuildSessionInfo(plainId);
		VPPXENet.SendNow(sender, "XE_OnSession", new Param1<ref VPPXESessionInfo>(info));
		if (m_Booted)
		{
			m_Registry.EnqueueRefresh();
			m_Types.EnsureIndex();
		}

		VPPXELog.Action(sender, "opened an XML Editor session", false);
	}

	void XE_CloseSession(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		if (!CheckPerm(sender, "", 0))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		m_OpenSessions.Remove(plainId);
		m_LastSeen.Remove(plainId);
		DropPendingOf(plainId);
		DropMessageUploadsOf(plainId);
		DropEventUploadsOf(plainId);
		if (m_Dist)
		{
			m_Dist.DropPendingOf(plainId);
		}

		DropRejectedOf(plainId);
		DropUploadsOf(plainId);
		VPPXENet.Cancel(plainId);
		VPPXELog.Action(sender, "closed the XML Editor session", false);
	}

	void XE_GetTypeIndex(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		string fileKey = data.param2;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		if (fileKey != "")
		{
			VPPXEFileEntry entry = m_Registry.Find(fileKey);
			if (!entry || entry.Kind != VPPXEFileKind.TYPES)
			{
				VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
				return;
			}
		}

		AddPending(PENDING_INDEX, sender, reqId, fileKey);
	}

	void XE_GetTypeDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		string typeName = data.param2;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		if (typeName == "" || typeName.Length() > VPPXEConst.MAX_NAME_LENGTH)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(typeName, VPPXEConst.MAX_NAME_LENGTH));
			return;
		}

		AddPending(PENDING_DETAILS, sender, reqId, typeName);
	}

	void XE_SaveTypeEdits(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXEEditBatch> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		VPPXEEditBatch part = data.param1;
		if (!part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|" + reqId.ToString();
		if (m_RejectedUploads.Contains(uploadKey))
		{
			m_RejectedUploads.Set(uploadKey, GetGame().GetTime());
			return;
		}

		if (!CheckPerm(sender, "MenuXMLEditor:EditTypes", reqId) || !CheckBooted(sender, reqId))
		{
			RejectUpload(uploadKey);
			return;
		}

		bool structural = false;
		if (part.Edits)
		{
			foreach (VPPXETypeEdit probe : part.Edits)
			{
				if (probe && probe.Op != VPPXEOp.UPDATE)
				{
					structural = true;
					break;
				}
			}
		}

		if (structural && !CheckPerm(sender, "MenuXMLEditor:AddRemoveTypes", reqId))
		{
			RejectUpload(uploadKey);
			return;
		}

		// A first part over a stored partial upload is a new request: the client's request counter restarted after a
		// reconnect and reused the id of an upload that never completed (the client never sends part 0 twice).
		VPPXEEditBatch acc = m_Uploads.Get(uploadKey);
		if (acc && part.PartIdx == 0)
		{
			DropUpload(uploadKey);
			acc = null;
		}

		if (!acc)
		{
			if (part.PartCount < 1 || part.PartCount > VPPXEConst.MAX_EDITS_PER_BATCH || part.PartIdx != 0)
			{
				RejectUpload(uploadKey);
				VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", part.FileKey);
				return;
			}

			acc = new VPPXEEditBatch();
			acc.ReqId = reqId;
			acc.FileKey = part.FileKey;
			acc.BaseRevision = part.BaseRevision;
			acc.Note = part.Note;
			acc.PartIdx = 0;
			acc.PartCount = part.PartCount;
			acc.Reason = part.Reason;
			m_Uploads.Set(uploadKey, acc);
			m_UploadParts.Set(uploadKey, 0);
		}
		else if (acc.PartCount != part.PartCount || acc.FileKey != part.FileKey || part.PartIdx != m_UploadParts.Get(uploadKey))
		{
			RejectUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", part.FileKey);
			return;
		}

		if (part.Edits)
		{
			if (acc.Edits.Count() + part.Edits.Count() > VPPXEConst.MAX_EDITS_PER_BATCH)
			{
				RejectUpload(uploadKey);
				VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", part.FileKey);
				return;
			}

			foreach (VPPXETypeEdit edit : part.Edits)
			{
				acc.Edits.Insert(edit);
			}
		}

		int received = m_UploadParts.Get(uploadKey) + 1;
		m_UploadParts.Set(uploadKey, received);
		m_UploadTimes.Set(uploadKey, GetGame().GetTime());
		if (received < acc.PartCount)
		{
			return;
		}

		DropUpload(uploadKey);
		VPPXESaveJob job = new VPPXESaveJob(this, sender, acc);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
		VPPXELog.Action(sender, string.Format("queued a save of %1 edit(s) to %2", acc.Edits.Count(), acc.FileKey), false);
	}

	void XE_GetIssues(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		string fileKey = data.param2;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		if (fileKey != "" && !m_Registry.Find(fileKey))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		AddPending(PENDING_ISSUES, sender, reqId, fileKey);
	}

	void XE_GetBackups(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:ViewBackups", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Backups.HandleList(sender, reqId, data.param2);
	}

	void XE_GetBackupDiff(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, string, int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:ViewBackups", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Backups.HandleDiff(sender, reqId, data.param2, data.param3);
	}

	void XE_RestoreBackup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:RestoreBackup", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Backups.HandleRestore(sender, reqId, data.param2);
	}

	void XE_DeleteBackups(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, ref array<string>> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:DeleteBackup", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		array<string> backupIds = data.param2;
		if (!backupIds)
		{
			backupIds = new array<string>();
		}

		m_Backups.HandleDelete(sender, reqId, backupIds);
	}

	void XE_PinBackup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, string, bool> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:DeleteBackup", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Backups.HandlePin(sender, reqId, data.param2, data.param3);
	}

	void XE_CreateSnapshot(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, string, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:EditTypes", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Backups.HandleSnapshot(sender, reqId, data.param2, data.param3);
	}

	// CREATE tab: a new custom types or messages file (folder, file name, kind) registered in cfgeconomycore.xml.
	// Types files need AddRemoveTypes, messages files EditMessages; refused in read-only mode.
	void XE_CreateTypesFile(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param4<int, string, string, int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		int kind = data.param4;
		if (!VPPXECreateRules.IsCreatableKind(kind))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		string createPerm = "MenuXMLEditor:AddRemoveTypes";
		if (kind == VPPXEFileKind.MESSAGES)
		{
			createPerm = "MenuXMLEditor:EditMessages";
		}
		else if (kind == VPPXEFileKind.EVENTS)
		{
			createPerm = "MenuXMLEditor:EditEvents";
		}
		else if (kind == VPPXEFileKind.SPAWNABLETYPES || kind == VPPXEFileKind.RANDOMPRESETS)
		{
			createPerm = "MenuXMLEditor:EditSpawnables";
		}

		if (!CheckPerm(sender, createPerm, reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		if (m_ReadOnlyMode)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_READONLY_FILE", "");
			return;
		}

		VPPXECreateFileJob job = new VPPXECreateFileJob(sender, reqId, data.param2, data.param3, kind);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	// FILES tab: unregister a <ce> file from cfgeconomycore.xml (the file itself stays; see VPPXERemoveCeFileJob).
	// Messages files need EditMessages, every other kind AddRemoveTypes, like the CREATE tab.
	void XE_RemoveCeFile(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		string fileKey = data.param2;
		string removePerm = "MenuXMLEditor:AddRemoveTypes";
		VPPXEFileEntry removeEntry = m_Registry.Find(fileKey);
		if (removeEntry && removeEntry.Kind == VPPXEFileKind.MESSAGES)
		{
			removePerm = "MenuXMLEditor:EditMessages";
		}
		else if (removeEntry && removeEntry.Kind == VPPXEFileKind.EVENTS)
		{
			removePerm = "MenuXMLEditor:EditEvents";
		}
		else if (removeEntry && (removeEntry.Kind == VPPXEFileKind.SPAWNABLETYPES || removeEntry.Kind == VPPXEFileKind.RANDOMPRESETS))
		{
			removePerm = "MenuXMLEditor:EditSpawnables";
		}

		if (!CheckPerm(sender, removePerm, reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		VPPXERemoveCeFileJob job = new VPPXERemoveCeFileJob(sender, reqId, fileKey);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	// MESSAGES tab: every messages file (db/messages.xml and the <ce type="messages"> files) as message rows.
	void XE_GetMessages(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		VPPXEMessagesListJob job = new VPPXEMessagesListJob(sender, reqId);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	// MESSAGES tab save: parts of VPPXEMessageRules.EDITS_PER_PART edits, assembled per admin and request; the last
	// part queues the save job.
	void XE_SaveMessages(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXEMessagesSave> data;
		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEMessagesSave part = data.param1;
		if (!sender || !part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|" + reqId.ToString();
		if (part.PartIdx == 0)
		{
			if (!CheckPerm(sender, "MenuXMLEditor:EditMessages", reqId) || !CheckBooted(sender, reqId))
			{
				return;
			}

			// a new save of this admin replaces any unfinished one
			DropMessageUploadsOf(plainId);
			VPPXEMessagesSave started = new VPPXEMessagesSave();
			started.ReqId = reqId;
			started.PartIdx = 0;
			started.PartCount = part.PartCount;
			started.FileKey = part.FileKey;
			started.BaseRevision = part.BaseRevision;
			m_MsgUploads.Set(uploadKey, started);
		}

		m_SideUploadTimes.Set(uploadKey, GetGame().GetTime());

		// no upload = part 0 was refused (already answered) or never came: later parts are dropped quietly
		VPPXEMessagesSave acc = m_MsgUploads.Get(uploadKey);
		if (!acc)
		{
			return;
		}

		if (part.PartIdx != acc.PartIdx || part.PartCount != acc.PartCount || part.PartCount < 1 || !part.Edits)
		{
			m_MsgUploads.Remove(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		foreach (VPPXEMessageEdit partEdit : part.Edits)
		{
			acc.Edits.Insert(partEdit);
		}

		if (acc.Edits.Count() > VPPXEMessageRules.MAX_ROWS * 2)
		{
			m_MsgUploads.Remove(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		acc.PartIdx = acc.PartIdx + 1;
		if (acc.PartIdx < acc.PartCount)
		{
			return;
		}

		m_MsgUploads.Remove(uploadKey);
		VPPXEMessagesSaveJob saveJob = new VPPXEMessagesSaveJob(sender, acc);
		VPPXEJobQueue.Get().Enqueue(saveJob, VPPXELane.INTERACTIVE);
	}

	// EVENTS tab: every events file (db/events.xml and the <ce type="events"> files) as event rows, preceded by the
	// cfgeventspawns.xml summary the checks need.
	void XE_GetEvents(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		VPPXEEventsListJob job = new VPPXEEventsListJob(sender, reqId);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	// EVENTS tab save: parts of VPPXEEventRules.EDITS_PER_PART edits, assembled per admin and request; the last part
	// queues the save job.
	void XE_SaveEvents(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXEEventsSave> data;
		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEEventsSave part = data.param1;
		if (!sender || !part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|e" + reqId.ToString();
		if (part.PartIdx == 0)
		{
			if (!CheckPerm(sender, "MenuXMLEditor:EditEvents", reqId) || !CheckBooted(sender, reqId))
			{
				return;
			}

			// a new save of this admin replaces any unfinished one
			DropUploadsWithPrefix(plainId + "|e");
			VPPXEEventsSave started = new VPPXEEventsSave();
			started.ReqId = reqId;
			started.PartIdx = 0;
			started.PartCount = part.PartCount;
			started.FileKey = part.FileKey;
			started.BaseRevision = part.BaseRevision;
			m_EvtUploads.Set(uploadKey, started);
		}

		// no upload = part 0 was refused (already answered) or never came: later parts are dropped quietly
		VPPXEEventsSave acc = m_EvtUploads.Get(uploadKey);
		if (!acc)
		{
			return;
		}

		m_SideUploadTimes.Set(uploadKey, GetGame().GetTime());
		if (part.PartIdx != acc.PartIdx || part.PartCount != acc.PartCount || part.PartCount < 1 || !part.Edits)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		foreach (VPPXEEventEdit partEdit : part.Edits)
		{
			acc.Edits.Insert(partEdit);
		}

		if (acc.Edits.Count() > VPPXEEventRules.MAX_ROWS * 2)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		acc.PartIdx = acc.PartIdx + 1;
		if (acc.PartIdx < acc.PartCount)
		{
			return;
		}

		DropSideUpload(uploadKey);
		VPPXEEventsSaveJob saveJob = new VPPXEEventsSaveJob(sender, acc);
		VPPXEJobQueue.Get().Enqueue(saveJob, VPPXELane.INTERACTIVE);
	}

	// EVENTS tab: cfgeventspawns.xml edits, parts of VPPXESpawnRules.EDITS_PER_PART, like XE_SaveEvents.
	void XE_SaveEventSpawns(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXESpawnsSave> data;
		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESpawnsSave part = data.param1;
		if (!sender || !part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|s" + reqId.ToString();
		if (part.PartIdx == 0)
		{
			if (!CheckPerm(sender, "MenuXMLEditor:EditEvents", reqId) || !CheckBooted(sender, reqId))
			{
				return;
			}

			// a new spawns save of this admin replaces any unfinished one
			DropUploadsWithPrefix(plainId + "|s");
			VPPXESpawnsSave started = new VPPXESpawnsSave();
			started.ReqId = reqId;
			started.PartIdx = 0;
			started.PartCount = part.PartCount;
			started.FileKey = part.FileKey;
			started.BaseRevision = part.BaseRevision;
			m_SpawnUploads.Set(uploadKey, started);
		}

		VPPXESpawnsSave acc = m_SpawnUploads.Get(uploadKey);
		if (!acc)
		{
			return;
		}

		m_SideUploadTimes.Set(uploadKey, GetGame().GetTime());
		if (part.PartIdx != acc.PartIdx || part.PartCount != acc.PartCount || part.PartCount < 1 || !part.Edits)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		foreach (VPPXESpawnEdit partEdit : part.Edits)
		{
			acc.Edits.Insert(partEdit);
		}

		if (acc.Edits.Count() > VPPXESpawnRules.MAX_ROWS * 2)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		acc.PartIdx = acc.PartIdx + 1;
		if (acc.PartIdx < acc.PartCount)
		{
			return;
		}

		DropSideUpload(uploadKey);
		VPPXESpawnsSaveJob saveJob = new VPPXESpawnsSaveJob(sender, acc);
		VPPXEJobQueue.Get().Enqueue(saveJob, VPPXELane.INTERACTIVE);
	}

	// EVENTS tab: cfgeventgroups.xml edits, parts of VPPXEGroupRules.EDITS_PER_PART, like XE_SaveEventSpawns.
	void XE_SaveEventGroups(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXEGroupsSave> data;
		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEGroupsSave part = data.param1;
		if (!sender || !part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|g" + reqId.ToString();
		if (part.PartIdx == 0)
		{
			if (!CheckPerm(sender, "MenuXMLEditor:EditEvents", reqId) || !CheckBooted(sender, reqId))
			{
				return;
			}

			// a new groups save of this admin replaces any unfinished one
			DropUploadsWithPrefix(plainId + "|g");
			VPPXEGroupsSave started = new VPPXEGroupsSave();
			started.ReqId = reqId;
			started.PartIdx = 0;
			started.PartCount = part.PartCount;
			started.FileKey = part.FileKey;
			started.BaseRevision = part.BaseRevision;
			m_GroupUploads.Set(uploadKey, started);
		}

		VPPXEGroupsSave acc = m_GroupUploads.Get(uploadKey);
		if (!acc)
		{
			return;
		}

		m_SideUploadTimes.Set(uploadKey, GetGame().GetTime());
		if (part.PartIdx != acc.PartIdx || part.PartCount != acc.PartCount || part.PartCount < 1 || !part.Edits)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		foreach (VPPXEGroupEdit partEdit : part.Edits)
		{
			acc.Edits.Insert(partEdit);
		}

		if (acc.Edits.Count() > VPPXEGroupRules.MAX_ROWS * 2)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		acc.PartIdx = acc.PartIdx + 1;
		if (acc.PartIdx < acc.PartCount)
		{
			return;
		}

		DropSideUpload(uploadKey);
		VPPXEGroupsSaveJob saveJob = new VPPXEGroupsSaveJob(sender, acc);
		VPPXEJobQueue.Get().Enqueue(saveJob, VPPXELane.INTERACTIVE);
	}

	// SPAWNABLES tab: every randompresets and spawnabletypes file (vanilla and <ce> files, the server's load order) as
	// rows, the ignore list with the first chunk.
	void XE_GetSpawnables(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		VPPXESpawnablesListJob job = new VPPXESpawnablesListJob(sender, reqId);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	// SPAWNABLES tab save: parts of VPPXESpwRules.EDITS_PER_PART edits of one file, like XE_SaveEvents.
	void XE_SaveSpawnables(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param1<ref VPPXESpawnablesSave> data;
		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESpawnablesSave part = data.param1;
		if (!sender || !part)
		{
			return;
		}

		int reqId = part.ReqId;
		string plainId = sender.GetPlainId();
		string uploadKey = plainId + "|p" + reqId.ToString();
		if (part.PartIdx == 0)
		{
			if (!CheckPerm(sender, "MenuXMLEditor:EditSpawnables", reqId) || !CheckBooted(sender, reqId))
			{
				return;
			}

			// a new spawnables save of this admin replaces any unfinished one
			DropUploadsWithPrefix(plainId + "|p");
			VPPXESpawnablesSave started = new VPPXESpawnablesSave();
			started.ReqId = reqId;
			started.PartIdx = 0;
			started.PartCount = part.PartCount;
			started.FileKey = part.FileKey;
			started.BaseRevision = part.BaseRevision;
			m_SpwUploads.Set(uploadKey, started);
		}

		VPPXESpawnablesSave acc = m_SpwUploads.Get(uploadKey);
		if (!acc)
		{
			return;
		}

		m_SideUploadTimes.Set(uploadKey, GetGame().GetTime());
		if (part.PartIdx != acc.PartIdx || part.PartCount != acc.PartCount || part.PartCount < 1 || !part.Edits)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		foreach (VPPXESpwEdit partEdit : part.Edits)
		{
			acc.Edits.Insert(partEdit);
		}

		if (acc.Edits.Count() > VPPXESpwRules.MAX_ROWS * 2)
		{
			DropSideUpload(uploadKey);
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		acc.PartIdx = acc.PartIdx + 1;
		if (acc.PartIdx < acc.PartCount)
		{
			return;
		}

		DropSideUpload(uploadKey);
		VPPXESpawnablesSaveJob saveJob = new VPPXESpawnablesSaveJob(sender, acc);
		VPPXEJobQueue.Get().Enqueue(saveJob, VPPXELane.INTERACTIVE);
	}

	// SPAWNABLES tab, TEST SPAWN AT ME: the type in front of the admin with the engine's own loot roll (ECE_EQUIP, the
	// Item Manager's "CE defaults" flags). The engine uses the loot config it loaded at start, not the files as they
	// are now. Needs the Item Manager spawn right too: it creates real, persistent items. Infected and animals get
	// their attachments only: the CE fills their cargo when they die, so equipping it now would double the loot
	// (CreateObjectEx hands only the requested ECE_EQUIP_* bits to the roll).
	void XE_TestSpawnable(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		string typeName = data.param2;
		if (!CheckPerm(sender, "MenuXMLEditor:EditSpawnables", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		PermissionManager pm = GetPermissionManager();
		if (!HasPerm(pm, plainId, "MenuItemManager:SpawnItem"))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_PERMISSION", "");
			VPPXELog.Action(sender, "was denied MenuItemManager:SpawnItem (XML Editor test spawn)", false);
			return;
		}

		if (!VPPXmlText.IsValidClassName(typeName) || !GetTypes() || !GetTypes().GetEffective(typeName))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_SPB_TEST_NOT_CE", typeName);
			return;
		}

		PlayerBase admin = pm.GetPlayerBaseByID(plainId);
		if (!admin || !admin.IsAlive())
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_SPB_TEST_NO_PLAYER", "");
			return;
		}

		vector adminDir = admin.GetDirection();
		vector spawnPos = admin.GetPosition() + adminDir * 1.5;
		string lowerType = typeName;
		lowerType.ToLower();
		bool isAiType = GetGame().IsKindOf(lowerType, "dz_lightai");
		int flags = ECE_SETUP | ECE_KEEPHEIGHT | ECE_PLACE_ON_SURFACE | ECE_EQUIP;
		if (isAiType)
		{
			flags = ECE_SETUP | ECE_KEEPHEIGHT | ECE_PLACE_ON_SURFACE | ECE_EQUIP_ATTACHMENTS | ECE_INITAI | ECE_CREATEPHYSICS;
		}

		Object created = GetGame().CreateObjectEx(typeName, spawnPos, flags);
		EntityAI root = EntityAI.Cast(created);
		if (!root)
		{
			if (created)
			{
				GetGame().ObjectDelete(created);
			}

			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_SPB_TEST_FAILED", typeName);
			return;
		}

		VPPXESpwTestResult result = new VPPXESpwTestResult();
		result.ReqId = reqId;
		result.TypeName = typeName;
		result.CargoOnDeath = isAiType;
		CollectTestSpawn(root, result);
		VPPXENet.SendNow(sender, "XE_OnSpawnableTest", new Param1<ref VPPXESpwTestResult>(result));
		int itemCount = result.Names.Count();
		VPPXELog.Action(sender, "test spawned " + typeName + " with " + itemCount.ToString() + " loot item(s)", true);
	}

	// Every entity in the test spawn's inventory (not the root itself): class, parent class, attachment or cargo, and
	// the quantity it got.
	protected void CollectTestSpawn(EntityAI root, VPPXESpwTestResult result)
	{
		array<EntityAI> contents = new array<EntityAI>;
		root.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, contents);
		foreach (EntityAI child : contents)
		{
			if (!child || child == root)
			{
				continue;
			}

			InventoryLocation loc = new InventoryLocation();
			child.GetInventory().GetCurrentInventoryLocation(loc);
			int place = VPPXESpwTestResult.PLACE_OTHER;
			if (loc.GetType() == InventoryLocationType.ATTACHMENT)
			{
				place = VPPXESpwTestResult.PLACE_ATTACHMENT;
			}
			else if (loc.GetType() == InventoryLocationType.CARGO)
			{
				place = VPPXESpwTestResult.PLACE_CARGO;
			}

			string parentName = "";
			EntityAI parentEntity = loc.GetParent();
			if (parentEntity)
			{
				parentName = parentEntity.GetType();
			}

			string childName = child.GetType();
			string detail = TestQuantityText(child);
			result.Names.Insert(childName);
			result.Parents.Insert(parentName);
			result.Places.Insert(place);
			result.Details.Insert(detail);
		}
	}

	// "45%" of the item's quantity or ammo, "" when it has none.
	protected string TestQuantityText(EntityAI spawnedEntity)
	{
		Magazine mag = Magazine.Cast(spawnedEntity);
		if (mag)
		{
			int ammo = mag.GetAmmoCount();
			int ammoMax = mag.GetAmmoMax();
			if (ammoMax <= 0)
			{
				return "";
			}

			return ammo.ToString() + "/" + ammoMax.ToString();
		}

		ItemBase item = ItemBase.Cast(spawnedEntity);
		if (!item || !item.HasQuantity())
		{
			return "";
		}

		float quantity = item.GetQuantity();
		float quantityMax = item.GetQuantityMax();
		if (quantityMax <= 0)
		{
			return "";
		}

		float fraction = quantity / quantityMax;
		return VPPXESpwRoll.Percent(fraction);
	}

	protected void DropEventUploadsOf(string plainId)
	{
		DropUploadsWithPrefix(plainId + "|e");
		DropUploadsWithPrefix(plainId + "|s");
		DropUploadsWithPrefix(plainId + "|g");
		DropUploadsWithPrefix(plainId + "|p");
	}

	// EVENTS, spawns, groups and SPAWNABLES uploads whose key starts with prefix.
	protected void DropUploadsWithPrefix(string prefix)
	{
		array<string> doomed = new array<string>();
		for (int i = 0; i < m_EvtUploads.Count(); i++)
		{
			string key = m_EvtUploads.GetKey(i);
			if (key.IndexOf(prefix) == 0)
			{
				doomed.Insert(key);
			}
		}

		for (int j = 0; j < m_SpawnUploads.Count(); j++)
		{
			string spawnKey = m_SpawnUploads.GetKey(j);
			if (spawnKey.IndexOf(prefix) == 0)
			{
				doomed.Insert(spawnKey);
			}
		}

		for (int g = 0; g < m_GroupUploads.Count(); g++)
		{
			string groupKey = m_GroupUploads.GetKey(g);
			if (groupKey.IndexOf(prefix) == 0)
			{
				doomed.Insert(groupKey);
			}
		}

		for (int p = 0; p < m_SpwUploads.Count(); p++)
		{
			string spwKey = m_SpwUploads.GetKey(p);
			if (spwKey.IndexOf(prefix) == 0)
			{
				doomed.Insert(spwKey);
			}
		}

		foreach (string doomedKey : doomed)
		{
			DropSideUpload(doomedKey);
		}
	}

	// A MESSAGES or EVENTS upload (whichever map holds the key).
	protected void DropSideUpload(string uploadKey)
	{
		m_MsgUploads.Remove(uploadKey);
		m_EvtUploads.Remove(uploadKey);
		m_SpawnUploads.Remove(uploadKey);
		m_GroupUploads.Remove(uploadKey);
		m_SpwUploads.Remove(uploadKey);
		m_SideUploadTimes.Remove(uploadKey);
	}

	protected void DropMessageUploadsOf(string plainId)
	{
		string prefix = plainId + "|";
		array<string> doomed = new array<string>();
		for (int i = 0; i < m_MsgUploads.Count(); i++)
		{
			string key = m_MsgUploads.GetKey(i);
			if (key.IndexOf(prefix) == 0)
			{
				doomed.Insert(key);
			}
		}

		foreach (string doomedKey : doomed)
		{
			DropSideUpload(doomedKey);
		}
	}

	void XE_GetDistribution(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, string, int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:DistributionMap", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Dist.RequestDistribution(sender, reqId, data.param2, data.param3);
	}

	void XE_LiveScan(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:LiveScan", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		m_Dist.RequestLiveScan(sender, reqId, data.param2);
	}

	// MAP tab: teleports the admin to a live-scan hit where it is now, height included (the scan only sends x / z).
	// The entity is found by its network id; when it no longer exists the scanned x / z at ground level is used.
	void XE_TeleportToEntity(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, int, vector> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		string plainId = sender.GetPlainId();
		PermissionManager pm = GetPermissionManager();
		if (!pm.VerifyPermission(plainId, "TeleportManager:TPPlayers") || !pm.VerifyPermission(plainId, "TeleportManager:TPSelf"))
		{
			return;
		}

		PlayerBase admin = pm.GetPlayerBaseByID(plainId);
		TeleportManager teleporter = GetTeleportManager();
		if (!admin || !teleporter)
		{
			return;
		}

		vector dest = data.param3;
		Object found = GetGame().GetObjectByNetworkId(data.param1, data.param2);
		if (found)
		{
			dest = found.GetPosition();
		}
		else
		{
			dest[1] = 0;
			pm.NotifyPlayer(plainId, "#VSTR_XMLE_TP_GONE", NotifyTypes.NOTIFY);
		}

		string destX = dest[0].ToString();
		string destY = dest[1].ToString();
		string destZ = dest[2].ToString();
		array<string> args = new array<string>();
		args.Insert(destX);
		args.Insert(destY);
		args.Insert(destZ);
		teleporter.TeleportToPoint(args, admin, plainId);
	}

	void XE_DeleteLiveObjects(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
		{
			return;
		}

		Param3<int, string, ref array<int>> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!sender)
		{
			return;
		}

		int reqId = data.param1;
		if (!CheckPerm(sender, "MenuXMLEditor:DeleteObjects", reqId) || !CheckBooted(sender, reqId))
		{
			return;
		}

		array<int> netPairs = data.param3;
		if (!netPairs)
		{
			netPairs = new array<int>();
		}

		m_Dist.DeleteLiveObjects(sender, reqId, data.param2, netPairs);
	}
};

// INTERACTIVE: builds the index reply (rows copied for the wire, chunks closed at TYPE_ROWS_PER_CHUNK rows or
// CHUNK_BYTES, never mixing files, at least one chunk per file) and queues it through VPPXENet.Send.
class VPPXEIndexSendJob : VPPXEJob
{
	protected XMLEditor m_Owner;
	protected string m_PlainId;
	protected int m_ReqId;
	protected string m_Key;
	protected bool m_PushAfter;
	protected int m_Phase;
	protected int m_RegistryRev;
	protected ref array<string> m_Keys;
	protected int m_FileCursor;
	protected int m_RowCursor;
	protected string m_CurKey;
	protected int m_CurRev;
	protected ref array<ref VPPXETypeRow> m_Rows;
	protected ref array<int> m_WireBits;
	protected ref array<ref VPPXETypeIndexChunk> m_Chunks;
	protected VPPXETypeIndexChunk m_Cur;
	protected int m_CurBytes;
	protected int m_SendCursor;

	void VPPXEIndexSendJob(XMLEditor owner, string plainId, int reqId, string fileKey)
	{
		m_Owner = owner;
		m_PlainId = plainId;
		m_ReqId = reqId;
		m_Key = fileKey;
		m_Chunks = new array<ref VPPXETypeIndexChunk>();
		m_Keys = new array<string>();
		m_FileCursor = -1;
	}

	void SetPushSessionAfter(bool push)
	{
		m_PushAfter = push;
	}

	override string GetLabel()
	{
		return "TypeIndexSend";
	}

	protected PlayerIdentity Requester()
	{
		return VPPXENet.ResolveIdentity(m_PlainId);
	}

	override bool Step()
	{
		if (!m_Owner)
		{
			return true;
		}

		if (m_Phase == 0)
		{
			if (m_Key != "")
			{
				m_Keys.Insert(m_Key);
			}
			else
			{
				array<VPPXEFileEntry> typesFiles = new array<VPPXEFileEntry>();
				m_Owner.GetRegistry().GetByKind(VPPXEFileKind.TYPES, typesFiles);
				foreach (VPPXEFileEntry entry : typesFiles)
				{
					m_Keys.Insert(entry.Key);
				}
			}

			m_RegistryRev = m_Owner.GetRegistry().GetRev();
			m_Phase = 1;
			// the index is current for this reply: a TYPES progress touches every index request on the client, so a
			// reply queued behind other INTERACTIVE work does not look dead (Progress ignores a null identity)
			PlayerIdentity startTo = Requester();
			VPPXENet.Progress(startTo, m_ReqId, VPPXEStage.TYPES, 100);
		}

		PlayerIdentity to = null;
		if (m_Phase == 2)
		{
			to = Requester();
			if (!to)
			{
				return true;
			}
		}

		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == 1)
			{
				BuildStep();
				if (m_Phase == 2)
				{
					to = Requester();
					if (!to)
					{
						return true;
					}
				}

				continue;
			}

			if (m_Phase == 2)
			{
				SendStep(to);
				continue;
			}

			return true;
		}

		return false;
	}

	protected void NewChunk(string fileKey, int fileRevision)
	{
		VPPXETypeIndexChunk chunk = new VPPXETypeIndexChunk();
		chunk.ReqId = m_ReqId;
		chunk.ChunkIdx = m_Chunks.Count();
		chunk.ChunkCount = 0;
		chunk.RegistryRev = m_RegistryRev;
		chunk.FileKey = fileKey;
		chunk.FileRevision = fileRevision;
		m_Chunks.Insert(chunk);
		m_Cur = chunk;
		m_CurBytes = 40 + fileKey.Length();
	}

	// Starts the next file, or copies up to 16 rows of the current one.
	protected void BuildStep()
	{
		if (!m_Rows)
		{
			m_FileCursor++;
			if (m_FileCursor >= m_Keys.Count())
			{
				if (m_Chunks.Count() == 0)
				{
					NewChunk("", 0);
				}

				m_SendCursor = 0;
				m_Phase = 2;
				return;
			}

			VPPXETypesService types = m_Owner.GetTypes();
			m_CurKey = m_Keys.Get(m_FileCursor);
			m_CurRev = types.GetFileRevision(m_CurKey);
			NewChunk(m_CurKey, m_CurRev);
			VPPXETypesFileData fd = types.GetFileData(m_CurKey);
			m_RowCursor = 0;
			if (fd && fd.Rows)
			{
				m_Rows = fd.Rows;
				m_WireBits = fd.WireBits;
			}
			else
			{
				m_Rows = new array<ref VPPXETypeRow>();
				m_WireBits = new array<int>();
			}

			return;
		}

		int copied = 0;
		int rowCount = m_Rows.Count();
		while (copied < 16 && m_RowCursor < rowCount)
		{
			VPPXETypeRow copy = VPPXETypeMerge.Copy(m_Rows.Get(m_RowCursor));
			if (m_WireBits && m_RowCursor < m_WireBits.Count())
			{
				copy.Issues = copy.Issues | m_WireBits.Get(m_RowCursor);
			}

			int size = VPPXETypesService.EstimateRow(copy) + 4;
			int inChunk = m_Cur.Rows.Count();
			if (inChunk > 0 && (inChunk >= VPPXEConst.TYPE_ROWS_PER_CHUNK || m_CurBytes + size > VPPXEConst.CHUNK_BYTES))
			{
				NewChunk(m_CurKey, m_CurRev);
			}

			m_Cur.Rows.Insert(copy);
			m_CurBytes += size;
			m_RowCursor++;
			copied++;
		}

		if (m_RowCursor >= rowCount)
		{
			m_Rows = null;
			m_WireBits = null;
		}
	}

	protected void SendStep(PlayerIdentity to)
	{
		int chunkCount = m_Chunks.Count();
		if (m_SendCursor >= chunkCount)
		{
			if (m_PushAfter)
			{
				m_Owner.QueueSessionPush(m_PlainId);
			}

			if (VPPXENet.NET_TRACE)
			{
				int traceReq = m_ReqId;
				string traceLine = "[XMLEditor][Net] index reply " + traceReq.ToString() + " for " + m_PlainId + ": " + chunkCount.ToString() + " chunks queued";
				Print(traceLine);
			}

			m_Phase = 3;
			return;
		}

		VPPXETypeIndexChunk chunk = m_Chunks.Get(m_SendCursor);
		chunk.ChunkCount = chunkCount;
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnTypeIndexChunk");
		Param1<ref VPPXETypeIndexChunk> netPayload = new Param1<ref VPPXETypeIndexChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(to, "XE_OnTypeIndexChunk", netRpc);
		m_Chunks.Set(m_SendCursor, null);
		m_SendCursor++;
	}

	override void OnAborted()
	{
		VPPXENet.Result(Requester(), m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
	}
};

// INTERACTIVE: collects registry, dist and types issues in bounded steps, builds fixed-width sort keys, ONE
// native sort, caps at MAX_ISSUES and chunks the reply (ISSUES_PER_CHUNK rows or CHUNK_BYTES).
class VPPXEIssuesJob : VPPXEJob
{
	protected XMLEditor m_Owner;
	protected string m_PlainId;
	protected int m_ReqId;
	protected string m_FileKey;
	protected int m_Phase;
	protected int m_Cursor;
	protected ref array<ref VPPXEIssue> m_All;
	protected ref array<string> m_Keys;
	protected ref array<ref VPPXEIssuesChunk> m_Chunks;
	protected VPPXEIssuesChunk m_Cur;
	protected int m_CurBytes;
	protected int m_Total;
	protected int m_Errors;
	protected int m_Warnings;
	protected int m_Infos;
	protected int m_SendCursor;

	void VPPXEIssuesJob(XMLEditor owner, string plainId, int reqId, string fileKey)
	{
		m_Owner = owner;
		m_PlainId = plainId;
		m_ReqId = reqId;
		m_FileKey = fileKey;
		m_All = new array<ref VPPXEIssue>();
		m_Keys = new array<string>();
		m_Chunks = new array<ref VPPXEIssuesChunk>();
	}

	override string GetLabel()
	{
		return "Issues";
	}

	protected PlayerIdentity Requester()
	{
		return VPPXENet.ResolveIdentity(m_PlainId);
	}

	override bool Step()
	{
		if (!m_Owner)
		{
			return true;
		}

		PlayerIdentity to = Requester();
		if (!to)
		{
			return true;
		}

		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == 0)
			{
				m_Owner.GetRegistry().CollectIssues(m_All);
				m_Owner.GetDist().CollectIssues(m_All);
				m_Cursor = 0;
				m_Phase = 1;
				continue;
			}

			if (m_Phase == 1)
			{
				m_Cursor = m_Owner.GetTypes().CollectIssuesStep(m_All, m_FileKey, m_Cursor, 200);
				if (m_Cursor < 0)
				{
					m_Cursor = 0;
					m_Phase = 2;
				}

				continue;
			}

			if (m_Phase == 2)
			{
				KeyStep();
				continue;
			}

			if (m_Phase == 3)
			{
				m_Keys.Sort();
				m_Cursor = 0;
				m_Phase = 4;
				continue;
			}

			if (m_Phase == 4)
			{
				ChunkStep();
				continue;
			}

			if (m_Phase == 5)
			{
				SendStep(to);
				continue;
			}

			return true;
		}

		VPPXENet.Progress(to, m_ReqId, VPPXEStage.ISSUES, m_Phase * 20);
		return false;
	}

	// Up to 64 issues per call: filter, severity totals and the key 2-sev | fileIdx | line | index.
	protected void KeyStep()
	{
		int count = m_All.Count();
		int stop = m_Cursor + 64;
		if (stop > count)
		{
			stop = count;
		}

		VPPXEFileRegistry reg = m_Owner.GetRegistry();
		for (int i = m_Cursor; i < stop; i++)
		{
			VPPXEIssue issue = m_All.Get(i);
			if (!issue || (m_FileKey != "" && issue.FileKey != m_FileKey))
			{
				continue;
			}

			int sev = issue.Severity;
			if (sev < VPPXESeverity.INFO)
			{
				sev = VPPXESeverity.INFO;
			}

			if (sev > VPPXESeverity.ERROR)
			{
				sev = VPPXESeverity.ERROR;
			}

			if (sev == VPPXESeverity.ERROR)
			{
				m_Errors++;
			}
			else if (sev == VPPXESeverity.WARNING)
			{
				m_Warnings++;
			}
			else
			{
				m_Infos++;
			}

			m_Total++;
			int fileIdx = reg.IndexOf(issue.FileKey);
			if (fileIdx < 0 || fileIdx > 9999)
			{
				fileIdx = 9999;
			}

			int line = issue.Line;
			if (line < 0)
			{
				line = 0;
			}

			if (line > 9999999)
			{
				line = 9999999;
			}

			string sortKey = VPPXmlText.PadInt(2 - sev, 1) + VPPXmlText.PadInt(fileIdx, 4) + VPPXmlText.PadInt(line, 7) + VPPXmlText.PadInt(i, 7);
			m_Keys.Insert(sortKey);
		}

		m_Cursor = stop;
		if (m_Cursor >= count)
		{
			m_Phase = 3;
		}
	}

	protected void NewChunk()
	{
		VPPXEIssuesChunk chunk = new VPPXEIssuesChunk();
		chunk.ReqId = m_ReqId;
		chunk.ChunkIdx = m_Chunks.Count();
		m_Chunks.Insert(chunk);
		m_Cur = chunk;
		m_CurBytes = 40;
	}

	// Up to 32 sorted keys per call, at most MAX_ISSUES in total.
	protected void ChunkStep()
	{
		if (m_Chunks.Count() == 0)
		{
			NewChunk();
		}

		int limit = m_Keys.Count();
		if (limit > VPPXEConst.MAX_ISSUES)
		{
			limit = VPPXEConst.MAX_ISSUES;
		}

		int stop = m_Cursor + 32;
		if (stop > limit)
		{
			stop = limit;
		}

		for (int i = m_Cursor; i < stop; i++)
		{
			string sortKey = m_Keys.Get(i);
			int idx = sortKey.Substring(sortKey.Length() - 7, 7).ToInt();
			VPPXEIssue issue = m_All.Get(idx);
			int size = VPPXETypesService.EstimateIssue(issue) + 4;
			int inChunk = m_Cur.Issues.Count();
			if (inChunk > 0 && (inChunk >= VPPXEConst.ISSUES_PER_CHUNK || m_CurBytes + size > VPPXEConst.CHUNK_BYTES))
			{
				NewChunk();
			}

			m_Cur.Issues.Insert(issue);
			m_CurBytes += size;
		}

		m_Cursor = stop;
		if (m_Cursor >= limit)
		{
			m_SendCursor = 0;
			m_Phase = 5;
		}
	}

	protected void SendStep(PlayerIdentity to)
	{
		int chunkCount = m_Chunks.Count();
		if (m_SendCursor >= chunkCount)
		{
			if (VPPXENet.NET_TRACE)
			{
				int traceReq = m_ReqId;
				string traceLine = "[XMLEditor][Net] issues reply " + traceReq.ToString() + " for " + m_PlainId + ": " + chunkCount.ToString() + " chunks queued";
				Print(traceLine);
			}

			m_Phase = 6;
			return;
		}

		VPPXEIssuesChunk chunk = m_Chunks.Get(m_SendCursor);
		chunk.ChunkCount = chunkCount;
		chunk.Total = m_Total;
		chunk.Errors = m_Errors;
		chunk.Warnings = m_Warnings;
		chunk.Infos = m_Infos;
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnIssuesChunk");
		Param1<ref VPPXEIssuesChunk> netPayload = new Param1<ref VPPXEIssuesChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(to, "XE_OnIssuesChunk", netRpc);
		m_Chunks.Set(m_SendCursor, null);
		m_SendCursor++;
	}

	override void OnAborted()
	{
		VPPXENet.Result(Requester(), m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
	}
};

XMLEditor GetXMLEditor()
{
	return XMLEditor.Cast(GetPluginManager().GetPluginByType(XMLEditor));
}
