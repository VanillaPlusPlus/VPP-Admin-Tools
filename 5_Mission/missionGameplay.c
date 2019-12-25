modded class MissionGameplay
{
	private bool  m_Toggles = false;
	Object targetObj;
	
	void MissionGameplay()
	{
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_HandleInvisiblity", "HandleInvisiblity", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_HandleFreeCam", "HandleFreeCam", this, SingeplayerExecutionType.Server );
		GetRPCManager().SendRPC( "RPC_PermitManager", "EnableToggles", NULL, true );
	}
	
	//Returns true when opens
	bool OpenToolBar()
	{
		ref VPPAdminHud adminMenu;
		if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
		{
			if (!adminMenu.IsShowing())
			{
				adminMenu.ShowMenu();
				return true;
			}else{
				adminMenu.HideMenu();
				return false;
			}
		}else{
			GetVPPUIManager().CreateMenu(VPPAdminHud,VPP_ADMIN_HUD);
			return true;
		}
		
		return false;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate( timeslice );
		
		Man player = GetGame().GetPlayer();
		PlayerBase playerPB = PlayerBase.Cast(player);
		Input input = GetGame().GetInput();
		
		if (input.LocalPress("UAUIBack", false))
		{
			VPPAdminHud AdminTab = VPPAdminHud.Cast( GetUIManager().FindMenu(VPP_ADMIN_HUD) );
			if (AdminTab != NULL && AdminTab.IsShowing())
			{
				AdminTab.HideMenu();
			}
		}
		
		if (m_Toggles)
		{
			if ( input.LocalPress("UAToggleAdminTools", false) && !GetVPPUIManager().IsTyping())
			{
				OpenToolBar();
			}
			
			//Shortcut key to the commands console
			if ( input.LocalPress("UAToggleCmdConsole", false) && !GetVPPUIManager().IsTyping())
			{
				if (OpenToolBar())
				{
					autoptr VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
					if (rootMenu.GetSubMenuByType(MenuCommandsConsole) == null)
						rootMenu.CreateSubMenu(MenuCommandsConsole);
					else
						MenuCommandsConsole.Cast(rootMenu.GetSubMenuByType(MenuCommandsConsole)).ShowSubMenu();
				}
			}
			
			if (GetVPPUIManager().GetKeybindsStatus() || GetVPPUIManager().IsTyping()) return;
			
			if ( input.LocalPress("UATeleportToCrosshair", false) || input.LocalHold("UATeleportToCrosshair", false))
			{
				//float oDistance = vector.Distance(GetGame().GetPlayer().GetPosition(),g_Game.GetCursorPos());
				//if (oDistance >= 1000) return;
                GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", new Param1<vector>(g_Game.GetCursorPos()), true);
			}
			
			if ( input.LocalPress("UADeleteObjCrosshair", false))
			{
				targetObj = g_Game.getObjectAtCrosshair(1000.0, 1.5,NULL);
				if (targetObj != null)
				{
					//Lock Controls and show mouse cursor 
					GetVPPUIManager().SetKeybindsStatus(true); //Lock shortcut keys
					GetGame().GetInput().ChangeGameFocus(1);
					GetGame().GetUIManager().ShowUICursor( true );
					PlayerControlDisable(INPUT_EXCLUDE_ALL);
					
					//Show confirmation of delete
					autoptr VPPDialogBox dialogBox = GetVPPUIManager().CreateDialogBox(null,true);
					dialogBox.InitDiagBox(DIAGTYPE.DIAG_YESNO, "Delete Object?", "Are you sure you wish to delete: ["+targetObj.GetType()+"]?", this);
				}
			}
			
			if ( input.LocalPress("UAToggleGodMode", false))
			{
				GetRPCManager().SendRPC( "RPC_PlayerManager", "ToggleGodmode", NULL, true);
			}
			
			if (input.LocalPress("UAToggleFreeCam", false))
			{
				GetRPCManager().SendRPC( "RPC_AdminTools", "ToggleFreeCam", NULL, true);
			}

			if (input.LocalPress("UASupriseButtSex", false))
			{
				array<string> itemTypes = { "barrel_green","Truck_01_WheelDouble","TransitBusWheel","TransitBusWheelDouble","Refridgerator","SeaChest","PowerGenerator","WoodenLog" };
				GetRPCManager().SendRPC( "RPC_MissionServer", "HandleChatCommand", new Param1<string>("/sph "+itemTypes.GetRandomElement()), true);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ThrowInHands, 1000, false);
			}
		}
		
		//Spectate mode Exit button
		if ( input.LocalPress("UAExitSpectate", false) && g_Game.IsSpectateMode())
		{
			g_Game.ReconnectToCurrentSession();
			g_Game.SetSpectateMode(false);
		}
	}
	
	void OnDiagResult(int result)
	{
		GetVPPUIManager().SetKeybindsStatus(false); //Lock shortcut keys
		GetGame().GetUIManager().ShowUICursor( false );
		GetGame().GetInput().ResetGameFocus();
		GetGame().GetInput().ChangeGameFocus(-1);
		PlayerControlEnable(false);
		
		if (result == DIAGRESULT.YES)
		{
			//Proceed to delete object at cursor
			if (targetObj != null)
				GetRPCManager().SendRPC( "RPC_AdminTools", "DeleteObject", NULL, true, NULL, targetObj);
		}
	}
	
	void EnableToggles( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
	   	Param1<bool> data;
        if ( !ctx.Read( data ) || sender != null ) return;

        if (type == CallType.Client)
        {
        	m_Toggles = data.param1;
			
			if (!m_Toggles)
			{
				autoptr VPPAdminHud adminMenu;
				if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
				{
					if (adminMenu.IsShowing())
					{
						adminMenu.HideMenu();
						GetVPPUIManager().DestroyMenuInstanceByType(VPPAdminHud);
					}
				}
			}
        }
    }
	
	void HandleInvisiblity(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
        if ( !ctx.Read( data ) || !m_Toggles || sender != null ) return;
		
		if (type == CallType.Client)
        {
			autoptr Entity pe = GetGame().GetPlayer();
        	if (pe != null && GetGame().GetPlayer().IsAlive())
			{
				pe.SetInvisible(data.param1);
			}
        }
	}
	
	void HandleFreeCam(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<bool> data;
        if ( !ctx.Read( data ) || !m_Toggles || sender != null ) return;
		
		if (type == CallType.Client)
        {
			if (data.param1)
			{
				PlayerBase player = PlayerBase.Cast( GetGame().GetPlayer() );
				if( player == null ) return;
				
				HumanInputController hic = player.GetInputController();
				if (IsFreeCamActive())
				{
					DestroyFreeCam();
				}else{
					if (!GetVPPUIManager().IsTyping())
						CreateFreeCamInstance();
				}
				hic.SetDisabled(IsFreeCamActive());
			}
		}
	}
	
	void ThrowInHands()
	{
		if (!m_Toggles) return;

		DayZPlayer player = DayZPlayer.Cast(GetGame().GetPlayer());
		InventoryItem item = InventoryItem.Cast(GetGame().GetPlayer().GetHumanInventory().GetEntityInHands());
		if( item )
		{
			Human hmn = GetGame().GetPlayer();
			HumanCommandWeapons	hcw = hmn.GetCommandModifier_Weapons();
			
			float lr = hcw.GetBaseAimingAngleLR();
			float ud = hcw.GetBaseAimingAngleUD();
			vector aimOrientation = player.GetOrientation();
			aimOrientation[0] = aimOrientation[0] + lr;
			aimOrientation[1] = aimOrientation[1] + ud;
			
			player.GetHumanInventory().ThrowEntity(item, aimOrientation.AnglesToVector(),10000);
		}
	}

	override void OnKeyPress(int key) 
	{
        VPPScriptedMenu menu = VPPScriptedMenu.Cast(GetUIManager().GetMenu());
        if (menu != null && key == KeyCode.KC_ESCAPE)
        {
        	return;
        }
        super.OnKeyPress(key);
        m_Hud.KeyPress(key);
    }
};

