/*
  _    _               _         ____  _     _           _   
 | |  | |             | |       / __ \| |   (_)         | |  
 | |__| | ___  _ __ __| | ___  | |  | | |__  _  ___  ___| |_ 
 |  __  |/ _ \| '__/ _` |/ _ \ | |  | | '_ \| |/ _ \/ __| __|
 | |  | | (_) | | | (_| |  __/ | |__| | |_) | |  __/ (__| |_ 
 |_|  |_|\___/|_|  \__,_|\___|  \____/|_.__/| |\___|\___|\__|
                                           _/ |              
                                          |__/               
*/
class InfectedHorde{
	
	private vector zonePos;
	private float currentLifeTime;
	private ref array<Object> m_SpawnedZombieObjs;
	private ref array<EntityAI> m_SpawnedZombieAI;
	
	void InfectedHorde(vector zoneLocation, int maxInfected, int minInfected, ref array<string> zombieClasses, bool canSpawnSpecial, bool canDropLoot, ref array<string> LootDrops,ref array<string> WeaponDrops, bool dropType){
		m_SpawnedZombieObjs = new array<Object>;
		m_SpawnedZombieAI = new array<EntityAI>;
		zonePos = zoneLocation;

		for(int i = 0; i < Math.RandomIntInclusive(minInfected, maxInfected); i++){
			vector randomLocationInZone;
			randomLocationInZone[0] = zoneLocation[0] + Math.RandomIntInclusive(-50,50);
			randomLocationInZone[2] = zoneLocation[2] + Math.RandomIntInclusive(-50,50);

			Object obj = GetGame().CreateObject(zombieClasses.GetRandomElement(), snapToGround(randomLocationInZone), false, true);
			m_SpawnedZombieObjs.Insert(obj);

			if(canSpawnSpecial){
				EntityAI zombieAI;
				if(Class.CastTo(zombieAI, obj)){
					int skin = Math.RandomIntInclusive(0, 5);	

					if(skin == 5){
						zombieAI.SetObjectMaterial( 0, "DZ\\data\\data\\laser.rvmat" );
						zombieAI.SetObjectMaterial( 1, "DZ\\data\\data\\laser.rvmat" );
						zombieAI.SetObjectMaterial( 2, "DZ\\data\\data\\laser.rvmat" );
					}
					
					if (canDropLoot){
						ZombieBase baseClass;
						if (Class.CastTo(baseClass, zombieAI)){
							baseClass.AttachEventHandle(LootDrops,WeaponDrops,Math.RandomIntInclusive(0,1),dropType);
						}
					}
				}
			}
		}
	}

	bool canDespawn(){
		foreach(Object Obj : m_SpawnedZombieObjs){
			EntityAI zombieAI;
			if(Class.CastTo(zombieAI,m_SpawnedZombieObjs) && zombieAI.IsAlive()){
				return false;
			}
			continue;
		}
		return true;
	}

	vector snapToGround(vector pos)
    {
        float pos_x = pos[0];
        float pos_z = pos[2];
        float pos_y = GetGame().SurfaceY( pos_x, pos_z );
        vector tmp_pos = Vector( pos_x, pos_y, pos_z );
        tmp_pos[1] = tmp_pos[1] + pos[1];

        return tmp_pos;
    }

	void deleteHorde(){
		foreach(Object obj : m_SpawnedZombieObjs){
			GetGame().ObjectDelete(obj);
		}
		m_SpawnedZombieAI.Clear();
		m_SpawnedZombieObjs.Clear();
	}

	vector getLocation(){
		return zonePos;
	}

	void addLifeTime(float timeslice){
		currentLifeTime += timeslice;
	}

	bool hasLivedLife(float maxLifeTime){
		return maxLifeTime < currentLifeTime;
	}
}