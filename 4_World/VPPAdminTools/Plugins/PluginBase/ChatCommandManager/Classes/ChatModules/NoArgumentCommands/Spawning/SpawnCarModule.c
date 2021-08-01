class SpawnCarChatModule extends ChatCommand
{
	const ref array<string> PARTS = {
			"civsedanhood", "civsedanhood", "CivSedanDoors_CoDriver", "CivSedanDoors_Driver", "CivSedanDoors_BackLeft",
			"CivSedanDoors_BackRight", "civsedantrunk","civsedanwheel","civsedanwheel",
			"civsedanwheel","civsedanwheel","SparkPlug", "EngineBelt", "CarBattery", "carradiator",
	};

    override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {        
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();
	
		Car sedan = Car.Cast(GetGame().CreateObject( "civiliansedan", caller.GetHeadingPosition(3), false, true, true ));
		
		foreach(string part : PARTS)
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