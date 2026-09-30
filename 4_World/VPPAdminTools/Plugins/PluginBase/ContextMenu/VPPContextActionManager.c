/*
	Context action registry (client + server plugin).
	- One registry of VPPContextAction rows, the same on client and server (actions are looked up by id).
	- Client: page building, permission-hint cache, execute/state/permission requests.
	- Server: the only dispatcher of RPC_VPPContextMenu (permission, target level, range, logging, webhooks).
*/
class VPPContextActionManager extends PluginBase
{
	protected ref map<string, ref VPPContextAction> m_Actions;
	protected ref array<VPPContextAction>           m_Sorted;        //weak entries, owned by m_Actions
	protected ref array<ref VPPContextProvider>     m_Providers;
	protected ref map<string,bool>                  m_PermCache;
	protected int                                   m_LastPermRequest;
	protected bool                                  m_PermRequested;
	protected int                                   m_Seq;
	protected ref map<string,int>                   m_Cooldowns;
	protected ref array<string>                     m_RegisteredPerms;
	protected ref map<string,bool>                  m_ServerPermSet; //every non-empty GetPermission() of the registered actions
	protected ref map<string,bool>                  m_QueuedIds;     //ids that came from the persistent queue
	protected bool                                  m_ServerReady;

	protected static VPPContextActionManager s_Instance; //weak
	//PERSISTENT for the whole script load: re-applied to every manager instance (every mission), never cleared
	protected static ref array<ref VPPContextAction>   s_QueuedActions;
	protected static ref array<ref VPPContextProvider> s_QueuedProviders;

	ref ScriptInvoker Event_OnResult;       //client: (int seq, string actionId, bool success, string messageKey, string messageArg, int newState)
	ref ScriptInvoker Event_OnState;        //client: (int seq, string targetKey, map<string,string> state)
	ref ScriptInvoker Event_OnPermissions;  //client: ()

	void VPPContextActionManager()
	{
		m_Actions         = new map<string, ref VPPContextAction>;
		m_Sorted          = new array<VPPContextAction>;
		m_Providers       = new array<ref VPPContextProvider>;
		m_PermCache       = new map<string,bool>;
		m_Cooldowns       = new map<string,int>;
		m_RegisteredPerms = new array<string>;
		m_ServerPermSet   = new map<string,bool>;
		m_QueuedIds       = new map<string,bool>;

		Event_OnResult      = new ScriptInvoker();
		Event_OnState       = new ScriptInvoker();
		Event_OnPermissions = new ScriptInvoker();

		s_Instance = this;

		//built-ins first and outside the override point: a mod that forgets super cannot wipe them
		RegisterBuiltIns();
		RegisterActions();
		RegisterProviders();
		ApplyPersistentQueues();

		if (GetGame().IsServer())
		{
			//before PermissionManager.OnInit builds the default group on first run
			RegisterServerPermissions();

			GetRPCManager().AddRPC("RPC_VPPContextMenu", "ExecuteAction", this, SingleplayerExecutionType.Server);
			GetRPCManager().AddRPC("RPC_VPPContextMenu", "RequestState", this, SingleplayerExecutionType.Server);
			GetRPCManager().AddRPC("RPC_VPPContextMenu", "RequestPermissions", this, SingleplayerExecutionType.Server);
		}

		if (!GetGame().IsDedicatedServer())
		{
			GetRPCManager().AddRPC("RPC_VPPContextMenuClient", "HandleResult", this, SingleplayerExecutionType.Client);
			GetRPCManager().AddRPC("RPC_VPPContextMenuClient", "HandleState", this, SingleplayerExecutionType.Client);
			GetRPCManager().AddRPC("RPC_VPPContextMenuClient", "HandlePermissions", this, SingleplayerExecutionType.Client);
			GetRPCManager().AddRPC("RPC_VPPContextMenuClient", "SyncMagazineAmmo", this, SingleplayerExecutionType.Client);
		}

		int actionCount = m_Actions.Count();
		int providerCount = m_Providers.Count();
		string summary = "[VPPContextMenu] registry ready: " + actionCount.ToString() + " actions, ";
		summary = summary + providerCount.ToString() + " providers";
		Print(summary);
	}

	void ~VPPContextActionManager()
	{
		//the static queues are NOT cleared: they are re-applied to the next instance
		if (s_Instance == this)
			s_Instance = null;
	}

	override void OnInit()
	{
		super.OnInit();
		if (GetGame().IsServer())
			RegisterServerPermissions();
	}

