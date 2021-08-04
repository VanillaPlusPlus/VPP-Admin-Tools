modded class MissionServer
{
	//Server vars
    private float m_LoginTimeMs   = 15000; //15 sec default
    private bool  m_ServerLocked  = false;
    private int   m_MaxKickAttempts = 5;
    private ref map<string,int> m_KickQueue;

    override void OnMissionStart()
    {
        super.OnMissionStart();
        Print("[MissionServer] OnMissionStart - Server");
        //Post message of server status
        GetWebHooksManager().PostData(ServerStatusMessage, new ServerStatusMessage());
        new VPPATInventorySlots;
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
        //Event handlers
        VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect").Insert(HandleOnPlayerConnect);
        VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected").Insert(HandleOnPlayerDisconnected);

        g_Game.SetServerName(GetServerName());
        //=============RPC's====================
        GetRPCManager().AddRPC( "RPC_MissionServer", "RequestLockServer", this, SingeplayerExecutionType.Server );
        GetRPCManager().AddRPC( "RPC_MissionServer", "HandleChatCommand", this, SingeplayerExecutionType.Server );      
        //======================================
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.FindLoginTime, 1500.0, false);
    }

    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        /*
        * Server part
        */
        PlayerIdentity identity;
        PlayerBase     player;
        ScriptInvoker onPlayerConnect;

        switch(eventTypeId)
        {
            case ClientReadyEventTypeID:
            ClientReadyEventParams readyParams;
            Class.CastTo(readyParams, params);
            
            identity = readyParams.param1;
            player   = PlayerBase.Cast(readyParams.param2);

            onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
            if(onPlayerConnect)
                onPlayerConnect.Invoke(player, identity, false, true);

            break;

            case ClientNewEventTypeID:
            ClientNewEventParams newParams;
            Class.CastTo(newParams, params);
            
            identity = newParams.param1;
            onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
            if(onPlayerConnect)
                onPlayerConnect.Invoke(player, identity, false, false);

            break;

            case ClientReconnectEventTypeID:
            ClientReconnectEventParams reconnectParams;
            Class.CastTo(reconnectParams, params);
            
            identity = reconnectParams.param1;
            
            onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
            if(onPlayerConnect)
                onPlayerConnect.Invoke(player, identity, false, true);

            break;
            
            case ClientDisconnectedEventTypeID:
            ClientDisconnectedEventParams discoParams;
            Class.CastTo(discoParams, params);      
            
            identity = discoParams.param1;
            player   = PlayerBase.Cast(discoParams.param2);

            ScriptInvoker onClientDisconnectedEvent = VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected");
            
            if(onClientDisconnectedEvent)
                onClientDisconnectedEvent.Invoke(player, identity);

            break;
                
            case LogoutCancelEventTypeID:
            LogoutCancelEventParams logoutCancelParams;
            Class.CastTo(logoutCancelParams, params);               
            Class.CastTo(player, logoutCancelParams.param1);

            identity = player.GetIdentity();

            onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
            if(onPlayerConnect)
                onPlayerConnect.Invoke(player, identity, true, false);

            break;

            case ClientPrepareEventTypeID:
            ClientPrepareEventParams clientPrepareParams;
            Class.CastTo(clientPrepareParams, params);

            onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
            
            if(onPlayerConnect)
                onPlayerConnect.Invoke(NULL, clientPrepareParams.param1, false, false);

            BannedPlayer bannedPlayer = GetBansManager().GetBannedPlayer(clientPrepareParams.param1.GetPlainId());
            if (bannedPlayer != NULL)
            {
                BanDuration expireDate = bannedPlayer.expirationDate;
                string banReason = bannedPlayer.banReason;
                if (expireDate.Permanent)
                    banReason += "\n Expiration Date: Permanent";
                else
                    banReason += string.Format("\n Expiration Date %1/%2/%3  %4%5%6",expireDate.Year.ToString(),expireDate.Month.ToString(),expireDate.Day.ToString(),expireDate.Hour.ToString(),":",expireDate.Minute.ToString());

                if ( !m_KickQueue.Contains( clientPrepareParams.param1.GetPlainId() ) )
                {
                    m_KickQueue.Insert(clientPrepareParams.param1.GetPlainId(), 0);
                    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, m_LoginTimeMs, true, clientPrepareParams.param1.GetPlainId(), banReason); 
                }
            }
            
            if (m_ServerLocked)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, m_LoginTimeMs, true, clientPrepareParams.param1.GetPlainId(),"#VSTR_NOTIFY_SERVER_LOCKED");

            break;
        }
    }

    override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
    {
        super.PlayerDisconnected(player, identity, uid);
        
        if ( identity != NULL )
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "left the server!"));
        else if ( player )
            GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(player.VPlayerGetName(), player.VPlayerGetSteamId(), "left the server!"));
    }

    /*
    **
    *  ///////////Server RPCs Section/////////////////
       ///////////////////////////////////////////////
    */
    private void FindLoginTime()
    {
        if (!GetHive())
        {
            //No hive, keep timer short.
            m_LoginTimeMs = 3000.0;
            return;
        }
        string globalsXmlPath = XMLEditor.Cast(GetPluginManager().GetPluginByType(XMLEditor)).GetMissionPath("\\db\\globals.xml");
        FileHandle file = OpenFile(globalsXmlPath, FileMode.READ);
        if (file != 0)
        {
            string line_content = "";
            FGets( file,  line_content );
            while ( line_content != "" )
            {
                line_content.ToLower();
                if (line_content.Contains("<var name=\"timelogin\""))
                {
                    line_content.Trim();
                    line_content.Replace("<var name=\"timelogin\" type=\"0\" value=\"", "");
                    line_content.Replace("\"/>", "");
                    m_LoginTimeMs = (line_content.ToInt() * 1000) + 1200.0;
                    break;
                }
                FGets( file,  line_content );
            }
            CloseFile(file);
        }
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.FindLoginTime);
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

    string GetServerName()
    {
        string result;
        string cfgPath;
        GetGame().CommandlineGetParam("config", cfgPath);
        string fullPathToConfig = "$CurrentDir:" + cfgPath;

        if ( cfgPath == string.Empty )
            fullPathToConfig = "$CurrentDir:serverDZ.cfg";
        
        if ( !FileExist(fullPathToConfig) )
            return "[SERVER NAME NOT FOUND]";

        FileHandle serverCfg = OpenFile(fullPathToConfig, FileMode.READ);
        if (serverCfg != 0)
        {
            array<string> cfgData = new array<string>;
            string line_content = "";
            int char_count = FGets( serverCfg,  line_content );
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
                                    result = dirtyName;
                                    break;
                                }
                            }
                            strIndex++;
                        }
                    }
                }
            }
        }
        return result;
    }
    
    void HandleChatCommand( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
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
                m_KickQueue.Remove(id);
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
            }
        }

        PlayerIdentity identity = GetPermissionManager().GetIdentityById( id );
        if( identity != NULL )
        {
            GetSimpleLogger().Log(string.Format("Kicking banned player \"%1\" (steamId=%2) kick message: (%3)", identity.GetName(), id, msg));
            GetRPCManager().VSendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( msg ), true, identity);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
            m_KickQueue.Remove(id);
        }
    }

    void HandleOnPlayerConnect(PlayerBase player, PlayerIdentity identity, bool canceledLogout, bool logAsLogin)
    {
        if(identity == NULL)
        {
            GetSimpleLogger().Log(string.Format("A Player attempted to connect but with a NULL PlayerIdentity"));
            Print(string.Format("[VPPAT] A Player attempted to connect but with a NULL PlayerIdentity"));
            return;
        }

        if (logAsLogin)
        {
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));

            if (!GetPlayerListManager().HasPlayerInList( identity.GetPlainId() ))
                GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(identity.GetName(), identity.GetPlainId(), "joined the server!"));
        }
        else if (canceledLogout)
        {
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) re-connected to server [canceledLogout]", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) re-connected to server [canceledLogout]", identity.GetName(), identity.GetPlainId()));
        }

        if(identity)
        {
            if(!GetPlayerListManager().HasPlayerInList(identity.GetPlainId()))
                GetPlayerListManager().AddUserServer(identity.GetName(), identity.GetPlainId(), identity.GetPlayerId());
            
            if(GetPermissionManager().HasUserGroup(identity.GetPlainId()))
                GetPlayerListManager().SendPlayerList(identity);
        }
    }

    void HandleOnPlayerDisconnected(PlayerBase player, PlayerIdentity identity)
    {
        if ( identity )
        {
            GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) disconnected from server.", identity.GetName(), identity.GetPlainId()));
            Print(string.Format("Player \"%1\" (steamid=%2) disconnected from server.", identity.GetName(), identity.GetPlainId()));
        }
        else
        {
            GetSimpleLogger().Log("HandleOnPlayerDisconnected Called but PlayerIdentity was NULL ? Early logout?");
            Print("[VPPAT] HandleOnPlayerDisconnected Called but PlayerIdentity was NULL ? Early logout?");
            return;
        }

        string uid = identity.GetPlainId();
        if (GetPlayerListManager().HasPlayerInList(uid))
        {
            GetPlayerListManager().RemoveUserServer(uid);
            Print("[VPPAT] Updating VPP Sync List! removed: "+uid);
        }
    }
};