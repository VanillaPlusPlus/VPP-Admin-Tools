#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerMission\\VPPSpawnPoints.c"
class SpawnPointsManager extends VPPModuleManager
{
	ref VPPSpawnPoints points;

	void SpawnPointsManager( DayZSurvival m_Mission )
	{
		m_VPPConfig = new VPPSpawnPoints();
		points = VPPSpawnPoints.Cast(m_VPPConfig);

		GetRPCManager().AddRPC( "RPC_SpawnPointsManager", "GetSpawnPointsArray", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_SpawnPointsManager", "UpdateSpawnPoints", this, SingeplayerExecutionType.Server );
	}

	void ~SpawnPointsManager()
	{

	}

	override void Init()
	{
		
	}

	void GetSpawnPointsArray(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if(type == CallType.Server)
		{
			ref array<ref VPPSpawnPoint> m_MapSpawnPoints = VPPSpawnPoints.Cast(m_MissionServer.GetConfigByType(VPPSpawnPoints)).getSpawnPointsArray();
			ref Param1<ref array<ref VPPSpawnPoint>> spawnData = new Param1<ref array<ref VPPSpawnPoint>>(m_MapSpawnPoints);
			GetRPCManager().SendRPC("RPC_ServerSettingsUI", "sortSpawnPoints", spawnData, true, sender);
		}
	}

	void UpdateSpawnPoints( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		ref Param1< ref array< ref VPPSpawnPoint> > data;
		if(!ctx.Read(data)) return;

		if (type == CallType.Server)
		{
			points.ClearAllPoints();
			points.ReplacePointsWithNew(data.param1);
			Print("UpdateSpawnPoints()... ");
		}
	}

	vector GetPosByName(string Name, int key)
	{
		ref array<ref VPPSpawnPoint> m_MapSpawnPoints = VPPSpawnPoints.Cast(m_MissionServer.GetConfigByType(VPPSpawnPoints)).getSpawnPointsArray();
    	foreach(VPPSpawnPoint point : m_MapSpawnPoints)
    	{
    		if (Name == point.getSpawnName())
    		{
    			return point.getSpawnLocation();
    		}
    	}
    	return "0 0 0";
	}

	ref array<string> GetAllSpawnsNames()
	{
		ref array<ref VPPSpawnPoint> m_MapSpawnPoints = VPPSpawnPoints.Cast(m_MissionServer.GetConfigByType(VPPSpawnPoints)).getSpawnPointsArray();
    	ref array<string> Locations = new array<string>;
    	foreach(VPPSpawnPoint point : m_MapSpawnPoints)
    	{
    		Locations.Insert(point.getSpawnName());
    	}
    	Print("Total Points: "+Locations.Count().ToString());
		return Locations;
	}

	vector GetRandomSpawnPoint()
	{
		ref array<ref VPPSpawnPoint> SpawnPoints = points.getSpawnPointsArray();
		int oRandom  = Math.RandomIntInclusive(0,SpawnPoints.Count() - 1);
		ref VPPSpawnPoint SelectedPoint = SpawnPoints.Get(oRandom);
		if (SelectedPoint.getSpawnLocation() != "0 0 0")
		{
			return SelectedPoint.getSpawnLocation();
		}
		return "0 0 0";
	}
}
