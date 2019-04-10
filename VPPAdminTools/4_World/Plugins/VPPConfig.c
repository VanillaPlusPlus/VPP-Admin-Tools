class VPPConfig
{
	void VPPConfig();
	void ~VPPConfig();

	void createDefaults();

	void save(){
		Print("Saving " + ClassName());
	}
	
	void load(){
		Print("Loading " + ClassName());
	}

	string getSavePath();

	string getTypeName(){
		return ClassName();
	}

	typename getType(){
		return getTypeName().ToType();
	}
}