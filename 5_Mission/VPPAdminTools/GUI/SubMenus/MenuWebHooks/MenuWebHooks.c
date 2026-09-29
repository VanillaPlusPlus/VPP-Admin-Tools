/*
	Webhooks menu (P2): a rail of webhooks with their delivery health, and three views of the selected one:
	EVENTS (which events it sends and their filters), TEMPLATE (the body of each event, validated / previewed / test
	sent by the server) and SETTINGS (name, URL, preset, delivery, privacy, custom hook.* variables).

	The EVENTS and SETTINGS views edit m_Work, a copy of the selected webhook; SAVE sends it whole (SaveWebhook).
	Templates are saved one event at a time from the TEMPLATE view. Server replies come back on RPC_MenuWebHooks.
*/
class MenuWebHooks extends AdminHudSubMenu
{
	const static int TAB_EVENTS = 0;
	const static int TAB_TEMPLATE = 1;
	const static int TAB_SETTINGS = 2;
	const static int STATS_EVERY_MS = 5000;
	const static int RAIL_W = 290;
	const static int GAP = 8;
	const static int TOP = 44;

	protected ref array<ref WebHook> m_Hooks;
	protected ref map<string, ref VPPWebhookStatsWire> m_Stats;
	protected ref array<string> m_RowIds;
	protected ref WebHook m_Work;
	protected string m_WorkId;
	protected bool m_HasWork;
	protected bool m_WorkIsNew;
	protected bool m_WorkDirty;
	protected int m_Access;
	protected bool m_AccessKnown;
	protected int m_Tab;
	protected int m_ReqSerial;
	protected int m_SaveReq;
	protected int m_DupReq;
	protected int m_ListSel;
	protected int m_PendingRow;
	protected bool m_PendingNew;
	protected string m_SelectAfterList;
	protected float m_LastW;
	protected float m_LastH;
	protected int m_NextStatsAt;
	protected bool m_Loaded;
	protected bool m_ListRetried;

	protected Widget m_Main;
	protected Widget m_Rail;
	protected TextListboxWidget m_HookList;
	protected ButtonWidget m_BtnNew;
	protected ButtonWidget m_BtnDuplicate;
	protected ButtonWidget m_BtnDelete;
	protected ButtonWidget m_BtnRefresh;
	protected TextWidget m_SubTitle;
	protected Widget m_WorkArea;
	protected Widget m_TabBar;
	protected ButtonWidget m_BtnTabEvents;
	protected ButtonWidget m_BtnTabTemplate;
	protected ButtonWidget m_BtnTabSettings;
	protected TextWidget m_TxtHookTitle;
	protected ButtonWidget m_BtnSaveHook;
	protected Widget m_HealthBar;
	protected TextWidget m_TxtHealth;
	protected Widget m_TxtWorkEmpty;
	protected Widget m_EventsRoot;
	protected Widget m_TemplateRoot;
	protected Widget m_SettingsRoot;

	protected ref VPPWebhookEventsView m_EventsView;
	protected ref VPPWebhookTemplateView m_TemplateView;
	protected ref VPPWebhookSettingsView m_SettingsView;
	protected VPPDialogBox m_Dialog;
	protected string m_DialogAction;

