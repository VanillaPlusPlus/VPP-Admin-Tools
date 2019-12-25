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
        AddLocation("Severograd", "8428.0 0.0 12767.1" );
        AddLocation("Krasnostav", "11172.0 0.0 12314.1");
        AddLocation("Mogilevka", "7537.8 0.0 5211.55" );
        AddLocation("Stary", "6046.94 0.0 7733.97" );
        AddLocation("Msta", "11322.55 0.0 5463.36" );
        AddLocation("Vybor", "3784.16 0.0 8923.48");
        AddLocation("Gorka", "9514.27 0.0 8863.69");
        AddLocation("Solnichni", "13402.57 0.0 6303.35" );
        AddLocation("NWAF", "4823.43 0.0 10457.16" );
        AddLocation("Blota", "4467.61 0.0 2496.14" );
        AddLocation("NEAF", "11921.43 0.0 12525.55");
        AddLocation("Cherno", "6649.22 0.0 2710.03");
        AddLocation("Elektro", "10553.55 0.0 2313.37");
        AddLocation("Berez", "12319.54 0.0 9530.15");
        AddLocation("Svetlojarsk", "13900.82 0.0 13258.12");
        AddLocation("Zelenogorsk", "2572.80 0.0 5105.09" );
        AddLocation("Lopatino", "2714.74 0.0 9996.33");
        AddLocation("Tisy", "1723.10 0.0 13983.88" );
        AddLocation("Novaya", "3395.28 0.0 13013.61");
        AddLocation("Novy", "7085.73 0.0 7720.85");
        AddLocation("Grishino", "5952.15 0.0 10367.71");
        AddLocation("Kabanino", "5363.97 0.0 8594.39");
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
				townPos = g_Game.SnapToGround(location.GetLocation());
			}
		}
		
		if(townPos == "0 0 0")
		{	
			GetPermissionManager().NotifyPlayer(teleportee.GetIdentity().GetPlainId(), "Invalid Town Name.", NotifyTypes.ERROR);
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
			PlayerIdentity identity = broughtPlayer.GetIdentity();
			
			if(identity != null)
			{
				if(identity.GetPlainId() == id)
				{
					GetSimpleLogger().Log("[Teleport Manager]:: BringPlayer(): You cannot bring yourself.");
					GetPermissionManager().NotifyPlayer(id, "You cannot bring yourself.", NotifyTypes.ERROR);
					return;
				}
				
				m_ReturnVectors.Insert(identity.GetPlainId(), broughtPlayer.GetPosition());
				
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
	}
	
	void ReturnPlayer(Man returnedPlayer, string id)
	{
		if(returnedPlayer != null)
		{
			PlayerIdentity identity = returnedPlayer.GetIdentity();
			if(returnedPlayer.IsAlive() && identity != null)
			{
				vector returnVector = m_ReturnVectors.Get(identity.GetPlainId());
				
				if(returnVector == "0 0 0")
				{
					GetSimpleLogger().Log("[Teleport Manager]:: ReturnPlayer(): No return position available.");
					GetPermissionManager().NotifyPlayer(id, "No return position available.", NotifyTypes.ERROR);
					return;
				}
				
				returnedPlayer.SetPosition(returnVector);
				m_ReturnVectors.Remove(id);
			}
		}
	}
	
	void GotoPlayer(Man target, PlayerBase self, string id)
	{
		if(target != null && target.GetIdentity() != null)
		{
			if(target.GetIdentity().GetPlainId() == id)
			{
				GetPermissionManager().NotifyPlayer(id, "You cannot teleport to yourself", NotifyTypes.ERROR);
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
				GetPermissionManager().NotifyPlayer(id, "Invalid Position", NotifyTypes.ERROR);
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
	void GetPlayerPositions(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if( type == CallType.Server )
		{
			if (GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuTeleportManager"))
			{
				ref array<ref VPPPlayerData> m_List = new array<ref VPPPlayerData>;
				autoptr array<Man>   m_Players = new array<Man>;
				GetGame().GetWorld().GetPlayerList( m_Players );
				
				foreach(Man player : m_Players)
					if(player != null)
						m_List.Insert( new VPPPlayerData(player.GetIdentity().GetName(), player.GetPosition()) );
				
				GetRPCManager().SendRPC( "RPC_MenuTeleportManager", "UpdateMap", new Param1<ref array<ref VPPPlayerData>>( m_List ), true, sender);
			}
		}
	}
	
	void GetData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuTeleportManager")) return;
			
			Load();
			GetRPCManager().SendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true);
		}
	}
	
	void RemoteTeleportPlayers(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param3<ref array<string>,string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:TPPlayers")) return;
			
			autoptr array<string> ids = data.param1;

			if (ids.Count() == 1 && ids[0] == "self" && GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:TPSelf"))
			{
				string logMessage = "[TeleportManager]:: TeleportRemote(): [" + sender.GetName() + " : " + sender.GetPlainId() + "] teleported to ";
				if (data.param3 != Vector(0,0,0) && data.param2 == "")
				{
					TeleportToPoint({data.param3[0].ToString(),data.param3[1].ToString(),data.param3[2].ToString()}, GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId()), sender.GetPlainId());
					logMessage = logMessage + data.param3[0].ToString() + "," + data.param3[1].ToString() + "," + data.param3[2].ToString();
				}
				else
				{
					TeleportToTown(data.param2, GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId()));
					logMessage = logMessage + data.param2;
				}
				
				GetSimpleLogger().Log(logMessage);
				return;
			}
			
			foreach(string id : ids)
			{
				if(GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:TPPlayers"))
				{
					PlayerBase targetPlayer = GetPermissionManager().GetPlayerBaseByID(id);
					
					if (data.param3 != Vector(0,0,0) && data.param2 == "")
						TeleportToPoint({data.param3[0].ToString(),data.param3[1].ToString(),data.param3[2].ToString()}, targetPlayer, sender.GetPlainId());
					else
						TeleportToTown(data.param2, targetPlayer);
					
					GetSimpleLogger().Log("[TeleportManager]:: TeleportRemote(): [" + sender.GetName() + " : " + sender.GetPlainId() + "]  targeted [" + targetPlayer.GetIdentity().GetName() + " : " + targetPlayer.GetIdentity().GetPlainId() + "] with teleport.");
				}
			}
		}
	}
	
	void RemoteRemovePreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:DeletePreset")) return;
			
			autoptr array<string> presetNames = data.param1;
			int totalDeleted = 0;
			foreach(string toDelete : presetNames)
			{
				foreach(ref VPPTeleportLocation location : m_TeleportLocations)
				{
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
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Sucessfully deleted "+totalDeleted+" saved teleport presets!",NotifyTypes.NOTIFY);
				Save();
				//send updated list to client
				GetRPCManager().SendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true);
			}
		}
	}
	
	void AddNewPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:AddNewPreset")) return;
			AddLocation(data.param1, data.param2);
			Save();
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Sucessfully added "+data.param1+" preset!",NotifyTypes.NOTIFY);
			//send updated list to client
			GetRPCManager().SendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true);
		}
	}
	
	void EditPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<string,vector,string,vector> data;
		if(type == CallType.Server)
		{
			if (!ctx.Read(data)) return;
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportManager:EditPreset")) return;
			
			bool success = false;
			foreach(ref VPPTeleportLocation location : m_TeleportLocations)
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
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Sucessfully edited "+data.param1+" preset!",NotifyTypes.NOTIFY);
				//send updated list to client
				GetRPCManager().SendRPC("RPC_MenuTeleportManager", "HandleData", new Param1<ref array<ref VPPTeleportLocation>>(m_TeleportLocations), true);
			}else{
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Unable to edited "+data.param1+" preset, something went wrong!",NotifyTypes.ERROR);
			}
		}
	}
}

/* Global getter */
TeleportManager GetTeleportManager()
{
	return TeleportManager.Cast(GetPluginManager().GetPluginByType(TeleportManager));
}
