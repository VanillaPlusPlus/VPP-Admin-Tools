class BuildingEntry : VPPPlayerTemplate
{
    private TextWidget 	   m_BuildingDisplayNameWidget;
	private CheckBoxWidget m_StatusCheckBox;
	private ButtonWidget   m_DeleteItem;
    private string 		   m_BuildingDisplayName;
	private bool   		   m_IsVisible;
	private ref BuildingTracker m_TrackerWidget;
	GridSpacerWidget 	   m_Grid;
	string m_objectID;
	Object placedObject;
    
    void BuildingEntry(GridSpacerWidget grid, Widget rootWidget, string displayName, string objectID, Object localObj = null)
    {
		m_Grid       = grid;
        m_BuildingDisplayName = displayName;
		m_objectID   = objectID;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingEntry.layout";
        m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_StatusCheckBox = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("Active"));
		
		m_DeleteItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteItem"));
		GetVPPUIManager().HookConfirmationDialog(m_DeleteItem, rootWidget,this,"DeleteItem", DIAGTYPE.DIAG_YESNO, "Delete Item", "Are you sure you wish to delete "+displayName+"? (You can't revert once you delete this item!)");
		
        m_BuildingDisplayNameWidget = TextWidget.Cast(m_EntryBox.FindAnyWidget("ItemName"));
        m_BuildingDisplayNameWidget.SetText(displayName);
		m_IsVisible = true;
		m_EntryBox.SetHandler(this);
		
		//use Object directly (for objects locally placed)
		if (localObj != null)
		{
			placedObject = localObj;
			m_TrackerWidget = new BuildingTracker(rootWidget, displayName, localObj, false);
		}else{
		//Find object using its network ID
			TStringArray strs = new TStringArray;
			objectID.Split( ",",strs );
			autoptr Object trackerObj = GetGame().GetObjectByNetworkId(strs[1].ToInt(), strs[0].ToInt()); //low,high
			if (trackerObj != null){
				placedObject = trackerObj;
				m_TrackerWidget = new BuildingTracker(rootWidget, displayName, trackerObj, false);
			}
		}
    }
    
    void ~BuildingEntry()
    {
        if (m_EntryBox != null)
        	m_EntryBox.Unlink();
    }
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_TrackerWidget == null) return false;
		
		if (w == m_StatusCheckBox)
		{
			if (m_StatusCheckBox.IsChecked()){
				m_StatusCheckBox.SetColor(ARGB(255,0,255,0));
				m_TrackerWidget.ShowTracker(true);
			}else{
				m_StatusCheckBox.SetColor(ARGB(255,255,0,0));
				m_TrackerWidget.ShowTracker(false);
			}
		}
		return false;
	}
	
	void DeleteItem(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			autoptr MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
			{
				objMenu.RemoveBuilding(placedObject,m_objectID);
				delete this;
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
    
    string GetDisplayName()
    {
        return m_BuildingDisplayName;
    }
	
	bool IsSelected()
	{
		return m_StatusCheckBox.IsChecked();
	}
	
	BuildingTracker GetTracker()
	{
		return m_TrackerWidget;
	}
};