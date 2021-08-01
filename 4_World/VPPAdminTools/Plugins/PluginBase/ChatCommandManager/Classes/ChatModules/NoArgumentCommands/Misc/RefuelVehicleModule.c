class RefuelVehicleChatModule extends ChatCommand
{	
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();
    
    	Car vehicle = Car.Cast(caller.GetCommand_Vehicle().GetTransport());
    	EntityAI carEntity = vehicle;
    	if ( vehicle == NULL || carEntity == NULL)
    		return;
		
		//Attachments
		dBodyApplyImpulse(carEntity, vector.Up);
		carEntity.OnDebugSpawn();
		
		string childClass;
		string cfgPath = CFG_VEHICLESPATH + " " + vehicle.GetType() + " attachments";
		
    	//Health
    	array<string> damageZones;
    	cfgPath = CFG_VEHICLESPATH + " " + vehicle.GetType() + " DamageSystem";
    	if (GetGame().ConfigIsExisting(cfgPath))
    	{
    		damageZones = new array<string>;
    		int dmgZoneCount = GetGame().ConfigGetChildrenCount(cfgPath);
    		if ( dmgZoneCount > 0 )
    		{
    			for (int i = 0; i < dmgZoneCount; ++i)
    			{
    				GetGame().ConfigGetChildName(cfgPath, i, childClass);
    				childClass.ToLower();
    				if ( childClass == "damagezones" )
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
				carEntity.SetHealthMax(dmgZone);
				//carEntity.SetHealth(dmgZone, "",carEntity.GetMaxHealth(dmgZone, ""));
			}
		}

		array<EntityAI> vehParts = new array<EntityAI>;
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
					if (partType.Contains("_ruined"))
					{
						partType.Replace("_ruined", "");
						GetGame().ObjectDelete(att);
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

		carEntity.SetSynchDirty();
		GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /refuel used on self.", callerName, callerID));
    	GetPermissionManager().NotifyPlayer(callerID, vehicle.Type().ToString() + ": reparied & refueled added, all fluids maxed",NotifyTypes.NOTIFY);
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /refuel used on self."));
    }
}