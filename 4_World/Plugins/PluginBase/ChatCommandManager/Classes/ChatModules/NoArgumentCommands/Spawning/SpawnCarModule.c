class SpawnCarChatModule extends ChatCommand
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

		autoptr array<string> parts = {
			"civsedanhood", "civsedanhood", "CivSedanDoors_CoDriver", "CivSedanDoors_Driver", "CivSedanDoors_BackLeft",
			"CivSedanDoors_BackRight", "civsedantrunk","civsedanwheel","civsedanwheel",
			"civsedanwheel","civsedanwheel","SparkPlug", "EngineBelt", "CarBattery", "carradiator",
		};
	
		Car sedan = Car.Cast(GetGame().CreateObject( "civiliansedan", caller.GetHeadingPosition(3), false, true, true ));
		
		foreach(string part : parts)
		{
			sedan.GetInventory().CreateAttachment(part);
		}
	
		for(int i = 0; i < 4; i++)
		{
			sedan.Fill(i, sedan.GetFluidCapacity(i));
		}

		GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /spawncar used on self.", callerName, callerID));
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /spawncar used on self."));
    }
}