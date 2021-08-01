class BanPlayerChatModule : ChatCommand
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
                string targetGUID = playerTarget.VPlayerGetHashedId();

				string banAuthorDetails = string.Format("%1|%2",callerName,callerID);
				GetBansManager().AddToBanList(new BannedPlayer(targetName, targetID, targetGUID, new BanDuration(1990,12,12,12,12), banAuthorDetails, "Banned By VPP Admin Tools"));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /ban command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}