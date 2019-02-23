#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\VPPModuleManager.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Tunables.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\AdminTool\\PermitManager.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\AdminTool\\AdminTool.c"

#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\AdminTool\\AdminTools.c"

#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\AdvancedLoadouts\\AdvancedLoadouts.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\CustomBuildings\\BuildingSpawner.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\SafeZone\\SafeZoneFunctions.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\InfectedHordes\\InfectedHordesSpawner.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerMission\\SpawnPointsManager.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerMission\\SpawnCoolDownSystem.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerMission\\DEXML.c"
#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerEvents\\VPPSupplyDrop.c"

class DayZSurvival : MissionServer
{
	private ref set<ref VPPModuleManager> m_Modules;
	private ref VPPConfigManager m_ConfigManager;
	protected float m_LogInTimerLength = GetGame().ServerConfigGetInt("LoginTime");     //The timer for players who join the server  (in seconds)
	bool m_StaminaStatus = false;

	string m_SpawnPoint = "";
	string m_SkinStr = "";
	vector m_pos = "0 0 0";

	ref DEXML m_DEXML;

	ref map<int, string> m_Gear;
	ref array<string> m_ScriptFiles;

	void DayZSurvival()
	{
		Print("VANILLA++ IS ALIVE!!");
		//=============RPC's====================
		GetRPCManager().AddRPC( "RPC_SpawnStartKit", "SpawnStartKit", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_CoolDownHandler", "CoolDownHandler", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_GetSpawnVectors", "GetSpawnVectors", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_GetLobbyTime", "GetLobbyTime", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ReadFromXML", "GetTypesXML", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_ModSettings", "GetSettings", this, SingeplayerExecutionType.Server );
		//======================================

		m_DEXML = new DEXML;
		m_Modules = new set<ref VPPModuleManager>;
		RegisterModules();
		m_ConfigManager = new VPPConfigManager(this, m_Modules);
		//UpdateList();
	}

	void ~DayZSurvival()
	{
		
	}

	ref VPPConfig GetConfigByType(typename configType)
	{
		for ( int i = 0; i < m_ConfigManager.GetConfigArray().Count(); ++i)
		{
			VPPConfig config = m_ConfigManager.GetConfigArray().Get(i);
			if (config.getType() == configType) 
			{
				return config;
			}
		}
		return NULL;
	}

	void GetSettings( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
        if (type == CallType.Server)
        {
			Param2<bool,bool> param;
			if (sender != NULL)
			{
				if (VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).loadOutsType() == "STATIC_LOADOUTS")
				{
					//Static Loadouts
					param = new Param2<bool,bool>>(true,VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).isForcedRandomSpawns());
       	 			GetRPCManager().SendRPC( "RPC_SortMenuType", "SortMenuType", param, true, sender);
				}
				else
				{
					//Random Loadouts
					param = new Param2<bool,bool>>(false,VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).isForcedRandomSpawns());
       	 			GetRPCManager().SendRPC( "RPC_SortMenuType", "SortMenuType", param, true, sender);
				}
			}
        }
	}

	void GetTypesXML( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		//string ItemClassName, string FieldCatagory
		Param2<string,string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
			m_DEXML.ReadFromXML(data.param1,data.param2,sender);
        }
	}

	void UpdateList()
    {
       	m_ScriptFiles = new array<string>;

		FindFileHandle FileSearch;
		string SearchDirectoy = "@VanillaPlusPlus";

		string FolderName;
		string FullPath;
		int 	file_attr;
		int		flags;

		if (FileExist(SearchDirectoy))
		{
			string path_find_pattern = SearchDirectoy + "\\*"; // [/*] [\\*] [//*/]

			FileSearch = FindFile(path_find_pattern, FolderName, file_attr, FindFileFlags.DIRECTORIES);

			bool found = true;
			while (found)
			{
				if ( file_attr == FileAttr.DIRECTORY )
				{
					FullPath = SearchDirectoy + "/" + FolderName;
					GetFileFromFolder(FullPath);
				}
				found = FindNextFile(FileSearch, FolderName, file_attr);
			}
			CloseFindFile(FileSearch);
			//results
			for (int i = 0; i < m_ScriptFiles.Count(); ++i)
		    {
		    	Print("[FileSearch] Total Located: "+m_ScriptFiles.Count() + " ScriptFilePath: "+m_ScriptFiles.Get(i));
		    }
		}
		else
		{
			Print("ModdedScripts Folder Missing!");
		}
    }

    void GetFileFromFolder(string FolderPath)
    {
    	FindFileHandle FileSearch;
		string SearchDirectoy = "@VanillaPlusPlus";

		string FileName;
		string FullPath;
		int 	file_attr;
		int		flags;

		if (FileExist(FolderPath))
		{
			string path_find_pattern = FolderPath + "\\*"; // [/*] [\\*] [//*/]

			FileSearch = FindFile(path_find_pattern, FileName, file_attr, FindFileFlags.DIRECTORIES);

			bool found = true;
			while (found)
			{
				if ( file_attr != FileAttr.DIRECTORY )
				{
					FullPath = FolderPath + "/" + FileName;
					m_ScriptFiles.Insert(FullPath);
				}
				else if ( file_attr == FileAttr.DIRECTORY )
				{
					FullPath = FolderPath + "/" + FileName;
					GetFileFromFolder(FullPath);
				}
				found = FindNextFile(FileSearch, FileName, file_attr);
			}
			CloseFindFile(FileSearch);
		}
    }

	ScriptModule GetScriptModule()
	{
		return GetGame().GetMission().MissionScript;
	}

	void RegisterModules()
	{
		m_Modules.Insert(new ModTunables(this));
		m_Modules.Insert(new SpawnPointsManager(this));
		m_Modules.Insert(new SpawnCoolDownSystem(this));

		//Test
		m_Modules.Insert(new VPPSupplyDrop(this));
		//----
		if (ModTunables.Cast(GetModule(ModTunables)).IsActive("AdminTools"))
		{
			m_Modules.Insert(new PermitManager(this));
			m_Modules.Insert(new AdminTool(this));

			m_Modules.Insert(new AdminTools(this)); //New one!
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActive("AdvancedLoadouts"))
		{
			m_Modules.Insert(new AdvancedLoadouts(this));
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActive("SafeZone"))
		{
			m_Modules.Insert(new SafeZoneHandler(this));
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActive("InfectedHordes"))
		{
			m_Modules.Insert(new InfectedHordesSpawner(this));
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActive("CustomBuildings"))
		{
			m_Modules.Insert(new BuildingSpawner(this, CustomBuildings.Cast(ModTunables.Cast(GetModule(ModTunables)).getBuildingList())));
		}
	}
	
	void InitModules()
	{
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).Init();
		}
	}

	ref set<ref VPPModuleManager> GetModuleSet()
	{
		return m_Modules;
	}
	
	VPPModuleManager GetModule(typename moduleType)
	{
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			VPPModuleManager module = m_Modules.Get(i);
			if (module.GetModuleType() == moduleType) 
			{
				return module;
			}
		}
		return NULL;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		for ( int i = 0; i < m_Modules.Count(); ++i)
		{
			m_Modules.Get(i).onUpdate(timeslice);
		}
	}

	override void OnInit()
	{
		super.OnInit();
		InitModules();
		
		
		/*ref array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition3D("7500 0 7500", 500, objects, NULL);
		
		foreach(Object obj : objects)
		{
			ref array<LOD> Objlods;
			Print("Object:: "+obj);
			if (obj.GetLODS(Objlods))
			{
				for (int xx = 0; xx < Objlods.Count(); ++xx)
				{
					Print("LODS:: "+Objlods.Get(xx));
				}
			}
		}*/

		
		if (!ModTunables.Cast(GetModule(ModTunables)).IsActiveMisc("Debugmode"))
		{
			Hive ce = CreateHive();
			if (ce)
			ce.InitOffline();
		    //ce.InitOnline( "127.0.0.1:9433/" );
			//ce.SetShardID( "00001" );
			//ce.SetEnviroment( "stable" );
			//ce.InitSandbox();
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActiveMisc("SessionFeed"))
		{
			g_Game.SetProfileString("SessionFeed", "true");
		}
		else
		{
			g_Game.SetProfileString("SessionFeed", "false");
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActiveMisc("StaminaStatus"))
		{
			m_StaminaStatus = true; //Disable Stamina
		}

		if (ModTunables.Cast(GetModule(ModTunables)).IsActiveMisc("ProxyExportMode"))
		{
			CETesting TestHive = GetTesting();
			TestHive.ExportProxyProto();
			TestHive.ExportProxyData( "7500 0 7500", 15000 );
		}
		
		//-----------
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(this.PlayerCounter, 110000, true);  //Default 120000 2 mins Looped
		//-----------
	}

	/*override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		super.PlayerDisconnected(player,identity,uid);
	}*/

	void SendMessage(bool Global, PlayerIdentity DirectIdentity , string Title, string Message, int msgLength, int FadeInTime, bool ForceShow, bool DoFadeIn, string ImagePath, float SizeX, float SizeY)
	{
		Param2<ref Param6<string,string,int,int,bool,bool>,ref Param3<string,float,float>> data;
		ref Param6<string,string,int,int,bool,bool> m_TextProperties;
		ref Param3<string,float,float> m_ImageProperties
		if (Global)
		{
			//Text Properties: Title, Message, Duration, FadeIn Time, Force show, DoFadeIn || Image Properties: path, Size X, Size Y
			m_TextProperties = new Param6<string,string,int,int,bool,bool>(Title,Message,msgLength,FadeInTime,ForceShow,DoFadeIn);
	        m_ImageProperties = new Param3<string,float,float>(ImagePath,SizeX,SizeY);
	        data = new Param2<ref Param6<string,string,int,int,bool,bool>,ref Param3<string,float,float>>(m_TextProperties,m_ImageProperties);
			GetRPCManager().SendRPC( "RPC_VPPNotificationUI", "AddToQueue", data, true );
		}
		else
		{
			m_TextProperties = new Param6<string,string,int,int,bool,bool>(Title,Message,msgLength,FadeInTime,ForceShow,DoFadeIn);
	        m_ImageProperties = new Param3<string,float,float>(ImagePath,SizeX,SizeY);
	        data = new Param2<ref Param6<string,string,int,int,bool,bool>,ref Param3<string,float,float>>(m_TextProperties,m_ImageProperties);
			GetRPCManager().SendRPC( "RPC_VPPNotificationUI", "AddToQueue", data, true, DirectIdentity);
		}
	}

	void GetLobbyTime( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< bool > data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	int  m_MaxIdelTime = GetGame().ServerConfigGetInt("LobbyMaxTime");
        	Param3<int,int,bool> param;
        	if (m_MaxIdelTime <= 0)
			{
				param = new Param3<int,int,bool>(220,m_LogInTimerLength,data.param1);
        		GetRPCManager().SendRPC( "RPC_VanillaPlusPlus", "SetLobbyMaxTime", param, true, sender );
			}
			else
			{
				param = new Param3<int,int,bool>(m_MaxIdelTime,m_LogInTimerLength,data.param1);
        		GetRPCManager().SendRPC( "RPC_VanillaPlusPlus", "SetLobbyMaxTime", param, true, sender );
			}
        }
	}

	void GetSpawnVectors( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	ref array<ref VPPSpawnPoint> m_MapSpawnPoints = VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).getSpawnPointsArray();
        	ref map<string,vector> m_Posintions = new map<string,vector>;
        	foreach(VPPSpawnPoint point : m_MapSpawnPoints)
        	{
        		m_Posintions.Insert(point.getSpawnName(),point.getSpawnLocation());
        	}

    	    Param1<ref map<string,vector>> map_param = new Param1<ref map<string,vector>>(m_Posintions);
        	GetRPCManager().SendRPC( "RPC_SortPositions", "SortPositions", map_param, true, sender );
        }
	}

	void CoolDownHandler( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param3<string,string,bool> data;
        if ( !ctx.Read( data ) ) return;
       
        if (type == CallType.Server)
        {
        	if (data.param2 != "randomPos")
        	{
	        	if (data.param3)
	        	{
	        		SpawnCoolDownSystem.Cast(GetModule(SpawnCoolDownSystem)).AddCooldown(data.param1,data.param2);
	        	}
	        	else
	        	{
	        		SpawnCoolDownSystem.Cast(GetModule(SpawnCoolDownSystem)).RemoveCooldown(data.param1,data.param2);
	        	}
        	}
        }
	}

	override void OnPreloadEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int queueTime)
	{
		if (GetHive())
		{
			useDB = true;
			queueTime = m_LogInTimerLength;
		}
		else
		{
			useDB = false;
			pos = "7500 0 7500";
			yaw = 0;
			queueTime = 1;
		}
	}

	override void TickScheduler(float timeslice)
	{
		GetGame().GetWorld().GetPlayerList(m_Players);
		if( m_Players.Count() == 0 ) return;
		for(int i = 0; i < SCHEDULER_PLAYERS_PER_TICK; i++)
		{
			if(m_currentPlayer >= m_Players.Count() )
			{
				m_currentPlayer = 0;
			}

			PlayerBase currentPlayer = PlayerBase.Cast(m_Players.Get(m_currentPlayer));
			currentPlayer.OnTick();

			if (m_StaminaStatus) {
			currentPlayer.GetStaminaHandler().SyncStamina(1000,1000);
            currentPlayer.GetStatStamina().Set(currentPlayer.GetStaminaHandler().GetStaminaCap());
			}
			if (GetModule(SafeZoneHandler)) { SafeZoneHandler.Cast(GetModule(SafeZoneHandler)).SafeZoneHandle(currentPlayer); }
			m_currentPlayer++;
		}
	}

	PlayerBase CreateCharacter(PlayerIdentity identity, string posName, int MapKey,ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		if (posName == "randomPos")
		{
			//Get Random SpawnPoint
			playerEnt = GetGame().CreatePlayer(identity, characterName, SpawnPointsManager.Cast(GetModule(SpawnPointsManager)).GetRandomSpawnPoint(), 0, "NONE");
			Class.CastTo(m_player, playerEnt);
			GetGame().SelectPlayer(identity, m_player);
		}
		else
		{
			playerEnt = GetGame().CreatePlayer(identity, characterName, SpawnPointsManager.Cast(GetModule(SpawnPointsManager)).GetPosByName(posName,MapKey), 0, "NONE");
			Class.CastTo(m_player, playerEnt);
			GetGame().SelectPlayer(identity, m_player);
		}
		return m_player;
	}

	override void OnEvent(EventType eventTypeId, Param params) 
	{
		super.OnEvent(eventTypeId,params);

		PlayerIdentity identity;
		switch(eventTypeId)
		{
			case ChatMessageEventTypeID:
			 ChatMessageEventParams chat_params = ChatMessageEventParams.Cast(params);
			 if (chat_params.param1 == 0 && chat_params.param2 != "") //trigger only when channel is Global == 0 and Player Name does not equal to null
				{
					Param4<int,string,string,string> request_info = new Param4<int,string,string,string>(chat_params.param1, chat_params.param2, chat_params.param3, chat_params.param4);
					AdminTool.Cast(GetModule(AdminTool)).RequestHandler(request_info); //Send the param to Admintools
				}
			break;
		}
	}

	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		int m_MapKey;
		ProcessData(ctx, m_SkinStr, m_SpawnPoint, m_MapKey);
		if (CreateCharacter(identity, m_SpawnPoint, m_MapKey,ctx, m_SkinStr))
		{
			GetGame().RPCSingleParam(m_player, ERPCs.RPC_CHARACTER_EQUIPPED, NULL, true, m_player.GetIdentity());
		}
		return m_player;
	}

	void SpawnStartKit( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param2<string,string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Server)
        {
        	//Dont touch you nerd. unless you know what you are doing :)
			ItemBase itemBs;
			EntityAI itemEnt;

        	string LoadOutName;
			string UID = data.param2;
        	if (VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).loadOutsType() == "STATIC_LOADOUTS")
        	{
        		LoadOutName = data.param1;
	        	AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).SpawnStaticLoadout(LoadOutName,UID);
        	}
        	else
        	{
        		string JsonData = data.param1;
	        	m_Gear = new map<int, string>;
	        	PlayerBase player;
				if (JsonData != "random")
				{
					JsonFileLoader<ref map<int,string>>.JsonLoadData( JsonData,m_Gear );
					player = PlayerBase.Cast(GetPlayerByUID(UID));
				    for (int i = 0; i < m_Gear.Count(); ++i)
				    {
				    	int oKey = m_Gear.GetKey(i);
				    	string Item = m_Gear.Get(oKey);
				    	itemBs = ItemBase.Cast( player.GetInventory().CreateInInventory( Item ) );
				    }
				    if (AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).CheckTunables("SpawnArmed")) { RandomWeaponOnStartup(player); }
				}
				else
				{
					player = PlayerBase.Cast(GetPlayerByUID(UID));
					if (GetModule(AdvancedLoadouts))
					{
						if (VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).loadOutsType() == "STATIC_LOADOUTS")
						{
							bool reqld = AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).LoadRandomStaticLD(player);
						}
						else if (VPPSpawnPoints.Cast(GetConfigByType(VPPSpawnPoints)).loadOutsType() == "CUSTOMIZABLE_LOADOUTS")
						{
							AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).LoadRndGenLoadout(player);
						}
						else
						{
							//Vanilla
							itemEnt = player.GetInventory().CreateInInventory( "Rag" );
							itemBs = ItemBase.Cast(itemEnt);							
							itemBs.SetQuantity(6);
						}
					}
					else
					{
						//Vanilla
						itemEnt = player.GetInventory().CreateInInventory( "Rag" );
						itemBs = ItemBase.Cast(itemEnt);							
						itemBs.SetQuantity(6);
					}
				}
        	}
        	SpawnEssentialItems( PlayerBase.Cast( GetPlayerByUID(UID) ) );
        }
	}

	PlayerBase GetPlayerByUID(string UID)
	{
		array<Man> players = new array<Man>;
		GetGame().GetWorld().GetPlayerList( players );
	    PlayerBase selectedPlayer;
   	    for (int i = 0; i < players.Count(); ++i)
   	    {
   	    	string PUID = players.Get(i).GetIdentity().GetId();
   	    	string GUID = players.Get(i).GetIdentity().GetPlainId();
   	    	if (PUID == UID)
   	    	{
   	    		selectedPlayer = players.Get(i);
   	    	}
   	    	else if ( GUID == UID)
   	    	{
   	    		selectedPlayer = players.Get(i);
   	    	}
   	    }
   	    return selectedPlayer;
	}

	void ProcessData(ParamsReadContext ctx, out string Skin, out string SpawnPoint, out int MapKey)
	{
		ref Param1<string> skinParam = new Param1<string>(Skin);
		if (ctx.Read(skinParam))
		{
			Skin = skinParam.param1;
		}
		ref Param1<string> SpawnPointParam = new Param1<string>(SpawnPoint);
		if (ctx.Read(SpawnPointParam))
		{
			SpawnPoint = SpawnPointParam.param1;
		}
		ref Param1<int> SPPKeyParam = new Param1<int>(MapKey);
		if (ctx.Read(SPPKeyParam))
		{
			MapKey = SPPKeyParam.param1;
		}
	}

	void GlobalMessage(int Channel, string Message)
	{
		if (Message != "")
		{
			GetGame().ChatPlayer(Message);
		}
	}

	void PlayerCounter()
	{
		array<Man> players = new array<Man>;
	    GetGame().GetPlayers( players );
	    int numbOfplayers = players.Count();
	    GlobalMessage(1,"Online Players: "+ numbOfplayers.ToString());
	}

	void RandomWeaponOnStartup(PlayerBase player)
	{
	    //Gun spawner Handle
		//SpawnGunIn( PlayerBase player, string ClassName, bool isPrimary, TstringArray Attachments, TstringArray Extras) NOTE: Set bool to 'true' IF weapon was primary
		int oRandValue = Math.RandomIntInclusive(0,2);
		switch(oRandValue.ToString())
		{
			case "0":
			AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).SpawnGunIn( player , "fnx45", true, {"fnp45_mrdsoptic","PistolSuppressor"},{"mag_fnx45_15rnd","mag_fnx45_15rnd"} );
			break;

			case "1":
			AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).SpawnGunIn( player , "CZ75", true, {"PistolSuppressor"} , {"Mag_CZ75_15Rnd","Mag_CZ75_15Rnd"} );
			break;

			case "2":
			AdvancedLoadouts.Cast(GetModule(AdvancedLoadouts)).SpawnGunIn( player , "makarovij70", true, {"PistolSuppressor"} , {"mag_ij70_8rnd","mag_ij70_8rnd"} );
			break;
		}
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{

	}

	void SpawnEssentialItems(PlayerBase player)
	{
		//Add any items you wish to spawn always on every player here.
		//eg 
		ItemBase itemBs;
		EntityAI itemEnt;

		player.GetHumanInventory().CreateInInventory( "Battery9V" );
		
		itemEnt = EntityAI.Cast(player.GetHumanInventory().CreateInInventory( "Rag" ));
		itemBs = ItemBase.Cast(itemEnt);
		if (itemBs != NULL)
		{
			itemBs.SetQuantity(6);
			player.SetQuickBarEntityShortcut(itemBs, 3, true);
		}
	}
}