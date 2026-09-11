class ServerManager extends PluginBase
{
	private int  m_TimeCounter;
	private bool m_RestartInProgress;
	
	void ServerManager()
	{
		/* RPCs */
		GetRPCManager().AddRPC( "RPC_ServerManager", "RequestServerMonitor", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ServerManager", "RequestActivityMap", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ServerManager", "RequestRestartServer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ServerManager", "RequestKickAllPlayers", this, SingeplayerExecutionType.Server );
	}

	void RequestRestartServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if (!ctx.Read(data)) return;
		
	 	if ( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "ServerManager:RestartServer")) return;
			
			if (!m_RestartInProgress)
			{
				m_TimeCounter       = data.param1;
				m_RestartInProgress = true;
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RestartTicker, 1000, true);
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ServerManager] Requested for server restart in: " + data.param1 + " seconds"));
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) requested a server restart", sender.GetName(), sender.GetPlainId()));
			}
			else
			{
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_RESTART_ERROR",NotifyTypes.ERROR);
			}
		}
	}
	
	void RestartTicker()
	{
		m_TimeCounter--;
		if (m_TimeCounter < 0)
		{
			GetGame().DisconnectSessionForce();
			GetGame().RestartMission();
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.RestartTicker);
		}
		else
		{
			//alert players
			array<Man> players = new array<Man>;
			GetGame().GetWorld().GetPlayerList(players);
			foreach(Man player : players)
			{
				if (PlayerBase.Cast(player) != null)
				{
					GetPermissionManager().NotifyPlayer(PlayerBase.Cast(player).VPlayerGetSteamId(),"#VSTR_NOTIFY_RESTART_IN "+m_TimeCounter, NotifyTypes.NOTIFY, 0.2);
				}
			}
		}
	}
	
	void RequestKickAllPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "ServerManager:KickAllPlayers")) return;
			
			array<Man> players = new array<Man>;
			GetGame().GetWorld().GetPlayerList(players);
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) just kicked all players from the server", sender.GetName(), sender.GetPlainId()));

			foreach(Man player : players)
			{
				if (player && PlayerBase.Cast(player).VPlayerGetSteamId() != sender.GetPlainId())
				{
					GetRPCManager().VSendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( data.param1 ), true, player.GetIdentity());
					GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Kicking Player: \"%3\" (steamid=%4)", sender.GetName(), sender.GetPlainId(), PlayerBase.Cast(player).VPlayerGetName(), PlayerBase.Cast(player).VPlayerGetSteamId()));
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ServerManager] Requested to kick all players from server: " + players.Count()));
		}
	}
	
	void RequestActivityMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server )
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuServerManager", "", false)) return;
			
			array<ref VPPPlayerData> m_List = new array<ref VPPPlayerData>;
			array<Man>   m_Players = new array<Man>;
			GetGame().GetWorld().GetPlayerList( m_Players );
			foreach(Man player : m_Players)
			{
				m_List.Insert( new VPPPlayerData(PlayerBase.Cast(player).VPlayerGetName(), player.GetPosition()) );
			}
			GetRPCManager().VSendRPC( "RPC_MenuServerManager", "UpdateActivityMap", new Param1<ref array<ref VPPPlayerData>>( m_List ), true, sender);
		}
	}

	void RequestServerMonitor( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if( type == CallType.Server )
		{
			GetRPCManager().VSendRPC( "RPC_MenuServerManager", "UpdateServerMonitor", new Param1<int>(GetGame().GetTime()), true, sender);
		}
	}
}