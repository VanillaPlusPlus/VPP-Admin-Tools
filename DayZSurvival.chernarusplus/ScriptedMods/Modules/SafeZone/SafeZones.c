#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\SafeZone\\SafeZone.c"

class SafeZones extends VPPConfig{
	const static string SETTINGS_FILE = "$profile:\\SafeZones.json";
	private ref array<ref SafeZone> m_safeZones;

	void SafeZones(){
		m_safeZones = new array<ref SafeZone>;
	}

	override void save(){
		super.save();
		JsonFileLoader<ref SafeZones>.JsonSaveFile(SETTINGS_FILE, this);
	}

	override void load(){
		super.load();
		if(FileExist(SETTINGS_FILE)){
			JsonFileLoader<ref SafeZones>.JsonLoadFile(SETTINGS_FILE, this);
		} else{
			Print("SafeZone.json was not found, creating.");
			createDefaults();
			save();
		}
	}

	override void createDefaults(){
		ref array<vector> locations = {
			Vector(7500, 0, 7500),
			Vector(7500, 0, 7500),
		};

		ref array<string> names = {
			"CenterOfMap",
			"CenterOfMap",
		};

		for(int i = 0; i < locations.Count(); i++){
			m_safeZones.Insert(new ref SafeZone(locations.Get(i), names.Get(i), 500));
		}
	}

	void addSafeZone(vector location, string name, float radius){
		m_safeZones.Insert(new ref SafeZone(location, name, radius));
		save();
	}

	ref array<ref SafeZone> getSafeZones(){
		return m_safeZones;
	}

	/*
		@Function: removeSafeZoneByName
		@Description: removes safezone by name

		@Param: name
		@Description: The name of the SafeZone you wish to remove from collection.
	*/
	void removeSafeZoneByName(string name){
		foreach(SafeZone safezone : m_safeZones){
			if(safezone.getName() == name){
				m_safeZones.RemoveItem(safezone);
			}
		}
	}

	/*
		@Function: isInSafeZone
		@Description: Checks to see if player is inside of any safezones.

		@Param: player
		@Description: The player to check.
	*/
	bool isInSafeZone(PlayerBase player){
		foreach(SafeZone safezone : m_safeZones){
			float distance = vector.Distance(player.GetPosition(), safezone.getLocation());
			if(distance <= safezone.getRadius()){
				return true;
			}
		}
		return false;
	}
}
