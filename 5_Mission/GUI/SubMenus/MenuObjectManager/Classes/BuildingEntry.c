class BuildingEntry : VPPPlayerTemplate
{
    private TextWidget 	   m_BuildingDisplayNameWidget;
	private CheckBoxWidget m_StatusCheckBox;
	private ButtonWidget   m_DeleteItem;
	private ButtonWidget   m_btnSelect;
	private ButtonWidget   m_btnEditItem;
	private ButtonWidget   m_btnPhysicsDrop;
    private string 		   m_BuildingDisplayName;
	private bool   		   m_IsVisible;
	private ref BuildingTracker m_TrackerWidget;
	ref SpawnedBuilding    m_SpawnedBuilding;
	GridSpacerWidget 	   m_Grid;
	string m_objectID;
	Object placedObject;
	
	private ref BuildingAttributesEditor editor;
    
    void BuildingEntry(GridSpacerWidget grid, Widget rootWidget, string displayName, string objectID, ref SpawnedBuilding sp, Object localObj = null)
    {
		m_SpawnedBuilding = sp;
		m_Grid       = grid;
        m_BuildingDisplayName = displayName;
		m_objectID   = objectID;
        m_LayoutPath = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingEntry.layout";
        m_EntryBox   = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_StatusCheckBox = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("Active"));
		
		m_DeleteItem  = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnDeleteItem"));
		m_btnEditItem = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnEditItem"));
		m_btnSelect   = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnSelect"));
		m_btnPhysicsDrop = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnPhysicsDrop"));
		GetVPPUIManager().HookConfirmationDialog(m_DeleteItem, rootWidget,this,"DeleteItem", DIAGTYPE.DIAG_YESNO, "Delete Item", "#VSTR_ESP_DEL_CONFIRM_2"+displayName+"#VSTR_ESP_DEL_CONFIRM_3");
		GetVPPUIManager().HookConfirmationDialog(m_btnPhysicsDrop, rootWidget,this,"PhysicsDrop", DIAGTYPE.DIAG_YESNO, "#VSTR_ALERT_ENABLE_PHYSICS", "#VSTR_TOOLTIP_PHYSICS");

        m_BuildingDisplayNameWidget = TextWidget.Cast(m_EntryBox.FindAnyWidget("ItemName"));
        m_BuildingDisplayNameWidget.SetText(displayName);
		m_IsVisible = true;
		m_EntryBox.SetHandler(this);
		
		//use Object directly (for objects locally placed)
		if (localObj != null)
		{
			placedObject = localObj;
			m_TrackerWidget = new BuildingTracker(rootWidget, displayName, localObj, true);
		}else{
		//Find object using its network ID
			TStringArray strs = new TStringArray;
			objectID.Split( ",",strs );
			autoptr Object trackerObj = GetGame().GetObjectByNetworkId(strs[1].ToInt(), strs[0].ToInt()); //low,high
			if (trackerObj != null){
				placedObject = trackerObj;
				m_TrackerWidget = new BuildingTracker(rootWidget, displayName, trackerObj, true);
			}
		}
		
		//Set to show tracker by defualt 
		m_StatusCheckBox.SetColor(ARGB(255,0,255,0));
		m_StatusCheckBox.SetChecked(true);
    }
    
    void ~BuildingEntry()
    {
        if (m_EntryBox != null)
        	m_EntryBox.Unlink();
    }
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		autoptr MenuObjectManager objMenu;
		
		if (w == m_btnEditItem)
		{
			editor = null;
			editor = new BuildingAttributesEditor(m_SpawnedBuilding);
			return true;
		}
		
		if (m_TrackerWidget == null) return false;
		
		if (w == m_btnSelect)
		{
			objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
			{
				objMenu.SetSelectedObject(placedObject);
			}
		}
		
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
				if (m_TrackerWidget != null)
				{
					m_TrackerWidget.ForceDeleteLocalObj();
				}
				delete this;
			}
		}
	}

	void PhysicsDrop(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			if ( placedObject )
			{
				placedObject.CreateDynamicPhysics(PhxInteractionLayers.DYNAMICITEM);
				placedObject.EnableDynamicCCD(true);
				placedObject.SetDynamicPhysicsLifeTime(10.0);
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(placedObject.EnableDynamicCCD, 9000, false, false);
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

	void SetSelected(bool state)
	{
		m_StatusCheckBox.SetChecked( state );
	}
	
	BuildingTracker GetTracker()
	{
		return m_TrackerWidget;
	}
};