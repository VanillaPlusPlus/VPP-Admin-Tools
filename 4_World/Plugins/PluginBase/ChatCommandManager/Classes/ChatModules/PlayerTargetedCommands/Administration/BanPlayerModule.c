class BanPlayerChatModule : ChatCommand
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

        foreach(Man target : targets)
        {
            PlayerBase playerTarget = PlayerBase.Cast(target);
            PlayerIdentity targetIdentity = playerTarget.GetIdentity();

            if(playerTarget != null && targetIdentity != null)
            {
                string targetName = targetIdentity.GetName();
                string targetID = targetIdentity.GetPlainId();
                string targetGUID = targetIdentity.GetId();

				string banAuthorDetails = string.Format("%1|%2",callerName,callerID);
				GetBansManager().AddToBanList(new BannedPlayer(targetName, targetID, targetGUID, new BanDuration(1990,12,12,12,12), banAuthorDetails, "Banned By VPP Admin Tools"));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /ban command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}