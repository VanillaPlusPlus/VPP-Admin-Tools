/*
	SETTINGS view of the webhooks menu: name, URL (masked for admins without MenuWebHooks:ViewURL: leave it to keep
	the stored one), enabled, preset + content type, rate limit, privacy switches and the custom hook.* variables.
	Edits go into the owner's work copy; a preset change asks first (saving rewrites the templates).
*/
class VPPWebhookSettingsView : Managed
{
	protected MenuWebHooks m_Owner;
	protected Widget m_Root;
	protected EditBoxWidget m_InputName;
	protected EditBoxWidget m_InputUrl;
	protected MultilineTextWidget m_TxtUrlHint;
	protected ref VPPWebhookToggle m_Enabled;
	protected Widget m_PresetHost;
	protected ref VPPDropDownMenu m_PresetDrop;
	protected ref array<string> m_PresetIds;
	protected EditBoxWidget m_InputContentType;
	protected EditBoxWidget m_InputRate;
	protected VPPXENumericEditHandler m_RateHandler;
	protected ref VPPWebhookToggle m_HideIds;
	protected ref VPPWebhookToggle m_HideAddr;
	protected TextListboxWidget m_VarList;
	protected EditBoxWidget m_InputVarName;
	protected EditBoxWidget m_InputVarValue;
	protected ButtonWidget m_BtnVarSet;
	protected ButtonWidget m_BtnVarRemove;

	protected ref array<string> m_VarNames;
	protected int m_VarSel;
	protected bool m_Editable;
	protected bool m_Loading;
	protected string m_PendingPreset;

	void VPPWebhookSettingsView(MenuWebHooks owner, Widget root)
	{
		m_Owner = owner;
		m_Root = root;
		m_PresetIds = new array<string>();
		m_VarNames = new array<string>();
		m_VarSel = -1;
		m_InputName = EditBoxWidget.Cast(root.FindAnyWidget("InputSetName"));
		m_InputUrl = EditBoxWidget.Cast(root.FindAnyWidget("InputSetUrl"));
		m_TxtUrlHint = MultilineTextWidget.Cast(root.FindAnyWidget("TxtSetUrlHint"));
		m_Enabled = new VPPWebhookToggle(root, "BtnSetEnabled");
		m_PresetHost = root.FindAnyWidget("SetPresetHost");
		m_InputContentType = EditBoxWidget.Cast(root.FindAnyWidget("InputSetContentType"));
		m_InputRate = EditBoxWidget.Cast(root.FindAnyWidget("InputSetRate"));
		m_HideIds = new VPPWebhookToggle(root, "BtnSetHideIds");
		m_HideAddr = new VPPWebhookToggle(root, "BtnSetHideAddr");
		m_VarList = TextListboxWidget.Cast(root.FindAnyWidget("VarList"));
		m_InputVarName = EditBoxWidget.Cast(root.FindAnyWidget("InputVarName"));
		m_InputVarValue = EditBoxWidget.Cast(root.FindAnyWidget("InputVarValue"));
		m_BtnVarSet = ButtonWidget.Cast(root.FindAnyWidget("BtnVarSet"));
		m_BtnVarRemove = ButtonWidget.Cast(root.FindAnyWidget("BtnVarRemove"));
		VPPXENumericEditHandler rateHandler = null;
		if (m_InputRate)
		{
			m_InputRate.GetScript(rateHandler);
		}

		m_RateHandler = rateHandler;

		if (m_RateHandler)
		{
			m_RateHandler.SetRange(0, 600);
			m_RateHandler.SetStep(5);
			m_RateHandler.m_OnValueChanged.Insert(OnRateChanged);
		}

		VPPWebhookPresets.Ids(m_PresetIds);
		string firstLabel = VPPWebhookUi.PresetLabel(m_PresetIds[0]);
		m_PresetDrop = new VPPDropDownMenu(m_PresetHost, firstLabel);
		foreach (string presetId : m_PresetIds)
		{
			string presetLabel = VPPWebhookUi.PresetLabel(presetId);
			m_PresetDrop.AddElement(presetLabel);
		}

		m_PresetDrop.SetIndex(0);
		m_PresetDrop.m_OnSelectItem.Insert(OnPresetSelected);
		m_Editable = true;
	}

	// ---------------------------------------------------------------- load

