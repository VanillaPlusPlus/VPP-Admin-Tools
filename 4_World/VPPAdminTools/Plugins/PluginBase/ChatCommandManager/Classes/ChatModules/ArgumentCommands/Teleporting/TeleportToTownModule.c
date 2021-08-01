class TeleportToTownChatModule : ChatCommand
{
    override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();

        foreach(string arg : args)
        {
            if(caller != null)
            {
                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /tpt used on self. Input (%3)", callerName, callerID, arg));
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /tpt used on self. Input:  " + arg));
                GetTeleportManager().TeleportToTown(arg, caller);
            }
        }
    }
}