	static VPPContextActionManager GetInstance()
	{
		return s_Instance;
	}

	//-----------------------------------------------------------------
	// extension points: override in a modded VPPContextActionManager, ALWAYS call super first
	//-----------------------------------------------------------------
	void RegisterActions()
	{
	}

	void RegisterProviders()
	{
	}

	protected void RegisterBuiltIns()
	{
		VPPCA_PlayerActions.Register(this);
		VPPCA_WeaponActions.Register(this);
		VPPCA_EntityActions.Register(this);
	}

	//-----------------------------------------------------------------
	// registry
	//-----------------------------------------------------------------
	bool AddAction(VPPContextAction action)
	{
		if (!action)
			return false;

		string id = action.GetId();
		if (id == "")
		{
			Print("[VPPContextMenu] action with an empty id rejected: " + action.ClassName());
			return false;
		}

		VPPContextAction existing = m_Actions.Get(id);
		if (existing == action)
			return true;

		if (existing)
		{
			Print("[VPPContextMenu] duplicate action id rejected: " + id);
			return false;
		}

		m_Actions.Set(id, action);
		RebuildSorted();
		UpdateServerPermSet(action);
		if (m_ServerReady)
			RegisterPermissionOf(action);
		return true;
	}

	bool ReplaceAction(string id, VPPContextAction action)
	{
		if (!action || action.GetId() != id || !m_Actions.Contains(id))
		{
			Print("[VPPContextMenu] ReplaceAction rejected for id: " + id);
			return false;
		}

		m_Actions.Set(id, action);
		RebuildSorted();
		UpdateServerPermSet(action);
		if (m_ServerReady)
			RegisterPermissionOf(action);
		return true;
	}

	bool RemoveAction(string id)
	{
		if (!m_Actions.Contains(id))
			return false;

		m_Actions.Remove(id);
		m_QueuedIds.Remove(id);
		RebuildSorted();
		return true;
	}

	VPPContextAction GetAction(string id)
	{
		return m_Actions.Get(id);
	}

	//sorted snapshot
	array<VPPContextAction> GetActions()
	{
		array<VPPContextAction> snapshot = new array<VPPContextAction>;
		snapshot.Copy(m_Sorted);
		return snapshot;
	}

	//dedupe by GetId() (replaces), kept in ascending GetOrder()
	void AddProvider(VPPContextProvider provider)
	{
		if (!provider)
			return;

		string id = provider.GetId();
		for (int i = 0; i < m_Providers.Count(); i++)
		{
			VPPContextProvider current = m_Providers.Get(i);
			if (current && current.GetId() == id)
			{
				m_Providers.RemoveOrdered(i);
				break;
			}
		}

		int at = m_Providers.Count();
		for (int j = 0; j < m_Providers.Count(); j++)
		{
			VPPContextProvider other = m_Providers.Get(j);
			if (other && provider.GetOrder() < other.GetOrder())
			{
				at = j;
				break;
			}
		}

		if (at >= m_Providers.Count())
			m_Providers.Insert(provider);
		else
			m_Providers.InsertAt(provider, at);
	}

	bool RemoveProvider(string providerId)
	{
		for (int i = 0; i < m_Providers.Count(); i++)
		{
			VPPContextProvider current = m_Providers.Get(i);
			if (current && current.GetId() == providerId)
			{
				m_Providers.RemoveOrdered(i);
				return true;
			}
		}
		return false;
	}

	//PERSISTENT: kept for the whole script load and applied to every manager instance; a newer queued
	//instance with the same id replaces the older queued one
	static void QueueAction(VPPContextAction action)
	{
		if (!action)
			return;

		string id = action.GetId();
		if (id == "")
			return;

		if (!s_QueuedActions)
			s_QueuedActions = new array<ref VPPContextAction>;

		bool replaced = false;
		for (int i = 0; i < s_QueuedActions.Count(); i++)
		{
			VPPContextAction queued = s_QueuedActions.Get(i);
			if (queued && queued.GetId() == id)
			{
				s_QueuedActions.Set(i, action);
				replaced = true;
				break;
			}
		}
		if (!replaced)
			s_QueuedActions.Insert(action);

		if (s_Instance)
			s_Instance.ApplyQueuedAction(action);
	}

