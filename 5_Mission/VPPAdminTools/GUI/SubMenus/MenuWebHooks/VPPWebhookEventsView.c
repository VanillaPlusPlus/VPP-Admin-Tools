/*
	EVENTS view of the webhooks menu: every event with its on/off state, and the filters of the selected one
	(kill / hit: PvP only, skip AI, minimum distance; admin actions: module include / exclude lists; server status:
	the interval). Edits go straight into the owner's work copy and mark it dirty.
*/
class VPPWebhookEventsView : Managed
{
	protected MenuWebHooks m_Owner;
	protected Widget m_Root;
	protected TextListboxWidget m_List;
	protected ButtonWidget m_BtnAllOn;
	protected ButtonWidget m_BtnAllOff;
	protected Widget m_Form;
	protected TextWidget m_TxtTitle;
	protected MultilineTextWidget m_TxtDesc;
	protected ref VPPWebhookToggle m_Enabled;
	protected ref VPPWebhookToggle m_Pvp;
	protected ref VPPWebhookToggle m_SkipAI;
	protected Widget m_LblMinDist;
	protected EditBoxWidget m_InputMinDist;
	protected VPPXENumericEditHandler m_MinDistHandler;
	protected Widget m_TxtMinDistUnit;
	protected Widget m_LblModules;
	protected EditBoxWidget m_InputModules;
	protected Widget m_LblExclude;
	protected EditBoxWidget m_InputExclude;
	protected Widget m_TxtModulesHint;
	protected Widget m_LblInterval;
	protected Widget m_IntervalHost;
	protected ref VPPDropDownMenu m_Interval;
	protected ref array<string> m_IntervalLabels;
	protected MultilineTextWidget m_TxtNote;
	protected ButtonWidget m_BtnTemplate;

	protected ref array<string> m_EventIds;
	protected int m_ListSel;
	protected bool m_Editable;
	protected bool m_Loading;

	void VPPWebhookEventsView(MenuWebHooks owner, Widget root)
	{
		m_Owner = owner;
		m_Root = root;
		m_EventIds = new array<string>();
		m_IntervalLabels = new array<string>();
		m_ListSel = -1;
		m_List = TextListboxWidget.Cast(root.FindAnyWidget("EvList"));
		m_BtnAllOn = ButtonWidget.Cast(root.FindAnyWidget("BtnEvAllOn"));
		m_BtnAllOff = ButtonWidget.Cast(root.FindAnyWidget("BtnEvAllOff"));
		m_Form = root.FindAnyWidget("EvForm");
		m_TxtTitle = TextWidget.Cast(root.FindAnyWidget("TxtEvTitle"));
		m_TxtDesc = MultilineTextWidget.Cast(root.FindAnyWidget("TxtEvDesc"));
		m_Enabled = new VPPWebhookToggle(root, "BtnEvEnabled");
		m_Pvp = new VPPWebhookToggle(root, "BtnEvPvp");
		m_SkipAI = new VPPWebhookToggle(root, "BtnEvSkipAI");
		m_LblMinDist = root.FindAnyWidget("LblEvMinDist");
		m_InputMinDist = EditBoxWidget.Cast(root.FindAnyWidget("InputEvMinDist"));
		m_TxtMinDistUnit = root.FindAnyWidget("TxtEvMinDistUnit");
		m_LblModules = root.FindAnyWidget("LblEvModules");
		m_InputModules = EditBoxWidget.Cast(root.FindAnyWidget("InputEvModules"));
		m_LblExclude = root.FindAnyWidget("LblEvExclude");
		m_InputExclude = EditBoxWidget.Cast(root.FindAnyWidget("InputEvExclude"));
		m_TxtModulesHint = root.FindAnyWidget("TxtEvModulesHint");
		m_LblInterval = root.FindAnyWidget("LblEvInterval");
		m_IntervalHost = root.FindAnyWidget("EvIntervalHost");
		m_TxtNote = MultilineTextWidget.Cast(root.FindAnyWidget("TxtEvNote"));
		m_BtnTemplate = ButtonWidget.Cast(root.FindAnyWidget("BtnEvTemplate"));
		VPPXENumericEditHandler minDistHandler = null;
		if (m_InputMinDist)
		{
			m_InputMinDist.GetScript(minDistHandler);
		}

		m_MinDistHandler = minDistHandler;

		if (m_MinDistHandler)
		{
			m_MinDistHandler.SetRange(0, 20000);
			m_MinDistHandler.SetStep(10);
			m_MinDistHandler.m_OnValueChanged.Insert(OnMinDistChanged);
		}

		VPPWebhookUi.IntervalLabels(m_IntervalLabels);
		string firstLabel = m_IntervalLabels[1];
		m_Interval = new VPPDropDownMenu(m_IntervalHost, firstLabel);
		foreach (string intervalLabel : m_IntervalLabels)
		{
			m_Interval.AddElement(intervalLabel);
		}

		m_Interval.SetIndex(1);
		m_Interval.m_OnSelectItem.Insert(OnIntervalSelected);
		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			m_EventIds.Insert(def.Id);
		}

