class EspToolsMenu extends AdminHudSubMenu
{
	private int M_UPDATE_INTERVAL = 5;
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
	private CheckBoxWidget   m_chkSelectAll; //Filter checkbox
	private CheckBoxWidget   m_ChkUseNameFilter;
	private CheckBoxWidget   m_ChkShowDetailed;
	private EditBoxWidget    m_InputClassNameFilter;
	private EditBoxWidget    m_InputUpdateInterval;
	private ImageWidget      m_ImgInfo;
	
	private ref array<ref VPPFilterEntry>   m_FilterEntry;
	private ref array<ref VPPESPItemEntry>  m_EspItemsEntry;
	private ref array<ref VPPESPTracker>	m_EspTrackers;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	
	void EspToolsMenu()
	{
		m_FilterEntry   = new array<ref VPPFilterEntry>;
		m_EspItemsEntry = new array<ref VPPESPItemEntry>;
		m_DataGrids     = new array<ref CustomGridSpacer>;
		m_EspTrackers   = new array<ref VPPESPTracker>;
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
		m_removeAllItems = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "removeAllItems"));
		m_delAllItems = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "delAllItems"));
		GetVPPUIManager().HookConfirmationDialog(m_delAllItems, M_SUB_WIDGET,this,"ConfirmDeleteAll", DIAGTYPE.DIAG_YESNO, "Delete Items", "Are you sure you wish to delete ALL ITEMS in the list? (You can't revert once you delete this item)");
		m_chkSelectAll = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("chkSelectAll"));
		m_ChkUseNameFilter = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkUseNameFilter"));
		m_ChkShowDetailed = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkShowDetailed"));
		m_InputClassNameFilter = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InputClassNameFilter"));
		m_InputUpdateInterval = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("InputUpdateInterval"));
		m_ImgInfo = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfo"));
		
		autoptr ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("Information:");
		toolTip.SetContentText("When the toggle button is green it means ESP will scan every second with new data.");
		
		InitFilters();
		InitEspItemsList();
		ShowSubMenu();
		
		m_Init = true;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_Init && !IsSubMenuVisible()) return;
		
		//Update ESP
		m_UpdateTick += timeslice;
		
		M_UPDATE_INTERVAL = m_InputUpdateInterval.GetText().ToInt();
		if (M_UPDATE_INTERVAL <= 0)
			M_UPDATE_INTERVAL = 5;
		
		if (m_UpdateTick >= M_UPDATE_INTERVAL && M_SCAN_ACTIVE)
		{
			m_UpdateTick = 0.0;
			autoptr array<Object> objects = new array<Object>;
			autoptr Man pilot = GetGame().GetPlayer();
			if (pilot != null)
			{
				GetGame().GetObjectsAtPosition(pilot.GetPosition(), m_SliderRadius.GetCurrent(), objects, null);
				if (objects)
				{
					foreach(Object obj : objects)
					{
						if (obj.IsBuilding())
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
						
						if (!CheckDuplicateTracker(obj))
						{
							//Player Check
							if (obj.IsKindOf("SurvivorBase") && m_ChkShowDetailed.IsChecked()){
								GetRPCManager().SendRPC( "RPC_VPPESPTools", "PlayerESP", new Param1<Object>(obj),true,null);
								m_EspTrackers.Insert(new VPPESPTracker( m_RootWidget.FindAnyWidget("Panel_Content"), "", obj, true) );
								return;
							}
							
							if (m_ChkUseNameFilter.IsChecked() && m_InputClassNameFilter.GetText() != "")
							{
								if (obj.IsKindOf(m_InputClassNameFilter.GetText()))
									m_EspTrackers.Insert(new VPPESPTracker( m_RootWidget.FindAnyWidget("Panel_Content"),obj.GetDisplayName(), obj, m_ChkShowDetailed.IsChecked()) );
							}else{
								if (CheckFilter(obj))
									m_EspTrackers.Insert(new VPPESPTracker( m_RootWidget.FindAnyWidget("Panel_Content"),obj.GetDisplayName(), obj, m_ChkShowDetailed.IsChecked()) );			
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
	
	void ConfirmDeleteAll(int result)
	{
		if (result == DIAGRESULT.YES)
			DeleteESPItems(null,true);
	}
	
	bool CheckFilter(Object obj)
	{
		foreach(VPPFilterEntry filter : m_FilterEntry)
		{
			if (filter.IsSelected())
			{
				if (obj.IsKindOf(filter.GetFilterName()))
					return true;
			}
		}
		return false;
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
	
	void InitFilters()
	{
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Rifle_Base"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Pistol_Base"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Ammunition_Base"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Inventory_Base"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Edible_Base"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "DZ_LightAI"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "SurvivorBase"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Transport"));
		m_FilterEntry.Insert(new VPPFilterEntry(m_SpacerParent, "Clothing_Base"));
	}
};