	static void QueueProvider(VPPContextProvider provider)
	{
		if (!provider)
			return;

		if (!s_QueuedProviders)
			s_QueuedProviders = new array<ref VPPContextProvider>;

		string id = provider.GetId();
		bool replaced = false;
		for (int i = 0; i < s_QueuedProviders.Count(); i++)
		{
			VPPContextProvider queued = s_QueuedProviders.Get(i);
			if (queued && queued.GetId() == id)
			{
				s_QueuedProviders.Set(i, provider);
				replaced = true;
				break;
			}
		}
		if (!replaced)
			s_QueuedProviders.Insert(provider);

		if (s_Instance)
			s_Instance.AddProvider(provider);
	}

	protected void ApplyQueuedAction(VPPContextAction action)
	{
		if (!action)
			return;

		string id = action.GetId();
		VPPContextAction existing = GetAction(id);
		if (existing == action)
			return;

		if (existing && m_QueuedIds.Contains(id))
		{
			ReplaceAction(id, action);
			return;
		}

		if (AddAction(action))
			m_QueuedIds.Set(id, true);
	}

	//never clears the queues: they are once-per-script-load registrations re-applied on every reconnect
	protected void ApplyPersistentQueues()
	{
		if (s_QueuedActions)
		{
			foreach (VPPContextAction queuedAction : s_QueuedActions)
			{
				ApplyQueuedAction(queuedAction);
			}
		}

		if (s_QueuedProviders)
		{
			foreach (VPPContextProvider queuedProvider : s_QueuedProviders)
			{
				AddProvider(queuedProvider);
			}
		}
	}

	//sort key: order clamped to [0, ORDER_MAX] as an int, then id
	protected void RebuildSorted()
	{
		map<string, VPPContextAction> byKey = new map<string, VPPContextAction>;
		array<string> keys = new array<string>;
		foreach (string id, VPPContextAction action : m_Actions)
		{
			if (!action)
				continue;

			int ord = action.GetOrder();
			if (ord < 0)
				ord = 0;
			if (ord > VPPContextConstants.ORDER_MAX)
				ord = VPPContextConstants.ORDER_MAX;

			string key = ord.ToStringLen(6) + "|" + action.GetId();
			byKey.Set(key, action);
			keys.Insert(key);
		}

		keys.Sort();

		m_Sorted.Clear();
		foreach (string sortedKey : keys)
		{
			m_Sorted.Insert(byKey.Get(sortedKey));
		}
	}

	protected void UpdateServerPermSet(VPPContextAction action)
	{
		if (!action)
			return;

		string perm = action.GetPermission();
		if (perm != "")
			m_ServerPermSet.Set(perm, true);
	}

	protected void RegisterPermissionOf(VPPContextAction action)
	{
		if (!action)
			return;

		string perm = action.GetPermission();
		if (perm == "" || !action.AutoRegisterPermission())
			return;
		if (m_RegisteredPerms.Find(perm) != -1)
			return;

		PermissionManager pm = GetPermissionManager();
		if (!pm)
			return;

		pm.AddPermissionType({perm});
		m_RegisteredPerms.Insert(perm);
	}

	//idempotent
	protected void RegisterServerPermissions()
	{
		m_ServerPermSet.Clear();

		array<string> pending = new array<string>;
		foreach (string id, VPPContextAction action : m_Actions)
		{
			if (!action)
				continue;

			string perm = action.GetPermission();
			if (perm == "")
				continue;

			m_ServerPermSet.Set(perm, true);

			if (!action.AutoRegisterPermission())
				continue;
			if (m_RegisteredPerms.Find(perm) != -1 || pending.Find(perm) != -1)
				continue;

			pending.Insert(perm);
		}

		PermissionManager pm = GetPermissionManager();
		if (pending.Count() > 0 && pm)
		{
			pm.AddPermissionType(pending);
			m_RegisteredPerms.InsertAll(pending);
		}

		m_ServerReady = true;
	}

	//-----------------------------------------------------------------
	// page building (client)
	//-----------------------------------------------------------------
	protected bool IsActionVisible(VPPContextAction action, VPPContextTarget target)
	{
		if (!action || !target)
			return false;
		if (!action.IsTargetType(target))
			return false;
		if (!action.SupportsMultiTarget() && target.IsMulti())
			return false;
		if (!action.CanShow(target))
			return false;
		return HasPermissionHint(action.GetPermission());
	}

	void BuildPage(VPPContextTarget target, string parentId, VPPContextItems outItems)
	{
		BuildPageAt(target, parentId, outItems, 0);
	}

