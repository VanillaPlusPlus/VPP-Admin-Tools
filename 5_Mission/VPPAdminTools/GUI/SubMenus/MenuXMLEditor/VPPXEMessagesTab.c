// VPP XML Editor: MESSAGES tab. Edits the server messages files (db/messages.xml and every <ce type="messages">
// file). Left: file dropdown, the file's messages (#, when, text) and ADD / DUPLICATE / DELETE. Right: the selected
// message: its modes (on connect, repeat, countdown, scheduled time) with their minutes, shut down, the text with
// #name / #port / #tmin inserts, a live preview with a plain summary of what the server does with it and warnings for
// everything the server ignores (rules in VPPXEMessageRules). Edits stay local until SAVE (XE_SaveMessages, one
// file per save, EditMessages); REVERT drops them.

class VPPXEMsgWork : Managed
{
	int OrigIndex;
	bool Deleted;
	string InputError;
	ref VPPXEMessageRow Row;
	ref VPPXEMessageRow Orig;

	void VPPXEMsgWork()
	{
		OrigIndex = -1;
		InputError = "";
		Row = new VPPXEMessageRow();
	}

	bool IsDirty()
	{
		if (OrigIndex < 0 || Deleted || !Orig)
		{
			return true;
		}

		return !Row.SameAs(Orig);
	}
};

class VPPXEMsgFile : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	bool ServerChanged;
	ref array<ref VPPXEMsgWork> Work;

	void VPPXEMsgFile()
	{
		Key = "";
		ErrorKey = "";
		Work = new array<ref VPPXEMsgWork>;
	}

	int DirtyCount()
	{
		int dirty = 0;
		foreach (VPPXEMsgWork work : Work)
		{
			if (work.IsDirty())
			{
				dirty++;
			}
		}

		return dirty;
	}
};

class VPPXEMsgIncoming : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	int ChunkCount;
	ref map<int, ref array<ref VPPXEMessageRow>> ChunkRows;

	void VPPXEMsgIncoming()
	{
		Key = "";
		ErrorKey = "";
		ChunkRows = new map<int, ref array<ref VPPXEMessageRow>>;
	}

	// Received chunks (a repeated chunk counts once).
	int Got()
	{
		return ChunkRows.Count();
	}

	// Rows in chunk order: their position is the message index UPDATE and DELETE address.
	void JoinRows(array<ref VPPXEMessageRow> outRows)
	{
		outRows.Clear();
		for (int c = 0; c < ChunkCount; c++)
		{
			array<ref VPPXEMessageRow> part = ChunkRows.Get(c);
			if (!part)
			{
				continue;
			}

			foreach (VPPXEMessageRow row : part)
			{
				outRows.Insert(row);
			}
		}
	}
};

class VPPXEMessagesTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int ROW_H = 28;
	const static int LABEL_W = 150;
	const static int INPUT_W = 90;
	const static int FOOTER_H = 36;
	const static int BAR_H = 40;
	const static int MODE_COUNT = 4;
	const static int NEW_DELAY = 2;
	const static int NEW_REPEAT = 15;
	const static int NEW_DEADLINE = 60;
	const static string NEW_TIME = "06:00";
	// Text set on a widget loses the "#" of every "#word" (the engine reads it as a stringtable key; "#name" shows
	// "name"). HASH_* is how this client writes a "#" so it survives, probed once against the text box (ProbeHashMode).
	const static int HASH_RAW = 0;
	const static int HASH_DOUBLED = 1;
	const static int HASH_BACKSLASH = 2;
	const static int HASH_FULLWIDTH = 3;
	const static string FULLWIDTH_HASH = "＃";

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_Left;
	protected Widget m_ListHeader;
	protected Widget m_FileHost;
	protected TextListboxWidget m_MsgList;
	protected ButtonWidget m_BtnAdd;
	protected ButtonWidget m_BtnDup;
	protected ButtonWidget m_BtnDelete;
	protected Widget m_Right;
	protected TextWidget m_TxtEditTitle;
	protected TextWidget m_TxtLine;
	protected Widget m_Form;
	protected TextWidget m_TxtWhen;
	protected ref array<ButtonWidget> m_BtnMode;
	protected ref array<Widget> m_FillMode;
	protected ref array<TextWidget> m_TxtMode;
	protected TextWidget m_LblDelay;
	protected EditBoxWidget m_InputDelay;
	protected TextWidget m_TxtDelayUnit;
	protected TextWidget m_LblRepeat;
	protected EditBoxWidget m_InputRepeat;
	protected TextWidget m_TxtRepeatUnit;
	protected TextWidget m_LblDeadline;
	protected EditBoxWidget m_InputDeadline;
	protected TextWidget m_TxtDeadlineUnit;
	protected TextWidget m_LblTime;
	protected EditBoxWidget m_InputTime;
	protected TextWidget m_LblDate;
	protected EditBoxWidget m_InputDate;
	protected ButtonWidget m_BtnShutdown;
	protected Widget m_FillShutdown;
	protected TextWidget m_TxtShutdown;
	protected TextWidget m_TxtTextTitle;
	protected TextWidget m_TxtBytes;
	protected EditBoxWidget m_InputText;
	protected TextWidget m_TxtInsert;
	protected ButtonWidget m_BtnPhName;
	protected ButtonWidget m_BtnPhPort;
	protected ButtonWidget m_BtnPhTmin;
	protected TextWidget m_TxtPreviewTitle;
	protected Widget m_PreviewCard;
	protected TextWidget m_TxtPreview;
	protected TextWidget m_TxtWarnings;
	protected TextWidget m_TxtEmpty;
	protected Widget m_ActionBar;
	protected TextWidget m_TxtStatus;
	protected ButtonWidget m_BtnRevert;
	protected ButtonWidget m_BtnSave;
	protected ref VPPDropDownMenu m_FileDD;

	protected ref map<string, ref VPPXEMsgFile> m_Files;
	protected ref array<string> m_FileKeys;
	protected string m_FileKey;
	protected string m_WantFile;
	protected ref array<int> m_ListWork;
	protected int m_SelWork;
	protected int m_ListSel;
	protected bool m_Loading;
	protected string m_LastDelay;
	protected string m_LastRepeat;
	protected string m_LastDeadline;
	protected string m_LastTime;
	protected string m_LastDate;
	protected string m_LastText;

	protected bool m_Shown;
	protected bool m_Loaded;
	protected int m_ReqId;
	protected bool m_Pending;
	protected int m_SentAt;
	protected int m_ExpectedFiles;
	protected ref map<string, ref VPPXEMsgIncoming> m_Incoming;
	protected string m_LoadedSig;
	protected ref map<string, bool> m_ForceFresh;
	protected int m_SaveReqId;
	protected bool m_Saving;
	protected int m_SaveSentAt;
	protected string m_SaveFile;
	protected string m_StatusText;
	protected bool m_StatusError;
	protected int m_HashMode;

	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXEMessagesTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_BtnMode = new array<ButtonWidget>;
		m_FillMode = new array<Widget>;
		m_TxtMode = new array<TextWidget>;
		m_Files = new map<string, ref VPPXEMsgFile>;
		m_FileKeys = new array<string>;
		m_ListWork = new array<int>;
		m_Incoming = new map<string, ref VPPXEMsgIncoming>;
		m_ForceFresh = new map<string, bool>;
		m_SelWork = -1;
		m_ListSel = -1;
		m_Unit = 1.0;
		m_FileKey = "";
		m_WantFile = "";
		m_LoadedSig = "";
		m_StatusText = "";

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorMessagesTab, host);
		m_Root.SetHandler(this);
		BindWidgets();
		ProbeHashMode();

		m_FileDD = new VPPDropDownMenu(m_FileHost, "");
		m_FileDD.m_OnSelectItem.Insert(OnFilePicked);
		SetTokenLabel("TxtPhName", "#name");
		SetTokenLabel("TxtPhPort", "#port");
		SetTokenLabel("TxtPhTmin", "#tmin");
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnMessagesChunk", this, SingleplayerExecutionType.Client);
		ShowEditor(false);
		UpdateActionBar();
	}

	protected void BindWidgets()
	{
		m_Left = m_Root.FindAnyWidget("MsgLeft");
		m_ListHeader = m_Root.FindAnyWidget("MsgListHeader");
		m_FileHost = m_Root.FindAnyWidget("MsgFileHost");
		m_MsgList = TextListboxWidget.Cast(m_Root.FindAnyWidget("MsgList"));
		m_BtnAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgAdd"));
		m_BtnDup = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgDup"));
		m_BtnDelete = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgDelete"));
		m_Right = m_Root.FindAnyWidget("MsgRight");
		m_TxtEditTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgEditTitle"));
		m_TxtLine = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgLine"));
		m_Form = m_Root.FindAnyWidget("MsgForm");
		m_TxtWhen = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgWhen"));
		array<string> modeSuffix = {"Connect", "Repeat", "Countdown", "Time"};
		foreach (string ms : modeSuffix)
		{
			m_BtnMode.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgMode" + ms)));
			m_FillMode.Insert(m_Root.FindAnyWidget("FillMsgMode" + ms));
			m_TxtMode.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgMode" + ms)));
		}

		m_LblDelay = TextWidget.Cast(m_Root.FindAnyWidget("LblMsgDelay"));
		m_InputDelay = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgDelay"));
		m_TxtDelayUnit = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgDelayUnit"));
		m_LblRepeat = TextWidget.Cast(m_Root.FindAnyWidget("LblMsgRepeat"));
		m_InputRepeat = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgRepeat"));
		m_TxtRepeatUnit = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgRepeatUnit"));
		m_LblDeadline = TextWidget.Cast(m_Root.FindAnyWidget("LblMsgDeadline"));
		m_InputDeadline = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgDeadline"));
		m_TxtDeadlineUnit = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgDeadlineUnit"));
		m_LblTime = TextWidget.Cast(m_Root.FindAnyWidget("LblMsgTime"));
		m_InputTime = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgTime"));
		m_LblDate = TextWidget.Cast(m_Root.FindAnyWidget("LblMsgDate"));
		m_InputDate = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgDate"));
		m_BtnShutdown = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgShutdown"));
		m_FillShutdown = m_Root.FindAnyWidget("FillMsgShutdown");
		m_TxtShutdown = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgShutdown"));
		m_TxtTextTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgTextTitle"));
		m_TxtBytes = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgBytes"));
		m_InputText = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputMsgText"));
		m_TxtInsert = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgInsert"));
		m_BtnPhName = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnPhName"));
		m_BtnPhPort = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnPhPort"));
		m_BtnPhTmin = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnPhTmin"));
		m_TxtPreviewTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgPreviewTitle"));
		m_PreviewCard = m_Root.FindAnyWidget("MsgPreviewCard");
		m_TxtPreview = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgPreview"));
		m_TxtWarnings = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgWarnings"));
		m_TxtEmpty = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgEmpty"));
		m_ActionBar = m_Root.FindAnyWidget("MsgActionBar");
		m_TxtStatus = TextWidget.Cast(m_Root.FindAnyWidget("TxtMsgStatus"));
		m_BtnRevert = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgRevert"));
		m_BtnSave = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMsgSave"));
	}

	// ---------------------------------------------------------------- tab API

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			if (m_FileDD)
			{
				m_FileDD.Close();
			}

			return;
		}

		OnResize();
		if (!m_Loaded || CurrentSig() != m_LoadedSig)
		{
			RequestList();
		}

		RebuildAll();
	}

	// True while a list or save request waits for its reply (the window holds XE_CloseSession back meanwhile).
	bool HasInFlight()
	{
		int now = NowMs();
		if (m_Pending && now - m_SentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return true;
		}

		return m_Saving && now - m_SaveSentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void Refresh()
	{
		RequestList();
	}

	// A new file list or new revisions: reload (FinishList keeps files with unsaved edits and flags the ones whose
	// server copy changed).
	void OnSessionChanged()
	{
		RebuildFileKeys();
		string sig = CurrentSig();
		if (m_Loaded && sig != m_LoadedSig)
		{
			FlagServerChanges();
			if (m_Shown)
			{
				RequestList();
			}
			else
			{
				m_Loaded = false;
			}
		}

		if (m_Shown)
		{
			RebuildAll();
		}
	}

	// MenuXMLEditor.OpenMessagesFile: show that file (after the next reply when it is not loaded yet).
	void SelectFile(string fileKey)
	{
		m_WantFile = fileKey;
		if (m_Files.Contains(fileKey))
		{
			SwitchFile(fileKey);
			m_WantFile = "";
		}
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
		PollListSelection();
		PollInputs();
		int now = NowMs();
		if (m_Pending && now - m_SentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Pending = false;
			SetStatus(Tr("#VSTR_XMLE_ERR_NOT_READY"), true);
		}

		if (m_Saving && now - m_SaveSentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Saving = false;
			SetStatus(Tr("#VSTR_XMLE_ERR_NOT_READY"), true);
			UpdateActionBar();
		}
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId <= 0)
		{
			return false;
		}

		string textKey = key;
		if (textKey == "")
		{
			textKey = "#VSTR_XMLE_ERR_INTERNAL";
		}

		if (reqId == m_ReqId)
		{
			m_Pending = false;
			if (!ok)
			{
				SetStatus(string.Format(Tr(textKey), arg), true);
			}

			return true;
		}

		if (reqId != m_SaveReqId)
		{
			return false;
		}

		if (!m_Saving)
		{
			return true;
		}

		m_Saving = false;
		if (ok)
		{
			string done = string.Format(Tr(textKey), FileLabel(m_SaveFile));
			m_Owner.Notify(done);
			SetStatus(done, false);
			m_ForceFresh.Set(m_SaveFile, true);
			RequestList();
		}
		else
		{
			string failed = string.Format(Tr(textKey), arg);
			m_Owner.NotifyError(failed);
			SetStatus(failed, true);
			if (textKey == "#VSTR_XMLE_ERR_STALE")
			{
				VPPXEMsgFile staleFile = m_Files.Get(m_SaveFile);
				if (staleFile)
				{
					staleFile.ServerChanged = true;
				}
			}
		}

		UpdateActionBar();
		return true;
	}

	// ---------------------------------------------------------------- server-to-client receiver

	void XE_OnMessagesChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEMessagesChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEMessagesChunk chunk = data.param1;
		if (!chunk || chunk.ReqId != m_ReqId || !m_Pending)
		{
			return;
		}

		m_SentAt = NowMs();
		if (chunk.FileCount <= 0)
		{
			m_ExpectedFiles = 0;
			FinishList();
			return;
		}

		m_ExpectedFiles = chunk.FileCount;
		VPPXEMsgIncoming incoming = m_Incoming.Get(chunk.FileKey);
		if (!incoming)
		{
			incoming = new VPPXEMsgIncoming();
			incoming.Key = chunk.FileKey;
			m_Incoming.Set(chunk.FileKey, incoming);
		}

		incoming.Revision = chunk.Revision;
		incoming.Editable = chunk.Editable;
		incoming.ErrorKey = chunk.ErrorKey;
		incoming.ChunkCount = chunk.ChunkCount;
		array<ref VPPXEMessageRow> chunkRows = new array<ref VPPXEMessageRow>;
		if (chunk.Rows)
		{
			foreach (VPPXEMessageRow row : chunk.Rows)
			{
				if (row)
				{
					chunkRows.Insert(row);
				}
			}
		}

		incoming.ChunkRows.Set(chunk.ChunkIdx, chunkRows);

		if (m_Incoming.Count() < m_ExpectedFiles)
		{
			return;
		}

		for (int i = 0; i < m_Incoming.Count(); i++)
		{
			VPPXEMsgIncoming check = m_Incoming.GetElement(i);
			if (check.Got() < check.ChunkCount)
			{
				return;
			}
		}

		FinishList();
	}

	// ---------------------------------------------------------------- loading

	protected void RequestList()
	{
		if (!m_Owner)
		{
			return;
		}

		m_ReqId = m_Owner.NextReqId();
		m_Pending = true;
		m_SentAt = NowMs();
		m_Incoming = new map<string, ref VPPXEMsgIncoming>;
		m_ExpectedFiles = 0;
		SetStatus(Tr("#VSTR_XMLE_STATUS_LOADING"), false);
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetMessages", new Param1<int>(m_ReqId), true, null);
	}

	// Files with unsaved edits keep them (flagged when the server copy changed meanwhile) unless a save or revert
	// asked for a fresh copy; every other file takes the server rows.
	protected void FinishList()
	{
		m_Pending = false;
		m_Loaded = true;
		m_LoadedSig = CurrentSig();
		map<string, ref VPPXEMsgFile> fresh = new map<string, ref VPPXEMsgFile>;
		for (int i = 0; i < m_Incoming.Count(); i++)
		{
			VPPXEMsgIncoming incoming = m_Incoming.GetElement(i);
			VPPXEMsgFile kept = m_Files.Get(incoming.Key);
			bool forced = m_ForceFresh.Contains(incoming.Key);
			if (kept && !forced && kept.DirtyCount() > 0)
			{
				if (kept.Revision != incoming.Revision)
				{
					kept.ServerChanged = true;
				}

				fresh.Set(incoming.Key, kept);
				continue;
			}

			VPPXEMsgFile file = new VPPXEMsgFile();
			file.Key = incoming.Key;
			file.Revision = incoming.Revision;
			file.Editable = incoming.Editable;
			file.ErrorKey = incoming.ErrorKey;
			array<ref VPPXEMessageRow> rows = new array<ref VPPXEMessageRow>;
			incoming.JoinRows(rows);
			int rowCount = rows.Count();
			for (int r = 0; r < rowCount; r++)
			{
				VPPXEMsgWork work = new VPPXEMsgWork();
				work.OrigIndex = r;
				work.Orig = new VPPXEMessageRow();
				work.Orig.CopyFrom(rows[r]);
				work.Row.CopyFrom(rows[r]);
				file.Work.Insert(work);
			}

			fresh.Set(incoming.Key, file);
		}

		m_Files = fresh;
		m_ForceFresh.Clear();
		m_Incoming = new map<string, ref VPPXEMsgIncoming>;
		RebuildFileKeys();
		if (m_WantFile != "" && m_Files.Contains(m_WantFile))
		{
			m_FileKey = m_WantFile;
			m_WantFile = "";
			m_SelWork = -1;
		}

		if (!m_Files.Contains(m_FileKey))
		{
			m_FileKey = "";
			if (m_FileKeys.Count() > 0)
			{
				m_FileKey = m_FileKeys[0];
			}

			m_SelWork = -1;
		}

		SetStatus("", false);
		RebuildAll();
	}

	// Messages files in session order.
	protected void RebuildFileKeys()
	{
		m_FileKeys.Clear();
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (fileInfo && fileInfo.Kind == VPPXEFileKind.MESSAGES)
			{
				m_FileKeys.Insert(fileInfo.Key);
			}
		}
	}

	protected string CurrentSig()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return "";
		}

		string sig = "";
		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || fileInfo.Kind != VPPXEFileKind.MESSAGES)
			{
				continue;
			}

			int revision = fileInfo.Revision;
			int flags = fileInfo.Flags;
			sig = sig + "|" + fileInfo.Key + ":" + revision.ToString() + ":" + flags.ToString();
		}

		return sig;
	}

	protected void FlagServerChanges()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || fileInfo.Kind != VPPXEFileKind.MESSAGES)
			{
				continue;
			}

			VPPXEMsgFile file = m_Files.Get(fileInfo.Key);
			if (file && file.DirtyCount() > 0 && file.Revision != fileInfo.Revision)
			{
				file.ServerChanged = true;
			}
		}
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT || !w)
		{
			return false;
		}

		int modeIdx = m_BtnMode.Find(ButtonWidget.Cast(w));
		if (modeIdx >= 0)
		{
			ToggleMode(modeIdx);
			return true;
		}

		if (w == m_BtnShutdown)
		{
			ToggleShutdown();
			return true;
		}

		if (w == m_BtnPhName)
		{
			InsertToken("#name");
			return true;
		}

		if (w == m_BtnPhPort)
		{
			InsertToken("#port");
			return true;
		}

		if (w == m_BtnPhTmin)
		{
			InsertToken("#tmin");
			return true;
		}

		if (w == m_BtnAdd)
		{
			AddMessage(null);
			return true;
		}

		if (w == m_BtnDup)
		{
			VPPXEMsgWork source = SelectedWork();
			if (source)
			{
				AddMessage(source.Row);
			}

			return true;
		}

		if (w == m_BtnDelete)
		{
			DeleteSelected();
			return true;
		}

		if (w == m_BtnSave)
		{
			StartSave();
			return true;
		}

		if (w == m_BtnRevert)
		{
			StartRevert();
			return true;
		}

		return false;
	}

	void OnFilePicked(int index)
	{
		m_FileDD.Close();
		if (index < 0 || index >= m_FileKeys.Count())
		{
			RebuildFileDropdown();
			return;
		}

		SwitchFile(m_FileKeys[index]);
	}

	protected void SwitchFile(string fileKey)
	{
		m_FileKey = fileKey;
		m_SelWork = -1;
		m_ListSel = -1;
		RebuildAll();
	}

	// ---------------------------------------------------------------- editing

	protected bool CanEdit()
	{
		VPPXEMsgFile file = CurrentFile();
		if (!file || !file.Editable || m_Saving)
		{
			return false;
		}

		// a save or revert reloads this file fresh: an edit made meanwhile would be dropped
		if (m_Pending && m_ForceFresh.Contains(m_FileKey))
		{
			return false;
		}

		return m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_MESSAGES);
	}

	protected void AddMessage(VPPXEMessageRow copyOf)
	{
		VPPXEMsgFile file = CurrentFile();
		if (!file || !CanEdit())
		{
			return;
		}

		VPPXEMsgWork work = new VPPXEMsgWork();
		if (copyOf)
		{
			work.Row.CopyFrom(copyOf);
		}
		else
		{
			work.Row.OnConnect = 1;
			work.Row.Delay = NEW_DELAY;
			work.Row.Text = string.Format(Tr("#VSTR_XMLE_MSG_NEW_TEXT"), "#name");
		}

		work.Row.Line = 0;
		file.Work.Insert(work);
		m_SelWork = file.Work.Count() - 1;
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// A message that came from the file is marked deleted (REVERT brings it back); one added here is dropped.
	protected void DeleteSelected()
	{
		VPPXEMsgFile file = CurrentFile();
		VPPXEMsgWork work = SelectedWork();
		if (!file || !work || !CanEdit())
		{
			return;
		}

		if (work.OrigIndex < 0)
		{
			file.Work.RemoveOrdered(m_SelWork);
		}
		else
		{
			work.Deleted = true;
		}

		m_SelWork = -1;
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// 0 on connect, 1 repeat, 2 countdown, 3 scheduled time. A mode's minutes are set by its field (always above 0);
	// turning a mode off clears its value, shut down goes with the last of countdown and time.
	protected void ToggleMode(int modeIdx)
	{
		VPPXEMsgWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		VPPXEMessageRow row = work.Row;
		if (modeIdx == 0)
		{
			if (row.OnConnect == 1)
			{
				row.OnConnect = 0;
				row.Delay = 0;
			}
			else
			{
				row.OnConnect = 1;
			}
		}
		else if (modeIdx == 1)
		{
			if (row.Repeat > 0)
			{
				row.Repeat = 0;
			}
			else
			{
				row.Repeat = NEW_REPEAT;
			}
		}
		else if (modeIdx == 2)
		{
			if (row.Deadline > 0)
			{
				row.Deadline = 0;
			}
			else
			{
				row.Deadline = NEW_DEADLINE;
			}
		}
		else
		{
			if (row.Time != "")
			{
				row.Time = "";
			}
			else
			{
				row.Time = NEW_TIME;
			}
		}

		if (row.Deadline <= 0 && row.Time == "")
		{
			row.Shutdown = 0;
		}

		work.InputError = "";
		LoadForm();
		RefreshSelectedRow();
		UpdateActionBar();
	}

	protected void ToggleShutdown()
	{
		VPPXEMsgWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		if (work.Row.Shutdown == 1)
		{
			work.Row.Shutdown = 0;
		}
		else if (work.Row.Deadline > 0 || work.Row.Time != "")
		{
			work.Row.Shutdown = 1;
		}

		UpdateDetails();
		RefreshSelectedRow();
		UpdateActionBar();
	}

	protected void InsertToken(string token)
	{
		if (!SelectedWork() || !CanEdit())
		{
			return;
		}

		m_InputText.SetText(DisplayText(ReadText(m_InputText) + TokenSeparator(ReadText(m_InputText)) + token));
		PollInputs();
	}

	// Fills the form from the selected message (or shows the empty state).
	protected void LoadForm()
	{
		VPPXEMsgWork work = SelectedWork();
		ShowEditor(work != null);
		if (!work)
		{
			UpdateEmptyText();
			return;
		}

		m_Loading = true;
		VPPXEMessageRow row = work.Row;
		m_InputDelay.SetText(MinutesText(row.Delay));
		m_InputRepeat.SetText(MinutesText(row.Repeat));
		m_InputDeadline.SetText(MinutesText(row.Deadline));
		string timePart = row.Time;
		string datePart = "";
		int space = row.Time.IndexOf(" ");
		if (space >= 0)
		{
			timePart = row.Time.Substring(0, space);
			datePart = row.Time.Substring(space + 1, row.Time.Length() - space - 1);
			datePart = datePart.Trim();
		}

		m_InputTime.SetText(timePart);
		m_InputDate.SetText(datePart);
		m_InputText.SetText(DisplayText(row.Text));
		RememberInputs();
		m_Loading = false;
		UpdateDetails();
	}

	protected string MinutesText(int minutes)
	{
		if (minutes <= 0)
		{
			return "";
		}

		return minutes.ToString();
	}

	protected void RememberInputs()
	{
		m_LastDelay = m_InputDelay.GetText();
		m_LastRepeat = m_InputRepeat.GetText();
		m_LastDeadline = m_InputDeadline.GetText();
		m_LastTime = m_InputTime.GetText();
		m_LastDate = m_InputDate.GetText();
		m_LastText = m_InputText.GetText();
	}

	protected void PollInputs()
	{
		VPPXEMsgWork work = SelectedWork();
		if (!work || m_Loading || !m_InputText)
		{
			return;
		}

		bool changed = false;
		if (m_InputDelay.GetText() != m_LastDelay || m_InputRepeat.GetText() != m_LastRepeat || m_InputDeadline.GetText() != m_LastDeadline)
		{
			changed = true;
		}

		if (m_InputTime.GetText() != m_LastTime || m_InputDate.GetText() != m_LastDate || m_InputText.GetText() != m_LastText)
		{
			changed = true;
		}

		if (!changed)
		{
			return;
		}

		RememberInputs();
		if (!CanEdit())
		{
			LoadForm();
			return;
		}

		ApplyForm(work);
		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
	}

	// Form -> row. An unreadable field keeps the row's value and sets InputError (SAVE is refused while set).
	protected void ApplyForm(VPPXEMsgWork work)
	{
		VPPXEMessageRow row = work.Row;
		work.InputError = "";
		int minutes = 0;
		if (VPPXEMessageRules.ParseMinutes(m_InputDelay.GetText(), minutes))
		{
			row.Delay = minutes;
		}
		else
		{
			work.InputError = "#VSTR_XMLE_MSG_ERR_MINUTES";
		}

		if (row.Repeat > 0)
		{
			if (VPPXEMessageRules.ParseMinutes(m_InputRepeat.GetText(), minutes) && minutes > 0)
			{
				row.Repeat = minutes;
			}
			else
			{
				work.InputError = "#VSTR_XMLE_MSG_ERR_POSITIVE";
			}
		}

		if (row.Deadline > 0)
		{
			if (VPPXEMessageRules.ParseMinutes(m_InputDeadline.GetText(), minutes) && minutes > 0)
			{
				row.Deadline = minutes;
			}
			else
			{
				work.InputError = "#VSTR_XMLE_MSG_ERR_POSITIVE";
			}
		}

		if (row.Time != "")
		{
			string timeText = m_InputTime.GetText();
			string dateText = m_InputDate.GetText();
			timeText = timeText.Trim();
			dateText = dateText.Trim();
			if (timeText == "")
			{
				work.InputError = "#VSTR_XMLE_MSG_ERR_TIME";
			}
			else if (dateText == "")
			{
				row.Time = timeText;
			}
			else
			{
				row.Time = timeText + " " + dateText;
			}
		}

		string text = ReadText(m_InputText);
		row.Text = text.Trim();
	}

	// ---------------------------------------------------------------- save and revert

	protected void StartSave()
	{
		VPPXEMsgFile file = CurrentFile();
		if (!file || !CanEdit())
		{
			return;
		}

		int dirty = file.DirtyCount();
		if (dirty == 0)
		{
			return;
		}

		int workCount = file.Work.Count();
		for (int i = 0; i < workCount; i++)
		{
			VPPXEMsgWork work = file.Work[i];
			if (work.Deleted || !work.IsDirty())
			{
				continue;
			}

			string problem = work.InputError;
			if (problem == "")
			{
				problem = VPPXEMessageRules.CheckRow(work.Row);
			}

			if (problem != "")
			{
				m_SelWork = i;
				RebuildList();
				LoadForm();
				m_Owner.NotifyError(Tr(problem));
				return;
			}
		}

		string body = string.Format(Tr("#VSTR_XMLE_MSG_DLG_SAVE_BODY"), dirty, FileLabel(file.Key));
		m_Owner.OpenConfirm("#VSTR_XMLE_MSG_DLG_SAVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmSave", false);
	}

	void OnConfirmSave(int result, string input)
	{
		VPPXEMsgFile file = CurrentFile();
		if (result != DIAGRESULT.YES || !file || !CanEdit())
		{
			return;
		}

		array<ref VPPXEMessageEdit> edits = new array<ref VPPXEMessageEdit>;
		foreach (VPPXEMsgWork work : file.Work)
		{
			if (!work.IsDirty())
			{
				continue;
			}

			VPPXEMessageEdit edit = new VPPXEMessageEdit();
			if (work.OrigIndex < 0)
			{
				if (work.Deleted)
				{
					continue;
				}

				edit.Op = VPPXEOp.ADD;
				edit.Index = -1;
			}
			else if (work.Deleted)
			{
				edit.Op = VPPXEOp.DELETE;
				edit.Index = work.OrigIndex;
			}
			else
			{
				edit.Op = VPPXEOp.UPDATE;
				edit.Index = work.OrigIndex;
			}

			edit.Row.CopyFrom(work.Row);
			edits.Insert(edit);
		}

		int total = edits.Count();
		if (total == 0)
		{
			return;
		}

		// parts of at most EDITS_PER_PART edits and about CHUNK_BYTES each (texts can be 1151 bytes)
		m_SaveReqId = m_Owner.NextReqId();
		array<ref VPPXEMessagesSave> parts = new array<ref VPPXEMessagesSave>;
		VPPXEMessagesSave part = null;
		int partBytes = 0;
		foreach (VPPXEMessageEdit queued : edits)
		{
			int size = VPPXEMessageRules.EstimateRow(queued.Row) + 16;
			if (!part || part.Edits.Count() >= VPPXEMessageRules.EDITS_PER_PART || partBytes + size > VPPXEMessageRules.CHUNK_BYTES)
			{
				part = new VPPXEMessagesSave();
				part.ReqId = m_SaveReqId;
				part.PartIdx = parts.Count();
				part.FileKey = file.Key;
				part.BaseRevision = file.Revision;
				parts.Insert(part);
				partBytes = 0;
			}

			part.Edits.Insert(queued);
			partBytes += size;
		}

		m_Saving = true;
		m_SaveSentAt = NowMs();
		m_SaveFile = file.Key;
		int partCount = parts.Count();
		foreach (VPPXEMessagesSave sendPart : parts)
		{
			sendPart.PartCount = partCount;
			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveMessages", new Param1<ref VPPXEMessagesSave>(sendPart), true, null);
		}

		SetStatus(Tr("#VSTR_XMLE_STATUS_SAVING"), false);
		UpdateActionBar();
		UpdateDetails();
	}

	protected void StartRevert()
	{
		VPPXEMsgFile file = CurrentFile();
		if (!file || m_Saving)
		{
			return;
		}

		int dirty = file.DirtyCount();
		if (dirty == 0 && !file.ServerChanged)
		{
			return;
		}

		string body = string.Format(Tr("#VSTR_XMLE_MSG_DLG_REVERT_BODY"), dirty, FileLabel(file.Key));
		m_Owner.OpenConfirm("#VSTR_XMLE_MSG_DLG_REVERT_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRevert", false);
	}

	void OnConfirmRevert(int result, string input)
	{
		VPPXEMsgFile file = CurrentFile();
		if (result != DIAGRESULT.YES || !file)
		{
			return;
		}

		m_ForceFresh.Set(file.Key, true);
		m_SelWork = -1;
		RequestList();
	}

	// ---------------------------------------------------------------- list, dropdown, details

	protected void RebuildAll()
	{
		RebuildFileDropdown();
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	protected void RebuildFileDropdown()
	{
		if (!m_FileDD)
		{
			return;
		}

		m_FileDD.RemoveAllElements();
		string current = "";
		foreach (string fileKey : m_FileKeys)
		{
			string label = FileDropdownLabel(fileKey);
			m_FileDD.AddElement(label);
			if (fileKey == m_FileKey)
			{
				current = label;
			}
		}

		if (current == "")
		{
			current = Tr("#VSTR_XMLE_MSG_NO_FILE_SHORT");
		}

		m_FileDD.SetText(current);
	}

	protected string FileDropdownLabel(string fileKey)
	{
		string label = FileLabel(fileKey);
		VPPXEMsgFile file = m_Files.Get(fileKey);
		if (!file)
		{
			return label;
		}

		int shown = 0;
		foreach (VPPXEMsgWork work : file.Work)
		{
			if (!work.Deleted)
			{
				shown++;
			}
		}

		label = label + " (" + shown.ToString() + ")";
		if (file.DirtyCount() > 0)
		{
			label = label + " *";
		}

		return label;
	}

	protected void RebuildList()
	{
		if (!m_MsgList)
		{
			return;
		}

		m_MsgList.ClearItems();
		m_ListWork.Clear();
		m_ListSel = -1;
		VPPXEMsgFile file = CurrentFile();
		if (!file)
		{
			return;
		}

		int workCount = file.Work.Count();
		int keepRow = -1;
		for (int i = 0; i < workCount; i++)
		{
			VPPXEMsgWork work = file.Work[i];
			if (work.Deleted)
			{
				continue;
			}

			int row = m_MsgList.AddItem("", null, 0);
			m_ListWork.Insert(i);
			FillListRow(row, work);
			if (i == m_SelWork)
			{
				keepRow = row;
			}
		}

		if (keepRow >= 0)
		{
			m_MsgList.SelectRow(keepRow);
			m_ListSel = keepRow;
		}
		else
		{
			m_SelWork = -1;
		}
	}

	protected void FillListRow(int row, VPPXEMsgWork work)
	{
		int number = row + 1;
		string numberText = number.ToString();
		if (work.OrigIndex < 0)
		{
			numberText = numberText + "+";
		}
		else if (work.IsDirty())
		{
			numberText = numberText + "*";
		}

		string text = work.Row.Text;
		if (text.LengthUtf8() > 90)
		{
			text = text.SubstringUtf8(0, 90) + "...";
		}

		m_MsgList.SetItem(row, numberText, null, 0);
		m_MsgList.SetItem(row, WhenText(work.Row), null, 1);
		m_MsgList.SetItem(row, DisplayText(text), null, 2);
		int color = ARGB(255, 255, 255, 255);
		if (!VPPXEMessageRules.IsActive(work.Row))
		{
			color = ARGB(255, 120, 125, 130);
		}

		if (work.InputError != "" || VPPXEMessageRules.CheckRow(work.Row) != "")
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (work.IsDirty())
		{
			color = ARGB(255, 232, 163, 61);
		}

		for (int column = 0; column < 3; column++)
		{
			m_MsgList.SetItemColor(row, column, color);
		}
	}

	protected void RefreshSelectedRow()
	{
		VPPXEMsgWork work = SelectedWork();
		if (!work || m_ListSel < 0)
		{
			return;
		}

		FillListRow(m_ListSel, work);
		if (m_FileDD)
		{
			m_FileDD.SetText(FileDropdownLabel(m_FileKey));
		}
	}

	// Short "when" column: On connect +2m, every 15m, countdown 60m, 06:00 daily, shutdown.
	protected string WhenText(VPPXEMessageRow row)
	{
		array<string> parts = new array<string>;
		if (row.OnConnect == 1)
		{
			parts.Insert(string.Format(Tr("#VSTR_XMLE_MSG_TAG_CONNECT"), row.Delay));
		}

		if (row.Repeat > 0)
		{
			parts.Insert(string.Format(Tr("#VSTR_XMLE_MSG_TAG_REPEAT"), row.Repeat));
		}

		if (row.Deadline > 0)
		{
			parts.Insert(string.Format(Tr("#VSTR_XMLE_MSG_TAG_COUNTDOWN"), row.Deadline));
		}

		if (row.Time != "")
		{
			parts.Insert(row.Time);
		}

		if (row.Shutdown == 1 && (row.Deadline > 0 || row.Time != ""))
		{
			parts.Insert(Tr("#VSTR_XMLE_MSG_TAG_SHUTDOWN"));
		}

		if (parts.Count() == 0)
		{
			return Tr("#VSTR_XMLE_MSG_TAG_IGNORED");
		}

		string joined = parts[0];
		for (int i = 1; i < parts.Count(); i++)
		{
			joined = joined + ", " + parts[i];
		}

		return joined;
	}

	protected void PollListSelection()
	{
		if (!m_MsgList)
		{
			return;
		}

		int row = m_MsgList.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		m_ListSel = row;
		m_SelWork = -1;
		if (row >= 0 && row < m_ListWork.Count())
		{
			m_SelWork = m_ListWork[row];
		}

		LoadForm();
		UpdateActionBar();
	}

	// Title, enable states, mode looks, byte counter, preview and warnings of the selected message.
	protected void UpdateDetails()
	{
		VPPXEMsgWork work = SelectedWork();
		if (!work)
		{
			return;
		}

		VPPXEMessageRow row = work.Row;
		bool editable = CanEdit();
		if (work.OrigIndex < 0)
		{
			m_TxtEditTitle.SetText(Tr("#VSTR_XMLE_MSG_EDIT_NEW"));
			m_TxtLine.SetText("");
		}
		else
		{
			int number = m_ListSel + 1;
			m_TxtEditTitle.SetText(string.Format(Tr("#VSTR_XMLE_MSG_EDIT_TITLE"), number));
			m_TxtLine.SetText(string.Format(Tr("#VSTR_XMLE_MSG_LINE"), work.Orig.Line));
		}

		SetModeLook(0, row.OnConnect == 1, editable);
		SetModeLook(1, row.Repeat > 0, editable);
		SetModeLook(2, row.Deadline > 0, editable);
		SetModeLook(3, row.Time != "", editable);
		bool shutdownAllowed = row.Deadline > 0 || row.Time != "";
		SetChipLook(m_BtnShutdown, m_FillShutdown, m_TxtShutdown, row.Shutdown == 1, editable && shutdownAllowed, true);
		SetInputEnabled(m_InputDelay, m_LblDelay, editable && row.OnConnect == 1);
		SetInputEnabled(m_InputRepeat, m_LblRepeat, editable && row.Repeat > 0);
		SetInputEnabled(m_InputDeadline, m_LblDeadline, editable && row.Deadline > 0);
		SetInputEnabled(m_InputTime, m_LblTime, editable && row.Time != "");
		SetInputEnabled(m_InputDate, m_LblDate, editable && row.Time != "");
		SetInputEnabled(m_InputText, m_TxtTextTitle, editable);
		m_BtnPhName.Enable(editable);
		m_BtnPhPort.Enable(editable);
		m_BtnPhTmin.Enable(editable);

		int bytes = row.Text.Length();
		m_TxtBytes.SetText(string.Format(Tr("#VSTR_XMLE_MSG_BYTES"), bytes, VPPXEMessageRules.MAX_TEXT_BYTES));
		if (bytes > VPPXEMessageRules.MAX_TEXT_BYTES)
		{
			m_TxtBytes.SetColor(ARGB(255, 194, 69, 69));
		}
		else
		{
			m_TxtBytes.SetColor(ARGB(255, 154, 160, 166));
		}

		m_TxtPreview.SetText(DisplayText(BuildPreview(row)));
		m_TxtWarnings.SetText(BuildWarnings(work));
	}

	protected void SetModeLook(int modeIdx, bool isOn, bool enabled)
	{
		if (modeIdx < 0 || modeIdx >= m_BtnMode.Count())
		{
			return;
		}

		ButtonWidget modeButton = m_BtnMode[modeIdx];
		Widget modeFill = m_FillMode[modeIdx];
		TextWidget modeText = m_TxtMode[modeIdx];
		SetChipLook(modeButton, modeFill, modeText, isOn, enabled, false);
	}

	protected void SetChipLook(ButtonWidget button, Widget fill, TextWidget label, bool isOn, bool enabled, bool danger)
	{
		int fillColor = ARGB(255, 27, 30, 34);
		int textColor = ARGB(255, 154, 160, 166);
		if (isOn)
		{
			if (danger)
			{
				fillColor = ARGB(255, 92, 34, 34);
				textColor = ARGB(255, 240, 120, 120);
			}
			else
			{
				fillColor = ARGB(255, 74, 54, 22);
				textColor = ARGB(255, 232, 163, 61);
			}
		}

		if (!enabled)
		{
			textColor = ARGB(255, 92, 97, 102);
		}

		if (fill)
		{
			fill.SetColor(fillColor);
		}

		if (label)
		{
			label.SetColor(textColor);
		}

		if (button)
		{
			button.Enable(enabled);
		}
	}

	protected void SetInputEnabled(EditBoxWidget input, TextWidget label, bool enabled)
	{
		if (input)
		{
			input.Enable(enabled);
			if (enabled)
			{
				input.SetColor(ARGB(255, 255, 255, 255));
			}
			else
			{
				input.SetColor(ARGB(140, 255, 255, 255));
			}
		}

		if (label)
		{
			if (enabled)
			{
				label.SetColor(ARGB(255, 204, 208, 212));
			}
			else
			{
				label.SetColor(ARGB(255, 92, 97, 102));
			}
		}
	}

	// The chat line as players see it (#name, #port filled in; #tmin as the first countdown value), then what the
	// server does with the message.
	protected string BuildPreview(VPPXEMessageRow row)
	{
		string chat = row.Text;
		string hostName = GetGame().GetHostName();
		if (hostName == "")
		{
			hostName = Tr("#VSTR_XMLE_MSG_SERVER_NAME");
		}

		string address = "";
		int port = 0;
		string portText = "2302";
		if (GetGame().GetHostAddress(address, port))
		{
			portText = port.ToString();
		}

		chat.Replace("#name", hostName);
		chat.Replace("#port", portText);
		int firstMinutes = FirstCountdown(row);
		chat.Replace("#tmin", firstMinutes.ToString());
		string text = chat;
		if (!VPPXEMessageRules.IsActive(row))
		{
			text = text + "\n\n" + Tr("#VSTR_XMLE_MSG_SUM_IGNORED");
			return text;
		}

		text = text + "\n";
		if (row.OnConnect == 1)
		{
			if (row.Delay > 0)
			{
				text = text + "\n" + string.Format(Tr("#VSTR_XMLE_MSG_SUM_CONNECT"), row.Delay);
			}
			else
			{
				text = text + "\n" + Tr("#VSTR_XMLE_MSG_SUM_CONNECT_NOW");
			}
		}

		if (row.Repeat > 0)
		{
			text = text + "\n" + string.Format(Tr("#VSTR_XMLE_MSG_SUM_REPEAT"), row.Repeat);
		}

		if (row.Deadline > 0)
		{
			text = text + "\n" + string.Format(Tr("#VSTR_XMLE_MSG_SUM_COUNTDOWN"), StepsText(row.Deadline));
			if (row.Shutdown == 1)
			{
				text = text + " " + string.Format(Tr("#VSTR_XMLE_MSG_SUM_SHUTDOWN_AT"), row.Deadline);
			}
			else
			{
				text = text + " " + string.Format(Tr("#VSTR_XMLE_MSG_SUM_ENDS_AT"), row.Deadline);
			}
		}

		if (row.Time != "")
		{
			text = text + "\n" + TimeSummary(row);
		}

		return text;
	}

	protected string TimeSummary(VPPXEMessageRow row)
	{
		int hour = 0;
		int minute = 0;
		int second = 0;
		int day = 0;
		int month = 0;
		int year = 0;
		int dateParts = VPPXEMessageRules.ParseTime(row.Time, hour, minute, second, day, month, year);
		if (dateParts < 0)
		{
			return Tr("#VSTR_XMLE_MSG_ERR_TIME");
		}

		string clock = VPPXmlText.Pad2(hour) + ":" + VPPXmlText.Pad2(minute);
		if (second > 0)
		{
			clock = clock + ":" + VPPXmlText.Pad2(second);
		}

		string summary;
		if (dateParts == 0)
		{
			summary = string.Format(Tr("#VSTR_XMLE_MSG_SUM_DAILY"), clock);
		}
		else if (dateParts == 1)
		{
			summary = string.Format(Tr("#VSTR_XMLE_MSG_SUM_MONTHLY"), clock, day);
		}
		else if (dateParts == 2)
		{
			summary = string.Format(Tr("#VSTR_XMLE_MSG_SUM_YEARLY"), clock, VPPXmlText.Pad2(day) + "." + VPPXmlText.Pad2(month));
		}
		else
		{
			summary = string.Format(Tr("#VSTR_XMLE_MSG_SUM_ONCE"), clock, VPPXmlText.Pad2(day) + "." + VPPXmlText.Pad2(month) + "." + year.ToString());
		}

		if (row.Shutdown == 1)
		{
			summary = summary + " " + Tr("#VSTR_XMLE_MSG_SUM_TIME_SHUTDOWN");
		}

		return summary;
	}

	// The first announced minutes of a countdown: the largest step at or below its length (60 for a scheduled time).
	protected int FirstCountdown(VPPXEMessageRow row)
	{
		int length = row.Deadline;
		if (length <= 0)
		{
			length = 60;
		}

		array<int> steps = new array<int>;
		VPPXEMessageRules.CountdownSteps(steps);
		foreach (int step : steps)
		{
			if (step <= length)
			{
				return step;
			}
		}

		return length;
	}

	protected string StepsText(int deadline)
	{
		array<int> steps = new array<int>;
		VPPXEMessageRules.CountdownSteps(steps);
		string text = "";
		foreach (int step : steps)
		{
			if (step > deadline)
			{
				continue;
			}

			if (text != "")
			{
				text = text + ", ";
			}

			text = text + step.ToString();
		}

		return text;
	}

	// Every rule the server applies silently, as lines (the input error first).
	protected string BuildWarnings(VPPXEMsgWork work)
	{
		VPPXEMessageRow row = work.Row;
		array<string> lines = new array<string>;
		int hour = 0;
		int minute = 0;
		int second = 0;
		int day = 0;
		int month = 0;
		int year = 0;
		if (work.InputError != "")
		{
			lines.Insert(Tr(work.InputError));
		}

		string rule = VPPXEMessageRules.CheckRow(row);
		if (rule != "" && rule != work.InputError)
		{
			lines.Insert(Tr(rule));
		}

		if (row.Text != "" && row.OnConnect != 1 && row.Repeat <= 0 && row.Deadline <= 0 && row.Time == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_MSG_WARN_NO_MODE"));
		}

		if (row.Delay > 0 && row.OnConnect != 1)
		{
			lines.Insert(Tr("#VSTR_XMLE_MSG_WARN_DELAY"));
		}

		if (row.Shutdown == 1 && row.Deadline <= 0 && row.Time == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_MSG_WARN_SHUTDOWN"));
		}

		if (row.Text.IndexOf("#tmin") >= 0 && row.Deadline <= 0 && row.Time == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_MSG_WARN_TMIN"));
		}

		if (row.Time != "" && VPPXEMessageRules.ParseTime(row.Time, hour, minute, second, day, month, year) == 3)
		{
			lines.Insert(Tr("#VSTR_XMLE_MSG_WARN_ONCE"));
		}

		string text = "";
		foreach (string line : lines)
		{
			if (text != "")
			{
				text = text + "\n";
			}

			text = text + line;
		}

		return text;
	}

	protected void ShowEditor(bool show)
	{
		if (m_Form)
		{
			m_Form.Show(show);
		}

		if (m_TxtEmpty)
		{
			m_TxtEmpty.Show(!show);
		}

		if (!show && m_TxtEditTitle)
		{
			m_TxtEditTitle.SetText(Tr("#VSTR_XMLE_MSG_EDIT_NONE"));
			m_TxtLine.SetText("");
		}
	}

	protected void UpdateEmptyText()
	{
		if (!m_TxtEmpty)
		{
			return;
		}

		VPPXEMsgFile file = CurrentFile();
		if (m_FileKeys.Count() == 0 && m_Loaded)
		{
			m_TxtEmpty.SetText(Tr("#VSTR_XMLE_MSG_NO_FILE"));
		}
		else if (file && file.ErrorKey != "")
		{
			m_TxtEmpty.SetText(Tr(file.ErrorKey));
		}
		else
		{
			m_TxtEmpty.SetText(Tr("#VSTR_XMLE_MSG_EMPTY"));
		}
	}

	// Status line: request state, read-only reasons, server-side changes, else the unsaved count.
	protected void UpdateActionBar()
	{
		VPPXEMsgFile file = CurrentFile();
		int dirty = 0;
		if (file)
		{
			dirty = file.DirtyCount();
		}

		bool editable = CanEdit();
		m_BtnAdd.Enable(editable);
		m_BtnDup.Enable(editable && SelectedWork() != null);
		m_BtnDelete.Enable(editable && SelectedWork() != null);
		m_BtnSave.Enable(editable && dirty > 0);
		bool revertable = dirty > 0;
		if (file && file.ServerChanged)
		{
			revertable = true;
		}

		m_BtnRevert.Enable(revertable && !m_Saving);
		string text = m_StatusText;
		int color = ARGB(255, 154, 160, 166);
		if (m_StatusError)
		{
			color = ARGB(255, 194, 69, 69);
		}

		if (text == "")
		{
			if (file && file.ServerChanged)
			{
				text = Tr("#VSTR_XMLE_MSG_SERVER_CHANGED");
				color = ARGB(255, 217, 178, 61);
			}
			else if (m_Owner && !m_Owner.HasPerm(VPPXEPerm.EDIT_MESSAGES))
			{
				text = Tr("#VSTR_XMLE_MSG_READONLY");
			}
			else if (file && !file.Editable)
			{
				text = Tr("#VSTR_XMLE_MSG_FILE_READONLY");
			}
			else if (dirty > 0)
			{
				text = string.Format(Tr("#VSTR_XMLE_DIRTY_FMT"), dirty);
				color = ARGB(255, 232, 163, 61);
			}
			else
			{
				text = Tr("#VSTR_XMLE_DIRTY_NONE");
			}
		}

		m_TxtStatus.SetText(text);
		m_TxtStatus.SetColor(color);
	}

	protected void SetStatus(string text, bool isError)
	{
		m_StatusText = text;
		m_StatusError = isError;
		UpdateActionBar();
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

	// Left: header (title + file dropdown), list, footer buttons.
	protected void LayoutLeft()
	{
		if (m_LeftW < 60 || m_LeftH < 100)
		{
			return;
		}

		float hostW = Math.Clamp(m_LeftW * 0.55, 150, 320);
		m_FileHost.SetPos(m_LeftW - hostW - 4, 2);
		m_FileHost.SetSize(hostW, 24);
		float footerY = m_LeftH - FOOTER_H;
		float listH = footerY - 32 - 4;
		if (listH < 40)
		{
			listH = 40;
		}

		m_MsgList.SetPos(6, 32);
		m_MsgList.SetSize(m_LeftW - 12, listH);
		float buttonW = (m_LeftW - 12 - 12) / 3;
		m_BtnAdd.SetPos(6, footerY + 3);
		m_BtnAdd.SetSize(buttonW, 30);
		m_BtnDup.SetPos(6 + buttonW + 6, footerY + 3);
		m_BtnDup.SetSize(buttonW, 30);
		m_BtnDelete.SetPos(6 + 2 * (buttonW + 6), footerY + 3);
		m_BtnDelete.SetSize(buttonW, 30);
	}

	// Right: header, the form rows (see the constants), the preview card, the warnings, the action bar at the bottom.
	protected void LayoutRight()
	{
		if (m_RightW < 200 || m_RightH < 200)
		{
			return;
		}

		float innerW = m_RightW - 20;
		float formH = m_RightH - 32 - BAR_H - 4;
		m_Form.SetPos(0, 32);
		m_Form.SetSize(m_RightW, formH);
		m_TxtEmpty.SetPos(20, 60);
		m_TxtEmpty.SetSize(m_RightW - 40, 80);
		m_TxtWhen.SetPos(10, 0);
		m_TxtWhen.SetSize(innerW, 20);
		float modeW = (innerW - 18) / MODE_COUNT;
		for (int i = 0; i < m_BtnMode.Count(); i++)
		{
			ButtonWidget modeButton = m_BtnMode[i];
			if (modeButton)
			{
				modeButton.SetPos(10 + i * (modeW + 6), 24);
				modeButton.SetSize(modeW, ROW_H);
			}
		}

		float inputX = 10 + LABEL_W + 4;
		float unitX = inputX + INPUT_W + 6;
		PlaceRow(m_LblDelay, m_InputDelay, m_TxtDelayUnit, 62, inputX, unitX);
		PlaceRow(m_LblRepeat, m_InputRepeat, m_TxtRepeatUnit, 94, inputX, unitX);
		PlaceRow(m_LblDeadline, m_InputDeadline, m_TxtDeadlineUnit, 126, inputX, unitX);
		float shutdownX = unitX + 50;
		float shutdownW = m_RightW - 10 - shutdownX;
		if (shutdownW < 120)
		{
			shutdownW = 120;
		}

		m_BtnShutdown.SetPos(shutdownX, 126);
		m_BtnShutdown.SetSize(shutdownW, ROW_H);
		m_LblTime.SetPos(10, 158);
		m_LblTime.SetSize(LABEL_W, ROW_H);
		m_InputTime.SetPos(inputX, 158);
		m_InputTime.SetSize(INPUT_W, ROW_H);
		m_LblDate.SetPos(unitX + 10, 158);
		m_LblDate.SetSize(LABEL_W, ROW_H);
		m_InputDate.SetPos(unitX + 10 + LABEL_W + 4, 158);
		m_InputDate.SetSize(INPUT_W + 30, ROW_H);
		m_TxtTextTitle.SetPos(10, 196);
		m_TxtTextTitle.SetSize(innerW * 0.5, 20);
		m_TxtBytes.SetPos(10 + innerW * 0.5, 196);
		m_TxtBytes.SetSize(innerW * 0.5, 20);
		m_InputText.SetPos(10, 218);
		m_InputText.SetSize(innerW, ROW_H);
		m_TxtInsert.SetPos(10, 252);
		m_TxtInsert.SetSize(80, 26);
		m_BtnPhName.SetPos(94, 252);
		m_BtnPhName.SetSize(80, 26);
		m_BtnPhPort.SetPos(180, 252);
		m_BtnPhPort.SetSize(80, 26);
		m_BtnPhTmin.SetPos(266, 252);
		m_BtnPhTmin.SetSize(80, 26);
		m_TxtPreviewTitle.SetPos(10, 288);
		m_TxtPreviewTitle.SetSize(innerW, 20);
		float previewH = 110;
		m_PreviewCard.SetPos(10, 310);
		m_PreviewCard.SetSize(innerW, previewH);
		m_TxtPreview.SetPos(8, 6);
		m_TxtPreview.SetSize(innerW - 16, previewH - 12);
		float warnY = 310 + previewH + 8;
		float warnH = formH - warnY - 4;
		if (warnH < 20)
		{
			warnH = 20;
		}

		m_TxtWarnings.SetPos(10, warnY);
		m_TxtWarnings.SetSize(innerW, warnH);
		m_ActionBar.SetPos(0, m_RightH - BAR_H);
		m_ActionBar.SetSize(m_RightW, BAR_H);
	}

	protected void PlaceRow(TextWidget label, EditBoxWidget input, TextWidget unit, float y, float inputX, float unitX)
	{
		label.SetPos(10, y);
		label.SetSize(LABEL_W, ROW_H);
		input.SetPos(inputX, y);
		input.SetSize(INPUT_W, ROW_H);
		unit.SetPos(unitX, y);
		unit.SetSize(44, ROW_H);
	}

	// ---------------------------------------------------------------- helpers

	// Writes a sample into the text box in each encoding and keeps the first one that reads back unchanged; when none
	// does, "#" is shown as a full-width sign and turned back into "#" when the box is read.
	protected void ProbeHashMode()
	{
		string sample = "a #name b";
		m_HashMode = HASH_FULLWIDTH;
		if (!m_InputText)
		{
			return;
		}

		for (int mode = HASH_RAW; mode < HASH_FULLWIDTH; mode++)
		{
			m_InputText.SetText(EncodeHash(sample, mode));
			string readBack = m_InputText.GetText();
			if (readBack == sample)
			{
				m_HashMode = mode;
				break;
			}
		}

		m_InputText.SetText("");
		Print("[XMLEditor] MESSAGES tab: '#' is shown with encoding " + m_HashMode.ToString() + " (0 raw, 1 ##, 2 backslash, 3 full-width)");
	}

	protected string EncodeHash(string text, int mode)
	{
		if (text.IndexOf("#") < 0 || mode == HASH_RAW)
		{
			return text;
		}

		string encoded = text;
		if (mode == HASH_DOUBLED)
		{
			encoded.Replace("#", "##");
		}
		else if (mode == HASH_BACKSLASH)
		{
			encoded.Replace("#", "\\#");
		}
		else
		{
			encoded.Replace("#", FULLWIDTH_HASH);
		}

		return encoded;
	}

	// Text for a widget, with every "#" kept.
	protected string DisplayText(string text)
	{
		return EncodeHash(text, m_HashMode);
	}

	// The text box content as file text (the full-width sign back to "#").
	protected string ReadText(EditBoxWidget box)
	{
		string text = box.GetText();
		if (m_HashMode == HASH_FULLWIDTH && text.IndexOf(FULLWIDTH_HASH) >= 0)
		{
			text.Replace(FULLWIDTH_HASH, "#");
		}

		return text;
	}

	protected string TokenSeparator(string text)
	{
		if (text == "" || text.Get(text.Length() - 1) == " ")
		{
			return "";
		}

		return " ";
	}

	protected void SetTokenLabel(string widgetName, string token)
	{
		TextWidget label = TextWidget.Cast(m_Root.FindAnyWidget(widgetName));
		if (label)
		{
			label.SetText(DisplayText(token));
		}
	}

	protected VPPXEMsgFile CurrentFile()
	{
		if (m_FileKey == "")
		{
			return null;
		}

		return m_Files.Get(m_FileKey);
	}

	protected VPPXEMsgWork SelectedWork()
	{
		VPPXEMsgFile file = CurrentFile();
		if (!file || m_SelWork < 0 || m_SelWork >= file.Work.Count())
		{
			return null;
		}

		VPPXEMsgWork work = file.Work[m_SelWork];
		if (work.Deleted)
		{
			return null;
		}

		return work;
	}

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