	void Load()
	{
		WebHook work = m_Owner.GetWork();
		if (!work)
		{
			return;
		}

		m_Loading = true;
		string workName = work.GetName();
		string workUrl = work.GetURL();
		m_InputName.SetText(workName);
		m_InputUrl.SetText(workUrl);
		string hintKey = "#VSTR_WH_SET_URL_HINT";
		if (WebHook.IsMasked(workUrl))
		{
			hintKey = "#VSTR_WH_SET_URL_MASKED";
		}

		string hint = VPPWebhookUi.Tr(hintKey);
		m_TxtUrlHint.SetText(hint);
		m_Enabled.SetOn(!work.m_Disabled);
		ShowPreset(work.m_Preset);
		m_InputContentType.SetText(work.m_ContentType);
		if (m_RateHandler)
		{
			m_RateHandler.SetValue(work.m_RateLimit);
		}

		m_HideIds.SetOn(work.m_HideIds);
		m_HideAddr.SetOn(work.m_HideServerAddress);
		m_VarSel = -1;
		RebuildVars();
		m_InputVarName.SetText("");
		m_InputVarValue.SetText("");
		m_Loading = false;
		ApplyEditable();
	}

	protected void ShowPreset(string presetId)
	{
		int index = m_PresetIds.Find(presetId);
		if (index < 0)
		{
			index = 0;
		}

		string label = VPPWebhookUi.PresetLabel(m_PresetIds[index]);
		m_PresetDrop.SetIndex(index);
		m_PresetDrop.SetText(label);
	}

	protected void RebuildVars()
	{
		WebHook work = m_Owner.GetWork();
		m_VarList.ClearItems();
		m_VarNames.Clear();
		if (!work || !work.m_Vars)
		{
			return;
		}

		int total = work.m_Vars.Count();
		int i = 0;
		int row = 0;
		string varName = "";
		string varValue = "";
		for (i = 0; i < total; i++)
		{
			varName = work.m_Vars.GetKey(i);
			varValue = work.m_Vars.GetElement(i);
			row = m_VarList.AddItem(varName, null, 0);
			m_VarList.SetItem(row, varValue, null, 1);
			m_VarNames.Insert(varName);
		}

		if (m_VarSel >= 0 && m_VarSel < m_VarNames.Count())
		{
			m_VarList.SelectRow(m_VarSel);
		}
		else
		{
			m_VarSel = -1;
			m_VarList.SelectRow(-1);
		}
	}

	// ---------------------------------------------------------------- edits

	void SetEditable(bool editable)
	{
		m_Editable = editable;
		ApplyEditable();
	}

	protected void ApplyEditable()
	{
		m_InputName.Enable(m_Editable);
		m_InputUrl.Enable(m_Editable);
		m_InputContentType.Enable(m_Editable);
		m_InputVarName.Enable(m_Editable);
		m_InputVarValue.Enable(m_Editable);
		m_BtnVarSet.Enable(m_Editable);
		m_BtnVarRemove.Enable(m_Editable && m_VarSel >= 0);
		m_Enabled.SetEnabled(m_Editable);
		m_HideIds.SetEnabled(m_Editable);
		m_HideAddr.SetEnabled(m_Editable);
		if (m_RateHandler)
		{
			m_RateHandler.SetEnabled(m_Editable);
		}
	}

	void OnRateChanged(EditBoxWidget box)
	{
		WebHook work = m_Owner.GetWork();
		if (m_Loading || !m_Editable || !work)
		{
			return;
		}

		int rate = 0;
		if (m_RateHandler.HasValue())
		{
			rate = m_RateHandler.GetValue();
		}

		work.m_RateLimit = rate;
		m_Owner.MarkDirty();
	}

	void OnPresetSelected(int index)
	{
		m_PresetDrop.Close();
		WebHook work = m_Owner.GetWork();
		if (index < 0 || index >= m_PresetIds.Count() || !work)
		{
			return;
		}

		string presetId = m_PresetIds[index];
		ShowPreset(work.m_Preset);
		if (m_Loading || !m_Editable || presetId == work.m_Preset)
		{
			return;
		}

		if (m_Owner.IsWorkNew())
		{
			ApplyPreset(presetId);
			return;
		}

		m_PendingPreset = presetId;
		m_Owner.Confirm("SET_PRESET", "#VSTR_WH_DLG_PRESET_TITLE", "#VSTR_WH_DLG_PRESET");
	}

