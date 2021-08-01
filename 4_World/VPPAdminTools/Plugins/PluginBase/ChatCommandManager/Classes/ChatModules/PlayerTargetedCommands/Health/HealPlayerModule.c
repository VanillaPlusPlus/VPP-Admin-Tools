class HealPlayerChatModule : ChatCommand
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

				playerTarget.SetHealth(playerTarget.GetMaxHealth("", ""));
				playerTarget.SetHealth( "","Blood", playerTarget.GetMaxHealth("","Blood"));
                playerTarget.SetHealth("","Shock", playerTarget.GetMaxHealth("","Shock"));
				playerTarget.GetStatHeatComfort().Set(playerTarget.GetStatHeatComfort().GetMax());
				playerTarget.GetStatTremor().Set(playerTarget.GetStatTremor().GetMin());
				playerTarget.GetStatWet().Set(playerTarget.GetStatWet().GetMin());
				playerTarget.GetStatEnergy().Set(playerTarget.GetStatEnergy().GetMax());
				playerTarget.GetStatWater().Set(playerTarget.GetStatWater().GetMax());
				playerTarget.GetStatDiet().Set(playerTarget.GetStatDiet().GetMax());
				playerTarget.GetStatSpecialty().Set(playerTarget.GetStatSpecialty().GetMax());
                playerTarget.HealBrokenLegs();
                
                if (playerTarget.GetBleedingManagerServer())
                {
                    int attempts = 0; //Fail safe, so we dont get stuck lol
                    int cuts = playerTarget.GetBleedingManagerServer().m_BleedingSources.Count();

                    while(cuts > 0)
                    {
                        attempts++;
                        if (attempts > 15)
                            return;

                        if (playerTarget.GetBleedingManagerServer())
                        {
                            playerTarget.GetBleedingManagerServer().RemoveMostSignificantBleedingSource();
                            cuts--;
                        }
                    }
                }

                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /heal used on: \"%3\" (steamid=%4)", callerName, callerID, targetName, targetID));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /heal command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}