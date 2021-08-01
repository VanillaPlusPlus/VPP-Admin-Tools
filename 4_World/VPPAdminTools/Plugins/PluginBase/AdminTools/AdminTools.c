class AdminTools extends PluginBase
{
	void AdminTools()
	{
		//---RPC's-----
		//GetRPCManager().AddRPC( "RPC_AdminTools", "AttachTo", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "DeleteObject", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "TeleportToPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "ToggleFreeCam", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "RepairVehicles", this, SingeplayerExecutionType.Server );
		//-------------
	}

    void DeleteObject(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "DeleteObjectAtCrosshair")) return;
            
            if (target)
            {
            	GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Deleted Object @ crosshair: "  + target.GetType()));
                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) deleted object \"%3\" (pos=%4)", sender.GetName(), sender.GetPlainId(), target.GetType(), target.GetPosition().ToString()));
                GetGame().ObjectDelete(target);
            }
        }
    }
	
    void AttachTo(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
	    Param1< Object > data; //Caries player
	    if ( !ctx.Read( data ) &&  !data.param1) return;
		
	    auto relativeLocalPos = target.CoordToLocal( data.param1.GetPosition() );
	    auto playerObj = PlayerBase.Cast( data.param1 );
	    if( playerObj ){
	        vector pLocalSpaceMatrix[ 4 ];
	        pLocalSpaceMatrix[ 3 ] = relativeLocalPos;
	        playerObj.LinkToLocalSpaceOf( target, pLocalSpaceMatrix );
	        return;
	    }
	    //Attach other non player objects using AddChild
	    data.param1.SetPosition( relativeLocalPos );
	    target.AddChild( data.param1, -1, false );
	}

	void TeleportToPosition( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<vector> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "TeleportToCrosshair")) return;
			PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			if (pb == null) return;
			
			
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
	
	void ToggleFreeCam(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server && sender != null)
        {
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "FreeCamera")) return;
			
			GetRPCManager().SendRPC( "RPC_HandleFreeCam", "HandleFreeCam", new Param1<bool>(true), true, sender);
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) toggled freecam", sender.GetName(), sender.GetPlainId()));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[AdminTools] Toggled Freecam"));
		}
	}

	void RepairVehicles(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Server && sender != null)
        {
        	if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "RepairVehiclesAtCrosshair"))
        		return;

	        string callerID;
	        string callerName;
            callerID = sender.GetPlainId();
            callerName = sender.GetName();
        
        	Car vehicle = Car.Cast( target );
        	EntityAI carEntity = EntityAI.Cast(target);
        	if ( vehicle == NULL || carEntity == NULL)
        		return;

        	//Attachments
			dBodyApplyImpulse(carEntity, vector.Up);
			carEntity.OnDebugSpawn();

        	//Health
        	array<string> damageZones;
        	string cfgPath = CFG_VEHICLESPATH + " " + target.GetType() + " DamageSystem";
        	if ( GetGame().ConfigIsExisting(cfgPath) )
        	{
        		damageZones = new array<string>;
        		int dmgZoneCount = GetGame().ConfigGetChildrenCount(cfgPath);
        		if ( dmgZoneCount > 0 )
        		{
        			for (int i = 0; i < dmgZoneCount; ++i)
        			{
        				string parentClass;
        				GetGame().ConfigGetChildName(cfgPath, i, parentClass);
        				parentClass.ToLower();
        				if ( parentClass == "damagezones" )
        				{
        					int dmgZoneIndex = GetGame().ConfigGetChildrenCount(cfgPath + " DamageZones");
        					for (int j = 0; j < dmgZoneIndex; ++j)
        					{
        						string childZone;
        						GetGame().ConfigGetChildName(cfgPath + " DamageZones", j, childZone);
        						damageZones.Insert( childZone );
        					}
        				}
        			}
        		}
        	}

			carEntity.AddHealth(carEntity.GetMaxHealth());
			//carEntity.SetHealth("EngineBelt", "Health", carEntity.GetMaxHealth());

			if ( damageZones && damageZones.Count() > 0 )
			{
				foreach(string dmgZone: damageZones)
				{
					carEntity.SetHealthMax(dmgZone);
				}
			}

			array<EntityAI> vehParts = new array<EntityAI>;
			TStringArray SlotNames = new TStringArray;
			string cfg_path = CFG_VEHICLESPATH + " " + target.GetType() + " attachments";
			GetGame().ConfigGetTextArray(cfg_path, SlotNames);	
			
			foreach(string carSlot : SlotNames)
					vehParts.Insert(carEntity.FindAttachmentBySlotName(carSlot));
		
			if (vehParts != null)
			{
				foreach(EntityAI att : vehParts)
				{
					if (att != NULL)
					{
						string partType = att.GetType();
						partType.ToLower();
						if ( partType.Contains("_ruined") )
						{
							partType.Replace("_ruined", "");
							GetGame().ObjectDelete( att );
							vehicle.GetInventory().CreateAttachment(partType);
						}
						else
						{
							att.AddHealth(att.GetMaxHealth());
							att.SetSynchDirty();
						}
					}
				}
			}
			
			GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) reparied & refueled vehicle (pos=%3)", callerName, callerID, vehicle.GetPosition().ToString()));
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, string.Format("\"%1\" (steamid=%2) reparied & refueled vehicle (pos=%3)", callerName, callerID, vehicle.GetPosition().ToString())));
        	GetPermissionManager().NotifyPlayer(callerID, vehicle.Type().ToString() + ": at crosshairs reparied & refueled, all fluids maxed",NotifyTypes.NOTIFY);
        }
	}
};