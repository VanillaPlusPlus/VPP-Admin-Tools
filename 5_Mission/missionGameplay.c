modded class MissionGameplay
{
	private bool  m_Toggles = false;
	private bool  m_FreeCamStatus;
	private VPPFreeCam m_freecam;
	
	void MissionGameplay()
	{
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_HandleInvisiblity", "HandleInvisiblity", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_HandleFreeCam", "HandleFreeCam", this, SingeplayerExecutionType.Server );
		GetRPCManager().SendRPC( "RPC_PermitManager", "EnableToggles", NULL, true );
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
				ref VPPAdminHud adminMenu;
				if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
				{
					if (!adminMenu.IsShowing())
					adminMenu.ShowMenu();
					else
					adminMenu.HideMenu();
				}else{
					GetVPPUIManager().CreateMenu(VPPAdminHud,VPP_ADMIN_HUD);
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
				GetRPCManager().SendRPC( "RPC_AdminTools", "DeleteObject", NULL, true, NULL, g_Game.getObjectAtCrosshair(1000.0, 1.5,NULL));
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
	
	void EnableToggles( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
	   	Param1<bool> data;
        if ( !ctx.Read( data ) ) return;

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
        if ( !ctx.Read( data ) || !m_Toggles) return;
		
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
        if ( !ctx.Read( data ) || !m_Toggles) return;
		
		if (type == CallType.Client)
        {
			if (data.param1)
			{
				if (m_FreeCamStatus)
				{
					m_FreeCamStatus = false;
					m_freecam.SetActive(false);
					GetGame().GetUIManager().CloseAll();
					GetGame().ObjectDelete(m_freecam);
					GetGame().SelectPlayer(null,GetGame().GetPlayer());
				}else{
					m_FreeCamStatus = true;
					vector pos;
					MiscGameplayFunctions.GetHeadBonePos(PlayerBase.Cast(GetGame().GetPlayer()), pos);
					m_freecam = VPPFreeCam.Cast(g_Game.CreateObject( "VPPFreeCam", pos, true ));
					m_freecam.SetActive(true);
				}
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
};

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