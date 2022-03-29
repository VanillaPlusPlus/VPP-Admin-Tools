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
	//the root directory layouts exists in
	const static string PREFIX = "VPPAdminTools/GUI/Layouts/";
	
	//Misc
	const static string VPPNotification 		 	 = PREFIX + "VPPNotification.layout";
	const static string VPPNotificationParent 		 = PREFIX + "VPPNotificationParent.layout";
	const static string PlayerESPCanvas 			 = PREFIX + "PlayerESPCanvas.layout";
	const static string CustomSubMenu 				 = PREFIX + "CustomSubMenu.layout";
	const static string InGameMenuButton 			 = PREFIX + "InGameMenuButton.layout";
	const static string SettingsMenuVPPATSection 	 = PREFIX + "SettingsMenuVPPATSection.layout";
	const static string SettingsMenuElement 		 = PREFIX + "SettingsMenuElement.layout";

	//Main toolbar
	const static string VPPAdminHud 				 = PREFIX + "AdminToolBar/VPPAdminHud.layout";
	const static string VPPButton 					 = PREFIX + "AdminToolBar/VPPButton.layout";

	//UI Helpers
	const static string VPPDialogBox 				 = PREFIX + "UIHelpers/VPPDialogBox.layout";
	const static string CustomGridSpacer 			 = PREFIX + "UIHelpers/CustomGridSpacer.layout";
	const static string VPPDragSelect 				 = PREFIX + "UIHelpers/VPPDragSelect.layout";
	const static string VPPInfoBox 					 = PREFIX + "UIHelpers/VPPInfoBox.layout";
	const static string CustomInputMenu 			 = PREFIX + "UIHelpers/CustomInputMenu.layout";

	//Dropdown
	const static string dropdown_prefab 			 = PREFIX + "VPPDropdown/dropdown_prefab.layout";
	const static string dropdown_element 			 = PREFIX + "VPPDropdown/dropdown_element.layout";
	
	//XML Editor
	const static string MenuXMLEditor 				 = PREFIX + "XMLEditorUI/MenuXMLEditor.layout";
	const static string itemscanresultscreen 		 = PREFIX + "XMLEditorUI/itemscanresultscreen.layout";

	//Webhooks Manager
	const static string MenuWebHooks 				 = PREFIX + "WebHooksUI/MenuWebHooks.layout";
	const static string WebHookEntry 				 = PREFIX + "WebHooksUI/WebHookEntry.layout";

	//Weather Manager
	const static string MenuWeatherManager 			 = PREFIX + "WeatherManagerUI/MenuWeatherManager.layout";
	
	//Teleport Manager
	const static string MenuTeleportManager 		 = PREFIX + "TeleportManagerUI/MenuTeleportManager.layout";
	const static string PopUpCreatePosition 		 = PREFIX + "TeleportManagerUI/PopUpCreatePosition.layout";
	const static string TeleportEntry 				 = PREFIX + "TeleportManagerUI/TeleportEntry.layout";

	//Server Manager
	const static string MenuServerManager 			 = PREFIX + "ServerManagerUI/MenuServerManager.layout";

	//Players Manager
	const static string MenuPlayerManager 			 = PREFIX + "PlayerManagerUI/MenuPlayerManager.layout";
	const static string VPPPlayerList 				 = PREFIX + "PlayerManagerUI/VPPPlayerList.layout";
	const static string VPPPlayerInfoBox 			 = PREFIX + "PlayerManagerUI/VPPPlayerInfoBox.layout";

	//Permissions Manager
	const static string MenuPermissionsEditor 		 = PREFIX + "PermissionManagerUI/MenuPermissionsEditor.layout";
	const static string VPPPermissionBox 			 = PREFIX + "PermissionManagerUI/VPPPermissionBox.layout";
	const static string VPPUserBox 					 = PREFIX + "PermissionManagerUI/VPPUserBox.layout";
	const static string VPPUserGroupBox 			 = PREFIX + "PermissionManagerUI/VPPUserGroupBox.layout";
	const static string SettingsPopUp 				 = PREFIX + "PermissionManagerUI/SettingsPopUp.layout";

	//Objects builder
	const static string MenuObjectManager 			 = PREFIX + "ObjectManagerUI/MenuObjectManager.layout";
	const static string EditDialogBuilding 			 = PREFIX + "ObjectManagerUI/EditDialogBuilding.layout";
	const static string BuildingEntry 				 = PREFIX + "ObjectManagerUI/BuildingEntry.layout";
	const static string EditDialog 					 = PREFIX + "ObjectManagerUI/EditDialog.layout";
	const static string BuildingSetEntry 			 = PREFIX + "ObjectManagerUI/BuildingSetEntry.layout";
	const static string BuildingTracker 			 = PREFIX + "ObjectManagerUI/BuildingTracker.layout";

	//Items Manager
	const static string MenuItemManager 			 = PREFIX + "ItemManagerUI/MenuItemManager.layout";
	const static string PopUpCreatePreset 			 = PREFIX + "ItemManagerUI/PopUpCreatePreset.layout";
	const static string EntryPresetItem 			 = PREFIX + "ItemManagerUI/EntryPresetItem.layout";

	//ESP Tools
	const static string EspToolsMenu 				 = PREFIX + "EspToolsUI/EspToolsMenu.layout";
	const static string EspItemEntry 				 = PREFIX + "EspToolsUI/EspItemEntry.layout";
	const static string EspTracker 					 = PREFIX + "EspToolsUI/EspTracker.layout";
	const static string EspTrackerDetailed 			 = PREFIX + "EspToolsUI/EspTrackerDetailed.layout";
	const static string EspTrackerPanelHeader 		 = PREFIX + "EspToolsUI/EspTrackerPanelHeader.layout";
	const static string EspTrackerInfos 			 = PREFIX + "EspToolsUI/EspTrackerInfos.layout";
	const static string EspTrackerBaseBuilding 		 = PREFIX + "EspToolsUI/EspTrackerBaseBuilding.layout";
	const static string EspTrackerActions 			 = PREFIX + "EspToolsUI/EspTrackerActions.layout";
	const static string EspTrackerPlayerHealth 		 = PREFIX + "EspToolsUI/EspTrackerPlayerHealth.layout";
	const static string FilterEntry 				 = PREFIX + "EspToolsUI/FilterEntry.layout";

	//Commands console menu
	const static string CommandsConsoleUI 			 = PREFIX + "CommandsConsoleUI/CommandsConsoleUI.layout";
	const static string TextElement 				 = PREFIX + "CommandsConsoleUI/TextElement.layout";

	//Bans Manager
	const static string MenuBansManager 			 = PREFIX + "BansManagerUI/MenuBansManager.layout";
	const static string BanDurationInputPopUp 		 = PREFIX + "BansManagerUI/BanDurationInputPopUp.layout";
};