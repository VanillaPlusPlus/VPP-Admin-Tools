class EspToolsMenu extends AdminHudSubMenu
{
	private int M_UPDATE_INTERVAL = 1;
	private float            m_UpdateTick;
	private bool             m_Init;
	private bool             M_SCAN_ACTIVE;
	private GridSpacerWidget m_SpacerParent; 	//Filters Spacer
	private GridSpacerWidget m_SpacerParentItems; //ESP items selected list
	private ref CustomGridSpacer m_LastGrid;
	private SliderWidget     m_SliderRadius;
	private ButtonWidget     m_btnToggle;
	private ButtonWidget     m_btnClear;
	private ButtonWidget     m_removeAllItems;
	private ButtonWidget     m_delAllItems;
	private ButtonWidget     m_btnAddNewFilter;
	private ButtonWidget     m_btnRestore;
	private CheckBoxWidget   m_chkSelectAll; //Filter checkbox
	private CheckBoxWidget   m_ChkUseNameFilter;
	private CheckBoxWidget   m_ChkShowDetailed;
	private EditBoxWidget    m_InputClassNameFilter;
	private EditBoxWidget    m_InputUpdateInterval;
	private ImageWidget      m_ImgInfo;
	private ScrollWidget     m_ScrollerItems;
	private ScrollWidget     m_Scroller;
	
	private ref array<ref VPPFilterEntry>   m_FilterEntry;
	private ref array<ref VPPESPItemEntry>  m_EspItemsEntry;
	private ref array<ref VPPESPTracker>	m_EspTrackers;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private ref array<ref EspFilterProperties> m_FilterProps;
	
	void EspToolsMenu()
	{
		m_FilterEntry   = new array<ref VPPFilterEntry>;
		m_EspItemsEntry = new array<ref VPPESPItemEntry>;
		m_DataGrids     = new array<ref CustomGridSpacer>;
		m_EspTrackers   = new array<ref VPPESPTracker>;
		m_FilterProps   = new array<ref EspFilterProperties>;
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		GetRPCManager().AddRPC( "RPC_VPPESPPlayerTracker", "HandleData", this, SingeplayerExecutionType.Server );
		
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
		m_ChkUseNameFilter = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkUseNameFilter"));
		m_ChkShowDetailed = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkShowDetailed"));
		m_InputClassNameFilter = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InputClassNameFilter"));
		m_InputUpdateInterval = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InputUpdateInterval"));
		m_ImgInfo = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfo"));
		
		m_ScrollerItems = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerItems"));
		m_Scroller = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("Scroller"));
		
		autoptr ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_ESP_TOOLTIP");
		
		LoadSavedFilters();
		InitEspItemsList();
		
		m_Init = true;
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_ScrollerItems.Show(!state);
		m_Scroller.Show(!state);
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_Init /*&& !IsSubMenuVisible()*/) return;
		
		//Update ESP
		m_UpdateTick += timeslice;
		
		M_UPDATE_INTERVAL = m_InputUpdateInterval.GetText().ToInt();
		if (M_UPDATE_INTERVAL <= 0)
			M_UPDATE_INTERVAL = 1;
		
		if (m_UpdateTick >= M_UPDATE_INTERVAL && M_SCAN_ACTIVE)
		{
			m_UpdateTick = 0.0;
			m_InputClassNameFilter.GetText().ToLower();
			
			autoptr VPPFilterEntry survivorFilter = GetFilter("SurvivorBase");
			
			if(survivorFilter && survivorFilter.IsSelected() || (m_ChkUseNameFilter.IsChecked() && m_InputClassNameFilter.GetText() == "survivorbase"))
			{
				array<Man> players = new array<Man>;
				players = ClientData.m_PlayerBaseList;
				array<string> requestedStats = new array<string>;
				
				foreach(Man man : players)
				{
					if (!CheckDuplicateTracker(man))
					{
						if(man.GetIdentity())
						{
							if(vector.Distance(GetGame().GetPlayer().GetPosition(), man.GetPosition()) <= m_SliderRadius.GetCurrent())
							{
								m_EspTrackers.Insert(new VPPESPTracker( man.GetIdentity().GetName(), man, true, survivorFilter.m_Props.color));
								
								if(m_ChkShowDetailed.IsChecked())
								{
									requestedStats.Insert(man.GetIdentity().GetId());
								}
							}
						}
					}
				}
				
				if(m_ChkShowDetailed.IsChecked())
					GetRPCManager().SendRPC( "RPC_VPPESPTools", "PlayerESP", new Param1<array<string>>(requestedStats),true,null);
			}
			
			autoptr array<Object> objects = new array<Object>;
			autoptr Man pilot = GetGame().GetPlayer();
			if (pilot)
			{
				GetGame().GetObjectsAtPosition(pilot.GetPosition(), m_SliderRadius.GetCurrent(), objects, null);
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
						
						autoptr VPPFilterEntry baseBuildingFilter = GetFilter("BaseBuildingBase");
						if (!CheckDuplicateTracker(obj))
						{
							if (baseBuildingFilter && baseBuildingFilter.IsSelected())
							{
								if(obj.IsInherited(Fence) || obj.IsInherited(FenceKit) || obj.IsInherited(Watchtower) || obj.IsInherited(WatchtowerKit) || obj.IsInherited(BaseBuildingBase))
								{
									m_EspTrackers.Insert(new VPPESPTracker( obj.GetDisplayName(), obj, m_ChkShowDetailed.IsChecked(),baseBuildingFilter.m_Props.color) );
								}
							}
						
							if (m_ChkUseNameFilter.IsChecked() && m_InputClassNameFilter.GetText() != "")
							{
								if (obj.IsInherited(m_InputClassNameFilter.GetText().ToType()))
									m_EspTrackers.Insert(new VPPESPTracker( obj.GetDisplayName(), obj, m_ChkShowDetailed.IsChecked()) );
							}else{
								autoptr EspFilterProperties filterAttributes = CheckFilter(obj);
								if (filterAttributes != null){
									m_EspTrackers.Insert(new VPPESPTracker( obj.GetDisplayName(), obj, m_ChkShowDetailed.IsChecked(), filterAttributes.color) );
								}			
							}
						}
					}
				}
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
			case m_SliderRadius:
			//Clear Trackers
			ClearTrackers();
			return true;
			break;
			
			//Clear items from items List
			case m_removeAllItems:
			ClearItemEntries();
			break;
			
			case m_chkSelectAll:
			foreach(VPPFilterEntry filter : m_FilterEntry)
				filter.SetSelected(m_chkSelectAll.IsChecked());
			break;
						
			case m_ChkShowDetailed:
			ClearTrackers();
			return true;
			break;
		}
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_InputClassNameFilter || w == m_InputUpdateInterval)
		{
			SetFocus(null);
			return true;
		}
		return false;
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		super.OnChange(w, x, y, finished);
		if (w == m_InputClassNameFilter && m_ChkUseNameFilter.IsChecked()){
			ClearTrackers();
			return true;
		}
		return false;
	}
	
	void RestoreFilters(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			MakeDefaultFilters();
			GetVPPUIManager().DisplayNotification("Added Defalut filters!");
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
	
	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<array<string>,array<ref PlayerStatsData>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Client)
		{	
			autoptr array<string> networkIds = data.param1;
			autoptr array<ref PlayerStatsData> stats = data.param2;
			
			if(networkIds == null || stats == null) return;
			
			for(int i = 0; i < networkIds.Count(); i++)
			{
				autoptr PlayerStatsData playerStats = stats[i];
				foreach(VPPESPTracker tracker : m_EspTrackers)
				{
					if(networkIds[i] == tracker.GetTrackingObject().GetNetworkIDString())
					{
						tracker.InitPlayerEspWidget(playerStats);
					}
				}
			}
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
	
	//return filter props if object matches filter
	EspFilterProperties CheckFilter(Object obj)
	{
		foreach(VPPFilterEntry filter : m_FilterEntry)
		{
			if(filter.GetFilterName() == "BaseBuildingBase" || filter.GetFilterName() == "SurvivorBase") continue;
			
			if (filter.IsSelected())
			{
				if (obj.IsKindOf(filter.GetFilterName()))
					return filter.m_Props;
			}
		}
		return null;
	}
	
	//DELETES & removes from list one or all selected items
	void DeleteESPItems(VPPESPItemEntry entry, bool all = false)
	{
		autoptr array<Object> ToDelete;
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
			foreach(VPPESPTracker tracker : m_EspTrackers)
			{
				if (tracker != null && tracker.GetTrackingObject() == entry.GetTargetObject())
					tracker.SetChecked(false);
			}
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
		foreach(VPPESPTracker tracker : m_EspTrackers)
		{
			if (tracker != null)
			{
				if (tracker.GetTrackingObject() != null && obj != null)
				{
					if (tracker.GetTrackingObject() == obj)
						return true;
				}
			}
		}
		return false;
	}
	
	void ClearTrackers()
	{
		foreach(VPPESPTracker tracker : m_EspTrackers)
		{
			if (tracker != null)
				delete tracker;
		}
		m_EspTrackers  = new array<ref VPPESPTracker>;
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
			ref VPPESPItemEntry entry = new VPPESPItemEntry(m_LastGrid.GetGrid(),target,M_SUB_WIDGET,itemDisplayName);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_EspItemsEntry.Insert(entry);
		}
		
		m_SpacerParentItems.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void ClearItemEntries()
	{
		//Deselect ALL trackers
		foreach(VPPESPTracker tracker : m_EspTrackers)
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
		m_FilterProps.Insert( new EspFilterProperties("Inventory_Base",ARGB(255,255,255,255)) );
		m_FilterProps.Insert( new EspFilterProperties("BaseBuildingBase",ARGB(255,255,255,255)) );
		m_FilterProps.Insert( new EspFilterProperties("Rifle_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("Pistol_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("Ammunition_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("Edible_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("DZ_LightAI",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("SurvivorBase",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("Clothing_Base",ARGB(255,255,255,255)));
		m_FilterProps.Insert( new EspFilterProperties("Transport",ARGB(255,255,255,255)));
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
		
		foreach(ref EspFilterProperties prop: m_FilterProps)
			m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, prop ,true));
	}
};