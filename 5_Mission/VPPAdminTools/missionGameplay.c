typedef Param2<string, string> ESPBonesParams; //from, to
modded class MissionGameplay
{
	static ref ScriptInvoker m_OnMissionPaused = new ScriptInvoker();
	static ref ScriptInvoker m_OnMissionUnpaused = new ScriptInvoker();

	//Client vars
    private bool   m_Toggles;
    private string m_VAdminPassword;

    private bool   m_ToolsToggled;
    private string m_systemMessage;
    private bool   m_isLoggedIn;
    private bool   m_MeshEspToggled;
    
    Object targetObj;

    ref array<ref ESPBonesParams> m_EspBones;
    Widget m_EspCanvas;
    CanvasWidget m_EspCanvasWidget;

	void MissionGameplay()
	{
		MissionGameplay.m_OnMissionPaused.Insert(OnMissionPaused);
		MissionGameplay.m_OnMissionUnpaused.Insert(OnMissionUnpaused);
	}
	
	void ~MissionGameplay()
	{
		MissionGameplay.m_OnMissionPaused.Clear();
		MissionGameplay.m_OnMissionUnpaused.Clear();
	}

	override void OnInit()
    {
        super.OnInit();
        
        Print("[MissionGameplay] OnInit - Client");
        //ESP Bones
        m_EspBones = new array<ref ESPBonesParams>;
        m_EspBones.Insert(new ESPBonesParams("neck",         "spine3"));
        m_EspBones.Insert(new ESPBonesParams("spine3",       "pelvis"));
        m_EspBones.Insert(new ESPBonesParams("neck",         "leftarm"));
        m_EspBones.Insert(new ESPBonesParams("leftarm",      "leftforearm"));
        m_EspBones.Insert(new ESPBonesParams("leftforearm",  "lefthand"));
        m_EspBones.Insert(new ESPBonesParams("lefthand",     "lefthandmiddle4"));
        m_EspBones.Insert(new ESPBonesParams("pelvis",       "leftupleg"));
        m_EspBones.Insert(new ESPBonesParams("leftupleg",    "leftleg"));
        m_EspBones.Insert(new ESPBonesParams("leftleg",      "leftfoot"));
        m_EspBones.Insert(new ESPBonesParams("leftfoot",     "lefttoebase"));
        m_EspBones.Insert(new ESPBonesParams("neck",         "rightarm"));
        m_EspBones.Insert(new ESPBonesParams("rightarm",     "rightforearm"));
        m_EspBones.Insert(new ESPBonesParams("rightforearm", "righthand"));
        m_EspBones.Insert(new ESPBonesParams("righthand",    "righthandmiddle4"));
        m_EspBones.Insert(new ESPBonesParams("pelvis",       "rightupleg"));
        m_EspBones.Insert(new ESPBonesParams("rightupleg",   "rightleg"));
        m_EspBones.Insert(new ESPBonesParams("rightleg",     "rightfoot"));
        m_EspBones.Insert(new ESPBonesParams("rightfoot",    "righttoebase"));

        GetRPCManager().AddRPC("RPC_MissionGameplay", "ServerLoginError", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC("RPC_MissionGameplay", "LoginAttemptFail", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC("RPC_MissionGameplay", "EnableToggles", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC("RPC_HandleFreeCam", "HandleFreeCam", this, SingeplayerExecutionType.Server);
        GetRPCManager().AddRPC("RPC_HandleMeshEspToggle", "HandleMeshEspToggle", this, SingeplayerExecutionType.Server);

        VPPKeybindsManager.RegisterBind("UAUIBack", VPPBinds.Press, "EscapeButtonPressed", this);
        VPPKeybindsManager.RegisterBind("UAToggleAdminTools", VPPBinds.Press, "ToggleAdminTools", this);
        VPPKeybindsManager.RegisterBind("UAOpenAdminTools", VPPBinds.Press, "OpenAdminTools", this);
        VPPKeybindsManager.RegisterBind("UAToggleCmdConsole", VPPBinds.Press, "ToggleCmdConsole", this);
        VPPKeybindsManager.RegisterBind("UATeleportToCrosshair", VPPBinds.Press|VPPBinds.Hold, "TeleportToCrosshair", this);
        VPPKeybindsManager.RegisterBind("UADeleteObjCrosshair", VPPBinds.Press, "DeleteObjCrosshair", this);
        VPPKeybindsManager.RegisterBind("UAToggleGodMode", VPPBinds.Press, "ToggleGodMode", this);
        VPPKeybindsManager.RegisterBind("UAToggleFreeCam", VPPBinds.Press, "ToggleFreeCam", this);
        VPPKeybindsManager.RegisterBind("UASupriseButtSex", VPPBinds.Press, "SupriseButtSex", this);
        VPPKeybindsManager.RegisterBind("UACopyPositionClipboard", VPPBinds.Press, "CopyPositionClipboard", this);
        VPPKeybindsManager.RegisterBind("UARepairVehicleAtCrosshairs", VPPBinds.Press, "RepairVehicleAtCrosshairs", this);
        VPPKeybindsManager.RegisterBind("UAExitSpectate", VPPBinds.Press, "ExitSpectate", this);
        VPPKeybindsManager.RegisterBind("UACollapseESPDropDwn", VPPBinds.DoubleClick, "PlayerEspDropdowns", this);
        VPPKeybindsManager.RegisterBind("UAToggleMeshEsp", VPPBinds.Press, "ToggleMeshESP", this);
        VPPKeybindsManager.RegisterBind("UAToggleInvis", VPPBinds.Press, "TogglePlayerInvis", this);
        
        m_EspCanvas = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/PlayerESPCanvas.layout");
        m_EspCanvasWidget = CanvasWidget.Cast(m_EspCanvas.FindAnyWidget("CanvasWidget"));
    }

    override void OnMissionStart()
    {
        super.OnMissionStart();
        Print("[MissionGameplay] OnMissionStart - Client");
    }

    override void OnMissionFinish()
    {
        super.OnMissionFinish();
        Print("[MissionGameplay] OnMissionFinish - Client");
    }

    vector TransformToScreenPos(vector pWorldPos)
    {
        float parent_width, parent_height;
        vector transformed_pos, screen_pos;
        
        screen_pos = GetGame().GetScreenPosRelative( pWorldPos );
        m_EspCanvas.GetParent().GetScreenSize(parent_width, parent_height);
        
        transformed_pos[0] = screen_pos[0] * parent_width;
        transformed_pos[1] = screen_pos[1] * parent_height;
        return transformed_pos;
    }
	
	void OnMissionPaused()
	{
		GetUApi().ActivateExclude("VPPCamControls");
		GetUApi().UpdateControls();
	}
	
	void OnMissionUnpaused()
	{
		GetUApi().ActivateGroup("VPPCamControls");
		GetUApi().UpdateControls();
	}
	
	override void Pause()
	{
		MissionGameplay.m_OnMissionPaused.Invoke();
		super.Pause();
	}
	
	override void Continue()
	{
		MissionGameplay.m_OnMissionUnpaused.Invoke();
		super.Continue();
	}

	override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        UpdatePlayerMeshEsp();
    }

    protected void UpdatePlayerMeshEsp()
    {
        if (!m_MeshEspToggled)
            return;

        PlayerBase admin = PlayerBase.Cast(GetGame().GetPlayer());
        if (!admin || !m_EspCanvas)
            return;

        int color = ARGBF(1, 1, 0, 0);
        m_EspCanvasWidget.Clear();
        if (GetVPPUIManager().GetMenuByType(VPPAdminHud))
        {
            EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
            if (!espMenu)
            VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).CreateSubMenu(EspToolsMenu);

            espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
            VPPFilterEntry pFilter = espMenu.GetFilter("SurvivorBase");
            if (pFilter)
                color = pFilter.GetFilterColor();
        }

        array<Man> players = new array<Man>;
        players = ClientData.m_PlayerBaseList;
        foreach(Man man : players)
        {
            if (!man)
                continue;
            if(man == GetGame().GetPlayer())
                continue;

            PlayerBase playerPB = PlayerBase.Cast(man);
            if (!playerPB)
                continue;

            //don't render when out of sight
            vector ScreenPosRelative = GetGame().GetScreenPosRelative(playerPB.GetPosition());
            if(ScreenPosRelative[0] >= 1 || ScreenPosRelative[0] == 0 || ScreenPosRelative[1] >= 1 || ScreenPosRelative[1] == 0)
                continue;
            else if(ScreenPosRelative[2] < 0)
                continue;

            foreach(ESPBonesParams params : m_EspBones)
            {
                int fromIdx = playerPB.GetBoneIndexByName(params.param1);
                int toIdx   = playerPB.GetBoneIndexByName(params.param2);

                vector fromPos = TransformToScreenPos(playerPB.GetBonePositionWS(fromIdx));
                vector toPos   = TransformToScreenPos(playerPB.GetBonePositionWS(toIdx));
                m_EspCanvasWidget.DrawLine(toPos[0], toPos[1], fromPos[0], fromPos[1], 2, color);
            }

            //Draw Face
            int headIdx = playerPB.GetBoneIndexByName("head");
            int neckIdx = playerPB.GetBoneIndexByName("neck");
            vector fpos = TransformToScreenPos(playerPB.GetBonePositionWS(headIdx));
            vector npos = TransformToScreenPos(playerPB.GetBonePositionWS(neckIdx));

            float size = 1 * fpos[1] - npos[1];
            for(int j = 0; j < 360; j++)
            {
                float x1 = fpos[0] + (size * Math.Cos(j * Math.DEG2RAD));
                float y1 = fpos[1] + (size * Math.Sin(j * Math.DEG2RAD));

                float x2 = fpos[0] + (size * Math.Cos((j + 1) * Math.DEG2RAD));
                float y2 = fpos[1] + (size * Math.Sin((j + 1) * Math.DEG2RAD));

                m_EspCanvasWidget.DrawLine(x1, y1, x2, y2, 2, color);
            }
        }
    }

    override void OnEvent(EventType eventTypeId, Param params)
    {
        /*
        * Client part
        */
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chat_params = ChatMessageEventParams.Cast(params);   
            chat_params.param2.ToLower();       
            if (chat_params.param2 == "system")
            {
                m_systemMessage = chat_params.param3;
                
                if (m_systemMessage == "Logged in as admin")
                {
                    m_isLoggedIn = true;
                }
                else if (m_systemMessage == "Logged out")
                {
                    m_isLoggedIn = false;
                }
            }
            super.OnEvent(eventTypeId, params);
            return;
        }
        super.OnEvent(eventTypeId, params);
    }

    bool IsLoggedInAsAdmin()
    {
        return m_isLoggedIn;
    }
    
    string GetSystemMessage()
    {
        return m_systemMessage;
    }

    override void OnKeyPress(int key) 
    {
        //Avoid Escape key exit done by other mods (stops a client-crash)
        VPPScriptedMenu menu = VPPScriptedMenu.Cast(GetGame().GetUIManager().GetMenu());
        if (menu && key == KeyCode.KC_ESCAPE)
        {
            return;
        }
        super.OnKeyPress(key);
    }

    /*
    **
    *  ///////////Client RPCs Section/////////////////
       ///////////////////////////////////////////////
    */
    void EscapeButtonPressed()
    {
        VPPAdminHud AdminTab = VPPAdminHud.Cast(GetGame().GetUIManager().FindMenu(VPP_ADMIN_HUD));
        if (AdminTab != NULL && AdminTab.IsShowing())
        {
            AdminTab.HideMenu();
        }
    }

    void ToggleAdminTools()
    {
        if (!m_Toggles)
        {
            if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
            {
                GetVPPUIManager().SetKeybindsStatus(true);
                GetGame().GetInput().ChangeGameFocus(1);
                GetGame().GetUIManager().ShowUICursor(true);
                PlayerControlDisable(INPUT_EXCLUDE_ALL);

                VPPDialogBox dialogBox = GetVPPUIManager().CreateDialogBox(NULL, true);
                dialogBox.InitDiagBox(DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Admin Login", "Please enter password to continue using the tool.\n*Input is hidden*", this, "OnDiagResultToggleTools");
                dialogBox.AllowCharInput();
                dialogBox.HideInputCharacters(true);
            }
            return;
        }

        m_ToolsToggled = !m_ToolsToggled;
        if ( m_ToolsToggled )
            GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_TOOLS_TOGGLE_ON", "V++ Admin Tools:", 3.0);
        else
            GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_TOOLS_TOGGLE_OFF", "V++ Admin Tools:", 3.0);
    }

    void OnDiagResultToggleTools(int result, string input)
    {
        GetVPPUIManager().SetKeybindsStatus(false);
        GetGame().GetUIManager().ShowUICursor(false);
        GetGame().GetInput().ResetGameFocus();
        GetGame().GetInput().ChangeGameFocus(-1);
        PlayerControlEnable(false);

        if (result == DIAGRESULT.OK)
        {
            if (input == string.Empty)
            {
                GetVPPUIManager().DisplayNotification("You need to enter a valid password.", "[VPPAT] LOGIN ERROR:", 5.0);
                return;
            }

            if (g_Game.GetFailedLoginAttempts() >= 6)
            {
                GetVPPUIManager().DisplayNotification("Too many failed login attempts, server will not accept attempts until next restart. Game Restart is also required!", "[VPPAT] LOGIN ERROR:", 8.0);
                return;
            }
            m_VAdminPassword = input;
            GetRPCManager().VSendRPC("RPC_PermitManager", "AdminLogin", NULL, true);
        }
    }

    void LoginAttemptFail(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            GetVPPUIManager().DisplayNotification(string.Format("Login information was incorrect. Attempts: (%1/6)", g_Game.GetFailedLoginAttempts()), "[VPPAT] LOGIN ERROR:", 8.0);
            g_Game.IncrementFailedLoginAttempts();
        }
    }

    void ServerLoginError(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            GetVPPUIManager().DisplayNotification(string.Format("The server has no set password! Please read the instructions found within [profile/VPPAdminTools/Permissions/credentials.txt] on how to setup a login password. (profile is the directory where your mods configurations are generated & stored including server logs)"), "[VPPAT] LOGIN ERROR:", 30.0);
        }
    }

    void EnableToggles( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
    {
        Param1<bool> data;
        if (!ctx.Read( data ) || sender != null)
            return;

        if (type == CallType.Client)
        {
            m_Toggles = data.param1;
            
            if (!m_Toggles)
            {
                GetVPPUIManager().DisplayNotification("failed to grant permissions. You are no longer part of a user group.", "V++ Admin Tools:", 8.0);

                VPPAdminHud adminMenu;
                if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
                {
                    if (adminMenu.IsShowing())
                    {
                        adminMenu.HideMenu();
                        GetVPPUIManager().DestroyMenuInstanceByType(VPPAdminHud);
                    }
                }
            }else{
                GetVPPUIManager().DisplayNotification("Successfully logged in as admin.", "V++ Admin Tools:", 5.0);
            }
        }
    }

    void OpenAdminTools()
    {
        if (!m_Toggles)
            return;

        if (!GetVPPUIManager().IsTyping())
        {
            if (m_ToolsToggled){
                OpenToolBar();
            }else{
                GetGame().Chat( "#VSTR_NOTIFY_ERR_NOTTOGGLE", "colorAction" );
            }
        }
    }

    void ToggleCmdConsole()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            if (OpenToolBar())
            {
                VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
                if (rootMenu.GetSubMenuByType(MenuCommandsConsole) == null)
                    rootMenu.CreateSubMenu(MenuCommandsConsole);
                else
                    MenuCommandsConsole.Cast(rootMenu.GetSubMenuByType(MenuCommandsConsole)).ShowSubMenu();
            }
        }
    }
    
    void TeleportToCrosshair()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            GetRPCManager().VSendRPC( "RPC_AdminTools", "TeleportToPosition", new Param1<vector>(g_Game.GetCursorPos()), true);
        }
    }

    void DeleteObjCrosshair()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            targetObj = g_Game.getObjectAtCrosshair(1000.0, 0.0,NULL);
            if (targetObj != null)
            {
                //Lock Controls and show mouse cursor 
                GetVPPUIManager().SetKeybindsStatus(true); //Lock shortcut keys
                GetGame().GetInput().ChangeGameFocus(1);
                GetGame().GetUIManager().ShowUICursor( true );
                PlayerControlDisable(INPUT_EXCLUDE_ALL);
                
                //Show confirmation of delete
                VPPDialogBox dialogBox = GetVPPUIManager().CreateDialogBox(null,true);
                dialogBox.InitDiagBox(DIAGTYPE.DIAG_YESNO, "#VSTR_NOTIFY_DEL_OBJ", "#VSTR_NOTIFY_Q_DEL" + "["+targetObj.GetType()+"]?", this);
            }
        }
    }

    void ToggleGodMode()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            GetRPCManager().VSendRPC( "RPC_PlayerManager", "ToggleGodmode", NULL, true);
        }
    }

    void ToggleFreeCam()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            GetRPCManager().VSendRPC( "RPC_AdminTools", "ToggleFreeCam", NULL, true);
        }
    }

    void SupriseButtSex()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            array<string> itemTypes = {"barrel_green","Truck_01_WheelDouble","TransitBusWheel","TransitBusWheelDouble","Refridgerator","SeaChest","PowerGenerator","WoodenLog"};
            GetRPCManager().VSendRPC( "RPC_MissionServer", "HandleChatCommand", new Param1<string>("/sph "+itemTypes.GetRandomElement()), true);
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ThrowInHands, 1000, false);
        }
    }

    void CopyPositionClipboard()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            Object target = g_Game.getObjectAtCrosshair(1000.0, 0.0,NULL);
            string targetType = target.GetType();
            string notificationMsg;
            if ( target == NULL )
            {
                notificationMsg = "#VSTR_NOTIFY_COPY_CLIPBOARD" + "\n\nPosition: " + PosToString(GetGame().GetPlayer().GetPosition());
                GetGame().CopyToClipboard(GetGame().GetPlayer().GetPosition().ToString());
            }
            else
            {
                if ( targetType == "" || targetType == string.Empty)
                    notificationMsg = string.Format("#VSTR_NOTIFY_COPY_POS" + "\n\nPosition:%1", PosToString(target.GetPosition()));
                else
                    notificationMsg = string.Format("Copied position of object: %1 to clipboard\n\nPosition: %2", target.GetType(), PosToString(target.GetPosition()));
                
                GetGame().CopyToClipboard(target.GetPosition().ToString());
            }
            GetVPPUIManager().DisplayNotification(notificationMsg, "V++ Admin Tools:", 10.0);
        }
    }

    void RepairVehicleAtCrosshairs()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            Car targetVehicle = Car.Cast(g_Game.getObjectAtCrosshair(1000.0, 0.0,NULL));
            if (targetVehicle)
            {
                GetRPCManager().VSendRPC("RPC_AdminTools", "RepairVehicles", NULL, true, NULL, targetVehicle);
            }
        }
    }

    void ExitSpectate()
    {
        if (g_Game.IsSpectateMode())
        {
            g_Game.ReconnectToCurrentSession();
            g_Game.SetSpectateMode(false);
        }
    }
    
    void PlayerEspDropdowns()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;
        
        EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
        if (espMenu)
        {
            espMenu.ExpandPlayerTrackerDropDowns();
        }
    }

    void ToggleMeshESP()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
            GetRPCManager().VSendRPC( "RPC_VPPESPTools", "ToggleMeshESP", null, true, null);
    }

    void TogglePlayerInvis()
    {
        if ((!m_Toggles) || (!m_ToolsToggled))
            return;

        if (!GetVPPUIManager().GetKeybindsStatus() && !GetVPPUIManager().IsTyping())
        {
            if (GetGame().GetPlayer() && GetGame().GetPlayer().GetIdentity())
            {
                GetRPCManager().VSendRPC("RPC_PlayerManager", "RequestInvisibility", new Param1<ref array<int>>({GetGame().GetPlayer().GetIdentity().GetPlayerId()}), true);
            }
        }
    }

    //Returns true when opens
    bool OpenToolBar()
    {
        VPPAdminHud adminMenu;
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
    
    string PosToString(vector pos)
    {
        string PosArray[3] = { pos[0].ToString(), pos[1].ToString(), pos[2].ToString() };
        return string.Format("(%1, %2, %3)", PosArray[0], PosArray[1], PosArray[2]);
    }
    
    //Cross-hair delete callback
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
                GetRPCManager().VSendRPC( "RPC_AdminTools", "DeleteObject", NULL, true, NULL, targetObj);
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
                    hic.SetDisabled(false);
                }
                else
                {
                    if (!GetVPPUIManager().IsTyping())
                    {
                        CreateFreeCamInstance();
                        hic.SetDisabled(true);
                    }
                }
            }
        }
    }

    void HandleMeshEspToggle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (!m_Toggles || sender != null)
            return;
        
        if (type == CallType.Client)
        {
            m_MeshEspToggled = !m_MeshEspToggled;
            m_EspCanvasWidget.Clear();

            if (m_MeshEspToggled)
                GetVPPUIManager().DisplayNotification("Player ESP Skeleton toggled ON!", "V++ Admin Tools:", 3.0);
            else
                GetVPPUIManager().DisplayNotification("Player ESP Skeleton toggled OFF!", "V++ Admin Tools:", 3.0);
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
            HumanCommandWeapons hcw = hmn.GetCommandModifier_Weapons();
            
            float lr = hcw.GetBaseAimingAngleLR();
            float ud = hcw.GetBaseAimingAngleUD();
            vector aimOrientation = player.GetOrientation();
            aimOrientation[0] = aimOrientation[0] + lr;
            aimOrientation[1] = aimOrientation[1] + ud;
            
            player.GetHumanInventory().ThrowEntity(item, aimOrientation.AnglesToVector(),10000);
        }
    }

    void LaunchItemInHands()
    {
        if (!m_Toggles)
            return;

        float launchSpeed = 45.0;
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        EntityAI item = GetGame().GetPlayer().GetHumanInventory().GetEntityInHands();

        if (!item)
            return;

        player.PredictiveDropEntity(item);
        vector dir = player.GetDirection();
        item.SetDirection(dir);
        float surfaceY = GetGame().SurfaceY(player.GetPosition()[0], player.GetPosition()[2]);
        vector pointPos = Vector(player.GetPosition()[0], surfaceY + 1.8, player.GetPosition()[2]);

        float angle = player.GetOrientation()[0];
        float rads = angle * Math.DEG2RAD;      

        float x = pointPos[0] + Math.Sin(rads) * 1.0;
        float z = pointPos[2] + Math.Cos(rads) * 1.0;

        item.SetPosition(Vector(x,pointPos[1],z));
        item.CreateDynamicPhysics(PhxInteractionLayers.DYNAMICITEM|PhxInteractionLayers.CHARACTER|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ROADWAY|PhxInteractionLayers.TERRAIN|PhxInteractionLayers.ITEM_SMALL|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE|PhxInteractionLayers.AI);
        item.EnableDynamicCCD(true);
        item.SetDynamicPhysicsLifeTime(10.0);

        SetVelocity(item, dir * launchSpeed);
        dBodyApplyForce(item, dir * launchSpeed * dBodyGetMass(item));
        /*
        vector impulse = vector.Up * 40;
        impulse[1] = 40 * 1.5;
        dBodyActive(item, ActiveState.ACTIVE);
        dBodyApplyImpulse(item, impulse);
        */
    }
    /*
    ****
    */
};