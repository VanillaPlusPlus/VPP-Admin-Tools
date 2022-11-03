class MenuObjectManager extends AdminHudSubMenu
{
	static const PhxInteractionLayers	HIT_MASK = PhxInteractionLayers.CHARACTER|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.VEHICLE|PhxInteractionLayers.ROADWAY|PhxInteractionLayers.TERRAIN|PhxInteractionLayers.ITEM_SMALL|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE|PhxInteractionLayers.AI;
	private bool 		      		 m_loaded = false;
	private EditBoxWidget     		 m_SearchInputBox;
	private TextListboxWidget 	  	 m_ItemListBox;
	private ItemPreviewWidget 		 m_ItemPreview;
	private EntityAI 	  	  		 m_PreviewObject;
	private ButtonWidget             m_btnCreateNewSet;
	private ButtonWidget             m_btnSaveChanges;
	private ButtonWidget             m_btnHelp;
	private CheckBoxWidget           m_chkSnapObjs;
	private CheckBoxWidget           m_chkGroundMode;
	private CheckBoxWidget           m_chkObjSurfaceSnap;
	private int 	 		  		 m_RotationX;
	private int 	 		  		 m_RotationY;
	private vector   		  		 m_ItemOrientation;
	private int                   	 m_searchBoxCount;
	private int    					 m_prevRow;
	private ref BuildingSetEditor    m_setAttributesEditor;
	private TextWidget               m_title_txt_curr_set;
	private EditBoxWidget 			 m_RotationStep;
	private ButtonWidget             m_btnReloadSets;
	private GridSpacerWidget         m_ParentGrid;
	private CheckBoxWidget           m_chkEnablePreview;
	private CheckBoxWidget           m_chkShowHideCards;
	private CheckBoxWidget 			 m_ChkFilterByScope;
	private ScrollWidget         	 m_ScrollerBuildingSets;
	private ScrollWidget         	 m_ScrollerSetItems;
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
	private Widget    m_PanelItemPreview;
	//------------
	private ref array<ref Param2<bool,ButtonWidget>> m_ShowHideButtons;
	private ref array<string> m_BuildingSets; //Names of all saved sets (from server)
	BuildingTracker m_SelectedParent;
	int keyMsDelay = 0;

	void MenuObjectManager()
	{
		/* RPCs */
		GetRPCManager().AddRPC("RPC_MenuObjectManager", "HandleData", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuObjectManager", "HandleSetsData", this, SingleplayerExecutionType.Client);
		//-------
		
		m_DataGrids 	  	 = new array<ref CustomGridSpacer>;
		m_DataGridsItems  	 = new array<ref CustomGridSpacer>;
		m_BuildingSetEntries = new array<ref BuildingSetEntry>;
		m_BuildingEntries    = new array<ref BuildingEntry>;
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuObjectManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget("Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget("BtnClose") );
		
		m_title_txt_curr_set = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("title_txt_curr_set"));
		m_ScrollerBuildingSets = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerBuildingSets"));
		m_ScrollerSetItems = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerSetItems"));
		m_RotationStep     = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("RotationStep"));
		m_chkSnapObjs  = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkSnapObjs"));
		m_chkGroundMode = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkEnableNoClip"));
		m_chkObjSurfaceSnap = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkObjSurfaceSnap"));
		m_btnSaveChanges  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnSaveChanges"));
		m_btnHelp  	= ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnHelp"));
	    m_btnCreateNewSet = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnCreateNewSet"));
		m_chkEnablePreview = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkEnablePreview"));
		m_chkShowHideCards = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkShowHideCards"));
		m_ChkFilterByScope = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkFilterByScope"));
		
		m_ShowHideButtons = new array<ref Param2<bool,ButtonWidget>>;
		m_SearchInputBox = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_ItemListBox    = TextListboxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemListBox") );
		m_ItemPreview    = ItemPreviewWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ItemPreview") );
		m_btnReloadSets  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnReloadSets"));
		m_ShowHideButtons.Insert(new Param2<bool,ButtonWidget>(true,ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnShowBuildingSets"))));
		m_ShowHideButtons.Insert(new Param2<bool,ButtonWidget>(true,ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnShowCurrSet"))));
		m_ShowHideButtons.Insert(new Param2<bool,ButtonWidget>(true,ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnShowCamOptions"))));
		
		m_ParentGrid      = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ParentGrid") );
		m_ParentGridItems = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ParentGridItems") );
		
		//---Panels---
		m_PanelBuildingSets  = M_SUB_WIDGET.FindAnyWidget("PanelBuildingSets");
		m_PanelCurrentSet    = M_SUB_WIDGET.FindAnyWidget("PanelCurrentSet");
		m_PanelItemPreview	 = M_SUB_WIDGET.FindAnyWidget("PanelItemPreview");
		m_PanelItemPreview.Show(false);
		//------------
		
		//Get Data from Server via RPC 
		GetRPCManager().VSendRPC("RPC_BuildingSetManager", "GetBuildingSets", null,true,null);
		//--
		
		UpdateFilter();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdatePreviewWidget, 100, true);
		m_loaded = true;
	}

	float Normalize(float a)
	{
		a = Math.ModFloat(a, 360.0); //calculate remainder between x, 360.0
		if (a < 0)
			a += 360.0;

		return a;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() || !m_loaded) return;
		
		Input input = GetGame().GetInput();
		
		if (m_SelectedSetData != null)
			m_btnSaveChanges.Enable(true);
		else
			m_btnSaveChanges.Enable(false);
		
		int newSrchCount = m_SearchInputBox.GetText().Length();
		if (newSrchCount != m_searchBoxCount)
		{
			//Update Filter
			UpdateFilter();
			m_searchBoxCount = newSrchCount;
		}

		//XML event groups copy
		if (input.LocalPress("UACopyPositionClipboard", false) && g_Game.IsLShiftHolding())
		{
			if (GetSelectedParent())
			{
				Object pObj = GetSelectedParent().GetTrackingObject();
				vector parentPos = pObj.GetPosition();
				vector originLocal = pObj.CoordToLocal(pObj.GetPosition());
				float angleParent = Normalize(pObj.GetOrientation()[0]);

				string outPut = string.Format("\t<!--<pos x=\"%1\" z=\"%2\" a=\"0\" y=\"%3\" group=\"GROUP_NAME\"/>-->\n", parentPos[0], parentPos[2], parentPos[1]);
				outPut += "\t<group name=\"GROUP_NAME_HERE\">\n";
				outPut += string.Format("\t\t<child type=\"%1\" deloot=\"0\" lootmax=\"3\" lootmin=\"1\" x=\"%2\" z=\"%3\" y=\"%4\" a=\"%5\"/>\n", pObj.GetType(), originLocal[0], originLocal[2], originLocal[1], angleParent);
				
				array<ref SpawnedBuilding> buildings_ = m_SelectedSetData.GetBuildings();
				foreach(SpawnedBuilding childObj: buildings_)
				{
					if (!childObj.GetObject() || childObj.GetObject() == pObj)
						continue;

					BuildingTracker tracker_ = GetTrackerByObject(childObj.GetObject());
					if (!tracker_ || !tracker_.IsSelected())
						continue;

					vector cPos = childObj.GetObject().GetPosition() - parentPos;
					float angle = Normalize(childObj.GetObject().GetOrientation()[0]);
					outPut += string.Format("\t\t<child type=\"%1\" deloot=\"0\" lootmax=\"3\" lootmin=\"1\" x=\"%2\" z=\"%3\" y=\"%4\" a=\"%5\"/>\n", childObj.GetObject().GetType(), cPos[0], cPos[2], cPos[1], angle);
				}
				outPut += "\t</group>";
				GetGame().CopyToClipboard(outPut);
				GetVPPUIManager().DisplayNotification("Successfully copied XML (eventgroups) formatting of objects to clipboard!", "Success", 3.0);
			}
		}

		//Deletion
		if (input.LocalPress("UADeleteObjCrosshair", false))
		{
			int total = 0;
			foreach(BuildingEntry entry : m_BuildingEntries)
			{
				if (total >= 1)
					break;

				if (!entry || !entry.GetTracker() || !entry.GetTracker().IsSelected())
					continue;

				total++;
			}

			if (total <= 0)
				return;

			VPPDialogBox dialogBox = GetVPPUIManager().CreateDialogBox(NULL, true);
			dialogBox.InitDiagBox(DIAGTYPE.DIAG_YESNO, "Delete Selected", "Are you sure you want to delete selected object(s)?", this, "OnDiagResultDeleteSlected");
		}

		//Movement part
		if (input.LocalHold("UASelectObject", false) && GetSelectedParent())
		{
			if (GetVPPUIManager().IsSelectionBoxDrawing() || g_Game.IsLeftCtrlDown())
				return;

			Widget w = GetWidgetUnderCursor();
			bool valid = (w == NULL || (w && w.GetName() == "BtnSelect") || (w && w.GetName() == "rootFrame"))
			if (!valid)
				return;

			Object worldObject = GetSelectedParent().GetTrackingObject();
			if (!worldObject)
				return;

			vector tm[4];
			worldObject.GetTransform(tm);
			vector ray_start = GetGame().GetCurrentCameraPosition();
			vector ray_end   = ray_start + GetGame().GetPointerDirection() * 10000;

			vector cursor_pos;
			vector cursor_dir;
			int    c_component;
			DayZPhysics.RaycastRV(ray_start, ray_end, cursor_pos, cursor_dir, c_component, null, GetGame().GetPlayer(), worldObject, false, m_chkGroundMode.IsChecked());

			vector minMax[2];
			worldObject.GetCollisionBox(minMax);
			vector offSetPos = worldObject.GetPosition();

			if (g_Game.IsLeftAltHolding()) //Z-Only axis
			{
				cursor_pos = ray_start + GetGame().GetPointerDirection() * vector.Distance(GetGame().GetCurrentCameraPosition(), worldObject.GetPosition());
				tm[3][1] = cursor_pos[1];
			}
			else if (g_Game.IsLShiftHolding()) //Rotation
			{
				vector rotation = worldObject.GetOrientation();
				float lr = input.LocalValue("UARotateLeft") - input.LocalValue("UARotateRight");
				float step = m_RotationStep.GetText().ToFloat();
				if (step <= 0.0)
					step = 120.0;

				rotation[0] = rotation[0] + (step * lr * timeslice);
				rotation.RotationMatrixFromAngles(tm);
				UpdateTrackerDetails();
			}
			else
			{
				if (m_chkObjSurfaceSnap.IsChecked()) //Surface snap mode (precise)
				{
					DayZPhysics.RayCastBullet(ray_start, ray_end, HIT_MASK, worldObject, NULL, cursor_pos, NULL, NULL);
					if (cursor_pos[1] <= 0)
						cursor_pos[1] = GetGame().SurfaceY(offSetPos[0], offSetPos[2]);
					
					cursor_pos[1] = cursor_pos[1] + ((minMax[1][1] - minMax[0][1]) / 2);
				}
				else if (m_chkSnapObjs.IsChecked()) //Terrain snap mode
				{
					cursor_pos[1] = GetGame().SurfaceY(tm[3][0], tm[3][2]) + ((minMax[1][1] - minMax[0][1]) / 2);
				}
				else
				{
					cursor_pos[1] = worldObject.GetPosition()[1]; //default
				}
				tm[3] = cursor_pos;
			}
			
			worldObject.SetTransform(tm);
			offSetPos = tm[3] - offSetPos;
			
			array<ref SpawnedBuilding> buildings = m_SelectedSetData.GetBuildings();
			foreach(SpawnedBuilding bld: buildings)
			{
				if (!bld.GetObject() || bld.GetObject() == worldObject)
					continue;

				BuildingTracker tracker = GetTrackerByObject(bld.GetObject());
				if (!tracker || !tracker.IsSelected())
					continue;

				//Z-Only axis
				if (g_Game.IsLeftAltHolding())
				{
					bld.GetObject().SetPosition(bld.GetObject().GetPosition() + offSetPos);
				}
				else if (g_Game.IsLShiftHolding()) //Rotation axis
				{
					bld.GetObject().SetOrientation(worldObject.GetOrientation());
				}

				//position
				if (!g_Game.IsLeftAltHolding() && !g_Game.IsLShiftHolding())
				{
					bld.GetObject().GetCollisionBox(minMax);
					vector childPos = bld.GetObject().GetPosition() + offSetPos;

					if (m_chkObjSurfaceSnap.IsChecked())
					{
						vector start_c = Vector(childPos[0], tm[3][1], childPos[2]);
						vector end_c   = Vector(childPos[0], GetGame().SurfaceY(childPos[0], childPos[2]), childPos[2]);
						vector hitPos;
						DayZPhysics.RayCastBullet(start_c, end_c, HIT_MASK, bld.GetObject(), NULL, hitPos, NULL, NULL);

						childPos[1] = hitPos[1];

						if (childPos[1] <= 0)
							childPos[1] = end_c[1];

						childPos[1] = childPos[1] + ((minMax[1][1] - minMax[0][1]) / 2);
					}
					else if (m_chkSnapObjs.IsChecked())
					{
						childPos[1] = GetGame().SurfaceY(childPos[0], childPos[2]) + ((minMax[1][1] - minMax[0][1]) / 2);
					}
					bld.GetObject().SetPosition(childPos);
				}
			}
		}

		//Deselection part
		//Check bind doesn't collide with "hold <bind> to multi-select"
		if (input.LocalPress("UADeSelectObject", false))
		{
			keyMsDelay = GetGame().GetTime();
		}
		else if (input.LocalRelease("UADeSelectObject", false) && (GetGame().GetTime() - keyMsDelay) < 255)
		{
			DeselectAllTrackers();
		}
	}
	
	override void OnMenuShow()
	{
		super.OnMenuShow();
		GetToolbarMenu().AllowSelectBoxDraw(true);
		GetToolbarMenu().HideIconsPanel(true);
	}

	override void OnMenuHide()
	{
		super.OnMenuHide();
		GetToolbarMenu().AllowSelectBoxDraw(false);
		GetToolbarMenu().HideIconsPanel(false);
	}

	void OnDiagResultDeleteSlected(int result)
    {
 		if (result == DIAGRESULT.YES)
        {
        	foreach(BuildingEntry entry : m_BuildingEntries)
			{
				if (!entry || !entry.GetTracker() || !entry.GetTracker().IsSelected())
					continue;

				entry.DeleteItem(DIAGRESULT.YES);
			}
        }
    }

	void UpdateBuildingAttributes(SpawnedBuilding target)
	{
		SpawnedBuilding building = m_SelectedSetData.GetSpawnedBuilding(target);
		if (target != null)
			building.SetActive(target.GetActive());
	}
	
	void SetSelectedObject(Object obj, bool showNotification = false, bool forceMultiSelect = false)
	{
		if (!obj)
			return;

		BuildingTracker tracker = GetTrackerByObject(obj);
		if (!tracker)
			return;

		if (!g_Game.IsLeftCtrlDown() && !forceMultiSelect)
		{
			DeselectAllTrackers();
		}

		tracker.Highlight(true);
		if (GetSelectedParent() == NULL)
		{
			SetSelectedParent(tracker);
		}

		if (showNotification){
			GetVPPUIManager().DisplayNotification("", "#VSTR_NOTIFY_OB_SELECTED"+ " " + obj, 0.5);
		}
	}
	
	void RemoveBuilding(Object toRemove, string networkdID = "")
	{
		SpawnedBuilding building = m_SelectedSetData.GetBuildingByType(toRemove);
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
			GetRPCManager().VSendRPC("RPC_BuildingSetManager", "RemoteDeleteSet", new Param1<string>(m_SelectedSetData.GetName()),true,null);
			m_SelectedSetData = null;
			SetSelectedParent(null);
		}
	}
	
	void UpdateBuildingSet(string setName, bool active)
	{
		if (m_SelectedSetData != null)
		{
			//Send RPC to update selected set
			GetRPCManager().VSendRPC("RPC_BuildingSetManager", "RemoteUpdateSet", new Param3<string,string,bool>(m_SelectedSetData.GetName(),setName,active),true,null);
			m_SelectedSetData = null;
			SetSelectedParent(null);
		}
	}
	
	void CreateNewSet(string setName, bool active)
	{
		//Send RPC to add new set
		GetRPCManager().VSendRPC("RPC_BuildingSetManager", "RemoteAddNewSet", new Param2<string,bool>(setName,active),true,null);
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
			GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_ATTS_SHOWING");
	}

	BuildingTracker GetSelectedParent()
	{
		return m_SelectedParent;
	}

	void SetSelectedParent(BuildingTracker t)
	{
		m_SelectedParent = t;
	}
	
	BuildingTracker GetTrackerByObject(Object obj)
	{
		BuildingTracker tracker;
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

	void ShowAllTrackers( bool show )
	{
		BuildingTracker tracker;
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null){
				tracker = entry.GetTracker();
				if (tracker != NULL)
				{
					entry.SetSelected(show);
					tracker.ShowTracker(show);
				}
			}
		}
	}
	
	void DeselectAllTrackers()
	{
		SetSelectedParent(NULL);
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry && entry.GetTracker()){
				entry.GetTracker().Highlight(false);
			}
		}
	}
	
	void UpdateTrackerDetails()
	{
		foreach(BuildingEntry entry : m_BuildingEntries){
			if (entry != null){
				if (entry.GetTracker())
				{
					entry.GetTracker().UpdateDataBoxes(true);
				}
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
			BuildingSetEntry entry = new BuildingSetEntry(m_LastGrid.GetGrid(),M_SUB_WIDGET,buildSetName);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_BuildingSetEntries.Insert(entry);
		}
		
		m_ParentGrid.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void AddBuildingEntry(string itemDisplayName, string networkId, SpawnedBuilding buildInfo = null, Object localObj = null)
	{
		if(m_LastGridItems.GetContentCount() == 100){
			m_LastGridItems = new CustomGridSpacer(m_ParentGridItems);
		 	m_DataGridsItems.Insert(m_LastGridItems);
		}
		
		if(m_LastGridItems.GetContentCount() < 100){
			BuildingEntry entry;
			if (localObj != null){
				entry = new BuildingEntry(m_LastGridItems.GetGrid(), M_SUB_WIDGET, itemDisplayName, networkId, buildInfo, localObj);
			}else{
				entry = new BuildingEntry(m_LastGridItems.GetGrid(), M_SUB_WIDGET, itemDisplayName, networkId, buildInfo);
			}
			
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
			m_title_txt_curr_set.SetText("Building Set: "+m_SelectedSetData.GetName());
			ClearBuildingEntries();
			array<ref SpawnedBuilding> buildings = new array<ref SpawnedBuilding>;
			m_SelectedSetData.GetSpawnedBuildings(buildings);
			foreach(SpawnedBuilding building: buildings)
				AddBuildingEntry(building.GetTypeName(), building.GetNetworkId(), building);
		}
	}
	/*
	\------------
	*/
	
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

            for ( int nClass = 0; nClass < nClasses; ++nClass )
            {
                string strName;
                GetGame().ConfigGetChildName( Config_Path, nClass, strName );

                int scope = GetGame().ConfigGetInt( Config_Path + " " + strName + " scope" );

                string base_name;
                bool baseType = GetGame().ConfigGetBaseName(Config_Path + " " + strName, base_name);
                base_name.ToLower();

                if ( m_ChkFilterByScope.IsChecked() )
                {
                	if ( scope != 1 )
                		continue;
                }
                else
                {
                	if ( base_name != "housenodestruct" )
                    	continue;
                }

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
		if (!IsSubMenuVisible() && !m_loaded)
			return;
		
		Widget widget_under_mouse = GetWidgetUnderCursor();

		if (m_chkEnablePreview.IsChecked() && (widget_under_mouse && widget_under_mouse.GetName() == "ItemListBox"))
		{
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
			m_PanelItemPreview.Show(true);
		}
		else
		{
			m_PanelItemPreview.Show(false);
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
			
			case "btnShowCurrSet":
			HideChildren(m_PanelCurrentSet,state);
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
		Object obj = GetGame().CreateObjectEx(type, pos, ECE_LOCAL);
		vector minMax[2];
		obj.GetCollisionBox(minMax);
		obj.SetPosition(Vector(pos[0], pos[1] + minMax[1][1] ,pos[2]));

		return obj;
	}
	
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		int oRow = m_ItemListBox.GetSelectedRow();
		string ItemClassName;
		if (m_SelectedSetData == null)
		{
			GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_NOSET_SELECTED");
			return false;
		}
		
		if (!m_SelectedSetData.GetActive())
		{
			GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_SET_INACTIVE");
			return false;
		}
		
		vector spawnPos;
		if (w == m_ItemListBox)
			spawnPos = g_Game.GetCursorPos();
		else
			spawnPos = g_Game.GetPosByCursor();
		
		Widget widget_under_mouse = GetWidgetUnderCursor();
		string wName = widget_under_mouse.GetName();
		if (oRow > -1 && m_SelectedSetData)
		{
			if (GetSelectedParent() && !g_Game.IsLeftCtrlDown())
			{
				DeselectAllTrackers();
			}

			if (wName == m_ItemListBox.GetName() || wName == "rootFrame")
			{
				m_ItemListBox.GetItemText(oRow, 0, ItemClassName);
				int low, high;
				Object localObj = CreateLocal(ItemClassName,spawnPos);
				localObj.GetNetworkID(low, high);
				AddBuildingEntry(ItemClassName, "0,0", m_SelectedSetData.AddBuildingObject(ItemClassName, localObj.GetPosition(), localObj.GetOrientation(), true, localObj), localObj);
				SetSelectedObject(localObj, false, true);
				return true;
			}
		}
		return super.OnDoubleClick(w, x, y, button);
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
		foreach(Param2<bool,ButtonWidget> data : m_ShowHideButtons)
		{
			if (w != null && w == data.param2)
			{
				if (w != null && w.GetChildren() != null)
				{
					ImageWidget img = ImageWidget.Cast(w.GetChildren());
					if (img == null) return false;
					
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
		}
		
		switch(w)
		{
			case m_btnReloadSets:
			if (m_SelectedSetData != null)
			{
				if (!m_SelectedSetData.GetActive())
				{
					m_SelectedSetData.ClearBuildings();
				}
				m_SelectedSetData = null;
				SetSelectedParent(null);
			}
			GetRPCManager().VSendRPC("RPC_BuildingSetManager", "GetBuildingSets", null,true,null);
			break;
			
			case m_btnCreateNewSet:
				CreateSetEditor(false);
			break;
			
			case m_btnSaveChanges:
				m_SelectedSetData.UpdateBuildingsData();
				GetRPCManager().VSendRPC("RPC_BuildingSetManager", "RemoteSaveEdits", new Param2<ref array<ref SpawnedBuilding>,string>(m_SelectedSetData.GetBuildings(),m_SelectedSetData.GetName()),true,null);
			    GetRPCManager().VSendRPC("RPC_BuildingSetManager", "RemoteUpdateSet", new Param3<string,string,bool>(m_SelectedSetData.GetName(),m_SelectedSetData.GetName(),m_SelectedSetData.GetActive()),true,null);
				delete m_SelectedSetData;
				m_SelectedSetData = null;
				SetSelectedParent(null);
			break;
			
			case m_btnHelp:
			string objhelp = "#VSTR_TOOLTIP_HELP_OBJMANAGER";
			
			GetVPPUIManager().DisplayNotification(objhelp,"Object Builder",15.0);
			break;

			case m_chkShowHideCards:
			if ( m_SelectedSetData != NULL )
			{
				ShowAllTrackers( m_chkShowHideCards.IsChecked() );
			}
			break;

			case m_ChkFilterByScope:
			UpdateFilter();
			break;
		}
		return super.OnClick(w, x, y, button);;
	}
};
