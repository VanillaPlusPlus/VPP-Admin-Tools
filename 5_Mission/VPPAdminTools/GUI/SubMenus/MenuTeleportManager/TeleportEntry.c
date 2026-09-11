/*
	Single row in the SAVED LOCATIONS sidebar list of the Teleport Manager.
	Two-line layout (name over coords) with per-row Locate / Teleport / Edit / Delete
	actions and a multi-select checkbox. A row "mode" (saved vs recent) controls which
	actions are visible: recent rows hide the checkbox + Edit and repurpose Delete to
	"remove from recents".
*/
class TeleportEntry: VPPPlayerTemplate
{
	protected ref VPPTeleportLocation   m_VPPTeleportLocation;
	protected CheckBoxWidget 	 		m_CheckBox;
	protected TextWidget                m_NameLbl;
	protected TextWidget                m_CoordsInput;
	protected ButtonWidget              m_BtnLocate;
	protected ButtonWidget              m_BtnTeleport;
	protected ButtonWidget              m_BtnEdit;
	protected ButtonWidget              m_BtnDelete;
	protected bool 				 		m_IsVisible;
	protected bool                      m_RecentMode;

	protected MenuTeleportManager       m_Owner;

	void TeleportEntry(GridSpacerWidget grid, VPPTeleportLocation teleportInfo, MenuTeleportManager owner, bool recentMode = false)
	{
		m_VPPTeleportLocation = teleportInfo;
		m_Owner               = owner;
		m_RecentMode          = recentMode;
		m_LayoutPath          = VPPATUIConstants.TeleportEntry;
		m_EntryBox            = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);

		m_CheckBox    = CheckBoxWidget.Cast( m_EntryBox.FindAnyWidget("CheckBox") );
		m_NameLbl     = TextWidget.Cast( m_EntryBox.FindAnyWidget("NameLbl") );
		m_CoordsInput = TextWidget.Cast( m_EntryBox.FindAnyWidget("CoordsInput") );
		m_BtnLocate   = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnLocate") );
		m_BtnTeleport = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnTeleport") );
		m_BtnEdit     = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnEdit") );
		m_BtnDelete   = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnDelete") );

		m_NameLbl.SetText(teleportInfo.GetName());
		m_CoordsInput.SetText(g_Game.VectorToString(teleportInfo.GetLocation()));

		//recent rows are read-only history: no multi-select, no rename
		if (m_RecentMode)
		{
			m_CheckBox.Show(false);
			m_BtnEdit.Show(false);
		}

		//Per-button callbacks via the global event handler => single dispatch (no double-fire).
		//The checkbox is left to toggle natively; we only read IsChecked() when needed.
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnLocate,   this, "OnRowAction");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnTeleport, this, "OnRowAction");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnEdit,     this, "OnRowAction");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_BtnDelete,   this, "OnRowAction");

		grid.Update();
		m_IsVisible = true;
	}

	void ~TeleportEntry()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
	}

	void OnRowAction(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT || !m_Owner)
			return;

		if (w == m_BtnLocate)
			m_Owner.OnEntryLocate(this);
		else if (w == m_BtnTeleport)
			m_Owner.OnEntryTeleport(this);
		else if (w == m_BtnEdit)
			m_Owner.OnEntryEdit(this);
		else if (w == m_BtnDelete)
			m_Owner.OnEntryDelete(this, m_RecentMode);
	}

	void SetVisible(bool state)
	{
		m_IsVisible = state;
		m_EntryBox.Show(m_IsVisible);
	}

	bool IsVisible()
	{
		return m_IsVisible;
	}

	bool IsRecentMode()
	{
		return m_RecentMode;
	}

	VPPTeleportLocation GetVPPTeleportLocation()
	{
		return m_VPPTeleportLocation;
	}

	CheckBoxWidget GetCheckBox()
	{
		return m_CheckBox;
	}

	string GetName()
	{
		return m_VPPTeleportLocation.GetName();
	}
};
