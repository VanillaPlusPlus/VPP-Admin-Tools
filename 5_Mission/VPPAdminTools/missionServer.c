modded class MissionServer
{
	//Server vars
    private float m_LoginTimeMs   = 15000; //15 sec default
    private bool  m_ServerLocked  = false;
    private int   m_MaxKickAttempts = 5;
    private ref map<string,int> m_KickQueue;
    private ref map<PlayerBase,bool> m_LogoutQueueInital;

    override void OnMissionStart()
    {
        super.OnMissionStart();
        Print("[MissionServer] OnMissionStart - Server");
        if (GetGame().ServerConfigGetInt("vppDisablePassword") > 0)
        {
            g_Game.DisablePasswordProtection(true);
            Print("[VPPAdminTools] Password Protection Is Disabled!");
        }
        new VPPATInventorySlots;

        string serverName = GetServerName();
        if (serverName == "[SERVER NAME NOT FOUND]")
        {
            if (GetSteamAPIManager().m_ServerDataCache)
                serverName = GetSteamAPIManager().m_ServerDataCache.name;
        }
        g_Game.SetServerName(serverName);
    }

    override void OnMissionFinish()
    {
        super.OnMissionFinish();
        Print("[MissionServer] OnMissionFinish - Server");
    }

    override void OnInit()
    {
        super.OnInit();
        Print("[MissionServer] OnInit - Server");

        m_KickQueue = new map<string,int>;
        m_LogoutQueueInital = new map<PlayerBase,bool>;
        //Event handlers
        VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect").Insert(HandleOnPlayerConnect);
        VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected").Insert(HandleOnPlayerDisconnected);

        //=============RPC's====================
        GetRPCManager().AddRPC( "RPC_MissionServer", "RequestLockServer", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_MissionServer", "HandleChatCommand", this, SingeplayerExecutionType.Server );      
        //======================================
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.FindLoginTime, 1500.0, true);
    }

    /*
    * Order of Events firing:
    * When player respawns                  : ClientPrepareEventTypeID ► ClientNewEventTypeID ► ClientNewReadyEventTypeID
    * When player joins (no saved character): ClientPrepareEventTypeID ► ClientNewEventTypeID ► ClientNewReadyEventTypeID
    * When player joins                     : ClientPrepareEventTypeID ► ClientReadyEventTypeID
    * When player logs out                  : ClientDisconnectedEventTypeID
    * When player cancels logout            : LogoutCancelEventTypeID
    */
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);

        PlayerIdentity identity;
        PlayerBase     player;
        ScriptInvoker onPlayerConnect;
        bool announceLogin;

        switch(eventTypeId)
        {
            //Called when player joins/respawns (PREP STAGE)
            case ClientPrepareEventTypeID:
            {
                Print("ClientPrepareEventTypeID");
                ClientPrepareEventParams clientPrepareParams;
                Class.CastTo(clientPrepareParams, params);

                identity = clientPrepareParams.param1;

                if (GetPermissionManager().HasUserGroup(identity.GetPlainId()))
                {
                    PlayerListManager.AddReceiver(identity);
                    PlayerListManager.AddEntry(identity.GetId(), identity);
                    PlayerListManager.SyncListToClient(identity);
                }
                else
                {
                    PlayerListManager.AddEntry(identity.GetId(), identity);
                }

                BannedPlayer bannedPlayer = GetBansManager().GetBannedPlayer(identity.GetPlainId());
                if (bannedPlayer != NULL)
                {
                    Print("Banned player");
                    BanDuration expireDate = bannedPlayer.expirationDate;
                    string banReason = bannedPlayer.banReason;
                    if (expireDate.Permanent)
                    {
                        banReason += "\n Expiration Date: Permanent";
                    }else{
                        banReason += string.Format("\n Expiration Date %1/%2/%3  %4%5%6",expireDate.Year.ToString(),expireDate.Month.ToString(),expireDate.Day.ToString(),expireDate.Hour.ToString(),":",expireDate.Minute.ToString());
                    }

                    if (!m_KickQueue.Contains(identity.GetPlainId()))
                    {
                        Print("Add to ban queue");
                        m_KickQueue.Insert(identity.GetPlainId(), 0);
                        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, 1/*m_LoginTimeMs*/, true, identity.GetPlainId(), banReason); 
                    }
                }
                
                if (m_ServerLocked)
                    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, m_LoginTimeMs, true, identity.GetPlainId(),"#VSTR_NOTIFY_SERVER_LOCKED");

                break;
            }

            //Called when a saved player joins (READY STAGE)
            case ClientReadyEventTypeID:
            {
                ClientReadyEventParams readyParams;
                Class.CastTo(readyParams, params);
                
                identity = readyParams.param1;
                player   = PlayerBase.Cast(readyParams.param2);

                PlayerListManager.SyncEntryToReceivers(identity.GetId()); //sync player list
                Print("ClientReadyEventTypeID::SyncEntryToReceivers");

                onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
                if(onPlayerConnect)
                {
                    announceLogin = !GetPlayerListManager().HasPlayerInList(identity.GetId());
                    onPlayerConnect.Invoke(player, identity, false, announceLogin);
                }

                break;
            }

            //Called when player joins(new character)/respawns (2nd PREP STAGE)
            case ClientNewEventTypeID:
            {
                ClientNewEventParams newParams;
                Class.CastTo(newParams, params);
                break;
            }

            //Called when player joins(new character)/respawns (READY STAGE)
            case ClientNewReadyEventTypeID:
            {
                ClientNewReadyEventParams newReadyParams; //PlayerIdentity, Man
                Class.CastTo(newReadyParams, params);

                identity = newReadyParams.param1;
                player   = PlayerBase.Cast(newReadyParams.param2);

                PlayerListManager.SyncEntryToReceivers(identity.GetId()); //sync player list
                Print("ClientNewReadyEventTypeID::SyncEntryToReceivers");

                onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
                if(onPlayerConnect)
                {
                    announceLogin = !GetPlayerListManager().HasPlayerInList(identity.GetId());
                    onPlayerConnect.Invoke(player, identity, false, announceLogin);
                }

                break;
            }

            //????
            case ClientReconnectEventTypeID:
            {
                ClientReconnectEventParams reconnectParams;
                Class.CastTo(reconnectParams, params);
                
                identity = reconnectParams.param1;
                
                onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
                if(onPlayerConnect)
                    onPlayerConnect.Invoke(player, identity, false, true);

                break;
            }

            //Called when logout is canceled by player before the logout timer expired.
            case LogoutCancelEventTypeID:
            {
                LogoutCancelEventParams logoutCancelParams;
                Class.CastTo(logoutCancelParams, params);               
                Class.CastTo(player, logoutCancelParams.param1);

                identity = player.GetIdentity();

                onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
                if(onPlayerConnect)
                {
                    onPlayerConnect.Invoke(player, identity, true, false);
                }

                break;
            }

            //Called for disconnect process start.
            case ClientDisconnectedEventTypeID:
            {
                ClientDisconnectedEventParams discoParams;
                Class.CastTo(discoParams, params);      
                
                identity = discoParams.param1;
                player   = PlayerBase.Cast(discoParams.param2);

                if (!m_LogoutQueueInital.Get(player) || !m_LogoutQueueInital.Contains(player))
                {
                    m_LogoutQueueInital.Insert(player, true);

                    //We don't invoke this again if player already in "m_LogoutQueueInital"
                    ScriptInvoker onClientDisconnectedEvent = VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected");
                    onClientDisconnectedEvent.Invoke(player, identity, false);
                }
                else if (!IsLogoutTimeExpired(player))
                {
                    string steam64 = player.VPlayerGetSteamId();
                    GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) disconnected early from server (EXIT NOW).", player.VPlayerGetName(), steam64));
                    Print(string.Format("Player \"%1\" (steamid=%2) disconnected early from server (EXIT NOW).", player.VPlayerGetName(), steam64));
                    GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(player.VPlayerGetName(), steam64, "disconnected early from server (EXIT NOW)."));
                }
                break;
            }
        }
    }

    /*
    * This method is called by vanilla, very last method during the disconnect process.
    * NOTICE: PlayerIdentity can be null at this stage!!! (especially when player uses 'EXIT NOW')
    */
    override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
    {
        super.PlayerDisconnected(player, identity, uid);
        
        if (identity)
        {
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "left the server!"));
        }
        else if (player)
        {
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(player.VPlayerGetName(), player.VPlayerGetSteamId(), "left the server!"));
        }

        if (PlayerListManager.RemoveEntry(uid))
        {
            Print("PlayerDisconnected -> PlayerListManager.RemoveEntry ► " + uid);
        }

        ScriptInvoker onClientDisconnectedEvent = VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected");    
        onClientDisconnectedEvent.Invoke(player, identity, true); //at this point the logout process is complete, we invoke just for logging.
    }

    private void FindLoginTime()
    {
        if (!GetHive() || !GetCEApi())
        {
            m_LoginTimeMs = 3000.0;  //No hive, keep timer short.
            return;
        }

        //Post message of server status
        GetWebHooksManager().PostServerBootup();

        m_LoginTimeMs = (GetCEApi().GetCEGlobalInt("TimeLogin") * 1000) + 1200.0;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.FindLoginTime);
        Print("m_LoginTimeMs: " + m_LoginTimeMs);
    }
    
    string GetServerName()
    {
        string cfgPath;
        GetGame().CommandlineGetParam("config", cfgPath);
        string fullPathToConfig;

        if (cfgPath == string.Empty)
        {
            fullPathToConfig = "$CurrentDir:serverDZ.cfg"; //default
        }
        else if (cfgPath.Substring(1, 2) == ":\\")
        {
            array<string> output = {};
            cfgPath.Split("\\", output);
            if (output && output.Count() > 0)
            {
                fullPathToConfig = string.Format("$CurrentDir:%1\\%2", output[output.Count() - 2], output[output.Count() - 1]);
            }
        }
        else
        {
            fullPathToConfig = "$CurrentDir:" + cfgPath;
        }

        if (!FileExist(fullPathToConfig))
        {
            fullPathToConfig = "$CurrentDir:serverDZ.cfg"; //default
        }

        FileHandle serverCfg = OpenFile(fullPathToConfig, FileMode.READ);
        if (serverCfg != 0)
        {
            array<string> cfgData = new array<string>;
            string line_content = "";
            int char_count = FGets(serverCfg, line_content);
            cfgData.Insert(line_content);
            
            while ( char_count != -1 )
            {
                cfgData.Insert(line_content);
                char_count = FGets( serverCfg,  line_content );
            }
            CloseFile(serverCfg);
            
            //Parse and find server name
            foreach(string line: cfgData)
            {
                while(line.Contains(" "))
                {
                    line.Replace(" ","");
                    line.TrimInPlace();
                }
                
                if (line.Length() >= 10)
                {
                    string tmpLine = line.Substring(0, 10);
                    if (tmpLine == "hostname=\"")
                    {
                        int strIndex;
                        while(line[strIndex] != string.Empty)
                        {
                            if (line[strIndex] == "\"" && line[strIndex + 1] != string.Empty)
                            {
                                if (line[strIndex + 1] == ";")
                                {
                                    string dirtyName = line.Substring(0,strIndex);
                                    dirtyName.Replace("hostname=\"", "");
                                    Print(dirtyName);
                                    return dirtyName;
                                }
                            }
                            strIndex++;
                        }
                    }
                }
            }
        }
        return "[SERVER NAME NOT FOUND]";
    }

    void RequestLockServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if ( type == CallType.Server )
        {
            if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "ServerManager:LockServer")) return;
            
            if (m_ServerLocked)
            {
                m_ServerLocked = false;
                GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_SERVER_LOCK_OFF",NotifyTypes.NOTIFY);
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ServerManager] Unlocked the server"));
            }else{
                m_ServerLocked = true;
                GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_SERVER_LOCK_ON",NotifyTypes.NOTIFY);
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ServerManager] Locked the server"));
            }
        }
    }
    
    void HandleChatCommand(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        Param1<string> data; //chat string
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
            GetChatCommandManager().ParseCommand(data.param1,sender.GetPlainId());
        }
    }

    void InvokeKickPlayer(string id, string msg)
    {
        if ( m_KickQueue.Contains(id) )
        {
            int attempts = m_KickQueue.Get(id) + 1;
            m_KickQueue.Set(id, attempts);

            if ( attempts >= m_MaxKickAttempts ){
                Print("max reached attempts kicking banned player");
                m_KickQueue.Remove(id);
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
            }
        }
        
        PlayerIdentity identity = GetPermissionManager().GetIdentityById( id );
        if( identity != NULL )
        {
            Print("Send kick RPC");
            GetSimpleLogger().Log(string.Format("Kicking banned player \"%1\" (steamId=%2) kick message: (%3)", identity.GetName(), id, msg));
            GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( msg ), true, identity);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
            m_KickQueue.Remove(id);
        }
    }

    /*
    * Called multiple times during login stage by these events:
    *   ClientPrepareEventTypeID
    *   ClientNewReadyEventTypeID
    *   ClientReadyEventTypeID
    *   ClientNewEventTypeID
    *   ClientReconnectEventTypeID
    *   LogoutCancelEventTypeID
    */
    void HandleOnPlayerConnect(PlayerBase player, PlayerIdentity identity, bool canceledLogout, bool logAsLogin)
    {
        if(identity == NULL)
        {
            GetSimpleLogger().Log(string.Format("A Player attempted to connect but with a NULL PlayerIdentity"));
            Print(string.Format("[VPPAT] A Player attempted to connect but with a NULL PlayerIdentity"));
            return;
        }

        if (logAsLogin && !canceledLogout)
        {
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "joined the server!"));
        }
        else if (canceledLogout)
        {
            m_LogoutQueueInital.Remove(player);
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "canceled logout"));
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) aborted logout process [canceledLogout]", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) aborted logout process [canceledLogout]", identity.GetName(), identity.GetPlainId()));
        }

        if(identity && player)
        {
            if(GetPermissionManager().HasUserGroup(identity.GetPlainId()))
            {
                GetRPCManager().SendRPC("RPC_MissionGameplay", "AuthCheck", new Param1<bool>(true), true, identity);
                if (g_Game.IsPasswordProtectionDisabled())
                {
                    GetRPCManager().VSendRPC("RPC_MissionGameplay", "EnableTogglesNonPassword", new Param1<bool>(true), true, identity);
                }
            }
        }
    }

    /*
    * Called when player disconnect, including when they are waiting/trigger for the logout timer to tick down to 0
    * if the logout is cancelled before the timer is complete, "HandleOnPlayerConnect" is invoked.
    */
    void HandleOnPlayerDisconnected(PlayerBase player, PlayerIdentity identity, bool finished)
    {
        if ((IsPlayerDisconnecting(player) || !finished) && identity)
        {
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) initiated disconnect process...", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("Player \"%1\" (steamid=%2) initiated disconnect process...", identity.GetName(), identity.GetPlainId()));
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "initiated disconnect process..⌛"));
        }
        else if (identity && finished)
        {
            m_LogoutQueueInital.Remove(player);
            //Normal logout, player waited for logout timer to deplete full.
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) disconnected from server.", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("Player \"%1\" (steamid=%2) disconnected from server.", identity.GetName(), identity.GetPlainId()));
            

            if (PlayerListManager.RemoveEntry(identity.GetId(), true))
            {
                Print("HandleOnPlayerDisconnected -> PlayerListManager.RemoveEntry ► " + identity.GetId());
            }
        }
    }

    bool IsLogoutTimeExpired(PlayerBase player)
    {
        if (!player)
            return false;

        LogoutInfo params = m_LogoutPlayers.Get(player);
        if (!params)
            params = m_NewLogoutPlayers.Get(player);

        return (params && GetGame().GetTime() > params.param1);
    }
};