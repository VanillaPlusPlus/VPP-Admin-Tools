class VPPESPItemEntry : VPPPlayerTemplate
{
    private TextWidget 	   m_ItemNameWidget;
	private ButtonWidget   m_RemoveItem;
	private ButtonWidget   m_DeleteItem;
	private ButtonWidget   m_btnTpToMe;
	private ButtonWidget   m_btnTpReturn;
	private ref array<vector> m_PrevPositions;

    private string 		   m_ItemName;
	private bool   		   m_IsVisible;
	Object                 m_TargetObject;
	GridSpacerWidget 	   m_Grid;
    
    void VPPESPItemEntry(GridSpacerWidget grid, Object target ,Widget rootWidget, string itemName)
    {
		m_Grid       = grid;
        m_ItemName   = itemName;
        m_LayoutPath = VPPATUIConstants.EspItemEntry;
        m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_EntryBox.SetHandler(this);
		m_TargetObject = target;
		m_RemoveItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnRemoveItem"));
		GetVPPUIManager().HookConfirmationDialog(m_RemoveItem, rootWidget,this,"RemoveItem", DIAGTYPE.DIAG_YESNO, "Remove Item", "#VSTR_ESP_Q_REMOVE "+itemName+" #VSTR_ESP_Q_REMOVE_2");
		
		m_DeleteItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteItem"));
		GetVPPUIManager().HookConfirmationDialog(m_DeleteItem, rootWidget,this,"DeleteItem", DIAGTYPE.DIAG_YESNO, "Delete Item", "#VSTR_ESP_DEL_CONFIRM_2 "+itemName+" #VSTR_ESP_DEL_CONFIRM_3");
		
		m_btnTpToMe = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnTpToMe"));
		GetVPPUIManager().HookConfirmationDialog(m_btnTpToMe, rootWidget, this, "BringItem", DIAGTYPE.DIAG_YESNO, "Bring/Teleport Item", "Do you want to teleport/bring item: " + itemName + " to you? (If in Freecamera, item will be teleported to camera)");
		
		m_btnTpReturn = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnTpReturn"));
		GetVPPUIManager().HookConfirmationDialog(m_btnTpReturn, rootWidget, this, "ReturnItem", DIAGTYPE.DIAG_YESNO, "Return Item", "Do you want to return item: " + itemName + " to it's previous location?");
		
		m_PrevPositions = {};
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

	void BringItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			vector pos = GetGame().GetPlayer().GetPosition();
			if (IsFreeCamActive())
				pos = VPPGetCurrentCameraPosition();

			//save previous position
			m_PrevPositions.Insert(GetTargetObject().GetPosition());
			GetRPCManager().VSendRPC("RPC_VPPESPTools", "BringReturnObj", new Param2<Object, vector>(GetTargetObject(), pos), true, null);
		}
	}

	void ReturnItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			if (!GetTargetObject())
			{
				GetVPPUIManager().DisplayNotification("Can't preform action, object not found!");
				return;
			}

			if (m_PrevPositions && m_PrevPositions.Count() > 0)
			{
				//Get most recent position
				int index = m_PrevPositions.Count() - 1;
				if (index >= 0)
				{
					GetRPCManager().VSendRPC("RPC_VPPESPTools", "BringReturnObj", new Param2<Object, vector>(GetTargetObject(), m_PrevPositions[index]), true, null);
					m_PrevPositions.RemoveOrdered(index);
				}
			}
			else
			{
				GetVPPUIManager().DisplayNotification("Couldn't return item, no previous saved positions!");
			}
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