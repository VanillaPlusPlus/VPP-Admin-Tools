class SpawnInHandsChatModule : ChatCommand
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

        foreach(string arg : args)
        {
            if(caller != null)
            {                       
                if(caller.GetHumanInventory().GetEntityInHands() == null)
                    caller.GetHumanInventory().CreateInHands(arg);
                else
                    caller.GetInventory().CreateInInventory(arg);

                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /sph used on self. Input (%3)", callerName, callerID, arg));
                GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /sph used on self. Input:  " + arg));
            }
        }
    }
}