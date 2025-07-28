class AdminTools extends PluginBase
{
	void AdminTools()
	{
		//---RPC's-----
		//GetRPCManager().AddRPC( "RPC_AdminTools", "AttachTo", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_AdminTools", "DeleteObject", this, SingeplayerExecutionType.Server );
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
	
	void ToggleFreeCam(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
        if (type == CallType.Server && sender != null)
        {
        	Param1<bool> data; //if true, freecam is already active.
        	if (!ctx.Read(data))
        		return;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "FreeCamera")) return;
			
			PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(sender.GetPlainId());
			if (pb == null)
				return;

			if (!data.param1)
			{
				//command handler to freeze player
				if (!pb.GetCommand_Vehicle())
				{
					pb.InitTablesCmd();
                	HumanCommandScript_VPPCam cmdFS = new HumanCommandScript_VPPCam(pb, pb.m_VPPCamHmnCmd);
                	pb.StartCommand_Script(cmdFS);
				}
			}
			else
			{
				//command handler to unfreeze player
                HumanCommandScript_VPPCam hcs = HumanCommandScript_VPPCam.Cast(pb.GetCommand_Script());
                if (hcs)
                {
                    hcs.SetFlagFinished(true);
                    hcs.m_bNeedFinish = true;
                }
			}

			GetRPCManager().VSendRPC( "RPC_HandleFreeCam", "HandleFreeCam", new Param1<bool>(true), true, sender);
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
			carEntity.SetHealthMax("", "Health");
			carEntity.SetHealthMax();
			CarScript.Cast(target).RefillAllLiquids();

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
	}
};