	protected void ApplyPreset(string presetId)
	{
		WebHook work = m_Owner.GetWork();
		if (!work)
		{
			return;
		}

		string oldDefault = VPPWebhookPresets.ContentTypeOf(work.m_Preset);
		work.m_Preset = presetId;
		if (work.m_ContentType == "" || work.m_ContentType == oldDefault)
		{
			work.m_ContentType = VPPWebhookPresets.ContentTypeOf(presetId);
		}

		m_Loading = true;
		ShowPreset(presetId);
		m_InputContentType.SetText(work.m_ContentType);
		m_Loading = false;
		m_Owner.MarkDirty();
	}

	void OnDialogYes(string action)
	{
		if (action == "SET_PRESET")
		{
			ApplyPreset(m_PendingPreset);
			m_PendingPreset = "";
		}
	}

	void OnDialogNo(string action)
	{
		if (action == "SET_PRESET")
		{
			m_PendingPreset = "";
		}
	}

	protected void SetVar()
	{
		WebHook work = m_Owner.GetWork();
		if (!work)
		{
			return;
		}

		string varName = m_InputVarName.GetText();
		string varValue = m_InputVarValue.GetText();
		varName = varName.Trim();
		if (varName.IndexOf("hook.") == 0)
		{
			varName = varName.Substring(5, varName.Length() - 5);
		}

		if (!VPPWebhookVarNames.IsPlain(varName))
		{
			m_Owner.Notify("#VSTR_WH_ERR_VAR_NAME");
			return;
		}

		if (!work.m_Vars.Contains(varName) && work.m_Vars.Count() >= 32)
		{
			m_Owner.Notify("#VSTR_WH_ERR_VAR_LIMIT");
			return;
		}

		work.m_Vars.Set(varName, varValue);
		m_VarSel = -1;
		RebuildVars();
		m_VarSel = m_VarNames.Find(varName);
		if (m_VarSel >= 0)
		{
			m_VarList.SelectRow(m_VarSel);
		}

		ApplyEditable();
		m_Owner.MarkDirty();
	}

	protected void RemoveVar()
	{
		WebHook work = m_Owner.GetWork();
		if (!work || m_VarSel < 0 || m_VarSel >= m_VarNames.Count())
		{
			return;
		}

		string varName = m_VarNames[m_VarSel];
		work.m_Vars.Remove(varName);
		m_VarSel = -1;
		RebuildVars();
		m_InputVarName.SetText("");
		m_InputVarValue.SetText("");
		ApplyEditable();
		m_Owner.MarkDirty();
	}

	bool OnClick(Widget w)
	{
		WebHook work = m_Owner.GetWork();
		if (!work || !m_Editable)
		{
			return false;
		}

		if (m_Enabled.Is(w))
		{
			m_Enabled.Flip();
			work.m_Disabled = !m_Enabled.IsOn();
			m_Owner.MarkDirty();
			return true;
		}

		if (m_HideIds.Is(w))
		{
			m_HideIds.Flip();
			work.m_HideIds = m_HideIds.IsOn();
			m_Owner.MarkDirty();
			return true;
		}

		if (m_HideAddr.Is(w))
		{
			m_HideAddr.Flip();
			work.m_HideServerAddress = m_HideAddr.IsOn();
			m_Owner.MarkDirty();
			return true;
		}

		if (w == m_BtnVarSet)
		{
			SetVar();
			return true;
		}

		if (w == m_BtnVarRemove)
		{
			RemoveVar();
			return true;
		}

		return false;
	}

	bool OnChange(Widget w)
	{
		WebHook work = m_Owner.GetWork();
		if (m_Loading || !m_Editable || !work)
		{
			return false;
		}

		string text = "";
		if (w == m_InputName)
		{
			text = m_InputName.GetText();
			work.SetName(text);
			m_Owner.MarkDirty();
			return true;
		}

		if (w == m_InputUrl)
		{
			text = m_InputUrl.GetText();
			work.SetURL(text);
			m_Owner.MarkDirty();
			return true;
		}

		if (w == m_InputContentType)
		{
			text = m_InputContentType.GetText();
			work.m_ContentType = text;
			m_Owner.MarkDirty();
			return true;
		}

		return false;
	}

