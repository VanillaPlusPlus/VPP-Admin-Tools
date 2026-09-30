// VPP XML Editor: CREATE tab. Left: every file registered through a <ce> block of cfgeconomycore.xml (folder, file,
// kind, state); a click copies the row's folder into the form, a double-click opens a types file in the Types tab.
// Right: the new custom file form (kind, folder, file name) validated live with VPPXECreateRules, a preview of the path
// and of the cfgeconomycore.xml lines, and CREATE, which asks the server (XE_CreateTypesFile: types files need
// AddRemoveTypes, messages files EditMessages).

class VPPXECreateTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int ROW_H = 26;
	const static int INPUT_X = 118;
	const static int KIND_Y = 40;
	const static int FOLDER_Y = 74;
	const static int FILE_Y = 108;
	const static int RULES_Y = 138;
	const static int PREVIEW_Y = 164;
	const static int PREVIEW_H = 110;
	const static int STATUS_Y = 282;
	const static int BUTTONS_Y = 312;
	const static int BUTTON_H = 30;
	const static int NOTE_Y = 354;
	const static int HINT_H = 48;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_Left;
	protected Widget m_ListHeader;
	protected TextListboxWidget m_CeList;
	protected TextWidget m_TxtCeHint;
	protected Widget m_Right;
	protected EditBoxWidget m_InputFolder;
	protected EditBoxWidget m_InputFile;
	protected TextWidget m_TxtRules;
	protected Widget m_PreviewCard;
	protected TextWidget m_TxtPreview;
	protected TextWidget m_TxtStatus;
	protected ButtonWidget m_BtnCreate;
	protected TextWidget m_TxtCreate;
	protected ButtonWidget m_BtnOpen;
	protected TextWidget m_TxtOpen;
	protected TextWidget m_TxtNote;
	protected Widget m_PickHost;
	protected ButtonWidget m_BtnKindTypes;
	protected TextWidget m_TxtKindTypes;
	protected ButtonWidget m_BtnKindMessages;
	protected TextWidget m_TxtKindMessages;
	protected ButtonWidget m_BtnKindEvents;
	protected TextWidget m_TxtKindEvents;
	protected ButtonWidget m_BtnKindSpawnable;
	protected TextWidget m_TxtKindSpawnable;
	protected ButtonWidget m_BtnKindPresets;
	protected TextWidget m_TxtKindPresets;
	protected int m_Kind;
	protected ref VPPDropDownMenu m_FolderDD;
	protected ref array<string> m_FolderChoices;
	protected ref array<string> m_RowKeys;

	protected bool m_Shown;
	protected int m_ListSel;
	protected string m_LastFolderText;
	protected string m_LastFileText;
	protected string m_ValidKey;
	protected string m_ValidFolder;
	protected string m_ValidFile;
	protected int m_ReqId;
	protected bool m_Pending;
	protected int m_SentAt;
	protected string m_CreatedKey;
	protected string m_ResultText;
	protected bool m_ResultOk;

	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXECreateTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_FolderChoices = new array<string>;
		m_RowKeys = new array<string>;
		m_ListSel = -1;
		m_Unit = 1.0;
		m_CreatedKey = "";
		m_ResultText = "";
		m_Kind = VPPXEFileKind.TYPES;

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorCreateTab, host);
		m_Root.SetHandler(this);
		BindWidgets();

		m_FolderDD = new VPPDropDownMenu(m_PickHost, Tr("#VSTR_XMLE_CR_PICK_FOLDER"));
		m_FolderDD.m_OnSelectItem.Insert(OnFolderPicked);
		m_BtnOpen.Show(false);
		UpdateForm(false);
	}

	protected void BindWidgets()
	{
		m_Left = m_Root.FindAnyWidget("CreateLeft");
		m_ListHeader = m_Root.FindAnyWidget("CreateListHeader");
		m_CeList = TextListboxWidget.Cast(m_Root.FindAnyWidget("CeList"));
		m_TxtCeHint = TextWidget.Cast(m_Root.FindAnyWidget("TxtCeHint"));
		m_Right = m_Root.FindAnyWidget("CreateRight");
		m_InputFolder = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputCrFolder"));
		m_InputFile = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputCrFile"));
		m_TxtRules = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrRules"));
		m_PreviewCard = m_Root.FindAnyWidget("CrPreviewCard");
		m_TxtPreview = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrPreview"));
		m_TxtStatus = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrStatus"));
		m_BtnCreate = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrCreate"));
		m_TxtCreate = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrCreate"));
		m_BtnOpen = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrOpen"));
		m_TxtOpen = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrOpen"));
		m_TxtNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrNote"));
		m_PickHost = m_Root.FindAnyWidget("CrFolderPickHost");
		m_BtnKindTypes = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrKindTypes"));
		m_TxtKindTypes = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrKindTypes"));
		m_BtnKindMessages = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrKindMessages"));
		m_TxtKindMessages = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrKindMessages"));
		m_BtnKindEvents = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrKindEvents"));
		m_TxtKindEvents = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrKindEvents"));
		m_BtnKindSpawnable = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrKindSpawnable"));
		m_TxtKindSpawnable = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrKindSpawnable"));
		m_BtnKindPresets = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnCrKindPresets"));
		m_TxtKindPresets = TextWidget.Cast(m_Root.FindAnyWidget("TxtCrKindPresets"));
	}

	// ---------------------------------------------------------------- tab API

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			if (m_FolderDD)
			{
				m_FolderDD.Close();
			}

			return;
		}

		RebuildList();
		RebuildFolderChoices();
		OnResize();
		UpdateForm(false);
	}

	// True while a create request waits for its reply (the window holds XE_CloseSession back meanwhile).
	bool HasInFlight()
	{
		return m_Pending && NowMs() - m_SentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void Refresh()
	{
		RebuildList();
		RebuildFolderChoices();
		UpdateForm(false);
	}

	// The folder input is wider when there is no existing folder to pick, so a shown tab lays out again.
	void OnSessionChanged()
	{
		RebuildList();
		RebuildFolderChoices();
		if (m_Shown)
		{
			OnResize();
		}

		UpdateForm(false);
	}

	void OnResize()
	{
		if (!m_Root || !m_Left || !m_Right)
		{
			return;
		}

		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW < 20 || rootH < 20)
		{
			return;
		}

		m_LastRootW = rootW;
		m_LastRootH = rootH;
		UpdateUnit();
		float leftW;
		float leftH;
		m_Left.GetScreenSize(leftW, leftH);
		m_LeftW = leftW / m_Unit;
		m_LeftH = leftH / m_Unit;
		float rightW;
		float rightH;
		m_Right.GetScreenSize(rightW, rightH);
		m_RightW = rightW / m_Unit;
		m_RightH = rightH / m_Unit;
		LayoutLeft();
		LayoutRight();
	}

	void OnUpdate(float timeslice)
	{
		if (!m_Shown)
		{
			return;
		}

		CheckRootResize();
		PollInputs();
		PollListSelection();
		if (m_Pending && NowMs() - m_SentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Pending = false;
			m_ResultOk = false;
			m_ResultText = Tr("#VSTR_XMLE_ERR_NOT_READY");
			UpdateForm(false);
		}
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId <= 0 || reqId != m_ReqId)
		{
			return false;
		}

		m_Pending = false;
		string textKey = key;
		if (textKey == "")
		{
			textKey = "#VSTR_XMLE_ERR_INTERNAL";
		}

		string text = string.Format(Tr(textKey), arg);
		m_ResultOk = ok;
		m_ResultText = text;
		if (ok)
		{
			m_CreatedKey = arg;
			m_InputFile.SetText("");
			m_LastFileText = "";
			m_Owner.Notify(text);
		}
		else
		{
			m_Owner.NotifyError(text);
		}

		RebuildList();
		RebuildFolderChoices();
		UpdateForm(false);
		return true;
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_BtnCreate)
		{
			StartCreate();
			return true;
		}

		if (w == m_BtnOpen)
		{
			OpenKey(m_CreatedKey);
			return true;
		}

		if (w == m_BtnKindTypes || w == m_BtnKindMessages || w == m_BtnKindEvents || w == m_BtnKindSpawnable || w == m_BtnKindPresets)
		{
			if (w == m_BtnKindMessages)
			{
				m_Kind = VPPXEFileKind.MESSAGES;
			}
			else if (w == m_BtnKindEvents)
			{
				m_Kind = VPPXEFileKind.EVENTS;
			}
			else if (w == m_BtnKindSpawnable)
			{
				m_Kind = VPPXEFileKind.SPAWNABLETYPES;
			}
			else if (w == m_BtnKindPresets)
			{
				m_Kind = VPPXEFileKind.RANDOMPRESETS;
			}
			else
			{
				m_Kind = VPPXEFileKind.TYPES;
			}

			UpdateForm(true);
			return true;
		}

		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_CeList)
		{
			int row = m_CeList.GetSelectedRow();
			if (row >= 0 && row < m_RowKeys.Count())
			{
				OpenKey(m_RowKeys[row]);
			}

			return true;
		}

		return false;
	}

	void OnFolderPicked(int index)
	{
		m_FolderDD.Close();
		m_FolderDD.SetText(Tr("#VSTR_XMLE_CR_PICK_FOLDER"));
		if (index < 0 || index >= m_FolderChoices.Count())
		{
			return;
		}

		m_InputFolder.SetText(m_FolderChoices[index]);
		UpdateForm(true);
	}

	// ---------------------------------------------------------------- create

	protected void StartCreate()
	{
		if (m_Pending || m_ValidKey == "" || !m_Owner.HasPerm(KindPerm()))
		{
			return;
		}

		string body = string.Format(Tr("#VSTR_XMLE_CR_DLG_BODY"), m_ValidKey);
		m_Owner.OpenConfirm("#VSTR_XMLE_CR_DLG_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmCreate", false);
	}

	void OnConfirmCreate(int result, string input)
	{
		if (result != DIAGRESULT.YES || m_Pending || m_ValidKey == "")
		{
			return;
		}

		m_ReqId = m_Owner.NextReqId();
		m_Pending = true;
		m_SentAt = NowMs();
		m_ResultText = "";
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_CreateTypesFile", new Param4<int, string, string, int>(m_ReqId, m_ValidFolder, m_ValidFile, m_Kind), true, null);
		UpdateForm(false);
	}

	// Types files open in the Types tab (scoped to the file), messages and events files in their own tabs.
	protected void OpenKey(string fileKey)
	{
		if (!IsOpenableFile(fileKey))
		{
			return;
		}

		VPPXEFileInfo fileInfo = m_Owner.GetModel().GetFile(fileKey);
		if (fileInfo.Kind == VPPXEFileKind.MESSAGES)
		{
			m_Owner.OpenMessagesFile(fileKey);
			return;
		}

		if (fileInfo.Kind == VPPXEFileKind.EVENTS)
		{
			m_Owner.OpenEventsFile(fileKey);
			return;
		}

		if (fileInfo.Kind == VPPXEFileKind.SPAWNABLETYPES || fileInfo.Kind == VPPXEFileKind.RANDOMPRESETS)
		{
			m_Owner.OpenSpawnablesFile(fileKey);
			return;
		}

		m_Owner.SetTypesScope(fileKey);
		m_Owner.ShowTab(VPPXETab.TYPES);
	}

	protected int KindPerm()
	{
		if (m_Kind == VPPXEFileKind.MESSAGES)
		{
			return VPPXEPerm.EDIT_MESSAGES;
		}

		if (m_Kind == VPPXEFileKind.EVENTS)
		{
			return VPPXEPerm.EDIT_EVENTS;
		}

		if (m_Kind == VPPXEFileKind.SPAWNABLETYPES || m_Kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return VPPXEPerm.EDIT_SPAWNABLES;
		}

		return VPPXEPerm.ADD_REMOVE;
	}

	protected void UpdateKindButtons()
	{
		int active = ARGB(255, 232, 163, 61);
		int inactive = ARGB(255, 154, 160, 166);
		if (m_TxtKindTypes)
		{
			if (m_Kind == VPPXEFileKind.TYPES)
			{
				m_TxtKindTypes.SetColor(active);
			}
			else
			{
				m_TxtKindTypes.SetColor(inactive);
			}
		}

		if (m_TxtKindMessages)
		{
			if (m_Kind == VPPXEFileKind.MESSAGES)
			{
				m_TxtKindMessages.SetColor(active);
			}
			else
			{
				m_TxtKindMessages.SetColor(inactive);
			}
		}

		if (m_TxtKindEvents)
		{
			if (m_Kind == VPPXEFileKind.EVENTS)
			{
				m_TxtKindEvents.SetColor(active);
			}
			else
			{
				m_TxtKindEvents.SetColor(inactive);
			}
		}

		if (m_TxtKindSpawnable)
		{
			if (m_Kind == VPPXEFileKind.SPAWNABLETYPES)
			{
				m_TxtKindSpawnable.SetColor(active);
			}
			else
			{
				m_TxtKindSpawnable.SetColor(inactive);
			}
		}

		if (m_TxtKindPresets)
		{
			if (m_Kind == VPPXEFileKind.RANDOMPRESETS)
			{
				m_TxtKindPresets.SetColor(active);
			}
			else
			{
				m_TxtKindPresets.SetColor(inactive);
			}
		}
	}

	// ---------------------------------------------------------------- form

	protected void PollInputs()
	{
		if (!m_InputFolder || !m_InputFile)
		{
			return;
		}

		string folderText = m_InputFolder.GetText();
		string fileText = m_InputFile.GetText();
		if (folderText != m_LastFolderText || fileText != m_LastFileText)
		{
			UpdateForm(true);
		}
	}

	// Validates the inputs, fills the preview and the status line and enables CREATE. inputsChanged clears the
	// message of the last create request.
	protected void UpdateForm(bool inputsChanged)
	{
		if (!m_InputFolder || !m_InputFile)
		{
			return;
		}

		string folderText = m_InputFolder.GetText();
		string fileText = m_InputFile.GetText();
		m_LastFolderText = folderText;
		m_LastFileText = fileText;
		if (inputsChanged)
		{
			m_ResultText = "";
		}

		string folder = VPPXECreateRules.NormalizeFolder(folderText);
		string fileName = VPPXECreateRules.NormalizeFileName(fileText);
		m_ValidKey = "";
		m_ValidFolder = "";
		m_ValidFile = "";
		string statusText = "";
		int statusColor = ColorMuted();
		if (folder == "" && fileName == "")
		{
			statusText = Tr("#VSTR_XMLE_CR_EMPTY");
		}
		else
		{
			string problem = VPPXECreateRules.CheckFolder(folder);
			if (problem == "")
			{
				problem = VPPXECreateRules.CheckFileName(fileName);
			}

			if (problem != "")
			{
				statusText = Tr(problem);
				statusColor = ColorRed();
			}
			else
			{
				string key = VPPXECreateRules.MakeKey(folder, fileName);
				if (IsRegisteredKey(key))
				{
					statusText = string.Format(Tr("#VSTR_XMLE_CR_ERR_EXISTS"), key);
					statusColor = ColorRed();
				}
				else
				{
					m_ValidKey = key;
					m_ValidFolder = folder;
					m_ValidFile = fileName;
					statusText = string.Format(Tr("#VSTR_XMLE_CR_READY"), key);
					statusColor = ColorGreen();
				}
			}
		}

		bool allowed = false;
		if (m_Owner && m_Owner.HasPerm(KindPerm()))
		{
			allowed = true;
		}

		if (!allowed)
		{
			if (m_Kind == VPPXEFileKind.MESSAGES)
			{
				statusText = Tr("#VSTR_XMLE_CR_NO_PERMISSION_MSG");
			}
			else if (m_Kind == VPPXEFileKind.EVENTS)
			{
				statusText = Tr("#VSTR_XMLE_CR_NO_PERMISSION_EVT");
			}
			else if (m_Kind == VPPXEFileKind.SPAWNABLETYPES || m_Kind == VPPXEFileKind.RANDOMPRESETS)
			{
				statusText = Tr("#VSTR_XMLE_CR_NO_PERMISSION_SPB");
			}
			else
			{
				statusText = Tr("#VSTR_XMLE_CR_NO_PERMISSION");
			}

			statusColor = ColorMuted();
		}

		if (m_Pending)
		{
			statusText = Tr("#VSTR_XMLE_STATUS_SAVING");
			statusColor = ColorMuted();
		}
		else if (m_ResultText != "")
		{
			statusText = m_ResultText;
			if (m_ResultOk)
			{
				statusColor = ColorGreen();
			}
			else
			{
				statusColor = ColorRed();
			}
		}

		m_TxtStatus.SetText(statusText);
		m_TxtStatus.SetColor(statusColor);
		m_TxtPreview.SetText(BuildPreview(folder, fileName));

		bool canCreate = allowed && !m_Pending && m_ValidKey != "";
		m_BtnCreate.Enable(canCreate);
		if (canCreate)
		{
			m_TxtCreate.SetColor(ARGB(255, 255, 255, 255));
		}
		else
		{
			m_TxtCreate.SetColor(ARGB(255, 92, 97, 102));
		}

		bool canOpen = m_CreatedKey != "" && IsOpenableFile(m_CreatedKey);
		m_BtnOpen.Show(canOpen);
		UpdateKindButtons();
	}

	// The file path, then the lines added to cfgeconomycore.xml (inside the existing block of the folder, or a new one).
	protected string BuildPreview(string folder, string fileName)
	{
		string shownFolder = folder;
		if (shownFolder == "")
		{
			shownFolder = "...";
		}

		string shownFile = fileName;
		if (shownFile == "")
		{
			shownFile = "....xml";
		}

		string text = string.Format(Tr("#VSTR_XMLE_CR_PREVIEW_FILE"), "$mission/" + shownFolder + "/" + shownFile);
		string fileTag = VPPXECreateRules.FileTag(shownFile, m_Kind);
		string ceTag = VPPXECreateRules.CeOpenTag(shownFolder);
		if (folder != "" && IsRegisteredFolder(folder))
		{
			text = text + "\n\n" + string.Format(Tr("#VSTR_XMLE_CR_PREVIEW_EXISTING"), ceTag);
			text = text + "\n    " + fileTag;
			return text;
		}

		text = text + "\n\n" + Tr("#VSTR_XMLE_CR_PREVIEW_NEW");
		text = text + "\n" + ceTag;
		text = text + "\n    " + fileTag;
		text = text + "\n</ce>";
		return text;
	}

	// ---------------------------------------------------------------- list and folder choices

	// One row per file registered through <ce>: folder, file (types count for TYPES), kind, state.
	protected void RebuildList()
	{
		if (!m_CeList)
		{
			return;
		}

		m_CeList.ClearItems();
		m_RowKeys.Clear();
		m_ListSel = -1;
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		int primary = ARGB(255, 255, 255, 255);
		int secondary = ARGB(255, 154, 160, 166);
		int keepRow = -1;
		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || (fileInfo.Flags & VPPXEFileFlag.FROM_CE) == 0)
			{
				continue;
			}

			string fileLabel = VPPXECreateRules.FileOfKey(fileInfo.Key);
			int fileColor = secondary;
			if (fileInfo.Kind == VPPXEFileKind.TYPES)
			{
				int entries = fileInfo.EntryCount;
				fileLabel = fileLabel + " (" + entries.ToString() + ")";
				fileColor = primary;
			}

			int row = m_CeList.AddItem(VPPXECreateRules.FolderOfKey(fileInfo.Key), null, 0);
			m_CeList.SetItem(row, fileLabel, null, 1);
			m_CeList.SetItem(row, Tr(VPPXEText.KindKey(fileInfo.Kind)), null, 2);
			m_CeList.SetItem(row, Tr(VPPXEFilesTab.StateKey(fileInfo.Flags)), null, 3);
			m_CeList.SetItemColor(row, 0, secondary);
			m_CeList.SetItemColor(row, 1, fileColor);
			m_CeList.SetItemColor(row, 2, secondary);
			m_CeList.SetItemColor(row, 3, VPPXEFilesTab.StateColor(fileInfo.Flags));
			m_RowKeys.Insert(fileInfo.Key);
			if (m_CreatedKey != "" && fileInfo.Key == m_CreatedKey)
			{
				keepRow = row;
			}
		}

		if (m_RowKeys.Count() == 0)
		{
			int emptyRow = m_CeList.AddItem("", null, 0);
			m_CeList.SetItem(emptyRow, Tr("#VSTR_XMLE_CR_LIST_EMPTY"), null, 1);
			m_CeList.SetItem(emptyRow, "", null, 2);
			m_CeList.SetItem(emptyRow, "", null, 3);
			m_CeList.SetItemColor(emptyRow, 1, secondary);
			m_RowKeys.Insert("");
		}

		if (keepRow >= 0)
		{
			m_CeList.SelectRow(keepRow);
			m_ListSel = keepRow;
		}
	}

	// Distinct <ce> folders (any kind), for the folder dropdown.
	protected void RebuildFolderChoices()
	{
		if (!m_FolderDD)
		{
			return;
		}

		m_FolderChoices.Clear();
		VPPXESessionInfo session = GetSession();
		if (session && session.Files)
		{
			foreach (VPPXEFileInfo fileInfo : session.Files)
			{
				if (!fileInfo || (fileInfo.Flags & VPPXEFileFlag.FROM_CE) == 0)
				{
					continue;
				}

				string folder = VPPXECreateRules.FolderOfKey(fileInfo.Key);
				if (folder != "" && VPPXmlText.FindNoCase(m_FolderChoices, folder) < 0)
				{
					m_FolderChoices.Insert(folder);
				}
			}
		}

		m_FolderDD.RemoveAllElements();
		foreach (string choice : m_FolderChoices)
		{
			m_FolderDD.AddElement(choice);
		}

		m_FolderDD.SetText(Tr("#VSTR_XMLE_CR_PICK_FOLDER"));
		m_PickHost.Show(m_FolderChoices.Count() > 0);
	}

	// A click on a row copies its folder into the form.
	protected void PollListSelection()
	{
		if (!m_CeList)
		{
			return;
		}

		int row = m_CeList.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		m_ListSel = row;
		if (row < 0 || row >= m_RowKeys.Count() || m_RowKeys[row] == "")
		{
			return;
		}

		string folder = VPPXECreateRules.FolderOfKey(m_RowKeys[row]);
		if (folder != "" && folder != m_InputFolder.GetText())
		{
			m_InputFolder.SetText(folder);
			UpdateForm(true);
		}
	}

	// ---------------------------------------------------------------- layout

	protected void UpdateUnit()
	{
		m_Unit = 1.0;
		if (!m_ListHeader)
		{
			return;
		}

		float headerW;
		float headerH;
		m_ListHeader.GetScreenSize(headerW, headerH);
		if (headerH > 1)
		{
			m_Unit = headerH / HEADER_H;
		}
	}

	protected void CheckRootResize()
	{
		if (!m_Root)
		{
			return;
		}

		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW != m_LastRootW || rootH != m_LastRootH)
		{
			OnResize();
		}
	}

	// Left: header 28, the list, the hint at the bottom.
	protected void LayoutLeft()
	{
		if (m_LeftW < 40 || m_LeftH < 80)
		{
			return;
		}

		float innerW = m_LeftW - 12;
		float hintY = m_LeftH - HINT_H - 6;
		m_TxtCeHint.SetPos(6, hintY);
		m_TxtCeHint.SetSize(innerW, HINT_H);
		float listH = hintY - 32 - 4;
		if (listH < 30)
		{
			listH = 30;
		}

		m_CeList.SetPos(6, 32);
		m_CeList.SetSize(innerW, listH);
	}

	// Right: folder row (input + existing-folder dropdown), file row, rules line, preview card, status, buttons, note.
	protected void LayoutRight()
	{
		if (m_RightW < 120 || m_RightH < 120)
		{
			return;
		}

		float pickW = Math.Clamp(m_RightW * 0.32, 130, 200);
		float pickX = m_RightW - 10 - pickW;
		m_PickHost.SetPos(pickX, FOLDER_Y + 1);
		m_PickHost.SetSize(pickW, 24);
		float folderW = pickX - 6 - INPUT_X;
		if (m_FolderChoices.Count() == 0)
		{
			folderW = m_RightW - 10 - INPUT_X;
		}

		if (folderW < 60)
		{
			folderW = 60;
		}

		float kindW = Math.Clamp((m_RightW - INPUT_X - 34) / 5, 70, 150);
		m_BtnKindTypes.SetPos(INPUT_X, KIND_Y);
		m_BtnKindTypes.SetSize(kindW, ROW_H);
		m_BtnKindMessages.SetPos(INPUT_X + kindW + 6, KIND_Y);
		m_BtnKindMessages.SetSize(kindW, ROW_H);
		m_BtnKindEvents.SetPos(INPUT_X + 2 * (kindW + 6), KIND_Y);
		m_BtnKindEvents.SetSize(kindW, ROW_H);
		m_BtnKindSpawnable.SetPos(INPUT_X + 3 * (kindW + 6), KIND_Y);
		m_BtnKindSpawnable.SetSize(kindW, ROW_H);
		m_BtnKindPresets.SetPos(INPUT_X + 4 * (kindW + 6), KIND_Y);
		m_BtnKindPresets.SetSize(kindW, ROW_H);
		m_InputFolder.SetPos(INPUT_X, FOLDER_Y);
		m_InputFolder.SetSize(folderW, ROW_H);
		float fileW = m_RightW - 10 - INPUT_X;
		if (fileW < 60)
		{
			fileW = 60;
		}

		m_InputFile.SetPos(INPUT_X, FILE_Y);
		m_InputFile.SetSize(fileW, ROW_H);
		m_TxtRules.SetPos(INPUT_X, RULES_Y);
		m_TxtRules.SetSize(fileW, 18);
		float cardW = m_RightW - 20;
		m_PreviewCard.SetPos(10, PREVIEW_Y);
		m_PreviewCard.SetSize(cardW, PREVIEW_H);
		m_TxtPreview.SetPos(8, 6);
		m_TxtPreview.SetSize(cardW - 16, PREVIEW_H - 12);
		m_TxtStatus.SetPos(10, STATUS_Y);
		m_TxtStatus.SetSize(cardW, 22);
		float createW = ButtonWidth("#VSTR_XMLE_CR_BTN_CREATE");
		float openW = ButtonWidth("#VSTR_XMLE_CR_BTN_OPEN");
		m_BtnCreate.SetPos(10, BUTTONS_Y);
		m_BtnCreate.SetSize(createW, BUTTON_H);
		m_BtnOpen.SetPos(10 + createW + 8, BUTTONS_Y);
		m_BtnOpen.SetSize(openW, BUTTON_H);
		float noteH = m_RightH - NOTE_Y - 8;
		if (noteH < 30)
		{
			noteH = 30;
		}

		m_TxtNote.SetPos(10, NOTE_Y);
		m_TxtNote.SetSize(cardW, noteH);
	}

	protected float ButtonWidth(string key)
	{
		string label = Tr(key);
		return Math.Clamp(40 + 8 * label.LengthUtf8(), 140, 260);
	}

	// ---------------------------------------------------------------- helpers

	protected VPPXESessionInfo GetSession()
	{
		if (!m_Owner || !m_Owner.GetModel())
		{
			return null;
		}

		return m_Owner.GetModel().GetSession();
	}

	protected bool IsRegisteredKey(string key)
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return false;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (fileInfo && VPPXmlText.EqualsNoCase(fileInfo.Key, key))
			{
				return true;
			}
		}

		return false;
	}

	protected bool IsRegisteredFolder(string folder)
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return false;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || (fileInfo.Flags & VPPXEFileFlag.FROM_CE) == 0)
			{
				continue;
			}

			if (VPPXmlText.EqualsNoCase(VPPXECreateRules.FolderOfKey(fileInfo.Key), folder))
			{
				return true;
			}
		}

		return false;
	}

	protected bool IsOpenableFile(string fileKey)
	{
		if (fileKey == "" || !m_Owner || !m_Owner.GetModel())
		{
			return false;
		}

		VPPXEFileInfo fileInfo = m_Owner.GetModel().GetFile(fileKey);
		if (!fileInfo)
		{
			return false;
		}

		bool spawnablesKind = fileInfo.Kind == VPPXEFileKind.SPAWNABLETYPES || fileInfo.Kind == VPPXEFileKind.RANDOMPRESETS;
		if (fileInfo.Kind != VPPXEFileKind.TYPES && fileInfo.Kind != VPPXEFileKind.MESSAGES && fileInfo.Kind != VPPXEFileKind.EVENTS && !spawnablesKind)
		{
			return false;
		}

		return (fileInfo.Flags & VPPXEFileFlag.EXISTS) != 0;
	}

	protected int ColorGreen()
	{
		return ARGB(255, 76, 175, 80);
	}

	protected int ColorRed()
	{
		return ARGB(255, 194, 69, 69);
	}

	protected int ColorMuted()
	{
		return ARGB(255, 154, 160, 166);
	}

	protected string Tr(string key)
	{
		if (m_Owner)
		{
			return m_Owner.Tr(key);
		}

		return Widget.TranslateString(key);
	}

	protected int NowMs()
	{
		return GetGame().GetTime();
	}
};
