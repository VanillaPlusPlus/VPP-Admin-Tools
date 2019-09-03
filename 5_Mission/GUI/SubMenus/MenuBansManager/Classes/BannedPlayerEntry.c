class BannedPlayerEntry: VPPPlayerTemplate
{
	protected ref BannedPlayer   m_Player;
	protected CheckBoxWidget 	 m_CheckBox;
	protected bool 				 m_IsVisible;
	
	void BannedPlayerEntry(GridSpacerWidget grid, ref BannedPlayer player)
	{
		m_Player      = player;
		m_LayoutPath  = "VPPAdminTools/GUI/Layouts/PlayerManagerUI/VPPPlayerList.layout";
		m_EntryBox    = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_CheckBox = CheckBoxWidget.Cast( m_EntryBox.FindAnyWidget("CheckBox") );
		m_CheckBox.SetText(player.playerName);
		grid.Update();
		m_IsVisible = true;
	}
	
	void ~BannedPlayerEntry()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
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
	
	BannedPlayer GetPlayer()
	{
		return m_Player;
	}
	
	CheckBoxWidget GetCheckBox()
	{
		return m_CheckBox;
	}
};