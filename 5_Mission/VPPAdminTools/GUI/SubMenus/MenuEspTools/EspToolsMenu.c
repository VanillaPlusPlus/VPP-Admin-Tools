class EspToolsMenu extends AdminHudSubMenu
{
	private int M_UPDATE_INTERVAL = 1;
	private float            m_UpdateTick;
	private bool             m_Init;
	private bool             M_SCAN_ACTIVE = false;
	private GridSpacerWidget m_SpacerParent; 	//Filters Spacer
	private GridSpacerWidget m_SpacerParentItems; //ESP items selected list
	private ref CustomGridSpacer m_LastGrid;
	private SliderWidget     m_SliderRadius;
	private ButtonWidget     m_btnToggle;
	private ButtonWidget     m_btnClear;
	private ButtonWidget     m_removeAllItems;
	private ButtonWidget     m_delAllItems;
	private ButtonWidget	 m_btnDelRadius;
	private ButtonWidget     m_btnAddNewFilter;
	private ButtonWidget     m_btnRestore;
	private CheckBoxWidget   m_chkSelectAll; //Filter checkbox
	CheckBoxWidget   		  m_ChkShowClassName;
	CheckBoxWidget 			 m_ChkShowDeadPlayers;
	private EditBoxWidget    m_InputUpdateInterval;
	private ImageWidget      m_ImgInfo;
	private ScrollWidget     m_ScrollerItems;
	private ScrollWidget     m_Scroller;
	
	ref array<ref VPPFilterEntry>   m_FilterEntry;
	ref array<ref VPPESPItemEntry>  m_EspItemsEntry;
	ref map<Object, ref VPPESPTracker>	  m_EspTrackers;
	ref array<ref CustomGridSpacer> m_DataGrids;
	ref array<ref EspFilterProperties> m_FilterProps;
	
	void EspToolsMenu()
	{
		m_FilterEntry   = new array<ref VPPFilterEntry>;
		m_EspItemsEntry = new array<ref VPPESPItemEntry>;
		m_DataGrids     = new array<ref CustomGridSpacer>;
		m_EspTrackers	= new map<Object, ref VPPESPTracker>;
		m_FilterProps   = new array<ref EspFilterProperties>;
	}

	void ~EspToolsMenu()
	{
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/EspToolsUI/EspToolsMenu.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_SpacerParentItems = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerParentItems"));
		m_SpacerParent = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerParent"));
		m_SliderRadius = SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderRadius"));
		m_btnToggle    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnToggle"));
		m_btnClear	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnClear"));
		m_btnRestore   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnRestore"));
		GetVPPUIManager().HookConfirmationDialog(m_btnRestore, M_SUB_WIDGET,this,"RestoreFilters", DIAGTYPE.DIAG_YESNO, "#VSTR_ESP_FILTER_RESTORE_TITLE", "#VSTR_ESP_FILTER_RESTORE");
		
		m_btnAddNewFilter = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnAddNewFilter"));
		GetVPPUIManager().HookConfirmationDialog(m_btnAddNewFilter, M_SUB_WIDGET,this,"CreateNewFilter", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_ESP_CREATE_FILTER", "#VSTR_ESP_FILTER_INFO", true);
		
		m_removeAllItems = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "removeAllItems"));
		m_delAllItems = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "delAllItems"));
		GetVPPUIManager().HookConfirmationDialog(m_delAllItems, M_SUB_WIDGET,this,"ConfirmDeleteAll", DIAGTYPE.DIAG_YESNO, "#VSTR_ESP_TITLE_DELETE", "#VSTR_ESP_DEL_CONFIRM");
		m_chkSelectAll = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkSelectAll"));
		m_ChkShowClassName = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkShowClassName"));
		m_ChkShowDeadPlayers = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkShowDeadPlayers"));
		m_InputUpdateInterval = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InputUpdateInterval"));
		m_ImgInfo = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfo"));
		
		m_btnDelRadius = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnDelRadius"));
		GetVPPUIManager().HookConfirmationDialog(m_btnDelRadius, M_SUB_WIDGET, this, "ScanDeleteInRadius", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Scan and delete", "Please enter a object class name to scan for, otherwise keep empty to scan all. Radius value is used from the slider.", true);

		m_ScrollerItems = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerItems"));
		m_Scroller = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("Scroller"));
		
		ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_ESP_TOOLTIP");
		
		LoadSavedFilters();
		InitEspItemsList();
		
		m_Init = true;
		//init thread
		thread UpdateEsp();
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_ScrollerItems.Show(!state);
		m_Scroller.Show(!state);
	}
	
	void UpdateEsp()
	{
		while (true)
		{
			if (!M_SCAN_ACTIVE)
				Sleep(1000);
			
			if (!GetGame().GetMission().IsMissionGameplay())
				break;
			
			if (M_SCAN_ACTIVE)
			{
				int totalTime = 0;
				map<Object, int> objects = new map<Object, int>; //This seems to work faster than filtering arrays with Find
				
				M_UPDATE_INTERVAL = m_InputUpdateInterval.GetText().ToInt();
				if (M_UPDATE_INTERVAL <= 0)
					M_UPDATE_INTERVAL = 1;
				
				ScanObjects(objects, totalTime);
				ScanPlayers();
				
				map<typename,int> activeFilters = new map<typename,int>;
				foreach(VPPFilterEntry filter : m_FilterEntry)
				{
					if (!filter.IsSelected())
						continue;
		
					activeFilters.Insert(filter.GetFilterName().ToType(), filter.m_Props.color);
				}
		
				foreach(Object obj, int useless : objects)
				{
					if (!obj)
						continue;
					if (!obj.HasNetworkID())
						continue;
		
					string objName = obj.GetDisplayName();
					if (m_ChkShowClassName.IsChecked() || objName == string.Empty)
						objName = obj.GetType();
		
					foreach(typename f, int color : activeFilters)
					{
						if (obj.IsInherited(f))
						{
							m_EspTrackers.Insert(obj, new VPPESPTracker(objName, obj, color));
						}
					}
				}

				Sleep(Math.Max(0,(M_UPDATE_INTERVAL * 1000.0) - totalTime)); //Next scan
				#ifdef VPPADMINTOOLS_DEBUG
				//Print("Total: " + objects.Count() + " totalTime: " + totalTime);
				#endif
			}
		}
	}
	
	void DoWait(int time, out int totalTime)
	{
		Sleep(time);
		totalTime += time;
	}

	void DoSleep(int time, out int totalTime, inout int waitIndex)
	{
		waitIndex += 1;
		if (waitIndex % 5 == 0){
			Sleep(time);
			totalTime += time;
		}
	}
	
	void ScanObjects(out map<Object, int> objects, out int totalTime)
	{
		int currentIteration = 1;
		vector centerPos = GetGame().GetPlayer().GetPosition();
		if (IsFreeCamActive())
			centerPos = GetFreeCamInstance().GetPosition();
		
		centerPos[1] = GetGame().SurfaceY(centerPos[0], centerPos[2]);
		
		float maxRadiusChunk = m_SliderRadius.GetCurrent() / 8;
		int totalIterations  = Math.Ceil(m_SliderRadius.GetCurrent() / maxRadiusChunk);
		
		#ifdef VPPADMINTOOLS_DEBUG
		//visualize it
		//Debug.DrawSphere(centerPos, maxRadiusChunk * 3.5, ARGB(110,110,0,0), ShapeFlags.TRANSP|ShapeFlags.NOOUTLINE);
		#endif

		array<Object> objectsInChunk = new array<Object>;
		GetGame().GetObjectsAtPosition(centerPos, maxRadiusChunk * 3.5, objectsInChunk, NULL);

		foreach(Object obj : objectsInChunk){
			objects.Insert(obj, 0); //push into hashmap
		}
		objectsInChunk.Clear();

		int waitIndex = 0;
		
		//iterate over the chunks
		for (int i = 2; i <= totalIterations; ++i)
		{
			currentIteration++;
			int numSubChunks = i * i;
			
			for (int j = 1; j <= numSubChunks; ++j)
			{
				objectsInChunk.Clear();
				
				//Find center pos of chunk
				vector chunkCenterPos = vector.Zero;
				float angle = (Math.PI - (Math.PI2 * j / numSubChunks));
				float distance = i * maxRadiusChunk;
				chunkCenterPos = (centerPos + Vector(distance * Math.Cos(angle), 0, distance * Math.Sin(angle)));
				chunkCenterPos[1] = GetGame().SurfaceY(chunkCenterPos[0], chunkCenterPos[2]);
				GetGame().GetObjectsAtPosition(chunkCenterPos, maxRadiusChunk, objectsInChunk, NULL);
				
				#ifdef VPPADMINTOOLS_DEBUG
				//visualize it
				//Debug.DrawSphere(chunkCenterPos, maxRadiusChunk, ARGB(35,110,0,0), ShapeFlags.TRANSP|ShapeFlags.NOOUTLINE);
				#endif
				
				foreach(Object _obj : objectsInChunk){
					objects.Insert(_obj, 0); //push into hashmap
				}
				objectsInChunk.Clear();
				DoSleep(1, totalTime, waitIndex);
			}
		}
	}
	
	void ScanPlayers()
	{
		VPPFilterEntry survivorFilter = GetFilter("SurvivorBase");
		if (survivorFilter && survivorFilter.IsSelected())
		{
			array<Man> players = new array<Man>;
			players = ClientData.m_PlayerBaseList;
			
			vector startPos = GetGame().GetPlayer().GetPosition();
			if (IsFreeCamActive())
				startPos = GetFreeCamInstance().GetPosition();

			foreach(Man man : players)
			{
				if (!man)
					continue;
				if (!man.IsAlive() && !ShowDeadPlayers())
					continue;

				if (man.GetIdentity())
				{
					if (vector.Distance(startPos, man.GetPosition()) <= m_SliderRadius.GetCurrent())
					{
						m_EspTrackers.Insert(man, new VPPESPTracker(man.GetIdentity().GetName(), man, survivorFilter.m_Props.color));
					}
				}
			}
		}
	}
	
	void ExpandPlayerTrackerDropDowns()
	{
		if (m_EspTrackers && m_EspTrackers.Count() > 0){
			foreach(VPPESPTracker tracker : m_EspTrackers)
			{
				bool expand = SurvivorBase.Cast(tracker.GetTrackingObject()) || BaseBuildingBase.Cast(tracker.GetTrackingObject());
				if (!expand)
					continue;
				
				tracker.ExpandHeaders();
			}
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_btnToggle:
			if (M_SCAN_ACTIVE)
			{
				m_btnToggle.SetColor(ARGB(255,255,0,0));
				M_SCAN_ACTIVE = false;
				ClearTrackers();
			}else{
				M_SCAN_ACTIVE = true;
				m_btnToggle.SetColor(ARGB(255,0,255,0));
				GetRPCManager().SendRPC( "RPC_VPPESPTools", "ToggleESP", null, true, null); //for logging
			}
			break;
			
			case m_btnClear:
			ClearTrackers(); 
			return true;
			break;
			
			case m_removeAllItems:
			ClearItemEntries();
			break;
			
			case m_chkSelectAll:
			foreach(VPPFilterEntry filter : m_FilterEntry)
				filter.SetSelected(m_chkSelectAll.IsChecked());
			break;
		}
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_InputUpdateInterval)
		{
			SetFocus(null);
			return true;
		}
		return false;
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		super.OnChange(w, x, y, finished);
		if (w == m_ChkShowClassName)
		{
			ClearTrackers();
			return true;
		}
		return false;
	}

	void ScanDeleteInRadius(int result, string objectType)
	{
		if (result == DIAGRESULT.OK)
		{
			objectType.ToLower();
			ClearItemEntries();
			if (m_SliderRadius.GetCurrent() > 301)
			{
				GetVPPUIManager().DisplayNotification("Could not start scan, radius too high! Maximum 300 meters.");
				return;
			}

			if (!GetGame().GetPlayer())
				return;

			//scan for nearby items
			vector startPos = GetGame().GetPlayer().GetPosition();
			if (IsFreeCamActive())
			{
				startPos = GetFreeCamInstance().GetPosition();
			}
			
			array<Object> objects = {};
			GetGame().GetObjectsAtPosition(startPos, m_SliderRadius.GetCurrent(), objects, null);
			if (objects)
			{
				foreach(Object obj : objects)
				{
					if (obj.IsBuilding() && !obj.IsInherited(GardenBase))
						continue;
					if (obj.IsRock())
						continue;
					if (obj.IsWoodBase())
						continue;
					if (!obj.HasNetworkID())
						continue;
					if (obj.IsBush())
						continue;
					if (obj.IsTree())
						continue;
					if (EntityAI.Cast(obj).IsPlayer())
						continue;
					if (objectType == string.Empty){
						AddEntry(obj.GetType(), obj);
						continue;
					}
					if (objectType != string.Empty && obj.IsKindOf(objectType)){
						AddEntry(obj.GetType(), obj);
					}
				}
			}
		}
	}
	
	void RestoreFilters(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			MakeDefaultFilters();
			GetVPPUIManager().DisplayNotification("Added Default filters!");
		}
	}
	
	void CreateNewFilter(int result, string userInput)
	{
		if (result == DIAGRESULT.OK && userInput != "")
		{
			m_FilterProps.Insert( new EspFilterProperties(userInput,ARGB(255,255,255,255)));
			SaveFilters();
			GetVPPUIManager().DisplayNotification("New filter added & saved!");
			
			m_FilterEntry   = new array<ref VPPFilterEntry>;
			m_FilterProps   = new array<ref EspFilterProperties>;
			
			LoadSavedFilters();
		}
	}

	void ConfirmDeleteAll(int result)
	{
		if (result == DIAGRESULT.YES)
			DeleteESPItems(null,true);
	}
	
	VPPFilterEntry GetFilter(string name)
	{
		foreach(VPPFilterEntry filter : m_FilterEntry)
		{
			if(filter.GetFilterName() == name)
			{
				return filter;
			}
		}
		return null;
	}
		
	//DELETES & removes from list one or all selected items
	void DeleteESPItems(VPPESPItemEntry entry, bool all = false)
	{
		array<Object> ToDelete;
		if (all)
		{
			ToDelete = new array<Object>;
			foreach(VPPESPItemEntry itemEntry : m_EspItemsEntry)
			{
				if (itemEntry != null)
				{
					if (itemEntry.GetTargetObject() != null)
						ToDelete.Insert(itemEntry.GetTargetObject());
				}
			}
			GetRPCManager().SendRPC( "RPC_VPPESPTools", "DeleteItems", new Param1<ref array<Object>>(ToDelete),true,null);
			ClearItemEntries();
			return;
		}
		
		ToDelete = new array<Object>;
		if (entry.GetTargetObject() != null)
		{
			ToDelete.Insert(entry.GetTargetObject());
			GetRPCManager().SendRPC( "RPC_VPPESPTools", "DeleteItems", new Param1<ref array<Object>>(ToDelete),true,null);
		}
		int index = m_EspItemsEntry.Find(entry);
		if (index > -1)
		{
			m_EspItemsEntry.RemoveItem(entry);
		}
	}
	
	//Removes item entry from list
	void RemoveEspItemEntry(VPPESPItemEntry entry)
	{
		int index = m_EspItemsEntry.Find(entry);
		if (index > -1)
		{
			VPPESPTracker tracker = m_EspTrackers.Get(tracker.GetTrackingObject());
			if (tracker)
			{
				tracker.SetChecked(false);
			}
			m_EspTrackers.Remove(tracker.GetTrackingObject());
			m_EspItemsEntry.RemoveItem(entry);
		}
	}
	
	void RemoveEntryByObject(Object obj)
	{
		foreach(VPPESPItemEntry entry : m_EspItemsEntry)
		{
			if (entry != null)
			{
				if (entry.GetTargetObject() == obj)
					RemoveEspItemEntry(entry);
			}
		}
	}
	
	bool CheckDuplicateTracker(Object obj)
	{
		return m_EspTrackers.Contains(obj);
	}
	
	void RemoveTracker(VPPESPTracker t)
	{
		VPPESPTracker tracker = m_EspTrackers.Get(t.GetTrackingObject());
		if (tracker == t){
			m_EspTrackers.Remove(t.GetTrackingObject());
		}
	}
	
	void ClearTrackers()
	{
		m_EspTrackers.Clear();
		m_EspTrackers  = new map<Object, ref VPPESPTracker>;
	}
	
	void InitEspItemsList()
	{
		foreach(CustomGridSpacer cusGrid : m_DataGrids){
			if (cusGrid != null)
				delete cusGrid;
		}
		m_DataGrids = new array<ref CustomGridSpacer>;
		
		//init first "page"
		m_DataGrids.Insert(new CustomGridSpacer(m_SpacerParentItems));
		m_LastGrid = m_DataGrids[0];
	}
	
	void AddEntry(string itemDisplayName, Object target)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_SpacerParentItems);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100)
		{
			VPPESPItemEntry entry = new VPPESPItemEntry(m_LastGrid.GetGrid(),target,M_SUB_WIDGET,itemDisplayName);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_EspItemsEntry.Insert(entry);
		}
		
		m_SpacerParentItems.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void ClearItemEntries()
	{
		//Deselect ALL trackers
		foreach(Object o, VPPESPTracker tracker : m_EspTrackers)
		{
			if (tracker != null)
				tracker.SetChecked(false);
		}
		
		foreach(VPPESPItemEntry entry : m_EspItemsEntry)
		{
			if (entry != null)
			{
				m_EspItemsEntry.RemoveItem(entry);
				delete entry;
			}
		}
		m_EspItemsEntry = new array<ref VPPESPItemEntry>;
		InitEspItemsList();
	}
	
	void RemoveFilterProps(EspFilterProperties prop)
	{
		m_FilterProps.RemoveItem(prop);
		SaveFilters();
		LoadSavedFilters();
	}
	
	void LoadSavedFilters()
	{
		m_FilterEntry   = new array<ref VPPFilterEntry>;
		m_FilterProps   = new array<ref EspFilterProperties>;
		
		if (FileExist("$profile:ESPFilters.json"))
		{
			JsonFileLoader<ref array<ref EspFilterProperties>>.JsonLoadFile( "$profile:ESPFilters.json", m_FilterProps );
			InitFilters();
			return;
		}
		MakeDefaultFilters();
	}
	
	void MakeDefaultFilters()
	{
		m_FilterProps.Insert(new EspFilterProperties("Inventory_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("ItemBase",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("BaseBuildingBase",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Rifle_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Pistol_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Ammunition_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Edible_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("DayZCreatureAI",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("SurvivorBase",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Clothing_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert(new EspFilterProperties("Transport",ARGB(255,255,255,255)));
				
		SaveFilters();
		InitFilters();
	}
	
	void SaveFilters()
	{
		JsonFileLoader<ref array<ref EspFilterProperties>>.JsonSaveFile( "$profile:ESPFilters.json", m_FilterProps );
		
		//Rescan ESP if active
		if (M_SCAN_ACTIVE)
			ClearTrackers();
	}
	
	void InitFilters()
	{
		m_FilterEntry   = new array<ref VPPFilterEntry>;
		
		foreach(EspFilterProperties prop: m_FilterProps)
			m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, prop ,true));
	}

	bool ShowDeadPlayers()
	{
		return m_ChkShowDeadPlayers.IsChecked();
	}
};