const int VPP_ADMIN_HUD 		  = 3215465487;
const int DRAG_TEST = 45454545454;


static bool VPPAT_IsModLoadded(string name)
{
	name.ToLower();
	int mod_count = GetGame().ConfigGetChildrenCount("CfgMods");
	for( int i = 2; i < mod_count; i++ )
	{
		string mod_name;
		GetGame().ConfigGetChildName("CfgMods", i, mod_name);
		mod_name.ToLower();
		if (mod_name == name)
			return true;
	}
	return false;
};

class VPPATProfileConstants
{
	//default
	const static float DEFAULT_CAM_SPEED 		 	= 5.0;
	const static float DEFAULT_CAM_BOOST 		 	= 10.0; //multiplier
	const static float DEFAULT_CAM_MOVE_DRAG   		= 0.7;
	const static float DEFAULT_CAM_MOUSE_SENSE 		= 0.6;
	const static float DEFAULT_CAM_SMOOTHNESS  		= 0.444;
	const static float DEFAULT_CAM_FOV 		 		= 1.0;

	//max
	const static float MAX_CAM_SPEED   				= 75.0;
	const static float MAX_CAM_BOOST 				= 30.0;
	const static float MAX_CAM_MOVE_DRAG 			= 0.5;
	const static float MAX_CAM_MOUSE_SENSE 			= 5.0;
	const static float MAX_CAM_SMOOTHNESS  			= 1.0;
	const static float MAX_CAM_FOV 		 			= 3.0;

	//min
	const static float MIN_CAM_SPEED   				= 0.01;
	const static float MIN_CAM_BOOST 				= 0.01;
	const static float MIN_CAM_MOVE_DRAG 			= 0.01;
	const static float MIN_CAM_MOUSE_SENSE 			= 0.01;
	const static float MIN_CAM_SMOOTHNESS  			= 0.01;
	const static float MIN_CAM_FOV 		 			= 0.01;
};

class VPPATUIConstants
{
	//Misc
	const static string VPPNotification 		 	 = "VPPAdminTools/GUI/Layouts/VPPNotification.layout";
	const static string VPPNotificationParent 		 = "VPPAdminTools/GUI/Layouts/VPPNotificationParent.layout";
	const static string PlayerESPCanvas 			 = "VPPAdminTools/GUI/Layouts/PlayerESPCanvas.layout";
	const static string CustomSubMenu 				 = "VPPAdminTools/GUI/Layouts/CustomSubMenu.layout";
	const static string InGameMenuButton 			 = "VPPAdminTools/GUI/Layouts/InGameMenuButton.layout";
	const static string SettingsMenuVPPATSection 	 = "VPPAdminTools/GUI/Layouts/SettingsMenuVPPATSection.layout";
	const static string SettingsMenuElement 		 = "VPPAdminTools/GUI/Layouts/SettingsMenuElement.layout";

	//Main toolbar
	const static string VPPAdminHud 				 = "VPPAdminTools/GUI/Layouts/AdminToolBar/VPPAdminHud.layout";
	const static string VPPButton 					 = "VPPAdminTools/GUI/Layouts/AdminToolBar/VPPButton.layout";

	//UI Helpers
	const static string VPPDialogBox 				 = "VPPAdminTools/GUI/Layouts/UIHelpers/VPPDialogBox.layout";
	const static string CustomGridSpacer 			 = "VPPAdminTools/GUI/Layouts/UIHelpers/CustomGridSpacer.layout";
	const static string VPPDragSelect 				 = "~NOT USED~";
	const static string VPPInfoBox 					 = "VPPAdminTools/GUI/Layouts/UIHelpers/VPPInfoBox.layout";
	const static string CustomInputMenu 			 = "VPPAdminTools/GUI/Layouts/UIHelpers/CustomInputMenu.layout";

	//Dropdown
	const static string dropdown_prefab 			 = "VPPAdminTools/GUI/Layouts/VPPDropdown/dropdown_prefab.layout";
	const static string dropdown_element 			 = "VPPAdminTools/GUI/Layouts/VPPDropdown/dropdown_element.layout";
	
	//XML Editor
	const static string MenuXMLEditor 				 = "VPPAdminTools/GUI/Layouts/XMLEditorUI/MenuXMLEditor.layout";
	const static string itemscanresultscreen 		 = "VPPAdminTools/GUI/Layouts/XMLEditorUI/itemscanresultscreen.layout";