	bool HasAnyAction(VPPContextTarget target)
	{
		return HasAnyActionAt(target, 0);
	}

	bool NeedsServerState(VPPContextTarget target)
	{
		if (!target)
			return false;

		foreach (VPPContextAction action : m_Sorted)
		{
			if (action && action.IsTargetType(target) && action.NeedsServerState(target))
				return true;
		}
		return false;
	}

	//empty or unknown -> true (hint only; the server always re-checks)
	bool HasPermissionHint(string permission)
	{
		if (permission == "")
			return true;
		if (m_PermCache.Contains(permission))
			return m_PermCache.Get(permission);
		return true;
	}

	//the depth travels as a parameter so an aborted third-party callback can never leave a stale counter
	protected void BuildPageAt(VPPContextTarget target, string parentId, VPPContextItems outItems, int depth)
	{
		if (!outItems)
			return;

		outItems.Clear();
		if (!target || depth >= VPPContextConstants.MAX_BUILD_DEPTH)
			return;

		foreach (VPPContextAction action : m_Sorted)
		{
			if (!action || action.GetParentId() != parentId)
				continue;
			if (!IsActionVisible(action, target))
				continue;

			if (action.GetKind() == EVPPContextKind.SUBMENU)
			{
				VPPContextTarget sub = action.GetSubmenuTarget(target);
				if (!sub)
					continue;

				if (sub == target)
				{
					if (!HasVisibleChildrenAt(target, action.GetId(), depth + 1))
						continue;
				}
				else
				{
					if (!HasAnyActionAt(sub, depth + 1))
						continue;
				}
			}

			outItems.AddAction(action, target);
		}

		if (parentId != "")
		{
			VPPContextAction parent = GetAction(parentId);
			if (parent)
				parent.OnBuildChildren(target, outItems);
		}

		//sort the default rows BEFORE the providers run, so provider InsertBefore/InsertAt/reorder survives
		outItems.Sort();

		array<VPPContextItem> preProvider = new array<VPPContextItem>;
		for (int pre = 0; pre < outItems.Count(); pre++)
		{
			preProvider.Insert(outItems.Get(pre));
		}

		foreach (VPPContextProvider provider : m_Providers)
		{
			if (provider)
				provider.OnBuild(target, parentId, outItems);
		}

		MergeAppendedByOrder(outItems, preProvider);
		outItems.NormalizeSeparators();
	}

	//rows a provider APPENDED (Add*/Insert, or InsertBefore with an unknown id) form a trailing block of new rows;
	//merge only that block into Order position. Rows the provider placed itself (or reordered) keep their position.
	protected void MergeAppendedByOrder(VPPContextItems items, array<VPPContextItem> preProvider)
	{
		int total = items.Count();
		int tailStart = total;
		while (tailStart > 0)
		{
			VPPContextItem last = items.Get(tailStart - 1);
			if (!last || preProvider.Find(last) >= 0)
				break;
			tailStart--;
		}
		if (tailStart >= total)
			return;

		array<ref VPPContextItem> merged = new array<ref VPPContextItem>;
		for (int h = 0; h < tailStart; h++)
		{
			merged.Insert(items.Get(h));
		}

		for (int t = tailStart; t < total; t++)
		{
			VPPContextItem added = items.Get(t);
			int insertIdx = merged.Count();
			for (int m = 0; m < merged.Count(); m++)
			{
				VPPContextItem existing = merged.Get(m);
				if (existing && existing.Kind != EVPPContextKind.BACK && existing.Order > added.Order)
				{
					insertIdx = m;
					break;
				}
			}
			if (insertIdx >= merged.Count())
				merged.Insert(added);
			else
				merged.InsertAt(added, insertIdx);
		}

		items.Clear();
		foreach (VPPContextItem mergedItem : merged)
		{
			items.Insert(mergedItem);
		}
	}

	protected bool HasVisibleChildrenAt(VPPContextTarget target, string id, int depth)
	{
		VPPContextItems children = new VPPContextItems();
		BuildPageAt(target, id, children, depth);
		for (int i = 0; i < children.Count(); i++)
		{
			VPPContextItem item = children.Get(i);
			if (item && item.Kind != EVPPContextKind.SEPARATOR)
				return true;
		}
		return false;
	}

	protected bool HasAnyActionAt(VPPContextTarget target, int depth)
	{
		VPPContextItems rootItems = new VPPContextItems();
		BuildPageAt(target, "", rootItems, depth);
		return rootItems.CountExecutable() > 0;
	}

