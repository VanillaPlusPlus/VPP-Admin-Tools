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
            if (playerTarget != null)
            {
                string targetName = playerTarget.VPlayerGetName();
                string targetID   = playerTarget.VPlayerGetSteamId();

                DamageSystem.ResetAllZones(playerTarget);
                playerTarget.GetModifiersManager().ResetAll();
                
                //Stats
                if (playerTarget.GetPlayerStats())
                {
                    playerTarget.GetPlayerStats().ResetAllStats();

                    playerTarget.GetStatBloodType().Set(playerTarget.GetStatBloodType().Get());
                    playerTarget.GetStatHeatComfort().Set(playerTarget.GetStatHeatComfort().GetMax());
                    playerTarget.GetStatTremor().Set(playerTarget.GetStatTremor().GetMin());
                    playerTarget.GetStatWet().Set(playerTarget.GetStatWet().GetMin());
                    playerTarget.GetStatEnergy().Set(playerTarget.GetStatEnergy().GetMax());
                    playerTarget.GetStatWater().Set(playerTarget.GetStatWater().GetMax());
                    playerTarget.GetStatDiet().Set(playerTarget.GetStatDiet().GetMax());
                    playerTarget.GetStatSpecialty().Set(playerTarget.GetStatSpecialty().GetMax());
                    playerTarget.GetStatHeatBuffer().Set(playerTarget.GetStatHeatBuffer().GetMax());
                }
    
                //Agents
                playerTarget.RemoveAllAgents();
                
                //Broken legs
                playerTarget.HealBrokenLegs();

                //Bleeding sources
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
                            int bit = playerTarget.GetBleedingManagerServer().GetMostSignificantBleedingSource();
                            if(bit != 0)
                            {
                                playerTarget.GetBleedingManagerServer().RemoveBleedingSourceNonInfectious(bit);
                                cuts--;
                            }
                        }
                    }
                }
                GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) /heal used on: \"%3\" (steamid=%4)", callerName, callerID, targetName, targetID));
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(callerID, callerName, "Chat Command Manager: /heal command used on: " + targetName + " ID: " + targetID));
            }
        }
    }
}