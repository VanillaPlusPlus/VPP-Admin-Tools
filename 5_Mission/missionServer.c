modded class MissionServer
{
	private float m_LoginTimeMs   = 15000; //15 sec default
	private bool  m_ServerLocked  = false;
	private int   m_MaxKickAttempts = 5;
	private ref map<string,int> m_KickQueue;
	
	void MissionServer()
	{
		m_KickQueue = new map<string,int>;
		//Event handlers
		VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect").Insert( HandleOnPlayerConnect );
		VPPATGetEventHandler().GetEventInvoker("OnPlayerDisconnected").Insert( HandleOnPlayerDisconnected );
		VPPATGetEventHandler().GetEventInvoker("OnClientDisconnectCancel").Insert( HandleOnPlayerConnect );

		//=============RPC's====================
		GetRPCManager().AddRPC( "RPC_MissionServer", "RequestLockServer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_MissionServer", "HandleChatCommand", this, SingeplayerExecutionType.Server );		
		//======================================
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.FindLoginTime, 1500.0, false);
	}

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
	
	void HandleChatCommand( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data; //chat string
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
			GetChatCommandManager().ParseCommand(data.param1,sender.GetPlainId());
		}
	}
	
	override void OnEvent(EventType eventTypeId, Param params) 
	{
		PlayerIdentity identity;
		PlayerBase 	   player;
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
				onPlayerConnect.Invoke(player, identity, false);

			break;

			case ClientNewEventTypeID:
			ClientNewEventParams newParams;
			Class.CastTo(newParams, params);
			
			identity = newParams.param1;
			onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
			
			if(onPlayerConnect)
				onPlayerConnect.Invoke(player, identity, false);
			
			break;

			case ClientReconnectEventTypeID:
			ClientReconnectEventParams reconnectParams;
			Class.CastTo(reconnectParams, params);
			
			identity = reconnectParams.param1;
			
			onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
			
			if(onPlayerConnect)
				onPlayerConnect.Invoke(player, identity, false);

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
				onPlayerConnect.Invoke(player, identity, true);

			break;

			case ClientPrepareEventTypeID:
			ClientPrepareEventParams clientPrepareParams;
			Class.CastTo(clientPrepareParams, params);

			onPlayerConnect = VPPATGetEventHandler().GetEventInvoker("OnPlayerConnect");
			
			if(onPlayerConnect)
				onPlayerConnect.Invoke(NULL, clientPrepareParams.param1, false); //Invoke anyway sometimes ClientReadyEventTypeID does not trigger.

			ref BannedPlayer bannedPlayer = GetBansManager().GetBannedPlayer(clientPrepareParams.param1.GetPlainId());
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

			if (!GetPlayerListManager().HasPlayerInList( clientPrepareParams.param1.GetPlainId() ))
				GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(clientPrepareParams.param1.GetName(), clientPrepareParams.param1.GetPlainId(), "joined the server!"));

			break;
		}
		super.OnEvent(eventTypeId,params);
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
			GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( msg ), true, identity);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
			m_KickQueue.Remove(id);
		}
	}

	void HandleOnPlayerConnect(PlayerBase player, PlayerIdentity identity, bool canceledLogout)
	{
		if(identity == NULL)
		{
			GetSimpleLogger().Log(string.Format("A Player attempted to connect but with a NULL PlayerIdentity"));
			Print(string.Format("[VPPAT] A Player attempted to connect but with a NULL PlayerIdentity"));
			return;
		}

		if ( !canceledLogout )
		{
			GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));
			Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) connected to server!", identity.GetName(), identity.GetPlainId()));
		}
		else
		{
			GetSimpleLogger().Log(string.Format("Player \"%1\" (steamId=%2) re-connected to server [canceledLogout]", identity.GetName(), identity.GetPlainId()));
			Print(string.Format("[VPPAT] Player \"%1\" (steamId=%2) re-connected to server [canceledLogout]", identity.GetName(), identity.GetPlainId()));
		}

		if(identity)
		{
			if(!GetPlayerListManager().HasPlayerInList(identity.GetPlainId()))
				GetPlayerListManager().AddUserServer(identity.GetName(), identity.GetPlainId());
			
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
		}else{
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