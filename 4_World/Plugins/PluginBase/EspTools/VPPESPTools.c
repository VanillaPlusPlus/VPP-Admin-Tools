class VPPESPTools extends PluginBase
{
	void VPPESPTools()
	{
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "DeleteItems", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "PlayerESP", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "ToggleESP", this, SingeplayerExecutionType.Server );
	}
	
	void DeleteItems(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<Object>> data;
		if( !ctx.Read( data ) ) return;
		
		if( type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:DeleteObjects")) return;
			
			autoptr array<Object> copyArray = new array<Object>;
			copyArray.Copy(data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "Deleted Objects Using ESP tools: total:[" + copyArray.Count() +"]"));
			foreach(Object obj : copyArray)
			{
				if (obj != null)
					GetGame().ObjectDelete(obj);
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully Deleted ["+copyArray.Count()+"] item(s)!",NotifyTypes.NOTIFY);
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] deleted objects using ESP",sender.GetPlainId(), sender.GetName()));
		}
	}

	void ToggleESP(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu")) return;

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "just toggled ON ESP Tools."));
		}
	}
		
	void PlayerESP(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<array<string>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server && data.param1 != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:PlayerESP")) return;
			
			autoptr array<string> playersIDs = new array<string>;
			autoptr array<ref PlayerStatsData> playerStats = new array<ref PlayerStatsData>;
			array<string> networkIds = new array<string>;
			
			playersIDs = data.param1;
			
			foreach(string pid : playersIDs)
			{
				autoptr PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(pid);
				
				if(targetPlayer == null) return;
				
				map<string,string> m_Stats = new map<string,string>;
				m_Stats.Insert("Name",targetPlayer.GetIdentity().GetName());
				m_Stats.Insert("Health",targetPlayer.GetHealth("","").ToString());
				m_Stats.Insert("Blood",targetPlayer.GetHealth("","Blood").ToString());
				m_Stats.Insert("Steam64",targetPlayer.GetIdentity().GetPlainId());
				autoptr PlayerStatsData statsData = new PlayerStatsData(m_Stats);
				
				networkIds.Insert(targetPlayer.GetNetworkIDString());
				playerStats.Insert(statsData);
			}
			GetRPCManager().SendRPC( "RPC_VPPESPPlayerTracker", "HandleData", new Param2<array<string>,array<ref PlayerStatsData>>(networkIds, playerStats), true, sender);
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just requested a scan for players nearby",sender.GetPlainId(), sender.GetName()));
		}
	}
};