		m_Editable = true;
	}

	string SelectedEventId()
	{
		if (m_ListSel < 0 || m_ListSel >= m_EventIds.Count())
		{
			return "";
		}

		return m_EventIds[m_ListSel];
	}

	protected VPPWebhookEventCfg CfgOf(string eventId)
	{
		WebHook work = m_Owner.GetWork();
		if (!work || !work.m_Events || eventId == "")
		{
			return null;
		}

		VPPWebhookEventCfg cfg = work.m_Events.Get(eventId);
		if (!cfg)
		{
			cfg = new VPPWebhookEventCfg();
			work.m_Events.Set(eventId, cfg);
		}

		return cfg;
	}

	// ---------------------------------------------------------------- load

	void Load()
	{
		if (!m_List)
		{
			return;
		}

		m_List.ClearItems();
		int total = m_EventIds.Count();
		int i = 0;
		for (i = 0; i < total; i++)
		{
			m_List.AddItem("", null, 0);
		}

		RefreshRows();
		if (m_ListSel < 0 || m_ListSel >= total)
		{
			m_ListSel = 0;
		}

		m_List.SelectRow(m_ListSel);
		LoadForm();
	}

	protected void RefreshRows()
	{
		int total = m_EventIds.Count();
		int i = 0;
		string eventId = "";
		string label = "";
		string state = "";
		int color = 0;
		VPPWebhookEventCfg cfg = null;
		for (i = 0; i < total; i++)
		{
			eventId = m_EventIds[i];
			label = VPPWebhookUi.EventLabel(eventId);
			cfg = CfgOf(eventId);
			state = VPPWebhookUi.Tr("#VSTR_WH_STATE_OFF");
			color = VPPWebhookUi.CLR_OFF;
			if (cfg && cfg.Enabled)
			{
				state = VPPWebhookUi.Tr("#VSTR_WH_STATE_ON");
				color = VPPWebhookUi.CLR_OK;
			}

			m_List.SetItem(i, label, null, 0);
			m_List.SetItem(i, state, null, 1);
			m_List.SetItemColor(i, 0, VPPWebhookUi.CLR_WHITE);
			m_List.SetItemColor(i, 1, color);
		}
	}

	protected void LoadForm()
	{
		string eventId = SelectedEventId();
		VPPWebhookEventCfg cfg = CfgOf(eventId);
		bool isKillHit = eventId == VPPWebhookDefs.EV_KILL || eventId == VPPWebhookDefs.EV_HIT;
		bool isAdmin = eventId == VPPWebhookDefs.EV_ADMIN;
		bool isStatus = eventId == VPPWebhookDefs.EV_STATUS;
		m_Form.Show(cfg != null);
		if (!cfg)
		{
			return;
		}

		m_Loading = true;
		string title = VPPWebhookUi.EventLabel(eventId);
		string desc = VPPWebhookUi.EventDesc(eventId);
		m_TxtTitle.SetText(title);
		m_TxtDesc.SetText(desc);
		m_Enabled.SetOn(cfg.Enabled);
		m_Pvp.SetOn(cfg.PvPOnly);
		m_SkipAI.SetOn(cfg.SkipAI);
		m_Pvp.Show(isKillHit);
		m_SkipAI.Show(isKillHit);
		m_LblMinDist.Show(isKillHit);
		m_InputMinDist.Show(isKillHit);
		m_TxtMinDistUnit.Show(isKillHit);
		if (m_MinDistHandler)
		{
			int minDistance = cfg.MinDistance;
			m_MinDistHandler.SetValue(minDistance);
		}

		m_LblModules.Show(isAdmin);
		m_InputModules.Show(isAdmin);
		m_LblExclude.Show(isAdmin);
		m_InputExclude.Show(isAdmin);
		m_TxtModulesHint.Show(isAdmin);
		string includeText = VPPWebhookUi.JoinList(cfg.Modules, ", ");
		string excludeText = VPPWebhookUi.JoinList(cfg.ExcludedModules, ", ");
		m_InputModules.SetText(includeText);
		m_InputExclude.SetText(excludeText);
		m_LblInterval.Show(isStatus);
		m_IntervalHost.Show(isStatus);
		WebHook work = m_Owner.GetWork();
		if (work)
		{
			VPPWebHookServerStatsTime workInterval = work.GetServerStatsInterval();
			int intervalIndex = VPPWebhookUi.IntervalIndex(workInterval);
			ShowInterval(intervalIndex);
		}

		string note = "";
		if (!isKillHit && !isAdmin && !isStatus)
		{
			note = VPPWebhookUi.Tr("#VSTR_WH_EV_NO_FILTERS");
		}

		m_TxtNote.SetText(note);
		m_Loading = false;
		ApplyEditable();
	}

	protected void ShowInterval(int index)
	{
		if (index < 0 || index >= m_IntervalLabels.Count())
		{
			return;
		}

		m_Interval.SetIndex(index);
		m_Interval.SetText(m_IntervalLabels[index]);
	}

	// ---------------------------------------------------------------- edits

	void SetEditable(bool editable)
	{
		m_Editable = editable;
		ApplyEditable();
	}

	protected void ApplyEditable()
	{
		m_Enabled.SetEnabled(m_Editable);
		m_Pvp.SetEnabled(m_Editable);
		m_SkipAI.SetEnabled(m_Editable);
		if (m_MinDistHandler)
		{
			m_MinDistHandler.SetEnabled(m_Editable);
		}

		if (m_InputModules)
		{
			m_InputModules.Enable(m_Editable);
		}

		if (m_InputExclude)
		{
			m_InputExclude.Enable(m_Editable);
		}

		if (m_BtnAllOn)
		{
			m_BtnAllOn.Enable(m_Editable);
		}

		if (m_BtnAllOff)
		{
			m_BtnAllOff.Enable(m_Editable);
		}
	}

	protected void Changed()
	{
		RefreshRows();
		m_Owner.MarkDirty();
	}

	protected void SetAll(bool enabled)
	{
		foreach (string eventId : m_EventIds)
		{
			VPPWebhookEventCfg cfg = CfgOf(eventId);
			if (cfg)
			{
				cfg.Enabled = enabled;
			}
		}

		LoadForm();
		Changed();
	}

	void OnMinDistChanged(EditBoxWidget box)
	{
		if (m_Loading || !m_Editable)
		{
			return;
		}

		string selectedId = SelectedEventId();
		VPPWebhookEventCfg cfg = CfgOf(selectedId);
		if (!cfg)
		{
			return;
		}

		float distance = 0;
		if (m_MinDistHandler && m_MinDistHandler.HasValue())
		{
			distance = m_MinDistHandler.GetValue();
		}

		cfg.MinDistance = distance;
		Changed();
	}

	void OnIntervalSelected(int index)
	{
		ShowInterval(index);
		m_Interval.Close();
		WebHook work = m_Owner.GetWork();
		if (m_Loading || !m_Editable || !work)
		{
			return;
		}

		VPPWebHookServerStatsTime interval = VPPWebhookUi.IntervalOf(index);
		if (interval == work.GetServerStatsInterval())
		{
			return;
		}

		work.SetServerStatsInterval(interval);
		Changed();
	}

	bool OnClick(Widget w)
	{
		string eventId = SelectedEventId();
		VPPWebhookEventCfg cfg = CfgOf(eventId);
		if (w == m_BtnTemplate)
		{
			m_Owner.OpenTemplate(eventId);
			return true;
		}

		if (!m_Editable)
		{
			return false;
		}

		if (w == m_BtnAllOn)
		{
			SetAll(true);
			return true;
		}

		if (w == m_BtnAllOff)
		{
			SetAll(false);
			return true;
		}

		if (!cfg)
		{
			return false;
		}

		if (m_Enabled.Is(w))
		{
			m_Enabled.Flip();
			cfg.Enabled = m_Enabled.IsOn();
			Changed();
			return true;
		}

		if (m_Pvp.Is(w))
		{
			m_Pvp.Flip();
			cfg.PvPOnly = m_Pvp.IsOn();
			Changed();
			return true;
		}

		if (m_SkipAI.Is(w))
		{
			m_SkipAI.Flip();
			cfg.SkipAI = m_SkipAI.IsOn();
			Changed();
			return true;
		}

		return false;
	}

	bool OnChange(Widget w)
	{
		if (m_Loading || !m_Editable)
		{
			return false;
		}

		string selectedId = SelectedEventId();
		VPPWebhookEventCfg cfg = CfgOf(selectedId);
		if (!cfg)
		{
			return false;
		}

		string text = "";
		if (w == m_InputModules)
		{
			text = m_InputModules.GetText();
			VPPWebhookUi.ParseModules(text, cfg.Modules);
			Changed();
			return true;
		}

		if (w == m_InputExclude)
		{
			text = m_InputExclude.GetText();
			VPPWebhookUi.ParseModules(text, cfg.ExcludedModules);
			Changed();
			return true;
		}

		return false;
	}

	void OnUpdate()
	{
		if (!m_List || !m_Root.IsVisible())
		{
			return;
		}

		int row = m_List.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		if (row < 0)
		{
			m_List.SelectRow(m_ListSel);
			return;
		}

		m_ListSel = row;
		LoadForm();
	}

	// The event list takes about a third (240..330 px), the form the rest; inputs and hints end inside the form.
	void Relayout(float width, float height)
	{
		float listW = width * 0.34;
		if (listW > 330)
		{
			listW = 330;
		}

		if (listW < 240)
		{
			listW = 240;
		}

		float listH = height - 44;
		float formX = listW + 10;
		float formW = width - formX;
		float fieldX = 190;
		float fieldW = formW - fieldX - 16;
		float allW = (listW - 10) / 2;
		m_List.SetSize(listW, listH);
		m_BtnAllOn.SetPos(0, height - 36);
		m_BtnAllOn.SetSize(allW, 30);
		m_BtnAllOff.SetPos(allW + 10, height - 36);
		m_BtnAllOff.SetSize(allW, 30);
		m_Form.SetPos(formX, 0);
		m_Form.SetSize(formW, height);
		m_BtnTemplate.SetPos(14, height - 44);
		m_TxtTitle.SetSize(formW - 36, 24);
		m_TxtDesc.SetSize(formW - 36, 42);
		m_TxtNote.SetSize(formW - 36, 60);
		m_InputModules.SetSize(fieldW, 28);
		m_InputExclude.SetSize(fieldW, 28);
		m_TxtModulesHint.SetSize(fieldW, 44);
		m_TxtMinDistUnit.SetSize(formW - 288 - 16, 28);
	}
};
