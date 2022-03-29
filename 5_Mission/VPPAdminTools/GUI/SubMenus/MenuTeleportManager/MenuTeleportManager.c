class MenuTeleportManager extends AdminHudSubMenu
{
	private float 						    updateInterval;
	private bool 	  	 	 				m_loaded;
	private ImageWidget						m_ImgInfo;
	private ScrollWidget	 			    m_Scroller;
	private GridSpacerWidget 			    m_ParentGrid;
	private ref CustomGridSpacer 			m_LastGrid;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private ref array<ref TeleportEntry> 	m_Entries;
	
	private ref PopUpNewPositionEditor  m_PopUpPositionEditor;
	private Widget           m_PopUpPositionEditorWidget;
	private Widget           m_Main;
	private MapWidget 	 	 m_Map;
	private ButtonWidget     m_btnAddPos;
	private ButtonWidget     m_btnEditPos;
	private ButtonWidget     m_BtnRemove;
	private ButtonWidget     m_btnTeleport;
	private ButtonWidget     m_btnRefresh;
	private CheckBoxWidget   m_ChkTpSelected;
	private CheckBoxWidget   m_chkSelectAll;
	
	void MenuTeleportManager()
	{
		/*RPCs*/
		GetRPCManager().AddRPC( "RPC_MenuTeleportManager", "HandleData", this);
		GetRPCManager().AddRPC( "RPC_MenuTeleportManager", "UpdateMap", this);
		/*-----*/
		
		m_Entries   = new array<ref TeleportEntry>;
		m_DataGrids = new array<ref CustomGridSpacer>;
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuTeleportManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		m_Main		  = M_SUB_WIDGET.FindAnyWidget( "Main");
		
		m_ImgInfo	  = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgInfo") );
		ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_TOOLTIP_TP_MENU_INFO");
		
		m_Scroller       = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "Scroller") );
		m_ParentGrid     = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SpacerParent") );
		m_Map 		     = MapWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "MapWidget"));
		WidgetEventHandler.GetInstance().RegisterOnDoubleClick( m_Map, this, "MapDoubleClick" );
		
		m_btnRefresh     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnRefresh"));
		m_btnAddPos      = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnAddPos"));
		m_btnEditPos     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnEditPos"));
		
		m_BtnRemove      = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnRemove"));
		GetVPPUIManager().HookConfirmationDialog(m_BtnRemove, M_SUB_WIDGET,this,"RemovePosition", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_DEL_PRESET_TP", "#VSTR_TOOLTIP_DEL_PRESET_TP");
		
		m_btnTeleport    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnTeleport"));
		m_ChkTpSelected  = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ChkTpSelected"));
		m_chkSelectAll   = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkSelectAll"));
		
		GetTeleportPositions();
		GetRPCManager().VSendRPC( "RPC_TeleportManager", "GetPlayerPositions", null, true, null);
		m_loaded = true;
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_Scroller.Show(!state);
		m_Map.Show(!state);
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() || !m_loaded)
			return;
		
		updateInterval += timeslice;
		if (updateInterval >= 1.0)
		{
			GetRPCManager().VSendRPC( "RPC_TeleportManager", "GetPlayerPositions", null, true, null);
			updateInterval = 0.0;
		}
		
		int selectedCount = GetSelected().Count();
		m_btnEditPos.Enable(selectedCount == 1);
		m_BtnRemove.Enable(selectedCount >= 1);
		m_btnTeleport.Enable(selectedCount == 1);
	}
		
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_btnRefresh:
			GetTeleportPositions();
			break;
			
			case m_chkSelectAll:
			foreach(TeleportEntry entry : m_Entries)
				entry.GetCheckBox().SetChecked(m_chkSelectAll.IsChecked());
			break;
			
			case m_btnTeleport:
				PreformTeleport();
			break;
			
			case m_btnAddPos:
			CreatePositionPopUp(Vector(0, 0, 0));
			break;
			
			case m_btnEditPos:
			TeleportEntry selected = GetSelected()[0];
			if (selected != null)
				CreatePositionPopUp(selected.GetVPPTeleportLocation().GetLocation(),true,selected.GetVPPTeleportLocation().GetName());
			break;
		}
		return false;
	}
	
	void UpdateMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref VPPPlayerData>> data;
		if(!ctx.Read(data)) return;
		
		array<ref VPPPlayerData> temp = data.param1;
		if( type == CallType.Client )
		{
			if(m_Map != null)
			{
				m_Map.ClearUserMarks();
				DayZPlayer client = GetGame().GetPlayer();
				MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
				array<ref VPPPlayerEntry> selectedPlayers = {};
				if (pManager)
					selectedPlayers = pManager.GetSelectedPlayers();

				foreach(VPPPlayerData info : temp)
				{
					if(client && client.GetPosition() == info.m_PlayerPos)
					{
						m_Map.AddUserMark(info.m_PlayerPos, "Me", ARGB(255,255,244,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa");
						continue;
					}

					if (!selectedPlayers || selectedPlayers.Count() <= 0)
					{
						m_Map.AddUserMark(info.m_PlayerPos, info.m_PlayerName, ARGB(255,0,255,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa");
						continue;
					}
					//check if player is selected
					for (int i = 0; i < selectedPlayers.Count(); ++i)
					{
						if (info.m_PlayerName == selectedPlayers[i].GetPlayerName() && selectedPlayers[i].IsSelected())
						{
							m_Map.AddUserMark(info.m_PlayerPos, info.m_PlayerName, ARGB(255,255,0,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa");
							break;
						}
					}
				}	
			}
		}
	 }
	
	
	//Called by PopUpNewPositionEditor, by this stage duplication check and proper data check is done.
	void SaveNewMarker(string name, vector position, bool editMode, string oldName = "", vector oldPosition = "0 0 0")
	{
		delete m_PopUpPositionEditorWidget;
		if (editMode)
			GetRPCManager().VSendRPC("RPC_TeleportManager", "EditPreset", new Param4<string,vector,string,vector>(oldName,oldPosition,name,position), true);
		else
			GetRPCManager().VSendRPC("RPC_TeleportManager", "AddNewPreset", new Param2<string,vector>(name,position), true);
	}
	
	void RemovePosition(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			array<ref TeleportEntry> selected = GetSelected();
			ref array<string> toDelete = new array<string>;
			foreach(TeleportEntry entry : selected)
			{
				if (entry != null)
					toDelete.Insert(entry.GetVPPTeleportLocation().GetName());
			}
			GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteRemovePreset", new Param1<ref array<string>>(toDelete), true);
		}
	}
	
	/*
		use to check if a position with the same name already exists
		returns true if duplicate item
	*/
	bool CheckDuplicate(string posName)
	{
		foreach(TeleportEntry entry : m_Entries)
		{
			if (entry != null && entry.GetVPPTeleportLocation().GetName() == posName)
				return true;
		}
		return false;
	}
	
	void MapDoubleClick(Widget w, int x, int y, int button)
	{
		if (button == MouseState.LEFT)
		{
			if (g_Game.IsLeftCtrlDown())
				CreatePositionPopUp(ScreenToWorld());
			else
				PreformTeleport(true);
		}
	}

	vector ScreenToWorld()
	{
		vector world_pos,ScreenToMap,dir,from,to;
		dir = GetGame().GetPointerDirection();
	    from = GetGame().GetCurrentCameraPosition();
	    to = from + ( dir * 1000 );
		world_pos = GetGame().GetScreenPos( to );
	    ScreenToMap = m_Map.ScreenToMap( world_pos );
	    return ScreenToMap;
	}
	
	void CreatePositionPopUp(vector position, bool editMode = false, string oldName = "")
	{
		if (m_PopUpPositionEditorWidget != null && m_PopUpPositionEditor != null)
		{
			m_PopUpPositionEditorWidget.Unlink();
			delete m_PopUpPositionEditor;
		}
		m_PopUpPositionEditorWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.PopUpCreatePosition, m_Main);
		m_PopUpPositionEditorWidget.GetScript(m_PopUpPositionEditor);
		m_PopUpPositionEditor.Init(this,position,editMode,oldName);
	}
	
	void PreformTeleport(bool customPos = false)
	{
		if (m_ChkTpSelected.IsChecked())
		{			
			MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			if (pManager)
			{
				array<string> selectedPlayers = pManager.GetSelectedPlayersIDs();
				if (selectedPlayers.Count() >= 1)
				{
					if (customPos)
						GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>(selectedPlayers,"",ScreenToWorld()), true);
						else
						GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>(selectedPlayers,GetSelected()[0].GetVPPTeleportLocation().GetName(),ScreenToWorld()), true);
				}
				else
				{
					GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_TP_PLAYER");
				}
			}
			else 
			{ 
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_TP_PLAYERS_NOTSELECT"); 
			}
		}
		else
		{
			if (GetSelected().Count() > 0 && !customPos)
				GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>({"self"},GetSelected()[0].GetVPPTeleportLocation().GetName(),ScreenToWorld()), true);
			else
				GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>({"self"},"",ScreenToWorld()), true);
		}
	}
	
	array<ref TeleportEntry> GetSelected()
	{
		array<ref TeleportEntry> selected = new array<ref TeleportEntry>;
		foreach(TeleportEntry entry : m_Entries)
		{
			if (entry != null && entry.GetCheckBox().IsChecked())
			{
				selected.Insert(entry);
			}
		}
		return selected;
	}
	
	void ToggleMapWidget(bool state)
	{
		if (m_Map == null) return;
		m_Map.Show(state);
	}
	
	void ClearEntriesList()
	{
		foreach(TeleportEntry entry : m_Entries)
		{
			if (entry != null)
			m_Entries.RemoveItem(entry);
				delete entry;
		}
		m_Entries = new array<ref TeleportEntry>;
	}
	
	void GetTeleportPositions()
	{
		GetRPCManager().VSendRPC("RPC_TeleportManager", "GetData", null, true);
	}
	
	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<ref array<ref VPPTeleportLocation>> data;
			if(!ctx.Read(data)) return;
			
			array<ref VPPTeleportLocation> teleportPositions = data.param1;
			
			ClearEntriesList();
			
			foreach(CustomGridSpacer cusGrid : m_DataGrids){
				if (cusGrid != null)
					delete cusGrid;
			}
			m_DataGrids = new array<ref CustomGridSpacer>;
			
			//init first "page"
			m_DataGrids.Insert(new CustomGridSpacer(m_ParentGrid));
			m_LastGrid = m_DataGrids[0];
			m_Scroller.Update();
			
			foreach(VPPTeleportLocation pos : teleportPositions){
				AddEntry(pos);
			}
		}
	}

	void AddEntry(VPPTeleportLocation pos)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100)
		{
			TeleportEntry entry = new TeleportEntry(m_LastGrid.GetGrid(), pos);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_Entries.Insert(entry);
		}
		
		m_ParentGrid.Update();
		m_Scroller.Update();
		m_LastGrid.GetGrid().Update();
	}
};