	void OnUpdate()
	{
		if (!m_VarList || !m_Root.IsVisible())
		{
			return;
		}

		int row = m_VarList.GetSelectedRow();
		if (row == m_VarSel)
		{
			return;
		}

		m_VarSel = row;
		WebHook work = m_Owner.GetWork();
		if (work && row >= 0 && row < m_VarNames.Count())
		{
			string varName = m_VarNames[row];
			string varValue = work.m_Vars.Get(varName);
			m_InputVarName.SetText(varName);
			m_InputVarValue.SetText(varValue);
		}

		ApplyEditable();
	}

	// Two columns that always fit the view: the form on the left (at most 560 px) and the custom variables in the
	// rest; every input, hint and button is sized from the column it is in.
	void Relayout(float width, float height)
	{
		float labelW = 166;
		float gap = 20;
		float leftW = width * 0.56;
		if (leftW > 560)
		{
			leftW = 560;
		}

		if (leftW < 400)
		{
			leftW = 400;
		}

		float inputW = leftW - labelW - 4;
		float varsX = leftW + gap;
		float varsW = width - varsX - 4;
		if (varsW < 220)
		{
			varsW = 220;
		}

		SizeWidth("TxtSetGeneral", leftW);
		SizeWidth("InputSetName", inputW);
		SizeWidth("InputSetUrl", inputW);
		SizeWidth("TxtSetUrlHint", inputW);
		SizeWidth("TxtSetFormat", leftW);
		SizeWidth("TxtSetPresetHint", inputW);
		SizeWidth("TxtSetDelivery", leftW);
		SizeWidth("TxtSetRateHint", leftW - labelW - 100);
		SizeWidth("TxtSetPrivacy", leftW);
		SizeWidth("TxtSetPrivacyHint", inputW);
		float toggleW = (inputW - 6) / 2;
		Widget hideIds = m_Root.FindAnyWidget("BtnSetHideIds");
		Widget hideAddr = m_Root.FindAnyWidget("BtnSetHideAddr");
		hideIds.SetSize(toggleW, 28);
		hideAddr.SetPos(labelW + toggleW + 6, 414);
		hideAddr.SetSize(toggleW, 28);
		SizeChild("BtnSetHideIds", "TxtSetHideIds", toggleW);
		SizeChild("BtnSetHideAddr", "TxtSetHideAddr", toggleW);

		Widget varsTitle = m_Root.FindAnyWidget("TxtSetVars");
		Widget varsHint = m_Root.FindAnyWidget("TxtSetVarsHint");
		varsTitle.SetPos(varsX, 0);
		varsTitle.SetSize(varsW, 22);
		varsHint.SetPos(varsX, 24);
		varsHint.SetSize(varsW, 36);
		float listH = height - 64 - 130;
		m_VarList.SetPos(varsX, 64);
		m_VarList.SetSize(varsW, listH);
		float rowY = 64 + listH + 8;
		float varLabelW = 74;
		float varInputW = varsW - varLabelW;
		Widget lblVarName = m_Root.FindAnyWidget("LblVarName");
		Widget lblVarValue = m_Root.FindAnyWidget("LblVarValue");
		lblVarName.SetPos(varsX, rowY);
		m_InputVarName.SetPos(varsX + varLabelW, rowY);
		m_InputVarName.SetSize(varInputW, 28);
		lblVarValue.SetPos(varsX, rowY + 34);
		m_InputVarValue.SetPos(varsX + varLabelW, rowY + 34);
		m_InputVarValue.SetSize(varInputW, 28);
		float buttonW = (varInputW - 6) / 2;
		m_BtnVarSet.SetPos(varsX + varLabelW, rowY + 70);
		m_BtnVarSet.SetSize(buttonW, 30);
		m_BtnVarRemove.SetPos(varsX + varLabelW + buttonW + 6, rowY + 70);
		m_BtnVarRemove.SetSize(buttonW, 30);
	}

	protected void SizeWidth(string widgetName, float width)
	{
		float oldW = 0;
		float oldH = 0;
		Widget target = m_Root.FindAnyWidget(widgetName);
		if (!target)
		{
			return;
		}

		target.GetSize(oldW, oldH);
		target.SetSize(width, oldH);
	}

	// A toggle's centred label follows its button's width.
	protected void SizeChild(string buttonName, string childName, float width)
	{
		float oldW = 0;
		float oldH = 0;
		Widget button = m_Root.FindAnyWidget(buttonName);
		if (!button)
		{
			return;
		}

		Widget child = button.FindAnyWidget(childName);
		if (!child)
		{
			return;
		}

		child.GetSize(oldW, oldH);
		child.SetSize(width, oldH);
	}
};