ref VPPFreeCam m_freecam;
bool m_FreeCamStatus;
static void CreateFreeCamInstance()
{
	vector pos;
	MiscGameplayFunctions.GetHeadBonePos(PlayerBase.Cast(GetGame().GetPlayer()), pos);
	m_freecam = VPPFreeCam.Cast(g_Game.CreateObject( "VPPFreeCam", pos, true ));
	m_freecam.SetActive(true);
	m_FreeCamStatus = true;
	//GetGame().GetMission().GetHud().Show( false );
	MissionGameplay.Cast( GetGame().GetMission() ).PlayerControlDisable(INPUT_EXCLUDE_ALL);
}

static bool IsFreeCamActive()
{
	return m_FreeCamStatus;
}

static VPPFreeCam GetFreeCamInstance()
{
	return m_freecam;
}

static void DestroyFreeCam()
{
	if (m_freecam == null) return;
	m_freecam.SetActive(false);
	m_FreeCamStatus = false;
	
	autoptr VPPAdminHud adminMenu;
	if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
	{
		if (adminMenu != null)
			adminMenu.HideMenu();
	}

	GetGame().ObjectDelete(m_freecam);
	GetGame().SelectPlayer(null,GetGame().GetPlayer());
	GetGame().GetMission().GetHud().Show( true );
	
	GetGame().GetInput().ResetGameFocus();
	GetGame().GetUIManager().ShowCursor(false);
	MissionGameplay.Cast( GetGame().GetMission() ).PlayerControlEnable(false);
}

/*
modded class MissionMainMenu
{
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		
		if ( key == KeyCode.KC_Q )
		{
			Print(GetGame().Connect( null, "127.0.0.1", 2302, "" ));
		}
	}
};
*.