	//Webhooks Manager
	const static string MenuWebHooks 				 = "VPPAdminTools/GUI/Layouts/WebHooksUI/MenuWebHooks.layout";
	const static string WebHookEntry 				 = "VPPAdminTools/GUI/Layouts/WebHooksUI/WebHookEntry.layout";

	//Weather Manager
	const static string MenuWeatherManager 			 = "VPPAdminTools/GUI/Layouts/WeatherManagerUI/MenuWeatherManager.layout";
	
	//Teleport Manager
	const static string MenuTeleportManager 		 = "VPPAdminTools/GUI/Layouts/TeleportManagerUI/MenuTeleportManager.layout";
	const static string PopUpCreatePosition 		 = "VPPAdminTools/GUI/Layouts/TeleportManagerUI/PopUpCreatePosition.layout";
	const static string TeleportEntry 				 = "VPPAdminTools/GUI/Layouts/TeleportManagerUI/TeleportEntry.layout";

	//Server Manager
	const static string MenuServerManager 			 = "VPPAdminTools/GUI/Layouts/ServerManagerUI/MenuServerManager.layout";

	//Players Manager
	const static string MenuPlayerManager 			 = "VPPAdminTools/GUI/Layouts/PlayerManagerUI/MenuPlayerManager.layout";
	const static string VPPPlayerList 				 = "VPPAdminTools/GUI/Layouts/PlayerManagerUI/VPPPlayerList.layout";
	const static string VPPPlayerInfoBox 			 = "VPPAdminTools/GUI/Layouts/PlayerManagerUI/VPPPlayerInfoBox.layout";

	//Permissions Manager
	const static string MenuPermissionsEditor 		 = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/MenuPermissionsEditor.layout";
	const static string VPPPermissionBox 			 = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/VPPPermissionBox.layout";
	const static string VPPUserBox 					 = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/VPPUserBox.layout";
	const static string VPPUserGroupBox 			 = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/VPPUserGroupBox.layout";
	const static string SettingsPopUp 				 = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/SettingsPopUp.layout";

	//Objects builder
	const static string MenuObjectManager 			 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/MenuObjectManager.layout";
	const static string EditDialogBuilding 			 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/EditDialogBuilding.layout";
	const static string BuildingEntry 				 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingEntry.layout";
	const static string EditDialog 					 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/EditDialog.layout";
	const static string BuildingSetEntry 			 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingSetEntry.layout";
	const static string BuildingTracker 			 = "VPPAdminTools/GUI/Layouts/ObjectManagerUI/BuildingTracker.layout";

	//Items Manager
	const static string MenuItemManager 			 = "VPPAdminTools/GUI/Layouts/ItemManagerUI/MenuItemManager.layout";
	const static string PopUpCreatePreset 			 = "VPPAdminTools/GUI/Layouts/ItemManagerUI/PopUpCreatePreset.layout";
	const static string EntryPresetItem 			 = "VPPAdminTools/GUI/Layouts/ItemManagerUI/EntryPresetItem.layout";

	//ESP Tools
	const static string EspToolsMenu 				 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspToolsMenu.layout";
	const static string EspItemEntry 				 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspItemEntry.layout";
	const static string EspTracker 					 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTracker.layout";
	const static string EspTrackerDetailed 			 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerDetailed.layout";
	const static string EspTrackerPanelHeader 		 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerPanelHeader.layout";
	const static string EspTrackerInfos 			 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerInfos.layout";
	const static string EspTrackerBaseBuilding 		 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerBaseBuilding.layout";
	const static string EspTrackerActions 			 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerActions.layout";
	const static string EspTrackerPlayerHealth 		 = "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerPlayerHealth.layout";
	const static string FilterEntry 				 = "VPPAdminTools/GUI/Layouts/EspToolsUI/FilterEntry.layout";

	//Commands console menu
	const static string CommandsConsoleUI 			 = "VPPAdminTools/GUI/Layouts/CommandsConsoleUI/CommandsConsoleUI.layout";
	const static string TextElement 				 = "VPPAdminTools/GUI/Layouts/CommandsConsoleUI/TextElement.layout";

	//Bans Manager
	const static string MenuBansManager 			 = "VPPAdminTools/GUI/Layouts/BansManagerUI/MenuBansManager.layout";
	const static string BanDurationInputPopUp 		 = "VPPAdminTools/GUI/Layouts/BansManagerUI/BanDurationInputPopUp.layout";

	//Spectate Tools
	const static string MenuSpectateTools 			 = "VPPAdminTools/GUI/Layouts/SpectateUI/MenuSpectateTools.layout";
};