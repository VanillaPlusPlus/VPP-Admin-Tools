/*
	TEMPLATE view of the webhooks menu: the body template of one event of the selected webhook.

	The text is fetched from the server (GetTemplate), edited in a multi-line box (shown with "＃" for "#", see
	VPPWebhookUi) and checked by the server: VALIDATE renders it with sample values and returns the problems, the JSON
	verdict of the game's own parser and the service limits; SAVE does the same and writes it when it has no errors;
	TEST sends the saved template for real; RESET puts the preset back. The insert list puts variables, filters and
	blocks at the text cursor.
*/
class VPPWebhookTemplateView : Managed
{
	const static int POLL_MS = 400;
	// editor metrics (Metron16): the editor is sized to its text so the scroller can show all of it
	const static int EDIT_LINE_H = 20;
	const static int EDIT_CHAR_W = 10;
	// result / preview text (sdf_MetronLight72, 13 px)
	const static int RES_LINE_H = 18;
	const static int RES_CHAR_W = 8;

	protected MenuWebHooks m_Owner;
	protected Widget m_Root;
	protected Widget m_EventHost;
	protected ref VPPDropDownMenu m_EventDrop;
	protected TextWidget m_TxtState;
	protected ButtonWidget m_BtnReset;
	protected Widget m_EditorCard;
	protected ScrollWidget m_Scroll;
	protected MultilineEditBoxWidget m_Editor;
	protected Widget m_Side;
	protected TextListboxWidget m_InsertList;
	protected Widget m_SideHint;
	protected Widget m_ResultTitle;
	protected Widget m_ResultCard;
	protected ScrollWidget m_ResultScroll;
	protected MultilineTextWidget m_TxtResult;
	protected Widget m_PreviewTitle;
	protected Widget m_PreviewCard;
	protected ScrollWidget m_PreviewScroll;
	protected MultilineTextWidget m_TxtPreview;
	protected ButtonWidget m_BtnValidate;
	protected ButtonWidget m_BtnSave;
	protected ButtonWidget m_BtnTest;
	protected TextWidget m_TxtStatus;

	protected ref array<string> m_EventIds;
	protected ref array<string> m_InsertTexts;
	protected string m_HookId;
	protected string m_EventId;
	protected string m_LoadedText;
	protected bool m_HasText;
	protected int m_LoadReq;
	protected int m_CheckReq;
	protected int m_TestReq;
	protected bool m_CheckIsSave;
	protected string m_CheckText;
	protected bool m_Dirty;
	protected int m_NextPollAt;
	protected string m_PendingEvent;
	protected int m_LastLines;
	protected int m_LastLongest;
	protected float m_EditorViewW;
	protected float m_EditorViewH;
	protected float m_ResultViewW;
	protected float m_PreviewViewW;

	void VPPWebhookTemplateView(MenuWebHooks owner, Widget root)
	{
		m_Owner = owner;
		m_Root = root;
		m_EventIds = new array<string>();
		m_InsertTexts = new array<string>();
		m_EventHost = root.FindAnyWidget("TplEventHost");
		m_TxtState = TextWidget.Cast(root.FindAnyWidget("TxtTplState"));
		m_BtnReset = ButtonWidget.Cast(root.FindAnyWidget("BtnTplReset"));
		m_EditorCard = root.FindAnyWidget("TplEditorCard");
		m_Scroll = ScrollWidget.Cast(root.FindAnyWidget("TplScroll"));
		m_Editor = MultilineEditBoxWidget.Cast(root.FindAnyWidget("TplEditor"));
		m_Side = root.FindAnyWidget("TplSide");
		m_InsertList = TextListboxWidget.Cast(root.FindAnyWidget("TplInsertList"));
		m_SideHint = root.FindAnyWidget("TxtTplSideHint");
		m_ResultTitle = root.FindAnyWidget("TxtTplResultTitle");
		m_ResultCard = root.FindAnyWidget("TplResultCard");
		m_ResultScroll = ScrollWidget.Cast(root.FindAnyWidget("TplResultScroll"));
		m_TxtResult = MultilineTextWidget.Cast(root.FindAnyWidget("TxtTplResult"));
		m_PreviewTitle = root.FindAnyWidget("TxtTplPreviewTitle");
		m_PreviewCard = root.FindAnyWidget("TplPreviewCard");
		m_PreviewScroll = ScrollWidget.Cast(root.FindAnyWidget("TplPreviewScroll"));
		m_TxtPreview = MultilineTextWidget.Cast(root.FindAnyWidget("TxtTplPreview"));
		m_BtnValidate = ButtonWidget.Cast(root.FindAnyWidget("BtnTplValidate"));
		m_BtnSave = ButtonWidget.Cast(root.FindAnyWidget("BtnTplSave"));
		m_BtnTest = ButtonWidget.Cast(root.FindAnyWidget("BtnTplTest"));
		m_TxtStatus = TextWidget.Cast(root.FindAnyWidget("TxtTplStatus"));
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			m_EventIds.Insert(def.Id);
		}

