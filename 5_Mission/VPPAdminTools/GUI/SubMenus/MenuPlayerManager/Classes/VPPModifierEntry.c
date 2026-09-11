/*
	Single row in the MODIFIERS tab of the Player Manager vitals panel.
	Shows modifier name + active dot, with an Add/Remove toggle button.
*/
class VPPModifierEntry
{
	private Widget            m_Root;
	private ImageWidget       m_Dot;
	private TextWidget        m_Name;
	private ButtonWidget      m_BtnToggle;
	private TextWidget        m_ToggleLbl;

	private MenuPlayerManager m_Owner;
	private int               m_ModifierId;
	private bool              m_Active;

	void VPPModifierEntry(Widget parent, MenuPlayerManager owner, int modifierId, string displayName, bool active)
	{
		m_Owner      = owner;
		m_ModifierId = modifierId;

		m_Root      = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPModifierEntry, parent);
		m_Dot       = ImageWidget.Cast(m_Root.FindAnyWidget("ModEntryDot"));
		m_Name      = TextWidget.Cast(m_Root.FindAnyWidget("ModEntryName"));
		m_BtnToggle = ButtonWidget.Cast(m_Root.FindAnyWidget("ModEntryToggle"));
		m_ToggleLbl = TextWidget.Cast(m_Root.FindAnyWidget("ModEntryToggleLbl"));

		m_Name.SetText(displayName);
		ApplyState(active);

		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnToggle, this, "OnToggleClick");
	}

	void ~VPPModifierEntry()
	{
		if (m_Root)
			m_Root.Unlink();
	}

	void ApplyState(bool active)
	{
		m_Active = active;
		if (active)
		{
			m_Dot.SetColor(ARGB(255, 76, 175, 80));   //positive-green
			m_Name.SetColor(ARGB(255, 232, 234, 237)); //text-primary
			m_ToggleLbl.SetText("Remove");
		}
		else
		{
			m_Dot.SetColor(ARGB(255, 92, 97, 102));    //text-muted
			m_Name.SetColor(ARGB(255, 154, 160, 166)); //text-secondary
			m_ToggleLbl.SetText("Add");
		}
	}

	void OnToggleClick(Widget w, int x, int y, int button)
	{
		if (w != m_BtnToggle || !m_Owner) return;

		m_Owner.RequestModifierToggle(m_ModifierId, !m_Active);
		ApplyState(!m_Active); //optimistic; server list refresh will correct if needed
	}

	int GetModifierID()
	{
		return m_ModifierId;
	}
}
