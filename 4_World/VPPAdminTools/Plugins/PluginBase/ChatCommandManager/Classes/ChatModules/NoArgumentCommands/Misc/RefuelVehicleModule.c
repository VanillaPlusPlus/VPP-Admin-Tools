class RefuelVehicleChatModule extends ChatCommand
{	
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(!caller)
        	return;
        if (!caller.GetCommand_Vehicle())
        	return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();
    
    	Car vehicle = Car.Cast(caller.GetCommand_Vehicle().GetTransport());
    	EntityAI carEntity = vehicle;
    	if ( vehicle == NULL || carEntity == NULL)
    		return;
		
		//Attachments
		dBodyApplyImpulse(carEntity, vector.Up);
		carEntity.SetHealthMax("", "Health");
		carEntity.SetHealthMax();
		CarScript.Cast(vehicle).RefillAllLiquids();

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

		if (damageZones && damageZones.Count() > 0)
		{
			foreach(string dmgZone: damageZones)
			{
				carEntity.SetHealthMax(dmgZone, "Health");
			}
		}

		//Repair existing attachments or create if missing
		TStringArray SlotNames = new TStringArray;
		string cfg_path = CFG_VEHICLESPATH + " " + vehicle.GetType() + " attachments";
		GetGame().ConfigGetTextArray(cfg_path, SlotNames);	
		
		foreach(string carSlot : SlotNames)
		{
			carSlot.ToLower();
			int slotId = InventorySlots.GetSlotIdFromString(carSlot);
			EntityAI attachment = vehicle.GetInventory().FindAttachment(slotId);
			if (!attachment)
			{
				string typeName = VPPATInventorySlots.SlotsItems[carSlot].GetRandomElement();
				typeName.ToLower();
				if (typeName.Contains("_ruined"))
					typeName = VPPATInventorySlots.SlotsItems[carSlot][0];

				vehicle.GetInventory().CreateAttachmentEx(typeName, slotId);
			}
			else
			{
				string partType = attachment.GetType();
				partType.ToLower();
				if (partType.Contains("_ruined"))
				{
					partType.Replace("_ruined", "");
					GetGame().ObjectDelete(attachment);
					vehicle.GetInventory().CreateInInventory(partType);
				}
				else
				{
					attachment.SetHealthMax("", "Health");
					attachment.SetSynchDirty();
				}
			}
		}
		carEntity.SetSynchDirty();
		GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /refuel used on self.", callerName, callerID));
    	GetPermissionManager().NotifyPlayer(callerID, vehicle.Type().ToString() + ": reparied & refueled added, all fluids maxed",NotifyTypes.NOTIFY);
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /refuel used on self."));
    }
};