	//-----------------------------------------------------------------
	// client -> server
	//-----------------------------------------------------------------
	int SendExecute(VPPContextAction action, VPPContextTarget target, VPPContextArgs args)
	{
		if (!action || !target)
			return -1;

		m_Seq++;
		VPPContextRequest req = new VPPContextRequest();
		target.WriteToRequest(req);
		req.Seq = m_Seq;
		req.ActionId = action.GetId();
		if (args && args.GetMap())
			req.Args.Copy(args.GetMap());

		GetRPCManager().VSendRPC("RPC_VPPContextMenu", "ExecuteAction", new Param1<ref VPPContextRequest>(req), true);
		return m_Seq;
	}

	int SendStateRequest(VPPContextTarget target)
	{
		if (!target)
			return -1;

		m_Seq++;
		VPPContextRequest req = new VPPContextRequest();
		target.WriteToRequest(req);
		req.Seq = m_Seq;
		req.ActionId = "";

		GetRPCManager().VSendRPC("RPC_VPPContextMenu", "RequestState", new Param1<ref VPPContextRequest>(req), true);
		return m_Seq;
	}

	//throttled to PERMISSION_REFRESH_MS unless forced
	void SendPermissionRequest(bool force)
	{
		int now = GetGame().GetTime();
		if (!force && m_PermRequested && now - m_LastPermRequest < VPPContextConstants.PERMISSION_REFRESH_MS)
			return;

		array<string> perms = new array<string>;
		foreach (VPPContextAction action : m_Sorted)
		{
			if (!action)
				continue;

			string perm = action.GetPermission();
			if (perm == "" || perms.Find(perm) != -1)
				continue;

			perms.Insert(perm);
			if (perms.Count() >= VPPContextConstants.MAX_PERMISSION_QUERY)
				break;
		}

		GetRPCManager().VSendRPC("RPC_VPPContextMenu", "RequestPermissions", new Param1<ref array<string>>(perms), true);
		m_LastPermRequest = now;
		m_PermRequested = true;
	}

	static void ClientNotify(string messageKey, string messageArg = "", float seconds = 3.0)
	{
		string text = Widget.TranslateString(messageKey);
		if (messageArg != "")
			text = text + " " + messageArg;

		string title = Widget.TranslateString(VPPContextConstants.NOTIFY_TITLE);
		NotificationSystem.AddNotificationExtended(seconds, title, text, VPPContextConstants.NOTIFY_ICON);
	}

