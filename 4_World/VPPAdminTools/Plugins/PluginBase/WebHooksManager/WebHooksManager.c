/*
	Webhooks: events (VPPWebhookDefs) rendered through per-webhook templates and sent through the engine's RestApi.

	- Producers call PostData(<MessageType>, message) as before (the message becomes an event through ToEvent) or
	  Emit(VPPWebhookEvent) directly (other mods can emit their own event ids).
	- Each webhook has a template per event in $profile:.../WebHooksManager/Templates/<id>/<event>.tpl, created from
	  its preset when missing, so server owners can also edit them in a text editor (ReloadTemplates picks changes up).
	- Delivery: VPPWebhookSender (rate limit, retries, stats), pumped here once a second.
	- Old configs (v0: per-category switches + "simplified") are migrated to events + the Discord preset with the same
	  switches, so nothing changes until an admin edits a template.
*/
class WebHooksManager: ConfigurablePlugin
{
	const static string ROOT_DIR = "$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager";
	const static string TEMPLATE_DIR = "$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager/Templates";
	const static string TEMPLATE_EXT = ".tpl";

	private ref array<ref WebHook> 		 M_DATA;

	[NonSerialized()]
	protected ref map<string, ref VPPWebhookSender> m_Senders;
	// senders of deleted / replaced webhooks: kept until the engine answered their requests
	[NonSerialized()]
	protected ref array<ref VPPWebhookSender> m_Retired;
	// next server status time (GetGame().GetTime()) per webhook id
	[NonSerialized()]
	protected ref map<string, int> m_StatusDue;
	[NonSerialized()]
	protected bool m_Ticking;
	[NonSerialized()]
	protected int m_IdSerial;
	// a preset template hash changed (WebHooks.json needs saving)
	[NonSerialized()]
	protected bool m_TemplatesDirty;

	void WebHooksManager()
	{
		M_DATA = new array<ref WebHook>;
		m_Senders = new map<string, ref VPPWebhookSender>();
		m_Retired = new array<ref VPPWebhookSender>();
		m_StatusDue = new map<string, int>();

		if (!FileExist(ROOT_DIR))
			MakeDirectory(ROOT_DIR);

		JSONPATH = ROOT_DIR + "/WebHooks.json";

		GetRPCManager().AddRPC("RPC_WebHooksManager", "GetWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "DeleteWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "CreateWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "EditWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "GetTemplate", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "ValidateTemplate", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "SaveTemplate", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "TestSend", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "ReloadTemplates", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "SaveWebhook", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "DeleteWebhookById", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "DuplicateWebhook", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "ResetTemplate", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "GetWebhookStats", this, SingleplayerExecutionType.Server);
	}

