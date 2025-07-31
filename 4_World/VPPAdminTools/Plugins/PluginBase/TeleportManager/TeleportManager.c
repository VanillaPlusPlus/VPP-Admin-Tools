class TeleportManager : ConfigurablePlugin
{
	private ref array<ref VPPTeleportLocation> m_TeleportLocations;
	
	[NonSerialized()]
	private ref map<string, vector> m_ReturnVectors;
	
	void TeleportManager()
	{
		/*RPCs*/
		GetRPCManager().AddRPC( "RPC_TeleportManager", "GetData", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "RemoteTeleportPlayers", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "RemoteRemovePreset", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "AddNewPreset", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "EditPreset", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "GetPlayerPositions", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "TeleportToPosition", this);
		GetRPCManager().AddRPC( "RPC_TeleportManager", "TeleportEntity", this);
		/*-----*/
		
		m_TeleportLocations = new array<ref VPPTeleportLocation>;
		m_ReturnVectors = new map<string, vector>;
		
		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/TeleportManager/TeleportLocation.json";
	}
	
	override void OnInit()
	{
		Load();
	}

	override void Save()
	{
		JsonFileLoader<TeleportManager>.JsonSaveFile(JSONPATH, this);
	}

	override void Load()
	{
		MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins/TeleportManager");
		if (FileExist(JSONPATH))
		{
			
			JsonFileLoader<TeleportManager>.JsonLoadFile(JSONPATH, this);
			return;
		}
		
		CreateDefaults();
		Save();
	}
	
	private void CreateDefaults()
	{
        AddLocation("Severograd", "8428.000000 106.752388 12767.099609" );
        AddLocation("Krasnostav", "11168.872070 198.915375 12316.125000");
        AddLocation("Mogilevka", "7537.799805 215.232330 5211.546387" );
        AddLocation("Stary", "6046.942383 301.413910 7733.970215" );
        AddLocation("Msta", "11322.547852 252.808182 5463.358887" );
        AddLocation("Vybor", "3784.159912 310.997986 8923.480469");
        AddLocation("Gorka", "9514.269531 303.805695 8863.692383");
        AddLocation("Solnichni", "13398.269531 5.952467 6303.471680" );
        AddLocation("NWAF", "4823.430176 339.054474 10457.160156" );
        AddLocation("Blota", "4467.609375 6.073610 2496.135498" );
        AddLocation("NEAF", "11921.429688 140.000000 12525.549805");
        AddLocation("Cherno", "6635.834961 13.351086 2745.285400");
        AddLocation("Elektro", "10553.549805 6.000000 2313.370117");
        AddLocation("Berez", "12319.942383 13.747149 9532.086914");
        AddLocation("Svetlojarsk", "13900.820313 19.006157 13258.120117");
        AddLocation("Zelenogorsk", "2572.800049 194.204575 5105.089844" );
        AddLocation("Lopatino", "2714.739990 273.989990 9996.330078");
        AddLocation("Tisy", "1700.161987 449.019745 13993.882813" );
        AddLocation("Novaya", "3395.287842 193.985077 13013.609375");
        AddLocation("Novy", "7085.729980 297.324524 7720.850098");
        AddLocation("Grishino", "5952.155762 267.564331 10367.708984");
        AddLocation("Kabanino", "5363.970215 331.655884 8594.389648");
		Save();
	}
	
	array<ref VPPTeleportLocation> GetTeleportLocations()
	{
		return m_TeleportLocations;
	}

	void AddLocation(string locationName, vector pos)
	{
		m_TeleportLocations.Insert(new VPPTeleportLocation(locationName, pos));
	}
	
	void TeleportToTown(string name, PlayerBase teleportee)
	{
		vector townPos;
		string lowerInput = name;
		name.ToLower();
		
		foreach(VPPTeleportLocation location : m_TeleportLocations)
		{
			string lowerTownName = location.GetName();
			lowerTownName.ToLower();
			if(lowerTownName == name)
			{
				//townPos = g_Game.SnapToGround(location.GetLocation());
				townPos = location.GetLocation();
				break;
			}
		}
		
		if(townPos == "0 0 0")
		{
			GetPermissionManager().NotifyPlayer(PlayerBase.Cast(teleportee).VPlayerGetSteamId(), "Invalid Town Name.", NotifyTypes.ERROR);
			return;
		}
		
		if(teleportee != null)
		{
			Transport veh = GetTransportVehicle(teleportee);
			if ( veh )
			{
				vector mat[4];
				veh.GetTransform(mat);
				mat[3] = townPos;
				veh.SetTransform(mat);
			}
			else
			{
				teleportee.SetPosition(townPos);
			}
		}
	}
	
	void BringPlayer(PlayerBase broughtPlayer, vector adminPos, string id)
	{
		if(broughtPlayer != null)
		{
			string broughtPlayerId = PlayerBase.Cast(broughtPlayer).VPlayerGetSteamId();
			
			if(broughtPlayerId == id)
			{
				GetSimpleLogger().Log("[Teleport Manager]:: BringPlayer(): You cannot bring yourself.");
				GetPermissionManager().NotifyPlayer(id, "#VSTR_NOTIFY_TP_BRINGSELF_ERR", NotifyTypes.ERROR);
				return;
			}
			
			m_ReturnVectors.Insert(broughtPlayerId, broughtPlayer.GetPosition());
			
			Transport veh = GetTransportVehicle(broughtPlayer);
			if ( veh )
			{
				vector mat[4];
				veh.GetTransform(mat);
				mat[3] = adminPos;
				veh.SetTransform(mat);
			}else{
				broughtPlayer.SetPosition(adminPos);
			}
		}
	}
	
	void ReturnPlayer(Man returnedPlayer, string id)
	{
		if(returnedPlayer != null)
		{
			string returnedPlayerId = PlayerBase.Cast(returnedPlayer).VPlayerGetSteamId();
			if(returnedPlayer.IsAlive())
			{
				vector returnVector = m_ReturnVectors.Get(returnedPlayerId);
				
				if(returnVector == "0 0 0")
				{
					GetPermissionManager().NotifyPlayer(id, "#VSTR_NOTIFY_ERR_TP_NORETURN", NotifyTypes.ERROR);
					return;
				}
				
				returnedPlayer.SetPosition(returnVector);
				m_ReturnVectors.Remove(id);
			}
		}
	}
	
	void GotoPlayer(Man target, PlayerBase self, string id)
	{
		if(target != null)
		{
			if(PlayerBase.Cast(target).VPlayerGetSteamId() == id)
			{
				GetPermissionManager().NotifyPlayer(id, "#VSTR_NOTIFY_ERR_TPSELF", NotifyTypes.ERROR);
				return;
			}
			
			m_ReturnVectors.Insert(id, self.GetPosition());
			
			Transport veh = GetTransportVehicle(self);
			if ( veh )
			{
				vector mat[4];
				veh.GetTransform(mat);
				mat[3] = target.GetPosition();
				veh.SetTransform(mat);
			}else{
				self.SetPosition(target.GetPosition());
			}
		}
	}
	
	void TeleportToPoint(array<string> args, PlayerBase caller, string id)
	{
		if(args.Count() == 3)
		{
			float x = args[0].ToFloat();
			float y = args[1].ToFloat();
			float z = args[2].ToFloat();
			
			if (y <= 0)
				y = GetGame().SurfaceY(x, z);

			if(x == 0 || z == 0)
			{
				GetPermissionManager().NotifyPlayer(id, "#VSTR_TP_ERR_INVALID", NotifyTypes.ERROR);
				return;
			}
			
			
			if(caller != null)
			{
				Transport veh = GetTransportVehicle(caller);
				if ( veh )
				{
					vector mat[4];
					veh.GetTransform(mat);
					mat[3] = Vector(x, y, z);
					veh.SetTransform(mat);
				}else{
					caller.SetPosition(Vector(x, y, z));
				}
				GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) teleported to (pos=%3)", caller.GetName(), id, Vector(x, y, z).ToString()));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(id, caller.GetName(), "Teleported to position: " + Vector(x, y, z).ToString()));
			}
		}
	}
	
	void RemoveLocationByName(string name)
	{
		foreach(VPPTeleportLocation tpPos : m_TeleportLocations)
		{
			if(tpPos.GetName() == name)
			{
				m_TeleportLocations.RemoveItem(tpPos);
				Save();
			}
		}
	}
	
	Transport GetTransportVehicle(PlayerBase player)
	{
		if ( player.GetCommand_Vehicle() )
		{
			Transport veh = player.GetCommand_Vehicle().GetTransport();
			if (veh != null)
			{
				return veh;
			}
		}
		return null;
	}
	
	/*
	 RPC Section
	*/
	void TeleportToPosition( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<vector> data;
        if (!ctx.Read(data)) return;

        if (type == CallType.Server)
        {
        	if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportToCrosshair")) return;
			PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			if (pb == null) return;
			
			m_ReturnVectors.Insert(sender.GetPlainId(), data.param1);

			if ( pb.GetCommand_Vehicle() )
			{
				Transport veh = pb.GetCommand_Vehicle().GetTransport();
				if (veh != null)
				{
					vector mat[4];
					veh.GetTransform(mat);
					mat[3] = data.param1;
					veh.SetTransform(mat);
				}
			}else{
				pb.SetPosition(data.param1);
			}
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) teleported to crosshair (pos=%3)", sender.GetName(), sender.GetPlainId(), data.param1.ToString()));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Teleported to crosshair @ position: " + data.param1));
        }
	}

	void GetPlayerPositions(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server )
		{
			if(sender != null)
			{
				if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:ViewPlayerPositions", "", false))
				{
					array<ref VPPPlayerData> m_List = new array<ref VPPPlayerData>;
					array<Man>   m_Players = new array<Man>;
					GetGame().GetWorld().GetPlayerList( m_Players );
					
					foreach(Man player : m_Players)
						if(player != null)
							m_List.Insert( new VPPPlayerData(PlayerBase.Cast(player).VPlayerGetName(), player.GetPosition()) );
					
					GetRPCManager().VSendRPC( "RPC_MenuTeleportManager", "UpdateMap", new Param1<ref array<ref VPPPlayerData>>( m_List ), true, sender);
				}
			}
		}
	}
	
	void GetData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if(sender != null)
			{
				if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuTeleportManager", "", false)) return;
				
				Load();
				GetRPCManager().VSendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true, sender);
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[TeleportManager] Sent Teleport Presets"));
			}
		}
	}
	
	void RemoteTeleportPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<ref array<string>,string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;

			if(sender != null)
			{	
				string senderId = sender.GetPlainId();
				if(!GetPermissionManager().VerifyPermission(senderId, "TeleportManager:TPPlayers")) return;

				array<string> ids = data.param1;

				if (ids.Count() == 1 && ids[0] == "self" && GetPermissionManager().VerifyPermission(senderId, "TeleportManager:TPSelf"))
				{
					string logMessage = "\"" + sender.GetName() + "\": (steamid=" + senderId + ") teleported to (pos=";
					if (data.param3 != Vector(0,0,0) && data.param2 == "")
					{
						TeleportToPoint({data.param3[0].ToString(),data.param3[1].ToString(),data.param3[2].ToString()}, GetPermissionManager().GetPlayerBaseByID(senderId), senderId);
						logMessage = logMessage + data.param3[0].ToString() + "," + data.param3[1].ToString() + "," + data.param3[2].ToString();
					}
					else
					{
						TeleportToTown(data.param2, GetPermissionManager().GetPlayerBaseByID(senderId));
						logMessage = logMessage + data.param2;
					}
					
					GetSimpleLogger().Log(logMessage + ")");
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(senderId, sender.GetName(), logMessage));
					return;
				}
				
				foreach(string id : ids)
				{
					if(GetPermissionManager().VerifyPermission(senderId, "TeleportManager:TPPlayers"))
					{
						PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
						
						if (data.param3 != Vector(0,0,0) && data.param2 == "")
							TeleportToPoint({data.param3[0].ToString(),data.param3[1].ToString(),data.param3[2].ToString()}, targetPlayer, senderId);
						else
							TeleportToTown(data.param2, targetPlayer);
						
						GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) Teleporting player: \"%3\" (steamid=%4) to (pos=%5)", sender.GetName(), sender.GetPlainId(), targetPlayer.VPlayerGetName(), targetPlayer.VPlayerGetSteamId(), data.param3.ToString()));
					}
				}
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(senderId, sender.GetName(), "[TeleportManager] Teleporting " + ids.Count() + " players to position: " + data.param3));
			}
		}
	}
	
	void RemoteRemovePreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;

			if(sender != null)
			{
				string senderId = sender.GetPlainId();
				if(!GetPermissionManager().VerifyPermission(senderId, "TeleportManager:DeletePreset")) return;
				
				array<string> presetNames = data.param1;
				int totalDeleted = 0;
				foreach(string toDelete : presetNames)
				{
					foreach(VPPTeleportLocation location : m_TeleportLocations)
					{
						if (!location)
							continue;
						
						if (location.GetName() == toDelete)
						{
							totalDeleted++;
							m_TeleportLocations.RemoveItem(location);
							delete location;
						}
					}
				}
				
				if (totalDeleted > 0)
				{
					GetPermissionManager().NotifyPlayer(senderId,"#VSTR_DEL_SUCESS"+totalDeleted+" saved teleport presets!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(senderId, sender.GetName(), "[TeleportManager] Deleted Preset(s): " + totalDeleted));
					Save();
					//send updated list to client
					GetRPCManager().VSendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true, sender);
				}
			}
		}
	}
	
	void AddNewPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;

			if(sender != null)
			{
				if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:AddNewPreset")) return;
				AddLocation(data.param1, data.param2);
				Save();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_ADD_SUCESS"+data.param1+" preset!",NotifyTypes.NOTIFY);
				//send updated list to client
				GetRPCManager().VSendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true, sender);
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[TeleportManager] Added new preset: " + data.param1 + " position: " + data.param2));
			}
		}
	}
	
	void EditPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<string,vector,string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;

			if(sender != null)
			{
				if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:EditPreset")) return;
				
				bool success = false;
				foreach(VPPTeleportLocation location : m_TeleportLocations)
				{
					if (location.GetName() == data.param1 && location.GetLocation() == data.param2)
					{
						location.SetName(data.param3);
						location.Setlocation(data.param4);
						success = true;
					}
				}
				
				if (success)
				{
					Save();
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"#VSTR_EDIT_SUCESS"+data.param1+" preset!",NotifyTypes.NOTIFY);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[TeleportManager] Edited Preset: " + data.param1));
					//send updated list to client
					GetRPCManager().VSendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true, sender);
				}else{
					GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Unable to edited "+data.param1+" preset, something went wrong!",NotifyTypes.ERROR);
				}
			}
		}
	}

	void TeleportEntity(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data))
				return;

			if (!sender)
				return;

			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:TeleportEntity"))
				return;

			if (target)
			{
				Transport veh;
				if (Class.CastTo(veh, target))
				{
					vector mat[4];
					veh.GetTransform(mat);
					mat[3] = data.param1;
					veh.SetTransform(mat);
					veh.SetPosition(data.param1);
					dBodyApplyImpulse(veh, vector.Up);
				}
				else
				{
					target.SetPosition(data.param1);
				}

				PlayerBase pb = PlayerBase.Cast(target);
				if (pb)
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ESP] Teleported Player: " + pb.VPlayerGetName() + " : " + pb.VPlayerGetSteamId() +" to position: " + data.param1.ToString()));
				else
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[ESP] Teleported an Entity: " + target + " to position: " + data.param1.ToString()));

				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Entity moved!", NotifyTypes.NOTIFY);	
			}
		}
	}
}

/* Global getter */
TeleportManager GetTeleportManager()
{
	return TeleportManager.Cast(GetPluginManager().GetPluginByType(TeleportManager));
}
