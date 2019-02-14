class VPPSupplyDropConfig extends VPPConfig
{
	private int TimeBetweenEach;
	private int DespawnTime;
	private int MaxDrops;
	private int DropZoneRadius;
	private ref array<string> AirDropLoot;
	private ref array<vector> AirDropLocations;
	static const string SETTINGS_FILE = "$profile:\\SupplyDropConfig.json";


	void VPPSupplyDropConfig()
	{
		GetRPCManager().AddRPC("RPC_SupplyDropServer", "getSupplyDropJson", this, SingeplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_SupplyDropServer", "saveSupplyDropJson", this, SingeplayerExecutionType.Server);
		
		AirDropLoot = new ref array<string>;
		AirDropLocations = new ref array<vector>;
		TimeBetweenEach = 1200;
		DespawnTime = 1800;
		MaxDrops = 0;
		DropZoneRadius = 125;
	}

	void getSupplyDropJson(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		if(type == CallType.Server){
			/*Param6<int, int, int, ref array<string>, ref array<vector>> param = new Param6<int, int, int, ref array<string>, ref array<vector>>(TimeBetweenEach, DespawnTime, MaxDrops, AirDropLoot, AirDropLocations,DropZoneRadius);
			GetRPCManager().SendRPC("RPC_SupplyDropClient", "handleSupplyDropJson", param, true, sender);
			*/
		}
	}

	void saveSupplyDropJson(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		Param5<int, int, int, ref array<string>, ref array<vector>> jsonData;

		if(!ctx.Read(jsonData)) return;

		if(type == CallType.Server){
			//addData(jsonData.param1, jsonData.param2, jsonData.param3, jsonData.param4, jsonData.param5);
			save();
		}
	}
	
	override void createDefaults()
	{
		ref array<string> loot = {
			"TTSKOPants",
			"TacticalBaconCan",
			"M4A1",
			"PlateCarrierComplete",
			"BakedBeansCan",
			"WaterBottle",
		};
		
		ref array<vector> locations = {
			"7500 0 7500",
		};
		
		int max = 0;
		int time = 1200;
		int despawnTime = 1800;
		int dropzoneradius = 125;
		
		addData(max, time, despawnTime, dropzoneradius, loot, locations);
	}
	override void save(){
		JsonFileLoader<VPPSupplyDropConfig>.JsonSaveFile(SETTINGS_FILE, this);
	}

	override void load(){
		if (FileExist(SETTINGS_FILE)) {
			JsonFileLoader<VPPSupplyDropConfig>.JsonLoadFile(SETTINGS_FILE, this);
		}else{
			Print("No Airdrop Config found, creating Airdrop json.");
			createDefaults();
			save();
		}
	}

	void addData(int maxDrops, int timebetweeneach, int despawnTimer, int dropzoneRadius, array<string> loot, array<vector> locations){
		AirDropLoot = loot;
		AirDropLocations = locations;
		TimeBetweenEach = timebetweeneach;
		DespawnTime = despawnTimer;
		MaxDrops = maxDrops;
		DropZoneRadius = dropzoneRadius;
	}
	
	ref array<string> getLootArray()
	{
		return AirDropLoot;
	}

	int getDespawnTime()
	{
		return DespawnTime;
	}
	
	vector chooseDropLocation()
	{
		return AirDropLocations.GetRandomElement();
	}

	int getTimeBetweenEach()
	{
		return TimeBetweenEach;
	}
	
	int getMaxDrops()
	{
		return MaxDrops;
	}
	
	int getDropRadius()
	{
		return DropZoneRadius;
	}
}