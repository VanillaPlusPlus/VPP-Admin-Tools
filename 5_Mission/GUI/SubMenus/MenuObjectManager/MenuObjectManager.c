class MenuObjectManager extends AdminHudSubMenu
{
	private bool 		      		 m_loaded = false;
	private EditBoxWidget     		 m_SearchInputBox;
	private TextListboxWidget 	  	 m_ItemListBox;
	private ItemPreviewWidget 		 m_ItemPreview;
	private EntityAI 	  	  		 m_PreviewObject;
	private ButtonWidget             m_btnCreateNewSet;
	private ButtonWidget             m_btnSaveChanges;
	private int 	 		  		 m_RotationX;
	private int 	 		  		 m_RotationY;
	private vector   		  		 m_ItemOrientation;
	private int                   	 m_searchBoxCount;
	private int    					 m_prevRow;
	private ref BuildingSetEditor    m_setAttributesEditor;
	private VPPFreeCam               m_Camera;
	private ButtonWidget             m_chkFreeCam;
	private ButtonWidget             m_chkTopDowncam;
	private EditBoxWidget            m_EditYaw;
	private EditBoxWidget            m_EditPitch;
	private EditBoxWidget            m_EditRoll;
	private EditBoxWidget            m_EditFOV;
	private ButtonWidget             m_btnReloadSets;
	private GridSpacerWidget         m_ParentGrid;
	private ref CustomGridSpacer 	 m_LastGrid;
	private GridSpacerWidget         m_ParentGridItems;
	private ref CustomGridSpacer 	 m_LastGridItems;
	private ref array<ref CustomGridSpacer> m_DataGrids;      //Building sets
	private ref array<ref CustomGridSpacer> m_DataGridsItems; //Items of a building set
	private ref array<ref BuildingSetEntry> m_BuildingSetEntries;
	private ref array<ref BuildingEntry> 	m_BuildingEntries;
	private ref BuildingSet 				m_SelectedSetData; //Holds Objects and infos about selected set (from server)
	//---Panels---
	private Widget    m_PanelBuildingSets;
	private Widget    m_PanelCurrentSet;
	private Widget    m_PanelPlaceNew;
	private Widget    m_PanelCamera;
	//------------
	private ref array<ref Param2<bool,ref ButtonWidget>> m_ShowHideButtons;
	private ref array<string> m_BuildingSets; //Names of all saved sets (from server)
	private Object worldObject;
	
	void MenuObjectManager()
	{
		/* RPCs */
		GetRPCManager().AddRPC("RPC_MenuObjectManager", "HandleData", this);
		GetRPCManager().AddRPC("RPC_MenuObjectManager", "HandleSetsData", this);
		//-------
		
		m_DataGrids 	  	 = new array<ref CustomGridSpacer>;
		m_DataGridsItems  	 = new array<ref CustomGridSpacer>;
		m_BuildingSetEntries = new array<ref BuildingSetEntry>;
		m_BuildingEntries    = new array<ref BuildingEntry>;
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/ObjectManagerUI/MenuObjectManager.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_EditYaw     = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "EditYawC"));
		m_EditPitch   = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "EditPitchC"));
		m_EditRoll    = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "EditRollC"));
		m_EditFOV     = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "EditFOV"));
		m_chkFreeCam  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkFreeCam"));
		m_btnSaveChanges  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnSaveChanges"));
	    m_chkTopDowncam = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkTopDowncam"));
		m_chkTopDowncam.Enable(false);
	    m_btnCreateNewSet = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnCreateNewSet"));
		
		m_ShowHideButtons = new array<ref Param2<bool,ref ButtonWidget>>;
		m_SearchInputBox = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_ItemListBox    = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBox") );
		m_ItemPreview    = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreview") );
		m_btnReloadSets  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnReloadSets"));
		m_ShowHideButtons.Insert( new Param2<bool,ref ButtonWidget>(true,ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnShowBuildingSets"))));
		m_ShowHideButtons.Insert( new Param2<bool,ref ButtonWidget>(true,ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnShowAtts"))));
		m_ShowHideButtons.Insert( new Param2<bool,ref ButtonWidget>(true,ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnShowCurrSet"))));
		m_ShowHideButtons.Insert( new Param2<bool,ref ButtonWidget>(true,ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "btnShowCamOptions"))));
		
		m_ParentGrid      = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ParentGrid") );
		m_ParentGridItems = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ParentGridItems") );
		
		//---Panels---
		m_PanelBuildingSets  = M_SUB_WIDGET.FindAnyWidget( "PanelBuildingSets");
		m_PanelCurrentSet    = M_SUB_WIDGET.FindAnyWidget( "PanelCurrentSet");
		m_PanelPlaceNew      = M_SUB_WIDGET.FindAnyWidget( "PanelPlaceNew");
		m_PanelCamera        = M_SUB_WIDGET.FindAnyWidget( "PanelCamera");
		//------------
		
		//Get Data from Server via RPC 
		GetRPCManager().SendRPC("RPC_BuildingSetManager", "GetBuildingSets", null,true,null);
		//--
		
		UpdateFilter();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreviewWidget, 100, true);
		ShowSubMenu();
		m_loaded = true;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return;
		Input input = GetGame().GetInput();
		
		if (m_SelectedSetData != null)
			m_btnSaveChanges.Enable(true);
		else
			m_btnSaveChanges.Enable(false);
		//---
		
		int newSrchCount = m_SearchInputBox.GetText().Length();
		if (newSrchCount != m_searchBoxCount)
		{
			//Update Filter
			UpdateFilter();
			m_searchBoxCount = newSrchCount;
		}
		
		if (GetGame().GetMouseState(MouseState.LEFT) & 0x80000000)
		{
			vector direction = GetGame().GetPointerDirection();
			vector start = GetGame().GetCurrentCameraPosition();
			vector end = start + (direction * 10000);
			set<Object> hitObjects = new set<Object>;
			
			//Select part test
			if (worldObject == null)
			{
				hitObjects = g_Game.GetObjectsAt(start,end,GetGame().GetPlayer(),2.0);
				if (hitObjects != null && hitObjects.Count() >= 1 && m_SelectedSetData != null)
				{
					if (hitObjects[0] != null && hitObjects[0].GetNetworkIDString().ToInt() > 0 || m_SelectedSetData.GetBuildingByType(hitObjects[0]) != null)
					{
						worldObject = hitObjects[0];
						DeselectAllTrackers();
						autoptr BuildingTracker tracker = GetTrackerByObject(worldObject);
						if (tracker != null)
							tracker.Highlight(true);

						GetVPPUIManager().DisplayNotification("Object: "+worldObject+" selected!");
					}
				}
			}
		}
		
		//Move x,y,z
		if (!g_Game.IsLeftAltHolding() && !g_Game.IsLShiftHolding() && input.LocalHold("UASelectObject",false) && worldObject != null)
		{
			worldObject.PlaceOnSurface();
			vector c_pos;
			vector c_dir;
			int    c_component;
			
			if ( DayZPhysics.RaycastRV( start, end, c_pos, c_dir, c_component, null, worldObject, GetGame().GetPlayer(), false, false ) )
			{
				worldObject.SetPosition(c_pos);
				worldObject.PlaceOnSurface();
				FixCollide(worldObject);
			}
		}
		
		//Move y axis
		if (g_Game.IsLeftAltHolding() && worldObject != null)
		{
			float currentY = worldObject.GetPosition()[1];
			if (input.LocalValue( "UARotateInc" ))
				currentY = currentY + (1.0 * timeslice);
			else if (input.LocalValue( "UARotateDec" ))
				currentY = currentY - (1.0 * timeslice);

			worldObject.SetPosition(Vector(worldObject.GetPosition()[0],currentY,worldObject.GetPosition()[2]));
			UpdateTrackerDetails();
		}
		
		//Move Rotation
		if (g_Game.IsLShiftHolding() && worldObject != null)
		{
			float rotation = worldObject.GetOrientation()[0];
			if (input.LocalValue( "UARotateInc" ))
				rotation = rotation + (15.0 * timeslice);
			else if (input.LocalValue( "UARotateDec" ))
				rotation = rotation - (15.0 * timeslice);
			
			worldObject.SetOrientation(Vector(rotation,worldObject.GetOrientation()[1],worldObject.GetOrientation()[2]));
			UpdateTrackerDetails();
		}
		
		//Deselect
		if (g_Game.IsLeftCtrlDown() && worldObject != null)
		{
			worldObject = null;
			DeselectAllTrackers();
			GetVPPUIManager().DisplayNotification("Object deselected!");
		}
		
		//Camera control update
		autoptr EditBoxWidget editBox = EditBoxWidget.Cast(GetWidgetUnderCursor());
		if (editBox != null && m_Camera)
		{
			vector camOrientation = m_Camera.GetOrientation();
			switch(editBox.GetName())
			{
				case "EditYawC":
					if (camOrientation[0] != m_EditYaw.GetText().ToFloat() && m_EditYaw.GetText().ToFloat() > 0)
						m_Camera.SetYaw(m_EditYaw.GetText().ToFloat());
				break;
				
				case "EditPitchC":
					if (camOrientation[1] != m_EditPitch.GetText().ToFloat() && m_EditPitch.GetText().ToFloat() > 0)
						m_Camera.SetPitch(m_EditPitch.GetText().ToFloat());
				break;
				
				case "EditRollC":
					if (camOrientation[2] != m_EditRoll.GetText().ToFloat() && m_EditRoll.GetText().ToFloat() > 0)
						m_Camera.SetRoll(m_EditRoll.GetText().ToFloat());
				break;
				
				case "EditFOV":
					if (m_Camera.GetFov() != m_EditFOV.GetText().ToFloat() && m_EditFOV.GetText().ToFloat() > 0)
						m_Camera.SetFov(m_EditFOV.GetText().ToFloat());
				break;
			}
		}
	}
	
	void RemoveBuilding(Object toRemove, string networkdID = "")
	{
		autoptr SpawnedBuilding building = m_SelectedSetData.GetBuildingByType(toRemove);
		if (building == null)
			building = m_SelectedSetData.GetBuildingByNetworkID(networkdID);
		
		if (building != null)
			m_SelectedSetData.RemoveBuilding(building);
	}
	
	void DeleteSelectedSet()
	{
		if (m_SelectedSetData != null)
		{
			//Send RPC to delete selected set
			GetRPCManager().SendRPC("RPC_BuildingSetManager", "RemoteDeleteSet", new Param1<string>(m_SelectedSetData.GetName()),true,null);
			m_SelectedSetData = null;
		}
	}
	
	void UpdateBuildingSet(string setName, bool active)
	{
		if (m_SelectedSetData != null)
		{
			//Send RPC to update selected set
			GetRPCManager().SendRPC("RPC_BuildingSetManager", "RemoteUpdateSet", new Param3<string,string,bool>(m_SelectedSetData.GetName(),setName,active),true,null);
			m_SelectedSetData = null;
		}
	}
	
	void CreateNewSet(string setName, bool active)
	{
		//Send RPC to add new set
		GetRPCManager().SendRPC("RPC_BuildingSetManager", "RemoteAddNewSet", new Param2<string,bool>(setName,active),true,null);
	}
	
	bool CheckDuplicateSet(string setName)
	{
		foreach(BuildingSetEntry entry : m_BuildingSetEntries){
			if (entry != null)
				return entry.GetSetName() == setName;
		}
		return false;
	}
	
	void CreateSetEditor(bool editMode = false)
	{
		if (m_setAttributesEditor == null)
			if (editMode)
				m_setAttributesEditor = new BuildingSetEditor(M_SUB_WIDGET.FindAnyWidget("PanelConfirmationBox"), m_SelectedSetData.GetName(), m_SelectedSetData.GetActive(), editMode);
			else
				m_setAttributesEditor = new BuildingSetEditor(M_SUB_WIDGET.FindAnyWidget("PanelConfirmationBox"), "NewSet", true, false);
		else
			GetVPPUIManager().DisplayError("Attributes editor already showing!");
	}
	
	BuildingTracker GetTrackerByObject(Object obj)
	{
		autoptr BuildingTracker tracker;
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null){
				tracker = entry.GetTracker();
				if (tracker != null && tracker.GetTrackingObject() == obj)
				{
					return tracker;
				}
			}
		}
		return null;
	}
	
	void DeselectAllTrackers()
	{
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null && entry.GetTracker() != null){
				entry.GetTracker().Highlight(false);
			}
		}
	}
	
	void UpdateTrackerDetails()
	{
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null){
				entry.GetTracker().UpdateDataBoxes(true);
			}
		}
	}
	
	void DeselectAllSets()
	{
		foreach(BuildingSetEntry entry : m_BuildingSetEntries){
			if (entry != null)
				entry.GetCheckBox().SetChecked(false);
		}
	}
	
	BuildingSetEntry GetSelectedSet()
	{
		foreach(BuildingSetEntry entry : m_BuildingSetEntries)
		{
			if (entry != null && entry.IsSelected())
				return entry;
		}
		return null;
	}
	
	void InitSetsList()
	{
		foreach(CustomGridSpacer cusGrid : m_DataGrids){
			if (cusGrid != null)
				delete cusGrid;
		}
		m_DataGrids = new array<ref CustomGridSpacer>;
		
		//init first "page"
		m_DataGrids.Insert(new CustomGridSpacer(m_ParentGrid));
		m_LastGrid = m_DataGrids[0];
	}
	
	void InitSetItemsList()
	{
		foreach(CustomGridSpacer cusGrid : m_DataGridsItems){
			if (cusGrid != null)
				delete cusGrid;
		}
		m_DataGridsItems = new array<ref CustomGridSpacer>;
		
		//init first "page"
		m_DataGridsItems.Insert(new CustomGridSpacer(m_ParentGridItems));
		m_LastGridItems = m_DataGridsItems[0];
	}
	
	void ClearBuildingSetEntries()
	{
		foreach(BuildingSetEntry entry : m_BuildingSetEntries){
			if (entry != null){
				m_BuildingSetEntries.RemoveItem(entry);
				delete entry;
			}
		}
		m_BuildingSetEntries = new array<ref BuildingSetEntry>;
		InitSetsList();
	}
	
	void ClearBuildingEntries()
	{
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null){
				m_BuildingEntries.RemoveItem(entry);
				delete entry;
			}
		}
		m_BuildingEntries = new array<ref BuildingEntry>;
		InitSetItemsList();
	}
	
	void AddBuildingSetEntry(string buildSetName)
	{
		if(m_LastGrid.GetContentCount() == 100){
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100){
			ref BuildingSetEntry entry = new BuildingSetEntry(m_LastGrid.GetGrid(),M_SUB_WIDGET,buildSetName);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_BuildingSetEntries.Insert(entry);
		}
		
		m_ParentGrid.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void AddBuildingEntry(string itemDisplayName, string networkId, Object localObj = null)
	{
		if(m_LastGridItems.GetContentCount() == 100){
			m_LastGridItems = new CustomGridSpacer(m_ParentGridItems);
		 	m_DataGridsItems.Insert(m_LastGridItems);
		}
		
		if(m_LastGridItems.GetContentCount() < 100){
			ref BuildingEntry entry;
			if (localObj != null)
				entry = new BuildingEntry(m_LastGridItems.GetGrid(),M_SUB_WIDGET,itemDisplayName,networkId,localObj);
			else
				entry = new BuildingEntry(m_LastGridItems.GetGrid(),M_SUB_WIDGET,itemDisplayName,networkId);
			
			m_LastGridItems.AddWidget(entry.m_EntryBox);
			m_BuildingEntries.Insert(entry);
		}
		
		m_ParentGridItems.Update();
		m_LastGridItems.GetGrid().Update();
	}
	
	/*
	\ RPC Section
	*/
	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Client)
		{
			ClearBuildingSetEntries();
			ClearBuildingEntries();
			m_BuildingSets = data.param1;
			foreach(string setName : m_BuildingSets)
				AddBuildingSetEntry(setName);
		}
	}
	
	void HandleSetsData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref BuildingSet> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Client)
		{
			if (m_SelectedSetData != null)
				delete m_SelectedSetData;
			
			m_SelectedSetData = data.param1;
			ClearBuildingEntries();
			autoptr array<ref SpawnedBuilding> buildings = new array<ref SpawnedBuilding>;
			m_SelectedSetData.GetSpawnedBuildings(buildings);
			foreach(ref SpawnedBuilding building: buildings)
				AddBuildingEntry(building.GetTypeName(),building.GetNetworkId());
		}
	}
	/*
	\------------
	*/
	
	void FixCollide( Object obj )
	{
	    if ( obj == null ) return;
	
	    vector roll = obj.GetOrientation();
	    roll[2] = roll[2] - 1;
	    obj.SetOrientation( roll );
	    roll[2] = roll[2] + 1;
	    obj.SetOrientation( roll );
	}
	
	void UpdateFilter()
	{
		m_ItemListBox.ClearItems();

        TStringArray cfgPaths = new TStringArray;
        cfgPaths.Insert( "CfgVehicles" );

        string strSearch = m_SearchInputBox.GetText();
        strSearch.ToLower();

        for (int x = 0; x < cfgPaths.Count(); ++x)
        {
            string Config_Path = cfgPaths.Get(x);
            int nClasses = g_Game.ConfigGetChildrenCount(Config_Path);

            for ( int nClass = 62; nClass < nClasses; ++nClass )
            {
                string strName;
                GetGame().ConfigGetChildName( Config_Path, nClass, strName );

                int scope = GetGame().ConfigGetInt( Config_Path + " " + strName + " scope" );

                if ( scope == 0 || scope == 2)
                    continue;

                string lowerCasedName = strName;
                lowerCasedName.ToLower();

                if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
                {
                    continue;
                }
                m_ItemListBox.AddItem( strName, NULL, 0 );
            }
        }
	}
	
	void UpdatePreviewWidget()
	{
		if (!IsSubMenuVisible() && !m_loaded) return;
		
		int oRow = m_ItemListBox.GetSelectedRow();
		string ItemClassName;
		
		if (oRow != -1 && oRow != m_prevRow)
		{
			m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
			if (GetGame().IsKindOf( ItemClassName, "dz_lightai" )) return;
			
			if (m_PreviewObject != null)
			GetGame().ObjectDelete(m_PreviewObject);
			
			m_PreviewObject = EntityAI.Cast(GetGame().CreateObject(ItemClassName,vector.Zero,true,false,false));
			if (m_PreviewObject != null)
			{
				m_ItemPreview.SetItem( m_PreviewObject );
				m_ItemPreview.SetModelPosition( Vector(0,0,0.5) );
				m_ItemPreview.SetModelOrientation( Vector(0,0,0) );
				m_ItemPreview.SetView( m_ItemPreview.GetItem().GetViewIndex() );
				m_ItemPreview.Show(true);
			}else{
				m_ItemPreview.Show(false);
			}
			m_ItemOrientation = Vector(0,0,0);
			m_prevRow = oRow;
		}
	}
	
	void UpdateItemRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector orientation;
		if (m_ItemOrientation[0] == 0 && m_ItemOrientation[1] == 0 && m_ItemOrientation[2] == 0)
		{
			orientation        = m_ItemPreview.GetModelOrientation();
			m_ItemOrientation  = m_ItemPreview.GetModelOrientation();
		}
		else orientation = m_ItemOrientation;
		
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);

		m_ItemPreview.SetModelOrientation( orientation );

		if ( !is_dragging )
			m_ItemOrientation = orientation;
	}
		
	void ShowPanel(string panelName, bool state)
	{
		switch(panelName)
		{
			case "btnShowBuildingSets":
			HideChildren(m_PanelBuildingSets,state);
			break;
			
			case "btnShowAtts":
			HideChildren(m_PanelPlaceNew,state);
			break;
			
			case "btnShowCurrSet":
			HideChildren(m_PanelCurrentSet,state);
			break;
			
			case "btnShowCamOptions":
			HideChildren(m_PanelCamera,state);
			break;
		}
	}
	
	void HideChildren(Widget parent, bool state)
	{
		Widget child;
		if (state)
			parent.SetAlpha(1);
		else
			parent.SetAlpha(0);

		child = parent.GetChildren();
		if (!child.GetName().Contains("title"))
			child.Show(state);
		
		while (child)
		{
			if (!child.GetName().Contains("title")){
				child.Show(state);
			}
			child = child.GetSibling();
		}
	}
	
	Object CreateLocal(string type, vector pos)
	{
		return g_Game.CreateObject( type, pos, true );
	}
	
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		super.OnDoubleClick(w, x, y, button);
		if (w == m_ItemListBox)
		{
			int oRow = m_ItemListBox.GetSelectedRow();
			string ItemClassName;
			if (oRow != -1 && m_SelectedSetData != null)
			{
				m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
				int low, high;
				Object localObj = CreateLocal(ItemClassName,g_Game.GetCursorPos());
				localObj.GetNetworkID( low, high );
				AddBuildingEntry(ItemClassName, "0,0",localObj);
				m_SelectedSetData.AddBuildingObject(ItemClassName, localObj.GetPosition(), localObj.GetOrientation(), true, localObj);
				return true;
			}
		}
		return false;
	}
	
	override void UpdateWindowPosition(int mouse_x, int mouse_y, bool is_dragging )
	{
		//We Don't want to allow drag for this window
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		super.OnMouseButtonDown(w, x, y, button);
		if (w == m_ItemPreview && button == MouseState.LEFT)
		{
			GetGame().GetDragQueue().Call(this, "UpdateItemRotation");
			g_Game.GetMousePos(m_RotationX, m_RotationY);
			return true;
		}
		return false;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		foreach(ref Param2<bool,ref ButtonWidget> data : m_ShowHideButtons)
		{
			if (w == data.param2)
			{
				autoptr ImageWidget img = ImageWidget.Cast(w.GetChildren());
				if (data.param1)
				{
					img.SetColor(ARGB(255,0,255,0));
					img.LoadImageFile(0,"set:dayz_gui image:icon_collapse");
					data.param1 = false;
					ShowPanel(data.param2.GetName(),false);
				}
				else
				{
					img.SetColor(ARGB(255,255,0,0));
					img.LoadImageFile(0,"set:dayz_gui image:icon_expand");
					data.param1 = true;
					ShowPanel(data.param2.GetName(),true);
				}
				break;
			}
		}
		
		switch(w)
		{
			case m_btnReloadSets:
			if (m_SelectedSetData != null)
			{
				delete m_SelectedSetData;
				m_SelectedSetData = null;
			}
			GetRPCManager().SendRPC("RPC_BuildingSetManager", "GetBuildingSets", null,true,null);
			break;
			
			case m_chkFreeCam:
			//Toggle freecam
			if (m_Camera == null)
			{
				vector pos;
				MiscGameplayFunctions.GetHeadBonePos(PlayerBase.Cast(GetGame().GetPlayer()), pos);
				m_Camera = VPPFreeCam.Cast(g_Game.CreateObject( "VPPFreeCam", pos, true ));
				m_Camera.SetActive(true);
				m_chkTopDowncam.Enable(true);
			}else{
				m_Camera.SetActive(false);
				GetGame().GetUIManager().CloseAll();
				GetGame().ObjectDelete(m_Camera);
				m_chkTopDowncam.Enable(false);
				GetGame().SelectPlayer(null,GetGame().GetPlayer());
				
			}
			break;
			
			case m_chkTopDowncam:
			//Snap cam to topdown angle
			if (m_Camera != null)
				m_Camera.SnapToAngle(-89);
			break;
			
			case m_btnCreateNewSet:
				CreateSetEditor(false);
			break;
			
			case m_btnSaveChanges:
				m_SelectedSetData.UpdateBuildingsData();
				GetRPCManager().SendRPC("RPC_BuildingSetManager", "RemoteSaveEdits", new Param2<ref array<ref SpawnedBuilding>,string>(m_SelectedSetData.GetBuildings(),m_SelectedSetData.GetName()),true,null);
			    delete m_SelectedSetData;
			break;
		}
		return super.OnClick(w, x, y, button);;
	}
};