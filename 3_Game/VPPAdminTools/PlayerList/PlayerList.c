/*
* Player list is synchronized between server and client(s) based on server events
* (join, leave, respawn)
* when an administrator client joins/respawns in the server, they are added as a RECEIVER and added into PLAYERS.
* The entire list of PLAYERS_CLIENT is then sent to the joining/respawning administrator client.
* On the event of a regular player leaving/joining/respawn their entry is updated and synchronized with all RECEIVERS
* RECEIVERS are administrators
* PLAYERS / PLAYERS_CLIENT is everyone
*/

class VPPATPlayerList
{
	static bool InitMaps = Init();

	static ref VPPATPlayerList m_Instance = new VPPATPlayerList();
	static ref map<string, PlayerIdentity> PLAYERS; 	//Bohemia UID, Identity ref (server only)
	static ref map<string, ref VPPUser> PLAYERS_CLIENT; //sending this via RPC to clients (client + server)
	static ref array<PlayerIdentity> RECEIVERS;			//hold onto administrator identities (server only)

	static bool Init()
	{
		PLAYERS = new map<string, PlayerIdentity>;
		PLAYERS_CLIENT = new map<string, ref VPPUser>;
		RECEIVERS = {};
		return true;
	}

	void VPPATPlayerList()
	{
		g_Game.Event_OnRPC.Insert(OnRPC);
	}

	void ~VPPATPlayerList()
	{
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
			VPPATPlayerList.PLAYERS_CLIENT.Clear();
			if (!ctx.Read(VPPATPlayerList.PLAYERS_CLIENT))
			{
				Print("VPPATRPCs.RPC_SYNC_PLAYER_LIST ~ FAIELD ~");
				return;
			}

			VPPATPlayerList.DebugListClient();
		}

		string uid;

		if (rpc_type == VPPATRPCs.RPC_PLAYERLIST_ADD)
		{
			VPPUser data;

			ctx.Read(uid);
			ctx.Read(data);

			if (uid == string.Empty || !data)
			{
				Print("VPPATRPCs.RPC_PLAYERLIST_ADD: took a shite");
				return;
			}

			bool added = AddEntryClient(uid, data);
			Print(string.Format("VPPATRPCs.RPC_PLAYERLIST_ADD: %1 %2 %3", uid, data, added));
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
		if (VPPATPlayerList.PLAYERS_CLIENT.Contains(uid))
		{
			VPPATPlayerList.PLAYERS_CLIENT.Set(uid, data);
			return true;
		}

		VPPATPlayerList.PLAYERS_CLIENT.Insert(uid, data);
		return true;
	}

	/*
	* (client)
	*/
	bool RemoveEntryClient(string uid)
	{
		if (VPPATPlayerList.PLAYERS_CLIENT.Contains(uid))
		{
			VPPATPlayerList.PLAYERS_CLIENT.Remove(uid);
			return true;
		}
		return false;
	}

//start-server-methods
	static void AddReceiver(notnull PlayerIdentity identity)
	{
		if (RECEIVERS.Find(identity) > -1)
			return; //duplicate check.

		RECEIVERS.Insert(identity);
		Print("[VPPATPlayerList] AddReceiver ► added: " + identity);
	}

	static void RemoveReceiver(notnull PlayerIdentity identity)
	{
		int index = RECEIVERS.Find(identity);
		if (identity && index > -1)
		{
			RECEIVERS.RemoveOrdered(index);
			Print("[VPPATPlayerList] RemoveReceiver ► removed: " + identity);
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
			VPPATPlayerList.SyncEntryToReceivers(uid);
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
			VPPATPlayerList.SyncEntryToReceivers(identity.GetId());
			return true;
		}
		return false;
	}

	static void BuildList()
	{
		if (PLAYERS.Count() <= 0)
		{
			Error("[VPPATPlayerList] BuildList() ► PLAYERS list is empty!");
			return;
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

		int rpc_type = VPPATRPCs.RPC_PLAYERLIST_ADD;
		PlayerIdentity identity = PLAYERS[uid];
		
		if (!PLAYERS.Contains(uid) || !identity)
		{
			Print("[VPPATPlayerList] SyncEntryToReceivers ► uid: " + uid + " isn't in the players list! sending a remove.");
			rpc_type = VPPATRPCs.RPC_PLAYERLIST_REMOVE;
		}

		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(uid);

		if (identity && rpc_type == VPPATRPCs.RPC_PLAYERLIST_ADD)
		{
			rpc.Write(new VPPUser(identity.GetName(), identity.GetPlainId(), identity.GetPlayerId()));
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
		Print("[VPPATPlayerList] ~debug~\n");
		foreach(string uid, PlayerIdentity identity : PLAYERS)
		{
			Print(string.Format("\t ► uid: %1 identity: %2", uid, identity));
		}
		Print("[VPPATPlayerList] Total entries in map: " + PLAYERS.Count());
	}

	static void DebugListClient()
	{
		Print("[VPPATPlayerList] ~debug~\n");
		foreach(string uid, VPPUser data : PLAYERS_CLIENT)
		{
			Print(string.Format("\t ► uid: %1 data: %2 %3 %4", uid, data.GetUserId(), data.GetUserName(), data.GetSessionId()));
		}
		Print("[VPPATPlayerList] Total entries in map: " + PLAYERS_CLIENT.Count());
	}
};