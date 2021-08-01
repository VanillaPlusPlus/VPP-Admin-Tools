class TeleportToPointChatModule extends ChatCommand
{
	override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();

        GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /tpp used on self. Input (%3)", callerName, callerID, args));
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /tpp used on self. Input:  " + args));
		GetTeleportManager().TeleportToPoint(args, caller, callerID);
	}
}