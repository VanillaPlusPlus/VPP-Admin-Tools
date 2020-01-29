modded class MissionServer
{
	private float m_LoginTimeMs;
	private bool  m_ServerLocked = false;
	
	void MissionServer()
	{
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
					m_LoginTimeMs = (line_content.ToInt() * 1000) + 5000.0;
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
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Server is NO LONGER locked!",NotifyTypes.NOTIFY);
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ServerManager] Unlocked the server"));
			}else{
				m_ServerLocked = true;
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Server is now locked!",NotifyTypes.NOTIFY);
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
		if (eventTypeId == ClientPrepareEventTypeID)
		{
			super.OnEvent(eventTypeId,params);
			
			ClientPrepareEventParams clientPrepareParams;
			Class.CastTo(clientPrepareParams, params);

			ref BannedPlayer bannedPlayer = GetBansManager().GetBannedPlayer(clientPrepareParams.param1.GetPlainId());
			if (bannedPlayer != null)
			{
				autoptr BanDuration expireDate = bannedPlayer.expirationDate;
				string banReason = bannedPlayer.banReason;
				if (expireDate.Permanent)
				{
					banReason += "\n Expiration Date: Permanent";
				}
				else
				{
					banReason += string.Format("\n Expiration Date %1/%2/%3  %4%5%6",expireDate.Year.ToString(),expireDate.Month.ToString(),expireDate.Day.ToString(),expireDate.Hour.ToString(),":",expireDate.Minute.ToString());
				}
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, m_LoginTimeMs, false, clientPrepareParams.param1.GetPlainId(), banReason); 
			}
			
			if (m_ServerLocked)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.InvokeKickPlayer, m_LoginTimeMs, false, clientPrepareParams.param1.GetPlainId(),"Server is Currently Locked!");
			}

			if (!GetPlayerListManager().HasPlayerInList( clientPrepareParams.param1.GetPlainId() ))
			{
				//WebHook Notifications
				GetWebHooksManager().PostData(JoinLeaveMessage, new JoinLeaveMessage(clientPrepareParams.param1.GetName(), clientPrepareParams.param1.GetPlainId(), "joined the server!"));
			}

		}else{
			super.OnEvent(eventTypeId,params);
		}
	}
	
	void InvokeKickPlayer(string id, string msg)
	{
		autoptr PlayerIdentity identity = GetPermissionManager().GetIdentityById( id );
		if(identity != null)
		{
			GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( msg ), true, identity);
			Print("InvokeKickPlayerInvokeKickPlayerInvokeKickPlayerInvokeKickPlayerInvokeKickPlayerInvokeKickPlayerInvokeKickPlayer");
		}
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.InvokeKickPlayer);
	}
		
	override void OnClientReconnectEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReconnectEvent(identity, player);
		if ( identity != null )
		{
			if(!GetPlayerListManager().HasPlayerInList(identity.GetPlainId()))
			{
				GetPlayerListManager().AddUserServer(identity.GetName(), identity.GetPlainId());
			}
			
			if(GetPermissionManager().HasUserGroup(identity.GetPlainId()))
			{
				GetPlayerListManager().SendPlayerList(identity);
			}
		}
	}
	
	override void UpdateLogoutPlayers()
	{
		for ( int i = 0; i < m_LogoutPlayers.Count(); i++ )
		{
			LogoutInfo info = m_LogoutPlayers.GetElement(i);
			
			if (GetGame().GetTime() >= info.param1)
			{
				PlayerIdentity identity;
				PlayerBase player = m_LogoutPlayers.GetKey(i);
				if (player)
				{
					identity = player.GetIdentity();
					if (identity != null)
					{
						if (GetPlayerListManager().HasPlayerInList(identity.GetPlainId()))
						{
							GetPlayerListManager().RemoveUserServer(identity.GetPlainId());
							Print("UpdateLogoutPlayers:: Updating VPP Sync List!");
						}
					}
					super.UpdateLogoutPlayers();
				}
			}
		}
	}
	
	/*
		overriding this function is a fucking BITCH TO DO. so i just copy pasta and add our shit to it. Fuck it.
	*/
	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		bool disconnectNow = true;
		
		// TODO: get out of vehicle
		// using database and no saving if authorization failed
		if (GetHive() && !authFailed)
		{			
			if (player.IsAlive())
			{	
				if (!m_LogoutPlayers.Contains(player))
				{
					Print("[Logout]: New player " + identity.GetPlainId() + " with logout time " + logoutTime.ToString());
					
					// inform client about logout time
					GetGame().SendLogoutTime(player, logoutTime);
			
					// wait for some time before logout and save
					LogoutInfo params = new LogoutInfo(GetGame().GetTime() + logoutTime * 1000, identity.GetPlainId());
					m_LogoutPlayers.Insert(player, params);
										
					// wait until logout timer runs out
					disconnectNow = false;		
				}
				return;
			}		
		}
		
		if (disconnectNow)
		{
			Print("[Logout]: New player " + identity.GetPlainId() + " with instant logout");
			
			// inform client about instant logout
			GetGame().SendLogoutTime(player, 0);
			
			PlayerDisconnected(player, identity, identity.GetPlainId());
		}
	}

	override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		if (identity != null) uid = identity.GetPlainId();
		
		if (GetPlayerListManager().HasPlayerInList(uid))
		{
			GetPlayerListManager().RemoveUserServer(uid);
			Print("PlayerDisconnected:: Updating VPP Sync List! removed: "+uid);
		}
	
		super.PlayerDisconnected(player,identity,uid);
	}
}