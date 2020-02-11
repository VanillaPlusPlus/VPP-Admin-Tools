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
		if (type == CallType.Server)
        {
        	Param1<ref array<string>> data; //targets
        	if ( !ctx.Read( data ) ) return;
		
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:BanPlayer")) return;
			
			autoptr array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:BanPlayer",tgId))
				{
					ref PlayerIdentity pid = GetPermissionManager().GetIdentityById(tgId);
					autoptr BanDuration banDuration = GetBansManager().GetCurrentTimeStamp();
					string banAuthorDetails = string.Format("%1|%2",sender.GetName(),sender.GetPlainId());
					banDuration.Permanent = true;
					if (GetBansManager().AddToBanList(new BannedPlayer(pid.GetName(),tgId,pid.GetId(),banDuration,banAuthorDetails,"Banned By Server Admin "+sender.GetName())))
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Player "+pid.GetName()+" is now banned!",NotifyTypes.NOTIFY);
					else
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Unable to ban player "+pid.GetName(),NotifyTypes.NOTIFY);
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Banned " + ids.Count() + " player(s)"));
		}
	}
	
	void KickPlayer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
        {
        	Param2<ref array<string>,string> data; //targets, reason
        	if ( !ctx.Read( data ) ) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KickPlayer")) return;
			autoptr array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KickPlayer",tgId))
				{
					ref PlayerIdentity tempiden = GetPermissionManager().GetIdentityById(tgId);
					if (tempiden != null)
					{
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Kicking player: "+tempiden.GetName(),NotifyTypes.NOTIFY);
						GetRPCManager().SendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( data.param2 ), true, tempiden);
						GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] Kicked player [%3]",sender.GetPlainId(), sender.GetName(), tempiden.GetName()));
					}
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Kicked "+ ids.Count() +" player(s)"));
		}
	}
	
	void TeleportHandle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
        {
        	Param2<bool,ref array<string>> data; //opertation type,targets
        	if ( !ctx.Read( data ) ) return;
			
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
		if (type == CallType.Server)
        {
        	ref Param1<string> data; //target id
       	 	if ( !ctx.Read( data ) ) return;

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
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Started to spectate player: "+ data.param1));
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
			GetGame().ObjectDelete(adminPlayer);
			GetRPCManager().SendRPC( "RPC_MenuPlayerManager", "InitSpectate", new Param1<Object>(spectateTarget), true, adminPlayer.GetIdentity() );
		}
	}
	
	void KillPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server)
        {
        	Param1<ref array<string>> data;
        	if ( !ctx.Read( data ) ) return;

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
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Killing "+ ids.Count() +" Players"));
		}
	}
	
	void SendMessage( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
        if (type == CallType.Server)
        {
        	Param3<string,string,ref array<string>> data;
        	if ( !ctx.Read( data ) ) return;

        	ref array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param3);
        	if (sender != NULL)
        	{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SendMessage"))
				{
					for (int i = 0; i < pGUID.Count(); ++i)
	    		    {
	    		    	ref PlayerIdentity TargetPlayer =  GetPermissionManager().GetIdentityById(pGUID.Get(i));
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
	    		    GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Sent message "+ data.param2 +" to " + pGUID.Count() + " Players"));
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
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] toggled godmode"));
					}
		  	}
        }
	}
	
	void GiveGodmode( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if( type == CallType.Server )
        {
        	Param1<string> data;
			if (!ctx.Read(data)) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:GiveGodmode",data.param1)) return;
			
			autoptr PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				if (TargetPlayer.GodModeStatus()){
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Revoked your godmode",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Revoked selected players godmode!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Revoked Godmode status from player: "+ TargetPlayer.GetIdentity().GetName() + " ID: " + data.param1));
				}
		    	else{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Gave you GodMode",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Gave selected player godmode!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Gave Godmode status to player: "+ TargetPlayer.GetIdentity().GetName() + " ID: " + data.param1));
				}
				
				TargetPlayer.setGodMode(!TargetPlayer.GodModeStatus());
				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] gave godmode to [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	void GiveUnlimitedAmmo( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if( type == CallType.Server )
        {
        	Param1<string> data;
			if (!ctx.Read(data)) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:GiveUnlimitedAmmo",data.param1)) return;
			autoptr PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				string targetPlayerName = TargetPlayer.GetIdentity().GetName();
				if (TargetPlayer.IsUnlimitedAmmo())
				{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Revoked your unlimited ammo",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Revoked selected players unlimited ammo!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Revoked unlimited ammo status from player: "+ targetPlayerName + " ID: " + data.param1));
				}else{
					GetPermissionManager().NotifyPlayer(data.param1,"Admin Gave you Unlimited Ammo!",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You Gave selected player Unlimited Ammo!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Gave unlimited ammo status to player: "+ targetPlayerName + " ID: " + data.param1));
				}
				
				TargetPlayer.SetUnlimitedAmmo(!TargetPlayer.IsUnlimitedAmmo());
				GetSimpleLogger().Log(string.Format("Player Name[%1] GUID[%2] gave unlimited ammo to [%3]",sender.GetPlainId(), sender.GetName(), data.param1));
			}
		}
	}
	
	
	void HealPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			Param1<ref array<string>> data; // player id's
			if(!ctx.Read(data)) return;
		

			if (sender == null) return;
			
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
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Healing "+ ids.Count() +" Players"));
		}
	}

	/*
		Blood,Health,Shock,Water,Food
	*/
	void SetPlayerStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			Param3<float,string,string> data; //stat level, player id, stat type
			if(!ctx.Read(data)) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SetPlayerStats") || !GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SetPlayerStats",data.param2)) return;
			
			autoptr PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param2);
			if (targetPlayer == null) return;
			
			string targetPlayerName = targetPlayer.GetIdentity().GetName();
			string targetPlayerId = targetPlayer.GetIdentity().GetPlainId();

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
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Set Player stats on target: " + targetPlayerId + " Name: " + targetPlayerName));
		}
	}
		
	void GetPlayerStats(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			Param1<string> data;
			if(!ctx.Read(data)) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuPlayerManager", "", false)) return;
			
			string id = data.param1;
			autoptr Man playerMan = GetPermissionManager().GetPlayerBaseByID(id);
			if (playerMan != null && sender != null)
			{
				ref PlayerIdentity identity = playerMan.GetIdentity();
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
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] GetPlayerStats Target ID: " + id + " Name: " + identity.GetName()));
				}
			}
		}
	}
	
	void RequestInvisibility(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender != null)
		{
			Param1<ref array<string>> data;
			if(!ctx.Read(data)) return;

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
		if (type == CallType.Server)
		{
			Param1<ref array<string>> data;
			if(!ctx.Read(data)) return;

			ref array<string> UIDS = data.param1;
			int pcount = UIDS.Count();
			if (sender != null)
			{
				if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuPlayerManager", "", false)) return;
				
				foreach(string id : UIDS)
				{
					autoptr Man playerMan = GetPermissionManager().GetPlayerBaseByID(id);
					if (playerMan != null && sender != null)
					{
						ref PlayerIdentity identity = playerMan.GetIdentity();
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
				if (pcount > 0)
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Requested Stats for player(s): " + pcount));
			}
		}
	}	
}