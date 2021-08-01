class PresetItemEntry: VPPPlayerTemplate
{
	protected ButtonWidget 	       m_RemoveItem;
	protected TextWidget           m_ItemName;
	protected bool 				   m_IsVisible;
	protected bool                 m_IsParent;
	protected bool                 m_Edited;
	protected string          	   m_typeName;
	protected ref PresetItemData   m_PresetItemData;
	
	void PresetItemEntry(GridSpacerWidget grid, PresetItemData data, string typeName, bool isParent)
	{
		m_typeName    = typeName;
		m_IsParent    = isParent;
		m_PresetItemData = data;
		m_LayoutPath  = "VPPAdminTools/GUI/Layouts/ItemManagerUI/EntryPresetItem.layout";
		m_EntryBox    = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_EntryBox.SetHandler(this);
		m_RemoveItem  = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnDelete") );
		m_ItemName    = TextWidget.Cast( m_EntryBox.FindAnyWidget("ItemName") );

		if (isParent){
			m_ItemName.SetText(typeName + "#VSTR_DISPLAY_PARENT");
			m_EntryBox.SetColor(ARGB(255,0,255,25));
		}else{
			m_ItemName.SetText(typeName);
		}
		grid.Update();
		m_IsVisible = true;
	}
	
	void ~PresetItemEntry()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
	}
	
	PresetItemData GetPresetData()
	{
		return m_PresetItemData;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_RemoveItem && button == MouseState.LEFT)
		{
			//Remove Item from preset
			GetPresetData().RemoveItem(m_typeName);
			SetEdited(true);
			MenuItemManager iManager = MenuItemManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuItemManager));
			if (iManager != null)
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(iManager.ReloadPresetData, 100, false);
			
			return true;
		}
		return false;
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == MouseState.RIGHT && w == m_ItemName)
		{
			string parentType;
			if (g_Game.IsLeftCtrlDown())
				parentType = "null";
			else
				parentType = m_typeName;
			
			
			m_PresetItemData.SetParentType(parentType);
			SetEdited(true);
			MenuItemManager iManager = MenuItemManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuItemManager));
			if (iManager != null)
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(iManager.ReloadPresetData, 100, false);
			
			return true;
		}
		return false;
	}
	
	void SetVisible(bool state)
	{
		m_IsVisible = state;
		m_EntryBox.Show(m_IsVisible);
	}
	
	void SetEdited(bool state)
	{
		m_Edited = state;
	}
	
	bool IsEdited()
	{
		return m_Edited;
	}
	
	bool IsVisible()
	{
		return m_IsVisible;
	}
	
	bool IsParent()
	{
		return m_IsParent;
	}
	
	string GetTypeName()
	{
		return m_typeName;
	}
};