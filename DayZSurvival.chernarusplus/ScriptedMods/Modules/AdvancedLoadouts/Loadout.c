class VPPLoadout{
	private string bag;
	private string glove;
	private string vest;
	private string top;
	private string pants;
	private string boots;
	private string headGear;
	private string face;

	void Loadout(string bag, string glove, string vest, string top, string pants,string boots, string headGear, string face){
		this.bag = bag;
		this.glove = glove;
		this.vest = vest
		this.top = top;
		this.pants = pants;
		this.boots = boots;
		this.headGear = headGear;
		this.face = face;
	}

	string getBag(){
		return bag;	
	}

	string getGlove(){
		return glove;
	}

	string getVest(){
		return vest;
	}

	string getTop(){
		return top;
	}

	string getPants(){
		return pants;
	}

	string getBoots(){
		return boots;
	}

	string getHeadGear(){
		return headGear;
	}
	
	string getFace(){
		return face;
	}
}

class VPPLoadouts extends VPPConfig{
	ref array<VPPLoadouts> loadouts;
	void VPPLoadouts(){
		loadouts = new array<VPPLoadouts>;
	}

	override void save(){
		JsonFileLoader<Loadout>.JsonSaveFile(SETTINGS_FILE, this);
	}

	override void load(){
		if(FileExist(SETTINGS_FILE)){
			JsonFileLoader<Loadout>.JsonLoadFile(SETTINGS_FILE, this);
		}else{
			createDefaults();
			save();
		}
	}

	override void createDefaults(){
		//Fill in defaults to Loadouts
		loadouts.Insert(new Loadout("","","","","","","",""));
		loadouts.Insert(new Loadout("","","","","","","",""));
		loadouts.Insert(new Loadout("","","","","","","",""));
		loadouts.Insert(new Loadout("","","","","","","",""));
		loadouts.Insert(new Loadout("","","","","","","",""));
		loadouts.Insert(new Loadout("","","","","","","",""));
	}
}