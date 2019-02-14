#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\SafeZone\\SafeZones.c"

class SafeZoneHandler extends VPPModuleManager
{
	string ENTRY_MESSAGE     = "Welcome to The SafeZone! Godmode ENABLED!";
	string EXIT_MESSAGE      = "You Have Left The SafeZone! Godmode DISABLED!";

	private SafeZones config;

	void SafeZoneHandler( DayZSurvival m_MissionServer )
	{
		m_VPPConfig = new SafeZones();
		Class.CastTo(config, m_VPPConfig);
	}

	override void Init()
	{
		Print("SafeZone:: Active!");
	}

	void SafeZoneHandle(PlayerBase player)
	{
		string ZoneCheck;
		string GUID = player.GetIdentity().GetPlainId();

		foreach(SafeZone zone : config.getSafeZones()){
			g_Game.GetProfileString("SafeZoneStatus"+ GUID, ZoneCheck);
			if (config.isInSafeZone(player)) //Player Inside Zone
			{
				if (ZoneCheck == "true") //Already in zone
				{
					return;
				}
				else
				{
					g_Game.SetProfileString("SafeZoneStatus"+ GUID, "true");
					Param1<string> Msgparam = new Param1<string>( ENTRY_MESSAGE );
					GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, Msgparam, true, player.GetIdentity());
				}
			}
			else if (!config.isInSafeZone(player)) //Plauer Outside of Zone
			{
				g_Game.GetProfileString("SafeZoneStatus"+ GUID, ZoneCheck);
				if (ZoneCheck == "false")
				{
					return;
				}
				else
				{
					g_Game.SetProfileString("SafeZoneStatus"+ GUID, "false");
					Msgparam = new Param1<string>( EXIT_MESSAGE );
					GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, Msgparam, true, player.GetIdentity());
				}
			}
		}
	}
}