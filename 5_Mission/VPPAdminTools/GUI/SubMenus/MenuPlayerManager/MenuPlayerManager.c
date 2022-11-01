class MenuPlayerManager extends AdminHudSubMenu
{
	private bool 						  m_Init;
	private GridSpacerWidget 			  m_GridPlayerInfo;
	private GridSpacerWidget 			  m_GridPlayerList;
	private TextWidget                    m_txtPlayerCount;
	private TextWidget 					  m_txtPlayerCountSelected;
	private string 						  m_TotalPlayersCount;

	private ref CustomGridSpacer 			m_LastGrid;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	
	private EditBoxWidget				  m_SearchInputBox;
	private string 			 			  m_searchBoxStr;
	private ScrollWidget				  m_PlayerList;
	private ScrollWidget				  m_PlayerInfoScroll;
	private ButtonWidget	 			  m_BtnRefreshPlayerList;
	private CheckBoxWidget 		  		  m_SelectAllPlayers;
	private ButtonWidget 				  m_btnFnAddPlayersToGrp;
	private ref array<ref VPPPlayerEntry> m_PlayerEntries;
	private ref array<ref VPPPlayerStats> m_PlayerStats;
	private bool 						  m_SelectionMode;
	private ref array<string> 		  	  IDs;
	private const float      			  m_FilterUpdate = 0.35;
	private float      		 			  m_FilterUpdateCurTick;
	
	//Stats Sliders
	private SliderWidget m_SliderHealth;
	private SliderWidget m_SliderBlood;
	private SliderWidget m_SliderShock;
	private SliderWidget m_SliderWater;
	private SliderWidget m_SliderEnergy;
	private SliderWidget m_SliderTemperature;
	
	private ButtonWidget m_BtnApplyHealth;
	private ButtonWidget m_BtnApplyBlood;
	private ButtonWidget m_BtnApplyShock;
	private ButtonWidget m_BtnApplyWater;
	private ButtonWidget m_BtnApplyEnergy;
	private ButtonWidget m_BtnApplyTemperature;
	//-------------
		
	//Action Buttons--
	private ButtonWidget m_ActionHeal;
	private ButtonWidget m_ActionMakeVomit;
	private ButtonWidget m_ActionKickPlayer;
	private ButtonWidget m_ActionKill;
	private ButtonWidget m_ActionSendMessage;
	private ButtonWidget m_ActionBanPlayer;
	private ButtonWidget m_ActionTpToMe;
	private ButtonWidget m_ActionTpMeTo;
	private ButtonWidget m_ActionSpectate;
	private ButtonWidget m_ActionGiveGodmode;
	private ButtonWidget m_ActionUnlimitedAmmo;
	private ButtonWidget m_ActionInvisible;
	private ButtonWidget m_ActionFreezePlayer;
	private ButtonWidget m_ActionScalePlayer;
	private ButtonWidget m_ActionReturnPlayer;
	//----------------
	
	void MenuPlayerManager()
	{
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "InitSpectate", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "SetPlayerCount", this, SingleplayerExecutionType.Client);
		
		m_DataGrids     = new array<ref CustomGridSpacer>;
		m_PlayerEntries = new array<ref VPPPlayerEntry>;
		m_PlayerStats   = new array<ref VPPPlayerStats>;

		VPPAdminHud.m_OnUpdate.Insert(this.OnUpdate);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuPlayerManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_GridPlayerInfo   	   = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "GridPlayerInfo") );
		m_GridPlayerList 	   = GridSpacerWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "GridPlayerList") );
		m_PlayerList 	   	   = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PlayerList") );
		m_PlayerInfoScroll 	   = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PlayerInfoScroll") );
		m_SearchInputBox 	   = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "SearchInputBox") );
		m_txtPlayerCount 	   = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "txtPlayerCount") );
		m_txtPlayerCountSelected = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("txtPlayerCountSelected"));
		
		m_SelectAllPlayers 	   = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkSelectAllPlayers") );
		m_BtnRefreshPlayerList = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnRefreshPlayerList"));
		m_btnFnAddPlayersToGrp = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnFnAddPlayersToGrp"));
		GetVPPUIManager().HookConfirmationDialog(m_btnFnAddPlayersToGrp, M_SUB_WIDGET,this,"FinishPlayerSelect", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_NOTICE", "#VSTR_TOOLTIP_WRN_ADDPLAYERTOGRP");
		
		//Stats Sliders
		m_SliderHealth  = SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderHealth"));
		m_SliderBlood	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderBlood"));
		m_SliderShock	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderShock"));
		m_SliderWater	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderWater"));
		m_SliderEnergy	= SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderEnergy"));
		m_SliderTemperature = SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderTemperature"));
		
		m_BtnApplyHealth  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyHealth"));
		m_BtnApplyBlood   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyBlood"));
		m_BtnApplyShock   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyShock"));
		m_BtnApplyWater   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyWater"));
		m_BtnApplyEnergy  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyEnergy"));
		m_BtnApplyTemperature  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyTemperature"));
		//-------------
		
		//Action Buttons
		m_ActionMakeVomit 	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionMakeVomit"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionMakeVomit, M_SUB_WIDGET, this, "VomiteDiagResult", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TOOLTIP_TITLE_VOMIT", "#VSTR_TOOLTIP_VOMIT");

		m_ActionHeal  		   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionHeal"));
		m_ActionKickPlayer     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionKickPlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionKickPlayer, M_SUB_WIDGET,this,"KickPlayer", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TOOLTIP_TITLE_KICK", "#VSTR_TOOLTIP_KICK_REASON", true);
		
		m_ActionKill	   	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionKill"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionKill, M_SUB_WIDGET,this,"KillSelectedPlayers", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_KILL_PLAYERS", "#VSTR_TOOLTIP_KILL_PLAYERS");
		
		m_ActionSendMessage    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionSendMessage"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionSendMessage, M_SUB_WIDGET,this,"SendMessageToPlayer", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TOOLTIP_TITLE_SEND_MSG", "#VSTR_TOOLTIP_SEND_MSG", true);
		
		m_ActionBanPlayer  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionBanPlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionBanPlayer, M_SUB_WIDGET,this,"BanPlayer", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_BAN_PLAYER", "#VSTR_TOOLTIP_BAN_PLAYER");
		
		m_ActionReturnPlayer  	= ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ActionReturnPlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionReturnPlayer, M_SUB_WIDGET,this,"ReturnPlayer", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_RET_PLAYER", "#VSTR_TOOLTIP_RET_PLAYER");
		
		m_ActionTpToMe  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionTpToMe"));
		m_ActionTpMeTo  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionTpMeTo"));
		m_ActionSpectate  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionSpectate"));
		
		m_ActionGiveGodmode    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionGiveGodmode"));
		m_ActionUnlimitedAmmo  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionUnlimitedAmmo"));
		m_ActionInvisible  	   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionInvisible"));
		m_ActionFreezePlayer   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionFreezePlayer"));

		m_ActionScalePlayer   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionScalePlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionScalePlayer, M_SUB_WIDGET, this, "PlayerScaleDiag", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Set Scale", "Insert value to change to, between 0.01 and 100.0 (Some specific values will result in the player to be frozen and uncontrollable. To avoid, use rounded numbers)");
		//--------------
		
		//init first "page"
		ResetPages();
		UpdateEntries();
		m_Init = true;
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_PlayerList.Show(!state);
		m_PlayerInfoScroll.Show(!state);
	}
	
	void ResetPages()
	{
		foreach(CustomGridSpacer cusGrid : m_DataGrids){
			if (cusGrid != null)
				delete cusGrid;
		}
		m_DataGrids = new array<ref CustomGridSpacer>;
		
		//init first "page"
		m_DataGrids.Insert(new CustomGridSpacer(m_GridPlayerList));
		m_LastGrid = m_DataGrids[0];
		m_GridPlayerList.Update();
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
		
		array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
		int pCount = selectedPlayers.Count();

		m_txtPlayerCountSelected.Show(pCount >= 1);
		if (pCount >= 1)
			m_txtPlayerCountSelected.SetText(string.Format("[%1]", pCount.ToString()));

		//Enable Actions
		m_ActionKickPlayer.Enable(pCount >= 1);
		m_ActionBanPlayer.Enable(pCount >= 1);
		m_ActionHeal.Enable(pCount >= 1);
		m_ActionMakeVomit.Enable(pCount >= 1);
		m_ActionScalePlayer.Enable(pCount >= 1);
		m_ActionReturnPlayer.Enable(pCount >= 1);
		m_ActionKill.Enable(pCount >= 1);
		m_ActionSendMessage.Enable(pCount >= 1);
		m_ActionTpToMe.Enable(pCount >= 1);
		m_ActionTpMeTo.Enable(pCount == 1);
		m_ActionGiveGodmode.Enable(pCount == 1);
		m_ActionUnlimitedAmmo.Enable(pCount == 1);
		m_ActionInvisible.Enable(pCount >= 1);
		m_ActionFreezePlayer.Enable(pCount >= 1);
		m_ActionSpectate.Enable(pCount == 1 & !g_Game.IsSpectateMode());
		
		//Sliders apply btns
		m_BtnApplyHealth.Enable(pCount >= 1);
		m_BtnApplyBlood.Enable(pCount >= 1);
		m_BtnApplyShock.Enable(pCount >= 1);
		m_BtnApplyWater.Enable(pCount >= 1);
		m_BtnApplyEnergy.Enable(pCount >= 1);
		m_BtnApplyTemperature.Enable(pCount >= 1);
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
	    foreach(VPPPlayerEntry entry : m_PlayerEntries)
	    {
			CheckBoxWidget checkBox = CheckBoxWidget.Cast(w);
			
			if(checkBox != null && checkBox == entry.GetCheckWidget())
			{
				SendForPlayerStats();
			}
	    }
		
		switch(w)
		{
			case m_SelectAllPlayers:
				//Clear stats data each time
				foreach(VPPPlayerStats pstat : m_PlayerStats)
				{
					if (pstat == null) continue;
					
					m_PlayerStats.RemoveItem(pstat);
					delete pstat;
				}
				
				foreach(VPPPlayerEntry e : m_PlayerEntries)
		    	{
					e.GetCheckWidget().SetChecked(m_SelectAllPlayers.IsChecked());
					e.SetSelected(m_SelectAllPlayers.IsChecked());
				}
				
			array<string> uids = GetSelectedPlayersIDs();
				if (uids.Count() >= 1)
					GetRPCManager().VSendRPC( "RPC_PlayerManager", "GetPlayerStatsGroup", new Param1<ref array<string>>(uids), true);
			break;
			
			case m_BtnRefreshPlayerList:
			ResetPages();
			UpdateEntries();
			break;
			
			case m_ActionHeal:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "HealPlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
			break;
			
			case m_ActionGiveGodmode:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "GiveGodmode", new Param1<string>(GetSelectedPlayersIDs()[0]), true);
			break;
			
			case m_ActionUnlimitedAmmo:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "GiveUnlimitedAmmo", new Param1<string>(GetSelectedPlayersIDs()[0]), true);
			break;

			case m_ActionInvisible:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "RequestInvisibility", new Param1<ref array<int>>(GetSelectedPlayersSessionIDs()), true);
			break;

			case m_ActionFreezePlayer:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "FreezePlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
			break;
			
			case m_ActionSpectate:
			SpectateTarget();
			break;
			
			case m_ActionTpToMe:
			GetRPCManager().VSendRPC("RPC_PlayerManager","TeleportHandle",new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.BRING,GetSelectedPlayersIDs()),true);
			break;
			
			case m_ActionTpMeTo:
			GetRPCManager().VSendRPC("RPC_PlayerManager","TeleportHandle",new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.GOTO,GetSelectedPlayersIDs()),true);
			break;
			
			//Sliders apply buttons
			case m_BtnApplyHealth:
			UpdateStat("Health");
			GetVPPUIManager().DisplayNotification(string.Format("#VSTR_NOTIFY_APPLY_HEALTH" + " (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyBlood:
			UpdateStat("Blood");
			GetVPPUIManager().DisplayNotification(string.Format("#VSTR_NOTIFY_APPLY_BLOOD" + " (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyShock:
			UpdateStat("Shock");
			GetVPPUIManager().DisplayNotification(string.Format("#VSTR_NOTIFY_APPLY_SHOCK" + " (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			break;
			
			case m_BtnApplyWater:
			GetVPPUIManager().DisplayNotification(string.Format("#VSTR_NOTIFY_APPLY_WATER" + " (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("Water");
			break;
			
			case m_BtnApplyEnergy:
			GetVPPUIManager().DisplayNotification(string.Format("#VSTR_NOTIFY_APPLY_ENERGY" + " (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("Energy");
			break;
		}
		return false;
	}

	void PlayerScaleDiag(int result, string inputText)
	{
		if(result == DIAGRESULT.OK)
		{
			float scale = inputText.ToFloat();
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "ChangePlayerScale", new Param2<ref array<string>,float>(GetSelectedPlayersIDs(), Math.Clamp(scale, 0.01, 100.0)), true);
		}
	}
	
	void VomiteDiagResult(int result, string inputText)
	{
		if(result == DIAGRESULT.OK)
		{
			int time = inputText.ToInt();

			if(time > 0)
			{
				GetRPCManager().VSendRPC( "RPC_PlayerManager", "MakePlayerVomit", new Param2<ref array<string>, int>(GetSelectedPlayersIDs(), time), true);
			}
		}
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
			UpdateEntries();
		
	}

	/*
		CallBack method used from confirmation box
	*/
	void ReturnPlayer(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC("RPC_PlayerManager", "TeleportHandle", new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.RETURN,GetSelectedPlayersIDs()), true);
		}
	}
	
	/*
		CallBack method used from confirmation box
	*/
	void BanPlayer(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "BanPlayer", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
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
				input = "#VSTR_NOTIFY_KICK_MESSAGE_PLAYER";
			
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "KickPlayer", new Param2<ref array<string>,string>(GetSelectedPlayersIDs(),input), true);
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
            VPPPlayerEntry entry = m_PlayerEntries.Get(x);            
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
	
	void InitSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
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
				
				VPPSpectateCam cam = VPPSpectateCam.Cast(GetGame().CreateObject( "VPPSpectateCam", data.param1.GetPosition(),true ));
				cam.SetTargetObj(PlayerBase.Cast(data.param1));
				cam.SetActive(true);
				g_Game.SetSpectateMode(true);
			}
		}
	}

	void SpectateTarget()
	{
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_REQ");
		GetRPCManager().VSendRPC("RPC_PlayerManager", "SpectatePlayer", new Param1<string>(GetSelectedPlayersIDs()[0]), true);
	}
	
	void KillSelectedPlayers(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "KillPlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true );
		}
	}
	
	void SendMessageToPlayer(int result,string userInput)
	{
		if (result == DIAGRESULT.OK && userInput != "")
		{
			array<string> uids = GetSelectedPlayersIDs();
        	GetRPCManager().VSendRPC( "RPC_PlayerManager", "SendMessage", new Param3<string,string,ref array<string>>("Server Admin:",userInput,uids), true );
		}
	}
	
	void TogglePlayerSelectMode(bool state)
	{
		m_SelectionMode = state;
	}	
	
	void SetPlayerCount(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<string> data;
			if(!ctx.Read(data))
				return;

			m_TotalPlayersCount = data.param1;
			m_txtPlayerCount.SetText(m_TotalPlayersCount);
		}
	}

	void HandlePlayerStats( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<ref PlayerStatsData> data;
		if(!ctx.Read(data)) return;
		
		PlayerStatsData statsData = data.param1;
		if( type == CallType.Client )
		{
			m_PlayerStats.Insert(new VPPPlayerStats(m_GridPlayerInfo, statsData.GetMap()));
			
			array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
			if (selectedPlayers.Count() == 1)
			{
				foreach(VPPPlayerStats stats : m_PlayerStats)
				{
					if (stats.GetID() == selectedPlayers.Get(0).GetID())
					{
						m_SliderBlood.SetCurrent(stats.GetStatValue("Blood").ToFloat());
						m_SliderHealth.SetCurrent(stats.GetStatValue("Health").ToFloat());
						m_SliderShock.SetCurrent(stats.GetStatValue("Shock").ToFloat());
						m_SliderWater.SetCurrent(stats.GetStatValue("Water").ToFloat());
						m_SliderEnergy.SetCurrent(stats.GetStatValue("Energy").ToFloat());
					}
				}
			}
		}
	}
	
	array<string> GetSelectedPlayersIDs()
	{
		array<string> uids = new array<string>;
		foreach(VPPPlayerEntry e : m_PlayerEntries)
    	{			
			if (e.GetCheckWidget().IsChecked())
			{
				uids.Insert(e.GetID());
			}
		}
		return uids;
	}

	array<int> GetSelectedPlayersSessionIDs()
	{
		array<int> uids = new array<int>;
		foreach(VPPPlayerEntry e : m_PlayerEntries)
    	{			
			if (e.GetCheckWidget().IsChecked())
			{
				uids.Insert(e.GetSessionId());
			}
		}
		return uids;
	}
	
	private void UpdateStat(string statType)
	{
		array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
		if (selectedPlayers == null || selectedPlayers.Count() < 1) return;
		
		foreach(VPPPlayerEntry entry : selectedPlayers)
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
			GetRPCManager().VSendRPC("RPC_PlayerManager", "SetPlayerStats", new Param3<float,string,string>(stateNewValue,entry.GetID(),statType), true); //stat level, player id, stat type
		}
	}

	private void FinishPlayerSelect(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			HideSubMenu();
			//Send over list of selected players
			MenuPermissionsEditor permsEditor = MenuPermissionsEditor.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPermissionsEditor));
			if (permsEditor)
			{
				if (!permsEditor.IsSubMenuVisible())
					permsEditor.ShowSubMenu();
				
				permsEditor.AddMembersToSelectedGroup(GetSelectedPlayers());
			}
		}
	}
	
	VPPPlayerEntry GetPlayerEntry(string id)
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
	
	array<ref VPPPlayerEntry> GetSelectedPlayers()
	{
		array<ref VPPPlayerEntry> selected = new array<ref VPPPlayerEntry>;
		foreach(VPPPlayerEntry entry : m_PlayerEntries)
		{
			if (entry != null && entry.IsSelected())
				selected.Insert(entry);
		}
		return selected;
	}
	
	private void UpdateEntries()
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GetPlayerCount", NULL, true);
		array<ref VPPUser> playerList = GetPlayerListManager().GetUsers();
		if(playerList)
		{
			m_PlayerEntries = Compare(playerList);
			SendForPlayerStats();
		}
	}
	
	void SendForPlayerStats()
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
		
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GetPlayerStatsGroup", new Param1<ref array<string>>(IDs), true);
	}
	
	private array<ref VPPPlayerEntry> Compare(array<ref VPPUser> b)
    {
        array<ref VPPPlayerEntry> new_list = new array<ref VPPPlayerEntry>;		
		
		//m_txtPlayerCount.SetText(b.Count().ToString());
		
		foreach(VPPUser player : b)
		{
			string id = player.GetUserId();
			string name = player.GetUserName();
			VPPPlayerEntry entry = GetPlayerEntry(id);
			
			if(m_LastGrid.GetContentCount() == 100)
			{
				m_LastGrid = new CustomGridSpacer(m_GridPlayerList);
			 	m_DataGrids.Insert(m_LastGrid);
			}
			
			if(entry == null)
			{
				if(m_LastGrid.GetContentCount() < 100)
				{
					VPPPlayerEntry NewEntry = new VPPPlayerEntry(m_LastGrid.GetGrid(), name, id, player.GetSessionId(),m_SelectAllPlayers.IsChecked());
					new_list.Insert(NewEntry);
					m_LastGrid.AddWidget(NewEntry.m_EntryBox);
				}
				m_LastGrid.GetGrid().Update();
				m_GridPlayerList.Update();
			}
			else
			{
				if(m_LastGrid.GetContentCount() == 100){
					m_LastGrid = new CustomGridSpacer(m_GridPlayerList);
				 	m_DataGrids.Insert(m_LastGrid);
				}
				
				entry.SetSelected(m_SelectAllPlayers.IsChecked() || entry.IsSelected());
				entry.RedrawWidgets(m_LastGrid,m_SelectAllPlayers.IsChecked() || entry.IsSelected());
				new_list.Insert(entry);
			}
			m_PlayerList.Update();
			m_GridPlayerList.Update();
		}
        return new_list;
    }
}