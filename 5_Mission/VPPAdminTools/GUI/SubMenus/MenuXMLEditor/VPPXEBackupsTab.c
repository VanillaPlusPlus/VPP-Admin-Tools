// VPP XML Editor (WP6): BACKUPS tab. Lists the server backups (When | File | Admin | Summary) with a file
// filter, shows the selected backup's diff ('vs current file' or 'changes after it') and runs snapshot,
// pin/unpin, restore and delete through confirmations. Unverified raw copies can be neither diffed nor restored.

class VPPXEBackupsTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int ACTION_H = 26;
	const static int BTN_GAP = 4;
	const static int LINE_H = 15;
	// dropdown_prefab.layout lays entries out in a GridSpacer fixed at Rows 50 (including 'All files')
	const static int FILE_DD_MAX_ROWS = 50;
	const static int AUTO_RETRY_MAX = 2;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_BackupsLeft;
	protected Widget m_BackupsHeader;
	protected TextListboxWidget m_BackupsList;
	protected Widget m_BackupsActions;
	protected ButtonWidget m_BtnSnapshot;
	protected ImageWidget m_ImgSnapshot;
	protected TextWidget m_TxtSnapshot;
	protected ButtonWidget m_BtnPin;
	protected ImageWidget m_ImgPin;
	protected TextWidget m_TxtPin;
	protected ButtonWidget m_BtnRestore;
	protected ImageWidget m_ImgRestore;
	protected TextWidget m_TxtRestore;
	protected ButtonWidget m_BtnDelete;
	protected ImageWidget m_ImgDelete;
	protected TextWidget m_TxtDelete;
	protected TextWidget m_TxtRetention;
	protected string m_PinKey;
	protected Widget m_FileHost;
	protected ref VPPDropDownMenu m_FileDD;
	protected ref array<string> m_FileKeys;
	protected string m_FileKeysSig;

	protected Widget m_BackupsRight;
	protected TextWidget m_TxtDiffHeader;
	protected ButtonWidget m_BtnVsCurrent;
	protected TextWidget m_TxtVsCurrent;
	protected ButtonWidget m_BtnVsNext;
	protected TextWidget m_TxtVsNext;
	protected TextWidget m_TxtDiffSummary;
	protected TextWidget m_TxtBkMeta;
	protected TextListboxWidget m_DiffList;
	protected TextWidget m_TxtDiffEmpty;

	protected bool m_Shown;
	protected bool m_Requested;
	protected bool m_ListDirty;
	protected bool m_RefreshQueued;
	protected bool m_RediffAfterList;
	protected string m_Filter;
	protected int m_LastActivity;

	protected int m_ListReqId;
	protected bool m_ListPending;
	protected bool m_ListTimedOut;
	protected int m_ListRetries;
	protected int m_ListGot;
	protected ref array<ref VPPXEBackupEntry> m_Incoming;
	protected ref array<ref VPPXEBackupEntry> m_Entries;
	protected bool m_HasList;
	protected int m_SelRow;
	protected string m_SelId;

	protected int m_DiffReqId;
	protected bool m_DiffPending;
	protected int m_DiffGot;
	protected int m_DiffMode;
	protected int m_DiffAdded;
	protected int m_DiffRemoved;
	protected int m_DiffChanged;
	protected bool m_DiffTruncated;
	protected bool m_DiffSemantic;
	protected ref array<ref VPPXEDiffRow> m_DiffRows;

	protected ref array<int> m_ActionReqIds;
	protected int m_RestoreReqId;
	protected string m_RestoreFileKey;
	protected string m_PendingSnapshotKey;
	protected string m_PendingRestoreId;
	protected string m_PendingRestoreKey;
	protected string m_PendingDeleteId;

	protected ref array<string> m_MetaParts;
	protected ref array<string> m_RetentionParts;
	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXEBackupsTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_FileKeys = new array<string>;
		m_Incoming = new array<ref VPPXEBackupEntry>;
		m_Entries = new array<ref VPPXEBackupEntry>;
		m_DiffRows = new array<ref VPPXEDiffRow>;
		m_ActionReqIds = new array<int>;
		m_MetaParts = new array<string>;
		m_RetentionParts = new array<string>;
		m_DiffMode = VPPXEDiffMode.VS_NEXT;
		m_SelRow = -1;
		m_Unit = 1.0;
		m_PinKey = "#VSTR_XMLE_BK_PIN";

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorBackupsTab, host);
		m_Root.SetHandler(this);
		BindWidgets();

		m_FileDD = new VPPDropDownMenu(m_FileHost, Tr("#VSTR_XMLE_BK_ALLFILES"));
		m_FileDD.m_OnSelectItem.Insert(OnFileSelected);
		m_FileKeys.Insert("");
		m_FileDD.AddElement(Tr("#VSTR_XMLE_BK_ALLFILES"));
		m_FileDD.SetIndex(0);

		ShowEmpty(Tr("#VSTR_XMLE_BK_SELECT"));
		m_TxtDiffSummary.SetText("");
		m_TxtBkMeta.SetText("");
		UpdateModeButtons();
		UpdateButtons();

		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnBackupList", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnBackupDiff", this, SingleplayerExecutionType.Client);
	}

	protected void BindWidgets()
	{
		m_BackupsLeft = m_Root.FindAnyWidget("BackupsLeft");
		m_BackupsHeader = m_Root.FindAnyWidget("BackupsHeader");
		m_BackupsList = TextListboxWidget.Cast(m_Root.FindAnyWidget("BackupsList"));
		m_BackupsActions = m_Root.FindAnyWidget("BackupsActions");
		m_BtnSnapshot = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBkSnapshot"));
		m_ImgSnapshot = ImageWidget.Cast(m_Root.FindAnyWidget("ImgBkSnapshot"));
		m_TxtSnapshot = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkSnapshot"));
		m_BtnPin = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBkPin"));
		m_ImgPin = ImageWidget.Cast(m_Root.FindAnyWidget("ImgBkPin"));
		m_TxtPin = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkPin"));
		m_BtnRestore = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBkRestore"));
		m_ImgRestore = ImageWidget.Cast(m_Root.FindAnyWidget("ImgBkRestore"));
		m_TxtRestore = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkRestore"));
		m_BtnDelete = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnBkDelete"));
		m_ImgDelete = ImageWidget.Cast(m_Root.FindAnyWidget("ImgBkDelete"));
		m_TxtDelete = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkDelete"));
		m_TxtRetention = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkRetention"));
		m_FileHost = m_Root.FindAnyWidget("BackupsFileHost");

		m_BackupsRight = m_Root.FindAnyWidget("BackupsRight");
		m_TxtDiffHeader = TextWidget.Cast(m_Root.FindAnyWidget("TxtDiffHeader"));
		m_BtnVsCurrent = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDiffVsCurrent"));
		m_TxtVsCurrent = TextWidget.Cast(m_Root.FindAnyWidget("TxtDiffVsCurrent"));
		m_BtnVsNext = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDiffVsNext"));
		m_TxtVsNext = TextWidget.Cast(m_Root.FindAnyWidget("TxtDiffVsNext"));
		m_TxtDiffSummary = TextWidget.Cast(m_Root.FindAnyWidget("TxtDiffSummary"));
		m_TxtBkMeta = TextWidget.Cast(m_Root.FindAnyWidget("TxtBkMeta"));
		m_DiffList = TextListboxWidget.Cast(m_Root.FindAnyWidget("DiffList"));
		m_TxtDiffEmpty = TextWidget.Cast(m_Root.FindAnyWidget("TxtDiffEmpty"));
	}

	// ---------------------------------------------------------------- tab API (INTERFACES section 6)

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			return;
		}

		UpdateRetention();
		UpdateFileDropdown();
		UpdateModeButtons();
		UpdateButtons();
		OnResize();
		// OnUpdate does not run while hidden: an expired list or diff request is dropped silently and the list is
		// asked again (a dropped diff is re-requested once the list arrives)
		bool expired = (m_ListPending || m_DiffPending) && NowMs() - m_LastActivity > VPPXEConst.INFLIGHT_TIMEOUT_MS;
		if (expired)
		{
			if (m_DiffPending)
			{
				m_RediffAfterList = true;
			}

			m_ListPending = false;
			m_DiffPending = false;
		}

		if (!m_Requested || m_ListDirty || expired || m_ListTimedOut)
		{
			RequestList();
		}
	}

	// True while a list, diff or action (restore, pin, delete, snapshot) request waits for its reply (bounded by
	// the in-flight timeout, since OnUpdate does not expire it while the tab is hidden). The window holds
	// XE_CloseSession back while this is true. Action ids are kept after the window so a late result still routes.
	bool HasInFlight()
	{
		bool pending = m_ListPending || m_DiffPending || m_RestoreReqId > 0 || m_ActionReqIds.Count() > 0;
		return pending && NowMs() - m_LastActivity <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void OnResize()
	{
		if (!m_Root || !m_BackupsLeft || !m_BackupsRight)
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
		m_BackupsLeft.GetScreenSize(leftW, leftH);
		m_LeftW = leftW / m_Unit;
		m_LeftH = leftH / m_Unit;
		float rightW;
		float rightH;
		m_BackupsRight.GetScreenSize(rightW, rightH);
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
			RefreshAfterAction();
		}

		PollSelection();
		CheckTimeouts();
	}

	// A list request that timed out is asked again (now when shown, otherwise on the next Show).
	void OnSessionChanged()
	{
		UpdateRetention();
		UpdateFileDropdown();
		UpdateButtons();
		if (m_ListTimedOut)
		{
			if (m_Shown)
			{
				RequestList();
			}
			else
			{
				m_ListDirty = true;
			}
		}
	}

	// Re-sends the list with the current filter; the selected diff is re-requested once the list arrives
	// (the backup may have been deleted meanwhile).
	void Refresh()
	{
		m_ListRetries = 0;
		RequestList();
		m_RediffAfterList = true;
	}

	// One save can report several files (the batch file plus a MOVE/COPY target): coalesced into one
	// list request on the next update.
	void OnFileSaved(string fileKey)
	{
		if (m_Shown)
		{
			m_RefreshQueued = true;
		}
		else
		{
			m_ListDirty = true;
		}
	}

	bool HandleProgress(VPPXEProgress p)
	{
		if (!p || p.ReqId <= 0)
		{
			return false;
		}

		if (p.ReqId == m_ListReqId)
		{
			m_LastActivity = NowMs();
			return true;
		}

		if (p.ReqId != m_DiffReqId && p.ReqId != m_RestoreReqId && m_ActionReqIds.Find(p.ReqId) < 0)
		{
			return false;
		}

		m_LastActivity = NowMs();
		int percent = p.Percent;
		string pct = percent.ToString() + "%";
		string stage = Tr(VPPXEText.StageKey(p.Stage));
		string text = string.Format(Tr("#VSTR_XMLE_PROGRESS_FMT"), stage, pct);
		if (p.ReqId == m_DiffReqId)
		{
			ShowEmpty(text);
		}
		else if (m_Owner)
		{
			m_Owner.SetStatus(text);
		}

		return true;
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId <= 0 || !m_Owner)
		{
			return false;
		}

		string message = string.Format(Tr(key), arg);
		if (reqId == m_ListReqId)
		{
			if (!ok)
			{
				m_ListPending = false;
				if (key == "#VSTR_XMLE_ERR_NOT_READY")
				{
					m_ListTimedOut = true;
				}

				m_Owner.NotifyError(message);
			}

			return true;
		}

		if (reqId == m_DiffReqId)
		{
			if (!ok)
			{
				m_DiffPending = false;
				ShowEmpty(message);
			}

			return true;
		}

		if (reqId == m_RestoreReqId)
		{
			m_RestoreReqId = 0;
			if (ok)
			{
				m_Owner.Notify(message);
			}
			else
			{
				m_Owner.NotifyError(message);
			}

			RefreshAfterAction();
			return true;
		}

		int actionIdx = m_ActionReqIds.Find(reqId);
		if (actionIdx < 0)
		{
			return false;
		}

		m_ActionReqIds.Remove(actionIdx);
		if (ok)
		{
			m_Owner.Notify(message);
		}
		else
		{
			m_Owner.NotifyError(message);
		}

		RefreshAfterAction();
		return true;
	}

	// Restore replies with a VPPXESaveResult (MenuXMLEditor forwards results that are not its own saves).
	bool HandleSaveResult(VPPXESaveResult r)
	{
		if (!r || m_RestoreReqId <= 0 || r.ReqId != m_RestoreReqId || !m_Owner)
		{
			return false;
		}

		m_RestoreReqId = 0;
		string fileKey = r.FileKey;
		if (fileKey == "")
		{
			fileKey = m_RestoreFileKey;
		}

		if (r.Ok)
		{
			m_Owner.Notify(string.Format(Tr("#VSTR_XMLE_BK_RESTORED"), FileLabel(fileKey)));
			if (r.NoticeKey != "")
			{
				m_Owner.Notify(Tr(r.NoticeKey));
			}
		}
		else
		{
			m_Owner.NotifyError(string.Format(Tr(r.ErrorKey), r.ErrorArg));
		}

		RefreshAfterAction();
		if (IsTypesFile(fileKey) && m_Owner.GetModel())
		{
			m_Owner.GetModel().RequestIndex(fileKey);
		}

		return true;
	}

	// ---------------------------------------------------------------- server-to-client receivers (section 4)

	void XE_OnBackupList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEBackupListChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			if (MenuXMLEditor.NET_TRACE)
			{
				Print("[XMLEditor][Net] client: XE_OnBackupList arrived but could not be read");
			}

			return;
		}

		VPPXEBackupListChunk chunk = data.param1;
		if (MenuXMLEditor.NET_TRACE && chunk)
		{
			int traceReq = chunk.ReqId;
			int traceIdx = chunk.ChunkIdx;
			int traceCount = chunk.ChunkCount;
			string traceLine = "[XMLEditor][Net] client: backup list chunk req " + traceReq.ToString() + " " + traceIdx.ToString() + "/" + traceCount.ToString() + ", awaited req " + m_ListReqId.ToString();
			Print(traceLine);
		}

		if (!chunk || chunk.ReqId != m_ListReqId)
		{
			return;
		}

		// a late reply to the request that timed out is still accepted (it re-arms the wait)
		if (!m_ListPending)
		{
			if (!m_ListTimedOut)
			{
				return;
			}

			m_ListPending = true;
			m_ListTimedOut = false;
		}

		m_LastActivity = NowMs();
		if (chunk.Entries)
		{
			foreach (VPPXEBackupEntry entry : chunk.Entries)
			{
				if (entry)
				{
					m_Incoming.Insert(entry);
				}
			}
		}

		m_ListGot++;
		if (m_ListGot >= chunk.ChunkCount)
		{
			FinishList();
		}
	}

	void XE_OnBackupDiff(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEDiffChunk> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		VPPXEDiffChunk chunk = data.param1;
		if (!chunk || !m_DiffPending || chunk.ReqId != m_DiffReqId)
		{
			return;
		}

		m_LastActivity = NowMs();
		m_DiffAdded = chunk.Added;
		m_DiffRemoved = chunk.Removed;
		m_DiffChanged = chunk.Changed;
		if (chunk.Truncated)
		{
			m_DiffTruncated = true;
		}

		m_DiffSemantic = chunk.Semantic;
		if (chunk.Rows)
		{
			foreach (VPPXEDiffRow diffRow : chunk.Rows)
			{
				if (diffRow)
				{
					m_DiffRows.Insert(diffRow);
				}
			}
		}

		m_DiffGot++;
		if (m_DiffGot >= chunk.ChunkCount)
		{
			FinishDiff();
		}
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_BtnSnapshot)
		{
			BeginSnapshot();
			return true;
		}

		if (w == m_BtnPin)
		{
			TogglePin();
			return true;
		}

		if (w == m_BtnRestore)
		{
			BeginRestore();
			return true;
		}

		if (w == m_BtnDelete)
		{
			BeginDelete();
			return true;
		}

		if (w == m_BtnVsCurrent)
		{
			SetDiffMode(VPPXEDiffMode.VS_CURRENT);
			return true;
		}

		if (w == m_BtnVsNext)
		{
			SetDiffMode(VPPXEDiffMode.VS_NEXT);
			return true;
		}

		return false;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return false;
	}

	// ---------------------------------------------------------------- dialog and dropdown callbacks

	void OnFileSelected(int index)
	{
		if (index < 0 || index >= m_FileKeys.Count())
		{
			return;
		}

		m_Filter = m_FileKeys[index];
		m_FileDD.SetText(FileEntryText(index));
		m_FileDD.SetIndex(index);
		m_FileDD.Close();
		RequestList();
	}

	void OnConfirmSnapshot(int result, string input)
	{
		string fileKey = m_PendingSnapshotKey;
		m_PendingSnapshotKey = "";
		if (result != DIAGRESULT.OK || fileKey == "" || !m_Owner)
		{
			return;
		}

		string note = input;
		if (note.LengthUtf8() > VPPXEConst.MAX_NOTE_LENGTH)
		{
			note = note.SubstringUtf8(0, VPPXEConst.MAX_NOTE_LENGTH);
		}

		int reqId = m_Owner.NextReqId();
		m_ActionReqIds.Insert(reqId);
		m_LastActivity = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_CreateSnapshot", new Param3<int, string, string>(reqId, fileKey, note), true, null);
	}

	void OnConfirmRestore(int result, string input)
	{
		string backupId = m_PendingRestoreId;
		m_PendingRestoreId = "";
		if (result != DIAGRESULT.YES || backupId == "" || !m_Owner)
		{
			return;
		}

		m_RestoreReqId = m_Owner.NextReqId();
		m_RestoreFileKey = m_PendingRestoreKey;
		m_LastActivity = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_RestoreBackup", new Param2<int, string>(m_RestoreReqId, backupId), true, null);
	}

	void OnConfirmDelete(int result, string input)
	{
		string backupId = m_PendingDeleteId;
		m_PendingDeleteId = "";
		if (result != DIAGRESULT.YES || backupId == "" || !m_Owner)
		{
			return;
		}

		array<string> ids = new array<string>;
		ids.Insert(backupId);
		int reqId = m_Owner.NextReqId();
		m_ActionReqIds.Insert(reqId);
		m_LastActivity = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_DeleteBackups", new Param2<int, ref array<string>>(reqId, ids), true, null);
	}

	// ---------------------------------------------------------------- list

	protected void RequestList()
	{
		if (!HasPerm(VPPXEPerm.VIEW_BACKUPS) || !m_Owner)
		{
			return;
		}

		m_ListReqId = m_Owner.NextReqId();
		m_ListPending = true;
		m_ListTimedOut = false;
		m_Requested = true;
		m_ListDirty = false;
		m_ListGot = 0;
		m_Incoming = new array<ref VPPXEBackupEntry>;
		m_LastActivity = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetBackups", new Param2<int, string>(m_ListReqId, m_Filter), true, null);
	}

	protected void RefreshAfterAction()
	{
		if (!m_Shown)
		{
			m_ListDirty = true;
			return;
		}

		RequestList();
		m_RediffAfterList = true;
	}

	protected void FinishList()
	{
		m_ListPending = false;
		m_ListRetries = 0;
		m_HasList = true;
		m_Entries = m_Incoming;
		m_Incoming = new array<ref VPPXEBackupEntry>;
		UpdateFileDropdown();
		RebuildList();
		if (!m_RediffAfterList)
		{
			return;
		}

		m_RediffAfterList = false;
		VPPXEBackupEntry selected = SelectedEntry();
		if (selected)
		{
			RequestDiffFor(selected);
		}
	}

	// Rows: short stamp MM-DD HH:MM, file, admin, ChangeCount: Summary. Colour: Unverified warning-yellow,
	// pinned accent-orange, MatchesCurrent positive-green (first match wins), else text-primary.
	protected void RebuildList()
	{
		m_BackupsList.ClearItems();
		m_SelRow = -1;
		int keepRow = -1;
		int total = m_Entries.Count();
		for (int i = 0; i < total; i++)
		{
			VPPXEBackupEntry entry = m_Entries[i];
			int changes = entry.ChangeCount;
			string summaryText = changes.ToString() + ": " + entry.Summary;
			int row = m_BackupsList.AddItem(ShortStamp(entry.Stamp), null, 0);
			m_BackupsList.SetItem(row, FileLabel(entry.FileKey), null, 1);
			m_BackupsList.SetItem(row, entry.AdminName, null, 2);
			m_BackupsList.SetItem(row, summaryText, null, 3);
			int color = EntryColor(entry);
			for (int column = 0; column < 4; column++)
			{
				m_BackupsList.SetItemColor(row, column, color);
			}

			if (m_SelId != "" && entry.Id == m_SelId)
			{
				keepRow = row;
			}
		}

		if (keepRow >= 0)
		{
			m_BackupsList.SelectRow(keepRow);
			m_SelRow = keepRow;
			UpdateMeta(m_Entries[keepRow]);
			UpdateButtons();
			return;
		}

		m_SelId = "";
		m_RediffAfterList = false;
		ClearDiffView();
		UpdateMeta(null);
		UpdateButtons();
		if (total == 0)
		{
			ShowEmpty(Tr("#VSTR_XMLE_BK_EMPTY"));
		}
		else
		{
			ShowEmpty(Tr("#VSTR_XMLE_BK_SELECT"));
		}
	}

	protected int EntryColor(VPPXEBackupEntry entry)
	{
		if (entry.Unverified)
		{
			return ARGB(255, 217, 178, 61);
		}

		if (entry.Pinned)
		{
			return ARGB(255, 232, 163, 61);
		}

		if (entry.MatchesCurrent)
		{
			return ARGB(255, 76, 175, 80);
		}

		return ARGB(255, 255, 255, 255);
	}

	// 'YYYY-MM-DD HH:MM:SS' -> 'MM-DD HH:MM'; the full stamp stays in TxtBkMeta.
	protected string ShortStamp(string stamp)
	{
		if (stamp.Length() < 16)
		{
			return stamp;
		}

		return stamp.Substring(5, 11);
	}

	protected void PollSelection()
	{
		if (!m_BackupsList)
		{
			return;
		}

		int row = m_BackupsList.GetSelectedRow();
		if (row == m_SelRow)
		{
			return;
		}

		m_SelRow = row;
		OnEntrySelected(row);
	}

	protected void OnEntrySelected(int row)
	{
		if (row < 0 || row >= m_Entries.Count())
		{
			m_SelId = "";
			ClearDiffView();
			UpdateMeta(null);
			UpdateButtons();
			if (m_Entries.Count() == 0)
			{
				ShowEmpty(Tr("#VSTR_XMLE_BK_EMPTY"));
			}
			else
			{
				ShowEmpty(Tr("#VSTR_XMLE_BK_SELECT"));
			}

			return;
		}

		VPPXEBackupEntry entry = m_Entries[row];
		m_SelId = entry.Id;
		UpdateMeta(entry);
		UpdateButtons();
		RequestDiffFor(entry);
	}

	protected VPPXEBackupEntry SelectedEntry()
	{
		if (m_SelId == "")
		{
			return null;
		}

		if (m_SelRow >= 0 && m_SelRow < m_Entries.Count() && m_Entries[m_SelRow].Id == m_SelId)
		{
			return m_Entries[m_SelRow];
		}

		foreach (VPPXEBackupEntry entry : m_Entries)
		{
			if (entry.Id == m_SelId)
			{
				return entry;
			}
		}

		return null;
	}

	// TxtBkMeta: stamp by admin (reason), then the summary, the note and the Unverified line.
	protected void UpdateMeta(VPPXEBackupEntry entry)
	{
		m_MetaParts.Clear();
		if (entry)
		{
			string reason = Tr(VPPXEText.ReasonKey(entry.Reason));
			m_MetaParts.Insert(string.Format(Tr("#VSTR_XMLE_BK_META"), entry.Stamp, entry.AdminName, reason));
			if (entry.Summary != "")
			{
				m_MetaParts.Insert(entry.Summary);
			}

			if (entry.Note != "")
			{
				m_MetaParts.Insert(string.Format(Tr("#VSTR_XMLE_BK_NOTE"), entry.Note));
			}

			if (entry.Unverified)
			{
				m_MetaParts.Insert(Tr("#VSTR_XMLE_BK_UNVERIFIED"));
			}
		}

		string text = "";
		for (int i = 0; i < m_MetaParts.Count(); i++)
		{
			if (i > 0)
			{
				text = text + "\n";
			}

			text = text + m_MetaParts[i];
		}

		if (m_TxtBkMeta)
		{
			m_TxtBkMeta.SetText(text);
		}

		LayoutRight();
	}

	// ---------------------------------------------------------------- diff

	protected void SetDiffMode(int mode)
	{
		if (mode == m_DiffMode)
		{
			return;
		}

		m_DiffMode = mode;
		UpdateModeButtons();
		VPPXEBackupEntry selected = SelectedEntry();
		if (selected)
		{
			RequestDiffFor(selected);
		}
	}

	protected void RequestDiffFor(VPPXEBackupEntry entry)
	{
		ClearDiffView();
		if (!entry || !m_Owner)
		{
			return;
		}

		if (entry.Unverified)
		{
			m_DiffReqId = 0;
			ShowEmpty(Tr("#VSTR_XMLE_BK_UNVERIFIED"));
			return;
		}

		m_DiffReqId = m_Owner.NextReqId();
		m_DiffPending = true;
		m_LastActivity = NowMs();
		ShowEmpty(Tr("#VSTR_XMLE_STATUS_LOADING"));
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetBackupDiff", new Param3<int, string, int>(m_DiffReqId, entry.Id, m_DiffMode), true, null);
	}

	protected void ClearDiffView()
	{
		m_DiffPending = false;
		m_DiffGot = 0;
		m_DiffAdded = 0;
		m_DiffRemoved = 0;
		m_DiffChanged = 0;
		m_DiffTruncated = false;
		m_DiffSemantic = false;
		m_DiffRows.Clear();
		if (m_DiffList)
		{
			m_DiffList.ClearItems();
		}

		if (m_TxtDiffSummary)
		{
			m_TxtDiffSummary.SetText("");
		}
	}

	// ADDED positive-green, REMOVED danger-red, CHANGED warning-yellow, INFO text-secondary.
	protected void FinishDiff()
	{
		m_DiffPending = false;
		m_DiffList.ClearItems();
		foreach (VPPXEDiffRow diffRow : m_DiffRows)
		{
			string entryText = diffRow.Entry;
			string fieldText = diffRow.Field;
			string beforeText = diffRow.Before;
			string afterText = diffRow.After;
			int color = ARGB(255, 255, 255, 255);
			if (diffRow.Kind == VPPXEDiffKind.INFO)
			{
				entryText = string.Format(Tr("#VSTR_XMLE_BK_LINES"), diffRow.Before, diffRow.After);
				fieldText = "";
				beforeText = "";
				afterText = "";
				color = ARGB(255, 154, 160, 166);
			}
			else if (diffRow.Kind == VPPXEDiffKind.ADDED)
			{
				color = ARGB(255, 76, 175, 80);
				if (m_DiffSemantic && fieldText == "")
				{
					fieldText = Tr("#VSTR_XMLE_DIFF_ENTRY_ADDED");
				}
			}
			else if (diffRow.Kind == VPPXEDiffKind.REMOVED)
			{
				color = ARGB(255, 194, 69, 69);
				if (m_DiffSemantic && fieldText == "")
				{
					fieldText = Tr("#VSTR_XMLE_DIFF_ENTRY_REMOVED");
				}
			}
			else if (diffRow.Kind == VPPXEDiffKind.CHANGED)
			{
				color = ARGB(255, 217, 178, 61);
			}

			int row = m_DiffList.AddItem(entryText, null, 0);
			m_DiffList.SetItem(row, fieldText, null, 1);
			m_DiffList.SetItem(row, beforeText, null, 2);
			m_DiffList.SetItem(row, afterText, null, 3);
			for (int column = 0; column < 4; column++)
			{
				m_DiffList.SetItemColor(row, column, color);
			}
		}

		string summaryText = string.Format(Tr("#VSTR_XMLE_BK_SUMMARY"), m_DiffAdded, m_DiffRemoved, m_DiffChanged);
		if (m_DiffTruncated)
		{
			summaryText = summaryText + "   " + string.Format(Tr("#VSTR_XMLE_BK_TRUNCATED"), m_DiffRows.Count());
		}

		m_TxtDiffSummary.SetText(summaryText);
		if (m_DiffRows.Count() == 0)
		{
			ShowEmpty(Tr("#VSTR_XMLE_BK_SAME"));
		}
		else
		{
			HideEmpty();
		}
	}

	protected void ShowEmpty(string text)
	{
		if (!m_TxtDiffEmpty)
		{
			return;
		}

		m_TxtDiffEmpty.SetText(text);
		m_TxtDiffEmpty.Show(true);
	}

	protected void HideEmpty()
	{
		if (m_TxtDiffEmpty)
		{
			m_TxtDiffEmpty.Show(false);
		}
	}

	protected void UpdateModeButtons()
	{
		int active = ARGB(255, 232, 163, 61);
		int inactive = ARGB(255, 154, 160, 166);
		if (m_TxtVsCurrent)
		{
			if (m_DiffMode == VPPXEDiffMode.VS_CURRENT)
			{
				m_TxtVsCurrent.SetColor(active);
			}
			else
			{
				m_TxtVsCurrent.SetColor(inactive);
			}
		}

		if (m_TxtVsNext)
		{
			if (m_DiffMode == VPPXEDiffMode.VS_NEXT)
			{
				m_TxtVsNext.SetColor(active);
			}
			else
			{
				m_TxtVsNext.SetColor(inactive);
			}
		}
	}

	// ---------------------------------------------------------------- actions

	// The snapshot file is the filter selection, else the selected backup's file.
	protected void BeginSnapshot()
	{
		if (!HasPerm(VPPXEPerm.EDIT_TYPES) || !m_Owner)
		{
			return;
		}

		string fileKey = m_Filter;
		VPPXEBackupEntry selected = SelectedEntry();
		if (fileKey == "" && selected)
		{
			fileKey = selected.FileKey;
		}

		if (fileKey == "")
		{
			m_Owner.NotifyError(Tr("#VSTR_XMLE_BK_PICK_FILE"));
			return;
		}

		m_PendingSnapshotKey = fileKey;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_SNAPSHOT_BODY"), FileLabel(fileKey));
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_SNAPSHOT_TITLE", body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnConfirmSnapshot", true);
	}

	// Unverified raw copies stay pinned (they are the only copy of an unreadable file), so they are not toggled.
	protected void TogglePin()
	{
		VPPXEBackupEntry selected = SelectedEntry();
		if (!HasPerm(VPPXEPerm.DELETE_BACKUPS) || !selected || selected.Unverified || !m_Owner)
		{
			return;
		}

		bool pinned = !selected.Pinned;
		int reqId = m_Owner.NextReqId();
		m_ActionReqIds.Insert(reqId);
		m_LastActivity = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_PinBackup", new Param3<int, string, bool>(reqId, selected.Id, pinned), true, null);
	}

	protected void BeginRestore()
	{
		VPPXEBackupEntry selected = SelectedEntry();
		if (!HasPerm(VPPXEPerm.RESTORE) || !selected || selected.Unverified || !m_Owner)
		{
			return;
		}

		m_PendingRestoreId = selected.Id;
		m_PendingRestoreKey = selected.FileKey;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_RESTORE_BODY"), FileLabel(selected.FileKey), selected.Stamp);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_RESTORE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRestore", false);
	}

	protected void BeginDelete()
	{
		VPPXEBackupEntry selected = SelectedEntry();
		if (!HasPerm(VPPXEPerm.DELETE_BACKUPS) || !selected || !m_Owner)
		{
			return;
		}

		m_PendingDeleteId = selected.Id;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_BKDEL_BODY"), 1);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_BKDEL_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmDelete", false);
	}

	protected void UpdateButtons()
	{
		VPPXEBackupEntry selected = SelectedEntry();
		bool hasSel = selected != null;
		bool verified = hasSel && !selected.Unverified;
		bool unpinned = hasSel && !selected.Pinned;
		EnableWidget(m_BtnSnapshot, HasPerm(VPPXEPerm.EDIT_TYPES));
		EnableWidget(m_BtnPin, HasPerm(VPPXEPerm.DELETE_BACKUPS) && verified);
		EnableWidget(m_BtnRestore, HasPerm(VPPXEPerm.RESTORE) && verified);
		EnableWidget(m_BtnDelete, HasPerm(VPPXEPerm.DELETE_BACKUPS) && unpinned);
		m_PinKey = "#VSTR_XMLE_BK_PIN";
		if (hasSel && selected.Pinned)
		{
			m_PinKey = "#VSTR_XMLE_BK_UNPIN";
		}

		if (m_TxtPin)
		{
			m_TxtPin.SetText(Tr(m_PinKey));
		}

		LayoutActions();
	}

	protected void UpdateRetention()
	{
		m_RetentionParts.Clear();
		VPPXESessionInfo session = GetSession();
		if (session)
		{
			m_RetentionParts.Insert(string.Format(Tr("#VSTR_XMLE_BK_RETENTION"), session.BackupMaxPerFile, session.BackupMaxAgeDays, session.BackupMaxTotalMB));
		}

		if (!m_TxtRetention)
		{
			return;
		}

		if (m_RetentionParts.Count() > 0)
		{
			m_TxtRetention.SetText(m_RetentionParts[0]);
		}
		else
		{
			m_TxtRetention.SetText("");
		}

		LayoutLeft();
	}

	// File filter: 'All files' plus at most FILE_DD_MAX_ROWS - 1 keys (entries past the 50 GridSpacer rows are never
	// laid out). Keys are chosen by priority (the current filter, the Types scope, files with backups, editable
	// types files, other editable files, the rest of the session files) and listed in the usual order: session
	// files first, then keys only seen in backup entries. The Types scope (also set by a Files-tab double-click)
	// keeps any types file reachable for its first snapshot however many files the mission registers.
	protected void UpdateFileDropdown()
	{
		if (!m_FileDD)
		{
			return;
		}

		array<string> chosen = new array<string>;
		if (m_Filter != "")
		{
			chosen.Insert(m_Filter);
		}

		string typesScope = "";
		if (m_Owner)
		{
			typesScope = m_Owner.GetTypesScope();
		}

		if (typesScope != "" && IsTypesFile(typesScope) && chosen.Find(typesScope) < 0)
		{
			chosen.Insert(typesScope);
		}

		foreach (VPPXEBackupEntry entry : m_Entries)
		{
			if (chosen.Count() >= FILE_DD_MAX_ROWS - 1)
			{
				break;
			}

			if (entry && entry.FileKey != "" && chosen.Find(entry.FileKey) < 0)
			{
				chosen.Insert(entry.FileKey);
			}
		}

		VPPXESessionInfo session = GetSession();
		if (session && session.Files)
		{
			for (int pass = 0; pass < 3; pass++)
			{
				ChooseSessionFiles(chosen, session.Files, pass);
			}
		}

		array<string> keys = new array<string>;
		if (session && session.Files)
		{
			foreach (VPPXEFileInfo sessionFile : session.Files)
			{
				if (sessionFile && chosen.Find(sessionFile.Key) >= 0 && keys.Find(sessionFile.Key) < 0)
				{
					keys.Insert(sessionFile.Key);
				}
			}
		}

		foreach (VPPXEBackupEntry backupEntry : m_Entries)
		{
			if (backupEntry && chosen.Find(backupEntry.FileKey) >= 0 && keys.Find(backupEntry.FileKey) < 0)
			{
				keys.Insert(backupEntry.FileKey);
			}
		}

		if (m_Filter != "" && keys.Find(m_Filter) < 0)
		{
			keys.Insert(m_Filter);
		}

		string sig = "";
		foreach (string key : keys)
		{
			sig = sig + key + "|";
		}

		if (sig == m_FileKeysSig)
		{
			return;
		}

		m_FileKeysSig = sig;
		m_FileKeys.Clear();
		m_FileKeys.Insert("");
		m_FileDD.RemoveAllElements();
		m_FileDD.AddElement(Tr("#VSTR_XMLE_BK_ALLFILES"));
		foreach (string fileKey : keys)
		{
			m_FileKeys.Insert(fileKey);
			m_FileDD.AddElement(FileLabel(fileKey));
		}

		int index = m_FileKeys.Find(m_Filter);
		if (index < 0)
		{
			index = 0;
		}

		m_FileDD.SetIndex(index);
		m_FileDD.SetText(FileEntryText(index));
	}

	// Adds the session files of one priority pass (0 editable types files, 1 other editable files, 2 the rest)
	// in session order until the dropdown cap is reached.
	protected void ChooseSessionFiles(array<string> chosen, array<ref VPPXEFileInfo> files, int pass)
	{
		foreach (VPPXEFileInfo fileInfo : files)
		{
			if (chosen.Count() >= FILE_DD_MAX_ROWS - 1)
			{
				return;
			}

			if (!fileInfo || fileInfo.Key == "" || chosen.Find(fileInfo.Key) >= 0)
			{
				continue;
			}

			bool editable = (fileInfo.Flags & VPPXEFileFlag.EDITABLE) != 0;
			int rank = 2;
			if (editable && fileInfo.Kind == VPPXEFileKind.TYPES)
			{
				rank = 0;
			}
			else if (editable)
			{
				rank = 1;
			}

			if (rank == pass)
			{
				chosen.Insert(fileInfo.Key);
			}
		}
	}

	protected string FileEntryText(int index)
	{
		if (index <= 0 || index >= m_FileKeys.Count())
		{
			return Tr("#VSTR_XMLE_BK_ALLFILES");
		}

		return FileLabel(m_FileKeys[index]);
	}

	protected void CheckTimeouts()
	{
		if (!m_ListPending && !m_DiffPending)
		{
			return;
		}

		if (NowMs() - m_LastActivity <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return;
		}

		if (m_DiffPending)
		{
			m_DiffPending = false;
			ShowEmpty(Tr("#VSTR_XMLE_ERR_NOT_READY"));
		}

		// a silent list request is asked again up to AUTO_RETRY_MAX times; after that it is 'timed out', which Show,
		// OnSessionChanged and a late reply to the same request all recover from
		if (m_ListPending)
		{
			if (m_ListRetries < AUTO_RETRY_MAX)
			{
				m_ListRetries++;
				RequestList();
				return;
			}

			m_ListPending = false;
			m_ListTimedOut = true;
			m_ListRetries = 0;
			if (m_Owner)
			{
				m_Owner.NotifyError(Tr("#VSTR_XMLE_ERR_NOT_READY"));
			}
		}
	}

	// ---------------------------------------------------------------- layout

	protected void UpdateUnit()
	{
		m_Unit = 1.0;
		if (!m_BackupsHeader)
		{
			return;
		}

		float headerW;
		float headerH;
		m_BackupsHeader.GetScreenSize(headerW, headerH);
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

	// Left: header 28, file filter host at 32 (its popup stays inside BackupsLeft), list, action row,
	// retention text at the bottom.
	protected void LayoutLeft()
	{
		if (m_LeftW < 40 || m_LeftH < 80)
		{
			return;
		}

		float innerW = m_LeftW - 12;
		int retentionLines = EstimateLines(m_RetentionParts, innerW);
		float retentionH = retentionLines * LINE_H + 4;
		float retentionY = m_LeftH - retentionH - 4;
		float actionsY = retentionY - ACTION_H - 4;
		m_FileHost.SetPos(6, 32);
		m_FileHost.SetSize(innerW, 24);
		m_TxtRetention.SetPos(6, retentionY);
		m_TxtRetention.SetSize(innerW, retentionH);
		m_BackupsActions.SetPos(6, actionsY);
		m_BackupsActions.SetSize(innerW, ACTION_H);
		float listY = 60;
		float listH = actionsY - listY - 4;
		if (listH < 30)
		{
			listH = 30;
		}

		m_BackupsList.SetPos(6, listY);
		m_BackupsList.SetSize(innerW, listH);
		LayoutActions();
	}

	protected void LayoutActions()
	{
		if (m_LeftW < 40)
		{
			return;
		}

		float innerW = m_LeftW - 12;
		float buttonW = (innerW - 3 * BTN_GAP) / 4;
		PlaceIconButton(m_BtnSnapshot, m_ImgSnapshot, m_TxtSnapshot, Tr("#VSTR_XMLE_BK_SNAPSHOT"), 0, buttonW);
		PlaceIconButton(m_BtnPin, m_ImgPin, m_TxtPin, Tr(m_PinKey), buttonW + BTN_GAP, buttonW);
		PlaceIconButton(m_BtnRestore, m_ImgRestore, m_TxtRestore, Tr("#VSTR_XMLE_BK_RESTORE"), 2 * (buttonW + BTN_GAP), buttonW);
		PlaceIconButton(m_BtnDelete, m_ImgDelete, m_TxtDelete, Tr("#VSTR_XMLE_BK_DELETE"), 3 * (buttonW + BTN_GAP), buttonW);
	}

	// Hides the icon when 24 + 7 px per character of the label exceeds the button width.
	protected void PlaceIconButton(Widget button, Widget iconWidget, TextWidget label, string text, float x, float width)
	{
		if (!button)
		{
			return;
		}

		button.SetPos(x, 0);
		button.SetSize(width, ACTION_H);
		bool showIcon = 24 + EstimateTextPx(text, 7) <= width;
		if (iconWidget)
		{
			iconWidget.Show(showIcon);
		}

		if (!label)
		{
			return;
		}

		if (showIcon)
		{
			label.SetPos(24, 0);
			label.SetSize(width - 28, ACTION_H);
		}
		else
		{
			label.SetPos(4, 0);
			label.SetSize(width - 8, ACTION_H);
		}
	}

	// Right: header 28 with the two mode buttons sized to their text, summary at 32, meta block, diff list.
	protected void LayoutRight()
	{
		if (m_RightW < 40 || m_RightH < 80)
		{
			return;
		}

		float nextW = TextButtonWidth(Tr("#VSTR_XMLE_BK_VS_NEXT"));
		float currentW = TextButtonWidth(Tr("#VSTR_XMLE_BK_VS_CURRENT"));
		float nextX = m_RightW - 6 - nextW;
		float currentX = nextX - 4 - currentW;
		m_BtnVsNext.SetPos(nextX, 2);
		m_BtnVsNext.SetSize(nextW, 24);
		m_BtnVsCurrent.SetPos(currentX, 2);
		m_BtnVsCurrent.SetSize(currentW, 24);
		// TxtDiffHeader (x 30, width a proportion of the full-width DiffHeader strip) ends 6 units before the
		// mode buttons, so a long title clips instead of running under them
		if (m_TxtDiffHeader)
		{
			float headerRatio = (currentX - 36) / m_RightW;
			if (headerRatio < 0.02)
			{
				headerRatio = 0.02;
			}

			m_TxtDiffHeader.SetSize(headerRatio, 1);
		}

		float innerW = m_RightW - 16;
		m_TxtDiffSummary.SetPos(8, 32);
		m_TxtDiffSummary.SetSize(innerW, 18);
		int metaLines = EstimateLines(m_MetaParts, innerW);
		float metaH = metaLines * LINE_H + 4;
		if (metaLines == 0)
		{
			metaH = 1;
		}

		m_TxtBkMeta.SetPos(8, 52);
		m_TxtBkMeta.SetSize(innerW, metaH);
		float listY = 52 + metaH + 4;
		float listH = m_RightH - listY - 6;
		if (listH < 30)
		{
			listH = 30;
		}

		m_DiffList.SetPos(6, listY);
		m_DiffList.SetSize(m_RightW - 12, listH);
		m_TxtDiffEmpty.SetPos(6, listY);
		m_TxtDiffEmpty.SetSize(m_RightW - 12, listH);
	}

	protected float TextButtonWidth(string text)
	{
		float width = 16 + EstimateTextPx(text, 7);
		return Math.Clamp(width, 60, 200);
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

	protected bool HasPerm(int permBit)
	{
		if (!m_Owner)
		{
			return false;
		}

		return m_Owner.HasPerm(permBit);
	}

	protected void EnableWidget(Widget w, bool enabled)
	{
		if (w)
		{
			w.Enable(enabled);
		}
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
