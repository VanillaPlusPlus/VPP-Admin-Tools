/*
* Player list is synchronized between server and client(s) based on server events
* (join, leave, respawn)
* when an administrator client joins/respawns in the server, they are added as a RECEIVER and added into PLAYERS.
* The entire list of PLAYERS_CLIENT is then sent to the joining/respawning administrator client.
* On the event of a regular player leaving/joining/respawn their entry is updated and synchronized with all RECEIVERS
* RECEIVERS are administrators
* PLAYERS / PLAYERS_CLIENT is everyone
*/

class PlayerListManager
{
	static ref PlayerListManager m_Instance = new PlayerListManager();
	static ref map<string, PlayerIdentity> PLAYERS; 	//Bohemia UID, Identity ref (server only)
	static ref map<string, ref VPPUser> PLAYERS_CLIENT; //sending this via RPC to clients (client + server)
	static ref array<PlayerIdentity> RECEIVERS;			//hold onto administrator identities (server only)

	static bool InitMaps = Init();
	static bool Init()
	{
		PLAYERS = new map<string, PlayerIdentity>;
		PLAYERS_CLIENT = new map<string, ref VPPUser>;
		RECEIVERS = {};
		return true;
	}

	void PlayerListManager()
	{
		m_Instance = this;
		g_Game.Event_OnRPC.Insert(OnRPC);
	}

	void ~PlayerListManager()
	{
		if (g_Game)
			g_Game.Event_OnRPC.Remove(OnRPC);
	}

	/*
	* RPC handler
	*/
	void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
#ifdef SERVER

#else
		if (rpc_type == VPPATRPCs.RPC_SYNC_PLAYER_LIST)
		{
			map<string, VPPUser> tmp = new map<string, VPPUser>;
			if (!ctx.Read(tmp))
			{
				Print("VPPATRPCs.RPC_SYNC_PLAYER_LIST ~ FAIELD ~");
				return;
			}

			PlayerListManager.PLAYERS_CLIENT.Clear();
			PlayerListManager.PLAYERS_CLIENT.Copy(tmp);
			PlayerListManager.DebugListClient();
		}

		string uid;
		if (rpc_type == VPPATRPCs.RPC_PLAYERLIST_ADD)
		{
			ref VPPUser user;
			if (!ctx.Read(uid) || !ctx.Read(user))
			{
				Print("VPPATRPCs.RPC_PLAYERLIST_ADD: error unable to break down RPC data!");
				return;
			}

			bool added = AddEntryClient(uid, user);
			Print(string.Format("VPPATRPCs.RPC_PLAYERLIST_ADD: %1 %2 %3", uid, user.GetUserName(), added));
		}

		if (rpc_type == VPPATRPCs.RPC_PLAYERLIST_REMOVE)
		{
			if (ctx.Read(uid))
			{
				bool removed = RemoveEntryClient(uid);
				Print("VPPATRPCs.RPC_PLAYERLIST_REMOVE: removed ► " + uid);
			}
		}
#endif
	}

	/*
	* (client)
	*/
	bool AddEntryClient(string uid, VPPUser data)
	{
		if (PlayerListManager.PLAYERS_CLIENT.Contains(uid))
		{
			PlayerListManager.PLAYERS_CLIENT.Set(uid, data);
			return true;
		}

		PlayerListManager.PLAYERS_CLIENT.Insert(uid, data);
		return true;
	}

	/*
	* (client)
	*/
	bool RemoveEntryClient(string uid)
	{
		if (PlayerListManager.PLAYERS_CLIENT.Contains(uid))
		{
			PlayerListManager.PLAYERS_CLIENT.Remove(uid);
			return true;
		}
		return false;
	}

	/*
	* Give the player count from what World module has. 
	* Best used to determine if our list is out of sync somehow
	*/
	int GetCountActual()
	{
		array<Man> players = new array<Man>;
       	GetGame().GetWorld().GetPlayerList(players);
       	return players.Count();
	}

	int GetCount()
	{
		if (GetGame().IsDedicatedServer() && GetGame().IsMultiplayer())
		{
			return PlayerListManager.PLAYERS.Count();
		}
		return PlayerListManager.PLAYERS_CLIENT.Count();
	}
	
	array<ref VPPUser> GetUsers()
	{
		//On server only
		if (GetGame().IsDedicatedServer() && GetGame().IsMultiplayer())
		{
			if (PLAYERS_CLIENT.Count() != GetCountActual())
				PlayerListManager.BuildList(); 
		}

		array<ref VPPUser> elements = {};
		for (int i = 0; i < PlayerListManager.PLAYERS_CLIENT.Count(); i++)
		{
			elements.Insert(PlayerListManager.PLAYERS_CLIENT.GetElement(i));
		}

		return elements;
	}

	/*
	* Works ONLY with Bohemia ID
	* Server / Client supported
	*/
	bool HasPlayerInList(string id)
	{
		if (GetGame().IsDedicatedServer() && GetGame().IsMultiplayer())
		{
			return PlayerListManager.PLAYERS[id] != NULL;
		}
		return PlayerListManager.PLAYERS_CLIENT[id] != NULL;
	}

