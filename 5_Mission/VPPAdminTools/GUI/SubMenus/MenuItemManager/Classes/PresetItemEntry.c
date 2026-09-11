/*
	Single row in the CONTENTS list of the Item Manager PRESETS tab.
	Parent item carries a crown badge; explicit Make-parent / Remove buttons
	replace the legacy right-click / Ctrl+right-click gestures.
*/
class PresetItemEntry: VPPPlayerTemplate
{
	protected ButtonWidget 	       m_RemoveItem;
	protected ButtonWidget 	       m_BtnMakeParent;
	protected ImageWidget 	       m_ImgParent;
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
		m_LayoutPath  = VPPATUIConstants.EntryPresetItem;
		m_EntryBox    = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_EntryBox.SetHandler(this);
		m_RemoveItem    = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnDelete") );
		m_BtnMakeParent = ButtonWidget.Cast( m_EntryBox.FindAnyWidget("BtnMakeParent") );
		m_ImgParent     = ImageWidget.Cast( m_EntryBox.FindAnyWidget("ImgParent") );
		m_ItemName      = TextWidget.Cast( m_EntryBox.FindAnyWidget("ItemName") );

		m_ItemName.SetText(typeName);
		m_ImgParent.Show(isParent);
		if (isParent)
			m_ItemName.SetColor(ARGB(255, 232, 163, 61));  //accent-orange
		else
			m_ItemName.SetColor(ARGB(255, 232, 234, 237)); //text-primary

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
		if (button != MouseState.LEFT) return false;
		
		if (w == m_RemoveItem)
		{
			//Remove Item from preset
			GetPresetData().RemoveItem(m_typeName);
			SetEdited(true);
			NotifyOwnerReload();
			return true;
		}
		
		if (w == m_BtnMakeParent)
		{
			//Toggle parent flag: crowning the current parent clears it
			if (m_IsParent)
				m_PresetItemData.SetParentType("null");
			else
				m_PresetItemData.SetParentType(m_typeName);
			
			SetEdited(true);
			NotifyOwnerReload();
			return true;
		}
		return false;
	}
	
	private void NotifyOwnerReload()
	{
		MenuItemManager iManager = MenuItemManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuItemManager));
		if (iManager != null)
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(iManager.ReloadPresetData, 100, false);
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
