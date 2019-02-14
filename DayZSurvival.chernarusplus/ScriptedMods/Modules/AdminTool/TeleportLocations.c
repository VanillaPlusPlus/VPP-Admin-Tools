class VPPTeleportLocations extends VPPConfig{
	const static string SETTINGS_FILE = "$profile:\\TeleportLocation.json";
	private ref array<ref VPPTeleportLocation> m_TeleportLocations;
	void VPPTeleportLocations(){
		m_TeleportLocations = new array<ref VPPTeleportLocation>;
	}

	override void save()
	{
		super.save();
		JsonFileLoader<ref VPPTeleportLocations>.JsonSaveFile(SETTINGS_FILE, this);
	}

	override void load()
	{
		super.load();
		if (FileExist(SETTINGS_FILE)) {
			JsonFileLoader<ref VPPTeleportLocations>.JsonLoadFile(SETTINGS_FILE, this);
		}else{
			Print("TeleportLocations.json not found. Creating...");
			createDefaults();
			save();
		}
	}
	
	override void createDefaults(){
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Severograd", "8428.0 0.0 12767.1" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Krasnostav", "11172.0 0.0 12314.1"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Mogilevka", "7537.8 0.0 5211.55" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Stary Sobor", "6046.94 0.0 7733.97" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Msta", "11322.55 0.0 5463.36" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Vybor", "3784.16 0.0 8923.48"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Gorka", "9514.27 0.0 8863.69"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Solnichni", "13402.57 0.0 6303.35" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("NWAF South", "4540.52 0.0 9645.84" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("NWAF Centre", "4823.43 0.0 10457.16" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("NWAF North", "4214.84 0.0 10977.78" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Blota Airfield", "4467.61 0.0 2496.14" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("NEAF", "11921.43 0.0 12525.55"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Cherno Centre", "6649.22 0.0 2710.03"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Cherno West", "6374.08 0.0 2361.01"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Cherno East", "7331.70 0.0 2850.03"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Elektro West", "10077.17 0.0 1988.65"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Elektro East", "10553.55 0.0 2313.37"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Berez Centre", "12319.54 0.0 9530.15"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Berez South", "11991.42 0.0 9116.95"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Berez North", "12823.14 0.0 10078.97" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Svetlojarsk", "13900.82 0.0 13258.12"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Zelenogorsk South", "2572.80 0.0 5105.09" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Zelenogorsk North", "2741.48 0.0 5416.69" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Lopatino", "2714.74 0.0 9996.33"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Tisy", "1723.10 0.0 13983.88" ));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Novaya Petrovka", "3395.28 0.0 13013.61"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Novy Sobor", "7085.73 0.0 7720.85"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Grishino", "5952.15 0.0 10367.71"));
		m_TeleportLocations.Insert(new ref VPPTeleportLocation("Kabanino", "5363.97 0.0 8594.39"));
		save();
	}

	void addLocation(string locationName, vector pos){
		m_TeleportLocations.Insert(new ref VPPTeleportLocation(locationName, pos));
		save();
	}

	ref array<ref VPPTeleportLocation> getLocationArray()
	{
		return m_TeleportLocations;
	} 

	void removeLocationByPos(vector pos){
		foreach(VPPTeleportLocation tpPos : m_TeleportLocations){
			if(tpPos.getLocation() == pos){
				m_TeleportLocations.RemoveItem(tpPos);
				save();
			}
		}
	}

	void removeLocationByName(string name){
		foreach(VPPTeleportLocation tpPos : m_TeleportLocations){
			if(tpPos.getName() == name){
				m_TeleportLocations.RemoveItem(tpPos);
				save();
			}
		}
	}

	bool containsLocationByName(string name){
		foreach(VPPTeleportLocation tpPos: m_TeleportLocations){
			if(tpPos.getName() == name){
				return true;
			}
		}
		return false;
	}

	bool containsLocationByVector(vector location){
		foreach(VPPTeleportLocation tpPos: m_TeleportLocations){
			if(tpPos.getLocation() == location){
				return true;
			}
		}
		return false;
	}
}

class VPPTeleportLocation{
	string name;
	vector position;
	void VPPTeleportLocation(string name, vector pos){
		this.name = name;
		this.position = pos;
	}

	string getName(){
		return name;
	}

	vector getLocation(){
		return position;
	}
}