		m_EventId = m_EventIds[0];
		string firstLabel = VPPWebhookUi.EventLabel(m_EventId);
		m_EventDrop = new VPPDropDownMenu(m_EventHost, firstLabel);
		foreach (string eventId : m_EventIds)
		{
			string eventLabel = VPPWebhookUi.EventLabel(eventId);
			m_EventDrop.AddElement(eventLabel);
		}

		m_EventDrop.SetIndex(0);
		m_EventDrop.m_OnSelectItem.Insert(OnEventSelected);
		m_HookId = "";
		m_LoadedText = "";
	}

	// ---------------------------------------------------------------- state

	protected bool CanEdit()
	{
		return m_Owner.CanDo(VPPWebhookAccess.EDIT_TEMPLATES) && m_Owner.HasWork() && !m_Owner.IsWorkNew();
	}

	bool IsDirty()
	{
		return m_Dirty;
	}

	// The selected webhook changed (or was saved / cleared): its template is fetched again when the view is shown.
	void OnWorkChanged()
	{
		string workId = m_Owner.GetWorkId();
		if (workId != m_HookId)
		{
			m_HookId = workId;
			m_LoadReq = 0;
			m_HasText = false;
			m_Dirty = false;
			m_LoadedText = "";
			SetEditorText("");
			SetResult("", "");
		}

		RebuildInsertList();
		if (m_Root.IsVisible())
		{
			OnShown();
		}

		UpdateEnabled();
	}

	void OnShown()
	{
		if (!m_Owner.HasWork())
		{
			return;
		}

		if (m_Owner.IsWorkNew())
		{
			SetStatus("#VSTR_WH_TPL_SAVE_HOOK_FIRST", VPPWebhookUi.CLR_ORANGE);
			UpdateEnabled();
			return;
		}

		if (!m_Owner.CanDo(VPPWebhookAccess.EDIT_TEMPLATES))
		{
			SetStatus("#VSTR_WH_TPL_NO_PERMISSION", VPPWebhookUi.CLR_ORANGE);
			UpdateEnabled();
			return;
		}

		if (!m_HasText && m_LoadReq == 0)
		{
			RequestTemplate();
		}

		UpdateEnabled();
	}

	void UpdateEnabled()
	{
		bool canEdit = CanEdit() && m_HasText;
		m_Editor.Enable(canEdit);
		m_BtnValidate.Enable(canEdit);
		m_BtnSave.Enable(canEdit && m_Dirty);
		m_BtnReset.Enable(canEdit);
		bool canTest = canEdit && !m_Dirty && m_Owner.CanDo(VPPWebhookAccess.TEST_SEND);
		m_BtnTest.Enable(canTest);
		UpdateStateText();
	}

	protected void UpdateStateText()
	{
		string key = "";
		int color = VPPWebhookUi.CLR_MUTED;
		if (m_LoadReq != 0)
		{
			key = "#VSTR_WH_TPL_LOADING";
		}
		else if (m_HasText && m_Dirty)
		{
			key = "#VSTR_WH_TPL_MODIFIED";
			color = VPPWebhookUi.CLR_ORANGE;
		}
		else if (m_HasText)
		{
			key = "#VSTR_WH_TPL_SAVED_STATE";
		}

		string stateText = "";
		if (key != "")
		{
			stateText = VPPWebhookUi.Tr(key);
		}

		m_TxtState.SetText(stateText);
		m_TxtState.SetColor(color);
	}

	protected void SetStatus(string key, int color)
	{
		string text = "";
		if (key != "")
		{
			text = VPPWebhookUi.Tr(key);
		}

		m_TxtStatus.SetText(text);
		m_TxtStatus.SetColor(color);
	}

	// ---------------------------------------------------------------- event selection

	void SelectEvent(string eventId)
	{
		int index = m_EventIds.Find(eventId);
		if (index < 0)
		{
			return;
		}

		ChangeEvent(index);
	}

	void OnEventSelected(int index)
	{
		m_EventDrop.Close();
		ChangeEvent(index);
	}

	protected void ChangeEvent(int index)
	{
		if (index < 0 || index >= m_EventIds.Count())
		{
			return;
		}

		string eventId = m_EventIds[index];
		if (eventId == m_EventId)
		{
			ShowEventInDrop();
			return;
		}

		if (m_Dirty)
		{
			m_PendingEvent = eventId;
			ShowEventInDrop();
			m_Owner.Confirm("TPL_DISCARD_EVENT", "#VSTR_WH_DLG_DISCARD_TITLE", "#VSTR_WH_DLG_DISCARD_TPL");
			return;
		}

		ApplyEvent(eventId);
	}

	protected void ApplyEvent(string eventId)
	{
		m_EventId = eventId;
		m_LoadReq = 0;
		m_HasText = false;
		m_Dirty = false;
		m_LoadedText = "";
		SetEditorText("");
		SetResult("", "");
		ShowEventInDrop();
		RebuildInsertList();
		OnShown();
	}

	protected void ShowEventInDrop()
	{
		int index = m_EventIds.Find(m_EventId);
		if (index < 0)
		{
			return;
		}

		string label = VPPWebhookUi.EventLabel(m_EventId);
		m_EventDrop.SetIndex(index);
		m_EventDrop.SetText(label);
	}

	void OnDialogYes(string action)
	{
		if (action == "TPL_DISCARD_EVENT")
		{
			m_Dirty = false;
			ApplyEvent(m_PendingEvent);
			m_PendingEvent = "";
			return;
		}

		if (action == "TPL_RESET")
		{
			SendReset();
		}
	}

	void OnDialogNo(string action)
	{
		if (action == "TPL_DISCARD_EVENT")
		{
			m_PendingEvent = "";
		}
	}

	// ---------------------------------------------------------------- server round trips

	protected void RequestTemplate()
	{
		if (m_HookId == "")
		{
			return;
		}

		m_LoadReq = m_Owner.NextReq();
		UpdateStateText();
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "GetTemplate", new Param3<int, string, string>(m_LoadReq, m_HookId, m_EventId), true);
	}

	void OnTemplate(VPPWebhookTemplateReply reply)
	{
		if (reply.HookId != m_HookId || reply.EventId != m_EventId)
		{
			return;
		}

		m_LoadReq = 0;
		m_LoadedText = reply.Text;
		m_HasText = true;
		m_Dirty = false;
		SetEditorText(reply.Text);
		SetResult("", "");
		SetStatus("", VPPWebhookUi.CLR_MUTED);
		UpdateEnabled();
	}

	protected void SendCheck(bool save)
	{
		if (!CanEdit() || !m_HasText)
		{
			return;
		}

		string text = GetEditorText();
		if (text.Length() > VPPWebhookTemplate.MAX_LENGTH)
		{
			SetStatus("#VSTR_WH_TPL_TOO_LONG", VPPWebhookUi.CLR_FAIL);
			return;
		}

		m_CheckReq = m_Owner.NextReq();
		m_CheckIsSave = save;
		m_CheckText = text;
		string rpcName = "ValidateTemplate";
		if (save)
		{
			rpcName = "SaveTemplate";
		}

		SetStatus("#VSTR_WH_TPL_CHECKING", VPPWebhookUi.CLR_MUTED);
		GetRPCManager().VSendRPC("RPC_WebHooksManager", rpcName, new Param4<int, string, string, string>(m_CheckReq, m_HookId, m_EventId, text), true);
	}

	void OnCheck(VPPWebhookCheckReply reply)
	{
		if (reply.ReqId != m_CheckReq || reply.HookId != m_HookId || reply.EventId != m_EventId)
		{
			return;
		}

		string result = BuildResultText(reply);
		string preview = "";
		if (reply.TemplateOk)
		{
			preview = VPPWebhookUi.TextToDisplay(reply.Preview);
		}

		SetResult(result, preview);
		if (reply.Saved)
		{
			m_LoadedText = m_CheckText;
			m_Dirty = GetEditorText() != m_LoadedText;
			SetStatus("#VSTR_WH_TPL_SAVED", VPPWebhookUi.CLR_OK);
		}
		else if (m_CheckIsSave)
		{
			SetStatus("#VSTR_WH_TPL_NOT_SAVED", VPPWebhookUi.CLR_FAIL);
		}
		else if (reply.TemplateOk && (!reply.JsonChecked || reply.JsonValid) && reply.ServiceProblems.Count() == 0)
		{
			SetStatus("#VSTR_WH_TPL_VALID", VPPWebhookUi.CLR_OK);
		}
		else
		{
			SetStatus("#VSTR_WH_TPL_PROBLEMS", VPPWebhookUi.CLR_FAIL);
		}

		UpdateEnabled();
	}

	// Template problems, then the JSON verdict of the game's parser and the service limits, one per line.
	protected string BuildResultText(VPPWebhookCheckReply reply)
	{
		string text = "";
		string line = "";
		string lineText = "";
		string colText = "";
		string format = "";
		foreach (VPPWebhookIssueWire issue : reply.Issues)
		{
			line = VPPWebhookUi.IssueText(issue);
			text = text + line + "\n";
		}

		if (reply.Issues.Count() == 0)
		{
			line = VPPWebhookUi.Tr("#VSTR_WH_RES_TPL_OK");
			text = text + line + "\n";
		}

		if (!reply.TemplateOk)
		{
			return VPPWebhookUi.TextToDisplay(text);
		}

		if (reply.JsonChecked && reply.JsonValid)
		{
			line = VPPWebhookUi.Tr("#VSTR_WH_RES_JSON_OK");
			text = text + line + "\n";
		}
		else if (reply.JsonChecked)
		{
			lineText = reply.JsonLine.ToString();
			colText = reply.JsonCol.ToString();
			format = VPPWebhookUi.Tr("#VSTR_WH_RES_JSON_BAD");
			line = string.Format(format, lineText, colText, reply.JsonMessage);
			text = text + line + "\n";
		}

		foreach (string problem : reply.ServiceProblems)
		{
			format = VPPWebhookUi.Tr("#VSTR_WH_RES_SERVICE");
			line = string.Format(format, problem);
			text = text + line + "\n";
		}

		return VPPWebhookUi.TextToDisplay(text);
	}

	protected void SendTest()
	{
		if (!CanEdit() || m_Dirty || !m_Owner.CanDo(VPPWebhookAccess.TEST_SEND))
		{
			return;
		}

		m_TestReq = m_Owner.NextReq();
		SetStatus("#VSTR_WH_TPL_TESTING", VPPWebhookUi.CLR_MUTED);
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "TestSend", new Param3<int, string, string>(m_TestReq, m_HookId, m_EventId), true);
	}

	void OnTestResult(VPPWebhookTestReply reply)
	{
		if (reply.ReqId != m_TestReq)
		{
			return;
		}

		string format = VPPWebhookUi.Tr("#VSTR_WH_TPL_TEST_FAILED");
		int color = VPPWebhookUi.CLR_FAIL;
		if (reply.Ok)
		{
			format = VPPWebhookUi.Tr("#VSTR_WH_TPL_TEST_OK");
			color = VPPWebhookUi.CLR_OK;
		}

		string status = string.Format(format, reply.Reason);
		m_TxtStatus.SetText(status);
		m_TxtStatus.SetColor(color);
	}

	protected void SendReset()
	{
		if (!CanEdit())
		{
			return;
		}

		m_LoadReq = m_Owner.NextReq();
		UpdateStateText();
		GetRPCManager().VSendRPC("RPC_WebHooksManager", "ResetTemplate", new Param3<int, string, string>(m_LoadReq, m_HookId, m_EventId), true);
	}

	// ---------------------------------------------------------------- editor text

	// Lines are set one by one with "＃" for "#" (see VPPWebhookUi).
	protected void SetEditorText(string text)
	{
		array<string> lines = new array<string>();
		string shown = "";
		int total = 0;
		int i = 0;
		VPPWebhookUi.SplitLines(text, lines);
		total = lines.Count();
		shown = VPPWebhookUi.ToDisplay(lines[0]);
		m_Editor.SetText(shown);
		for (i = 1; i < total; i++)
		{
			shown = VPPWebhookUi.ToDisplay(lines[i]);
			m_Editor.SetLine(i, shown);
		}

		FitEditor(true);
	}

	protected string GetEditorText()
	{
		string text = "";
		string line = "";
		string raw = "";
		int total = m_Editor.GetLinesCount();
		int i = 0;
		for (i = 0; i < total; i++)
		{
			line = "";
			m_Editor.GetLine(i, line);
			raw = VPPWebhookUi.FromDisplay(line);
			if (i > 0)
			{
				text = text + "\n";
			}

			text = text + raw;
		}

		return text;
	}

	// Puts a snippet at the text cursor (the line is rebuilt with SetLine).
	protected void InsertAtCursor(string snippet)
	{
		if (!CanEdit() || !m_HasText)
		{
			return;
		}

		int lineIndex = m_Editor.GetCarriageLine();
		int column = m_Editor.GetCarriagePos();
		int total = m_Editor.GetLinesCount();
		string current = "";
		string before = "";
		string after = "";
		string shownSnippet = VPPWebhookUi.ToDisplay(snippet);
		if (total == 0)
		{
			m_Editor.SetText(shownSnippet);
			CheckDirty();
			return;
		}

		if (lineIndex < 0 || lineIndex >= total)
		{
			lineIndex = total - 1;
		}

		m_Editor.GetLine(lineIndex, current);
		int length = current.Length();
		if (column < 0 || column > length)
		{
			column = length;
		}

		if (column > 0)
		{
			before = current.Substring(0, column);
		}

		if (column < length)
		{
			after = current.Substring(column, length - column);
		}

		m_Editor.SetLine(lineIndex, before + shownSnippet + after);
		FitEditor(false);
		CheckDirty();
	}

	protected void CheckDirty()
	{
		if (!m_HasText)
		{
			return;
		}

		string current = GetEditorText();
		bool dirty = current != m_LoadedText;
		if (dirty == m_Dirty)
		{
			return;
		}

		m_Dirty = dirty;
		UpdateEnabled();
	}

	// Variables of the event (+ the webhook's hook.* ones), then filters and blocks.
	protected void RebuildInsertList()
	{
		m_InsertList.ClearItems();
		m_InsertTexts.Clear();
		VPPWebhookEventDef def = VPPWebhookDefs.Get(m_EventId);
		if (def)
		{
			foreach (VPPWebhookVar webhookVar : def.Vars)
			{
				AddInsert("{{" + webhookVar.Name + "}}", "{{" + webhookVar.Name + "}}");
			}
		}

		WebHook work = m_Owner.GetWork();
		if (work && work.m_Vars)
		{
			int varTotal = work.m_Vars.Count();
			for (int i = 0; i < varTotal; i++)
			{
				string hookName = "hook." + work.m_Vars.GetKey(i);
				AddInsert("{{" + hookName + "}}", "{{" + hookName + "}}");
			}
		}

		AddInsert("if VAR ... /if", "{{#if VAR}}{{/if}}");
		AddInsert("if VAR ... else ... /if", "{{#if VAR}}{{else}}{{/if}}");
		AddInsert("unless VAR ... /unless", "{{#unless VAR}}{{/unless}}");
		AddInsert("ifeq VAR value ... /ifeq", "{{#ifeq VAR value}}{{/ifeq}}");
		AddInsert("{{! comment }}", "{{! comment }}");
		AddInsert("|upper", "|upper");
		AddInsert("|lower", "|lower");
		AddInsert("|trim", "|trim");
		AddInsert("|truncate:100", "|truncate:100");
		AddInsert("|default:none", "|default:none");
		AddInsert("|round:1", "|round:1");
		AddInsert("|number", "|number");
		AddInsert("|steamurl", "|steamurl");
		AddInsert("|json", "|json");
		AddInsert("|raw", "|raw");
		m_InsertList.SelectRow(-1);
	}

	// label: shown in the list (no "#": the listbox font is not the one proven with "＃"); snippet: inserted text.
	protected void AddInsert(string label, string snippet)
	{
		m_InsertList.AddItem(label, null, 0);
		m_InsertTexts.Insert(snippet);
	}

	protected void SetResult(string result, string preview)
	{
		string shownResult = result;
		string shownPreview = preview;
		if (shownResult == "")
		{
			shownResult = VPPWebhookUi.Tr("#VSTR_WH_TPL_RESULT_EMPTY");
		}

		if (shownPreview == "")
		{
			shownPreview = VPPWebhookUi.Tr("#VSTR_WH_TPL_PREVIEW_EMPTY");
		}

		m_TxtResult.SetText(shownResult);
		m_TxtPreview.SetText(shownPreview);
		SizeText(m_TxtResult, shownResult, m_ResultViewW, true);
		SizeText(m_TxtPreview, shownPreview, m_PreviewViewW, false);
		m_ResultScroll.VScrollToPos01(0);
		m_PreviewScroll.VScrollToPos01(0);
		m_PreviewScroll.HScrollToPos01(0);
	}

	// Height (and for unwrapped text, width) from the text itself, at least the visible width.
	protected void SizeText(Widget textWidget, string text, float viewW, bool wrapped)
	{
		array<string> lines = new array<string>();
		int rows = 0;
		int longest = 0;
		int length = 0;
		int perRow = 0;
		int total = 0;
		int i = 0;
		float width = viewW - 24;
		if (width < 100)
		{
			width = 100;
		}

		int widthPx = width;
		perRow = widthPx / RES_CHAR_W;
		if (perRow < 10)
		{
			perRow = 10;
		}

		VPPWebhookUi.SplitLines(text, lines);
		total = lines.Count();
		for (i = 0; i < total; i++)
		{
			length = lines[i].LengthUtf8();
			if (length > longest)
			{
				longest = length;
			}

			if (wrapped)
			{
				rows = rows + 1 + length / perRow;
			}
			else
			{
				rows = rows + 1;
			}
		}

		if (!wrapped && longest * RES_CHAR_W + 20 > width)
		{
			width = longest * RES_CHAR_W + 20;
		}

		int height = (rows + 1) * RES_LINE_H;
		textWidget.SetSize(width, height);
	}

	// ---------------------------------------------------------------- events

	bool OnClick(Widget w)
	{
		if (w == m_BtnValidate)
		{
			SendCheck(false);
			return true;
		}

		if (w == m_BtnSave)
		{
			SendCheck(true);
			return true;
		}

		if (w == m_BtnTest)
		{
			SendTest();
			return true;
		}

		if (w == m_BtnReset)
		{
			if (CanEdit())
			{
				m_Owner.Confirm("TPL_RESET", "#VSTR_WH_DLG_RESET_TITLE", "#VSTR_WH_DLG_RESET");
			}

			return true;
		}

		return false;
	}

	void OnUpdate()
	{
		if (!m_Root.IsVisible())
		{
			return;
		}

		int now = GetGame().GetTime();
		if (now < m_NextPollAt)
		{
			return;
		}

		m_NextPollAt = now + POLL_MS;
		CheckDirty();
		FitEditor(false);
	}

	bool OnDoubleClick(Widget w)
	{
		if (w != m_InsertList)
		{
			return false;
		}

		int row = m_InsertList.GetSelectedRow();
		if (row >= 0 && row < m_InsertTexts.Count())
		{
			string snippet = m_InsertTexts[row];
			InsertAtCursor(snippet);
		}

		return true;
	}

	// Sizes the editor to its lines (at least the visible area) when the line count or the longest line changed.
	protected void FitEditor(bool force)
	{
		int lines = m_Editor.GetLinesCount();
		int longest = 0;
		int i = 0;
		int length = 0;
		string line = "";
		for (i = 0; i < lines; i++)
		{
			line = "";
			m_Editor.GetLine(i, line);
			length = line.LengthUtf8();
			if (length > longest)
			{
				longest = length;
			}
		}

		if (!force && lines == m_LastLines && longest == m_LastLongest)
		{
			return;
		}

		m_LastLines = lines;
		m_LastLongest = longest;
		int textW = longest * EDIT_CHAR_W + 60;
		int textH = (lines + 2) * EDIT_LINE_H;
		float width = m_EditorViewW - 24;
		float height = m_EditorViewH - 12;
		if (textW > width)
		{
			width = textW;
		}

		if (textH > height)
		{
			height = textH;
		}

		m_Editor.SetSize(width, height);
	}

	void Relayout(float width, float height)
	{
		float sideW = 230;
		float editorW = width - sideW - 10;
		float resultsH = 214;
		float actionsY = height - 32;
		float resultsY = actionsY - 8 - resultsH;
		float titlesY = resultsY - 22;
		float editorH = titlesY - 4 - 34;
		float halfW = (width - 10) / 2;
		m_EditorCard.SetSize(editorW, editorH);
		m_Scroll.SetSize(editorW - 8, editorH - 8);
		m_EditorViewW = editorW - 8;
		m_EditorViewH = editorH - 8;
		m_ResultViewW = halfW - 8;
		m_PreviewViewW = halfW - 8;
		FitEditor(true);
		m_Side.SetPos(editorW + 10, 34);
		m_Side.SetSize(sideW, editorH);
		m_InsertList.SetSize(sideW - 8, editorH - 74);
		m_SideHint.SetPos(8, editorH - 40);
		m_ResultTitle.SetPos(0, titlesY);
		m_ResultCard.SetPos(0, resultsY);
		m_ResultCard.SetSize(halfW, resultsH);
		m_ResultScroll.SetSize(halfW - 8, resultsH - 8);

		m_PreviewTitle.SetPos(halfW + 10, titlesY);
		m_PreviewCard.SetPos(halfW + 10, resultsY);
		m_PreviewCard.SetSize(halfW, resultsH);
		m_PreviewScroll.SetSize(halfW - 8, resultsH - 8);

		m_BtnValidate.SetPos(0, actionsY);
		m_BtnSave.SetPos(148, actionsY);
		m_BtnTest.SetPos(296, actionsY);
		m_TxtStatus.SetPos(446, actionsY);
		m_TxtStatus.SetSize(width - 446, 30);
	}
};
