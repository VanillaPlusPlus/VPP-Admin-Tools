class VPPFilterEntry : VPPPlayerTemplate
{
    private CheckBoxWidget m_FilterToggle;
    private string 		   m_FilterName;
	private bool   		   m_IsVisible;
	GridSpacerWidget 	   m_Grid;
    
    void VPPFilterEntry(GridSpacerWidget grid, string filterName, bool startCheck = false)
    {
		m_Grid       = grid;
        m_FilterName = filterName;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/EspToolsUI/FilterEntry.layout";
        m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
        m_FilterToggle = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_FilterToggle.SetText(filterName);
        m_FilterToggle.SetChecked(startCheck);
		m_IsVisible = true;
    }
    
    void ~VPPFilterEntry()
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
        return m_FilterToggle;
    }
    
    string GetFilterName()
    {
        return m_FilterName;
    }
    
	void SetSelected(bool checked)
	{
		m_FilterToggle.SetChecked(checked);
	}
	
    bool IsSelected()
    {
        return m_FilterToggle.IsChecked();
    }
}