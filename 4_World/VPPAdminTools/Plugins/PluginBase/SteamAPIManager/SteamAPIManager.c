class SteamAPIManager: ConfigurablePlugin
{
	private string STEAM_API_KEY;
	private float  QUERY_INTERVAL_SEC = 60.0; //seconds
	private string QUERY_PORT;
	private string SERVER_IP;

	[NonSerialized()]
	private ref Timer m_QueryTimer;
	[NonSerialized()]
	private string m_QueryPort;
	[NonSerialized()]
	ref VPPATSteamGameServer m_ServerDataCache;

	void SteamAPIManager()
	{
		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/SteamAPI.json";
	}

	override void OnInit()
	{
		if (!FileExist(JSONPATH))
		{
			GetSimpleLogger().Log("[SteamAPIManager] configuration file missing...creating default.");
			Save();
			return;
		}

		//load settings
		JsonFileLoader<SteamAPIManager>.JsonLoadFile(JSONPATH, this);
		g_Game.VSetServerIP(SERVER_IP);

		//Query port
		if (!QUERY_PORT || QUERY_PORT == string.Empty)
		{
			QUERY_PORT = GetGame().ServerConfigGetInt("steamQueryPort").ToString();
			if (!QUERY_PORT || QUERY_PORT == string.Empty)
			{
				GetSimpleLogger().Log("[SteamAPIManager] ERROR couldn't fetch server query port from configs...");
			}
		}

		//Fetch current server's IP, either from config, launch params or last resort via cURL
		if (SERVER_IP == string.Empty)
		{
			bool foundIp = GetCLIParam("ip", SERVER_IP);
	        if (!foundIp || SERVER_IP == string.Empty)
	        {
	        	//Resort to using cURL to get server's IP
	        	GetSimpleLogger().Log("[SteamAPIManager] WARNING couldn't fetch server IP from configs; using cURL...");
	        	VPPAT_IPIFY cb = new VPPAT_IPIFY();
				GetRestApi().GetRestContext("http://api.ipify.org/").GET(cb, "");
	        }
	        else
	        {
	        	g_Game.VSetServerIP(SERVER_IP);
	        }
		}

		if (STEAM_API_KEY != string.Empty && STEAM_API_KEY.Length() == 32)
		{
			//start timed task to query Steam
			m_QueryTimer = new Timer();
			if (QUERY_INTERVAL_SEC < 30.0)
			{
				GetSimpleLogger().Log("[SteamAPIManager] WARNING QUERY_INTERVAL_SEC is too short, minimum 30 seconds! Interval set to 30 seconds.");
				QUERY_INTERVAL_SEC = 30.0;
			}
			QueryServerData();
			m_QueryTimer.Run(QUERY_INTERVAL_SEC, this, "QueryServerData", NULL, true);
			GetSimpleLogger().Log("[SteamAPIManager] loaded configuration...query timer active! @ interval: " + QUERY_INTERVAL_SEC);
		}
		else
		{
			GetSimpleLogger().Log("[SteamAPIManager] ERROR No steam API key configured, and or key is invalid.");
		}
	}

	override void Save()
	{
		JsonFileLoader<SteamAPIManager>.JsonSaveFile(JSONPATH, this);
	}

	/*
	* Called by the timer setup with the interval of your choice, and or manually if you wish.
	* updates cached data about current server (players, pop, queue etc)
	*/
	void QueryServerData()
	{
		if (!g_Game.VGetServerIP() || g_Game.VGetServerIP() == string.Empty)
		{
			GetSimpleLogger().Log("[SteamAPIManager] ERROR Failed to query Steam for server data, invalid server IP: " + g_Game.VGetServerIP());
			return;
		}

		VPPATSteamAPICB steamCb = new VPPATSteamAPICB();
		string req = string.Format("?key=%1&filter=\\addr\\%2&limit=10", GetSteamAPIKey(), g_Game.VGetServerIP());
		GetRestApi().GetRestContext("https://api.steampowered.com/IGameServersService/GetServerList/v1/").GET(steamCb, req);
	}

	/*
	* Don't call this manually unless you wan't to reload the same data into Cache
	* Called by VPPATSteamAPICB after query was return with data successfully from Steam.
	*/
	void HandleSteamQueryCb()
	{
		VPPATSteamAPIResponse dta;
		JsonFileLoader<ref VPPATSteamAPIResponse>.JsonLoadFile(string.Format("$profile:GameServer_%1.json", g_Game.VGetServerIP()), dta);
		if (dta == NULL)
			return;

		array<ref VPPATSteamGameServer> servers = dta.response.servers;
		foreach(VPPATSteamGameServer server: servers)
		{
			if (server && server.addr == (string.Format("%1:%2", g_Game.VGetServerIP(), QUERY_PORT)))
			{
				m_ServerDataCache = server;
				g_Game.SetServerName(server.name);
				break;
			}
		}
	}

	string GetSteamAPIKey()
	{
		return STEAM_API_KEY;
	}

	string GetTotalPlayerCount()
	{
		array<Man> players = {};
		GetGame().GetWorld().GetPlayerList(players);

		string count  	= players.Count().ToString();
		string maxCount = GetGame().ServerConfigGetInt("maxPlayers").ToString();
		string result   = string.Format("%1/%2", count, maxCount);
		if (m_ServerDataCache && m_ServerDataCache.GetQueuedPlayers() > 0)
		{
			result += string.Format(" (+%1)", m_ServerDataCache.GetQueuedPlayers());
		}
		return result;
	}
};

SteamAPIManager GetSteamAPIManager()
{
	return SteamAPIManager.Cast(GetPluginManager().GetPluginByType(SteamAPIManager));
};