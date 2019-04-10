class VPPKeybindsConfig
{
	const string SAVE_PATH = "$profile:VPPKeyBinds.json";

	private string KEY_ADMIN_TOOLS;
	private string KEY_MAP_MENU;
	private string KEY_TP_TO_CROSSHAIR;
	private string KEY_DELETE_ON_CROSSHAIR;
	private string KEY_TOGGLE_GODEMODE;

 	void VPPKeybindsConfig()
 	{
 		if ( FileExist(SAVE_PATH) )
 		{
 			JsonFileLoader<VPPKeybindsConfig>.JsonLoadFile(SAVE_PATH, this);
 		}else{
 			makeDefaults();
 		}
 	}

 	void ~VPPKeybindsConfig()
 	{

 	}

 	void saveConfig()
 	{
 		JsonFileLoader<VPPKeybindsConfig>.JsonSaveFile(SAVE_PATH, this);
 		//Reload after save.
 		JsonFileLoader<VPPKeybindsConfig>.JsonLoadFile(SAVE_PATH, this);
 	}

 	void makeDefaults()
 	{
 		KEY_ADMIN_TOOLS 		= "KC_HOME";
		KEY_MAP_MENU		    = "KC_M";
		KEY_TP_TO_CROSSHAIR 	= "KC_H";
		KEY_DELETE_ON_CROSSHAIR = "KC_BACK";
		KEY_TOGGLE_GODEMODE		= "KC_K";

		saveConfig();
 	}

 	string getActionKey(string actionName)
 	{
 		switch(actionName)
 		{
 			case "AdminTools":
 			return KEY_ADMIN_TOOLS;
 			break;

 			case "MapMenu":
			return KEY_MAP_MENU;
 			break;

 			case "CrosshairTP":
			return KEY_TP_TO_CROSSHAIR;
 			break;

 			case "CrosshairDelete":
 			return KEY_DELETE_ON_CROSSHAIR;
 			break;
			
			case "GodModeToggle":
 			return KEY_TOGGLE_GODEMODE;
 			break;
 		}
 		return "NULL";
 	}

 	int getKeyEnum(string keyBindName){
 		ref map<int,string> keyMap = g_Game.getKeyBindsMap();
 		int keyEnum;
 		string keyName;
		for(int i = 0; i < keyMap.Count(); i++){
			keyEnum = keyMap.GetKey(i);
			keyName = keyMap.Get(keyEnum);
			if(keyBindName == keyName){
				return keyEnum;
			}
		}
		return -1;
	}

 	int getActionKeyEnum(string actionName)
 	{
 		int result;
 		switch(actionName)
 		{
 			case "AdminTools":
			result = getKeyEnum( getActionKey("AdminTools") );
 			break;

 			case "MapMenu":
			result = getKeyEnum( getActionKey("MapMenu") );
 			break;

 			case "CrosshairTP":
			result = getKeyEnum( getActionKey("CrosshairTP") );
 			break;

 			case "CrosshairDelete":
 			result = getKeyEnum( getActionKey("CrosshairDelete") );
 			break;
			
			case "GodModeToggle":
 			result = getKeyEnum( getActionKey("GodModeToggle") );
 			break;
 		}
 		return result;
 	}

 	void setActionKey(string key, string actionName)
 	{
 		switch(actionName)
 		{
 			case "AdminTools":
			KEY_ADMIN_TOOLS = key;
 			break;

 			case "MapMenu":
			KEY_MAP_MENU = key;
 			break;

 			case "CrosshairTP":
			KEY_TP_TO_CROSSHAIR = key;
 			break;

 			case "CrosshairDelete":
 			KEY_DELETE_ON_CROSSHAIR = key;
 			break;
			
			case "GodModeToggle":
 			KEY_TOGGLE_GODEMODE = key;
 			break;
 		}
 		saveConfig();
 	}
}