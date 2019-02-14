/*
   _____                                      
  / ____|                                     
 | (___  _ __   __ ___      ___ __   ___ _ __ 
  \___ \| '_ \ / _` \ \ /\ / / '_ \ / _ \ '__|
  ____) | |_) | (_| |\ V  V /| | | |  __/ |   
 |_____/| .__/ \__,_| \_/\_/ |_| |_|\___|_|   
        | |                                   
        |_|                                   
*/

#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\InfectedHordes\\InfectedHorde.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\InfectedHordes\\InfectedHordeConfig.c"


class InfectedHordesSpawner extends VPPModuleManager
{
    private float timeSnapshot;
    private float lifeTimeClock;

    ref array<ref InfectedHorde> m_hordes;

    ref InfectedHordeConfig config;

    void InfectedHordesSpawner( DayZSurvival m_MissionServer )
    {
         config = new InfectedHordeConfig();
         config.load();
         m_hordes = new array<ref InfectedHorde>;
    }

    void Init(){
        Print("Infected Hordes...INIT!");
    }

    void onUpdate(float timeslice){
        timeSnapshot += timeslice;
        lifeTimeClock += timeslice;

        if(config.getHordeSpawnTime() <= timeSnapshot){
            timeSnapshot = 0;

            if(m_hordes.Count() != config.getMaxHordes()){
                vector spawnPos = config.getHordeZoneLocation();
                if(spawnPos != "0 0 0"){
                    ref InfectedHorde insertHorde = new InfectedHorde(spawnPos, config.getMinInfectedCount(), config.getMaxInfectedCount(), config.getZombieClasses(), config.canSpawnSpecialInfected());
                    m_hordes.Insert(insertHorde);
                    
                    if(config.shouldSendBroadcast()){
						string zoneName = config.getHordeZoneLocationName(spawnPos);
						m_MissionServer.SendMessage(true,NULL,"Infected Hordes:","A horde of infected has been spotted by survivors around: "+zoneName,10,2,false,true,"",0,0);
                    }
                    Print("Horde spawned at " + insertHorde.getLocation().ToString());
                }
            }
        }

        if(m_hordes.Count() == 0){
            lifeTimeClock = 0;
            return;
        }

        if(config.hasLifeTime()){
            foreach(InfectedHorde horde : m_hordes){
                horde.addLifeTime(timeslice);

                if(horde.hasLivedLife(config.getDespawnTime())){
                    if(config.shouldSendBroadcast()){
                        //Send Broadcast Message
                    }

                    Print("Horde spawned at " + horde.getLocation().ToString() + " has lived it's life and depsawned.");
                    horde.deleteHorde();
                    m_hordes.RemoveItem(horde);
                }
            }
        } else{
            foreach(InfectedHorde m_horde : m_hordes){
                if(m_horde.canDespawn()){
                    m_hordes.RemoveItem(m_horde);
                }
            }
        }
    }
}