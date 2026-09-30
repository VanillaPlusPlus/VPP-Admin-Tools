/*
	XML Editor window (WP5): header, tab bar, type rail (scope + Show filter + search with prefixes),
	tab hosts (TYPES = VPPXETypesTab, MAP/BACKUPS/FILES = WP6 tabs), the S2C receivers of the shell,
	the STALE/rebase flow, the save locks, paced edit uploads and dialogs through a weak VPPDialogBox handle.
*/
class MenuXMLEditor extends AdminHudSubMenu
{
	const static int SEARCH_DEBOUNCE_MS = 350;
	const static float TIMEOUT_CHECK_INTERVAL = 1.0;
	const static int TAB_X0 = 38;
	const static int TAB_BAR_H = 30;
	//dropdown_prefab.layout lays entries out in a GridSpacer fixed at Rows 50; entries past that are never shown (rest reachable via c:/u:/v:/t: search)
	const static int FILTER_DD_MAX_ROWS = 50;
	const static int INDEX_RETRY_MAX = 2;
	// TEMPORARY delivery trace (pairs with VPPXENet.NET_TRACE on the server); set false once confirmed.
	const static bool NET_TRACE = false;

	protected bool m_Loaded;
	protected ref VPPXEClientModel m_Model;
	protected ref VPPXETypesTab m_TypesTab;
	protected ref VPPXEMapTab m_MapTab;
	protected ref VPPXEBackupsTab m_BackupsTab;
	protected ref VPPXEFilesTab m_FilesTab;
	protected ref VPPXECreateTab m_CreateTab;
	protected ref VPPXEMessagesTab m_MessagesTab;
	protected ref VPPXEEventsTab m_EventsTab;
	protected ref VPPXESpawnablesTab m_SpawnablesTab;

	protected VPPDialogBox m_Dialog;
	protected bool m_DialogWasOpen;
	protected Class m_DialogCbInst;
	protected string m_DialogCbFunc;

	protected int m_ReqCounter;
	protected int m_ActiveTab;
	protected bool m_RailToggledOff;
	protected bool m_SkipShowOpen;
	protected bool m_BrokenHidden;
	protected bool m_SessionOpen;
	protected bool m_CloseWanted;
	protected string m_Scope;
	protected string m_SelectedType;
	//config class picked under the "not registered" filter (m_SelectedType is "" meanwhile)
	protected string m_UnregSelected;
	protected int m_DetailsReqId;
	protected float m_TimeoutTimer;
	protected int m_LastReqActivity;

	protected Widget m_Main;
	protected Widget m_Body;
	protected Widget m_TabBar;
	protected Widget m_PanelRail;
	protected Widget m_PanelContent;
	protected TextWidget m_SubTitle;
	protected TextWidget m_TxtStatus;
	protected TextWidget m_TxtFilesBadge;
	protected TextWidget m_TxtRowCount;
	protected Widget m_PendingChip;
	protected ImageWidget m_ImgInfoTooltip;
	protected ImageWidget m_ImgSearchHelp;
	protected ImageWidget m_ImgToggleRail;
	protected ButtonWidget m_BtnRefresh;
	protected ButtonWidget m_BtnToggleRail;
	protected ButtonWidget m_BtnAddType;
	protected ButtonWidget m_BtnBulkEdit;
	protected EditBoxWidget m_SearchInput;
	protected TextListboxWidget m_TypesList;
	protected Widget m_ReadOnlyBanner;
	protected Widget m_FilterHost;
	protected Widget m_FileScopeHost;
	protected ref array<ButtonWidget> m_TabButtons;
	protected ref array<TextWidget> m_TabTexts;
	protected ref array<ImageWidget> m_TabImages;
	protected ref array<Widget> m_TabHosts;
	protected ref array<string> m_TabKeys;
	protected ref array<int> m_TabOrder;

	protected ref VPPDropDownMenu m_ScopeDD;
	protected ref VPPDropDownMenu m_FilterDD;
	protected ref array<string> m_ScopeKeys;
	protected ref array<string> m_ScopeLabels;
	protected ref array<int> m_FilterKinds;
	protected ref array<int> m_FilterArgs;
	protected ref array<string> m_FilterLabels;
	protected int m_FilterIdx;
	protected string m_ScopeSig;
	protected string m_FilterSig;

	protected ref array<string> m_RailNames;
	protected ref map<string, int> m_RailIndex;
	protected int m_LastRailRow;
	protected string m_LastSearch;
	protected int m_SearchChangedAt;
	protected bool m_SearchPending;

	protected ref array<ref VPPXEEditBatch> m_UploadQueue;
	protected int m_UploadCursor;
	protected bool m_UploadPumping;
	protected ref map<string, int> m_SaveLockOwners;
	protected ref map<string, int> m_IndexRetries;

	protected float m_LastMainW;
	protected float m_LastMainH;
	protected float m_LastUnit;
	protected float m_LastBodyW;
	protected float m_LastBodyH;

	void MenuXMLEditor()
	{
		m_ActiveTab = VPPXETab.TYPES;
		m_Scope = "";
		m_SelectedType = "";
		m_UnregSelected = "";
		m_DialogCbFunc = "";
		m_LastSearch = "";
		m_ScopeSig = "-";
		m_FilterSig = "-";
		m_LastRailRow = -1;
		m_TabButtons = new array<ButtonWidget>();
		m_TabTexts = new array<TextWidget>();
		m_TabImages = new array<ImageWidget>();
		m_TabHosts = new array<Widget>();
		m_TabKeys = {"#VSTR_XMLE_TAB_TYPES", "#VSTR_XMLE_TAB_MAP", "#VSTR_XMLE_TAB_BACKUPS", "#VSTR_XMLE_TAB_FILES", "#VSTR_XMLE_TAB_CREATE", "#VSTR_XMLE_TAB_MESSAGES", "#VSTR_XMLE_TAB_EVENTS", "#VSTR_XMLE_TAB_SPAWNABLES"};
		// tab bar order (indexes of VPPXETab): MAP right after TYPES, then MESSAGES, EVENTS and SPAWNABLES
		m_TabOrder = {VPPXETab.TYPES, VPPXETab.MAP, VPPXETab.MESSAGES, VPPXETab.EVENTS, VPPXETab.SPAWNABLES, VPPXETab.BACKUPS, VPPXETab.FILES, VPPXETab.CREATE};
		m_ScopeKeys = new array<string>();
		m_ScopeLabels = new array<string>();
		m_FilterKinds = new array<int>();
		m_FilterArgs = new array<int>();
		m_FilterLabels = new array<string>();
		m_RailNames = new array<string>();
		m_RailIndex = new map<string, int>();
		m_UploadQueue = new array<ref VPPXEEditBatch>();
		m_SaveLockOwners = new map<string, int>();
		m_IndexRetries = new map<string, int>();
	}

