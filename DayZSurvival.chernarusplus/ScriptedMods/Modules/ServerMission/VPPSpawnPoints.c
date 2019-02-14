class VPPSpawnPoints extends VPPConfig {

	static const string SETTINGS_FILE = "$profile:\\VPPSpawnPoints.json";
	private bool CUSTOMIZABLE_LOADOUTS;
	private bool STATIC_LOADOUTS;
	private bool FORCED_RANDOM_SPAWNS;
	private ref array<ref VPPSpawnPoint> spawnPoints;

	void VPPSpawnPoints(){
		spawnPoints = new array<ref VPPSpawnPoint>;

		GetRPCManager().AddRPC("RPC_SpawnPointsServer", "removeSpawnPoint", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_SpawnPointsServer", "addSpawnPoint", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_SpawnPointsServer", "getConfigInfo", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_SpawnPointsServer", "setConfigInfo", this, SingeplayerExecutionType.Server);
	}

	void ~VPPSpawnPoints(){
		
	}
	
	void removeSpawnPoint(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		Param2<string, vector> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Server){
			removeSpawnPointByName(data.param1);
		}
	}

	void addSpawnPoint(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		Param3<string, vector, float> addData;

		if(!ctx.Read(addData)) return;

		if(type == CallType.Server){
			addSpawnPointToCollection(addData.param1, addData.param2, addData.param3);
		}
	}

	void getConfigInfo(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		if(type == CallType.Server){
			ref Param3<bool,bool,bool> oParams = new Param3<bool,bool,bool>(CUSTOMIZABLE_LOADOUTS,STATIC_LOADOUTS,FORCED_RANDOM_SPAWNS);
	    	GetRPCManager().SendRPC( "RPC_ServerSettingsUI", "sortSpawnSettings", oParams, true, sender );
		}
	}

	void setConfigInfo(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		
		Param3<bool,bool,bool> Data;
		if(!ctx.Read(Data)) return;

		if(type == CallType.Server){
			CUSTOMIZABLE_LOADOUTS = Data.param1;
			STATIC_LOADOUTS		  = Data.param2;
			FORCED_RANDOM_SPAWNS  = Data.param3;
			save();
		}
	}

	override void save(){
		super.save();
		JsonFileLoader<VPPSpawnPoints>.JsonSaveFile(SETTINGS_FILE, this);
	}

	override void load(){
		super.load();
		if (FileExist(SETTINGS_FILE)) {
			JsonFileLoader<VPPSpawnPoints>.JsonLoadFile(SETTINGS_FILE, this);
		}else{
			createDefaults();
			save();
		}
	}
	
	override void createDefaults(){
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint0", "1598.47 0 2319.08", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint1", "1183.56 0 4387.4", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint2", "1614.98 0 5237.16", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint3", "1965.51 0 5226.71", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint4", "3101.78 0 4450.58", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint5", "3811.87 0 4967.03", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint6", "4793.88 0 4765.16", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint7", "2424.25 0 5810.53", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint8", "2424.25 0 5810.53", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint9", "2239.14 0 6537.76", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint10", "2709.21 0 6805.49", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint11", "3240.48 0 6675.31", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint12", "3559.28 0 6400.25", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint13", "4084.57 0 6218.31", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint14", "4411.33 0 6208.7", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint15", "4728.26 0 6283.69", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint16", "4712.08 0 5806.04", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint17", "4856.5 0 6278.57", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint18", "5947.13 0 6679.44", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint19", "3973.08 0 7975.11", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint20", "4137.5 0 8612.45", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint21", "3196.51 0 8906.59", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint22", "1969.12 0 9354.73", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint23", "651.716 0 9155.5", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint24", "2819.51 0 9707.45", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint25", "3483.69 0 9972.09", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint26", "2681.46 0 10454.5", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint27", "2227.24 0 10781.8", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint28", "3416.78 0 10811.1", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint29", "3416.78 0 10811.1", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint30", "4150.98 0 10895.3", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint31", "4491.77 0 10640.6", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint32", "4547.1 0 10293.8", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint33", "4846.42 0 9857.94", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint34", "5095.83 0 9538.65", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint35", "5303.58 0 9313.91", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint36", "5000.43 0 9170.28", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint37", "7147.12 0 9735.82", 900));
		spawnPoints.Insert(new VPPSpawnPoint("SpawnPoint38", "7448.82 0 11635.9", 900));

		CUSTOMIZABLE_LOADOUTS = true;
		STATIC_LOADOUTS		  = false;
		FORCED_RANDOM_SPAWNS  = false;
	}

	string loadOutsType()
	{
		if (CUSTOMIZABLE_LOADOUTS && !STATIC_LOADOUTS)
		{
			return "CUSTOMIZABLE_LOADOUTS";
		}
		else if (!CUSTOMIZABLE_LOADOUTS && STATIC_LOADOUTS){
			return "STATIC_LOADOUTS";
		}
		return "STATIC_LOADOUTS"; //Default
	}

	bool isForcedRandomSpawns()
	{
		return FORCED_RANDOM_SPAWNS;
	}

	void ClearAllPoints()
	{
		spawnPoints = new array<ref VPPSpawnPoint>;
	}

	void ReplacePointsWithNew(ref array<ref VPPSpawnPoint> NewPositions)
	{
		spawnPoints = NewPositions;
		save();
	}

	void addSpawnPointToCollection(string name, vector location, float cooldown = 600){
		spawnPoints.Insert(new VPPSpawnPoint(name, location, cooldown));
		save();
	}

	void emptySpawnPoints(){
		foreach(VPPSpawnPoint point : spawnPoints){
			spawnPoints.RemoveItem(point);
			save();
		}
	}

	void removeSpawnPointByVector(vector location){
		foreach(VPPSpawnPoint point : spawnPoints){
			if(point.getSpawnLocation() == location){
				spawnPoints.RemoveItem(point);
				save();
			}
		}
	}

	void removeSpawnPointByName(string name){
		foreach(VPPSpawnPoint point : spawnPoints){
			if(point.getSpawnName() == name){
				spawnPoints.RemoveItem(point);
				save();
			}
		}
	}

	vector getSpawnByName(string locationName)
	{
		foreach(VPPSpawnPoint point : spawnPoints){
			if(point.getSpawnName() == locationName){
				return point.getSpawnLocation();
			}
		}
		return "0 0 0";
	}

	ref array<ref VPPSpawnPoint> getSpawnPointsArray()
	{
		return spawnPoints;
	}

	bool containsSpawnPointByName(string name){
		foreach(VPPSpawnPoint point : spawnPoints){
			if(point.getSpawnName() == name){
				return true;
			}
		}
		return false;
	}

	bool containsSpawnPointByVector(vector location){
		foreach(VPPSpawnPoint point : spawnPoints){
			if(point.getSpawnLocation() == location){
				return true;
			}
		}
		return false;
	}
}