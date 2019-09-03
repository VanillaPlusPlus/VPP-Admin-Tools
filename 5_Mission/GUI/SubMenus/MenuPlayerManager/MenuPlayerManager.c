class MenuPlayerManager extends AdminHudSubMenu
{
	private bool 						  m_Init;
	private GridSpacerWidget 			  m_GridPlayerInfo;
	private GridSpacerWidget 			  m_Grid_1;
	private int                           m_Grid_1_Count = 0;
	private GridSpacerWidget 			  m_Grid_2;
	private int                           m_Grid_2_Count = 0;
	private GridSpacerWidget              m_CurrentGrid;
	
	private EditBoxWidget				  m_SearchInputBox;
	private string 			 			  m_searchBoxStr;
	private ScrollWidget				  m_PlayerList;
	private ButtonWidget	 			  m_BtnRefreshPlayerList;
	private CheckBoxWidget 		  		  m_SelectAllPlayers;
	private ButtonWidget 				  m_btnFnAddPlayersToGrp;
	private ref array<ref VPPPlayerEntry> m_PlayerEntries;
	private ref array<ref VPPPlayerStats> m_PlayerStats;
	private bool 						  m_SelectionMode;
	private autoptr array<string> 		  IDs;
	private const float      			  m_FilterUpdate = 0.35;
	private float      		 			  m_FilterUpdateCurTick;
	
	//Stats Sliders
	private SliderWidget m_SliderHealth;
	private SliderWidget m_SliderBlood;
	private SliderWidget m_SliderShock;
	private SliderWidget m_SliderWater;
	private SliderWidget m_SliderEnergy;
	
	private ButtonWidget m_BtnApplyHealth;
	private ButtonWidget m_BtnApplyBlood;
	private ButtonWidget m_BtnApplyShock;
	private ButtonWidget m_BtnApplyWater;
	private ButtonWidget m_BtnApplyEnergy;
	//-------------
		
	//Action Buttons--
	private ButtonWidget m_ActionHeal;
	private ButtonWidget m_ActionMakeInvis;
	private ButtonWidget m_ActionKickPlayer;
	private ButtonWidget m_ActionKill;
	private ButtonWidget m_ActionSendMessage;
	private ButtonWidget m_ActionBanPlayer;
	private ButtonWidget m_ActionTpToMe;
	private ButtonWidget m_ActionTpMeTo;
	private ButtonWidget m_ActionSpectate;
	//----------------
	
	void MenuPlayerManager()
	{
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "InitSpectate", this, SingleplayerExecutionType.Server);
		
		m_PlayerEntries = new array<ref VPPPlayerEntry>;
		m_PlayerStats   = new array<ref VPPPlayerStats>;
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/PlayerManagerUI/MenuPlayerManager.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_GridPlayerInfo   	   = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "GridPlayerInfo") );
		m_Grid_1 	   	   	   = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "Grid_1") );
		m_Grid_2			   = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "Grid_2") );
		m_PlayerList 	   	   = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PlayerList") );
		m_SearchInputBox 	   = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		
		m_SelectAllPlayers 	   = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkSelectAllPlayers") );
		m_BtnRefreshPlayerList = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnRefreshPlayerList"));
		m_btnFnAddPlayersToGrp = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnFnAddPlayersToGrp"));
		GetVPPUIManager().HookConfirmationDialog(m_btnFnAddPlayersToGrp, M_SUB_WIDGET,this,"FinishPlayerSelect", DIAGTYPE.DIAG_YESNO, "Notice", "Are you sure you wish to add selected players to selected user group?");
		
		//Stats Sliders
		m_SliderHealth  = SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderHealth"));
		m_SliderBlood	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderBlood"));
		m_SliderShock	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderShock"));
		m_SliderWater	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderWater"));
		m_SliderEnergy	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SliderEnergy"));
		
		m_BtnApplyHealth  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnApplyHealth"));
		m_BtnApplyBlood   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnApplyBlood"));
		m_BtnApplyShock   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnApplyShock"));
		m_BtnApplyWater   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnApplyWater"));
		m_BtnApplyEnergy  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnApplyEnergy"));
		//-------------
		
		//Action Buttons
		m_ActionMakeInvis 	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionMakeInvis"));
		m_ActionHeal  		   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionHeal"));
		m_ActionKickPlayer     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionKickPlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionKickPlayer, M_SUB_WIDGET,this,"KickPlayer", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Kick Player", "Please type kick reason or leave empty.!", true);
		
		m_ActionKill	   	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionKill"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionKill, M_SUB_WIDGET,this,"KillSelectedPlayers", DIAGTYPE.DIAG_YESNO, "Kill Player(s)", "Are you sure you wish to kill selected players?");
		
		m_ActionSendMessage    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionSendMessage"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionSendMessage, M_SUB_WIDGET,this,"SendMessageToPlayer", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Send Message", "Please type the message you wish to send!", true);
		
		m_ActionBanPlayer  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionBanPlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionBanPlayer, M_SUB_WIDGET,this,"BanPlayer", DIAGTYPE.DIAG_YESNO, "Ban Player(s)", "Are you sure you wish to ban selected players? Note: you can edit the ban duration after within the Bans Manager Menu");
		
		
		m_ActionTpToMe  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionTpToMe"));
		m_ActionTpMeTo  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionTpMeTo"));
		m_ActionSpectate  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionSpectate"));
		//--------------
		
		UpdateEntries();
		ShowSubMenu();
		m_Init = true;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && M_SUB_WIDGET == null) return;
		
		m_FilterUpdateCurTick += timeslice;
		if (m_FilterUpdateCurTick >= m_FilterUpdate)
		{
			string newSrch = m_SearchInputBox.GetText();
			if (newSrch != m_searchBoxStr)
			{
				UpdateFilter();
				m_searchBoxStr = newSrch;
			}	
			m_FilterUpdateCurTick = 0.0;
		}
		
		if (m_SelectionMode)
		{
			m_btnFnAddPlayersToGrp.Show(true);
			if (GetSelectedPlayers().Count() >= 1)
				m_btnFnAddPlayersToGrp.Enable(true);
			else
				m_btnFnAddPlayersToGrp.Enable(false);
		}else{
			 m_btnFnAddPlayersToGrp.Show(false);
		}

		if(m_PlayerEntries.Count() != GetPlayerListManager().GetCount())
		{
			UpdateEntries();
		}
		
		autoptr array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
		//Enable Actions
		m_ActionKickPlayer.Enable(selectedPlayers.Count() >= 1);
		m_ActionBanPlayer.Enable(selectedPlayers.Count() >= 1);
		m_ActionHeal.Enable(selectedPlayers.Count() >= 1);
		m_ActionMakeInvis.Enable(selectedPlayers.Count() >= 1);
		m_ActionKill.Enable(selectedPlayers.Count() >= 1);
		m_ActionSendMessage.Enable(selectedPlayers.Count() >= 1);
		m_ActionTpToMe.Enable(selectedPlayers.Count() >= 1);
		m_ActionTpMeTo.Enable(selectedPlayers.Count() == 1);
		m_ActionSpectate.Enable(selectedPlayers.Count() == 1 & !g_Game.IsSpectateMode());
		
		//Sliders apply btns
		m_BtnApplyHealth.Enable(selectedPlayers.Count() >= 1);
		m_BtnApplyBlood.Enable(selectedPlayers.Count() >= 1);
		m_BtnApplyShock.Enable(selectedPlayers.Count() >= 1);
		m_BtnApplyWater.Enable(selectedPlayers.Count() >= 1);
		m_BtnApplyEnergy.Enable(selectedPlayers.Count() >= 1);
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
	    foreach(VPPPlayerEntry entry : m_PlayerEntries)
	    {
			autoptr CheckBoxWidget checkBox = CheckBoxWidget.Cast(w);
			
			if(checkBox != null && checkBox == entry.GetCheckWidget())
			{
				SendForPlayerStats();
			}
	    }
		
		switch(w)
		{
			case m_SelectAllPlayers:
				//Clear stats data each time
				foreach(ref VPPPlayerStats pstat : m_PlayerStats)
				{
					if (pstat == null) continue;
					
					m_PlayerStats.RemoveItem(pstat);
					delete pstat;
				}
				
				foreach(ref VPPPlayerEntry e : m_PlayerEntries)
		    	{
					e.GetCheckWidget().SetChecked(m_SelectAllPlayers.IsChecked());
				}
				
			autoptr array<string> uids = GetSelectedPlayersIDs();
				if (uids.Count() >= 1)
					GetRPCManager().SendRPC( "RPC_PlayerManager", "GetPlayerStatsGroup", new Param1<ref array<string>>(uids), true);
			break;
			
			case m_BtnRefreshPlayerList:
			UpdateEntries();
			break;
			
			case m_ActionMakeInvis:
			GetRPCManager().SendRPC( "RPC_PlayerManager", "RequestInvisibility", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
			break;
			
			case m_ActionHeal:
			GetRPCManager().SendRPC( "RPC_PlayerManager", "HealPlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
			break;
			
			case m_ActionSpectate:
			SpectateTarget();
			break;
			
			case m_ActionTpToMe:
			GetRPCManager().SendRPC("RPC_PlayerManager","TeleportHandle",new Param2<bool,ref array<string>>(false,GetSelectedPlayersIDs()),true);
			break;
			
			case m_ActionTpMeTo:
			GetRPCManager().SendRPC("RPC_PlayerManager","TeleportHandle",new Param2<bool,ref array<string>>(true,GetSelectedPlayersIDs()),true);
			break;
			
			//Sliders apply buttons
			case m_BtnApplyHealth:
			UpdateStat("Health");
			GetVPPUIManager().DisplayNotification(string.Format("Applying new Health value on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyBlood:
			UpdateStat("Blood");
			GetVPPUIManager().DisplayNotification(string.Format("Applying new Blood value on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyShock:
			UpdateStat("Shock");
			GetVPPUIManager().DisplayNotification(string.Format("Applying new Shock value on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyWater:
			GetVPPUIManager().DisplayNotification(string.Format("Applying new Water value on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("Water");
			break;
			
			case m_BtnApplyEnergy:
			GetVPPUIManager().DisplayNotification(string.Format("Applying new Energy/Food value on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("Energy");
			break;
		}
		return false;
	}
	
	override void HideSubMenu()
	{
		super.HideSubMenu();
		if (m_SelectionMode)
			TogglePlayerSelectMode(false);
	}
	
	override void ShowSubMenu()
	{
		super.ShowSubMenu();
		if (!m_Init) return;
			//Refresh player list
			m_Grid_1_Count = 0;
			m_Grid_2_Count = 0;
			UpdateEntries();
		
	}
	
	/*
		CallBack method used from confirmation box
	*/
	void BanPlayer(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().SendRPC( "RPC_PlayerManager", "BanPlayer", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
		}
	}
	
	/*
		CallBack method used from confirmation box
	*/
	void KickPlayer(int result, string input)
	{
		if (result == DIAGRESULT.OK)
		{
			if (input == "")
				input = "Kicked By Server Admin!";
			
			GetRPCManager().SendRPC( "RPC_PlayerManager", "KickPlayer", new Param2<ref array<string>,string>(GetSelectedPlayersIDs(),input), true);
		}
	}
	
	/* Player list search filter function */
	void UpdateFilter()
	{
		foreach(VPPPlayerEntry en : m_PlayerEntries)
		{
			if (en != null && en.IsVisible())
				 en.SetVisible(false);
		}

        string strSearch = m_SearchInputBox.GetText();
        strSearch.ToLower();

        for (int x = 0; x < m_PlayerEntries.Count(); ++x)
        {
            autoptr VPPPlayerEntry entry = m_PlayerEntries.Get(x);            
			string strName = entry.GetPlayerName();
            string lowerCasedName = strName;
            lowerCasedName.ToLower();
			 
            if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
            {
			 	entry.SetVisible(false);
			 	m_PlayerList.Update();
                continue;
            }
			
            entry.SetVisible(true);
			m_PlayerList.Update();
        }
	}
	
	void InitSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, ref Object target)
	{
		Param1<Object> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Client )
		{
			if (data.param1 != null)
			{
				if (GetGame().GetPlayer() != null)
				{
					GetGame().ObjectDelete(GetGame().GetPlayer());
					GetGame().SelectPlayer(null, null);
				}
				
				ref VPPSpectateCam cam = VPPSpectateCam.Cast(GetGame().CreateObject( "VPPSpectateCam", data.param1.GetPosition(),true ));
				cam.SetTargetObj(PlayerBase.Cast(data.param1));
				cam.SetActive(true);
				g_Game.SetSpectateMode(true);
			}
		}
	}

	void SpectateTarget()
	{
		GetVPPUIManager().DisplayNotification("Requesting to sepctate selected player...");
		GetRPCManager().SendRPC("RPC_PlayerManager", "SpectatePlayer", new Param1<string>(GetSelectedPlayersIDs()[0]), true);
	}
	
	void KillSelectedPlayers(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().SendRPC( "RPC_PlayerManager", "KillPlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true );
		}
	}
	
	void SendMessageToPlayer(int result,string userInput)
	{
		if (result == DIAGRESULT.OK && userInput != "")
		{
			autoptr array<string> uids = GetSelectedPlayersIDs();
        	GetRPCManager().SendRPC( "RPC_PlayerManager", "SendMessage", new Param3<string,string,ref array<string>>("Server Admin:",userInput,uids), true );
		}
	}
	
	void TogglePlayerSelectMode(bool state)
	{
		m_SelectionMode = state;
	}	
	
	void HandlePlayerStats( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<ref PlayerStatsData> data;
		if(!ctx.Read(data)) return;
		
		autoptr PlayerStatsData statsData = data.param1;
		if( type == CallType.Client )
		{
			m_PlayerStats.Insert(new VPPPlayerStats(m_GridPlayerInfo, statsData.GetMap()));
			
			autoptr array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
			if (selectedPlayers.Count() == 1)
			{
				foreach(ref VPPPlayerStats stats : m_PlayerStats)
				{
					if (stats.GetID() == selectedPlayers.Get(0).GetID())
					{
						m_SliderBlood.SetCurrent(stats.GetStatVlaue("Blood").ToFloat());
						m_SliderHealth.SetCurrent(stats.GetStatVlaue("Health").ToFloat());
						m_SliderShock.SetCurrent(stats.GetStatVlaue("Shock").ToFloat());
						m_SliderWater.SetCurrent(stats.GetStatVlaue("Water").ToFloat());
						m_SliderEnergy.SetCurrent(stats.GetStatVlaue("Energy").ToFloat());
					}
				}
			}
		}
	}
	
	array<string> GetSelectedPlayersIDs()
	{
		autoptr array<string> uids = new array<string>;
		foreach(ref VPPPlayerEntry e : m_PlayerEntries)
    	{			
			if (e.GetCheckWidget().IsChecked())
			{
				uids.Insert(e.GetID());
			}
		}
		return uids;
	}
	
	private void IncremnetGrid()
	{
		if (m_CurrentGrid == m_Grid_1)
		{
			m_Grid_1_Count++;
		}else{
			m_Grid_2_Count++;
		}
	}
	
	private int GetGridCount(GridSpacerWidget grid)
	{
		if (grid == m_Grid_1)
		{
			return m_Grid_1_Count;
		}
		return m_Grid_2_Count;
	}

	private void UpdateStat(string statType)
	{
		autoptr array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
		if (selectedPlayers == null || selectedPlayers.Count() < 1) return;
		
		foreach(ref VPPPlayerEntry entry : selectedPlayers)
		{
			float stateNewValue;
			switch(statType)
			{
				case "Blood":
				stateNewValue = m_SliderBlood.GetCurrent();
				break;
				
				case "Health":
				stateNewValue = m_SliderHealth.GetCurrent();
				break;
				
				case "Shock":
				stateNewValue = m_SliderShock.GetCurrent();
				break;
				
				case "Water":
				stateNewValue = m_SliderWater.GetCurrent();
				break;
				
				case "Energy":
				stateNewValue = m_SliderEnergy.GetCurrent();
				break;
			}
			GetRPCManager().SendRPC("RPC_PlayerManager", "SetPlayerStats", new Param3<float,string,string>(stateNewValue,entry.GetID(),statType), true); //stat level, player id, stat type
		}
	}

	private void FinishPlayerSelect(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			HideSubMenu();
			//Send over list of selected players
			autoptr MenuPermissionsEditor permsEditor = MenuPermissionsEditor.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPermissionsEditor));
			if (permsEditor)
			{
				if (!permsEditor.IsSubMenuVisible())
					permsEditor.ShowSubMenu();
				
				permsEditor.AddMembersToSelectedGroup(GetSelectedPlayers());
			}
		}
	}
	
	private VPPPlayerEntry GetPlayerEntry(string id)
	{
		foreach(VPPPlayerEntry entry : m_PlayerEntries)
		{
			if(entry.GetID() == id)
			{
				return entry;
			}
		}
		return null;
	}
	
	private array<ref VPPPlayerEntry> GetSelectedPlayers()
	{
		autoptr array<ref VPPPlayerEntry> selected = new array<ref VPPPlayerEntry>;
		foreach(VPPPlayerEntry entry : m_PlayerEntries)
		{
			if (entry != null && entry.IsSelected())
				selected.Insert(entry);
		}
		return selected;
	}
	
	private void UpdateEntries()
	{
		autoptr array<ref VPPUser> playerList = GetPlayerListManager().GetUsers();
		
		if(playerList)
		{
			m_PlayerEntries = Compare(playerList);
			SendForPlayerStats();
		}
	}
	
	private void SendForPlayerStats()
	{
		delete m_PlayerStats;
		m_PlayerStats = new array<ref VPPPlayerStats>;
		
		delete IDs;
		IDs = new array<string>;
		
		foreach(VPPPlayerEntry entry : m_PlayerEntries)
		{
			if(entry.IsSelected())
			{
				IDs.Insert(entry.GetID());
			}
		}
		
		GetRPCManager().SendRPC("RPC_PlayerManager", "GetPlayerStatsGroup", new Param1<ref array<string>>(IDs), true);
	}
	
	private array<ref VPPPlayerEntry> Compare(array<ref VPPUser> b)
    {
        autoptr array<ref VPPPlayerEntry> new_list = new array<ref VPPPlayerEntry>;		
		
		foreach(VPPUser player : b)
		{
			string id = player.GetUserId();
			string name = player.GetUserName();
			autoptr VPPPlayerEntry entry = GetPlayerEntry(id);
			
			if(GetGridCount(m_Grid_1) == 100)
			{
				m_CurrentGrid = m_Grid_2;
			}else{
				m_CurrentGrid = m_Grid_1;
			}
			
			if(entry == null)
			{
				if(m_SelectAllPlayers.IsChecked())
				{
					new_list.Insert(new VPPPlayerEntry(m_CurrentGrid, name, id, true));
				}else{
					new_list.Insert(new VPPPlayerEntry(m_CurrentGrid, name, id));
				}
				IncremnetGrid();
			}
			else
			{
				if(m_SelectAllPlayers.IsChecked())
				{
					entry.SetSelected(true);
				}
				new_list.Insert(entry);
			}
			m_CurrentGrid.Update();
			m_PlayerList.Update();
		}
        return new_list;
    }
}