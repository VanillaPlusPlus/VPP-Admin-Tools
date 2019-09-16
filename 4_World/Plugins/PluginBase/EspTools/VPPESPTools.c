class VPPESPTools extends PluginBase
{
	void VPPESPTools()
	{
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "DeleteItems", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPESPTools", "PlayerESP", this, SingeplayerExecutionType.Server );
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
			foreach(Object obj : copyArray)
			{
				if (obj != null)
					GetGame().ObjectDelete(obj);
			}
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Successfully Deleted ["+copyArray.Count()+"] item(s)!",NotifyTypes.NOTIFY);
		}
	}
		
	void PlayerESP(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<array<Man>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server && data.param1 != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "EspToolsMenu:PlayerESP")) return;
			
			array<Man> players  = new array<Man>;
			array<ref PlayerStatsData> playerStats = new array<ref PlayerStatsData>;
			
			players = data.param1;
			
			foreach(Man targetPlayer : players)
			{
				autoptr PlayerIdentity targetIdentity = targetPlayer.GetIdentity();
				
				if(!targetIdentity) return;
				
				if (targetPlayer == null)
				{
					Print("TARGET OBJ:: NULL!!");
					return;
				}
				map<string,string> m_Stats = new map<string,string>;
				m_Stats.Insert("Name",targetIdentity.GetName());
				m_Stats.Insert("Health",targetPlayer.GetHealth("","").ToString());
				m_Stats.Insert("Blood",targetPlayer.GetHealth("","Blood").ToString());
				m_Stats.Insert("Steam64",targetIdentity.GetPlainId());
				//m_Stats.Insert("GUID",targetIdentity.GetId());
				autoptr PlayerStatsData statsData = new PlayerStatsData(m_Stats);
				
				playerStats.Insert(statsData);
			}
			GetRPCManager().SendRPC( "RPC_VPPESPPlayerTracker", "HandleData", new Param2<array<Man>, array<ref PlayerStatsData>>(data.param1, playerStats), true, sender);
		}
	}
};