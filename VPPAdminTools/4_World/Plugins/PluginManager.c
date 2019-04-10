modded class PluginManager
{
	private ref array<ref VPPConfig> m_ModConfigs = new array<ref VPPConfig>;

	void PluginManager()
	{
		Print("Vanilla++ Plugins....getting Registered");
	}

	void Init()
	{
		super.Init();
		//----------------------------------------------------------------------
		// 						   Register modules
		//----------------------------------------------------------------------
		//		        Module Class Name 				Client	Server
		//----------------------------------------------------------------------
		RegisterPlugin( "PermitManager",			    false, 	true );
		RegisterPlugin( "AdminTools",					false, 	true );
		RegisterPlugin( "ChatBasedAdminTool",			false, 	true );
	}

	override void RegisterPlugin(string plugin_class_name, bool reg_on_client, bool reg_on_server, bool reg_on_release = true)
	{
		super.RegisterPlugin(plugin_class_name,reg_on_client, reg_on_server, reg_on_release);
		if ( GetGame().IsMultiplayer() )
		{
			if ( GetGame().IsServer() )
			{
				switch(plugin_class_name)
				{
					case "PermitManager":
					m_ModConfigs.Insert( new AdminCaches);
					break;

					case "AdminTools":
					m_ModConfigs.Insert( new VPPTeleportLocations);
					break;

					case "ChatBasedAdminTool":
					Print("no config for ChatBasedAdminTools");
					break;
				}
			}
		}
	}

	override void PluginsInit()
	{
		super.PluginsInit();
	    LoadConfigs();
	}

	ref VPPConfig GetConfigByType(typename configType)
	{
	    for (int i = 0; i < m_ModConfigs.Count(); ++i)
	    {
	    	ref VPPConfig config = m_ModConfigs.Get(i);
	    	if (config.getType() == configType){
	    		return m_ModConfigs.Get(i);
	    	}
	    }
		return NULL;
	}

    void LoadConfigs()
    {
    	foreach(VPPConfig modconfigs : m_ModConfigs){
    		modconfigs.load();
    	}
    }
};
