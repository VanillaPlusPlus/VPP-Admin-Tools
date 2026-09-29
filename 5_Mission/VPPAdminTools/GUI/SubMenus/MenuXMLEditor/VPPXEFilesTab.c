// VPP XML Editor (WP6): FILES tab. The registry grouped by kind with a state per file and the EOL legend on
// the left; the issues list (server-sorted, filtered client-side by the selected file and a severity
// dropdown) on the right. Double-clicking a types file scopes the rail to it; double-clicking a types issue
// opens that type.

class VPPXEFilesTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int TOOLBAR_H = 30;
	const static int LINE_H = 15;
	const static int HINT_H = 18;
	const static int AUTO_RETRY_MAX = 2;
	const static int REMOVE_H = 30;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_FilesLeft;
	protected Widget m_FilesHeader;
	protected TextListboxWidget m_FilesList;
	protected TextWidget m_TxtFilesLegend;
	protected ButtonWidget m_BtnRemove;
	protected TextWidget m_TxtRemove;
	protected int m_RemoveReqId;
	protected string m_RemoveKey;
	protected Widget m_FilesRight;
	protected Widget m_IssuesToolbar;
	protected TextWidget m_TxtIssuesTitle;
	protected TextWidget m_TxtIssuesCount;
	protected ButtonWidget m_BtnIssuesRefresh;
	protected Widget m_SevHost;
	protected ref VPPDropDownMenu m_SevDD;
	protected ref array<int> m_SevLevels;
	protected TextListboxWidget m_IssuesList;
	protected TextWidget m_TxtIssuesHint;

	protected bool m_Shown;
	protected bool m_Requested;
	protected bool m_Dirty;
	protected bool m_RefreshQueued;
	protected int m_LastActivity;
	protected string m_SessionSig;

	protected ref array<string> m_RowKeys;
	protected string m_FileFilter;
	protected int m_FilesSel;

	protected int m_ReqId;
	protected bool m_Pending;
	protected bool m_TimedOut;
	protected int m_Retries;
	protected int m_Got;
	protected ref array<ref VPPXEIssue> m_Incoming;
	protected ref array<ref VPPXEIssue> m_Issues;
	protected bool m_HasIssues;
	protected int m_Errors;
	protected int m_Warnings;
	protected int m_Infos;
	protected int m_MinSeverity;
	protected ref array<int> m_IssueRows;

	protected ref array<string> m_LegendParts;
	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXEFilesTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_SevLevels = new array<int>;
		m_RowKeys = new array<string>;
		m_Incoming = new array<ref VPPXEIssue>;
		m_Issues = new array<ref VPPXEIssue>;
		m_IssueRows = new array<int>;
		m_LegendParts = new array<string>;
		m_FilesSel = -1;
		m_Unit = 1.0;

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorFilesTab, host);
		m_Root.SetHandler(this);
		BindWidgets();

		m_SevDD = new VPPDropDownMenu(m_SevHost, Tr("#VSTR_XMLE_ISS_SEV_ALL"));
		m_SevDD.m_OnSelectItem.Insert(OnSeveritySelected);
		m_SevDD.AddElement(Tr("#VSTR_XMLE_ISS_SEV_ALL"));
		m_SevLevels.Insert(VPPXESeverity.INFO);
		m_SevDD.AddElement(Tr("#VSTR_XMLE_ISS_SEV_ERR"));
		m_SevLevels.Insert(VPPXESeverity.ERROR);
		m_SevDD.AddElement(Tr("#VSTR_XMLE_ISS_SEV_WARN"));
		m_SevLevels.Insert(VPPXESeverity.WARNING);
		m_SevDD.SetIndex(0);
		m_MinSeverity = VPPXESeverity.INFO;

		m_TxtIssuesCount.SetText("");
		m_TxtIssuesHint.SetText(Tr("#VSTR_XMLE_ISS_HINT"));

		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnIssuesChunk", this, SingleplayerExecutionType.Client);
	}

	protected void BindWidgets()
	{
		m_FilesLeft = m_Root.FindAnyWidget("FilesLeft");
		m_FilesHeader = m_Root.FindAnyWidget("FilesHeader");
		m_FilesList = TextListboxWidget.Cast(m_Root.FindAnyWidget("FilesList"));
		m_TxtFilesLegend = TextWidget.Cast(m_Root.FindAnyWidget("TxtFilesLegend"));
		m_BtnRemove = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnFilesRemove"));
		m_TxtRemove = TextWidget.Cast(m_Root.FindAnyWidget("TxtFilesRemove"));
		m_FilesRight = m_Root.FindAnyWidget("FilesRight");
		m_IssuesToolbar = m_Root.FindAnyWidget("IssuesToolbar");
		m_TxtIssuesTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtIssuesTitle"));
		m_TxtIssuesCount = TextWidget.Cast(m_Root.FindAnyWidget("TxtIssuesCount"));
		m_BtnIssuesRefresh = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnIssuesRefresh"));
		m_SevHost = m_Root.FindAnyWidget("IssuesSeverityHost");
		m_IssuesList = TextListboxWidget.Cast(m_Root.FindAnyWidget("IssuesList"));
		m_TxtIssuesHint = TextWidget.Cast(m_Root.FindAnyWidget("TxtIssuesHint"));
	}

	// ---------------------------------------------------------------- tab API (INTERFACES section 6)

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			return;
		}

		RebuildFiles();
		UpdateLegend();
		OnResize();
		// OnUpdate does not run while hidden: a request that expired meanwhile is dropped silently and asked again
		bool expired = m_Pending && NowMs() - m_LastActivity > VPPXEConst.INFLIGHT_TIMEOUT_MS;
		if (expired)
		{
			m_Pending = false;
		}

		if (!m_Requested || m_Dirty || expired || m_TimedOut)
		{
			RequestIssues();
		}
	}

	// True while the issues request waits for its reply (bounded by the in-flight timeout, since OnUpdate does
	// not expire it while the tab is hidden). The window holds XE_CloseSession back while this is true.
	bool HasInFlight()
	{
		return m_Pending && NowMs() - m_LastActivity <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void OnResize()
	{
		if (!m_Root || !m_FilesLeft || !m_FilesRight)
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
		m_FilesLeft.GetScreenSize(leftW, leftH);
		m_LeftW = leftW / m_Unit;
		m_LeftH = leftH / m_Unit;
		float rightW;
		float rightH;
		m_FilesRight.GetScreenSize(rightW, rightH);
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
		if (m_RefreshQueued)
		{
			m_RefreshQueued = false;
			RequestIssues();
		}

		PollFileSelection();
		// a silent request is asked again up to AUTO_RETRY_MAX times; after that it is 'timed out', which Show,
		// OnSessionChanged and a late reply to the same request all recover from
		if (m_Pending && NowMs() - m_LastActivity > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			if (m_Retries < AUTO_RETRY_MAX)
			{
				m_Retries++;
				RequestIssues();
			}
			else
			{
				m_Pending = false;
				m_TimedOut = true;
				m_Retries = 0;
				m_TxtIssuesCount.SetText(Tr("#VSTR_XMLE_ERR_NOT_READY"));
			}
		}
	}

	// Rebuilds the file list; the issues are re-requested (when shown) only when the session really changed or
	// the last request timed out.
	void OnSessionChanged()
	{
		RebuildFiles();
		UpdateLegend();
		string sig = CurrentSessionSig();
		if (!m_Requested || (sig == m_SessionSig && !m_TimedOut))
		{
			return;
		}

		if (m_Shown)
		{
			RequestIssues();
		}
		else
		{
			m_Dirty = true;
		}
	}

	void Refresh()
	{
		m_Retries = 0;
		RequestIssues();
	}

	// One save can report several files: coalesced into one issues request on the next update.
	void OnFileSaved(string fileKey)
	{
		if (m_Shown)
		{
			m_RefreshQueued = true;
		}
		else
		{
			m_Dirty = true;
		}
	}

	bool HandleProgress(VPPXEProgress p)
	{
		if (!p || p.ReqId <= 0 || !m_Pending || p.ReqId != m_ReqId)
		{
			return false;
		}

		m_LastActivity = NowMs();
		int percent = p.Percent;
		string pct = percent.ToString() + "%";
		string stage = Tr(VPPXEText.StageKey(p.Stage));
		m_TxtIssuesCount.SetText(string.Format(Tr("#VSTR_XMLE_PROGRESS_FMT"), stage, pct));
		return true;
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId > 0 && reqId == m_RemoveReqId)
		{
			m_RemoveReqId = 0;
			string resultKey = key;
			if (resultKey == "")
			{
				resultKey = "#VSTR_XMLE_ERR_INTERNAL";
			}

			string resultText = string.Format(Tr(resultKey), arg);
			if (ok)
			{
				m_Owner.Notify(resultText);
			}
			else
			{
				m_Owner.NotifyError(resultText);
			}

			UpdateRemoveButton();
			return true;
		}

		if (reqId <= 0 || reqId != m_ReqId)
		{
			return false;
		}

		if (!ok)
		{
			m_Pending = false;
			if (key == "#VSTR_XMLE_ERR_NOT_READY")
			{
				m_TimedOut = true;
			}

			m_TxtIssuesCount.SetText(string.Format(Tr(key), arg));
		}

		return true;
	}

	// ---------------------------------------------------------------- server-to-client receiver (section 4)

	void XE_OnIssuesChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEIssuesChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			if (MenuXMLEditor.NET_TRACE)
			{
				Print("[XMLEditor][Net] client: XE_OnIssuesChunk arrived but could not be read");
			}

			return;
		}

		VPPXEIssuesChunk chunk = data.param1;
		if (MenuXMLEditor.NET_TRACE && chunk)
		{
			int traceReq = chunk.ReqId;
			int traceIdx = chunk.ChunkIdx;
			int traceCount = chunk.ChunkCount;
			string traceLine = "[XMLEditor][Net] client: issues chunk req " + traceReq.ToString() + " " + traceIdx.ToString() + "/" + traceCount.ToString() + ", awaited req " + m_ReqId.ToString();
			Print(traceLine);
		}

		if (MenuXMLEditor.NET_TRACE && !chunk)
		{
			Print("[XMLEditor][Net] client: XE_OnIssuesChunk arrived with a null payload");
		}

		if (!chunk || chunk.ReqId != m_ReqId)
		{
			return;
		}

		// a late reply to the request that timed out is still accepted (it re-arms the wait)
		if (!m_Pending)
		{
			if (!m_TimedOut)
			{
				return;
			}

			m_Pending = true;
			m_TimedOut = false;
		}

		m_LastActivity = NowMs();
		m_Errors = chunk.Errors;
		m_Warnings = chunk.Warnings;
		m_Infos = chunk.Infos;
		if (chunk.Issues)
		{
			foreach (VPPXEIssue issue : chunk.Issues)
			{
				if (issue)
				{
					m_Incoming.Insert(issue);
				}
			}
		}

		m_Got++;
		if (m_Got >= chunk.ChunkCount)
		{
			FinishIssues();
		}
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_BtnIssuesRefresh)
		{
			Refresh();
			return true;
		}

		if (w == m_BtnRemove)
		{
			StartRemove();
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

		if (w == m_FilesList)
		{
			OpenSelectedFile();
			return true;
		}

		if (w == m_IssuesList)
		{
			OpenSelectedIssue();
			return true;
		}

		return false;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return false;
	}

	void OnSeveritySelected(int index)
	{
		if (index < 0 || index >= m_SevLevels.Count())
		{
			return;
		}

		m_MinSeverity = m_SevLevels[index];
		m_SevDD.SetText(SeverityEntryText(index));
		m_SevDD.SetIndex(index);
		m_SevDD.Close();
		RebuildIssues();
	}

	// ---------------------------------------------------------------- files list

	// A header row per kind (accent-orange, name + count), then one row per file with its state.
	protected void RebuildFiles()
	{
		if (!m_FilesList)
		{
			return;
		}

		m_FilesList.ClearItems();
		m_RowKeys.Clear();
		m_FilesSel = -1;
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		int maxKind = 0;
		foreach (VPPXEFileInfo scanned : session.Files)
		{
			if (scanned && scanned.Kind > maxKind)
			{
				maxKind = scanned.Kind;
			}
		}

		int keepRow = -1;
		int orange = ARGB(255, 232, 163, 61);
		int primary = ARGB(255, 255, 255, 255);
		int secondary = ARGB(255, 154, 160, 166);
		for (int kind = 0; kind <= maxKind; kind++)
		{
			int count = 0;
			foreach (VPPXEFileInfo counted : session.Files)
			{
				if (counted && counted.Kind == kind)
				{
					count++;
				}
			}

			if (count == 0)
			{
				continue;
			}

			string kindName = Tr(VPPXEText.KindKey(kind));
			int headerRow = m_FilesList.AddItem(kindName + " (" + count.ToString() + ")", null, 0);
			m_FilesList.SetItem(headerRow, "", null, 1);
			m_FilesList.SetItem(headerRow, "", null, 2);
			m_FilesList.SetItemColor(headerRow, 0, orange);
			m_RowKeys.Insert("");
			foreach (VPPXEFileInfo fileInfo : session.Files)
			{
				if (!fileInfo || fileInfo.Kind != kind)
				{
					continue;
				}

				string stateKey = StateKey(fileInfo.Flags);
				int stateColor = StateColor(fileInfo.Flags);
				int row = m_FilesList.AddItem(FileLabel(fileInfo.Key), null, 0);
				m_FilesList.SetItem(row, kindName, null, 1);
				m_FilesList.SetItem(row, Tr(stateKey), null, 2);
				m_FilesList.SetItemColor(row, 0, primary);
				m_FilesList.SetItemColor(row, 1, secondary);
				m_FilesList.SetItemColor(row, 2, stateColor);
				m_RowKeys.Insert(fileInfo.Key);
				if (m_FileFilter != "" && fileInfo.Key == m_FileFilter)
				{
					keepRow = row;
				}
			}
		}

		if (keepRow >= 0)
		{
			m_FilesList.SelectRow(keepRow);
			m_FilesSel = keepRow;
			UpdateRemoveButton();
			return;
		}

		if (m_FileFilter != "")
		{
			m_FileFilter = "";
			RebuildIssues();
		}

		UpdateRemoveButton();
	}

	// State by priority: interrupted, parse/read error, missing, changed outside, restart pending,
	// read only, OK.
	// Static: the CREATE tab uses the same states.
	static string StateKey(int flags)
	{
		if ((flags & VPPXEFileFlag.INTERRUPTED) != 0)
		{
			return "#VSTR_XMLE_STATE_INTERRUPTED";
		}

		if ((flags & VPPXEFileFlag.PARSE_ERROR) != 0 || (flags & VPPXEFileFlag.READ_ERROR) != 0)
		{
			return "#VSTR_XMLE_STATE_ERROR";
		}

		if ((flags & VPPXEFileFlag.EXISTS) == 0)
		{
			return "#VSTR_XMLE_STATE_MISSING";
		}

		if ((flags & VPPXEFileFlag.EXTERNAL_CHANGE) != 0)
		{
			return "#VSTR_XMLE_STATE_EXTERNAL";
		}

		if ((flags & VPPXEFileFlag.PENDING_RESTART) != 0)
		{
			return "#VSTR_XMLE_STATE_RESTART";
		}

		if ((flags & VPPXEFileFlag.EDITABLE) == 0)
		{
			return "#VSTR_XMLE_STATE_READONLY";
		}

		return "#VSTR_XMLE_STATE_OK";
	}

	static int StateColor(int flags)
	{
		if ((flags & VPPXEFileFlag.INTERRUPTED) != 0 || (flags & VPPXEFileFlag.PARSE_ERROR) != 0 || (flags & VPPXEFileFlag.READ_ERROR) != 0 || (flags & VPPXEFileFlag.EXISTS) == 0)
		{
			return ARGB(255, 194, 69, 69);
		}

		if ((flags & VPPXEFileFlag.EXTERNAL_CHANGE) != 0 || (flags & VPPXEFileFlag.PENDING_RESTART) != 0)
		{
			return ARGB(255, 217, 178, 61);
		}

		if ((flags & VPPXEFileFlag.EDITABLE) == 0)
		{
			return ARGB(255, 154, 160, 166);
		}

		return ARGB(255, 76, 175, 80);
	}

	// Selecting a file filters the issues; a kind header row means all files.
	protected void PollFileSelection()
	{
		if (!m_FilesList)
		{
			return;
		}

		int row = m_FilesList.GetSelectedRow();
		if (row == m_FilesSel)
		{
			return;
		}

		m_FilesSel = row;
		UpdateRemoveButton();
		string filter = "";
		if (row >= 0 && row < m_RowKeys.Count())
		{
			filter = m_RowKeys[row];
		}

		if (filter == m_FileFilter)
		{
			return;
		}

		m_FileFilter = filter;
		RebuildIssues();
	}

	// ---------------------------------------------------------------- remove a <ce> file

	protected string SelectedKey()
	{
		if (!m_FilesList)
		{
			return "";
		}

		int row = m_FilesList.GetSelectedRow();
		if (row < 0 || row >= m_RowKeys.Count())
		{
			return "";
		}

		return m_RowKeys[row];
	}

	// Messages files need EditMessages, events EditEvents, spawnables EditSpawnables, every other kind AddRemoveTypes
	// (the server checks the same).
	protected bool CanRemove(string fileKey)
	{
		if (fileKey == "" || !m_Owner || !m_Owner.GetModel() || m_RemoveReqId > 0)
		{
			return false;
		}

		VPPXEFileInfo fileInfo = m_Owner.GetModel().GetFile(fileKey);
		if (!fileInfo || (fileInfo.Flags & VPPXEFileFlag.FROM_CE) == 0)
		{
			return false;
		}

		if (fileInfo.Kind == VPPXEFileKind.MESSAGES)
		{
			return m_Owner.HasPerm(VPPXEPerm.EDIT_MESSAGES);
		}

		if (fileInfo.Kind == VPPXEFileKind.EVENTS)
		{
			return m_Owner.HasPerm(VPPXEPerm.EDIT_EVENTS);
		}

		if (fileInfo.Kind == VPPXEFileKind.SPAWNABLETYPES || fileInfo.Kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return m_Owner.HasPerm(VPPXEPerm.EDIT_SPAWNABLES);
		}

		return m_Owner.HasPerm(VPPXEPerm.ADD_REMOVE);
	}

	protected void UpdateRemoveButton()
	{
		if (!m_BtnRemove)
		{
			return;
		}

		bool allowed = CanRemove(SelectedKey());
		m_BtnRemove.Enable(allowed);
		if (m_TxtRemove)
		{
			if (allowed)
			{
				m_TxtRemove.SetColor(ARGB(255, 255, 255, 255));
			}
			else
			{
				m_TxtRemove.SetColor(ARGB(255, 92, 97, 102));
			}
		}
	}

	protected void StartRemove()
	{
		string fileKey = SelectedKey();
		if (!CanRemove(fileKey))
		{
			return;
		}

		if (m_Owner.GetModel().HasPendingEdits(fileKey))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_FILES_ERR_PENDING"), FileLabel(fileKey)));
			return;
		}

		m_RemoveKey = fileKey;
		string body = string.Format(Tr("#VSTR_XMLE_FILES_DLG_REMOVE_BODY"), fileKey);
		m_Owner.OpenConfirm("#VSTR_XMLE_FILES_DLG_REMOVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRemove", false);
	}

	void OnConfirmRemove(int result, string input)
	{
		if (result != DIAGRESULT.YES || !CanRemove(m_RemoveKey))
		{
			return;
		}

		m_RemoveReqId = m_Owner.NextReqId();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_RemoveCeFile", new Param2<int, string>(m_RemoveReqId, m_RemoveKey), true, null);
		UpdateRemoveButton();
	}

	protected void OpenSelectedFile()
	{
		int row = m_FilesList.GetSelectedRow();
		if (row < 0 || row >= m_RowKeys.Count() || !m_Owner)
		{
			return;
		}

		string fileKey = m_RowKeys[row];
		if (!IsTypesFile(fileKey))
		{
			return;
		}

		m_Owner.SetTypesScope(fileKey);
		m_Owner.ShowTab(VPPXETab.TYPES);
	}

	protected void UpdateLegend()
	{
		m_LegendParts.Clear();
		m_LegendParts.Insert(Tr("#VSTR_XMLE_FILES_LEGEND"));
		VPPXESessionInfo session = GetSession();
		if (session)
		{
			m_LegendParts.Insert(Tr(VPPXEText.EolKey(session.EolMode)));
		}

		string text = m_LegendParts[0];
		for (int i = 1; i < m_LegendParts.Count(); i++)
		{
			text = text + "\n" + m_LegendParts[i];
		}

		if (m_TxtFilesLegend)
		{
			m_TxtFilesLegend.SetText(text);
		}

		LayoutLeft();
	}

	// ---------------------------------------------------------------- issues

	protected void RequestIssues()
	{
		if (!m_Owner)
		{
			return;
		}

		m_ReqId = m_Owner.NextReqId();
		m_Pending = true;
		m_TimedOut = false;
		m_Requested = true;
		m_Dirty = false;
		m_Got = 0;
		m_Incoming = new array<ref VPPXEIssue>;
		m_SessionSig = CurrentSessionSig();
		m_LastActivity = NowMs();
		m_TxtIssuesCount.SetText(Tr("#VSTR_XMLE_STATUS_LOADING"));
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetIssues", new Param2<int, string>(m_ReqId, ""), true, null);
	}

	protected void FinishIssues()
	{
		m_Pending = false;
		m_Retries = 0;
		m_TimedOut = false;
		m_HasIssues = true;
		m_Issues = m_Incoming;
		m_Incoming = new array<ref VPPXEIssue>;
		m_TxtIssuesCount.SetText(string.Format(Tr("#VSTR_XMLE_ISS_COUNT"), m_Errors, m_Warnings, m_Infos));
		RebuildIssues();
	}

	// Rows: severity, line (- for 0), entry (the file for file-level issues), formatted message; colour by severity.
	protected void RebuildIssues()
	{
		if (!m_IssuesList)
		{
			return;
		}

		m_IssuesList.ClearItems();
		m_IssueRows.Clear();
		if (!m_HasIssues)
		{
			return;
		}

		int total = m_Issues.Count();
		for (int i = 0; i < total; i++)
		{
			VPPXEIssue issue = m_Issues[i];
			if (issue.Severity < m_MinSeverity)
			{
				continue;
			}

			if (m_FileFilter != "" && issue.FileKey != m_FileFilter)
			{
				continue;
			}

			string lineText = "-";
			int lineNo = issue.Line;
			if (lineNo > 0)
			{
				lineText = lineNo.ToString();
			}

			string entryText = issue.Entry;
			if (entryText == "")
			{
				entryText = FileLabel(issue.FileKey);
			}

			string messageKey = VPPXEText.IssueKey(issue.Code);
			string message = issue.Arg;
			if (messageKey != "")
			{
				message = string.Format(Tr(messageKey), issue.Arg);
			}

			int color = SeverityColor(issue.Severity);
			int row = m_IssuesList.AddItem(Tr(VPPXEText.SeverityKey(issue.Severity)), null, 0);
			m_IssuesList.SetItem(row, lineText, null, 1);
			m_IssuesList.SetItem(row, entryText, null, 2);
			m_IssuesList.SetItem(row, message, null, 3);
			for (int column = 0; column < 4; column++)
			{
				m_IssuesList.SetItemColor(row, column, color);
			}

			m_IssueRows.Insert(i);
		}

		if (m_IssueRows.Count() > 0)
		{
			return;
		}

		int emptyRow = m_IssuesList.AddItem("", null, 0);
		m_IssuesList.SetItem(emptyRow, "", null, 1);
		m_IssuesList.SetItem(emptyRow, "", null, 2);
		m_IssuesList.SetItem(emptyRow, Tr("#VSTR_XMLE_ISS_NONE"), null, 3);
		m_IssuesList.SetItemColor(emptyRow, 3, ARGB(255, 154, 160, 166));
		m_IssueRows.Insert(-1);
	}

	protected int SeverityColor(int severity)
	{
		if (severity == VPPXESeverity.ERROR)
		{
			return ARGB(255, 194, 69, 69);
		}

		if (severity == VPPXESeverity.WARNING)
		{
			return ARGB(255, 217, 178, 61);
		}

		return ARGB(255, 154, 160, 166);
	}

	protected string SeverityEntryText(int index)
	{
		if (index == 1)
		{
			return Tr("#VSTR_XMLE_ISS_SEV_ERR");
		}

		if (index == 2)
		{
			return Tr("#VSTR_XMLE_ISS_SEV_WARN");
		}

		return Tr("#VSTR_XMLE_ISS_SEV_ALL");
	}

	protected void OpenSelectedIssue()
	{
		int row = m_IssuesList.GetSelectedRow();
		if (row < 0 || row >= m_IssueRows.Count() || !m_Owner)
		{
			return;
		}

		int issueIdx = m_IssueRows[row];
		if (issueIdx < 0 || issueIdx >= m_Issues.Count())
		{
			return;
		}

		VPPXEIssue issue = m_Issues[issueIdx];
		if (issue.Entry == "" || !IsTypesFile(issue.FileKey))
		{
			return;
		}

		m_Owner.SelectType(issue.Entry, true);
	}

	// RegistryRev plus key, revision and flags of every file: a change means the issues may differ.
	protected string CurrentSessionSig()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return "";
		}

		int registryRev = session.RegistryRev;
		string sig = registryRev.ToString();
		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo)
			{
				continue;
			}

			int revision = fileInfo.Revision;
			int flags = fileInfo.Flags;
			sig = sig + "|" + fileInfo.Key + ":" + revision.ToString() + ":" + flags.ToString();
		}

		return sig;
	}

	// ---------------------------------------------------------------- layout

	protected void UpdateUnit()
	{
		m_Unit = 1.0;
		if (!m_FilesHeader)
		{
			return;
		}

		float headerW;
		float headerH;
		m_FilesHeader.GetScreenSize(headerW, headerH);
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

	// Left: header 28, files list, REMOVE FROM CE, legend (EOL mode) at the bottom.
	protected void LayoutLeft()
	{
		if (m_LeftW < 40 || m_LeftH < 80)
		{
			return;
		}

		float innerW = m_LeftW - 12;
		int legendLines = EstimateLines(m_LegendParts, innerW);
		float legendH = legendLines * LINE_H + 4;
		float legendY = m_LeftH - legendH - 6;
		m_TxtFilesLegend.SetPos(6, legendY);
		m_TxtFilesLegend.SetSize(innerW, legendH);
		float removeY = legendY - REMOVE_H - 6;
		if (m_BtnRemove)
		{
			m_BtnRemove.SetPos(6, removeY);
			m_BtnRemove.SetSize(innerW, REMOVE_H);
		}

		float listH = removeY - 32 - 4;
		if (listH < 30)
		{
			listH = 30;
		}

		m_FilesList.SetPos(6, 32);
		m_FilesList.SetSize(innerW, listH);
	}

	// Right: IssuesToolbar (the last child of FilesRight, placed at the top) with the severity host
	// right-aligned before the refresh button; the issues list below; the hint line at the bottom.
	protected void LayoutRight()
	{
		if (m_RightW < 40 || m_RightH < 80)
		{
			return;
		}

		float hostW = Math.Clamp(m_RightW * 0.3, 140, 200);
		float hostX = m_RightW - 6 - 26 - 6 - hostW;
		m_SevHost.SetPos(hostX, 3);
		m_SevHost.SetSize(hostW, 24);
		float titleW = 12 + EstimateTextPx(Tr("#VSTR_XMLE_ISS_TITLE"), 9);
		m_TxtIssuesTitle.SetPos(28, 0);
		m_TxtIssuesTitle.SetSize(titleW, TOOLBAR_H);
		float countX = 28 + titleW + 6;
		float countW = hostX - countX - 8;
		if (countW < 20)
		{
			countW = 20;
		}

		m_TxtIssuesCount.SetPos(countX, 0);
		m_TxtIssuesCount.SetSize(countW, TOOLBAR_H);
		float listY = TOOLBAR_H + 4;
		float listH = m_RightH - listY - HINT_H - 8;
		if (listH < 30)
		{
			listH = 30;
		}

		m_IssuesList.SetPos(6, listY);
		m_IssuesList.SetSize(m_RightW - 12, listH);
		m_TxtIssuesHint.SetPos(8, m_RightH - HINT_H - 4);
		m_TxtIssuesHint.SetSize(m_RightW - 16, HINT_H);
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

	protected string FileLabel(string fileKey)
	{
		if (m_Owner && m_Owner.GetModel())
		{
			return m_Owner.GetModel().FileLabel(fileKey);
		}

		return fileKey;
	}

	protected bool IsTypesFile(string fileKey)
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

		return fileInfo.Kind == VPPXEFileKind.TYPES;
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

	// Width estimate in layout px: pxPerChar per character plus 2 px per extra UTF-8 byte.
	protected int EstimateTextPx(string text, int pxPerChar)
	{
		int bytes = text.Length();
		int chars = text.LengthUtf8();
		return pxPerChar * chars + 2 * (bytes - chars);
	}

	protected int EstimateLines(array<string> parts, float width)
	{
		float usable = width;
		if (usable < 20)
		{
			usable = 20;
		}

		int total = 0;
		foreach (string part : parts)
		{
			int px = EstimateTextPx(part, 7);
			int lines = Math.Ceil(px / usable);
			if (lines < 1)
			{
				lines = 1;
			}

			total += lines;
		}

		return total;
	}
};
