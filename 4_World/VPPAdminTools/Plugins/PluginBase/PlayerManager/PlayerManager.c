class PlayerManager extends PluginBase
{
	private ref Timer m_RPCDelay;
	
	void PlayerManager()
	{
		/* RPCs */
		GetRPCManager().AddRPC("RPC_PlayerManager", "GetPlayerStats", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "MakePlayerVomit", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "GetPlayerStatsGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "RequestInvisibility", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "ToggleGodmode", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "SendMessage", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "SetPlayerStats", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "HealPlayers", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "KillPlayers", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "SpectatePlayer", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "TeleportHandle", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "KickPlayer", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "BanPlayer", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "GiveGodmode", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "GiveUnlimitedAmmo", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "FreezePlayers", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "ChangePlayerScale", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PlayerManager", "GetPlayerCount", this, SingeplayerExecutionType.Server);
	}
	
	void GetPlayerCount(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type != CallType.Server)
			return;
	#ifndef DIAG_DEVELOPER
		if(sender == null)
			return;
	#endif
		if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuPlayerManager"))
		{
			GetRPCManager().VSendRPC( "RPC_MenuPlayerManager", "SetPlayerCount", new Param1<string>(GetSteamAPIManager().GetTotalPlayerCount()), true, sender);
		}
	}

	void ChangePlayerScale(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type != CallType.Server)
			return;

		Param2<ref array<string>, float> data;
		if(!ctx.Read(data))
			return;
	#ifndef DIAG_DEVELOPER
		if(sender == null)
			return;
	#endif
		array<string> ids = data.param1;
		foreach(string targetID : ids)
		{
			if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:ChangeScale",targetID))
			{
				PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(targetID);
				if(targetPlayer)
				{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Player "+targetPlayer.VPlayerGetName()+" changed scale to " + data.param2.ToString(), NotifyTypes.NOTIFY);
					targetPlayer.m_VScalePlayer = data.param2 != 1.0;
					targetPlayer.VPPSetScaleValue(data.param2);
					targetPlayer.SetSynchDirty();
				}
			}
		}
	}

	void MakePlayerVomit(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type != CallType.Server) return;

		Param2<ref array<string>, int> data;

		if(!ctx.Read(data)) return;
	#ifndef DIAG_DEVELOPER
		if(sender == null) return;
	#endif
		
		array<string> ids = data.param1;

		foreach(string targetID : ids)
		{
			if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:MakePlayerVomit",targetID))
			{
				PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(targetID);

				if(targetPlayer != null && !targetPlayer.GetCommand_Vehicle())
				{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Player "+targetPlayer.GetName()+" is vomiting for " + data.param2.ToString() + " seconds!", NotifyTypes.NOTIFY);
					SymptomBase vomitSymptom = targetPlayer.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_VOMIT);

					if(vomitSymptom != null)
					{
						vomitSymptom.SetDuration(data.param2);
					}
				}
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
			
			array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:BanPlayer",tgId))
				{
					BanDuration banDuration = GetBansManager().GetCurrentTimeStamp();
					string banAuthorDetails = string.Format("%1|%2",sender.GetName(),sender.GetPlainId());
					banDuration.Permanent = true;
					if (GetBansManager().AddToBanList(new BannedPlayer(GetPermissionManager().GetPlayerBaseByID(tgId).VPlayerGetName(), tgId, GetPermissionManager().GetPlayerBaseByID(tgId).VPlayerGetHashedId() ,banDuration,banAuthorDetails,"#VSTR_BAN_REASON "+sender.GetName())))
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Player "+GetPermissionManager().GetPlayerBaseByID(tgId).VPlayerGetName()+" #VSTR_BANNED",NotifyTypes.NOTIFY);
					else
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_ERROR_BAN_PLAYER "+GetPermissionManager().GetPlayerBaseByID(tgId).VPlayerGetName(),NotifyTypes.NOTIFY);

					GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) banned player: (steamid=%3)", sender.GetName(), sender.GetPlainId(), ids));
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
			array<string> ids = data.param1;
			foreach(string tgId : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KickPlayer",tgId))
				{
					PlayerBase tg = GetPermissionManager().GetPlayerBaseByID(tgId);
					if (tg != null)
					{
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_KICK_PLAYER"+tg.VPlayerGetName(),NotifyTypes.NOTIFY);
						GetRPCManager().VSendRPC( "RPC_MissionGameplay", "KickClientHandle", new Param1<string>( data.param2 ), true, tg.GetIdentity());
						GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) kicked player: \"%3\"", sender.GetName(), sender.GetPlainId(), tg.VPlayerGetName()));
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
        	Param2<VPPAT_TeleportType,ref array<string>> data; //opertation type,targets
        	if (!ctx.Read(data)) return;
			
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportToPlayer") || !GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportPlayerTo")) return;
			
			Man self = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			array<string> ids = data.param2;
			foreach(string targetId: ids)
			{
				switch(data.param1)
				{
					case VPPAT_TeleportType.GOTO:
						GetTeleportManager().GotoPlayer(GetPermissionManager().GetPlayerBaseByID(targetId), PlayerBase.Cast(self), sender.GetPlainId());
					break;

					case VPPAT_TeleportType.BRING:
						if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:TeleportPlayerTo",targetId))
							return;
					
						GetTeleportManager().BringPlayer(GetPermissionManager().GetPlayerBaseByID(targetId), self.GetPosition(), "");
					break;

					case VPPAT_TeleportType.RETURN:
						GetTeleportManager().ReturnPlayer(GetPermissionManager().GetPlayerBaseByID(targetId), sender.GetPlainId());
					break;
				}
			}
		}
	}
	
	void SpectatePlayer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
        {
        	Param1<string> data; //target id
       	 	if ( !ctx.Read( data ) ) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SpectatePlayer")) return;
			
			if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:SpectatePlayer",data.param1))
			{
				if (sender.GetPlainId() == data.param1)
				{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"You can't spectate your self!",NotifyTypes.ERROR);
					return;
				}
				
				PlayerBase tgp = GetPermissionManager().GetPlayerBaseByID(data.param1);
				if (tgp != null)
				{
					PlayerBase adminPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
					if (adminPlayer != null)
						adminPlayer.SetPosition(Vector(tgp.GetPosition()[0],-120,tgp.GetPosition()[2]));

					m_RPCDelay = new Timer();
					m_RPCDelay.Run(2.0,this,"InvokeSpectate", new Param2<string,string>(data.param1,sender.GetPlainId()),false);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_SPECTATE",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Started to spectate player: "+ data.param1));
				}

				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) started to spectate player: \"%3\"", sender.GetName(), sender.GetPlainId(), data.param1));
			}
		}
	}
	
	void InvokeSpectate(string id, string adminID)
	{
		PlayerBase spectateTarget = GetPermissionManager().GetPlayerBaseByID(id);
		PlayerBase adminPlayer    = GetPermissionManager().GetPlayerBaseByID(adminID);
		if (spectateTarget == null) return;

		if (adminPlayer != null){
			GetGame().ObjectDelete(adminPlayer);
			GetRPCManager().VSendRPC( "RPC_MenuPlayerManager", "InitSpectate", new Param1<Object>(spectateTarget), true, adminPlayer.GetIdentity() );
		}
	}
	
	void KillPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server)
        {
        	Param1<ref array<string>> data;
        	if ( !ctx.Read( data ) ) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KillPlayers")) return;
			
			array<string> ids = data.param1;
			if (ids.Count() < 1) return;
			
			foreach(string id : ids)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(),"PlayerManager:KillPlayers",id))
				{
					PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					if (targetPlayer != null)
					{
						targetPlayer.SetHealth(0);
						GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) executed kill command on: \"%3\"", sender.GetName(), sender.GetPlainId(), id));
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

        	array<string> pGUID = new array<string>;
        	pGUID.Copy(data.param3);
        	if (sender != NULL)
        	{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SendMessage"))
				{
					for (int i = 0; i < pGUID.Count(); ++i)
	    		    {
	    		    	PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(pGUID.Get(i));
	    		    	if (pb != NULL)
	    		    	{
	    		    		//Global, Identity ,Title, Message, Duration, FadeIn Time, Force show, DoFadeIn , Imagepath, Size X, Size Y
	                        g_Game.SendMessage(
	                         false,
	                         pb.GetIdentity(),
	                         data.param1,
	                         data.param2,
	                         10,
	                         2,
	                         false,
	                         true,
	                         "",
	                         0,
	                         0);
	                        GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) sent message (%3) to: (steamid=%4)", sender.GetName(), sender.GetPlainId(), data.param2,pGUID.Get(i)));
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
				PlayerBase AdminPlayer = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			
				if(AdminPlayer)
				{
					if (AdminPlayer.GodModeStatus())
                  	GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_GODMODE_OFF",NotifyTypes.NOTIFY);
	    			else
	                GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_GODMODE_ON",NotifyTypes.NOTIFY);
	    			
					AdminPlayer.setGodMode(!AdminPlayer.GodModeStatus());
					GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) toggled godmode", sender.GetName(), sender.GetPlainId()));
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
			
			PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				if (TargetPlayer.GodModeStatus())
				{
					GetPermissionManager().NotifyPlayer(data.param1,"#VSTR_NOTIFY_REVOKE_GODMODE",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_REVOKE_GODMODE_ADMIN",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Revoked Godmode status from player: "+ TargetPlayer.VPlayerGetName() + " ID: " + data.param1));
				}
		    	else
		    	{
					GetPermissionManager().NotifyPlayer(data.param1,"#VSTR_NOTIFY_GIVE_GODMODE",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_GIVE_GODMODE_ADMIN",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Gave Godmode status to player: "+ TargetPlayer.VPlayerGetName() + " ID: " + data.param1));
				}
				
				TargetPlayer.setGodMode(!TargetPlayer.GodModeStatus());
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) gave godmode to (steamid=%3)", sender.GetName(), sender.GetPlainId(), data.param1));
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
			PlayerBase TargetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param1);
			if(TargetPlayer)
			{
				if (TargetPlayer.VPPIsUnlimitedAmmo())
				{
					GetPermissionManager().NotifyPlayer(data.param1,"#VSTR_NOTIFY_REVOKE_AMMO",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_REVOKE_AMMO_ADMIN",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Revoked unlimited ammo status from player: "+ TargetPlayer.VPlayerGetName() + " ID: " + data.param1));
				}else{
					GetPermissionManager().NotifyPlayer(data.param1,"#VSTR_NOTIFY_GIVE_AMMO",NotifyTypes.NOTIFY);
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_NOTIFY_GIVE_AMMO_ADMIN",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Gave unlimited ammo status to player: "+ TargetPlayer.VPlayerGetName() + " ID: " + data.param1));
				}
				
				TargetPlayer.VPPSetUnlimitedAmmo(!TargetPlayer.VPPIsUnlimitedAmmo());
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) gave unlimited ammo to (steamid=%3)", sender.GetName(), sender.GetPlainId(), data.param1));
			}
		}
	}
	
	void FreezePlayers( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if( type == CallType.Server )
        {
        	Param1<ref array<string>> data; // player id's
			if(!ctx.Read(data)) return;

		#ifndef DIAG_DEVELOPER
			if (sender == null)
				return;
		#endif

			array<string> ids = data.param1;
			if (ids.Count() < 1) return;

			string adminID  = sender.GetPlainId();

			if (!GetPermissionManager().VerifyPermission(adminID, "PlayerManager:FreezePlayers"))
				return;

			foreach(string id : ids)
			{
				if (GetPermissionManager().VerifyPermission(adminID, "PlayerManager:FreezePlayers", id))
				{
					PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					if (targetPlayer != NULL)
					{
						targetPlayer.VPPFreezePlayer( !targetPlayer.VPPIsFreezeControls() );

						if ( targetPlayer.VPPIsFreezeControls() )
							GetPermissionManager().NotifyPlayer(sender.GetPlainId(), string.Format("Freeze Controls Set to [ TRUE ] for [%1] player(s)", ids.Count()), NotifyTypes.NOTIFY);
						else
							GetPermissionManager().NotifyPlayer(sender.GetPlainId(), string.Format("Freeze Controls Set to [ FALSE ] for [%1] player(s)", ids.Count()), NotifyTypes.NOTIFY);

						GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Froze player controls (steamid=%3)", sender.GetName(), sender.GetPlainId(), id));
					}
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Freezing "+ ids.Count() +" Player(s) control"));
        }
	}
	
	void HealPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			Param1<ref array<string>> data; // player id's
			if(!ctx.Read(data)) return;
		
		#ifndef DIAG_DEVELOPER
			if (sender == null) return;
		#endif
			
			string adminID  = sender.GetPlainId();
			if (!GetPermissionManager().VerifyPermission(adminID, "PlayerManager:HealPlayers")) return;
			
			array<string> ids = data.param1;
			if (ids.Count() < 1) return;
			
			foreach(string id : ids)
			{
				PlayerBase targetPlayer;
				if (GetPermissionManager().VerifyPermission(adminID, "PlayerManager:HealPlayers", id))
				{
					targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					if (targetPlayer != null)
					{
						targetPlayer.SetHealth(targetPlayer.GetMaxHealth("", ""));
						targetPlayer.SetHealth("","Blood", targetPlayer.GetMaxHealth("","Blood"));
						targetPlayer.SetHealth("","Shock", targetPlayer.GetMaxHealth("","Shock"));
						targetPlayer.GetStatHeatComfort().Set(targetPlayer.GetStatHeatComfort().GetMax());
						targetPlayer.GetStatTremor().Set(targetPlayer.GetStatTremor().GetMin());
						targetPlayer.GetStatWet().Set(targetPlayer.GetStatWet().GetMin());
						targetPlayer.GetStatEnergy().Set(targetPlayer.GetStatEnergy().GetMax());
						targetPlayer.GetStatWater().Set(targetPlayer.GetStatWater().GetMax());
						targetPlayer.GetStatDiet().Set(targetPlayer.GetStatDiet().GetMax());
						targetPlayer.GetStatSpecialty().Set(targetPlayer.GetStatSpecialty().GetMax());
						targetPlayer.GetStatHeatBuffer().Set(targetPlayer.GetStatHeatBuffer().GetMax());
						
						targetPlayer.HealBrokenLegs();
						if (targetPlayer.GetBleedingManagerServer())
		                {
		                    int attempts = 0; //fail safe, so we dont get stuck lol
		                    int cuts = targetPlayer.GetBleedingManagerServer().m_BleedingSources.Count();

		                    while(cuts > 0)
		                    {
		                        attempts++;
		                        if (attempts > 15)
		                            return;

		                        if (targetPlayer.GetBleedingManagerServer())
		                        {
		                        	int bit = targetPlayer.GetBleedingManagerServer().GetMostSignificantBleedingSource();
									if(bit != 0)
									{
		                            	targetPlayer.GetBleedingManagerServer().RemoveBleedingSourceNonInfectious(bit);
		                            	cuts--;
									}
		                        }
		                    }
		                }
						GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) healed player (steamid=%3)", sender.GetName(), sender.GetPlainId(), id));
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
			
			PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(data.param2);
			if (targetPlayer == null) return;
			
			string targetPlayerName = targetPlayer.VPlayerGetName();
			string targetPlayerId   = targetPlayer.VPlayerGetSteamId();

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
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) just updated a health stat on (steamid=%3)", sender.GetName(), sender.GetPlainId(), data.param2));

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
			PlayerBase playerMan = GetPermissionManager().GetPlayerBaseByID(id);
			if (playerMan != null && sender != null)
			{
				string inHandsName = "";
				
				if(playerMan.GetHumanInventory() != null && playerMan.GetHumanInventory().GetEntityInHands() != null)
					inHandsName = playerMan.GetHumanInventory().GetEntityInHands().GetDisplayName();
			
				if(inHandsName == "")
					inHandsName = "None";

				map<string,string> dataMap = new map<string,string>;
				dataMap.Insert("Guid",playerMan.VPlayerGetHashedId());
				dataMap.Insert("Name",playerMan.VPlayerGetName());
				dataMap.Insert("Blood",playerMan.GetHealth("GlobalHealth", "Blood").ToString());
				dataMap.Insert("Health",playerMan.GetHealth("GlobalHealth", "").ToString());
				dataMap.Insert("Shock",playerMan.GetHealth("GlobalHealth", "Shock").ToString());
				dataMap.Insert("Water",PlayerBase.Cast(playerMan).GetStatWater().Get().ToString());
				dataMap.Insert("Energy",PlayerBase.Cast(playerMan).GetStatEnergy().Get().ToString());
				dataMap.Insert("Weapon",inHandsName);
				dataMap.Insert("SteamID",playerMan.VPlayerGetSteamId());
				dataMap.Insert("UserGroup",GetPermissionManager().GetPlayerUserGrpNameByID(id));
	
				PlayerStatsData stats = new PlayerStatsData(dataMap);
				GetRPCManager().VSendRPC( "RPC_MenuPlayerManager", "HandlePlayerStats", new Param1<ref PlayerStatsData>(stats), true, sender);
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] GetPlayerStats Target ID: " + id + " Name: " + playerMan.VPlayerGetSteamId()));
			}
		}
	}
	
	void RequestInvisibility(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender != NULL)
		{
			Param1<ref array<int>> data;
			if(!ctx.Read(data)) return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "PlayerManager:SetPlayerInvisible")) return;

			array<int> ids = data.param1;
			foreach(int id : ids)
			{
				PlayerBase pb = GetPermissionManager().GetPlayerBaseBySessionID(id);
				if (pb != NULL)
				{
					pb.VPPSetInvisibility(!pb.InvisibilityStatus());

					if (pb.InvisibilityStatus())
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(), string.Format("Invisibility Set to [ TRUE ] for [%1] player(s)", ids.Count()), NotifyTypes.NOTIFY);
					else
						GetPermissionManager().NotifyPlayer(sender.GetPlainId(), string.Format("Invisibility Set to [ FALSE ] for [%1] player(s)", ids.Count()), NotifyTypes.NOTIFY);
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Set Invisibility on player(s): " + ids.Count()));
		}
	}
	
	void GetPlayerStatsGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server)
		{
			Param1<ref array<string>> data;
			if(!ctx.Read(data)) return;

			array<string> UIDS = data.param1;
			int pcount = UIDS.Count();
			if (sender != null)
			{
				if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuPlayerManager", "", false)) return;
				
				foreach(string id : UIDS)
				{
					PlayerBase playerMan = GetPermissionManager().GetPlayerBaseByID(id);
					if (playerMan != null && sender != null)
					{
						string inHandsName = "";
				
						if(playerMan.GetHumanInventory() != null && playerMan.GetHumanInventory().GetEntityInHands() != null)
							inHandsName = playerMan.GetHumanInventory().GetEntityInHands().GetDisplayName();
						
						if(inHandsName == "")
							inHandsName = "None";
						
						map<string,string> dataMap = new map<string,string>;
						dataMap.Insert("Guid",playerMan.VPlayerGetHashedId());
						dataMap.Insert("Name",playerMan.VPlayerGetName());
						dataMap.Insert("Blood",playerMan.GetHealth("GlobalHealth", "Blood").ToString());
						dataMap.Insert("Health",playerMan.GetHealth("GlobalHealth", "").ToString());
						dataMap.Insert("Shock",playerMan.GetHealth("GlobalHealth", "Shock").ToString());
						dataMap.Insert("Water",PlayerBase.Cast(playerMan).GetStatWater().Get().ToString());
						dataMap.Insert("Energy",PlayerBase.Cast(playerMan).GetStatEnergy().Get().ToString());
						dataMap.Insert("Weapon",inHandsName);
						dataMap.Insert("SteamID",playerMan.VPlayerGetSteamId());
						dataMap.Insert("UserGroup",GetPermissionManager().GetPlayerUserGrpNameByID(id));
						
						PlayerStatsData stats = new PlayerStatsData(dataMap);
						GetRPCManager().VSendRPC( "RPC_MenuPlayerManager", "HandlePlayerStats", new Param1<ref PlayerStatsData>(stats), true, sender);
					}
				}
				if (pcount > 0)
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PlayerManager] Requested Stats for player(s): " + pcount));
			}
		}
	}	
}