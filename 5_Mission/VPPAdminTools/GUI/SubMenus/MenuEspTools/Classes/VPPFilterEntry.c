class VPPFilterEntry : VPPPlayerTemplate
{
    private CheckBoxWidget  m_FilterToggle;
    private ButtonWidget    m_btnDeleteFilter;
	private bool   		    m_IsVisible;
	GridSpacerWidget 	    m_Grid;
	ref EspFilterProperties m_Props;
	XComboBoxWidget 		m_ColorCombo;
    
    void VPPFilterEntry(GridSpacerWidget grid, EspFilterProperties props, bool startCheck = false)
    {
		m_Grid       = grid;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/EspToolsUI/FilterEntry.layout";
        m_EntryBox = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
        m_FilterToggle = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
        m_ColorCombo   = XComboBoxWidget.Cast(m_EntryBox.FindAnyWidget("ColorCombo"));
        m_btnDeleteFilter   = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteFilter"));
        m_FilterToggle.SetText(props.filterType);
        m_FilterToggle.SetChecked(startCheck);
		m_IsVisible = true;
		m_Props = props;
		m_EntryBox.SetHandler(this);
		m_ColorCombo.SetCurrentItem(props.comboIndex);
    }
    
    void ~VPPFilterEntry()
    {
        if (m_EntryBox != null)
        m_EntryBox.Unlink();
    }
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		EspToolsMenu espMenu;
		
		if (w == m_FilterToggle)
		{
			espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu)
				espMenu.ClearTrackers();
			return true;
		}
		
		if (w == m_btnDeleteFilter)
		{
			espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu)
			{
				espMenu.RemoveFilterProps(m_Props);
			}
			return true;
		}
		
		if (w == m_ColorCombo)
		{
			int index = m_ColorCombo.GetCurrentItem();
			switch(index)
			{
				//White
				case 0:
				m_Props.color = ARGB(255,255,255,255);
				break;
				
				//Green
				case 1:
				m_Props.color = ARGB(255,0,255,0);
				break;
				
				//Blue
				case 2:
				m_Props.color = ARGB(255,0,0,255);
				break;
				
				//Yellow
				case 3:
				m_Props.color = ARGB(255,255,255,0);
				break;
				
				//Orange
				case 4:
				m_Props.color = ARGB(255,226,109,18);
				break;
				
				//Red
				case 5:
				m_Props.color = ARGB(255,255,0,0);
				break;
				
				//Purple
				case 6:
				m_Props.color = ARGB(255,100,11,234);
				break;

				//Black
				case 7:
				m_Props.color = ARGB(255,0,0,0);
				break;
			}
			m_Props.comboIndex = index;
			
			espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu)
			{
				espMenu.SaveFilters();
				return true;
			}
		}
		return false;
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
        return m_Props.filterType;
    }
	
	int GetFilterColor()
	{
		return m_Props.color;
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