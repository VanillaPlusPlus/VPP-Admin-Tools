//////////////////////////////////////////////
//Airdrop System 			   			 	//
//Author: GravityWolf 					 	//
//Github: github.com/gravitywolfnotamused	//
//////////////////////////////////////////////

#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\VPPSupplyPlane.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\VPPSupplyDropConfig.c"

class VPPSupplyDrop extends VPPModuleManager 
{
	private float timeSnapshot;
	private float roundedSnapShot;
	private float lifeTimeClock;
	
	private bool hasAirdrop = false;
	private bool debuggingMode = false;
	private int  dropZoneRadius;
	
	ref SupplyPlanes planes;
	ref Supplies supplies;
	ref VPPSupplyDropConfig config;

	void VPPSupplyDrop( DayZSurvival m_Mission )
	{	
		planes = new SupplyPlanes;
		supplies = new Supplies;

		m_VPPConfig = new VPPSupplyDropConfig();
		Class.CastTo(config, m_VPPConfig);
	
		dropZoneRadius = config.getDropRadius();
		GetRPCManager().AddRPC("RPC_SupplyDropServer", "sendSupplyDropJson", this, SingeplayerExecutionType.Server);
	}
	
	void ~VPPSupplyDrop(){
		delete planes;
		delete supplies;
		delete m_VPPConfig;
	}
	
	void Init(){
		Print("SupplyDrop::Init Plugin Initialized.");
	}
	
	void onUpdate(float timeslice){		
		timeSnapshot += timeslice;
		
		UpdateDebugTimer();

		if(supplies.Count() != 0){		
			lifeTimeClock += timeslice;
		}

		if(planes.Count() == 0){
			hasAirdrop = false;
		}else{
			hasAirdrop = true;
		}
		
		if(timeSnapshot >= config.getTimeBetweenEach()){
			timeSnapshot = 0;
			roundedSnapShot = 0;

			if(canSpawnDrop()){
				planes.Insert(new VPPSupplyPlane(getStartingPosition(), config.chooseDropLocation()));
			}
		}

		if(hasAirdrop){
			foreach(VPPSupplyPlane plane : planes){
				int RandomdropRadius = Math.RandomIntInclusive(25,dropZoneRadius);
				if (plane != NULL)
				{
					if(Math.Round(plane.getPosition()[0]) >= Math.Round(plane.getDropPosition()[0]) - RandomdropRadius && Math.Round(plane.getPosition()[0]) <= Math.Round(plane.getDropPosition()[0]) + RandomdropRadius)
					{
						if(Math.Round(plane.getPosition()[2]) >= Math.Round(plane.getDropPosition()[2]) - RandomdropRadius && Math.Round(plane.getPosition()[2]) <= Math.Round(plane.getDropPosition()[2]) + RandomdropRadius)
						{
							if(!plane.hasDroppedSupplies())
							{
								supplies.Insert(new VPPSupply(Vector(plane.getPosition()[0],plane.getPosition()[1] - 15,plane.getPosition()[2]), config.getLootArray()));
								plane.setHasDroppedSupplies();
							}
						}
					}
					
					if((plane.getPosition()[0] < 1 || plane.getPosition()[2] < 1 || plane.getPosition()[0] > 15400 || plane.getPosition()[2] > 15400)){
						GetGame().ObjectDelete(plane.getObject());
						planes.RemoveItem(plane);			
						Print("Plane hit out of bounds, deleting.");
					}
					plane.move();
				}
			}
		}

		if(supplies.Count() != 0){
			foreach(VPPSupply supplyDrop : supplies){
				supplyDrop.setCurrentLifetime(timeslice);
				
				if(config.getDespawnTime() < supplyDrop.getLifeTime()){
					GetGame().ObjectDelete(supplyDrop.getObject());
					supplies.RemoveItem(supplyDrop);
					Print("Supply Create has lived it's life, it has been despawned.");
					continue;
				}

				if (!supplyDrop.hasGravity()) 
					supplyDrop.applyGravity();
			}
		}
	}

	bool canSpawnDrop(){
		return planes.Count() + supplies.Count() != config.getMaxDrops();
	}

	void sendSupplyDropJson(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		//Param5<int, int, int, ref array<string>, ref array<vector>> jsonData = new Param5<int, int, int, ref array<string>, ref array<vector>>();

		if(type == CallType.Server){
			GetRPCManager().SendRPC("RPC_SupplyDropClient", "handleSupplyDropJson", NULL, true, sender);
		}
	}

	private void addSupplyDrop(vector dropSite){
		if(canSpawnDrop()){
			planes.Insert(new VPPSupplyPlane(getStartingPosition(), dropSite));
		}
	}

	void sendSupplyDrop(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target){
		Param1<vector> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Server){
			addSupplyDrop(data.param1);
		}
	}
	
	void UpdateDebugTimer(){
		if(debuggingMode){
			if(Math.Round(timeSnapshot) > roundedSnapShot){
				roundedSnapShot = Math.Round(timeSnapshot);
				Print("Snapshot is: " + roundedSnapShot);
			}
		}
	}
	
	vector getStartingPosition(){
		vector starting = "0 0 1";
		starting[0] = Math.RandomFloat(0,15100);

		if(starting[0] >= 14400){
			starting[2] = Math.RandomFloat(0, 12200);
		}
		return starting;
	}
}