//start-server-methods
	static void AddReceiver(notnull PlayerIdentity identity)
	{
		if (RECEIVERS.Find(identity) > -1)
			return; //duplicate check.

		RECEIVERS.Insert(identity);
		Print("[PlayerListManager] AddReceiver ► added: " + identity);
	}

	static void RemoveReceiver(notnull PlayerIdentity identity)
	{
		int index = RECEIVERS.Find(identity);
		if (identity && index > -1)
		{
			RECEIVERS.RemoveOrdered(index);
			Print("[PlayerListManager] RemoveReceiver ► removed: " + identity);
		}
	}

	/*
	* Add entry, doesn't handle sync!
	* if already exists, replaced by new ptr of identity.
	*/
	static bool AddEntry(string uid, PlayerIdentity identity = NULL)
	{
		if (PLAYERS.Contains(uid))
		{
			PLAYERS.Set(uid, identity);
			return true;
		}

		PLAYERS.Insert(uid, identity);
		return true;
	}

	/*
	* remove entry, handles sync automatically.
	*/
	static bool RemoveEntry(string uid, bool ignoreNull = false)
	{
		if (PLAYERS.Contains(uid) && (ignoreNull || PLAYERS[uid] == NULL))
		{
			PLAYERS.Remove(uid);
			PlayerListManager.SyncEntryToReceivers(uid);
			return true;
		}
		return false;
	}

	/*
	* remove entry by PlayerIdentity, handles sync automatically.
	*/
	static bool RemoveEntry(PlayerIdentity identity)
	{
		if (identity && PLAYERS.Contains(identity.GetId()))
		{
			PLAYERS.Remove(identity.GetId());
			PlayerListManager.SyncEntryToReceivers(identity.GetId());
			return true;
		}
		return false;
	}

	static void BuildList()
	{
		if (PLAYERS.Count() <= 0)
		{
			//fail-safe measure incase PLAYERS was wiped, rebuild one from world array
			array<Man> players = new array<Man>;
       		GetGame().GetWorld().GetPlayerList(players);
       		for (int i = 0; i < players.Count(); ++i)
       		{
       			PLAYERS.Insert(players[i].GetIdentity().GetId(), players[i].GetIdentity());
       		}
		}

		PLAYERS_CLIENT.Clear();
		foreach(string uid, PlayerIdentity identity : PLAYERS)
		{
			PLAYERS_CLIENT.Insert(uid, new VPPUser(identity.GetName(), identity.GetPlainId(), identity.GetPlayerId()));
		}
	}

	/*
	* Sends entire Sync list to a specified client via RPC
	*/
	static void SyncListToClient(notnull PlayerIdentity identity)
	{
		if (!identity)
			return;

		BuildList();

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(PLAYERS_CLIENT);
		rpc.Send(NULL, VPPATRPCs.RPC_SYNC_PLAYER_LIST, true, identity);
		delete rpc;
	}

	/*
	* Sends an entry of a player by using uid to all RECEIVERS clients
	* Automatically determines weather to send an ADD or REMOVE RPC
	* @Param0: Bohemia ID of the player you want to sync to all RECEIVERS.
	*/
	static bool SyncEntryToReceivers(string uid)
	{
		if (RECEIVERS.Count() <= 0)
			return false;

		if (PLAYERS_CLIENT.Count() != GetPlayerListManager().GetCountActual())
			BuildList();

		int rpc_type = VPPATRPCs.RPC_PLAYERLIST_ADD;
		PlayerIdentity identity = PLAYERS[uid];
		
		if (!PLAYERS.Contains(uid) || !identity)
		{
			Print("[PlayerListManager] SyncEntryToReceivers ► uid: " + uid + " isn't in the players list! sending a remove.");
			rpc_type = VPPATRPCs.RPC_PLAYERLIST_REMOVE;
		}

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(uid);

		if (identity && rpc_type == VPPATRPCs.RPC_PLAYERLIST_ADD)
		{
			if (!PLAYERS_CLIENT[uid])
			{
				Print("[PlayerListManager] SyncEntryToReceivers:: ERROR! unable to fetch VPPUser ► " + uid);
				return false;
			}
			rpc.Write(PLAYERS_CLIENT[uid]);
		}
		
		for (int i = 0; i < RECEIVERS.Count(); ++i)
		{
			if (!RECEIVERS[i])
				continue;

			rpc.Send(NULL, rpc_type, true, RECEIVERS[i]);
		}

		delete rpc;
		return true;
	}
//end-server-methods

	static void DebugListServer()
	{
		Print("[PlayerListManager] ~debug~\n");
		foreach(string uid, PlayerIdentity identity : PLAYERS)
		{
			Print(string.Format("\t ► uid: %1 identity: %2", uid, identity));
		}
		Print("[PlayerListManager] Total entries in map: " + PLAYERS.Count());
	}

	static void DebugListClient()
	{
		Print("[PlayerListManager] ~debug~\n");
		foreach(string uid, VPPUser data : PLAYERS_CLIENT)
		{
			Print(string.Format("\t ► uid: %1 data: %2 %3 %4", uid, data.GetUserId(), data.GetUserName(), data.GetSessionId()));
		}
		Print("[PlayerListManager] Total entries in map: " + PLAYERS_CLIENT.Count());
	}
};

PlayerListManager GetPlayerListManager()
{
    return PlayerListManager.m_Instance;
};