class HealPlayerChatModule : ChatCommand
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

				playerTarget.SetHealth( playerTarget.GetMaxHealth( "", "" ) );
				playerTarget.SetHealth( "","Blood", playerTarget.GetMaxHealth( "", "Blood" ) );
				playerTarget.GetStatHeatComfort().Set(playerTarget.GetStatHeatComfort().GetMax());
				playerTarget.GetStatTremor().Set(playerTarget.GetStatTremor().GetMin());
				playerTarget.GetStatWet().Set(playerTarget.GetStatWet().GetMin());
				playerTarget.GetStatEnergy().Set(playerTarget.GetStatEnergy().GetMax());
				playerTarget.GetStatWater().Set(playerTarget.GetStatWater().GetMax());
				playerTarget.GetStatDiet().Set(playerTarget.GetStatDiet().GetMax());
				playerTarget.GetStatSpecialty().Set(playerTarget.GetStatSpecialty().GetMax());
				playerTarget.SetBleedingBits(0);

                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /heal used on: \"%3\" (steamid=%4)", callerName, callerID, targetName, targetID));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /heal command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}