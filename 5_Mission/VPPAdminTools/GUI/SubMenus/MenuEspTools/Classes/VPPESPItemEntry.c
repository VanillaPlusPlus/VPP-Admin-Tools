class VPPESPItemEntry : VPPPlayerTemplate
{
    private TextWidget 	   m_ItemNameWidget;
	private ButtonWidget   m_RemoveItem;
	private ButtonWidget   m_DeleteItem;
    private string 		   m_ItemName;
	private bool   		   m_IsVisible;
	Object                 m_TargetObject;
	GridSpacerWidget 	   m_Grid;
    
    void VPPESPItemEntry(GridSpacerWidget grid, Object target ,Widget rootWidget, string itemName)
    {
		m_Grid       = grid;
        m_ItemName   = itemName;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspItemEntry.layout";
        m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_EntryBox.SetHandler(this);
		m_TargetObject = target;
		m_RemoveItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnRemoveItem"));
		GetVPPUIManager().HookConfirmationDialog(m_RemoveItem, rootWidget,this,"RemoveItem", DIAGTYPE.DIAG_YESNO, "Remove Item", "#VSTR_ESP_Q_REMOVE"+itemName+"#VSTR_ESP_Q_REMOVE_2");
		
		m_DeleteItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteItem"));
		GetVPPUIManager().HookConfirmationDialog(m_DeleteItem, rootWidget,this,"DeleteItem", DIAGTYPE.DIAG_YESNO, "Delete Item", "#VSTR_ESP_DEL_CONFIRM_2"+itemName+"#VSTR_ESP_DEL_CONFIRM_3");
		
        m_ItemNameWidget = TextWidget.Cast(m_EntryBox.FindAnyWidget("ItemName"));
        m_ItemNameWidget.SetText(itemName);
		m_IsVisible = true;
    }
    
    void ~VPPESPItemEntry()
    {
        if (m_EntryBox != null)
        	m_EntryBox.Unlink();
    }
	
	void RemoveItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu)
				espMenu.RemoveEspItemEntry(this);
		}
	}
	
	void DeleteItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu)
				espMenu.DeleteESPItems(this);
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
    
    string GetItemName()
    {
        return m_ItemName;
    }
	
	Object GetTargetObject()
	{
		return m_TargetObject;
	}
};