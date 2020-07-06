class BringPlayerChatModule : ChatCommand
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

                GetTeleportManager().BringPlayer(PlayerBase.Cast(target), caller.GetPosition(), callerID);
                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /bring used on: \"%3\" (steamid=%4)", callerName, callerID, targetName, targetID));
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /bring command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}