	void ~MenuXMLEditor()
	{
		if (GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.PumpUploads);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CloseSessionTick);
		}

		if (m_TypesTab)
		{
			m_TypesTab.ReleasePreview();
		}

		if (m_SpawnablesTab)
		{
			m_SpawnablesTab.ReleasePreview();
		}
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET = CreateWidgets(VPPATUIConstants.MenuXMLEditor);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel = M_SUB_WIDGET.FindAnyWidget("Header");
		m_closeButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnClose"));
		ResizeWindow(true);

		m_Main = M_SUB_WIDGET.FindAnyWidget("Main");
		m_Body = M_SUB_WIDGET.FindAnyWidget("Body");
		m_TabBar = M_SUB_WIDGET.FindAnyWidget("TabBar");
		m_PanelRail = M_SUB_WIDGET.FindAnyWidget("PanelRail");
		m_PanelContent = M_SUB_WIDGET.FindAnyWidget("PanelContent");
		m_SubTitle = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SubTitle"));
		m_TxtStatus = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtStatus"));
		m_TxtFilesBadge = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtFilesBadge"));
		m_TxtRowCount = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtRowCount"));
		m_PendingChip = M_SUB_WIDGET.FindAnyWidget("PendingChip");
		m_ImgInfoTooltip = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfoTooltip"));
		m_ImgSearchHelp = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgSearchHelp"));
		m_ImgToggleRail = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgToggleRail"));
		m_BtnRefresh = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnRefresh"));
		m_BtnToggleRail = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnToggleRail"));
		m_BtnAddType = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnAddType"));
		m_BtnBulkEdit = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnBulkEdit"));
		m_SearchInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SearchInput"));
		m_TypesList = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TypesList"));
		m_ReadOnlyBanner = M_SUB_WIDGET.FindAnyWidget("ReadOnlyBanner");
		m_FilterHost = M_SUB_WIDGET.FindAnyWidget("FilterHost");
		m_FileScopeHost = M_SUB_WIDGET.FindAnyWidget("FileScopeHost");

		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabTypes")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabMap")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabBackups")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabFiles")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabCreate")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabMessages")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabEvents")));
		m_TabButtons.Insert(ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabSpawnables")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabTypes")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabMap")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabBackups")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabFiles")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabCreate")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabMessages")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabEvents")));
		m_TabTexts.Insert(TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtTabSpawnables")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabTypes")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabMap")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabBackups")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabFiles")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabCreate")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabMessages")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabEvents")));
		m_TabImages.Insert(ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgTabSpawnables")));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostTypes"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostMap"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostBackups"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostFiles"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostCreate"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostMessages"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostEvents"));
		m_TabHosts.Insert(M_SUB_WIDGET.FindAnyWidget("TabHostSpawnables"));

		m_ReqCounter = 1000 + (GetGame().GetTime() % 100000) * 10;
		m_Model = new VPPXEClientModel(this);

		m_ScopeDD = new VPPDropDownMenu(m_FileScopeHost, Tr("#VSTR_XMLE_SCOPE_ALL"));
		m_ScopeDD.m_OnSelectItem.Insert(OnScopeSelected);
		m_FilterDD = new VPPDropDownMenu(m_FilterHost, Tr("#VSTR_XMLE_FILTER_ALL"));
		m_FilterDD.m_OnSelectItem.Insert(OnFilterSelected);
		RebuildScopeDropdown();
		RebuildFilterDropdown();

		m_TypesTab = new VPPXETypesTab(m_TabHosts[VPPXETab.TYPES], this);
		m_MapTab = new VPPXEMapTab(m_TabHosts[VPPXETab.MAP], this);
		m_BackupsTab = new VPPXEBackupsTab(m_TabHosts[VPPXETab.BACKUPS], this);
		m_FilesTab = new VPPXEFilesTab(m_TabHosts[VPPXETab.FILES], this);
		m_CreateTab = new VPPXECreateTab(m_TabHosts[VPPXETab.CREATE], this);
		m_MessagesTab = new VPPXEMessagesTab(m_TabHosts[VPPXETab.MESSAGES], this);
		m_EventsTab = new VPPXEEventsTab(m_TabHosts[VPPXETab.EVENTS], this);
		m_SpawnablesTab = new VPPXESpawnablesTab(m_TabHosts[VPPXETab.SPAWNABLES], this);

		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnSession", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnProgress", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnActionResult", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnTypeIndexChunk", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnTypeDetails", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnSaveResult", this, SingleplayerExecutionType.Client);

		SetupTooltips();
		m_LastMainW = -1;
		m_LastBodyW = -1;
		m_Loaded = true;
		UpdateTabButtons();
		UpdateAddButton();
		ShowTab(VPPXETab.TYPES);
		SetStatus("#VSTR_XMLE_STATUS_CONNECTING");
		SendOpenSession();
		m_SkipShowOpen = true;
	}

	protected void SetupTooltips()
	{
		ToolTipHandler menuTip;
		if (m_ImgInfoTooltip)
		{
			m_ImgInfoTooltip.GetScript(menuTip);
		}

		if (menuTip)
		{
			menuTip.SetTitle("#VSTR_TOOLTIP_TITLE");
			menuTip.SetContentText("#VSTR_XMLE_TOOLTIP_MENU");
		}

		ToolTipHandler searchTip;
		if (m_ImgSearchHelp)
		{
			m_ImgSearchHelp.GetScript(searchTip);
		}

		if (searchTip)
		{
			searchTip.SetTitle("#VSTR_TOOLTIP_TITLE");
			searchTip.SetContentText("#VSTR_XMLE_TOOLTIP_SEARCH");
		}
	}

	//---------------------------------------------------------------- public API (INTERFACES section 6)

	VPPXEClientModel GetModel()
	{
		return m_Model;
	}

	bool HasPerm(int permBit)
	{
		if (!m_Model)
		{
			return false;
		}

		VPPXESessionInfo session = m_Model.GetSession();
		if (!session)
		{
			return false;
		}

		return (session.PermMask & permBit) != 0;
	}

	string GetSelectedType()
	{
		return m_SelectedType;
	}

	string GetTypesScope()
	{
		return m_Scope;
	}

	int GetActiveTab()
	{
		return m_ActiveTab;
	}

	int NextReqId()
	{
		m_LastReqActivity = GetGame().GetTime();
		m_ReqCounter++;
		return m_ReqCounter;
	}

	string Tr(string keyOrText)
	{
		if (keyOrText.Length() > 0 && keyOrText.Get(0) == "#")
		{
			return Widget.TranslateString(keyOrText);
		}

		return keyOrText;
	}

	void Notify(string text)
	{
		GetVPPUIManager().DisplayNotification(Tr(text));
	}

	void NotifyError(string text)
	{
		GetVPPUIManager().DisplayError(Tr(text));
	}

	void SetStatus(string text)
	{
		if (m_TxtStatus)
		{
			m_TxtStatus.SetText(Tr(text));
		}
	}

	protected string FormatKey(string key, string arg)
	{
		if (key == "")
		{
			return string.Format(Tr("#VSTR_XMLE_ERR_INTERNAL"), arg);
		}

		return string.Format(Tr(key), arg);
	}

	void SelectType(string typeName, bool switchToTypesTab)
	{
		if (typeName == "")
		{
			return;
		}

		m_UnregSelected = "";
		m_SelectedType = m_Model.DisplayNameOf(typeName);
		RequestDetails();
		if (switchToTypesTab)
		{
			ShowTab(VPPXETab.TYPES);
		}

		m_TypesTab.ShowType(m_SelectedType);
		m_MapTab.OnTypeSelected(m_SelectedType);
		SelectRailRow(m_SelectedType);
	}

	// "not registered" filter: a game config class without a types entry. No details request (the server has no row
	// for it); the TYPES tab explains that and offers REGISTER.
	void SelectUnregistered(string className)
	{
		if (className == "")
		{
			return;
		}

		m_SelectedType = "";
		m_UnregSelected = className;
		if (m_ActiveTab != VPPXETab.TYPES)
		{
			ShowTab(VPPXETab.TYPES);
		}

		m_MapTab.OnTypeSelected("");
		m_TypesTab.ShowUnregistered(className);
	}

	// The class shown as "not registered" now has a types entry (this admin's REGISTER or another admin's save): open it.
	protected void CheckUnregisteredSelection()
	{
		if (m_UnregSelected == "" || !m_Model.HasName(m_UnregSelected))
		{
			return;
		}

		SelectType(m_UnregSelected, false);
	}

	// CREATE tab: show a messages file in the MESSAGES tab.
	void OpenMessagesFile(string fileKey)
	{
		ShowTab(VPPXETab.MESSAGES);
		m_MessagesTab.SelectFile(fileKey);
	}

	// CREATE tab: show an events file in the EVENTS tab.
	void OpenEventsFile(string fileKey)
	{
		ShowTab(VPPXETab.EVENTS);
		m_EventsTab.SelectFile(fileKey);
	}

	// CREATE tab: show a spawnabletypes or randompresets file in the SPAWNABLES tab.
	void OpenSpawnablesFile(string fileKey)
	{
		ShowTab(VPPXETab.SPAWNABLES);
		m_SpawnablesTab.SelectFile(fileKey);
	}

	// TYPES inspector: the type's spawnabletypes entry in the SPAWNABLES tab (or an ADD dialog when it has none).
	void OpenSpawnablesType(string typeName)
	{
		ShowTab(VPPXETab.SPAWNABLES);
		m_SpawnablesTab.SelectType(typeName);
	}

	// The SPAWNABLES tab's loaded rows answer the TYPES inspector's "spawns with" / "found in".
	VPPXESpawnablesTab GetSpawnablesTab()
	{
		return m_SpawnablesTab;
	}

	void SetTypesScope(string fileKey)
	{
		m_Scope = fileKey;
		if (m_Scope != "" && !m_Model.IsTypesFile(m_Scope))
		{
			m_Scope = "";
		}

		int idx = m_ScopeKeys.Find(m_Scope);
		if (idx < 0)
		{
			//a file past the scope dropdown cap: the rebuild swaps it into the last row
			RebuildScopeDropdown();
			idx = m_ScopeKeys.Find(m_Scope);
		}

		if (idx >= 0)
		{
			m_ScopeDD.SetText(m_ScopeLabels[idx]);
			m_ScopeDD.SetIndex(idx);
		}

		RebuildRail();
		UpdateSubtitle();
		UpdateAddButton();
		m_TypesTab.OnScopeChanged();
	}

	//every lowercase name of the current scope matching the Show filter and the search (uncapped)
	void GetFilteredNames(array<string> outLower)
	{
		string searchText = "";
		if (m_SearchInput)
		{
			searchText = m_SearchInput.GetText();
		}

		m_Model.CollectMatches(m_Scope, searchText, CurrentFilterKind(), CurrentFilterArg(), outLower);
	}

	//dialogs: title/body are #keys or pre-formatted text; callback void cbFunc(int result, string input)
	void OpenConfirm(string title, string body, int diagType, Class cbInst, string cbFunc, bool allowChars)
	{
		// the open dialog keeps its callback (a second one would take its answer)
		if (IsDialogOpen())
		{
			return;
		}

		m_DialogCbInst = cbInst;
		m_DialogCbFunc = cbFunc;
		if (m_MapTab)
		{
			m_MapTab.SetMapVisible(false);
		}

		if (m_EventsTab)
		{
			m_EventsTab.SetMapAllowed(false);
		}

		m_Dialog = GetVPPUIManager().CreateDialogBox(M_SUB_WIDGET);
		if (!m_Dialog)
		{
			m_DialogCbInst = null;
			m_DialogCbFunc = "";
			RestoreMapAfterDialog();
			return;
		}

		m_DialogWasOpen = true;
		m_Dialog.InitDiagBox(diagType, Tr(title), Tr(body), this, "OnDialogResult");
		if (allowChars)
		{
			m_Dialog.AllowCharInput();
		}
	}

	//VPPDialogBox calls this with one int (YES/NO/CANCEL) or with Param2 (input OK)
	// Object Manager, SEND TO EVENT GROUP: the objects of a local build go back to their group (EVENTS tab). false when
	// the tab could not take them now (the Object Manager keeps the build then).
	bool ApplyBuilderEdit(VPPXEGroupImport build, string groupName)
	{
		ShowTab(VPPXETab.EVENTS);
		return m_EventsTab.ApplyBuilderEdit(build, groupName);
	}

	// An input dialog with text already in the box.
	void OpenConfirmInput(string title, string body, Class cbInst, string cbFunc, string prefill)
	{
		OpenConfirm(title, body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, cbInst, cbFunc, true);
		if (m_Dialog)
		{
			m_Dialog.SetInputText(prefill);
		}
	}

	void OnDialogResult(int result, string input)
	{
		Class cbInst = m_DialogCbInst;
		string cbFunc = m_DialogCbFunc;
		m_Dialog = null;
		m_DialogWasOpen = false;
		m_DialogCbInst = null;
		m_DialogCbFunc = "";
		RestoreMapAfterDialog();
		if (cbInst && cbFunc != "")
		{
			GetGame().GameScript.CallFunctionParams(cbInst, cbFunc, null, new Param2<int, string>(result, input));
		}
	}

	bool IsDialogOpen()
	{
		return m_Dialog != null;
	}

	protected void RestoreMapAfterDialog()
	{
		if (m_EventsTab)
		{
			m_EventsTab.SetMapAllowed(IsSubMenuVisible() && !IsDialogOpen() && !m_BrokenHidden);
		}

		if (!m_MapTab)
		{
			return;
		}

		if (m_ActiveTab == VPPXETab.MAP && IsSubMenuVisible() && !IsDialogOpen())
		{
			m_MapTab.SetMapVisible(true);
		}
	}

	//a dialog that vanished without a callback (menu closed, HUD closed) must not keep the map hidden
	protected void CheckDialogHandle()
	{
		if (m_DialogWasOpen && !IsDialogOpen())
		{
			m_DialogWasOpen = false;
			m_DialogCbInst = null;
			m_DialogCbFunc = "";
			RestoreMapAfterDialog();
		}
	}

	override void HideBrokenWidgets(bool state)
	{
		m_BrokenHidden = state;
		if (m_EventsTab)
		{
			m_EventsTab.SetMapAllowed(!state && !IsDialogOpen());
		}

		if (!m_MapTab)
		{
			return;
		}

		bool visible = false;
		if (!state && m_ActiveTab == VPPXETab.MAP && !IsDialogOpen())
		{
			visible = true;
		}

		m_MapTab.SetMapVisible(visible);
	}

	//---------------------------------------------------------------- requests

	protected void SendOpenSession()
	{
		m_SessionOpen = true;
		m_CloseWanted = false;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CloseSessionTick);
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_OpenSession", new Param1<int>(VPPXEConst.PROTOCOL), true, null);
	}

	//XE_CloseSession once the window is hidden. The server then drops this admin's pending requests and queued
	//chunks, so the close waits while an index reply, a save lock or an upload is still in flight (all of them
	//end or expire through CheckTimeouts, which keeps running while the window is hidden).
	//Tab streams (issues, backup list/diff, distribution, live scan, queued live deletes) are registered by the
	//tabs and do not touch m_LastReqActivity, so the close also waits while a tab reports HasInFlight() (bounded
	//by INFLIGHT_TIMEOUT_MS since the tab's last progress, as a hidden tab's OnUpdate does not expire it), and
	//until no request id was issued and no shell progress arrived for INFLIGHT_TIMEOUT_MS.
	protected void TryCloseSession()
	{
		if (!m_CloseWanted || !m_SessionOpen || IsSubMenuVisible())
		{
			return;
		}

		if (m_UploadPumping || m_Model.HasPendingIndex() || m_Model.IsAnyLocked())
		{
			return;
		}

		if (m_FilesTab && m_FilesTab.HasInFlight())
		{
			return;
		}

		if (m_CreateTab && m_CreateTab.HasInFlight())
		{
			return;
		}

		if (m_MessagesTab && m_MessagesTab.HasInFlight())
		{
			return;
		}

		if (m_EventsTab && m_EventsTab.HasInFlight())
		{
			return;
		}

		if (m_SpawnablesTab && m_SpawnablesTab.HasInFlight())
		{
			return;
		}

		if (m_BackupsTab && m_BackupsTab.HasInFlight())
		{
			return;
		}

		if (m_MapTab && m_MapTab.HasInFlight())
		{
			return;
		}

		if (GetGame().GetTime() - m_LastReqActivity < VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return;
		}

		m_CloseWanted = false;
		m_SessionOpen = false;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CloseSessionTick);
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_CloseSession", new Param1<int>(0), true, null);
	}

	//1 s GUI call-queue repeat started by OnMenuHide while a close is deferred. OnUpdate only runs while the
	//admin HUD is on the menu stack, so without this a deferred close (and the timeouts it waits on) would stall
	//until the HUD is opened again.
	protected void CloseSessionTick()
	{
		if (!m_Loaded || !m_CloseWanted || !m_SessionOpen || IsSubMenuVisible())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CloseSessionTick);
			return;
		}

		CheckTimeouts();
		TryCloseSession();
	}

	void RequestDetails()
	{
		if (m_SelectedType == "")
		{
			return;
		}

		m_DetailsReqId = NextReqId();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetTypeDetails", new Param2<int, string>(m_DetailsReqId, m_SelectedType), true, null);
	}

	protected void DoRefresh()
	{
		m_IndexRetries.Clear();
		m_Model.RequestIndex("");
		SendOpenSession();
		RequestDetails();
		SetStatus("#VSTR_XMLE_STATUS_LOADING");
		if (m_ActiveTab == VPPXETab.TYPES)
		{
			m_TypesTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.MAP)
		{
			m_MapTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.BACKUPS)
		{
			m_BackupsTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.FILES)
		{
			m_FilesTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.MESSAGES)
		{
			m_MessagesTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.EVENTS)
		{
			m_EventsTab.Refresh();
		}
		else if (m_ActiveTab == VPPXETab.SPAWNABLES)
		{
			m_SpawnablesTab.Refresh();
		}
		else
		{
			m_CreateTab.Refresh();
		}
	}

	//---------------------------------------------------------------- paced uploads (UPLOAD_PARTS_PER_100MS)

	void QueueUpload(VPPXEUploadBatch up)
	{
		if (!up)
		{
			return;
		}

		//every save locks its files (LockFile) right before it is queued here: remember which request holds each lock
		m_SaveLockOwners.Set(up.FileKey, up.ReqId);
		if (up.TargetFileKey != "")
		{
			m_SaveLockOwners.Set(up.TargetFileKey, up.ReqId);
		}

		for (int i = 0; i < up.Parts.Count(); i++)
		{
			m_UploadQueue.Insert(up.Parts[i]);
		}

		if (m_UploadPumping)
		{
			return;
		}

		m_UploadPumping = true;
		PumpUploads();
		if (m_UploadPumping)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.PumpUploads, 100, true);
		}
	}

	void PumpUploads()
	{
		int sent = 0;
		while (m_UploadCursor < m_UploadQueue.Count() && sent < VPPXEConst.UPLOAD_PARTS_PER_100MS)
		{
			VPPXEEditBatch part = m_UploadQueue[m_UploadCursor];
			m_UploadCursor++;
			if (!part)
			{
				continue;
			}

			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveTypeEdits", new Param1<ref VPPXEEditBatch>(part), true, null);
			sent++;
			if (part.PartIdx == part.PartCount - 1)
			{
				m_Model.TouchLocks(part.ReqId);
			}
		}

		if (m_UploadCursor < m_UploadQueue.Count())
		{
			return;
		}

		m_UploadQueue.Clear();
		m_UploadCursor = 0;
		if (m_UploadPumping)
		{
			m_UploadPumping = false;
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.PumpUploads);
		}
	}

	protected void CancelUploads(int reqId)
	{
		array<ref VPPXEEditBatch> keep = new array<ref VPPXEEditBatch>();
		for (int i = m_UploadCursor; i < m_UploadQueue.Count(); i++)
		{
			VPPXEEditBatch part = m_UploadQueue[i];
			if (part && part.ReqId != reqId)
			{
				keep.Insert(part);
			}
		}

		m_UploadQueue = keep;
		m_UploadCursor = 0;
	}

	//true while fileKey is still locked by reqId; false once that lock expired (CheckTimeouts) or a newer save took it,
	//in which case edits staged since the expiry are not part of reqId's batch
	protected bool OwnsSaveLock(string fileKey, int reqId)
	{
		if (!m_Model.IsFileLocked(fileKey))
		{
			return false;
		}

		int owner;
		if (!m_SaveLockOwners.Find(fileKey, owner))
		{
			return false;
		}

		return owner == reqId;
	}

	protected void ForgetSaveLockOwners(int reqId)
	{
		array<string> keys = new array<string>();
		for (int i = 0; i < m_SaveLockOwners.Count(); i++)
		{
			if (m_SaveLockOwners.GetElement(i) == reqId)
			{
				keys.Insert(m_SaveLockOwners.GetKey(i));
			}
		}

		for (int j = 0; j < keys.Count(); j++)
		{
			m_SaveLockOwners.Remove(keys[j]);
		}
	}

	//---------------------------------------------------------------- tabs

	protected bool CanOpenTab(int tab)
	{
		if (tab == VPPXETab.MAP)
		{
			return HasPerm(VPPXEPerm.DIST_MAP) || HasPerm(VPPXEPerm.LIVE_SCAN);
		}

		if (tab == VPPXETab.BACKUPS)
		{
			return HasPerm(VPPXEPerm.VIEW_BACKUPS);
		}

		return true;
	}

	void ShowTab(int tab)
	{
		if (tab < 0 || tab >= m_TabHosts.Count())
		{
			return;
		}

		if (!CanOpenTab(tab))
		{
			NotifyError("#VSTR_XMLE_NO_PERMISSION_TAB");
			return;
		}

		m_ActiveTab = tab;
		for (int i = 0; i < m_TabHosts.Count(); i++)
		{
			Widget host = m_TabHosts[i];
			if (host)
			{
				host.Show(i == tab);
			}
		}

		m_TypesTab.Show(tab == VPPXETab.TYPES);
		m_BackupsTab.Show(tab == VPPXETab.BACKUPS);
		m_FilesTab.Show(tab == VPPXETab.FILES);
		m_CreateTab.Show(tab == VPPXETab.CREATE);
		m_MessagesTab.Show(tab == VPPXETab.MESSAGES);
		m_EventsTab.Show(tab == VPPXETab.EVENTS);
		m_SpawnablesTab.Show(tab == VPPXETab.SPAWNABLES);
		m_MapTab.Show(tab == VPPXETab.MAP);
		ApplyRailVisibility();
		UpdateTabButtons();
		UpdateReadOnlyBanner();
		CheckDialogHandle();
		m_LastBodyW = -1;
	}

	//re-runs the active tab's show path when the window is shown again (OnMenuShow, HUD reopen). A tab's
	//OnUpdate does not run while the window is hidden, so a tab request that expired meanwhile is dropped
	//silently and asked again here, before that OnUpdate could report it as an error.
	protected void ReshowActiveTab()
	{
		if (!CanOpenTab(m_ActiveTab))
		{
			return;
		}

		if (m_ActiveTab == VPPXETab.FILES)
		{
			m_FilesTab.Show(true);
		}
		else if (m_ActiveTab == VPPXETab.CREATE)
		{
			m_CreateTab.Show(true);
		}
		else if (m_ActiveTab == VPPXETab.MESSAGES)
		{
			m_MessagesTab.Show(true);
		}
		else if (m_ActiveTab == VPPXETab.EVENTS)
		{
			m_EventsTab.Show(true);
			m_EventsTab.SetMapAllowed(!m_BrokenHidden && !IsDialogOpen());
		}
		else if (m_ActiveTab == VPPXETab.SPAWNABLES)
		{
			m_SpawnablesTab.Show(true);
		}
		else if (m_ActiveTab == VPPXETab.BACKUPS)
		{
			m_BackupsTab.Show(true);
		}
		else if (m_ActiveTab == VPPXETab.MAP)
		{
			m_MapTab.Show(true);
			//Show(true) makes the map visible: keep it hidden while another admin window has the priority
			if (m_BrokenHidden)
			{
				m_MapTab.SetMapVisible(false);
			}
		}
	}

	protected void ApplyRailVisibility()
	{
		bool railVisible = !m_RailToggledOff;
		if (m_ActiveTab != VPPXETab.TYPES && m_ActiveTab != VPPXETab.MAP)
		{
			railVisible = false;
		}

		m_PanelRail.Show(railVisible);
		if (railVisible)
		{
			m_PanelContent.SetPos(0.288, 0);
			m_PanelContent.SetSize(0.708, 1);
		}
		else
		{
			m_PanelContent.SetPos(0.004, 0);
			m_PanelContent.SetSize(0.992, 1);
		}

		if (m_ImgToggleRail)
		{
			if (m_RailToggledOff)
			{
				m_ImgToggleRail.SetColor(ARGB(255, 154, 160, 166));
			}
			else
			{
				m_ImgToggleRail.SetColor(ARGB(255, 232, 163, 61));
			}
		}

		m_LastBodyW = -1;
	}

	protected void UpdateReadOnlyBanner()
	{
		bool readOnly = false;
		if (m_Model.GetSession() && !HasPerm(VPPXEPerm.EDIT_TYPES))
		{
			readOnly = true;
		}

		m_ReadOnlyBanner.Show(readOnly && m_ActiveTab == VPPXETab.TYPES);
	}

	//tab labels: active text-primary, others text-secondary, not permitted text-muted; buttons sized to their labels
	protected void UpdateTabButtons()
	{
		float x = TAB_X0;
		for (int oi = 0; oi < m_TabOrder.Count(); oi++)
		{
			int i = m_TabOrder[oi];
			if (i < 0 || i >= m_TabButtons.Count())
			{
				continue;
			}

			ButtonWidget btn = m_TabButtons[i];
			TextWidget txt = m_TabTexts[i];
			ImageWidget img = m_TabImages[i];
			if (!btn)
			{
				continue;
			}

			bool allowed = CanOpenTab(i);
			int color = ARGB(255, 154, 160, 166);
			if (i == m_ActiveTab)
			{
				color = ARGB(255, 255, 255, 255);
			}
			else if (!allowed)
			{
				color = ARGB(255, 92, 97, 102);
			}

			btn.Enable(allowed);
			if (txt)
			{
				txt.SetColor(color);
			}

			if (img)
			{
				img.SetColor(color);
			}

			string label = Tr(m_TabKeys[i]);
			float width = Math.Clamp(50 + 10 * label.LengthUtf8(), 90, 220);
			if (i == VPPXETab.FILES && m_TxtFilesBadge && m_TxtFilesBadge.IsVisible())
			{
				width = width + 30;
			}

			btn.SetPos(x, 0);
			btn.SetSize(width, 24);
			x = x + width + 4;
		}
	}

	protected void ResizeActiveTab()
	{
		if (m_ActiveTab == VPPXETab.TYPES)
		{
			m_TypesTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.MAP)
		{
			m_MapTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.BACKUPS)
		{
			m_BackupsTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.FILES)
		{
			m_FilesTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.MESSAGES)
		{
			m_MessagesTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.EVENTS)
		{
			m_EventsTab.OnResize();
		}
		else if (m_ActiveTab == VPPXETab.SPAWNABLES)
		{
			m_SpawnablesTab.OnResize();
		}
		else
		{
			m_CreateTab.OnResize();
		}
	}

	protected void UpdateActiveTab(float timeslice)
	{
		if (m_ActiveTab == VPPXETab.TYPES)
		{
			m_TypesTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.MAP)
		{
			m_MapTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.BACKUPS)
		{
			m_BackupsTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.FILES)
		{
			m_FilesTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.MESSAGES)
		{
			m_MessagesTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.EVENTS)
		{
			m_EventsTab.OnUpdate(timeslice);
		}
		else if (m_ActiveTab == VPPXETab.SPAWNABLES)
		{
			m_SpawnablesTab.OnUpdate(timeslice);
		}
		else
		{
			m_CreateTab.OnUpdate(timeslice);
		}
	}

	//---------------------------------------------------------------- header, badge, rail footer

	protected void UpdateSubtitle()
	{
		VPPXESessionInfo session = m_Model.GetSession();
		if (!session)
		{
			return;
		}

		string scopeLabel = session.WorldName;
		int count = m_Model.GetNameCount();
		if (m_Scope != "")
		{
			scopeLabel = m_Model.FileLabel(m_Scope);
			count = m_Model.GetFileNameCount(m_Scope);
			VPPXEFileInfo scopeInfo = m_Model.GetFile(m_Scope);
			if (count == 0 && scopeInfo)
			{
				count = scopeInfo.EntryCount;
			}
		}
		else if (count == 0)
		{
			array<string> typesFiles = m_Model.GetTypesFiles();
			for (int i = 0; i < typesFiles.Count(); i++)
			{
				VPPXEFileInfo fileInfo = m_Model.GetFile(typesFiles[i]);
				if (fileInfo)
				{
					count = count + fileInfo.EntryCount;
				}
			}
		}

		m_SubTitle.SetText(string.Format(Tr("#VSTR_XMLE_SUBTITLE_FMT"), scopeLabel, count));
	}

	protected void UpdatePendingChip()
	{
		bool pending = false;
		for (int i = 0; i < m_Model.GetFileCount(); i++)
		{
			VPPXEFileInfo fileInfo = m_Model.GetFileByIdx(i);
			if (fileInfo && (fileInfo.Flags & VPPXEFileFlag.PENDING_RESTART) != 0)
			{
				pending = true;
				break;
			}
		}

		string label = Tr("#VSTR_XMLE_PENDING_RESTART");
		float width = Math.Clamp(16 + 7 * label.LengthUtf8(), 80, 220);
		m_PendingChip.SetSize(width, 22);
		m_PendingChip.Show(pending);
	}

	//sum of FileInfo.IssueCount; red when a file has PARSE_ERROR, READ_ERROR or INTERRUPTED, else yellow
	protected void UpdateBadge()
	{
		int total = 0;
		bool broken = false;
		int badFlags = VPPXEFileFlag.PARSE_ERROR | VPPXEFileFlag.READ_ERROR | VPPXEFileFlag.INTERRUPTED;
		for (int i = 0; i < m_Model.GetFileCount(); i++)
		{
			VPPXEFileInfo fileInfo = m_Model.GetFileByIdx(i);
			if (!fileInfo)
			{
				continue;
			}

			total = total + fileInfo.IssueCount;
			if ((fileInfo.Flags & badFlags) != 0)
			{
				broken = true;
			}
		}

		m_TxtFilesBadge.SetText(total.ToString());
		m_TxtFilesBadge.Show(total > 0);
		if (broken)
		{
			m_TxtFilesBadge.SetColor(ARGB(255, 194, 69, 69));
		}
		else
		{
			m_TxtFilesBadge.SetColor(ARGB(255, 217, 178, 61));
		}

		UpdateTabButtons();
	}

	//BtnAddType: hidden without ADD_REMOVE, disabled without an EDITABLE target; buttons sized to their labels
	protected void UpdateAddButton()
	{
		bool addRemove = HasPerm(VPPXEPerm.ADD_REMOVE);
		string target = "";
		if (m_TypesTab)
		{
			target = m_TypesTab.AddTargetFile();
		}

		bool addOk = addRemove && HasPerm(VPPXEPerm.EDIT_TYPES) && target != "" && !m_Model.IsFileLocked(target);
		m_BtnAddType.Show(addRemove);
		m_BtnAddType.Enable(addOk);
		m_BtnBulkEdit.Enable(HasPerm(VPPXEPerm.EDIT_TYPES));
		SizeRailButtons();
	}

	protected void SizeRailButtons()
	{
		string addLabel = Tr("#VSTR_XMLE_BTN_ADD");
		string bulkLabel = Tr("#VSTR_XMLE_BTN_BULK");
		float addW = Math.Clamp(36 + 7 * addLabel.LengthUtf8(), 56, 122);
		float bulkW = Math.Clamp(36 + 7 * bulkLabel.LengthUtf8(), 56, 122);
		float x = 8;
		if (m_BtnAddType.IsVisible())
		{
			m_BtnAddType.SetPos(x, 26);
			m_BtnAddType.SetSize(addW, 26);
			x = x + addW + 6;
		}

		m_BtnBulkEdit.SetPos(x, 26);
		m_BtnBulkEdit.SetSize(bulkW, 26);
	}

	//screen pixels per layout unit, measured on the exact 30-unit TabBar row (Main is "scaled 1", so units are not pixels)
	protected float GetUnit()
	{
		if (!m_TabBar)
		{
			return 1.0;
		}

		float barW;
		float barH;
		m_TabBar.GetScreenSize(barW, barH);
		if (barH <= 1)
		{
			return 1.0;
		}

		return barH / TAB_BAR_H;
	}

	//Body and TypesList heights in layout units (the layout cannot express "parent minus N units")
	protected void Layout()
	{
		float mainW;
		float mainH;
		m_Main.GetScreenSize(mainW, mainH);
		if (mainH <= 0)
		{
			return;
		}

		float unit = GetUnit();
		float bodyH = mainH / unit - 68 - 4;
		if (bodyH < 120)
		{
			bodyH = 120;
		}

		m_Body.SetSize(1, bodyH);
		float listH = bodyH - 118 - 56 - 4;
		if (listH < 40)
		{
			listH = 40;
		}

		m_TypesList.SetSize(0.95, listH);
	}

	//---------------------------------------------------------------- scope and Show filter dropdowns

	//'All' plus at most FILTER_DD_MAX_ROWS - 1 files (the dropdown GridSpacer shows 50 rows); the current scope is always
	//kept, and rows of files past the cap stay reachable through the merged 'All' scope
	protected void RebuildScopeDropdown()
	{
		array<string> keys = new array<string>();
		array<string> labels = new array<string>();
		keys.Insert("");
		labels.Insert(Tr("#VSTR_XMLE_SCOPE_ALL"));
		array<string> typesFiles = m_Model.GetTypesFiles();
		for (int i = 0; i < typesFiles.Count(); i++)
		{
			if (keys.Count() >= FILTER_DD_MAX_ROWS)
			{
				break;
			}

			keys.Insert(typesFiles[i]);
			labels.Insert(m_Model.FileLabel(typesFiles[i]));
		}

		if (m_Scope != "" && keys.Find(m_Scope) < 0 && typesFiles.Find(m_Scope) >= 0 && keys.Count() > 1)
		{
			int lastIdx = keys.Count() - 1;
			keys.Set(lastIdx, m_Scope);
			labels.Set(lastIdx, m_Model.FileLabel(m_Scope));
		}

		string sig = "";
		for (int j = 0; j < labels.Count(); j++)
		{
			sig = sig + labels[j] + "|";
		}

		if (sig == m_ScopeSig)
		{
			return;
		}

		m_ScopeSig = sig;
		m_ScopeKeys = keys;
		m_ScopeLabels = labels;
		m_ScopeDD.RemoveAllElements();
		for (int k = 0; k < m_ScopeLabels.Count(); k++)
		{
			m_ScopeDD.AddElement(m_ScopeLabels[k]);
		}

		if (m_ScopeKeys.Find(m_Scope) < 0)
		{
			m_Scope = "";
		}

		int idx = m_ScopeKeys.Find(m_Scope);
		m_ScopeDD.SetText(m_ScopeLabels[idx]);
		m_ScopeDD.SetIndex(idx);
	}

	void OnScopeSelected(int index)
	{
		if (index < 0 || index >= m_ScopeKeys.Count())
		{
			m_ScopeDD.Close();
			return;
		}

		m_ScopeDD.SetText(m_ScopeLabels[index]);
		m_ScopeDD.SetIndex(index);
		m_ScopeDD.Close();
		SetTypesScope(m_ScopeKeys[index]);
	}

	protected void AddFilter(int kind, int arg, string label)
	{
		if (m_FilterLabels.Count() >= FILTER_DD_MAX_ROWS)
		{
			return;
		}

		m_FilterKinds.Insert(kind);
		m_FilterArgs.Insert(arg);
		m_FilterLabels.Insert(label);
	}

	protected void AddLimitsFilters(int kind, string key, array<string> names, int cap)
	{
		if (!names)
		{
			return;
		}

		string fmt = Tr(key);
		for (int i = 0; i < names.Count() && i < cap; i++)
		{
			AddFilter(kind, i, string.Format(fmt, names[i]));
		}
	}

	protected void RebuildFilterDropdown()
	{
		int oldKind = CurrentFilterKind();
		int oldArg = CurrentFilterArg();
		m_FilterKinds.Clear();
		m_FilterArgs.Clear();
		m_FilterLabels.Clear();
		AddFilter(VPPXEClientModel.FILTER_ALL, -1, Tr("#VSTR_XMLE_FILTER_ALL"));
		AddFilter(VPPXEClientModel.FILTER_UNREGISTERED, -1, Tr("#VSTR_XMLE_FILTER_UNREGISTERED"));
		AddFilter(VPPXEClientModel.FILTER_ISSUES, -1, Tr("#VSTR_XMLE_FILTER_ISSUES"));
		AddFilter(VPPXEClientModel.FILTER_OVERRIDDEN, -1, Tr("#VSTR_XMLE_FILTER_OVERRIDDEN"));
		AddFilter(VPPXEClientModel.FILTER_UNSAVED, -1, Tr("#VSTR_XMLE_FILTER_UNSAVED"));
		AddFilter(VPPXEClientModel.FILTER_CONFLICTS, -1, Tr("#VSTR_XMLE_FILTER_CONFLICTS"));
		AddFilter(VPPXEClientModel.FILTER_SPAWNING, -1, Tr("#VSTR_XMLE_FILTER_SPAWNING"));
		AddFilter(VPPXEClientModel.FILTER_NOSPAWN, -1, Tr("#VSTR_XMLE_FILTER_NOSPAWN"));
		AddFilter(VPPXEClientModel.FILTER_NOCAT, -1, Tr("#VSTR_XMLE_FILTER_NOCAT"));
		VPPXELimits lim = m_Model.GetLimits();
		AddLimitsFilters(VPPXEClientModel.FILTER_CATEGORY, "#VSTR_XMLE_FILTER_CATEGORY", lim.Categories, 1000);
		AddLimitsFilters(VPPXEClientModel.FILTER_USAGE, "#VSTR_XMLE_FILTER_USAGE", lim.Usages, 32);
		AddLimitsFilters(VPPXEClientModel.FILTER_VALUE, "#VSTR_XMLE_FILTER_VALUE", lim.Values, 32);
		AddLimitsFilters(VPPXEClientModel.FILTER_TAG, "#VSTR_XMLE_FILTER_TAG", lim.Tags, 32);

		string sig = "";
		for (int i = 0; i < m_FilterLabels.Count(); i++)
		{
			sig = sig + m_FilterLabels[i] + "|";
		}

		m_FilterIdx = 0;
		for (int j = 0; j < m_FilterKinds.Count(); j++)
		{
			if (m_FilterKinds[j] == oldKind && m_FilterArgs[j] == oldArg)
			{
				m_FilterIdx = j;
				break;
			}
		}

		if (sig != m_FilterSig)
		{
			m_FilterSig = sig;
			m_FilterDD.RemoveAllElements();
			for (int k = 0; k < m_FilterLabels.Count(); k++)
			{
				m_FilterDD.AddElement(m_FilterLabels[k]);
			}
		}

		m_FilterDD.SetText(m_FilterLabels[m_FilterIdx]);
		m_FilterDD.SetIndex(m_FilterIdx);
	}

	void OnFilterSelected(int index)
	{
		if (index < 0 || index >= m_FilterLabels.Count())
		{
			m_FilterDD.Close();
			return;
		}

		m_FilterDD.SetText(m_FilterLabels[index]);
		m_FilterDD.SetIndex(index);
		m_FilterDD.Close();
		m_FilterIdx = index;
		RebuildRail();
	}

	protected int CurrentFilterKind()
	{
		if (m_FilterIdx < 0 || m_FilterIdx >= m_FilterKinds.Count())
		{
			return VPPXEClientModel.FILTER_ALL;
		}

		return m_FilterKinds[m_FilterIdx];
	}

	protected int CurrentFilterArg()
	{
		if (m_FilterIdx < 0 || m_FilterIdx >= m_FilterArgs.Count())
		{
			return -1;
		}

		return m_FilterArgs[m_FilterIdx];
	}

	//---------------------------------------------------------------- rail

	protected int RailColor(string lower, map<string, int> stagedFlags)
	{
		int bits = stagedFlags.Get(lower);
		if ((bits & VPPXEClientModel.STAGED_CONFLICT) != 0)
		{
			return ARGB(255, 61, 125, 214);
		}

		if ((bits & VPPXEClientModel.STAGED_PENDING) != 0)
		{
			return ARGB(255, 232, 163, 61);
		}

		int severity = m_Model.RowSeverityOf(lower, m_Scope);
		if (severity == VPPXESeverity.ERROR)
		{
			return ARGB(255, 194, 69, 69);
		}

		if (severity == VPPXESeverity.WARNING)
		{
			return ARGB(255, 217, 178, 61);
		}

		return ARGB(255, 255, 255, 255);
	}

	protected string ScalarCell(string lower, int fieldBit)
	{
		int num;
		string winner;
		if (m_Model.DisplayScalar(lower, m_Scope, fieldBit, num, winner))
		{
			return num.ToString();
		}

		return "-";
	}

	protected void FillRailRow(int row, string lower, map<string, int> stagedFlags)
	{
		m_TypesList.SetItem(row, ScalarCell(lower, VPPXEField.NOMINAL), null, 1);
		m_TypesList.SetItem(row, ScalarCell(lower, VPPXEField.MIN), null, 2);
		int color = RailColor(lower, stagedFlags);
		m_TypesList.SetItemColor(row, 0, color);
		m_TypesList.SetItemColor(row, 1, color);
		m_TypesList.SetItemColor(row, 2, color);
	}

	//rows in the model's sorted order (computed once per index change), filtered by scope, Show filter and search
	protected void RebuildRail()
	{
		if (!m_Model || !m_TypesList)
		{
			return;
		}

		string searchText = "";
		if (m_SearchInput)
		{
			searchText = m_SearchInput.GetText();
		}

		if (CurrentFilterKind() == VPPXEClientModel.FILTER_UNREGISTERED)
		{
			RebuildUnregisteredRail(searchText);
			return;
		}

		array<string> matches = new array<string>();
		int total = m_Model.CountScope(m_Scope);
		int count = m_Model.CollectMatches(m_Scope, searchText, CurrentFilterKind(), CurrentFilterArg(), matches);
		map<string, int> stagedFlags = new map<string, int>();
		m_Model.BuildStagedFlags(m_Scope, stagedFlags);
		m_TypesList.ClearItems();
		m_RailNames.Clear();
		m_RailIndex.Clear();
		int shown = count;
		if (shown > VPPXEConst.RAIL_MAX_ROWS)
		{
			shown = VPPXEConst.RAIL_MAX_ROWS;
		}

		string selectedLower = VPPXEClientModel.LowerOf(m_SelectedType);
		int selectedRow = -1;
		for (int i = 0; i < shown; i++)
		{
			string lower = matches[i];
			int row = m_TypesList.AddItem(m_Model.DisplayNameOf(lower), null, 0);
			FillRailRow(row, lower, stagedFlags);
			m_RailNames.Insert(lower);
			m_RailIndex.Set(lower, row);
			if (lower == selectedLower)
			{
				selectedRow = row;
			}
		}

		if (count > VPPXEConst.RAIL_MAX_ROWS)
		{
			m_TxtRowCount.SetText(string.Format(Tr("#VSTR_XMLE_ROWS_CAPPED"), shown, count));
		}
		else
		{
			m_TxtRowCount.SetText(string.Format(Tr("#VSTR_XMLE_ROWCOUNT"), count, total));
		}

		if (selectedRow >= 0)
		{
			m_TypesList.SelectRow(selectedRow);
			m_TypesList.EnsureVisible(selectedRow);
		}

		m_LastRailRow = m_TypesList.GetSelectedRow();
		if (m_TypesTab)
		{
			m_TypesTab.OnFilterChanged();
		}
	}

	//"not registered" filter: game config classes without a types entry, grey, with a green "+" (can be registered)
	protected void RebuildUnregisteredRail(string searchText)
	{
		array<string> matches = new array<string>();
		int count = m_Model.CollectUnregistered(searchText, matches);
		int total = m_Model.GetUnregisteredTotal();
		int nameColor = ARGB(255, 154, 160, 166);
		int plusColor = ARGB(255, 76, 175, 80);
		m_TypesList.ClearItems();
		m_RailNames.Clear();
		m_RailIndex.Clear();
		int shown = count;
		if (shown > VPPXEConst.RAIL_MAX_ROWS)
		{
			shown = VPPXEConst.RAIL_MAX_ROWS;
		}

		string selectedLower = VPPXEClientModel.LowerOf(m_UnregSelected);
		int selectedRow = -1;
		for (int i = 0; i < shown; i++)
		{
			string lower = matches[i];
			int row = m_TypesList.AddItem(VPPXEConfigClasses.DisplayOf(lower), null, 0);
			m_TypesList.SetItem(row, "+", null, 1);
			m_TypesList.SetItem(row, "", null, 2);
			m_TypesList.SetItemColor(row, 0, nameColor);
			m_TypesList.SetItemColor(row, 1, plusColor);
			m_TypesList.SetItemColor(row, 2, nameColor);
			m_RailNames.Insert(lower);
			m_RailIndex.Set(lower, row);
			if (lower == selectedLower)
			{
				selectedRow = row;
			}
		}

		if (count > VPPXEConst.RAIL_MAX_ROWS)
		{
			m_TxtRowCount.SetText(string.Format(Tr("#VSTR_XMLE_ROWS_CAPPED"), shown, count));
		}
		else
		{
			m_TxtRowCount.SetText(string.Format(Tr("#VSTR_XMLE_ROWCOUNT"), count, total));
		}

		if (selectedRow >= 0)
		{
			m_TypesList.SelectRow(selectedRow);
			m_TypesList.EnsureVisible(selectedRow);
		}

		m_LastRailRow = m_TypesList.GetSelectedRow();
		if (m_TypesTab)
		{
			m_TypesTab.OnFilterChanged();
		}
	}

	protected void RefreshRailRow(string lower)
	{
		if (CurrentFilterKind() == VPPXEClientModel.FILTER_UNREGISTERED)
		{
			return;
		}

		int row;
		if (!m_RailIndex.Find(lower, row))
		{
			return;
		}

		map<string, int> stagedFlags = new map<string, int>();
		m_Model.BuildStagedFlags(m_Scope, stagedFlags);
		FillRailRow(row, lower, stagedFlags);
	}

	//called by the TYPES tab after staging changes ("" = several types or files changed)
	void OnStagingChanged(string typeName)
	{
		if (typeName == "")
		{
			RebuildRail();
		}
		else
		{
			RefreshRailRow(VPPXEClientModel.LowerOf(typeName));
		}

		UpdateAddButton();
	}

	protected void SelectRailRow(string typeName)
	{
		int row;
		if (!m_RailIndex.Find(VPPXEClientModel.LowerOf(typeName), row))
		{
			return;
		}

		if (m_TypesList.GetSelectedRow() != row)
		{
			m_TypesList.SelectRow(row);
			m_TypesList.EnsureVisible(row);
		}

		m_LastRailRow = row;
	}

	protected void OnRailRowChosen(int row)
	{
		m_LastRailRow = row;
		if (row < 0 || row >= m_RailNames.Count())
		{
			return;
		}

		string lower = m_RailNames[row];
		if (CurrentFilterKind() == VPPXEClientModel.FILTER_UNREGISTERED && !m_Model.HasName(lower))
		{
			if (lower != VPPXEClientModel.LowerOf(m_UnregSelected))
			{
				SelectUnregistered(VPPXEConfigClasses.DisplayOf(lower));
			}

			return;
		}

		if (lower == VPPXEClientModel.LowerOf(m_SelectedType))
		{
			return;
		}

		SelectType(m_Model.DisplayNameOf(lower), false);
	}

	protected void PollRailSelection()
	{
		int row = m_TypesList.GetSelectedRow();
		if (row != m_LastRailRow)
		{
			OnRailRowChosen(row);
		}
	}

	protected void PollSearch()
	{
		if (!m_SearchInput)
		{
			return;
		}

		int now = GetGame().GetTime();
		string current = m_SearchInput.GetText();
		if (current != m_LastSearch)
		{
			m_LastSearch = current;
			m_SearchPending = true;
			m_SearchChangedAt = now;
		}

		if (m_SearchPending && now - m_SearchChangedAt >= SEARCH_DEBOUNCE_MS)
		{
			m_SearchPending = false;
			RebuildRail();
		}
	}

	//---------------------------------------------------------------- lifecycle

	override void OnMenuShow()
	{
		super.OnMenuShow();
		if (!m_Loaded)
		{
			return;
		}

		if (m_SkipShowOpen)
		{
			m_SkipShowOpen = false;
		}
		else
		{
			SendOpenSession();
		}

		m_LastMainW = -1;
		m_LastBodyW = -1;
		CheckDialogHandle();
		ReshowActiveTab();
	}

	override void OnMenuHide()
	{
		super.OnMenuHide();
		if (!m_Loaded)
		{
			return;
		}

		m_CloseWanted = true;
		TryCloseSession();
		if (m_CloseWanted && m_SessionOpen)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.CloseSessionTick);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.CloseSessionTick, 1000, true);
		}
	}

	override void OnAdminHudOpened()
	{
		super.OnAdminHudOpened();
		if (!m_Loaded || !IsSubMenuVisible())
		{
			return;
		}

		SendOpenSession();
		CheckDialogHandle();
		ReshowActiveTab();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_Loaded)
		{
			return;
		}

		m_TimeoutTimer += timeslice;
		if (m_TimeoutTimer >= TIMEOUT_CHECK_INTERVAL)
		{
			m_TimeoutTimer = 0;
			CheckTimeouts();
			TryCloseSession();
		}

		if (!IsSubMenuVisible())
		{
			return;
		}

		float mainW;
		float mainH;
		m_Main.GetScreenSize(mainW, mainH);
		float curUnit = GetUnit();
		if (mainW != m_LastMainW || mainH != m_LastMainH || curUnit != m_LastUnit)
		{
			m_LastMainW = mainW;
			m_LastMainH = mainH;
			m_LastUnit = curUnit;
			Layout();
		}

		float bodyW;
		float bodyH;
		m_Body.GetScreenSize(bodyW, bodyH);
		if (bodyW != m_LastBodyW || bodyH != m_LastBodyH)
		{
			m_LastBodyW = bodyW;
			m_LastBodyH = bodyH;
			ResizeActiveTab();
		}

		CheckDialogHandle();
		PollSearch();
		PollRailSelection();
		UpdateActiveTab(timeslice);
	}

	//expired index requests (no chunk or progress for 30 s) are asked again up to INDEX_RETRY_MAX times per file
	//key before ERR_NOT_READY is shown; save locks older than INFLIGHT_TIMEOUT_MS re-request their file's index.
	//No retry while the window is hidden with a close pending: each request would push XE_CloseSession back, and
	//the XE_OnSession reply to the next XE_OpenSession recollects the missing index anyway.
	protected void CheckTimeouts()
	{
		array<int> expired = new array<int>();
		array<string> expiredKeys = new array<string>();
		m_Model.CollectExpiredIndexReqs(expired, expiredKeys);
		bool gaveUp = false;
		bool canRetry = !m_CloseWanted || IsSubMenuVisible();
		for (int e = 0; e < expiredKeys.Count(); e++)
		{
			string expiredKey = expiredKeys[e];
			int tries = 0;
			if (m_IndexRetries.Contains(expiredKey))
			{
				tries = m_IndexRetries.Get(expiredKey);
			}

			if (canRetry && tries < INDEX_RETRY_MAX)
			{
				m_IndexRetries.Set(expiredKey, tries + 1);
				m_Model.RequestIndex(expiredKey);
				continue;
			}

			m_IndexRetries.Remove(expiredKey);
			if (canRetry)
			{
				gaveUp = true;
			}
		}

		if (gaveUp)
		{
			NotifyError("#VSTR_XMLE_ERR_NOT_READY");
			SetStatus("#VSTR_XMLE_STATUS_READY");
		}

		array<string> expiredLocks = new array<string>();
		m_Model.CollectExpiredLocks(expiredLocks);
		for (int i = 0; i < expiredLocks.Count(); i++)
		{
			m_Model.RequestIndex(expiredLocks[i]);
		}

		if (expiredLocks.Count() > 0)
		{
			m_TypesTab.RefreshState(true);
			UpdateAddButton();
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!w)
		{
			return false;
		}

		if (w == m_BtnRefresh)
		{
			DoRefresh();
			return true;
		}

		if (w == m_BtnToggleRail)
		{
			m_RailToggledOff = !m_RailToggledOff;
			ApplyRailVisibility();
			return true;
		}

		for (int i = 0; i < m_TabButtons.Count(); i++)
		{
			if (m_TabButtons[i] && w == m_TabButtons[i])
			{
				ShowTab(i);
				return true;
			}
		}

		if (w == m_BtnAddType)
		{
			ShowTab(VPPXETab.TYPES);
			m_TypesTab.BeginAdd();
			return true;
		}

		if (w == m_BtnBulkEdit)
		{
			ShowTab(VPPXETab.TYPES);
			m_TypesTab.ShowBulk();
			return true;
		}

		return super.OnClick(w, x, y, button);
	}

	override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
	{
		if (w == m_TypesList)
		{
			OnRailRowChosen(row);
			return true;
		}

		return false;
	}

	//---------------------------------------------------------------- S2C receivers (INTERFACES section 4)

	void XE_OnSession(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXESessionInfo> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESessionInfo info = data.param1;
		if (!info || !m_Loaded)
		{
			return;
		}

		HandleSession(info);
	}

	void XE_OnProgress(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEProgress> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEProgress progress = data.param1;
		if (!progress || !m_Loaded)
		{
			return;
		}

		HandleProgress(progress);
	}

	void XE_OnActionResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<int, bool, string, string> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data) || !m_Loaded)
		{
			return;
		}

		HandleActionResult(data.param1, data.param2, data.param3, data.param4);
	}

	void XE_OnTypeIndexChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXETypeIndexChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			if (NET_TRACE)
			{
				Print("[XMLEditor][Net] client: XE_OnTypeIndexChunk arrived but could not be read");
			}

			return;
		}

		VPPXETypeIndexChunk chunk = data.param1;
		if (NET_TRACE && chunk)
		{
			int traceReq = chunk.ReqId;
			int traceIdx = chunk.ChunkIdx;
			int traceCount = chunk.ChunkCount;
			int traceRows = 0;
			if (chunk.Rows)
			{
				traceRows = chunk.Rows.Count();
			}

			bool traceOwned = false;
			if (m_Model)
			{
				traceOwned = m_Model.OwnsIndexReq(traceReq);
			}

			string traceLine = "[XMLEditor][Net] client: index chunk req " + traceReq.ToString() + " " + traceIdx.ToString() + "/" + traceCount.ToString() + ", " + traceRows.ToString() + " rows, awaited " + traceOwned.ToString();
			Print(traceLine);
		}

		if (NET_TRACE && !chunk)
		{
			Print("[XMLEditor][Net] client: XE_OnTypeIndexChunk arrived with a null payload");
		}

		if (!chunk || !m_Loaded)
		{
			return;
		}

		HandleIndexChunk(chunk);
	}

	void XE_OnTypeDetails(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXETypeDetails> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXETypeDetails details = data.param1;
		if (!details || !m_Loaded)
		{
			return;
		}

		if (VPPXEClientModel.LowerOf(details.Name) != VPPXEClientModel.LowerOf(m_SelectedType))
		{
			return;
		}

		m_TypesTab.ShowDetails(details);
	}

	void XE_OnSaveResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXESaveResult> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESaveResult result = data.param1;
		if (!result || !m_Loaded)
		{
			return;
		}

		if (!m_Model.IsSaveReq(result.ReqId))
		{
			m_BackupsTab.HandleSaveResult(result);
			return;
		}

		HandleOwnSaveResult(result);
	}

	//---------------------------------------------------------------- receiver logic

	protected void HandleSession(VPPXESessionInfo info)
	{
		int indexVersion = m_Model.GetIndexVersion();
		m_Model.SetSession(info);
		if (info.Protocol != VPPXEConst.PROTOCOL)
		{
			NotifyError("#VSTR_XMLE_ERR_PROTOCOL");
		}

		RebuildScopeDropdown();
		RebuildFilterDropdown();
		UpdatePendingChip();
		UpdateBadge();
		UpdateAddButton();
		UpdateReadOnlyBanner();
		if (!CanOpenTab(m_ActiveTab))
		{
			ShowTab(VPPXETab.TYPES);
		}
		else
		{
			UpdateTabButtons();
		}

		//every TYPES file whose Revision differs from the rows' FileRevision (all on the first session).
		//Skipped while the window is hidden with a close pending: each request would push the close back, and the
		//XE_OnSession reply to the next XE_OpenSession recollects the same needs from the rows' revisions.
		array<string> needs = new array<string>();
		if (!m_CloseWanted || IsSubMenuVisible())
		{
			m_Model.CollectIndexNeeds(needs);
		}

		for (int i = 0; i < needs.Count(); i++)
		{
			string key = needs[i];
			bool hadRows = m_Model.HasRows(key);
			m_Model.RequestIndex(key);
			if (hadRows && m_Model.HasPendingEdits(key) && !m_Model.IsFileLocked(key))
			{
				if (m_Model.TakeOtherAdminNotice(key, m_Model.GetSessionRevision(key)))
				{
					Notify(string.Format(Tr("#VSTR_XMLE_NOTIFY_OTHER_ADMIN"), m_Model.FileLabel(key)));
				}
			}
		}

		if (indexVersion != m_Model.GetIndexVersion())
		{
			RebuildRail();
		}

		UpdateSubtitle();
		if (!info.IndexReady || m_Model.HasPendingIndex())
		{
			SetStatus("#VSTR_XMLE_STATUS_LOADING");
		}
		else
		{
			SetStatus("#VSTR_XMLE_STATUS_READY");
		}

		m_TypesTab.OnSessionChanged();
		m_MapTab.OnSessionChanged();
		m_BackupsTab.OnSessionChanged();
		m_FilesTab.OnSessionChanged();
		m_CreateTab.OnSessionChanged();
		m_MessagesTab.OnSessionChanged();
		m_EventsTab.OnSessionChanged();
		m_SpawnablesTab.OnSessionChanged();
	}

	protected void HandleIndexChunk(VPPXETypeIndexChunk chunk)
	{
		array<string> doneFiles = new array<string>();
		array<int> doneRevs = new array<int>();
		if (!m_Model.AddIndexChunk(chunk, doneFiles, doneRevs))
		{
			return;
		}

		for (int i = 0; i < doneFiles.Count(); i++)
		{
			string key = doneFiles[i];
			m_IndexRetries.Remove(key);
			if (m_Model.IsRebasePending(key))
			{
				int orphans;
				int conflicts = m_Model.Rebase(key, orphans);
				string label = m_Model.FileLabel(key);
				Notify(string.Format(Tr("#VSTR_XMLE_NOTIFY_REBASED"), label, conflicts));
				if (orphans > 0)
				{
					Notify(string.Format(Tr("#VSTR_XMLE_NOTIFY_ORPHANED"), orphans, label));
				}
			}

			//the save lock ends with the first index reply that carries a new FileRevision; a reply older than the
			//session is not re-requested here (the next XE_OnSession does it), so no request loop is possible
			m_Model.ReleaseLockOnIndex(key, doneRevs[i]);
		}

		RebuildRail();
		UpdateSubtitle();
		UpdateAddButton();
		m_TypesTab.OnRowsChanged();
		CheckUnregisteredSelection();
		if (!m_Model.HasPendingIndex())
		{
			SetStatus("#VSTR_XMLE_STATUS_READY");
		}
	}

	protected void HandleOwnSaveResult(VPPXESaveResult result)
	{
		m_Model.ForgetSave(result.ReqId);
		CancelUploads(result.ReqId);
		string label = m_Model.FileLabel(result.FileKey);
		bool stale = result.ErrorKey == "#VSTR_XMLE_ERR_STALE" || result.ErrorKey == "VSTR_XMLE_ERR_STALE";
		if (result.Ok)
		{
			Notify(string.Format(Tr("#VSTR_XMLE_SAVED"), result.Applied, label));
			if (result.NoticeKey != "")
			{
				Notify(result.NoticeKey);
			}

			if (OwnsSaveLock(result.FileKey, result.ReqId))
			{
				//the file stayed locked by this request for the whole flight, so every staged edit of it was in the batch
				m_Model.ClearFile(result.FileKey);
				if (result.NewRevision == m_Model.GetLockRev(result.FileKey))
				{
					m_Model.UnlockFile(result.FileKey);
				}
				else
				{
					m_Model.SetLockAwaitIndex(result.FileKey);
				}
			}
			else if (m_Model.HasPendingEdits(result.FileKey))
			{
				//the lock expired before this late result (or a newer save holds it): edits may have been staged since,
				//so nothing is cleared; the rebase on the next index reply marks the types this batch changed as conflicts
				m_Model.MarkRebase(result.FileKey);
			}

			m_Model.RequestIndex(result.FileKey);
			if (result.TouchedFiles)
			{
				for (int i = 0; i < result.TouchedFiles.Count(); i++)
				{
					string touched = result.TouchedFiles[i];
					if (touched == result.FileKey)
					{
						continue;
					}

					if (m_Model.HasPendingEdits(touched))
					{
						m_Model.MarkRebase(touched);
					}

					if (OwnsSaveLock(touched, result.ReqId))
					{
						m_Model.SetLockAwaitIndex(touched);
					}

					m_Model.RequestIndex(touched);
					m_BackupsTab.OnFileSaved(touched);
					m_FilesTab.OnFileSaved(touched);
				}
			}

			RequestDetails();
			m_TypesTab.OnSaveFinished(result.ReqId, true);
		}
		else if (stale)
		{
			m_Model.UnlockReq(result.ReqId);
			m_Model.MarkRebase(result.FileKey);
			m_Model.RequestIndex(result.FileKey);
			//the server names the stale file in ErrorArg; a MOVE/COPY target must be refreshed too, or the retry keeps
			//checking collisions and previewing effect changes against the old target rows
			string staleKey = result.ErrorArg;
			if (staleKey != "" && staleKey != result.FileKey && m_Model.IsTypesFile(staleKey))
			{
				if (m_Model.HasPendingEdits(staleKey))
				{
					m_Model.MarkRebase(staleKey);
				}

				m_Model.RequestIndex(staleKey);
			}

			NotifyError("#VSTR_XMLE_ERR_STALE");
			m_TypesTab.OnSaveFinished(result.ReqId, false);
		}
		else
		{
			m_Model.UnlockReq(result.ReqId);
			NotifyError(FormatKey(result.ErrorKey, result.ErrorArg));
			m_TypesTab.OnSaveFinished(result.ReqId, false);
		}

		ForgetSaveLockOwners(result.ReqId);
		m_BackupsTab.OnFileSaved(result.FileKey);
		m_FilesTab.OnFileSaved(result.FileKey);
		SetStatus("#VSTR_XMLE_STATUS_READY");
		RebuildRail();
		UpdateAddButton();
		m_TypesTab.OnRowsChanged();
		CheckUnregisteredSelection();
	}

	protected void HandleProgress(VPPXEProgress progress)
	{
		m_LastReqActivity = GetGame().GetTime();
		bool ours = m_Model.OwnsIndexReq(progress.ReqId) || m_Model.IsSaveReq(progress.ReqId);
		if (progress.ReqId != 0 && progress.ReqId == m_DetailsReqId)
		{
			ours = true;
		}

		string stageText = Tr(VPPXEText.StageKey(progress.Stage));
		string percentText = progress.Percent.ToString() + "%";
		if (progress.Stage == VPPXEStage.TYPES || progress.Stage == VPPXEStage.REGISTRY)
		{
			m_Model.TouchAllIndexReqs();
		}

		if (ours)
		{
			m_Model.TouchIndexReq(progress.ReqId);
			m_Model.TouchLocks(progress.ReqId);
			SetStatus(string.Format(Tr("#VSTR_XMLE_PROGRESS_FMT"), stageText, percentText));
			return;
		}

		if (m_MapTab.HandleProgress(progress))
		{
			return;
		}

		if (m_BackupsTab.HandleProgress(progress))
		{
			return;
		}

		if (m_FilesTab.HandleProgress(progress))
		{
			return;
		}

		SetStatus(string.Format(Tr("#VSTR_XMLE_PROGRESS_FMT"), stageText, percentText));
	}

	protected void HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (m_Model.OwnsIndexReq(reqId))
		{
			if (!ok)
			{
				m_Model.DropIndexReq(reqId);
				NotifyError(FormatKey(key, arg));
				SetStatus("#VSTR_XMLE_STATUS_READY");
			}

			return;
		}

		if (reqId != 0 && reqId == m_DetailsReqId)
		{
			if (!ok)
			{
				NotifyError(FormatKey(key, arg));
			}

			return;
		}

		if (m_Model.IsSaveReq(reqId))
		{
			if (!ok)
			{
				//upload failures (ERR_INTERNAL, ERR_PERMISSION, ERR_VALIDATION) release the save lock and keep the edits
				m_Model.ForgetSave(reqId);
				m_Model.UnlockReq(reqId);
				ForgetSaveLockOwners(reqId);
				CancelUploads(reqId);
				NotifyError(FormatKey(key, arg));
				SetStatus("#VSTR_XMLE_STATUS_READY");
				m_TypesTab.OnSaveFinished(reqId, false);
				UpdateAddButton();
			}

			return;
		}

		if (m_MapTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_BackupsTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_FilesTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_CreateTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_MessagesTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_EventsTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (m_SpawnablesTab.HandleActionResult(reqId, ok, key, arg))
		{
			return;
		}

		if (!ok)
		{
			NotifyError(FormatKey(key, arg));
		}
	}
};
