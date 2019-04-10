modded class MissionGameplay
{
	private ref array<ref LocalPlayer> m_LocalPlayers;
	private ref VPPAdminHud       m_AdminHud;
	private VPPKeybindsMenu       m_VPPKeybindsMenu;
	private bool 				  m_AdminMenuShowing;
	private bool				  m_KeyBindsMenuShowing;
	private bool 				  m_Toggles = false;

	void MissionGameplay()
	{
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "InitESPBox", this, SingeplayerExecutionType.Server );
		GetRPCManager().SendRPC( "RPC_AdminTools", "EnableToggles", NULL, true );

		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.UpdateESP, 6000, true);
		
		//Input action test (ESP)
		UAInput input = GetUApi().RegisterInput( "UAUISelectPlayer", "Select Player (ESP)", "core" );
		input.AddAlternative();
		input.BindCombo( "kF" );
	}
	
	void ~MissionGameplay()
	{
		if (g_Game.CheckESP())
		{
			g_Game.setESPStatus(false);
			g_Game.SetESPType(true);
		}
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate( timeslice );
		
		Input input = GetGame().GetInput();
		if (input.LocalPress("UAUIBack", false) && (m_AdminMenuShowing || m_KeyBindsMenuShowing))
		{
			ref VPPAdminHud AdminHud = VPPAdminHud.Cast( GetUIManager().FindMenu(VPP_ADMIN_HUD) );
			if (AdminHud)
			{
				GetUIManager().HideScriptedMenu(AdminHud);
				PlayerControlEnable();
				g_Game.GetUIManager().ShowUICursor(false);
				m_AdminMenuShowing = false;
				GetGame().GetMission().GetHud().Show( true );
			}

			VPPKeybindsMenu KeyBindsMenu = VPPKeybindsMenu.Cast( GetUIManager().FindMenu(VPP_KEYBINDS_MENU) );
			if (KeyBindsMenu)
			{
				GetUIManager().HideScriptedMenu(KeyBindsMenu);
				PlayerControlEnable();
				m_KeyBindsMenuShowing = false;
			}
		}
	}
	
	void EnableToggles( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<bool> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	if (data.param1) m_Toggles = true;
        }
    }

    void UpdateESP()
	{
		if (!g_Game.CheckESP() || GetGame().GetPlayer() == NULL) return;
		m_LocalPlayers = new array<ref LocalPlayer>;
		ref array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition3D(GetGame().GetPlayer().GetPosition(), 1100, objects, NULL);
		for (int i = 0; i < objects.Count(); ++i)
		{
			Object obj;
			PlayerBase playerFound;
			obj = Object.Cast( objects.Get(i) );

			if (GetGame().ObjectIsKindOf(obj,"SurvivorBase"))
			{
				if (Class.CastTo(playerFound,obj))
				{
					if (obj != GetGame().GetPlayer())
					{
						int PID = Math.RandomIntInclusive(0,99999999999);
						ref LocalPlayer m_PlayerCahce = new LocalPlayer(obj,PID);
						m_LocalPlayers.Insert(m_PlayerCahce);
						
						ref Param2<ref PlayerBase, int> param = new Param2<ref PlayerBase, int>(playerFound,PID);
       					GetRPCManager().SendRPC( "RPC_AdminTools", "GetPlayerInfo", param, true);
					}
				}
			}
		}
	}

	void InitESPBox( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param6<string,string,string,float,float,int> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
			if (m_LocalPlayers != NULL)
			{
				for (int i = 0; i < m_LocalPlayers.Count(); ++i)
				{
					ref LocalPlayer m_Player = m_LocalPlayers.Get(i);
					if (m_Player.GetUniqueID() == data.param6)
					{
						ref ESPBox DisplayBox = new ESPBox(m_Player.GetPlayerVehicle(),data.param1,data.param2,data.param3,data.param4,data.param5);
					}
				}
			}
        }
	}

	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		m_Hud.KeyPress(key);
		UIScriptedMenu menu;

		if (m_VPPKeybindsMenu != NULL)
		{
			m_VPPKeybindsMenu.KeyPressReg(key);
		}

		if ( key == KeyCode.KC_INSERT)
		{
			VPPKeybindsMenu KeyBindsMenu = VPPKeybindsMenu.Cast( GetUIManager().FindMenu(VPP_KEYBINDS_MENU) );
			if (KeyBindsMenu && KeyBindsMenu.getStatus())
			{
				KeyBindsMenu.setStatus(false);
				GetUIManager().HideScriptedMenu(KeyBindsMenu);
				PlayerControlEnable();
				GetGame().GetMission().GetHud().Show( true );
				g_Game.GetUIManager().ShowUICursor(false);
			}
			else
			{
				bool oInit = false;
				menu = GetUIManager().GetMenu();
				if (menu == NULL)
				{
					if (m_VPPKeybindsMenu == NULL)
					{
						m_VPPKeybindsMenu = VPPKeybindsMenu.Cast( GetUIManager().EnterScriptedMenu(VPP_KEYBINDS_MENU, NULL) );
					}
					else if ( GetUIManager().FindMenu(VPP_KEYBINDS_MENU) == NULL )
					{
						GetUIManager().ShowScriptedMenu(m_VPPKeybindsMenu, NULL);
					}
					oInit = true;
					m_VPPKeybindsMenu.setStatus(true);
				}
				
				if (menu && menu == m_VPPKeybindsMenu)
				{
					oInit = true;
					m_VPPKeybindsMenu.setStatus(true);
				}
				
				if (m_VPPKeybindsMenu && oInit)
				{
					PlayerControlDisable(INPUT_EXCLUDE_INVENTORY);
					GetGame().GetMission().GetHud().Show( false );
					g_Game.GetUIManager().ShowUICursor(true);
				}
			}
		}
		
		if (m_Toggles)
		{
			if ( key == g_Game.getKeyBindsConfig().getActionKeyEnum("CrosshairTP") )
			{
				float oDistance = vector.Distance(GetGame().GetPlayer().GetPosition(),g_Game.GetCursorPos());
				if (oDistance >= 1100) return;
				ref Param3<vector,ref array<string>,bool> m_Data = new Param3<vector,ref array<string>,bool>(g_Game.GetCursorPos(),NULL,true);
                GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", m_Data, true);
			}

			if ( key == g_Game.getKeyBindsConfig().getActionKeyEnum("CrosshairDelete") )
			{
				GetRPCManager().SendRPC( "RPC_AdminTools", "DeleteObject", m_Data, true, NULL, g_Game.getObjectAtCrosshair(1000.0, 1.5,NULL));
			}

			if ( key == g_Game.getKeyBindsConfig().getActionKeyEnum("GodModeToggle") )
			{
				GetRPCManager().SendRPC( "RPC_AdminTools", "ToggleGodmode", NULL, true);
			}
		}

		if ( key == g_Game.getKeyBindsConfig().getActionKeyEnum("AdminTools") )
		{
			if (!m_Toggles) return;

			if (m_AdminMenuShowing)
			{
				ref VPPAdminHud AdminHud = VPPAdminHud.Cast( GetUIManager().FindMenu(VPP_ADMIN_HUD) );
				if (AdminHud)
				{
					GetUIManager().HideScriptedMenu(AdminHud);
					PlayerControlEnable();
					g_Game.GetUIManager().ShowUICursor(false);
					m_AdminMenuShowing = false;
					GetGame().GetMission().GetHud().Show( true );
				}
			}
			else
			{
				bool init = false;
				menu = GetUIManager().GetMenu();
				if (menu == NULL && !m_AdminMenuShowing)
				{
					if (m_AdminHud == NULL)
					{
						m_AdminHud = VPPAdminHud.Cast( GetUIManager().EnterScriptedMenu(VPP_ADMIN_HUD, NULL) );
					}
					else if ( GetUIManager().FindMenu(VPP_ADMIN_HUD) == NULL )
					{
						GetUIManager().ShowScriptedMenu(m_AdminHud, NULL);
					}
					init = true;
					m_AdminMenuShowing = true;
				}
				
				if (menu && menu == m_AdminHud)
				{
					init = true;
					m_AdminMenuShowing = true;
				}
				
				if (m_AdminHud && init)
				{
					PlayerControlDisable(INPUT_EXCLUDE_INVENTORY);
					GetGame().GetMission().GetHud().Show( false );
					m_AdminHud.HideAdminHud(false);
				}
			}
		}
	}
};