	//-----------------------------------------------------------------
	// server handlers
	//-----------------------------------------------------------------
	void ExecuteAction(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;

		Param1<ref VPPContextRequest> data;
		if (!ctx.Read(data) || !data.param1)
			return;
		if (!sender)
			return;

		VPPContextRequest req = data.param1;
		string senderId = sender.GetPlainId();
		string senderName = sender.GetName();

		//1. per-sender, per-action cooldown (stamped only for known ids, so junk ids never grow the map)
		int now = GetGame().GetTime();
		string cooldownKey = senderId + ":" + req.ActionId;
		if (m_Cooldowns.Contains(cooldownKey) && now - m_Cooldowns.Get(cooldownKey) < VPPContextConstants.SERVER_ACTION_COOLDOWN_MS)
		{
			FailReply(sender, req, "#VSTR_CTX_RESULT_BUSY", false);
			return;
		}

		//2. unknown id
		VPPContextAction action = GetAction(req.ActionId);
		if (!action)
		{
			string unknownLine = LogPrefix(senderName, senderId) + "unknown action rejected: " + req.ActionId;
			LogServer(unknownLine);
			FailReply(sender, req, "#VSTR_CTX_RESULT_INVALID", false);
			return;
		}
		m_Cooldowns.Set(cooldownKey, now);

		//3. client-only actions never run on the server
		string perm = action.GetPermission();
		if (perm == "")
		{
			string clientOnlyLine = LogPrefix(senderName, senderId) + "client-only action rejected: " + req.ActionId;
			LogServer(clientOnlyLine);
			FailReply(sender, req, "#VSTR_CTX_RESULT_INVALID", false);
			return;
		}

		//4. base permission (VerifyPermission already toasts on rejection -> silent failure)
		PermissionManager pm = GetPermissionManager();
		if (!pm)
			return;
		if (!pm.VerifyPermission(senderId, perm))
		{
			FailReply(sender, req, "", true);
			return;
		}

		//5. server-side target resolution
		VPPContextTarget ctxTarget = VPPContextTarget.FromRequest(req);
		VPPContextArgs args = VPPContextArgs.FromMap(req.Args);

		if (ctxTarget.IsPlayer())
		{
			//6. per-player user-group level filter; SetPlayers re-resolves the primary (T2)
			if (action.UsesTargetPermissionLevel())
			{
				array<string> permittedIds = new array<string>;
				array<int> permittedSessions = new array<int>;
				array<string> requestedIds = ctxTarget.GetPlayerIds();
				array<int> requestedSessions = ctxTarget.GetSessionIds();
				for (int i = 0; i < requestedIds.Count(); i++)
				{
					string playerId = requestedIds.Get(i);
					if (playerId == "" || permittedIds.Find(playerId) != -1)
						continue;
					if (!IsCanonicalPlayerId(pm, playerId))
						continue;
					if (!pm.VerifyPermission(senderId, perm, playerId))
						continue;

					int session = -1;
					if (i < requestedSessions.Count())
						session = requestedSessions.Get(i);

					permittedIds.Insert(playerId);
					permittedSessions.Insert(session);
				}

				if (permittedIds.Count() == 0)
				{
					FailReply(sender, req, "", true);
					return;
				}
				ctxTarget.SetPlayers(permittedIds, permittedSessions);
			}

			if (ctxTarget.IsMulti() && !action.SupportsMultiTarget())
			{
				FailReply(sender, req, "#VSTR_CTX_RESULT_INVALID", false);
				return;
			}
		}
		else
		{
			//7a. existence first
			if (!ctxTarget.HasObject())
			{
				FailReply(sender, req, "#VSTR_CTX_RESULT_TARGET_GONE", false);
				return;
			}

			//7b. range from the admin's body
			Object obj = ctxTarget.GetObject();
			PlayerBase adminBody = pm.GetPlayerBaseByID(senderId);
			if (adminBody && vector.Distance(adminBody.GetPosition(), obj.GetPosition()) > VPPContextConstants.MAX_OBJECT_RANGE)
			{
				FailReply(sender, req, "#VSTR_CTX_RESULT_INVALID", false);
				return;
			}

			//7c. ONLY NOW the item-owner level check (static non-EntityAI objects skip it)
			if (action.UsesTargetPermissionLevel())
			{
				EntityAI ent = ctxTarget.GetEntity();
				if (ent)
				{
					PlayerBase owner = PlayerBase.Cast(ent.GetHierarchyRootPlayer());
					if (owner && owner.VPlayerGetSteamId() != "" && !pm.VerifyPermission(senderId, perm, owner.VPlayerGetSteamId()))
					{
						FailReply(sender, req, "", true);
						return;
					}
				}
			}
		}

		//8. type + action-specific server gate (rule A4)
		if (!action.IsTargetType(ctxTarget) || !action.CanExecuteServer(sender, ctxTarget, args))
		{
			FailReply(sender, req, "#VSTR_CTX_RESULT_INVALID", false);
			return;
		}

		//9. execute
		VPPContextResult result = new VPPContextResult();
		action.OnExecuteServer(sender, ctxTarget, args, result);

		//10. central log + webhook
		string msg = action.GetLogMessage(sender, ctxTarget, args, result);
		string line = LogPrefix(senderName, senderId) + msg;
		if (!result.Success)
			line = line + " FAILED";
		if (result.LogDetail != "")
			line = line + " | " + result.LogDetail;
		LogServer(line);

		if (result.Success)
		{
			WebHooksManager hooks = GetWebHooksManager();
			if (hooks)
			{
				string hookMsg = "[ContextMenu] " + msg;
				hooks.PostData(AdminActivityMessage, new AdminActivityMessage(senderId, senderName, hookMsg));
			}
		}

		//11. reply
		SendResult(sender, req.Seq, req.ActionId, result);
	}

	//state cooldown keys are per target, so drop stale stamps once the map grows (never touches live cooldowns)
	protected void PruneCooldowns(int now)
	{
		if (m_Cooldowns.Count() < 256)
			return;

		array<string> staleKeys = new array<string>;
		foreach (string stampKey, int stampTime : m_Cooldowns)
		{
			if (now - stampTime >= 1000)
				staleKeys.Insert(stampKey);
		}
		foreach (string staleKey : staleKeys)
		{
			m_Cooldowns.Remove(staleKey);
		}
	}

