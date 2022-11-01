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
        m_LayoutPath = VPPATUIConstants.VPPPlayerList;
        m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
        m_PlayerToggle = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_PlayerToggle.SetText(playerName);
        m_PlayerToggle.SetChecked(startCheck);
		m_IsVisible = true;
        LabelOwnerName();
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
        LabelOwnerName();
	}
    
    void ~VPPPlayerEntry()
    {
        if (m_EntryBox != null)
        m_EntryBox.Unlink();
    }

    void LabelOwnerName()
    {
        if (GetID() == g_Game.VPPAT_GetSteam64Id())
            m_PlayerToggle.SetTextColor(ARGB(255,0,255,2));
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
        if (!m_EntryBox)
            return false;

        bool isSelected = (m_PlayerToggle && m_PlayerToggle.IsChecked());
        if (isSelected)
            m_EntryBox.SetColor(ARGB(140,0,0,255));
        else
            m_EntryBox.SetColor(ARGB(140,255,255,255));

        return isSelected;
    }
};