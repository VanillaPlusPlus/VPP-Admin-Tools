class MenuTeleportManager extends AdminHudSubMenu
{
	//category filter kinds
	private const int CAT_ALL    = 0;
	private const int CAT_TOWNS  = 1;
	private const int CAT_CUSTOM = 2;
	private const int CAT_RECENT = 3;

	private float 						    updateInterval;
	private float                           searchInterval;
	private bool 	  	 	 				m_loaded;
	private ScrollWidget	 			    m_Scroller;
	private GridSpacerWidget 			    m_ParentGrid;
	private ref CustomGridSpacer 			m_LastGrid;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private ref array<ref TeleportEntry> 	m_Entries;
	private ref array<ref VPPTeleportLocation> m_AllLocations;

	private ref PopUpNewPositionEditor  m_PopUpPositionEditor;
	private Widget           m_PopUpPositionEditorWidget;
	private Widget           m_Main;
	MapWidget 	 	 		 m_Map;
	private ButtonWidget     m_btnAddPos;
	private ButtonWidget     m_BtnRemove;
	private ButtonWidget     m_btnTeleport;
	private ButtonWidget     m_btnRefresh;
	private ButtonWidget     m_btnCopyCoords;
	private ButtonWidget     m_BtnTargetSelf;
	private ButtonWidget     m_BtnTargetSelected;
	private TextWidget       m_TargetSelfLbl;
	private TextWidget       m_TargetSelectedLbl;
	private ImageWidget      m_ImgTargetSelf;
	private ImageWidget      m_ImgTargetSelected;
	private CheckBoxWidget   m_chkSelectAll;
	private CheckBoxWidget   m_ChkShowMarkers;
	private TextWidget	 	 m_txtCursorPos;
	private TextWidget       m_txtLocationCount;
	private EditBoxWidget    m_SearchBox;

	private ref VPPDropDownMenu m_CategoryDropDown;
	private ref array<string>   m_CategoryLabels;
	private ref array<string>   m_TownNames;
	private int                 m_CurrentCategory;

	private ref TeleportManagerClientPrefs m_Prefs;

	private bool                m_TargetSelected;   //false = self, true = selected players
	private string              m_LastSearch;
	private vector              m_LastCursorPos;

	void MenuTeleportManager()
	{
		/*RPCs*/
		GetRPCManager().AddRPC( "RPC_MenuTeleportManager", "HandleData", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC( "RPC_MenuTeleportManager", "UpdateMap", this, SingleplayerExecutionType.Client);
		/*-----*/

		m_Entries      = new array<ref TeleportEntry>;
		m_DataGrids    = new array<ref CustomGridSpacer>;
		m_AllLocations = new array<ref VPPTeleportLocation>;

		m_CategoryLabels = {"All", "Towns", "Custom", "Recent"};
		BuildTownSet();
	}

	//mirrors CreateDefaults() in the server-side TeleportManager.c; client-only classification
	private void BuildTownSet()
	{
		m_TownNames = {
			"Severograd","Krasnostav","Mogilevka","Stary","Msta","Vybor","Gorka",
			"Solnichni","NWAF","Blota","NEAF","Cherno","Elektro","Berez","Svetlojarsk",
			"Zelenogorsk","Lopatino","Tisy","Novaya","Novy","Grishino","Kabanino"
		};
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuTeleportManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		m_Main		  = M_SUB_WIDGET.FindAnyWidget( "Main");

		m_Scroller          = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "Scroller") );
		m_ParentGrid        = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SpacerParent") );
		m_Map 		        = MapWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "MapWidget"));

		m_btnRefresh        = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnRefresh"));
		m_btnAddPos         = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnAddPos"));
		m_btnCopyCoords     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnCopyCoords"));

		m_BtnRemove         = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnRemove"));
		GetVPPUIManager().HookConfirmationDialog(m_BtnRemove, M_SUB_WIDGET,this,"RemovePosition", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_DEL_PRESET_TP", "#VSTR_TOOLTIP_DEL_PRESET_TP");

		m_btnTeleport       = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnTeleport"));
		m_BtnTargetSelf     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnTargetSelf"));
		m_BtnTargetSelected = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnTargetSelected"));
		m_TargetSelfLbl     = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "TargetSelfLbl"));
		m_TargetSelectedLbl = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "TargetSelectedLbl"));
		m_ImgTargetSelf     = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ImgTargetSelf"));
		m_ImgTargetSelected = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ImgTargetSelected"));

		m_chkSelectAll      = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkSelectAll"));
		m_ChkShowMarkers    = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ChkShowMarkers"));
		m_txtCursorPos      = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "txtCursorPos"));
		m_txtLocationCount  = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "txtLocationCount"));
		m_SearchBox         = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SearchInputBox"));

		//category filter dropdown (design-system VPPDropDownMenu)
		m_CategoryDropDown = new VPPDropDownMenu( M_SUB_WIDGET.FindAnyWidget("CategoryDropDownPanel"), m_CategoryLabels[0] );
		foreach (string catLabel : m_CategoryLabels)
			m_CategoryDropDown.AddElement(catLabel);
		m_CategoryDropDown.m_OnSelectItem.Insert( OnSelectCategory );
		m_CurrentCategory = CAT_ALL;

		//local recents store
		m_Prefs = TeleportManagerClientPrefs.Load();

		//default target = self
		SetTarget(false);

		GetTeleportPositions();
		GetRPCManager().VSendRPC( "RPC_TeleportManager", "GetPlayerPositions", null, true, null);
		m_loaded = true;
	}

	override void HideBrokenWidgets(bool state)
	{
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

		//debounced search (recheck a few times a second)
		searchInterval += timeslice;
		if (searchInterval >= 0.25)
		{
			searchInterval = 0.0;
			string cur = "";
			if (m_SearchBox)
				cur = m_SearchBox.GetText();
			if (cur != m_LastSearch)
			{
				m_LastSearch = cur;
				ApplySearchFilter();
			}
		}

		int selectedCount = GetSelected().Count();
		m_BtnRemove.Enable(selectedCount >= 1);
		m_btnTeleport.Enable(selectedCount >= 1);

		Widget w = GetWidgetUnderCursor();
		if (w && w == m_Map)
		{
			vector pos = ScreenToWorld();
			pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
			m_LastCursorPos = pos;

			string posTxt = string.Format("X: %1   Y: %2   Z: %3", pos[0], pos[1], pos[2]);
			m_txtCursorPos.SetText(posTxt);
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_btnRefresh:
			GetTeleportPositions();
			break;

			case m_chkSelectAll:
			foreach(TeleportEntry entry : m_Entries)
			{
				if (entry != null && entry.IsVisible() && !entry.IsRecentMode())
					entry.GetCheckBox().SetChecked(m_chkSelectAll.IsChecked());
			}
			break;

			case m_btnTeleport:
				PreformTeleport();
			break;

			case m_btnAddPos:
			CreatePositionPopUp(Vector(0, 0, 0));
			break;

			case m_btnCopyCoords:
			{
				vector myPos = vector.Zero;
				if (GetGame().GetPlayer())
					myPos = GetGame().GetPlayer().GetPosition();
				GetGame().CopyToClipboard(myPos.ToString());
				GetVPPUIManager().DisplayNotification("Copied your position to clipboard!", "Position Copy", 3.0);
			}
			break;

			case m_BtnTargetSelf:
			SetTarget(false);
			break;

			case m_BtnTargetSelected:
			SetTarget(true);
			break;
		}

		return super.OnClick(w, x, y, button);
	}

	/*
	\ Target toggle (Self | Selected players)
	*/
	void SetTarget(bool selected)
	{
		m_TargetSelected = selected;

		int active   = ARGB(255, 232, 163, 61);   //accent-orange
		int inactive = ARGB(255, 154, 160, 166);   //text-secondary

		int selfColor     = active;
		int selectedColor = inactive;
		if (selected)
		{
			selfColor     = inactive;
			selectedColor = active;
		}

		if (m_TargetSelfLbl)     m_TargetSelfLbl.SetColor(selfColor);
		if (m_TargetSelectedLbl) m_TargetSelectedLbl.SetColor(selectedColor);
		if (m_ImgTargetSelf)     m_ImgTargetSelf.SetColor(selfColor);
		if (m_ImgTargetSelected) m_ImgTargetSelected.SetColor(selectedColor);
	}

	/*
	\ Category dropdown callback
	*/
	void OnSelectCategory(int index)
	{
		if (index < 0 || index >= m_CategoryLabels.Count()) return;

		m_CategoryDropDown.SetText(m_CategoryLabels[index]);
		m_CategoryDropDown.SetIndex(index);
		m_CategoryDropDown.Close();
		m_CurrentCategory = index;
		if (m_chkSelectAll) m_chkSelectAll.SetChecked(false);
		RebuildList();
	}

	private bool IsTown(string name)
	{
		string n = name;
		n.ToLower();
		foreach (string t : m_TownNames)
		{
			string tl = t;
			tl.ToLower();
			if (tl == n)
				return true;
		}
		return false;
	}

	void UpdateMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref VPPPlayerData>> data;
		if(!ctx.Read(data)) return;

		array<ref VPPPlayerData> temp = data.param1;
		if (type == CallType.Client)
		{
			if (!m_Map)
				return;

			m_Map.ClearUserMarks();
			UpdateMapEx();
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

	//override to add your markers, don't call method yourself. It's called from UpdateMap()
	//This method is called on every update RPC received from server (interval of 1 second)
	void UpdateMapEx()
	{
		//Add Teleport positions markers on map
		if (m_ChkShowMarkers.IsChecked())
		{
			foreach(TeleportEntry entry : m_Entries)
			{
				if (entry == null || !entry.IsVisible())
					continue;
				VPPTeleportLocation loc = entry.GetVPPTeleportLocation();
				m_Map.AddUserMark(loc.GetLocation(), loc.GetName(), ARGB(255,255,0,255), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\objective_CA.paa");
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
		foreach(VPPTeleportLocation loc : m_AllLocations)
		{
			if (loc != null && loc.GetName() == posName)
				return true;
		}
		return false;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (button == MouseState.LEFT)
		{
			if (w == m_Map)
			{
				if (g_Game.IsLeftCtrlDown()){
					//Snap Y to surface
					vector pos = ScreenToWorld();
					pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
					CreatePositionPopUp(pos);
				}else{
					PreformTeleport(true);
				}

				return true;
			}
		}

		if (button == MouseState.RIGHT && w == m_Map)
		{
			vector _pos = ScreenToWorld();
			_pos[1] = GetGame().SurfaceY(_pos[0], _pos[2]);
			GetGame().CopyToClipboard(_pos.ToString());
			GetVPPUIManager().DisplayNotification("Copied map cursor position!", "Position Copy", 3.0);
			return true;
		}

		return super.OnDoubleClick(w, x, y, button);
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

	/*
	\ Teleport core. serverTownName != "" => server resolves by name, otherwise it uses pos.
	\ recordName is what gets stored in the local recents history.
	*/
	void DoTeleport(string recordName, string serverTownName, vector pos)
	{
		ref array<string> ids = new array<string>;
		if (m_TargetSelected)
		{
			MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			if (!pManager)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_TP_PLAYERS_NOTSELECT");
				return;
			}
			array<string> selectedPlayers = pManager.GetSelectedPlayersIDs();
			if (selectedPlayers.Count() < 1)
			{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_TP_PLAYER");
				return;
			}
			foreach (string id : selectedPlayers)
				ids.Insert(id);
		}
		else
		{
			ids.Insert("self");
		}

		GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>(ids, serverTownName, pos), true);

		if (m_Prefs)
			m_Prefs.RecordRecent(recordName, pos);
	}

	void PreformTeleport(bool customPos = false)
	{
		if (customPos)
		{
			vector pos = ScreenToWorld();
			pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
			DoTeleport("Custom Point", "", pos);
			return;
		}

		array<ref TeleportEntry> selected = GetSelected();
		if (selected.Count() > 0)
		{
			TeleportEntry e = selected[0];
			VPPTeleportLocation loc = e.GetVPPTeleportLocation();
			DoTeleport(e.GetName(), e.GetName(), loc.GetLocation());
		}
	}

	/*
	\ Per-row callbacks (invoked by TeleportEntry)
	*/
	void OnEntryLocate(TeleportEntry e)
	{
		if (e == null || m_Map == null) return;
		float scale = Math.Clamp(m_Map.GetScale() / 3, 0.1, 2.5);
		m_Map.SetScale(scale);
		m_Map.SetMapPos(e.GetVPPTeleportLocation().GetLocation());
	}

	void OnEntryTeleport(TeleportEntry e)
	{
		if (e == null) return;
		VPPTeleportLocation loc = e.GetVPPTeleportLocation();
		if (e.IsRecentMode())
			DoTeleport(e.GetName(), "", loc.GetLocation());            //recents teleport to exact stored coords
		else
			DoTeleport(e.GetName(), e.GetName(), loc.GetLocation());   //saved locations resolve by name server-side
	}

	void OnEntryEdit(TeleportEntry e)
	{
		if (e == null) return;
		VPPTeleportLocation loc = e.GetVPPTeleportLocation();
		CreatePositionPopUp(loc.GetLocation(), true, loc.GetName());
	}

	void OnEntryDelete(TeleportEntry e, bool recentMode)
	{
		if (e == null) return;

		if (recentMode)
		{
			if (m_Prefs)
			{
				array<ref TeleportRecentEntry> recents = m_Prefs.GetRecents();
				for (int i = 0; i < recents.Count(); ++i)
				{
					if (recents[i] != null && recents[i].Name == e.GetName())
					{
						m_Prefs.RemoveRecentAt(i);
						break;
					}
				}
			}
			RebuildList();
		}
		else
		{
			ref array<string> toDelete = new array<string>;
			toDelete.Insert(e.GetVPPTeleportLocation().GetName());
			GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteRemovePreset", new Param1<ref array<string>>(toDelete), true);
		}
	}

	void OnEntryChecked()
	{
		//hook kept for future select-all tri-state sync; button enable handled in OnUpdate
	}

	array<ref TeleportEntry> GetSelected()
	{
		array<ref TeleportEntry> selected = new array<ref TeleportEntry>;
		foreach(TeleportEntry entry : m_Entries)
		{
			if (entry != null && !entry.IsRecentMode() && entry.GetCheckBox().IsChecked())
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
		m_Entries.Clear();
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

			m_AllLocations = data.param1;
			if (m_AllLocations == null)
				m_AllLocations = new array<ref VPPTeleportLocation>;

			RebuildList();
		}
	}

	//Builds the visible entry list from either saved locations (filtered by category) or recents.
	void RebuildList()
	{
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

		if (m_CurrentCategory == CAT_RECENT)
		{
			if (m_Prefs)
			{
				foreach(TeleportRecentEntry r : m_Prefs.GetRecents())
				{
					if (r == null) continue;
					VPPTeleportLocation loc = new VPPTeleportLocation(r.Name, r.GetLocation());
					AddEntry(loc, true);
				}
			}
		}
		else
		{
			foreach(VPPTeleportLocation pos : m_AllLocations)
			{
				if (pos == null) continue;
				if (m_CurrentCategory == CAT_TOWNS  && !IsTown(pos.GetName())) continue;
				if (m_CurrentCategory == CAT_CUSTOM &&  IsTown(pos.GetName())) continue;
				AddEntry(pos, false);
			}
		}

		ApplySearchFilter();
	}

	//toggles row visibility based on the search box; refreshes the count badge
	void ApplySearchFilter()
	{
		string q = "";
		if (m_SearchBox)
			q = m_SearchBox.GetText();
		q.ToLower();

		int shown = 0;
		foreach(TeleportEntry e : m_Entries)
		{
			if (e == null) continue;
			string n = e.GetName();
			n.ToLower();
			bool match = (q == "" || n.Contains(q));
			e.SetVisible(match);
			if (match) shown++;
		}

		if (m_txtLocationCount)
			m_txtLocationCount.SetText(shown.ToString());

		m_ParentGrid.Update();
		m_Scroller.Update();
	}

	void AddEntry(VPPTeleportLocation pos, bool recentMode = false)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}

		if(m_LastGrid.GetContentCount() < 100)
		{
			TeleportEntry entry = new TeleportEntry(m_LastGrid.GetGrid(), pos, this, recentMode);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_Entries.Insert(entry);
		}

		m_ParentGrid.Update();
		m_Scroller.Update();
		m_LastGrid.GetGrid().Update();
	}
};
