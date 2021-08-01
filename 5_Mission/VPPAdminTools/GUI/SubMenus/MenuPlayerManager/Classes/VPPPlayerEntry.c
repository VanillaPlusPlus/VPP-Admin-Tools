class VPPPlayerEntry : VPPPlayerTemplate
{
    private CheckBoxWidget m_PlayerToggle;
    private string m_PlayerName;
    private string m_id;
    private int    m_SessionId;
	private bool   m_IsVisible;
	GridSpacerWidget m_Grid;
    
    void VPPPlayerEntry(GridSpacerWidget grid, string playerName, string id, int sessionId, bool startCheck = false)
    {
		m_Grid       = grid;
        m_PlayerName = playerName;
        m_id = id;
        m_SessionId = sessionId;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/PlayerManagerUI/VPPPlayerList.layout";
        m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
        m_PlayerToggle = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_PlayerToggle.SetText(playerName);
        m_PlayerToggle.SetChecked(startCheck);
		m_IsVisible = true;
    }
	
	void RedrawWidgets(CustomGridSpacer newGrid , bool selected = false)
	{
		if (m_EntryBox != null)
       	 	m_EntryBox.Unlink();
		
		m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, newGrid.GetGrid());
        m_PlayerToggle = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_PlayerToggle.SetText(m_PlayerName);
        m_PlayerToggle.SetChecked(selected);
		newGrid.AddWidget(m_EntryBox);
		m_IsVisible = true;
	}
    
    void ~VPPPlayerEntry()
    {
        if (m_EntryBox != null)
        m_EntryBox.Unlink();
    }
	
	void SetVisible(bool state)
	{
		m_IsVisible = state;
		m_EntryBox.Show(state);
	}
	
	bool IsVisible()
	{
		return m_EntryBox.IsVisible();
	}
  
    CheckBoxWidget GetCheckWidget()
    {
        return m_PlayerToggle;
    }
    
    string GetID()
    {
        return m_id;
    }

    int GetSessionId()
    {
        return m_SessionId;
    }
    
    string GetPlayerName()
    {
        return m_PlayerName;
    }
    
	void SetSelected(bool checked)
	{
        if (m_PlayerToggle)
		  m_PlayerToggle.SetChecked(checked);
	}
	
    bool IsSelected()
    {
        return (m_PlayerToggle && m_PlayerToggle.IsChecked());
    }
}