	void RequestState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;

		Param1<ref VPPContextRequest> data;
		if (!ctx.Read(data) || !data.param1)
			return;
		if (!sender)
			return;

		VPPContextRequest req = data.param1;
		string senderId = sender.GetPlainId();

		VPPContextTarget ctxTarget = VPPContextTarget.FromRequest(req);
		if (!ctxTarget)
			return;
		//computed IMMEDIATELY: stored fields are never overwritten (T1), so this equals the client key
		string replyKey = ctxTarget.GetKey();

		//per sender AND per target: the client records a key as requested before the reply arrives,
		//so a per-sender-only cooldown would silently starve a second page (e.g. a fast retarget)
		int now = GetGame().GetTime();
		PruneCooldowns(now);
		string cooldownKey = senderId + ":state:" + replyKey;
		if (m_Cooldowns.Contains(cooldownKey) && now - m_Cooldowns.Get(cooldownKey) < VPPContextConstants.SERVER_STATE_COOLDOWN_MS)
			return;
		m_Cooldowns.Set(cooldownKey, now);

		PermissionManager pm = GetPermissionManager();
		if (!pm)
			return;

		map<string,string> state = new map<string,string>;

		bool canCollect = true;
		if (ctxTarget.IsPlayer())
			canCollect = IsCanonicalPlayerId(pm, ctxTarget.GetPlayerId());

		//a name-mismatched admin would get one 15s toast per state-collecting action (VerifyPermission ignores
		//sendNotify there); RequestPermissions already delivers the single toast, so reply with an empty map silently
		if (canCollect && IsSenderNameRejected(pm, senderId))
			canCollect = false;

		if (canCollect)
		{
			foreach (VPPContextAction action : m_Sorted)
			{
				if (!action)
					continue;

				string perm = action.GetPermission();
				if (perm == "")
					continue;
				if (!action.IsTargetType(ctxTarget) || !action.NeedsServerState(ctxTarget))
					continue;

				string levelTarget = "";
				if (ctxTarget.IsPlayer() && action.UsesTargetPermissionLevel())
					levelTarget = ctxTarget.GetPlayerId();

				if (pm.VerifyPermission(senderId, perm, levelTarget, false))
					action.OnCollectServerState(sender, ctxTarget, state);
			}
		}

