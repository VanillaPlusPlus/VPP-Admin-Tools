class VPPATSteamAPIResponse
{
	ref VPPATSteamAPIData response;
	void VPPATSteamAPIResponse();
};

class VPPATSteamAPIData
{
	ref array<ref VPPATSteamGameServer> servers;
	void VPPATSteamAPIData();
};

class VPPATSteamGameServer
{
	string addr;
	int    gameport;
	string steamid;
	string name;
	int    appid;
	string gamedir;
	string version;
	string product;
	int    region;
	int    players;
	int    max_players;
	int    bots;
	string Map;
	string gametype;
	
	int GetQueuedPlayers()
	{
		string temp = gametype;
		temp.Replace("battleye,external,privHive,shard,", "");
		temp.Replace("battleye,no3rd,external,privHive,shard,", "");
		temp = temp.Substring(3,3);
		return temp.ToInt();
	}
};