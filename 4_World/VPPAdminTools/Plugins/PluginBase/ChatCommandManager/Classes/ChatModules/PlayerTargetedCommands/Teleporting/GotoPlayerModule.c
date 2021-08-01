class GotoPlayerChatModule : ChatCommand
{
    override void ExecuteCommand(PlayerBase caller, array<Man> targets, array<string> args)
    {
        if(caller == null) return;

        string callerID   = caller.VPlayerGetSteamId();
        string callerName = caller.VPlayerGetName();

        foreach(Man target : targets)
        {
            PlayerBase playerTarget = PlayerBase.Cast(target);
            if(playerTarget != null)
            {
                string targetName = playerTarget.VPlayerGetName();
                string targetID   = playerTarget.VPlayerGetSteamId();

                GetTeleportManager().GotoPlayer(target, caller, callerID);
                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /goto used on: \"%3\" (steamid=%4)", callerName, callerID, targetName, targetID));
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /goto command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}