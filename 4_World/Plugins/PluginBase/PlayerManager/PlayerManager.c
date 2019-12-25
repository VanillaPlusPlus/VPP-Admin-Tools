class PlayerManager extends PluginBase
{
	private ref Timer m_RPCDelay;
	private ref array<string> m_InvisiblePlayers;
	
	void PlayerManager()
	{
		m_InvisiblePlayers = new array<string>;
		/* RPCs */
		GetRPCManager().AddRPC("RPC_PlayerManager", "GetPlayerStats", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "GetPlayerStatsGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "RequestInvisibility", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "ToggleGodmode", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "SendMessage", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "SetPlayerStats", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "HealPlayers", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "KillPlayers", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "SpectatePlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "TeleportHandle", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "KickPlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "BanPlayer", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "GiveGodmode", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PlayerManager", "GiveUnlimitedAmmo", this, SingeplayerExecutionType.Server );
	}
	
	override void OnInit()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.UpdateInvisPlayers, 10000, true);
	}
	
	void UpdateInvisPlayers()
	{
		if (m_InvisiblePlayers.Count() <= 0) return;
		
		GetSimpleLogger().Log("[PlayerManager] UpdateInvisPlayers(): Updating "+m_InvisiblePlayers.Count()+" Players invis coat");
		foreach(string id : m_InvisiblePlayers)
		{
			autoptr PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(id);
			if (pb != null)
			{
				pb.setInvisibility(true);
			}
		}
	}
	
	void BanPlayer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data; //targets
        if ( !ctx.Read( data ) ) return;
		
		if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:BanPlayer")) return;
			
			autoptr array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:BanPlayer",tgId))
				{
					autoptr PlayerIdentity pid = GetPermissionManager().GetIdentityById(tgId);
					autoptr BanDuration banDuration = GetBansManager().GetCurrentTimeStamp();
					string banAuthorDetails = string.Format("%1|%2",sender.GetName(),sender.GetPlainId());
					banDuration.Permanent = true;
					if (GetBansManager().AddToBanList(new BannedPlayer(pid.GetName(),tgId,pid.GetId(),banDuration,banAuthorDetails,"Banned By Server Admin "+sender.GetName())))
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Player "+pid.GetName()+" is now banned!",NotifyTypes.NOTIFY);
					else
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Unable to ban player "+pid.GetName(),NotifyTypes.NOTIFY);
				}
			}
		}
	}
	
	void KickPlayer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<string>,string> data; //targets, reason
        if ( !ctx.Read( data ) ) return;
		
		if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KickPlayer")) return;
			autoptr array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KickPlayer",tgId))
				{
					autoptr PlayerIdentity tempiden = GetPermissionManager().GetIdentityById(tgId);
					if (tempiden != null)
					{
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Kicking player: "+tempiden.GetName(),NotifyTypes.NOTIFY);
						GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( data.param2 ), true, tempiden);
						GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Kicked player [%3]",sender.GetPlainId(), sender.GetName(), tempiden.GetName()));
					}
				}
			}
		}
	}
	
	void TeleportHandle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<bool,ref array<string>> data; //opertation type,targets
        if ( !ctx.Read( data ) ) return;
		
		if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportToPlayer") || !GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportPlayerTo")) return;
			
			autoptr Man self = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			autoptr array<string> ids = data.param2;
			foreach(string targetId: ids)
			{
				if (data.param1) //TeleportToPlayer
				{					
					GetTeleportManager().GotoPlayer(GetPermissionManager().GetPlayerBaseByID(targetId), self, sender.GetPlainId());
				}
				else
				{
					if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportPlayerTo",targetId)) return;
					
					GetTeleportManager().BringPlayer(GetPermissionManager().GetPlayerBaseByID(targetId), self.GetPosition(), "");
				}
			}
		}
	}
	
	void SpectatePlayer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		ref Param1<string> data; //target id
        if ( !ctx.Read( data ) ) return;
		
		if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SpectatePlayer")) return;
			
			if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SpectatePlayer",data.param1))
			{
				if (sender.GetPlainId() == data.param1)
				{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You can't spectate your self!",NotifyTypes.ERROR);
					return;
				}
				
				autoptr PlayerBase tgp = GetPermissionManager().GetPlayerBaseByID(data.param1);
				if (tgp != null)
				{
					autoptr PlayerBase adminPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
					if (adminPlayer != null)
						adminPlayer.SetPosition(Vector(tgp.GetPosition()[0],-120,tgp.GetPosition()[2]));

					m_RPCDelay = new Timer();
					m_RPCDelay.Run(2.0,this,"InvokeSpectate", new Param2<string,string>(data.param1,sender.GetPlainId()),false);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Press (PAGE-UP) to exit spectate!",NotifyTypes.NOTIFY);
				}

				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] started to spectate player [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	void InvokeSpectate(string id, string adminID)
	{
		autoptr PlayerBase spectateTarget = GetPermissionManager().GetPlayerBaseByID(id);
		autoptr PlayerBase adminPlayer    = GetPermissionManager().GetPlayerBaseByID(adminID);
		if (spectateTarget == null) return;

		if (adminPlayer != null){
			autoptr PlayerIdentity adminIdentity = adminPlayer.GetIdentity();
			GetGame().ObjectDelete(adminPlayer);
			GetRPCManager().SendRPC( "RPC_MenuPlayerManager", "InitSpectate", new Param1<Object>(spectateTarget), true, adminIdentity );
		}
	}
	
	void KillPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KillPlayers")) return;
			
			autoptr array<string> ids = data.param1;
			if (ids.Count() < 1) return;
			
			foreach(string id : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KillPlayers",id))
				{
					autoptr PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					if (targetPlayer != null)
					{
						targetPlayer.SetHealth(0);
						GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] executed kill command on [%3]",sender.GetPlainId(), sender.GetName(), id));
					}
				}
			}
		}
	}
	
	void SendMessage( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param3<string,string,ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param3);
        	autoptr PlayerIdentity TargetPlayer;
        	if (sender != NULL)
        	{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SendMessage"))
				{
					for (int i = 0; i < pGUID.Count(); ++i)
	    		    {
	    		    	TargetPlayer =  GetPermissionManager().GetIdentityById(pGUID.Get(i));
	    		    	if (TargetPlayer != NULL)
	    		    	{
	    		    		//Global, Identity ,Title, Message, Duration, FadeIn Time, Force show, DoFadeIn , Imagepath, Size X, Size Y
	                        g_Game.SendMessage(
	                         false,
	                         TargetPlayer,
	                         data.param1,
	                         data.param2,
	                         10,
	                         2,
	                         false,
	                         true,
	                         "",
	                         0,
	                         0);

	                        GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] sent a message to [%3]",sender.GetPlainId(), sender.GetName(), pGUID.Get(i)));
	    		    	}
	    		    }
				}
        	}
        }
	}
	
	void ToggleGodmode( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
        if( type == CallType.Server )
        {
            if (sender && GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:GodMode"))
            {
    				autoptr PlayerBase AdminPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
				
					if(AdminPlayer)
					{
						if (AdminPlayer.GodModeStatus())
	                  	GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"GodMode Toggled OFF!",NotifyTypes.NOTIFY);
		    			else
		                GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"GodMode Toggled ON!",NotifyTypes.NOTIFY);
		    			
						AdminPlayer.setGodMode(!AdminPlayer.GodModeStatus());
						GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just toggled godmode",sender.GetPlainId(), sender.GetName()));
					}
		  		}
        }
	}
	
	void GiveGodmode( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server )
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:GiveGodmode",data.param1)) return;
			
			autoptr PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				if (TargetPlayer.GodModeStatus()){
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Revoked your godmode",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Revoked selected players godmode!",NotifyTypes.NOTIFY);
				}
		    	else{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Gave you GodMode",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Gave selected player godmode!",NotifyTypes.NOTIFY);
				}
				
				TargetPlayer.setGodMode(!TargetPlayer.GodModeStatus());
				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] gave godmode to [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	void GiveUnlimitedAmmo( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server )
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:GiveUnlimitedAmmo",data.param1)) return;
			autoptr PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				if (TargetPlayer.IsUnlimitedAmmo())
				{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Revoked your unlimited ammo",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Revoked selected players unlimited ammo!",NotifyTypes.NOTIFY);
				}else{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Gave you Unlimited Ammo!",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Gave selected player Unlimited Ammo!",NotifyTypes.NOTIFY);
				}
				
				TargetPlayer.SetUnlimitedAmmo(!TargetPlayer.IsUnlimitedAmmo());
				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] gave unlimited ammo to [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	
	void HealPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data; // player id's
		if(!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			string adminID  = sender.GetPlainId();
			if (!GetPermissionManager().VerifyPermission(adminID, "PlayerManager:HealPlayers")) return;
			
			autoptr array<string> ids = data.param1;
			if (ids.Count() < 1) return;
			
			foreach(string id : ids)
			{
				autoptr PlayerBase targetPlayer;
				if (GetPermissionManager().VerifyPermission(adminID, "PlayerManager:HealPlayers", id))
				{
					targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					if (targetPlayer != null)
					{
						targetPlayer.SetHealth( targetPlayer.GetMaxHealth( "", "" ) );
						targetPlayer.SetHealth( "","Blood", targetPlayer.GetMaxHealth( "", "Blood" ) );
						targetPlayer.GetStatHeatComfort().Set(targetPlayer.GetStatHeatComfort().GetMax());
						targetPlayer.GetStatTremor().Set(targetPlayer.GetStatTremor().GetMin());
						targetPlayer.GetStatWet().Set(targetPlayer.GetStatWet().GetMin());
						targetPlayer.GetStatEnergy().Set(targetPlayer.GetStatEnergy().GetMax());
						targetPlayer.GetStatWater().Set(targetPlayer.GetStatWater().GetMax());
						targetPlayer.GetStatDiet().Set(targetPlayer.GetStatDiet().GetMax());
						targetPlayer.GetStatSpecialty().Set(targetPlayer.GetStatSpecialty().GetMax());
						targetPlayer.SetBleedingBits(0);

						GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] healed player [%3]",sender.GetPlainId(), sender.GetName(), id));
					}
				}
			}
		}
	}

	/*
		Blood,Health,Shock,Water,Food
	*/
	void SetPlayerStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<float,string,string> data; //stat level, player id, stat type
		if(!ctx.Read(data)) return;

		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SetPlayerStats") || !GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SetPlayerStats",data.param2)) return;
			
			autoptr PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param2);
			if (targetPlayer == null) return;
			
			switch(data.param3)
			{
				case "Blood":
				targetPlayer.SetHealth( "","Blood", data.param1 );
				break;
				
				case "Health":
				targetPlayer.SetHealth( data.param1 );
				break;
				
				case "Shock":
				targetPlayer.SetHealth( "","Shock", data.param1 );
				break;
				
				case "Water":
				targetPlayer.GetStatWater().Set(data.param1);
				break;
				
				case "Energy":
				targetPlayer.GetStatEnergy().Set(data.param1);
				break;
			}
			GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] just updated a health stat on [%3]",sender.GetPlainId(), sender.GetName(), data.param2));
		}
	}
		
	void GetPlayerStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		
		if(!ctx.Read(data)) return;

		if (type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuPlayerManager")) return;
			
			string id = data.param1;
			autoptr Man playerMan = GetPermissionManager().GetPlayerBaseByID(id);
			if (playerMan != null && sender != null)
			{
				autoptr PlayerIdentity identity = playerMan.GetIdentity();
				if (identity != null)
				{
					string inHandsName = "";
					
					if(playerMan.GetHumanInventory() != null && playerMan.GetHumanInventory().GetEntityInHands() != null)
						inHandsName = playerMan.GetHumanInventory().GetEntityInHands().GetDisplayName();
				
					if(inHandsName == "")
						inHandsName = "None";

					autoptr map<string,string> dataMap = new map<string,string>;
					dataMap.Insert("Guid",identity.GetId());
					dataMap.Insert("Name",identity.GetName());
					dataMap.Insert("Blood",playerMan.GetHealth("GlobalHealth", "Blood").ToString());
					dataMap.Insert("Health",playerMan.GetHealth("GlobalHealth", "").ToString());
					dataMap.Insert("Shock",playerMan.GetHealth("GlobalHealth", "Shock").ToString());
					dataMap.Insert("Water",PlayerBase.Cast(playerMan).GetStatWater().Get().ToString());
					dataMap.Insert("Energy",PlayerBase.Cast(playerMan).GetStatEnergy().Get().ToString());
					dataMap.Insert("Weapon",inHandsName);
					dataMap.Insert("SteamID",identity.GetPlainId());
					dataMap.Insert("UserGroup",GetPermissionManager().GetPlayerUserGrpNameByID(id));
		
					ref PlayerStatsData stats = new PlayerStatsData(dataMap);
					GetRPCManager().SendRPC( "RPC_MenuPlayerManager", "HandlePlayerStats", new Param1<ref PlayerStatsData>(stats), true, sender);
				}
			}
		}
	}
	
	void RequestInvisibility(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if(!ctx.Read(data)) return;
		
		if (type == CallType.Server && sender != null)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SetPlayerInvisible")) return;

			autoptr array<string> ids = data.param1;
			foreach(string id : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SetPlayerInvisible",id))
				{
					autoptr PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(id);
					if (pb != null)
					{
						if (pb.InvisibilityStatus())
						{
							if (m_InvisiblePlayers.Find(id) != -1)
							{
								m_InvisiblePlayers.Remove(m_InvisiblePlayers.Find(id));
								GetSimpleLogger().Log("[PlayerManager] RequestInvisibility(): Removing "+id+" from invis players array");
							}
							pb.setInvisibility(false);
							GetRPCManager().SendRPC( "RPC_HandleInvisiblity", "HandleInvisiblity", new Param1<bool>(false), true, pb.GetIdentity());
						}
						else
						{
							if (m_InvisiblePlayers.Find(id) == -1)
							{
								m_InvisiblePlayers.Insert(id);
								GetSimpleLogger().Log("[PlayerManager] RequestInvisibility(): Adding "+id+" to invis players array");
							}
							pb.setInvisibility(true);
							GetRPCManager().SendRPC( "RPC_HandleInvisiblity", "HandleInvisiblity", new Param1<bool>(true), true, pb.GetIdentity());
						}	
					}
				}
			}
		}
	}
	
	void GetPlayerStatsGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if(!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			ref array<string> UIDS = data.param1;
			if (sender != null)
			{
				if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuPlayerManager")) return;
				
				foreach(string id : UIDS)
				{
					autoptr Man playerMan = GetPermissionManager().GetPlayerBaseByID(id);
					if (playerMan != null && sender != null)
					{
						autoptr PlayerIdentity identity = playerMan.GetIdentity();
						if (identity != null)
						{
							string inHandsName = "";
					
							if(playerMan.GetHumanInventory() != null && playerMan.GetHumanInventory().GetEntityInHands() != null)
								inHandsName = playerMan.GetHumanInventory().GetEntityInHands().GetDisplayName();
							
							if(inHandsName == "")
								inHandsName = "None";
							
							autoptr map<string,string> dataMap = new map<string,string>;
							dataMap.Insert("Guid",identity.GetId());
							dataMap.Insert("Name",identity.GetName());
							dataMap.Insert("Blood",playerMan.GetHealth("GlobalHealth", "Blood").ToString());
							dataMap.Insert("Health",playerMan.GetHealth("GlobalHealth", "").ToString());
							dataMap.Insert("Shock",playerMan.GetHealth("GlobalHealth", "Shock").ToString());
							dataMap.Insert("Water",PlayerBase.Cast(playerMan).GetStatWater().Get().ToString());
							dataMap.Insert("Energy",PlayerBase.Cast(playerMan).GetStatEnergy().Get().ToString());
							dataMap.Insert("Weapon",inHandsName);
							dataMap.Insert("SteamID",identity.GetPlainId());
							dataMap.Insert("UserGroup",GetPermissionManager().GetPlayerUserGrpNameByID(id));
							
							ref PlayerStatsData stats = new PlayerStatsData(dataMap);
							GetRPCManager().SendRPC( "RPC_MenuPlayerManager", "HandlePlayerStats", new Param1<ref PlayerStatsData>(stats), true, sender);
						}
					}
				}
			}
		}
	}	
}