		Param3<int,string,ref map<string,string>> reply = new Param3<int,string,ref map<string,string>>(req.Seq, replyKey, state);
		GetRPCManager().VSendRPC("RPC_VPPContextMenuClient", "HandleState", reply, true, sender);
	}

	void RequestPermissions(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;

		Param1<ref array<string>> data;
		if (!ctx.Read(data) || !data.param1)
			return;
		if (!sender)
			return;

		string senderId = sender.GetPlainId();
		PermissionManager pm = GetPermissionManager();
		if (!pm)
			return;

		array<string> requested = data.param1;
		int limit = requested.Count();
		if (limit > VPPContextConstants.MAX_PERMISSION_QUERY)
			limit = VPPContextConstants.MAX_PERMISSION_QUERY;

		//VerifyPermission toasts #VSTR_ERROR_NAME_MISMATCH on the force-saved-name path regardless of sendNotify.
		//Detect that case once so a mismatched admin gets a single toast instead of one per registry permission.
		bool nameRejected = IsSenderNameRejected(pm, senderId);
		bool nameToastSent = false;

		//only registry strings reach VerifyPermission: no unknown-permission log spam / log-flood vector
		map<string,bool> answers = new map<string,bool>;
		for (int i = 0; i < limit; i++)
		{
			string perm = requested.Get(i);
			if (answers.Contains(perm))
				continue;

			if (!m_ServerPermSet.Contains(perm))
			{
				answers.Set(perm, false);
			}
			else if (nameRejected)
			{
				//one real VerifyPermission call keeps VPP's single mismatch toast; the rest answer false silently
				if (!nameToastSent)
				{
					pm.VerifyPermission(senderId, perm, "", false);
					nameToastSent = true;
				}
				answers.Set(perm, false);
			}
			else
			{
				answers.Set(perm, pm.VerifyPermission(senderId, perm, "", false));
			}
		}

		GetRPCManager().VSendRPC("RPC_VPPContextMenuClient", "HandlePermissions", new Param1<ref map<string,bool>>(answers), true, sender);
	}

	//-----------------------------------------------------------------
	// client handlers
	//-----------------------------------------------------------------
	void HandleResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param6<int,string,bool,string,string,int> data;
		if (!ctx.Read(data))
			return;

		if (data.param4 != "")
			ClientNotify(data.param4, data.param5);

		Event_OnResult.Invoke(data.param1, data.param2, data.param3, data.param4, data.param5, data.param6);
	}

	void HandleState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param3<int,string,ref map<string,string>> data;
		if (!ctx.Read(data))
			return;

		map<string,string> state = data.param3;
		if (!state)
			state = new map<string,string>;

		Event_OnState.Invoke(data.param1, data.param2, state);
	}

	void HandlePermissions(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<ref map<string,bool>> data;
		if (!ctx.Read(data))
			return;

		m_PermCache = new map<string,bool>;
		if (data.param1)
			m_PermCache.Copy(data.param1);

		Event_OnPermissions.Invoke();
	}

	//client (every player, admin or not): a server-side ammo change made outside the weapon FSM (VPPCA_WeaponOps.SyncMagazineAmmo). Idempotent.
	void SyncMagazineAmmo(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;

		Param1<int> data;
		if (!ctx.Read(data))
			return;

		Magazine mag = Magazine.Cast(target);
		if (!mag)
			return;

		int count = data.param1;
		if (count < 0)
			count = 0;

		int maxCount = mag.GetAmmoMax();
		if (count > maxCount)
			count = maxCount;

		if (mag.GetAmmoCount() != count)
			mag.LocalSetAmmoCount(count);
	}

	//-----------------------------------------------------------------
	// server internals
	//-----------------------------------------------------------------
	protected void SendResult(PlayerIdentity sender, int seq, string actionId, VPPContextResult result)
	{
		if (!sender || !result)
			return;

		string key = result.MessageKey;
		if (result.Silent)
			key = "";

		Param6<int,string,bool,string,string,int> reply = new Param6<int,string,bool,string,string,int>(seq, actionId, result.Success, key, result.MessageArg, result.NewState);
		GetRPCManager().VSendRPC("RPC_VPPContextMenuClient", "HandleResult", reply, true, sender);
	}

	protected void FailReply(PlayerIdentity sender, VPPContextRequest req, string key, bool silent)
	{
		VPPContextResult result = new VPPContextResult();
		result.Fail(key);
		result.Silent = silent;

		int seq = 0;
		string actionId = "";
		if (req)
		{
			seq = req.Seq;
			actionId = req.ActionId;
		}
		SendResult(sender, seq, actionId, result);
	}

	//GetPlayerBaseByID also matches the HASHED id, and VerifyPermission skips the level check for a
	//hashed target: an id that resolves to an online player must be that player's steam64
	protected bool IsCanonicalPlayerId(PermissionManager pm, string playerId)
	{
		if (playerId == "")
			return false;
		if (!pm)
			return true;

		PlayerBase pb = pm.GetPlayerBaseByID(playerId);
		if (!pb)
			return true;

		string steam64 = pb.VPlayerGetSteamId();
		if (steam64 == "")
			return true;
		return steam64 == playerId;
	}

	//VerifyPermission toasts #VSTR_ERROR_NAME_MISMATCH on the force-saved-name path regardless of sendNotify;
	//mirrors that check so batch handlers can answer false without one toast per permission
	protected bool IsSenderNameRejected(PermissionManager pm, string senderId)
	{
		if (!pm)
			return false;

		UserGroup senderGroup = pm.GetUserGroup(senderId);
		if (!senderGroup || !senderGroup.IsForceSavedName())
			return false;

		VPPUser savedUser = senderGroup.FindUser(senderId);
		PlayerBase senderPlayer = pm.GetPlayerBaseByID(senderId);
		if (savedUser && senderPlayer && savedUser.GetUserName() != senderPlayer.VPlayerGetName())
			return true;
		return false;
	}

	protected string LogPrefix(string senderName, string senderId)
	{
		string prefix = "\"" + senderName + "\" (steamid=" + senderId;
		prefix = prefix + ") [ContextMenu] ";
		return prefix;
	}

	protected void LogServer(string line)
	{
		VPPLogManager logger = GetSimpleLogger();
		if (logger)
			logger.Log(line);
	}
};

VPPContextActionManager GetVPPContextActionManager()
{
	if (!GetPluginManager())
		return null;
	return VPPContextActionManager.Cast(GetPluginManager().GetPluginByType(VPPContextActionManager));
}
