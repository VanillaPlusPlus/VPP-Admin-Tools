class TeleportEntry: VPPPlayerTemplate
{
	protected ref VPPTeleportLocation   m_VPPTeleportLocation;
	protected CheckBoxWidget 	 		m_CheckBox;
	protected TextWidget                m_CoordsInput;
	protected bool 				 		m_IsVisible;
	
	void TeleportEntry(GridSpacerWidget grid, VPPTeleportLocation teleportInfo)
	{
		m_VPPTeleportLocation      = teleportInfo;
		m_LayoutPath  = VPPATUIConstants.TeleportEntry;
		m_EntryBox    = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_CheckBox = CheckBoxWidget.Cast( m_EntryBox.FindAnyWidget("CheckBox") );
		m_CheckBox.SetText(teleportInfo.GetName());
		m_CoordsInput = TextWidget.Cast(m_EntryBox.FindAnyWidget("CoordsInput"));
		m_CoordsInput.SetText(g_Game.VectorToString(teleportInfo.GetLocation()));
		m_CheckBox.SetHandler(this);
		grid.Update();
		m_IsVisible = true;
	}
	
	void ~TeleportEntry()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_CheckBox)
		{
			m_CheckBox.SetChecked(!m_CheckBox.IsChecked());
			MenuTeleportManager tManager = MenuTeleportManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuTeleportManager));
			
			float scale = Math.Clamp(tManager.m_Map.GetScale() / 3, 0.1, 2.5);
			tManager.m_Map.SetScale(scale);
			tManager.m_Map.SetMapPos(GetVPPTeleportLocation().GetLocation());
			return true;
		}
		return super.OnClick(w, x, y, button);
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
	
	VPPTeleportLocation GetVPPTeleportLocation()
	{
		return m_VPPTeleportLocation;
	}
	
	CheckBoxWidget GetCheckBox()
	{
		return m_CheckBox;
	}
};