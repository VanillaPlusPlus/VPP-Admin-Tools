class RefuelVehicleChatModule extends ChatCommand
{	
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {        
    	PlayerIdentity identity = caller.GetIdentity();

        if(identity == null) return;

        string callerID;
        string callerName;

        if(identity != null)
        {
            callerID = identity.GetPlainId();
            callerName = identity.GetName();
        }

		Human human = Human.Cast(caller);
		Car toBeFilled = Car.Cast(human.GetCommand_Vehicle().GetTransport());
	
		if(toBeFilled == null) return;
	
		EntityAI carEntity = toBeFilled;
		carEntity.SetHealth("", "",carEntity.GetMaxHealth());
		carEntity.SetHealth("Engine", "Health",carEntity.GetMaxHealth());
		carEntity.SetHealth("EngineBelt", "Health",carEntity.GetMaxHealth());
	
		ref array<EntityAI> vehParts = new array<EntityAI>;
		TStringArray SlotNames = new TStringArray;
		string cfg_path = CFG_VEHICLESPATH + " " + toBeFilled.GetType() + " attachments";
		GetGame().ConfigGetTextArray(cfg_path, SlotNames);	
		
		foreach(string carSlot : SlotNames)
				vehParts.Insert(toBeFilled.FindAttachmentBySlotName(carSlot));
	
		if (vehParts != null){
			foreach(EntityAI att : vehParts)
			{
				if (att != null)
				{
					att.SetHealth("", "Health",att.GetMaxHealth());
					att.SetSynchDirty();
				}
			}
		}
	
		float fuelReq = toBeFilled.GetFluidCapacity( CarFluid.FUEL ) - (toBeFilled.GetFluidCapacity( CarFluid.FUEL ) * toBeFilled.GetFluidFraction( CarFluid.FUEL ));
		float oilReq = toBeFilled.GetFluidCapacity( CarFluid.OIL ) - (toBeFilled.GetFluidCapacity( CarFluid.OIL ) * toBeFilled.GetFluidFraction( CarFluid.OIL ));
		float coolantReq = toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) - (toBeFilled.GetFluidCapacity( CarFluid.COOLANT ) * toBeFilled.GetFluidFraction( CarFluid.COOLANT ));
		float brakeReq = toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) - (toBeFilled.GetFluidCapacity( CarFluid.BRAKE ) * toBeFilled.GetFluidFraction( CarFluid.BRAKE ));
		toBeFilled.Fill( CarFluid.FUEL, fuelReq );
		toBeFilled.Fill( CarFluid.OIL, oilReq );
		toBeFilled.Fill( CarFluid.COOLANT, coolantReq );
		toBeFilled.Fill( CarFluid.BRAKE, brakeReq );
		toBeFilled.SetSynchDirty();
		GetPermissionManager().NotifyPlayer(callerID, carEntity.Type().ToString() + ": reparied & refueled [" +fuelReq+ "L] added, all fluids maxed",NotifyTypes.NOTIFY);
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /refuel used on self."));
    }
}