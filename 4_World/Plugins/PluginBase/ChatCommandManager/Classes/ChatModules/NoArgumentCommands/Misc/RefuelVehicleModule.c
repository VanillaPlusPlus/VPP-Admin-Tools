class RefuelVehicleChatModule extends ChatCommand
{	
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
    	PlayerIdentity identity = caller.GetIdentity();

        if(identity == null) return;

        string callerID;
        string callerName;
        callerID = identity.GetPlainId();
        callerName = identity.GetName();
    
    	Car vehicle = Car.Cast(caller.GetCommand_Vehicle().GetTransport());
    	EntityAI carEntity = vehicle;
    	if ( vehicle == NULL || carEntity == NULL)
    		return;

    	//Health
    	ref array<string> damageZones;
    	string cfgPath = CFG_VEHICLESPATH + " " + vehicle.GetType() + " DamageSystem";
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
		carEntity.SetHealth("EngineBelt", "Health", carEntity.GetMaxHealth());

		if ( damageZones && damageZones.Count() > 0 )
		{
			foreach(string dmgZone: damageZones)
			{
				carEntity.SetHealth(dmgZone, "",carEntity.GetMaxHealth(dmgZone, ""));
			}
		}

		ref array<EntityAI> vehParts = new array<EntityAI>;
		TStringArray SlotNames = new TStringArray;
		string cfg_path = CFG_VEHICLESPATH + " " + vehicle.GetType() + " attachments";
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
		
		//Liquids
		float fuelReq = vehicle.GetFluidCapacity( CarFluid.FUEL ) - (vehicle.GetFluidCapacity( CarFluid.FUEL ) * vehicle.GetFluidFraction( CarFluid.FUEL ));
		float oilReq = vehicle.GetFluidCapacity( CarFluid.OIL ) - (vehicle.GetFluidCapacity( CarFluid.OIL ) * vehicle.GetFluidFraction( CarFluid.OIL ));
		float coolantReq = vehicle.GetFluidCapacity( CarFluid.COOLANT ) - (vehicle.GetFluidCapacity( CarFluid.COOLANT ) * vehicle.GetFluidFraction( CarFluid.COOLANT ));
		float brakeReq = vehicle.GetFluidCapacity( CarFluid.BRAKE ) - (vehicle.GetFluidCapacity( CarFluid.BRAKE ) * vehicle.GetFluidFraction( CarFluid.BRAKE ));
		vehicle.Fill( CarFluid.FUEL, fuelReq );
		vehicle.Fill( CarFluid.OIL, oilReq );
		vehicle.Fill( CarFluid.COOLANT, coolantReq );
		vehicle.Fill( CarFluid.BRAKE, brakeReq );
		vehicle.SetSynchDirty();
		vehicle.Synchronize();

		GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /refuel used on self.", callerName, callerID));
    	GetPermissionManager().NotifyPlayer(callerID, vehicle.Type().ToString() + ": reparied & refueled [" +fuelReq+ "L] added, all fluids maxed",NotifyTypes.NOTIFY);
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /refuel used on self."));
    }
}