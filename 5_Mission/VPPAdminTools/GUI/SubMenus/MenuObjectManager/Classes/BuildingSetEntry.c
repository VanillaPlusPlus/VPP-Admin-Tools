class BuildingSetEntry : VPPPlayerTemplate
{
    private TextWidget 	   m_SetNameWidget;
	private CheckBoxWidget m_StatusCheckBox;
	private ButtonWidget   m_DeleteItem;
	private ButtonWidget   m_EditItem;
    private string 		   m_SetName;
	private bool   		   m_IsVisible;
	GridSpacerWidget 	   m_Grid;
    
    void BuildingSetEntry(GridSpacerWidget grid, Widget rootWidget,string itemSetName)
    {
		m_Grid       = grid;
        m_SetName    = itemSetName;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingSetEntry.layout";
        m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_StatusCheckBox = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("Active"));
		GetVPPUIManager().HookConfirmationDialog(m_StatusCheckBox, rootWidget,this,"CheckBoxEvent", DIAGTYPE.DIAG_YESNO, "#VSTR_TITLE_WARNING", "#VSTR_TOOLTIP_SELECT_SET");
		m_EditItem   = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnEditItem"));
		
		m_DeleteItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteItem"));
		GetVPPUIManager().HookConfirmationDialog(m_DeleteItem, rootWidget,this,"DeleteItem", DIAGTYPE.DIAG_YESNO, "Delete Item", "#VSTR_ESP_DEL_CONFIRM_2"+itemSetName+"#VSTR_ESP_DEL_CONFIRM_3");
		
        m_SetNameWidget = TextWidget.Cast(m_EntryBox.FindAnyWidget("ItemName"));
        m_SetNameWidget.SetText(itemSetName);
		m_IsVisible = true;
		m_EntryBox.SetHandler(this);
    }
    
    void ~BuildingSetEntry()
    {
        if (m_EntryBox != null)
        	m_EntryBox.Unlink();
    }
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_EditItem && m_StatusCheckBox.IsChecked())
		{
			MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			objMenu.CreateSetEditor(true);
			return true;
		}
		return false;
	}
	
	void CheckBoxEvent(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			objMenu.DeselectAllSets();
			m_StatusCheckBox.SetChecked(true);
			//Request server to load selected entry data
			GetRPCManager().SendRPC("RPC_BuildingSetManager", "GetSetData", new Param1<string>(m_SetName),true,null);
		}else{
			m_StatusCheckBox.SetChecked(false);
		}
	}
	
	void DeleteItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
				objMenu.DeleteSelectedSet();
		}
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
    
    string GetSetName()
    {
        return m_SetName;
    }
	
	bool IsSelected()
	{
		return m_StatusCheckBox.IsChecked();
	}
	
	CheckBoxWidget GetCheckBox()
	{
		return m_StatusCheckBox;
	}
};