	void ~WebHooksManager()
	{
		if (GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.Tick);
		}
	}

	override void OnInit()
	{
		Load();
		if (!GetGame().IsServer())
		{
			return;
		}

		bool migrated = false;
		foreach (WebHook hook : M_DATA)
		{
			if (hook && hook.Migrate(NextSerial()))
			{
				migrated = true;
			}
		}

		if (migrated)
		{
			Save();
			Print("[WebHooksManager] Webhooks migrated to templated events (v" + WebHook.VERSION.ToString() + ")");
		}

		WriteReadme();
		foreach (WebHook loaded : M_DATA)
		{
			if (loaded)
			{
				StartSender(loaded);
			}
		}

		SaveIfTemplatesChanged();

		if (!m_Ticking)
		{
			m_Ticking = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.Tick, 1000, true);
		}
	}

	override void Load()
	{
		if (FileExist( JSONPATH ))
		{
			JsonFileLoader<WebHooksManager>.JsonLoadFile(JSONPATH, this);
			Print("[WebHooksManager] Loading Saved Webhooks configuration");
		}

		if (!M_DATA)
		{
			M_DATA = new array<ref WebHook>;
		}
	}

	override void Save()
	{
		JsonFileLoader<WebHooksManager>.JsonSaveFile(JSONPATH, this);
	}

	protected int NextSerial()
	{
		m_IdSerial++;
		return m_IdSerial;
	}

	// ---------------------------------------------------------------- events

	/*
		Called from many other instances such as admin logs and vppat plugins
	*/
	void PostData(typename messageType, WebHookMessageBase dataClass)
	{
		if (!dataClass)
		{
			return;
		}

		VPPWebhookEvent webhookEvent = dataClass.ToEvent();
		if (webhookEvent)
		{
			Emit(webhookEvent);
		}
	}

	void PostServerBootup()
	{
		VPPWebhookEvent bootEvent = new VPPWebhookEvent(VPPWebhookDefs.EV_BOOT);
		Emit(bootEvent);
	}

	// Sends an event to every webhook that has it enabled and whose filters let it through.
	void Emit(VPPWebhookEvent webhookEvent)
	{
		if (!webhookEvent || !CanSend() || !M_DATA || M_DATA.Count() == 0)
		{
			return;
		}

		FillCommonVars(webhookEvent);
		foreach (WebHook hook : M_DATA)
		{
			if (!hook || !hook.IsEventEnabled(webhookEvent.Id))
			{
				continue;
			}

			VPPWebhookEventCfg cfg = hook.GetEventCfg(webhookEvent.Id);
			if (!Passes(cfg, webhookEvent))
			{
				continue;
			}

			SendTo(hook, webhookEvent, "", 0);
		}
	}

	// false when the webhook has no sender or no usable template for the event (nothing is sent)
	protected bool SendTo(WebHook hook, VPPWebhookEvent webhookEvent, string testBy, int testReqId)
	{
		VPPWebhookSender sender = m_Senders.Get(hook.m_Id);
		if (!sender)
		{
			return false;
		}

		string body = sender.Render(webhookEvent);
		if (body == "")
		{
			return false;
		}

		sender.Enqueue(webhookEvent.Id, body, testBy, testReqId);
		return true;
	}

	protected bool CanSend()
	{
		return GetGame().IsServer() && GetGame().IsDedicatedServer();
	}

	// server / players / time variables every event has (a producer's own values win)
	protected void FillCommonVars(VPPWebhookEvent webhookEvent)
	{
		map<string, string> common = new map<string, string>();
		common.Set("event", webhookEvent.Id);
		string serverName = g_Game.GetServerName();
		common.Set("server.name", serverName);
		string serverIp = g_Game.VGetServerIP();
		common.Set("server.ip", serverIp);
		int queryPort = GetGame().ServerConfigGetInt("steamQueryPort");
		common.Set("server.port", queryPort.ToString());
		string worldName = "";
		GetGame().GetWorldName(worldName);
		worldName.ToLower();
		common.Set("server.map", worldName);
		string playerCount = "0";
		if (GetSteamAPIManager())
		{
			playerCount = GetSteamAPIManager().GetTotalPlayerCount();
		}

		common.Set("players", playerCount);
		VPPWebhookClock.Fill(common);
		for (int i = 0; i < common.Count(); i++)
		{
			string commonName = common.GetKey(i);
			if (!webhookEvent.Vars.Contains(commonName))
			{
				string commonValue = common.GetElement(i);
				webhookEvent.Set(commonName, commonValue);
			}
		}
	}

	// The per event filters of a webhook.
	protected bool Passes(VPPWebhookEventCfg cfg, VPPWebhookEvent webhookEvent)
	{
		if (!cfg)
		{
			return true;
		}

		string eventId = webhookEvent.Id;
		if (eventId == VPPWebhookDefs.EV_KILL || eventId == VPPWebhookDefs.EV_HIT)
		{
			string kind = webhookEvent.Get("cause");
			if (eventId == VPPWebhookDefs.EV_HIT)
			{
				kind = webhookEvent.Get("source.kind");
			}

			if (cfg.PvPOnly && kind != "player")
			{
				return false;
			}

			if (cfg.SkipAI && (kind == "infected" || kind == "animal"))
			{
				return false;
			}

			if (cfg.MinDistance > 0)
			{
				string distanceText = webhookEvent.Get("distance");
				float distance = distanceText.ToFloat();
				if (distanceText == "" || distance < cfg.MinDistance)
				{
					return false;
				}
			}

			return true;
		}

		if (eventId == VPPWebhookDefs.EV_ADMIN)
		{
			string moduleName = webhookEvent.Get("module");
			moduleName.ToLower();
			if (cfg.ExcludedModules && cfg.ExcludedModules.Find(moduleName) >= 0)
			{
				return false;
			}

			if (cfg.Modules && cfg.Modules.Count() > 0 && cfg.Modules.Find(moduleName) < 0)
			{
				return false;
			}
		}

		return true;
	}

	// ---------------------------------------------------------------- senders, templates

	protected void StartSender(WebHook hook)
	{
		VPPWebhookSender sender = new VPPWebhookSender(hook);
		m_Senders.Set(hook.m_Id, sender);
		LoadTemplates(hook, sender, true);
		int due = GetGame().GetTime() + hook.GetServerStatsInterval() * 1000;
		m_StatusDue.Set(hook.m_Id, due);
	}

	// The old sender keeps living until its requests are answered (the engine holds its callbacks).
	protected void RetireSender(string hookId)
	{
		VPPWebhookSender sender = m_Senders.Get(hookId);
		if (sender)
		{
			m_Retired.Insert(sender);
		}

		m_Senders.Remove(hookId);
		m_StatusDue.Remove(hookId);
	}

	string TemplatePath(WebHook hook, string eventId)
	{
		return TEMPLATE_DIR + "/" + hook.m_Id + "/" + eventId + TEMPLATE_EXT;
	}

	// Reads every event template of the webhook (missing files are written from the preset first) and compiles it.
	// Returns how many templates have errors (they are logged; such an event sends nothing until fixed).
	protected int LoadTemplates(WebHook hook, VPPWebhookSender sender, bool logIssues)
	{
		EnsureHookDir(hook);
		int broken = 0;
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			string path = TemplatePath(hook, def.Id);
			RefreshPresetTemplate(hook, def.Id, false);
			string text = "";
			VPPXmlText.ReadAll(path, text);
			VPPWebhookTemplate tpl = new VPPWebhookTemplate();
			tpl.Parse(text, def);
			sender.SetTemplate(def.Id, tpl);
			if (tpl.HasErrors())
			{
				broken++;
				if (logIssues)
				{
					LogIssues(hook, def.Id, tpl);
				}
			}
		}

		return broken;
	}

	protected void LogIssues(WebHook hook, string eventId, VPPWebhookTemplate tpl)
	{
		string hookName = hook.GetName();
		foreach (VPPWebhookIssue issue : tpl.Issues)
		{
			string where = eventId + TEMPLATE_EXT + ":" + issue.Line.ToString() + ":" + issue.Col.ToString();
			Print("[WebHooksManager] \"" + hookName + "\" " + where + " " + issue.Message);
		}
	}

	protected void EnsureHookDir(WebHook hook)
	{
		if (!FileExist(TEMPLATE_DIR))
		{
			MakeDirectory(TEMPLATE_DIR);
		}

		string hookDir = TEMPLATE_DIR + "/" + hook.m_Id;
		if (!FileExist(hookDir))
		{
			MakeDirectory(hookDir);
		}
	}

	// (Re)writes every template of the webhook from its preset.
	protected void WritePresetTemplates(WebHook hook)
	{
		EnsureHookDir(hook);
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			RefreshPresetTemplate(hook, def.Id, true);
		}
	}

	// Writes the preset template of an event when the file is missing, when force is set, or when the file is still
	// exactly the preset text this server wrote earlier (so preset fixes reach untouched files). A file an admin
	// edited, or one written before hashes were tracked that differs from the preset, is left alone.
	protected void RefreshPresetTemplate(WebHook hook, string eventId, bool force)
	{
		string path = TemplatePath(hook, eventId);
		string presetText = VPPWebhookPresets.TemplateFor(hook.m_Preset, eventId);
		int presetHash = VPPXmlText.NormalizedHash(presetText);
		string current = "";
		int currentHash = 0;
		int writtenHash = 0;
		bool known = hook.m_PresetHashes.Contains(eventId);
		if (known)
		{
			writtenHash = hook.m_PresetHashes.Get(eventId);
		}

		if (!force && FileExist(path))
		{
			VPPXmlText.ReadAll(path, current);
			currentHash = VPPXmlText.NormalizedHash(current);
			if (currentHash == presetHash)
			{
				if (!known || writtenHash != presetHash)
				{
					hook.m_PresetHashes.Set(eventId, presetHash);
					m_TemplatesDirty = true;
				}

				return;
			}

			if (!known || writtenHash != currentHash)
			{
				return;
			}

			string hookName = hook.GetName();
			Print("[WebHooksManager] \"" + hookName + "\" " + eventId + TEMPLATE_EXT + " updated to the current preset");
		}

		VPPXmlText.WriteAll(path, presetText);
		hook.m_PresetHashes.Set(eventId, presetHash);
		m_TemplatesDirty = true;
	}

	protected void SaveIfTemplatesChanged()
	{
		if (!m_TemplatesDirty)
		{
			return;
		}

		m_TemplatesDirty = false;
		Save();
	}

	// Templates/README.txt: the syntax, the webhooks' folders and every event's variables.
	protected void WriteReadme()
	{
		if (!FileExist(TEMPLATE_DIR))
		{
			MakeDirectory(TEMPLATE_DIR);
		}

		string nl = "\n";
		string text = "VPP Admin Tools webhook templates" + nl + "=================================" + nl + nl;
		text = text + "Each webhook has a folder named after its id with one <event>.tpl per event: the request body sent for it." + nl;
		text = text + "Edit them here or in the WebHooks menu; the server reads them at start (or on Reload in the menu)." + nl;
		text = text + "Delete a file to get the webhook's preset template back." + nl + nl;
		text = text + "Syntax:" + nl;
		text = text + "  {{victim.name}}                        a variable (JSON-escaped for JSON webhooks)" + nl;
		text = text + "  {{victim.id|steamurl}}                 filters: upper lower trim truncate:N default:TEXT round:N number steamurl raw json" + nl;
		text = text + "  {{#if killer.id}} ... {{else}} ... {{/if}}   shown when the variable is not empty" + nl;
		text = text + "  {{#unless killer.id}} ... {{/unless}}       shown when it is empty" + nl;
		text = text + "  {{#ifeq cause suicide}} ... {{/ifeq}}       shown when it equals the value" + nl;
		text = text + "  {{! comment }}" + nl;
		text = text + "  {{hook.<name>}}                        a custom variable of the webhook (username, avatar, chat_id, ...)" + nl + nl;
		text = text + "Webhooks:" + nl;
		foreach (WebHook hook : M_DATA)
		{
			if (hook)
			{
				string hookName = hook.GetName();
				text = text + "  " + hook.m_Id + "  " + hookName + "  (" + hook.m_Preset + ")" + nl;
			}
		}

		text = text + nl + "Events and their variables:" + nl;
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			text = text + nl + "  " + def.Id + nl;
			foreach (VPPWebhookVar webhookVar : def.Vars)
			{
				text = text + "    {{" + webhookVar.Name + "}}  e.g. " + webhookVar.Sample + nl;
			}
		}

		string readmePath = TEMPLATE_DIR + "/README.txt";
		VPPXmlText.WriteAll(readmePath, text);
	}

	// ---------------------------------------------------------------- tick

	void Tick()
	{
		int now = GetGame().GetTime();
		foreach (WebHook hook : M_DATA)
		{
			if (!hook)
			{
				continue;
			}

			VPPWebhookSender sender = m_Senders.Get(hook.m_Id);
			if (!sender)
			{
				continue;
			}

			if (hook.IsEventEnabled(VPPWebhookDefs.EV_STATUS) && m_StatusDue.Contains(hook.m_Id) && now >= m_StatusDue.Get(hook.m_Id))
			{
				int nextDue = now + hook.GetServerStatsInterval() * 1000;
				m_StatusDue.Set(hook.m_Id, nextDue);
				PostStatusTo(hook);
			}

			sender.Pump();
		}

		for (int r = m_Retired.Count() - 1; r >= 0; r--)
		{
			VPPWebhookSender retired = m_Retired[r];
			retired.Pump();
			if (retired.PendingRequests() == 0)
			{
				m_Retired.Remove(r);
			}
		}
	}

	protected void PostStatusTo(WebHook hook)
	{
		if (!CanSend())
		{
			return;
		}

		ServerStatusMessage status = new ServerStatusMessage();
		VPPWebhookEvent statusEvent = status.ToEvent();
		FillCommonVars(statusEvent);
		SendTo(hook, statusEvent, "", 0);
	}

	// ---------------------------------------------------------------- menu RPCs (list / create / edit / delete)

	protected bool CanViewUrls(string plainId)
	{
		return GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:ViewURL", "", false);
	}

	protected void SendList(PlayerIdentity sender)
	{
		string plainId = sender.GetPlainId();
		bool showUrl = CanViewUrls(plainId);
		array<ref WebHook> copies = new array<ref WebHook>();
		foreach (WebHook hook : M_DATA)
		{
			if (hook)
			{
				WebHook copy = hook.CopyForClient(showUrl);
				copies.Insert(copy);
			}
		}

		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(copies), true, sender);
		int access = AccessMask(plainId);
		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnAccess", new Param1<int>(access), true, sender);
	}

	// What the admin may do in the menu (VPPWebhookAccess bits), so it can disable what the server would refuse.
	protected int AccessMask(string plainId)
	{
		int access = 0;
		PermissionManager perms = GetPermissionManager();
		if (perms.VerifyPermission(plainId, "MenuWebHooks:Create", "", false))
		{
			access = access | VPPWebhookAccess.CREATE;
		}

		if (perms.VerifyPermission(plainId, "MenuWebHooks:Edit", "", false))
		{
			access = access | VPPWebhookAccess.EDIT;
		}

		if (perms.VerifyPermission(plainId, "MenuWebHooks:Delete", "", false))
		{
			access = access | VPPWebhookAccess.DELETE;
		}

		if (perms.VerifyPermission(plainId, "MenuWebHooks:EditTemplates", "", false))
		{
			access = access | VPPWebhookAccess.EDIT_TEMPLATES;
		}

		if (perms.VerifyPermission(plainId, "MenuWebHooks:TestSend", "", false))
		{
			access = access | VPPWebhookAccess.TEST_SEND;
		}

		if (perms.VerifyPermission(plainId, "MenuWebHooks:ViewURL", "", false))
		{
			access = access | VPPWebhookAccess.VIEW_URL;
		}

		return access;
	}

	void GetWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks", "", false))
				return;

			SendList(sender);
		}
	}

	void DeleteWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param2<int,string> data;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Delete"))
				return;

			if (!ctx.Read(data))
				return;

			if (data.param1 >= 0 && data.param1 < M_DATA.Count() && M_DATA.Get(data.param1).GetName() == data.param2)
			{
				WebHook removed = M_DATA.Get(data.param1);
				string removedName = removed.GetName();
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Deleted WebHook: " + removedName));
				RetireSender(removed.m_Id);
				M_DATA.RemoveOrdered(data.param1);
				Save();
				WriteReadme();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: "+data.param2 + " was successfully deleted!", NotifyTypes.NOTIFY);
				SendList(sender);
			}
			else
			{
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Error deleting webhook: "+data.param2 + "\nobject not found in array!", NotifyTypes.NOTIFY);
			}
		}
	}

	void CreateWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param1<ref WebHook> data;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Create"))
				return;

			if (!ctx.Read(data))
				return;

			WebHook webHook = data.param1;
			if (!webHook)
				return;

			string createdUrl = webHook.GetURL();
			if (createdUrl == "" || WebHook.IsMasked(createdUrl))
				return;

			// a fresh webhook: the preset follows the menu's "simplified" switch, the events its category switches
			webHook.m_Id = "";
			webHook.m_Version = 0;
			webHook.m_Preset = "";
			webHook.m_ContentType = "";
			webHook.Migrate(NextSerial());
			M_DATA.Insert(webHook);
			WritePresetTemplates(webHook);
			StartSender(webHook);

			string createdName = webHook.GetName();
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Created new WebHook: " + createdName));
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"New WebHook: " + createdName + " successfully created and saved!", NotifyTypes.NOTIFY);
			Save();
			WriteReadme();
			SendList(sender);
		}
	}

	// The current menu edits name, URL, the category switches, the status interval and "simplified": they are merged
	// into the stored webhook, which keeps its id, templates, variables and filters.
	void EditWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param3<int,string,ref WebHook> data;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Edit"))
				return;

			if (!ctx.Read(data))
				return;

			if (data.param1 < 0 || data.param1 >= M_DATA.Count() || !data.param3)
				return;

			WebHook stored = M_DATA.Get(data.param1);
			if (!stored)
				return;

			WebHook edited = data.param3;
			string storedName = stored.GetName();
			string editedName = edited.GetName();
			if (storedName != data.param2)
			{
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Failed to save changes to webhook: " + editedName + "\n\nCould not find old instance, another admin deleted it?", NotifyTypes.NOTIFY);
				return;
			}

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Edited WebHook: " + storedName));
			stored.SetName(editedName);
			string newUrl = edited.GetURL();
			if (newUrl != "" && !WebHook.IsMasked(newUrl))
				stored.SetURL(newUrl);

			stored.m_deathKillLogs = edited.m_deathKillLogs;
			stored.m_adminActivityLog = edited.m_adminActivityLog;
			stored.m_admHitLog = edited.m_admHitLog;
			stored.m_joinLeaveLog = edited.m_joinLeaveLog;
			stored.m_serverStatusLog = edited.m_serverStatusLog;
			VPPWebHookServerStatsTime editedInterval = edited.GetServerStatsInterval();
			stored.SetServerStatsInterval(editedInterval);
			stored.SyncEventsFromSwitches();
			bool presetChanged = false;
			if (stored.m_simplifiedMessages != edited.m_simplifiedMessages && VPPWebhookPresets.IsDiscord(stored.m_Preset))
			{
				stored.m_Preset = VPPWebhookPresets.DISCORD_EMBED;
				if (edited.m_simplifiedMessages)
					stored.m_Preset = VPPWebhookPresets.DISCORD_SIMPLE;

				presetChanged = true;
			}

			stored.m_simplifiedMessages = edited.m_simplifiedMessages;
			stored.Migrate(NextSerial());
			if (presetChanged)
				WritePresetTemplates(stored);

			RetireSender(stored.m_Id);
			StartSender(stored);
			Save();
			WriteReadme();
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: " + editedName + " was successfully edited & saved!", NotifyTypes.NOTIFY);
			SendList(sender);
		}
	}

	// ---------------------------------------------------------------- webhook RPCs of the new menu

	protected void ReplySaved(PlayerIdentity sender, int reqId, bool ok, string hookId, string error)
	{
		VPPWebhookSaveReply reply = new VPPWebhookSaveReply();
		reply.ReqId = reqId;
		reply.Ok = ok;
		reply.HookId = hookId;
		reply.Error = error;
		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnWebhookSaved", new Param1<ref VPPWebhookSaveReply>(reply), true, sender);
	}

	protected bool NameInUse(string name, string exceptId)
	{
		string lowerName = name;
		lowerName.ToLower();
		foreach (WebHook hook : M_DATA)
		{
			if (!hook || hook.m_Id == exceptId)
			{
				continue;
			}

			string otherName = hook.GetName();
			otherName.ToLower();
			if (otherName == lowerName)
			{
				return true;
			}
		}

		return false;
	}

	// Creates (empty or unknown m_Id) or updates a webhook with everything the menu edits. A changed preset rewrites
	// the webhook's templates from the new preset; the legacy switches are kept in step for older tools.
	void SaveWebhook(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param2<int, ref WebHook> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		WebHook incoming = data.param2;
		int reqId = data.param1;
		if (!incoming)
		{
			return;
		}

		WebHook stored = FindHook(incoming.m_Id);
		bool creating = false;
		if (!stored)
		{
			creating = true;
		}

		string permission = "MenuWebHooks:Edit";
		if (creating)
		{
			permission = "MenuWebHooks:Create";
		}

		if (!GetPermissionManager().VerifyPermission(plainId, permission))
		{
			return;
		}

		string newName = incoming.GetName();
		newName = newName.Trim();
		if (newName.Length() < 3)
		{
			ReplySaved(sender, reqId, false, incoming.m_Id, "NAME_SHORT");
			return;
		}

		string exceptId = "";
		if (stored)
		{
			exceptId = stored.m_Id;
		}

		if (NameInUse(newName, exceptId))
		{
			ReplySaved(sender, reqId, false, incoming.m_Id, "NAME_IN_USE");
			return;
		}

		string newUrl = incoming.GetURL();
		newUrl = newUrl.Trim();
		bool urlGiven = newUrl != "" && !WebHook.IsMasked(newUrl);
		if (creating && !urlGiven)
		{
			ReplySaved(sender, reqId, false, "", "URL_MISSING");
			return;
		}

		string newPreset = incoming.m_Preset;
		if (!VPPWebhookPresets.IsPreset(newPreset))
		{
			newPreset = VPPWebhookPresets.DISCORD_EMBED;
		}

		if (creating)
		{
			stored = new WebHook(newName, newUrl);
			stored.m_Preset = newPreset;
			stored.Migrate(NextSerial());
			M_DATA.Insert(stored);
		}

		bool presetChanged = stored.m_Preset != newPreset;
		stored.SetName(newName);
		if (urlGiven)
		{
			stored.SetURL(newUrl);
		}

		stored.m_Preset = newPreset;
		string contentType = incoming.m_ContentType;
		contentType = contentType.Trim();
		if (contentType == "")
		{
			contentType = VPPWebhookPresets.ContentTypeOf(newPreset);
		}

		stored.m_ContentType = contentType;
		int rateLimit = incoming.m_RateLimit;
		if (rateLimit < 0)
		{
			rateLimit = 0;
		}

		if (rateLimit > 600)
		{
			rateLimit = 600;
		}

		stored.m_RateLimit = rateLimit;
		stored.m_Disabled = incoming.m_Disabled;
		stored.m_HideIds = incoming.m_HideIds;
		stored.m_HideServerAddress = incoming.m_HideServerAddress;
		VPPWebHookServerStatsTime interval = ValidInterval(incoming.m_serverStatsInterval);
		stored.SetServerStatsInterval(interval);
		CopyVars(incoming, stored);
		CopyEvents(incoming, stored);
		stored.SyncSwitchesFromEvents();
		if (creating || presetChanged)
		{
			WritePresetTemplates(stored);
		}

		RetireSender(stored.m_Id);
		StartSender(stored);
		Save();
		SaveIfTemplatesChanged();
		WriteReadme();
		string verb = "Edited";
		if (creating)
		{
			verb = "Created";
		}

		string logText = "[WebHooksManager] " + verb + " WebHook: " + newName;
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(plainId, sender.GetName(), logText));
		ReplySaved(sender, reqId, true, stored.m_Id, "");
		SendList(sender);
	}

	protected VPPWebHookServerStatsTime ValidInterval(VPPWebHookServerStatsTime interval)
	{
		if (interval == VPPWebHookServerStatsTime.ONE_MINUTE || interval == VPPWebHookServerStatsTime.FIVE_MINUTES)
		{
			return interval;
		}

		if (interval == VPPWebHookServerStatsTime.TEN_MINUTES || interval == VPPWebHookServerStatsTime.FIFTEEN_MINUTES)
		{
			return interval;
		}

		return VPPWebHookServerStatsTime.FIVE_MINUTES;
	}

	// hook.* variables: plain names only (letters, digits, _ and .), at most 32, values up to 500 characters.
	protected void CopyVars(WebHook source, WebHook target)
	{
		target.m_Vars.Clear();
		if (!source.m_Vars)
		{
			return;
		}

		int total = source.m_Vars.Count();
		for (int i = 0; i < total; i++)
		{
			if (target.m_Vars.Count() >= 32)
			{
				break;
			}

			string varName = source.m_Vars.GetKey(i);
			string varValue = source.m_Vars.GetElement(i);
			varName = varName.Trim();
			if (!VPPWebhookVarNames.IsPlain(varName))
			{
				continue;
			}

			if (varValue.Length() > 500)
			{
				varValue = varValue.Substring(0, 500);
			}

			target.m_Vars.Set(varName, varValue);
		}
	}

	// Event settings for the known events (unknown ids are ignored); module names are kept lower case and trimmed.
	protected void CopyEvents(WebHook source, WebHook target)
	{
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			VPPWebhookEventCfg sourceCfg = null;
			if (source.m_Events)
			{
				sourceCfg = source.m_Events.Get(def.Id);
			}

			VPPWebhookEventCfg cfg = target.m_Events.Get(def.Id);
			if (!cfg)
			{
				cfg = new VPPWebhookEventCfg();
				target.m_Events.Set(def.Id, cfg);
			}

			if (!sourceCfg)
			{
				continue;
			}

			cfg.Enabled = sourceCfg.Enabled;
			cfg.PvPOnly = sourceCfg.PvPOnly;
			cfg.SkipAI = sourceCfg.SkipAI;
			cfg.MinDistance = Math.Clamp(sourceCfg.MinDistance, 0, 20000);
			CopyModules(sourceCfg.Modules, cfg.Modules);
			CopyModules(sourceCfg.ExcludedModules, cfg.ExcludedModules);
		}
	}

	protected void CopyModules(array<string> source, array<string> target)
	{
		target.Clear();
		if (!source)
		{
			return;
		}

		foreach (string moduleName : source)
		{
			string cleanName = moduleName.Trim();
			cleanName.ToLower();
			if (cleanName == "" || target.Find(cleanName) >= 0 || target.Count() >= 50)
			{
				continue;
			}

			target.Insert(cleanName);
		}
	}

	void DeleteWebhookById(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param1<string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:Delete"))
		{
			return;
		}

		int index = -1;
		for (int i = 0; i < M_DATA.Count(); i++)
		{
			if (M_DATA[i] && M_DATA[i].m_Id == data.param1)
			{
				index = i;
				break;
			}
		}

		if (index < 0)
		{
			SendList(sender);
			return;
		}

		WebHook removed = M_DATA[index];
		string removedName = removed.GetName();
		RetireSender(removed.m_Id);
		M_DATA.RemoveOrdered(index);
		Save();
		WriteReadme();
		string logText = "[WebHooksManager] Deleted WebHook: " + removedName;
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(plainId, sender.GetName(), logText));
		SendList(sender);
	}

	// A copy with a new id, " (copy)" name and copies of all its template files.
	void DuplicateWebhook(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param2<int, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:Create"))
		{
			return;
		}

		WebHook source = FindHook(data.param2);
		if (!source)
		{
			ReplySaved(sender, data.param1, false, data.param2, "NOT_FOUND");
			return;
		}

		WebHook copy = source.CopyForClient(true);
		copy.m_Id = WebHook.NewId(NextSerial());
		string baseName = source.GetName() + " (copy)";
		string copyName = baseName;
		int attempt = 2;
		while (NameInUse(copyName, ""))
		{
			copyName = baseName + " " + attempt.ToString();
			attempt++;
		}

		copy.SetName(copyName);
		copy.m_PresetHashes.Copy(source.m_PresetHashes);
		EnsureHookDir(copy);
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			string fromPath = TemplatePath(source, def.Id);
			string toPath = TemplatePath(copy, def.Id);
			string text = "";
			bool readOk = false;
			if (FileExist(fromPath))
			{
				readOk = VPPXmlText.ReadAll(fromPath, text);
			}

			if (readOk)
			{
				VPPXmlText.WriteAll(toPath, text);
			}
		}

		M_DATA.Insert(copy);
		StartSender(copy);
		Save();
		SaveIfTemplatesChanged();
		WriteReadme();
		string logText = "[WebHooksManager] Duplicated WebHook: " + copyName;
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(plainId, sender.GetName(), logText));
		ReplySaved(sender, data.param1, true, copy.m_Id, "");
		SendList(sender);
	}

	// Puts one event template back to the webhook's preset and answers like GetTemplate.
	void ResetTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param3<int, string, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:EditTemplates"))
		{
			return;
		}

		WebHook hook = FindHook(data.param2);
		VPPWebhookEventDef def = VPPWebhookDefs.Get(data.param3);
		if (!hook || !def)
		{
			return;
		}

		EnsureHookDir(hook);
		RefreshPresetTemplate(hook, def.Id, true);
		string text = "";
		string path = TemplatePath(hook, def.Id);
		VPPXmlText.ReadAll(path, text);
		VPPWebhookSender hookSender = m_Senders.Get(hook.m_Id);
		if (hookSender)
		{
			VPPWebhookTemplate tpl = new VPPWebhookTemplate();
			tpl.Parse(text, def);
			hookSender.SetTemplate(def.Id, tpl);
		}

		SaveIfTemplatesChanged();
		VPPWebhookTemplateReply reply = new VPPWebhookTemplateReply();
		reply.ReqId = data.param1;
		reply.HookId = hook.m_Id;
		reply.EventId = def.Id;
		reply.Text = text;
		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnTemplate", new Param1<ref VPPWebhookTemplateReply>(reply), true, sender);
	}

	void GetWebhookStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuWebHooks", "", false))
		{
			return;
		}

		array<ref VPPWebhookStatsWire> rows = new array<ref VPPWebhookStatsWire>();
		foreach (WebHook hook : M_DATA)
		{
			if (!hook)
			{
				continue;
			}

			VPPWebhookSender hookSender = m_Senders.Get(hook.m_Id);
			if (!hookSender)
			{
				continue;
			}

			VPPWebhookStatsWire row = new VPPWebhookStatsWire();
			row.HookId = hook.m_Id;
			row.Queued = hookSender.Stats.Queued;
			row.Delivered = hookSender.Stats.Delivered;
			row.NoReply = hookSender.Stats.NoReply;
			row.Failed = hookSender.Stats.Failed;
			row.Dropped = hookSender.Stats.Dropped;
			row.Waiting = hookSender.QueueCount();
			row.ConsecutiveFailures = hookSender.Stats.ConsecutiveFailures;
			row.LastError = hookSender.Stats.LastError;
			row.LastErrorTime = hookSender.Stats.LastErrorTime;
			row.LastSuccessTime = hookSender.Stats.LastSuccessTime;
			rows.Insert(row);
		}

		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnStats", new Param1<ref array<ref VPPWebhookStatsWire>>(rows), true, sender);
	}

	// ---------------------------------------------------------------- template RPCs (webhook editor)

	protected WebHook FindHook(string hookId)
	{
		foreach (WebHook hook : M_DATA)
		{
			if (hook && hook.m_Id == hookId)
			{
				return hook;
			}
		}

		return null;
	}

	void GetTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param3<int, string, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuWebHooks:EditTemplates"))
		{
			return;
		}

		WebHook hook = FindHook(data.param2);
		if (!hook || !VPPWebhookDefs.Get(data.param3))
		{
			return;
		}

		VPPWebhookTemplateReply reply = new VPPWebhookTemplateReply();
		reply.ReqId = data.param1;
		reply.HookId = data.param2;
		reply.EventId = data.param3;
		string text = "";
		string path = TemplatePath(hook, data.param3);
		VPPXmlText.ReadAll(path, text);
		reply.Text = text;
		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnTemplate", new Param1<ref VPPWebhookTemplateReply>(reply), true, sender);
	}

	void ValidateTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		HandleTemplate(type, ctx, sender, false);
	}

	void SaveTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		HandleTemplate(type, ctx, sender, true);
	}

	// Checks a template like the server will use it: template problems, then rendered with sample values, the JSON
	// verdict of the game's parser (+ where) and the service's limits. save: also writes it when it has no errors.
	protected void HandleTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, bool save)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param4<int, string, string, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:EditTemplates"))
		{
			return;
		}

		WebHook hook = FindHook(data.param2);
		VPPWebhookEventDef def = VPPWebhookDefs.Get(data.param3);
		if (!hook || !def)
		{
			return;
		}

		VPPWebhookCheckReply reply = CheckTemplate(hook, def, data.param4);
		reply.ReqId = data.param1;
		if (save && reply.TemplateOk)
		{
			EnsureHookDir(hook);
			string path = TemplatePath(hook, def.Id);
			VPPXmlText.WriteAll(path, data.param4);
			VPPWebhookSender hookSender = m_Senders.Get(hook.m_Id);
			if (hookSender)
			{
				VPPWebhookTemplate tpl = new VPPWebhookTemplate();
				tpl.Parse(data.param4, def);
				hookSender.SetTemplate(def.Id, tpl);
			}

			reply.Saved = true;
			string hookName = hook.GetName();
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(plainId, sender.GetName(), "[WebHooksManager] Edited the " + def.Id + " template of WebHook: " + hookName));
		}

		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnTemplateCheck", new Param1<ref VPPWebhookCheckReply>(reply), true, sender);
	}

	VPPWebhookCheckReply CheckTemplate(WebHook hook, VPPWebhookEventDef def, string text)
	{
		VPPWebhookCheckReply reply = new VPPWebhookCheckReply();
		reply.HookId = hook.m_Id;
		reply.EventId = def.Id;
		VPPWebhookTemplate tpl = new VPPWebhookTemplate();
		tpl.Parse(text, def);
		foreach (VPPWebhookIssue issue : tpl.Issues)
		{
			VPPWebhookIssueWire wire = new VPPWebhookIssueWire();
			wire.IsError = issue.IsError;
			wire.Line = issue.Line;
			wire.Col = issue.Col;
			wire.Code = issue.Code;
			wire.Arg = issue.Arg;
			wire.Message = issue.Message;
			reply.Issues.Insert(wire);
		}

		reply.TemplateOk = !tpl.HasErrors();
		if (!reply.TemplateOk)
		{
			return reply;
		}

		map<string, string> samples = new map<string, string>();
		def.FillSamples(samples);
		for (int i = 0; i < hook.m_Vars.Count(); i++)
		{
			string hookKey = hook.m_Vars.GetKey(i);
			string hookValue = hook.m_Vars.GetElement(i);
			samples.Set("hook." + hookKey, hookValue);
		}

		bool jsonBody = hook.m_ContentType.Contains("json");
		string preview = tpl.Render(samples, jsonBody);
		reply.Preview = preview;
		if (!jsonBody)
		{
			return reply;
		}

		reply.JsonChecked = true;
		VPPJsonCheckResult verdict = VPPJsonCheck.Check(preview);
		reply.JsonValid = verdict.Valid;
		reply.JsonMessage = verdict.Message;
		reply.JsonLine = verdict.Line;
		reply.JsonCol = verdict.Col;
		if (verdict.Valid && VPPWebhookPresets.UsesDiscordLimits(hook.m_Preset))
		{
			VPPDiscordLint.Check(preview, reply.ServiceProblems);
		}

		return reply;
	}

	// Sends the event's sample message now; the answer comes back through OnTestResult.
	void TestSend(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param3<int, string, string> data;
		if (!ctx.Read(data))
		{
			return;
		}

		string plainId = sender.GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "MenuWebHooks:TestSend"))
		{
			return;
		}

		WebHook hook = FindHook(data.param2);
		VPPWebhookEventDef def = VPPWebhookDefs.Get(data.param3);
		if (!hook || !def)
		{
			return;
		}

		VPPWebhookEvent sample = new VPPWebhookEvent(def.Id);
		def.FillSamples(sample.Vars);
		FillCommonVars(sample);
		string hookName = hook.GetName();
		if (!SendTo(hook, sample, plainId, data.param1))
		{
			OnTestResult(plainId, data.param1, hook.m_Id, def.Id, false, "TEMPLATE_ERROR");
			return;
		}

		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(plainId, sender.GetName(), "[WebHooksManager] Test send of " + def.Id + " to WebHook: " + hookName));
	}

	void OnTestResult(string plainId, int reqId, string hookId, string eventId, bool ok, string reason)
	{
		PlayerIdentity admin = FindIdentity(plainId);
		if (!admin)
		{
			return;
		}

		VPPWebhookTestReply reply = new VPPWebhookTestReply();
		reply.ReqId = reqId;
		reply.HookId = hookId;
		reply.EventId = eventId;
		reply.Ok = ok;
		reply.Reason = reason;
		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnTestResult", new Param1<ref VPPWebhookTestReply>(reply), true, admin);
		string verdict = "failed: " + reason;
		if (ok)
		{
			verdict = "delivered (" + reason + ")";
		}

		GetPermissionManager().NotifyPlayer(plainId, "Webhook test " + eventId + " " + verdict, NotifyTypes.NOTIFY);
	}

	protected PlayerIdentity FindIdentity(string plainId)
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		foreach (Man player : players)
		{
			if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == plainId)
			{
				return player.GetIdentity();
			}
		}

		return null;
	}

	// Re-reads every template file (edits made on disk) and reports which webhooks have broken templates.
	void ReloadTemplates(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
		{
			return;
		}

		Param1<int> data;
		if (!ctx.Read(data))
		{
			return;
		}

		if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuWebHooks:EditTemplates"))
		{
			return;
		}

		VPPWebhookReloadReply reply = new VPPWebhookReloadReply();
		reply.ReqId = data.param1;
		foreach (WebHook hook : M_DATA)
		{
			VPPWebhookSender hookSender = m_Senders.Get(hook.m_Id);
			if (!hookSender)
			{
				continue;
			}

			int broken = LoadTemplates(hook, hookSender, true);
			reply.HookIds.Insert(hook.m_Id);
			reply.TemplateErrors.Insert(broken);
		}

		SaveIfTemplatesChanged();

		GetRPCManager().VSendRPC("RPC_MenuWebHooks", "OnTemplatesReloaded", new Param1<ref VPPWebhookReloadReply>(reply), true, sender);
	}
};

WebHooksManager GetWebHooksManager()
{
	return WebHooksManager.Cast(GetPluginManager().GetPluginByType(WebHooksManager));
};