	void MenuWebHooks()
	{
		m_Hooks = new array<ref WebHook>();
		m_Stats = new map<string, ref VPPWebhookStatsWire>();
		m_RowIds = new array<string>();
		m_ListSel = -1;
		m_PendingRow = -1;
		m_Tab = TAB_EVENTS;
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "PopulateList", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnAccess", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnWebhookSaved", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnStats", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnTemplate", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnTemplateCheck", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnTestResult", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuWebHooks", "OnTemplatesReloaded", this, SingleplayerExecutionType.Client);
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET = CreateWidgets(VPPATUIConstants.MenuWebHooks);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel = M_SUB_WIDGET.FindAnyWidget("Header");
		m_closeButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnClose"));
		m_BtnRefresh = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnRefresh"));
		m_SubTitle = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SubTitle"));
		m_Main = M_SUB_WIDGET.FindAnyWidget("Main");
		m_Rail = M_SUB_WIDGET.FindAnyWidget("Rail");
		m_HookList = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("HookList"));
		m_BtnNew = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnNew"));
		m_BtnDuplicate = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnDuplicate"));
		m_BtnDelete = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnDelete"));
		m_WorkArea = M_SUB_WIDGET.FindAnyWidget("Work");
		m_TabBar = M_SUB_WIDGET.FindAnyWidget("TabBar");
		m_BtnTabEvents = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabEvents"));
		m_BtnTabTemplate = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabTemplate"));
		m_BtnTabSettings = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabSettings"));
		m_TxtHookTitle = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtHookTitle"));
		m_BtnSaveHook = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnSaveHook"));
		m_HealthBar = M_SUB_WIDGET.FindAnyWidget("HealthBar");
		m_TxtHealth = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtHealth"));
		m_TxtWorkEmpty = M_SUB_WIDGET.FindAnyWidget("TxtWorkEmpty");
		m_EventsRoot = M_SUB_WIDGET.FindAnyWidget("EventsView");
		m_TemplateRoot = M_SUB_WIDGET.FindAnyWidget("TemplateView");
		m_SettingsRoot = M_SUB_WIDGET.FindAnyWidget("SettingsView");
		m_EventsView = new VPPWebhookEventsView(this, m_EventsRoot);
		m_TemplateView = new VPPWebhookTemplateView(this, m_TemplateRoot);
		m_SettingsView = new VPPWebhookSettingsView(this, m_SettingsRoot);
		m_Loaded = true;
		SetTab(TAB_EVENTS);
		UpdateWorkState();
		RequestList();
	}

	override void OnMenuShow()
	{
		super.OnMenuShow();
		RequestList();
		m_NextStatsAt = 0;
	}

	// ---------------------------------------------------------------- access / accessors for the views

	bool CanDo(int accessBit)
	{
		return (m_Access & accessBit) != 0;
	}

	bool CanEditWork()
	{
		if (!m_HasWork)
		{
			return false;
		}

		if (m_WorkIsNew)
		{
			return CanDo(VPPWebhookAccess.CREATE);
		}

		return CanDo(VPPWebhookAccess.EDIT);
	}

	WebHook GetWork()
	{
		return m_Work;
	}

	bool HasWork()
	{
		return m_HasWork;
	}

	bool IsWorkNew()
	{
		return m_WorkIsNew;
	}

	string GetWorkId()
	{
		return m_WorkId;
	}

	Widget GetDialogParent()
	{
		return M_SUB_WIDGET;
	}

	int NextReq()
	{
		m_ReqSerial++;
		return m_ReqSerial;
	}

	void MarkDirty()
	{
		if (!m_HasWork)
		{
			return;
		}

		m_WorkDirty = true;
		UpdateWorkState();
		RefreshRailRows();
	}

	void Notify(string key)
	{
		string text = VPPWebhookUi.Tr(key);
		GetVPPUIManager().DisplayNotification(text);
	}

	// ---------------------------------------------------------------- requests

	protected void RequestList()
	{
		if (!m_Loaded)
		{
			return;
		}

		GetRPCManager().VSendRPC("RPC_WebHooksManager", "GetWebHooks", null, true);
	}

	protected void RequestStats()
	{
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "GetWebhookStats", null, true);
	}

	// ---------------------------------------------------------------- RPC receivers

	void PopulateList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref WebHook>> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		m_Hooks = data.param1;
		if (!m_Hooks)
		{
			m_Hooks = new array<ref WebHook>();
		}

		RepairList();

		string wanted = m_SelectAfterList;
		m_SelectAfterList = "";
		if (wanted == "" && m_HasWork && !m_WorkIsNew)
		{
			wanted = m_WorkId;
		}

		if (wanted != "" && !FindHook(wanted))
		{
			wanted = "";
			if (!m_WorkIsNew)
			{
				ClearWork();
			}
		}

		if (wanted == "" && !m_HasWork && m_Hooks.Count() > 0)
		{
			wanted = m_Hooks[0].m_Id;
		}

		if (wanted != "" && (!m_WorkDirty || m_WorkId != wanted || m_WorkIsNew))
		{
			LoadWork(wanted);
		}

		RebuildRail();
		UpdateSubtitle();
		UpdateWorkState();
		m_NextStatsAt = 0;
	}

	void OnAccess(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		m_Access = data.param1;
		m_AccessKnown = true;
		UpdateWorkState();
	}

	void OnWebhookSaved(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPWebhookSaveReply> data;
		if (type != CallType.Client || !ctx.Read(data) || !data.param1)
		{
			return;
		}

		VPPWebhookSaveReply reply = data.param1;
		if (reply.ReqId != m_SaveReq && reply.ReqId != m_DupReq)
		{
			return;
		}

		if (!reply.Ok)
		{
			Notify("#VSTR_WH_ERR_" + reply.Error);
			return;
		}

		if (reply.ReqId == m_SaveReq)
		{
			m_WorkDirty = false;
			m_WorkIsNew = false;
			m_HasWork = false;
			Notify("#VSTR_WH_NOTIFY_SAVED");
		}
		else
		{
			Notify("#VSTR_WH_NOTIFY_DUPLICATED");
		}

		m_SelectAfterList = reply.HookId;
	}

	void OnStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref VPPWebhookStatsWire>> data;
		if (type != CallType.Client || !ctx.Read(data) || !data.param1)
		{
			return;
		}

		m_Stats.Clear();
		foreach (VPPWebhookStatsWire row : data.param1)
		{
			if (row)
			{
				m_Stats.Set(row.HookId, row);
			}
		}

		RefreshRailRows();
		UpdateHealth();
	}

	void OnTemplate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPWebhookTemplateReply> data;
		if (type != CallType.Client || !ctx.Read(data) || !data.param1)
		{
			return;
		}

		m_TemplateView.OnTemplate(data.param1);
	}

	void OnTemplateCheck(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPWebhookCheckReply> data;
		if (type != CallType.Client || !ctx.Read(data) || !data.param1)
		{
			return;
		}

		m_TemplateView.OnCheck(data.param1);
	}

	void OnTestResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPWebhookTestReply> data;
		if (type != CallType.Client || !ctx.Read(data) || !data.param1)
		{
			return;
		}

		m_TemplateView.OnTestResult(data.param1);
		m_NextStatsAt = 0;
	}

	void OnTemplatesReloaded(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
	}

	// ---------------------------------------------------------------- work (the edited webhook)

	// Every webhook of the list made safe to read. Missing event settings mean the list arrived damaged: it is logged
	// (with the webhook) and asked for once more.
	protected void RepairList()
	{
		int damaged = 0;
		for (int i = m_Hooks.Count() - 1; i >= 0; i--)
		{
			WebHook hook = m_Hooks[i];
			if (!hook)
			{
				m_Hooks.Remove(i);
				damaged++;
				continue;
			}

			int repaired = hook.RepairEvents();
			if (repaired > 0)
			{
				string hookName = hook.GetName();
				Print("[VPPAdminTools] WebHooks: " + repaired.ToString() + " event setting(s) of \"" + hookName + "\" arrived empty from the server");
				damaged++;
			}
		}

		if (damaged > 0 && !m_ListRetried)
		{
			m_ListRetried = true;
			RequestList();
			return;
		}

		if (damaged == 0)
		{
			m_ListRetried = false;
		}
	}

	WebHook FindHook(string hookId)
	{
		foreach (WebHook hook : m_Hooks)
		{
			if (hook && hook.m_Id == hookId)
			{
				return hook;
			}
		}

		return null;
	}

	protected void LoadWork(string hookId)
	{
		WebHook stored = FindHook(hookId);
		if (!stored)
		{
			ClearWork();
			return;
		}

		WebHook copy = stored.CopyForClient(true);
		if (!copy)
		{
			ClearWork();
			return;
		}

		m_Work = copy;
		m_WorkId = hookId;
		m_HasWork = true;
		m_WorkIsNew = false;
		m_WorkDirty = false;
		LoadViews();
	}

	protected void ClearWork()
	{
		m_Work = null;
		m_WorkId = "";
		m_HasWork = false;
		m_WorkIsNew = false;
		m_WorkDirty = false;
		LoadViews();
	}

	// A new, unsaved webhook: Discord embed preset, every event on except hits (they are frequent).
	protected void StartNewWork()
	{
		WebHook fresh = new WebHook("", "");
		fresh.m_Preset = VPPWebhookPresets.DISCORD_EMBED;
		fresh.m_ContentType = VPPWebhookPresets.CT_JSON;
		fresh.SetServerStatsInterval(VPPWebHookServerStatsTime.FIVE_MINUTES);
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			VPPWebhookEventCfg cfg = new VPPWebhookEventCfg();
			cfg.Enabled = def.Id != VPPWebhookDefs.EV_HIT;
			fresh.m_Events.Set(def.Id, cfg);
		}

		m_Work = fresh;
		m_WorkId = "";
		m_HasWork = true;
		m_WorkIsNew = true;
		m_WorkDirty = true;
		LoadViews();
		RebuildRail();
		SetTab(TAB_SETTINGS);
		UpdateWorkState();
	}

	protected void LoadViews()
	{
		m_EventsView.Load();
		m_SettingsView.Load();
		m_TemplateView.OnWorkChanged();
		UpdateWorkState();
		UpdateHealth();
	}

	protected void SaveWork()
	{
		if (!m_HasWork || !m_Work || !CanEditWork())
		{
			return;
		}

		string workName = m_Work.GetName();
		workName = workName.Trim();
		if (workName.Length() < 3)
		{
			Notify("#VSTR_WH_ERR_NAME_SHORT");
			SetTab(TAB_SETTINGS);
			return;
		}

		string workUrl = m_Work.GetURL();
		workUrl = workUrl.Trim();
		if (m_WorkIsNew && workUrl == "")
		{
			Notify("#VSTR_WH_ERR_URL_MISSING");
			SetTab(TAB_SETTINGS);
			return;
		}

		m_SaveReq = NextReq();
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "SaveWebhook", new Param2<int, ref WebHook>(m_SaveReq, m_Work), true);
	}

	protected void DuplicateWork()
	{
		if (!m_HasWork || m_WorkIsNew || !CanDo(VPPWebhookAccess.CREATE))
		{
			return;
		}

		m_DupReq = NextReq();
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "DuplicateWebhook", new Param2<int, string>(m_DupReq, m_WorkId), true);
	}

	protected void DeleteWork()
	{
		if (!m_HasWork)
		{
			return;
		}

		if (m_WorkIsNew)
		{
			ClearWork();
			RebuildRail();
			return;
		}

		if (!CanDo(VPPWebhookAccess.DELETE))
		{
			return;
		}

		GetRPCManager().VSendRPC("RPC_WebHooksManager", "DeleteWebhookById", new Param1<string>(m_WorkId), true);
		ClearWork();
	}

	// ---------------------------------------------------------------- dialogs

	// action: what to do on YES (DISCARD_ROW, DISCARD_NEW, DELETE, plus the views' own actions through Confirm).
	void Confirm(string action, string titleKey, string bodyKey)
	{
		string title = VPPWebhookUi.Tr(titleKey);
		string body = VPPWebhookUi.Tr(bodyKey);
		m_DialogAction = action;
		m_Dialog = GetVPPUIManager().CreateDialogBox(M_SUB_WIDGET);
		if (!m_Dialog)
		{
			m_DialogAction = "";
			return;
		}

		m_Dialog.InitDiagBox(DIAGTYPE.DIAG_YESNO, title, body, this, "OnDialogResult");
	}

	void OnDialogResult(int result)
	{
		string action = m_DialogAction;
		m_DialogAction = "";
		m_Dialog = null;
		if (result != DIAGRESULT.YES)
		{
			if (action == "DISCARD_ROW")
			{
				m_PendingRow = -1;
			}

			m_TemplateView.OnDialogNo(action);
			m_SettingsView.OnDialogNo(action);
			return;
		}

		if (action == "DISCARD_ROW")
		{
			m_WorkDirty = false;
			m_WorkIsNew = false;
			ApplyRailRow(m_PendingRow);
			m_PendingRow = -1;
			return;
		}

		if (action == "DISCARD_NEW")
		{
			m_WorkDirty = false;
			StartNewWork();
			return;
		}

		if (action == "DELETE")
		{
			DeleteWork();
			return;
		}

		m_TemplateView.OnDialogYes(action);
		m_SettingsView.OnDialogYes(action);
	}

	// ---------------------------------------------------------------- rail

	protected void RebuildRail()
	{
		if (!m_HookList)
		{
			return;
		}

		m_HookList.ClearItems();
		m_RowIds.Clear();
		int selectRow = -1;
		foreach (WebHook hook : m_Hooks)
		{
			if (!hook)
			{
				continue;
			}

			m_HookList.AddItem("", null, 0);
			m_RowIds.Insert(hook.m_Id);
			if (m_HasWork && !m_WorkIsNew && hook.m_Id == m_WorkId)
			{
				selectRow = m_RowIds.Count() - 1;
			}
		}

		if (m_HasWork && m_WorkIsNew)
		{
			m_HookList.AddItem("", null, 0);
			m_RowIds.Insert("");
			selectRow = m_RowIds.Count() - 1;
		}

		RefreshRailRows();
		m_ListSel = selectRow;
		if (selectRow >= 0)
		{
			m_HookList.SelectRow(selectRow);
		}
		else
		{
			m_HookList.SelectRow(-1);
		}
	}

	// Name and state column of every row (NEW / OFF / FAIL / OK / IDLE, * = unsaved changes).
	protected void RefreshRailRows()
	{
		int total = m_RowIds.Count();
		int row = 0;
		string hookId = "";
		string name = "";
		string state = "";
		int stateColor = 0;
		WebHook hook = null;
		VPPWebhookStatsWire stats = null;
		for (row = 0; row < total; row++)
		{
			hookId = m_RowIds[row];
			hook = null;
			if (hookId != "")
			{
				hook = FindHook(hookId);
			}

			name = "";
			if (hook)
			{
				name = hook.GetName();
			}

			if (m_HasWork && m_Work && ((hookId == "" && m_WorkIsNew) || (hookId == m_WorkId && !m_WorkIsNew)))
			{
				name = m_Work.GetName();
				if (name == "")
				{
					name = VPPWebhookUi.Tr("#VSTR_WH_UNNAMED");
				}

				if (m_WorkDirty)
				{
					name = name + " *";
				}
			}

			state = VPPWebhookUi.Tr("#VSTR_WH_STATE_IDLE");
			stateColor = VPPWebhookUi.CLR_IDLE;
			stats = null;
			if (hookId != "")
			{
				stats = m_Stats.Get(hookId);
			}

			if (hookId == "")
			{
				state = VPPWebhookUi.Tr("#VSTR_WH_STATE_NEW");
				stateColor = VPPWebhookUi.CLR_NEW;
			}
			else if (hook && hook.m_Disabled)
			{
				state = VPPWebhookUi.Tr("#VSTR_WH_STATE_OFF");
				stateColor = VPPWebhookUi.CLR_OFF;
			}
			else if (stats && stats.ConsecutiveFailures > 0)
			{
				state = VPPWebhookUi.Tr("#VSTR_WH_STATE_FAIL");
				stateColor = VPPWebhookUi.CLR_FAIL;
			}
			else if (stats && (stats.Delivered > 0 || stats.NoReply > 0))
			{
				state = VPPWebhookUi.Tr("#VSTR_WH_STATE_OK");
				stateColor = VPPWebhookUi.CLR_OK;
			}

			m_HookList.SetItem(row, name, null, 0);
			m_HookList.SetItem(row, state, null, 1);
			m_HookList.SetItemColor(row, 0, VPPWebhookUi.CLR_WHITE);
			m_HookList.SetItemColor(row, 1, stateColor);
		}
	}

	protected void PollRail()
	{
		if (!m_HookList)
		{
			return;
		}

		int row = m_HookList.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		// a click on the empty part of the list keeps the current webhook
		if (row < 0 && m_ListSel >= 0)
		{
			m_HookList.SelectRow(m_ListSel);
			return;
		}

		if (m_WorkDirty)
		{
			m_PendingRow = row;
			m_HookList.SelectRow(m_ListSel);
			Confirm("DISCARD_ROW", "#VSTR_WH_DLG_DISCARD_TITLE", "#VSTR_WH_DLG_DISCARD");
			return;
		}

		ApplyRailRow(row);
	}

	protected void ApplyRailRow(int row)
	{
		string hookId = "";
		if (row >= 0 && row < m_RowIds.Count())
		{
			hookId = m_RowIds[row];
		}

		if (hookId == "")
		{
			if (!(m_HasWork && m_WorkIsNew))
			{
				ClearWork();
			}
		}
		else
		{
			LoadWork(hookId);
		}

		RebuildRail();
		UpdateWorkState();
	}

	// ---------------------------------------------------------------- tabs, state, health

	void SetTab(int tabIndex)
	{
		m_Tab = tabIndex;
		UpdateWorkState();
		if (m_Tab == TAB_TEMPLATE)
		{
			m_TemplateView.OnShown();
		}
	}

	void OpenTemplate(string eventId)
	{
		SetTab(TAB_TEMPLATE);
		m_TemplateView.SelectEvent(eventId);
	}

	protected void PaintTab(ButtonWidget tabButton, string suffix, bool active)
	{
		if (!tabButton)
		{
			return;
		}

		int color = VPPWebhookUi.CLR_SECONDARY;
		if (active)
		{
			color = VPPWebhookUi.CLR_ORANGE;
		}

		Widget tabText = tabButton.FindAnyWidget("Txt" + suffix);
		Widget tabImage = tabButton.FindAnyWidget("Img" + suffix);
		if (tabText)
		{
			tabText.SetColor(color);
		}

		if (tabImage)
		{
			tabImage.SetColor(color);
		}
	}

	protected void UpdateWorkState()
	{
		if (!m_Loaded)
		{
			return;
		}

		bool hasWork = m_HasWork && m_Work != null;
		m_EventsRoot.Show(hasWork && m_Tab == TAB_EVENTS);
		m_TemplateRoot.Show(hasWork && m_Tab == TAB_TEMPLATE);
		m_SettingsRoot.Show(hasWork && m_Tab == TAB_SETTINGS);
		m_TxtWorkEmpty.Show(!hasWork);
		m_TabBar.Show(hasWork);
		PaintTab(m_BtnTabEvents, "TabEvents", m_Tab == TAB_EVENTS);
		PaintTab(m_BtnTabTemplate, "TabTemplate", m_Tab == TAB_TEMPLATE);
		PaintTab(m_BtnTabSettings, "TabSettings", m_Tab == TAB_SETTINGS);
		string title = "";
		if (hasWork)
		{
			title = m_Work.GetName();
			if (title == "")
			{
				title = VPPWebhookUi.Tr("#VSTR_WH_UNNAMED");
			}

			if (m_WorkDirty)
			{
				title = title + "  " + VPPWebhookUi.Tr("#VSTR_WH_UNSAVED");
			}
		}

		m_TxtHookTitle.SetText(title);
		bool canEdit = CanEditWork();
		m_BtnSaveHook.Enable(hasWork && canEdit && m_WorkDirty);
		m_BtnNew.Enable(CanDo(VPPWebhookAccess.CREATE));
		m_BtnDuplicate.Enable(hasWork && !m_WorkIsNew && CanDo(VPPWebhookAccess.CREATE));
		bool canDelete = hasWork && (m_WorkIsNew || CanDo(VPPWebhookAccess.DELETE));
		m_BtnDelete.Enable(canDelete);
		m_EventsView.SetEditable(canEdit);
		m_SettingsView.SetEditable(canEdit);
		m_TemplateView.UpdateEnabled();
	}

	protected void UpdateSubtitle()
	{
		if (!m_SubTitle)
		{
			return;
		}

		string countText = m_Hooks.Count().ToString();
		string format = VPPWebhookUi.Tr("#VSTR_WH_SUBTITLE");
		string subtitle = string.Format(format, countText);
		m_SubTitle.SetText(subtitle);
	}

	protected void UpdateHealth()
	{
		if (!m_TxtHealth)
		{
			return;
		}

		if (!m_HasWork || m_WorkIsNew)
		{
			m_TxtHealth.SetText(VPPWebhookUi.Tr("#VSTR_WH_HEALTH_NONE"));
			return;
		}

		VPPWebhookStatsWire stats = m_Stats.Get(m_WorkId);
		if (!stats)
		{
			m_TxtHealth.SetText(VPPWebhookUi.Tr("#VSTR_WH_HEALTH_NONE"));
			return;
		}

		string deliveredText = stats.Delivered.ToString();
		string noReplyText = stats.NoReply.ToString();
		string failedText = stats.Failed.ToString();
		string droppedText = stats.Dropped.ToString();
		string waitingText = stats.Waiting.ToString();
		string format = VPPWebhookUi.Tr("#VSTR_WH_HEALTH");
		string line = string.Format(format, deliveredText, noReplyText, failedText, droppedText, waitingText);
		if (stats.LastError != "")
		{
			string errorFormat = VPPWebhookUi.Tr("#VSTR_WH_HEALTH_LAST_ERROR");
			string errorText = string.Format(errorFormat, stats.LastErrorTime, stats.LastError);
			line = line + "   " + errorText;
		}

		m_TxtHealth.SetText(line);
		int color = VPPWebhookUi.CLR_SECONDARY;
		if (stats.ConsecutiveFailures > 0)
		{
			color = VPPWebhookUi.CLR_FAIL;
		}

		m_TxtHealth.SetColor(color);
	}

	// ---------------------------------------------------------------- layout

	// Stretches the containers to the window (the layout is drawn for 1260 x 820).
	protected void Relayout()
	{
		float mainW = 0;
		float mainH = 0;
		m_Main.GetScreenSize(mainW, mainH);
		if (mainW <= 0 || mainH <= 0)
		{
			return;
		}

		if (mainW == m_LastW && mainH == m_LastH)
		{
			return;
		}

		m_LastW = mainW;
		m_LastH = mainH;
		float bodyH = mainH - TOP - GAP;
		float workX = GAP + RAIL_W + GAP;
		float workW = mainW - workX - GAP;
		m_Rail.SetPos(GAP, TOP);
		m_Rail.SetSize(RAIL_W, bodyH);
		m_HookList.SetSize(RAIL_W - 12, bodyH - 32 - 80);
		float halfButtonW = (RAIL_W - 18) / 2;
		m_BtnNew.SetPos(6, bodyH - 74);
		m_BtnNew.SetSize(RAIL_W - 12, 32);
		m_BtnDuplicate.SetPos(6, bodyH - 36);
		m_BtnDuplicate.SetSize(halfButtonW, 32);
		m_BtnDelete.SetPos(12 + halfButtonW, bodyH - 36);
		m_BtnDelete.SetSize(halfButtonW, 32);
		m_WorkArea.SetPos(workX, TOP);
		m_WorkArea.SetSize(workW, bodyH);
		m_TabBar.SetSize(workW, 32);
		m_HealthBar.SetPos(0, bodyH - 30);
		m_HealthBar.SetSize(workW, 30);
		m_TxtHealth.SetSize(workW - 40, 30);
		float viewH = bodyH - 38 - 36;
		m_EventsRoot.SetSize(workW, viewH);
		m_TemplateRoot.SetSize(workW, viewH);
		m_SettingsRoot.SetSize(workW, viewH);
		m_EventsView.Relayout(workW, viewH);
		m_TemplateView.Relayout(workW, viewH);
		m_SettingsView.Relayout(workW, viewH);
	}

	// ---------------------------------------------------------------- events

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_Loaded || !IsSubMenuVisible())
		{
			return;
		}

		Relayout();
		PollRail();
		m_EventsView.OnUpdate();
		m_TemplateView.OnUpdate();
		m_SettingsView.OnUpdate();
		int now = GetGame().GetTime();
		if (now >= m_NextStatsAt)
		{
			m_NextStatsAt = now + STATS_EVERY_MS;
			RequestStats();
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Loaded)
		{
			return super.OnClick(w, x, y, button);
		}

		if (w == m_BtnRefresh)
		{
			RequestList();
			m_NextStatsAt = 0;
			return true;
		}

		if (w == m_BtnNew)
		{
			if (m_WorkDirty)
			{
				Confirm("DISCARD_NEW", "#VSTR_WH_DLG_DISCARD_TITLE", "#VSTR_WH_DLG_DISCARD");
			}
			else
			{
				StartNewWork();
			}

			return true;
		}

		if (w == m_BtnDuplicate)
		{
			DuplicateWork();
			return true;
		}

		if (w == m_BtnDelete)
		{
			if (m_WorkIsNew)
			{
				DeleteWork();
			}
			else
			{
				Confirm("DELETE", "#VSTR_WH_DLG_DELETE_TITLE", "#VSTR_WH_DLG_DELETE");
			}

			return true;
		}

		if (w == m_BtnSaveHook)
		{
			SaveWork();
			return true;
		}

		if (w == m_BtnTabEvents)
		{
			SetTab(TAB_EVENTS);
			return true;
		}

		if (w == m_BtnTabTemplate)
		{
			SetTab(TAB_TEMPLATE);
			return true;
		}

		if (w == m_BtnTabSettings)
		{
			SetTab(TAB_SETTINGS);
			return true;
		}

		if (m_HasWork && m_Tab == TAB_EVENTS && m_EventsView.OnClick(w))
		{
			return true;
		}

		if (m_HasWork && m_Tab == TAB_TEMPLATE && m_TemplateView.OnClick(w))
		{
			return true;
		}

		if (m_HasWork && m_Tab == TAB_SETTINGS && m_SettingsView.OnClick(w))
		{
			return true;
		}

		return super.OnClick(w, x, y, button);
	}

	// The INSERT list inserts on a double-click only (the mouse wheel also moves a list's selection).
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (m_Loaded && m_HasWork && m_Tab == TAB_TEMPLATE && button == MouseState.LEFT && m_TemplateView.OnDoubleClick(w))
		{
			return true;
		}

		return super.OnDoubleClick(w, x, y, button);
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (m_Loaded && m_HasWork)
		{
			if (m_Tab == TAB_EVENTS && m_EventsView.OnChange(w))
			{
				return true;
			}

			if (m_Tab == TAB_SETTINGS && m_SettingsView.OnChange(w))
			{
				return true;
			}
		}

		return super.OnChange(w, x, y, finished);
	}
};
