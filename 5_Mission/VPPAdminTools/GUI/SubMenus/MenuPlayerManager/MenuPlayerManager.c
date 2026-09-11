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
	private SliderWidget m_SliderHeatBuffer;
	
	private ButtonWidget m_BtnApplyHealth;
	private ButtonWidget m_BtnApplyBlood;
	private ButtonWidget m_BtnApplyShock;
	private ButtonWidget m_BtnApplyWater;
	private ButtonWidget m_BtnApplyEnergy;
	private ButtonWidget m_BtnApplyTemperature;
	private ButtonWidget m_BtnApplyHeatBuffer;
	//-------------
	
	//Vitals panel tabs (EDIT VITALS | MODIFIERS)--
	private ButtonWidget 			   m_BtnTabVitals;
	private ButtonWidget 			   m_BtnTabModifiers;
	private TextWidget 				   m_TabVitalsLbl;
	private TextWidget 				   m_TabModifiersLbl;
	private Widget 					   m_SlidersStack;
	private ScrollWidget 			   m_ModifiersScroll;
	private GridSpacerWidget 		   m_ModifiersGrid;
	private bool 					   m_ModifiersTabActive;
	private string 					   m_ModifiersTargetId;
	private ref array<ref VPPModifierEntry> m_ModifierEntries;
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
	private ButtonWidget m_ActionUnfreezePlayer;
	private ButtonWidget m_ActionStopBleeding;
	private ButtonWidget m_ActionSetPosition;
	private ButtonWidget m_ActionClearInventory;
	private ButtonWidget m_ActionScalePlayer;
	private ButtonWidget m_ActionReturnPlayer;
	//----------------
	
	//Collapsible action sections--
	private ref VPPCollapsibleSection m_SectionMgmt;
	private ref VPPCollapsibleSection m_SectionMove;
	private ref VPPCollapsibleSection m_SectionState;
	private ref VPPCollapsibleSection m_SectionGear;
	private ref VPPCollapsibleSection m_SectionTools;
	//----------------
	
	void MenuPlayerManager()
	{
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerStats", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "SetPlayerCount", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuPlayerManager", "HandlePlayerModifiers", this, SingleplayerExecutionType.Client);
		
		m_DataGrids       = new array<ref CustomGridSpacer>;
		m_PlayerEntries   = new array<ref VPPPlayerEntry>;
		m_PlayerStats     = new array<ref VPPPlayerStats>;
		m_ModifierEntries = new array<ref VPPModifierEntry>;
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
		m_SliderHeatBuffer  = SliderWidget.Cast(M_SUB_WIDGET.FindAnyWidget("SliderHeatBuffer"));
		
		m_BtnApplyHealth  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyHealth"));
		m_BtnApplyBlood   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyBlood"));
		m_BtnApplyShock   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyShock"));
		m_BtnApplyWater   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyWater"));
		m_BtnApplyEnergy  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyEnergy"));
		m_BtnApplyTemperature  = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyTemperature"));
		m_BtnApplyHeatBuffer   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyHeatBuffer"));
		
		//value readouts: comfort shown as -100..100 (%), heat buffer as -30..30
		VPPSliderRowHandler rowHandler;
		m_SliderTemperature.GetScript(rowHandler);
		if (rowHandler) rowHandler.SetDisplayTransform(2.0, -100.0);
		m_SliderHeatBuffer.GetScript(rowHandler);
		if (rowHandler) rowHandler.SetDisplayTransform(1.0, -30.0);
		//-------------
		
		//Vitals panel tabs + modifiers list
		m_BtnTabVitals    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabVitals"));
		m_BtnTabModifiers = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnTabModifiers"));
		m_TabVitalsLbl    = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TabVitalsLbl"));
		m_TabModifiersLbl = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TabModifiersLbl"));
		m_SlidersStack    = M_SUB_WIDGET.FindAnyWidget("SlidersStack");
		m_ModifiersScroll = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ModifiersScroll"));
		m_ModifiersGrid   = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ModifiersGrid"));
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
		m_ActionUnfreezePlayer = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionUnfreezePlayer"));
		m_ActionStopBleeding   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionStopBleeding"));
		
		m_ActionSetPosition    = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionSetPosition"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionSetPosition, M_SUB_WIDGET, this, "SetPositionDiag", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Set Position", "Enter coordinates as \"X Y Z\" (or \"X Z\" to snap to ground)", true);
		
		m_ActionClearInventory = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionClearInventory"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionClearInventory, M_SUB_WIDGET, this, "ClearInventoryDiag", DIAGTYPE.DIAG_YESNO, "Clear Inventory", "Remove ALL items from the selected player(s)? This cannot be undone.");

		m_ActionScalePlayer   = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ActionScalePlayer"));
		GetVPPUIManager().HookConfirmationDialog(m_ActionScalePlayer, M_SUB_WIDGET, this, "PlayerScaleDiag", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Set Scale", "Insert value to change to, between 0.01 and 100.0 (Some specific values will result in the player to be frozen and uncontrollable. To avoid, use rounded numbers)");
		//--------------
		
		//Collapsible action sections
		m_SectionMgmt  = new VPPCollapsibleSection(M_SUB_WIDGET, "SectionHeader_Mgmt",  "SectionContent_Mgmt",  "SectionChevron_Mgmt");
		m_SectionMove  = new VPPCollapsibleSection(M_SUB_WIDGET, "SectionHeader_Move",  "SectionContent_Move",  "SectionChevron_Move");
		m_SectionState = new VPPCollapsibleSection(M_SUB_WIDGET, "SectionHeader_State", "SectionContent_State", "SectionChevron_State");
		m_SectionGear  = new VPPCollapsibleSection(M_SUB_WIDGET, "SectionHeader_Gear",  "SectionContent_Gear",  "SectionChevron_Gear");
		m_SectionTools = new VPPCollapsibleSection(M_SUB_WIDGET, "SectionHeader_Tools", "SectionContent_Tools", "SectionChevron_Tools");
		//--------------
		
		//init first "page"
		ResetPages();
		UpdateEntries();
		m_Init = true;
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
		m_ActionUnfreezePlayer.Enable(pCount >= 1);
		m_ActionStopBleeding.Enable(pCount >= 1);
		m_ActionSetPosition.Enable(pCount >= 1);
		m_ActionClearInventory.Enable(pCount >= 1);
		m_ActionSpectate.Enable(pCount == 1 && !g_Game.IsSpectateMode());
		
		//Sliders apply btns
		m_BtnApplyHealth.Enable(pCount >= 1);
		m_BtnApplyBlood.Enable(pCount >= 1);
		m_BtnApplyShock.Enable(pCount >= 1);
		m_BtnApplyWater.Enable(pCount >= 1);
		m_BtnApplyEnergy.Enable(pCount >= 1);
		m_BtnApplyTemperature.Enable(pCount >= 1);
		m_BtnApplyHeatBuffer.Enable(pCount >= 1);
		
		//Modifiers tab needs exactly one target
		m_BtnTabModifiers.Enable(pCount == 1);
		if (m_ModifiersTabActive)
		{
			if (pCount != 1)
			{
				SetPanelTab(false);
			}
			else if (selectedPlayers.Get(0).GetID() != m_ModifiersTargetId)
			{
				RequestModifiers(selectedPlayers.Get(0).GetID());
			}
		}
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
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "FreezePlayers", new Param2<ref array<string>,int>(GetSelectedPlayersIDs(), 1), true);
			break;

			case m_ActionUnfreezePlayer:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "FreezePlayers", new Param2<ref array<string>,int>(GetSelectedPlayersIDs(), 0), true);
			break;

			case m_ActionStopBleeding:
			GetRPCManager().VSendRPC( "RPC_PlayerManager", "StopBleedingPlayers", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
			GetVPPUIManager().DisplayNotification(string.Format("Stopped bleeding on (%1) player(s)",GetSelectedPlayers().Count().ToString()));
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
			
			case m_BtnApplyTemperature:
			GetVPPUIManager().DisplayNotification(string.Format("Applied heat comfort to (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("Temperature");
			break;
			
			case m_BtnApplyHeatBuffer:
			GetVPPUIManager().DisplayNotification(string.Format("Applied heat buffer to (%1) player(s)",GetSelectedPlayers().Count().ToString()));
			UpdateStat("HeatBuffer");
			break;
			
			case m_BtnTabVitals:
			SetPanelTab(false);
			break;
			
			case m_BtnTabModifiers:
			SetPanelTab(true);
			break;
		}
		return false;
	}
	
	/*
		Switches the vitals panel between the slider editor and the
		dynamic modifiers list (requires exactly one selected player)
	*/
	void SetPanelTab(bool showModifiers)
	{
		array<ref VPPPlayerEntry> selectedPlayers = GetSelectedPlayers();
		if (showModifiers && selectedPlayers.Count() != 1)
			showModifiers = false;
		
		m_ModifiersTabActive = showModifiers;
		m_SlidersStack.Show(!showModifiers);
		m_ModifiersScroll.Show(showModifiers);
		
		if (showModifiers)
		{
			m_TabVitalsLbl.SetColor(ARGB(255, 154, 160, 166));    //text-secondary
			m_TabModifiersLbl.SetColor(ARGB(255, 232, 234, 237)); //text-primary
			RequestModifiers(selectedPlayers.Get(0).GetID());
		}
		else
		{
			m_TabVitalsLbl.SetColor(ARGB(255, 232, 234, 237));
			m_TabModifiersLbl.SetColor(ARGB(255, 154, 160, 166));
			m_ModifiersTargetId = "";
		}
	}
	
	void RequestModifiers(string playerId)
	{
		m_ModifiersTargetId = playerId;
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GetPlayerModifiers", new Param1<string>(playerId), true);
	}
	
	void RefreshModifiers()
	{
		if (m_ModifiersTabActive && m_ModifiersTargetId != "")
			RequestModifiers(m_ModifiersTargetId);
	}
	
	/*
		Called by VPPModifierEntry toggle buttons.
		Modifier (de)activation settles on the next server modifier tick,
		so re-fetch the list shortly after to confirm the real state.
	*/
	void RequestModifierToggle(int modifierId, bool state)
	{
		if (m_ModifiersTargetId == "") return;
		
		GetRPCManager().VSendRPC("RPC_PlayerManager", "SetPlayerModifier", new Param3<string,int,bool>(m_ModifiersTargetId, modifierId, state), true);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.RefreshModifiers, 1500, false);
	}
	
	void HandlePlayerModifiers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<string, ref array<int>, ref array<string>, ref array<bool>> data;
		if (!ctx.Read(data)) return;
		
		if (type != CallType.Client) return;
		if (data.param1 != m_ModifiersTargetId) return; //stale response
		
		m_ModifierEntries.Clear();
		
		array<int>    ids     = data.param2;
		array<string> names   = data.param3;
		array<bool>   actives = data.param4;
		
		for (int i = 0; i < ids.Count(); ++i)
		{
			m_ModifierEntries.Insert(new VPPModifierEntry(m_ModifiersGrid, this, ids[i], PrettifyModifierName(names[i]), actives[i]));
		}
		
		m_ModifiersGrid.Update();
		m_ModifiersScroll.Update();
	}
	
	//"WoundInfectStage1" -> "Wound Infect Stage 1"
	private string PrettifyModifierName(string rawName)
	{
		string output = "";
		for (int i = 0; i < rawName.Length(); ++i)
		{
			string c = rawName.Get(i);
			string upper = c;
			upper.ToUpper();
			
			bool isUpperOrDigit = (c == upper);
			if (i > 0 && isUpperOrDigit)
				output += " ";
			
			output += c;
		}
		return output;
	}
	
	/*
		CallBack method used from confirmation box
		Accepts "X Y Z" or "X Z" (Y resolved to surface level server-side)
	*/
	void SetPositionDiag(int result, string inputText)
	{
		if (result != DIAGRESULT.OK || inputText == "") return;
		
		inputText.Replace(","," ");
		array<string> tokens = new array<string>;
		inputText.Split(" ", tokens);
		
		//drop empties left by double spaces
		for (int i = tokens.Count() - 1; i >= 0; --i)
		{
			tokens[i] = tokens[i].Trim();
			if (tokens[i] == "")
				tokens.Remove(i);
		}
		
		vector pos;
		if (tokens.Count() == 3)
			pos = Vector(tokens[0].ToFloat(), tokens[1].ToFloat(), tokens[2].ToFloat());
		else if (tokens.Count() == 2)
			pos = Vector(tokens[0].ToFloat(), 0, tokens[1].ToFloat()); //y<=0 snaps to surface
		else
		{
			GetVPPUIManager().DisplayNotification("Set Position: invalid coordinates! Use \"X Y Z\" or \"X Z\"");
			return;
		}
		
		GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>,string,vector>(GetSelectedPlayersIDs(), "", pos), true);
	}
	
	/*
		CallBack method used from confirmation box
	*/
	void ClearInventoryDiag(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC("RPC_PlayerManager", "ClearInventory", new Param1<ref array<string>>(GetSelectedPlayersIDs()), true);
		}
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
	
	void SpectateTarget()
	{
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SPECTATE_REQ");
		//route through the spectate client handler (NOT the legacy RPC_PlayerManager
		//path) so the death-grace preference is synced to the server before entering
		GetSpectateClient().RequestSpectate(GetSelectedPlayersIDs()[0]);
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
						
						if (stats.HasStat("Temperature"))
							m_SliderTemperature.SetCurrent((stats.GetStatValue("Temperature").ToFloat() + 1.0) * 50.0); //-1..+1 -> 0..100
						
						if (stats.HasStat("HeatBuffer"))
							m_SliderHeatBuffer.SetCurrent(stats.GetStatValue("HeatBuffer").ToFloat() + 30.0); //-30..+30 -> 0..60
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
				
				case "Temperature":
				//slider 0..100 -> heat comfort -1..+1
				stateNewValue = (m_SliderTemperature.GetCurrent() / 50.0) - 1.0;
				break;
				
				case "HeatBuffer":
				//slider 0..60 -> heat buffer -30..+30
				stateNewValue = m_SliderHeatBuffer.GetCurrent() - 30.0;
				break;
			}
			GetRPCManager().VSendRPC("RPC_PlayerManager", "SetPlayerStats", new Param3<float,string,string>(stateNewValue,entry.GetID(),statType), true); //stat level, player id, stat type
		}
		
		//re-fetch stats so the info cards reflect the newly applied values
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.SendForPlayerStats, 350, false);
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
		GetPlayerListManager().RequestForceSync();
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GetPlayerCount", NULL, true);
		m_PlayerEntries = Compare();
		SendForPlayerStats();
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
	
	private array<ref VPPPlayerEntry> Compare()
    {
        array<ref VPPPlayerEntry> new_list = new array<ref VPPPlayerEntry>;		
		array<ref VPPUser> playerList = GetPlayerListManager().GetUsers();

		//m_txtPlayerCount.SetText(playerList.Count().ToString());
		for (int i = 0; i < playerList.Count(); ++i)
		{
			VPPUser player = playerList[i];
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