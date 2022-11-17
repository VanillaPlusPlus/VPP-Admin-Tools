modded class PluginManager
{
	void PluginManager()
	{
		Print("Vanilla++ AdminTools Plugins Registering..");
	}

	override void Init()
	{
		super.Init();
		//----------------------------------------------------------------------
		// 						   Register modules
		//----------------------------------------------------------------------
		//		        Module Class Name 				Client	Server
		//----------------------------------------------------------------------

		if (GetGame().IsClient() || !GetGame().IsMultiplayer())
		{
			RegisterPlugin("VPPKeybindsManager",		 true,      false); //Client only
			RegisterPlugin("VPPUIManager",			     true,      false); //Client only
		}

#ifndef SERVER
	#ifdef DIAG_DEVELOPER //offline client Diag only
		if ((GetGame().IsMultiplayer() && GetGame().IsDedicatedServer()) || (!GetGame().IsMultiplayer() && !GetGame().IsDedicatedServer()))
	#endif
#else
		if (GetGame().IsDedicatedServer()) //Otherwise server only
#endif
		{
			RegisterPlugin("XMLEditor", 			 		 false, 	true);
			RegisterPlugin("PluginFileHandler", 			 false, 	true);
			RegisterPlugin("VPPLogManager",					 false,		true);
			RegisterPlugin("PermissionManager",			 	 false, 	true);
			RegisterPlugin("BansManager",			 	 	 false, 	true);
			RegisterPlugin("WebHooksManager",				 false,		true);
			RegisterPlugin("AdminTools",					 false, 	true);
			RegisterPlugin("ChatCommandManager",			 false, 	true);
			RegisterPlugin("WeatherManager", 				 false,     true);
			RegisterPlugin("TimeManager", 					 false,     true);
			RegisterPlugin("PlayerManager", 				 false,     true);
			RegisterPlugin("ServerManager", 				 false,     true);
			RegisterPlugin("TeleportManager",				 false,		true);
			RegisterPlugin("VPPItemManager",		         false,		true);
			RegisterPlugin("VPPESPTools",		             false,		true);
			RegisterPlugin("BuildingSetManager", 			 false, 	true);
			RegisterPlugin("SteamAPIManager", 			 	 false, 	true);

			MakeDirectory("$profile:VPPAdminTools");
			MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins");
			MakeDirectory("$profile:VPPAdminTools/Exports");
			MakeDirectory("$profile:VPPAdminTools/Backups");
		}
	}
};
