class NamalskEvents extends ChatCommand
{
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

		Mission ms = GetGame().GetMission();
		Class eventManager;
		EnScript.GetClassVar(ms, "m_EventManagerServer", 0, eventManager);
		
		string eventTypeStr = args[0];
		typename eventType = eventTypeStr.ToType();
		if (eventType && eventType.IsInherited(String("NamEventBase").ToType()))
		{
			bool isRunning;
			GetGame().GameScript.CallFunction(eventManager, "IsEventActive", isRunning, eventType);
			if (isRunning)
			{
				GetPermissionManager().NotifyPlayer(caller.VPlayerGetSteamId(), "Could not start event! Already running: " + eventTypeStr, NotifyTypes.NOTIFY);
				return;
			}
			
			GetGame().GameScript.CallFunctionParams(eventManager, "StartEvent", NULL, new Param2<typename, bool>(eventType, true));
			GetPermissionManager().NotifyPlayer(caller.VPlayerGetSteamId(), "Running Namalsk Event: " + eventTypeStr, NotifyTypes.NOTIFY);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(caller.VPlayerGetSteamId(), caller.VPlayerGetName(), "Forcing a Namalsk event:  " + args));
		}else{
			GetPermissionManager().NotifyPlayer(caller.VPlayerGetSteamId(), "Error, could not run event. Type invalid! Please check spelling.", NotifyTypes.NOTIFY);
		}
	}
}