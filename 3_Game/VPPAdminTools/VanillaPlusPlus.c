modded class DayZGame
{
	private ref VPPEventHandler vppatEventHandler;
	private bool   m_SpectateStatus;
	private bool   m_IsLShiftHolding;

	private bool   m_DisablePasswordProtection;
	private int    m_FailedLoginCount = 0;
	private string m_AdminPassword;

	private string 			  m_VServerIP;

	private string 			  m_ServerName;
	private int 			  m_Vtps = 0;
    private int 			  m_Vtps_time = 0;
    private int 			  m_Vticks = 0;

    private bool 					m_VPPATOnlineServicesInit;
    private ref VPPATProfileOptions m_VPPATProfileOptions;

    private ref map<string,string> 	 m_VPPDebugMissions; //terrain name, path

	void DayZGame()
	{
		new PlayerListManager();
		m_VPPDebugMissions = new map<string,string>;
		Print("[DayZ Game]:: DayZGame(): Initializing V++ Admin Tools.");
		vppatEventHandler = new VPPEventHandler();
		GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RegisterRPCs, 500, false); //Register RPCs after 10 seconds of game boot
		m_VPPATProfileOptions = new VPPATProfileOptions();

		#ifndef SERVER
			SetupDebugMission("ChernarusPlus");
			SetupDebugMission("Enoch");
		#endif
	}

	void CreateDirectoriesFromPath(string basePath, string path)
	{
		array<string> output = {};
		path.Split("\\", output);
		for (int i = 0; i < output.Count() - 1; ++i)
			MakeDirectory(string.Format("%1/%2", basePath, output[i]));
	}

	void SetupDebugMission(string terrain, bool forceOverride = false)
	{
		terrain.ToLower();
		string path = string.Format("$saves:VPPDebugMissions/Debug.%1", terrain);

		MakeDirectory("$saves:VPPDebugMissions");
		string missionPath = string.Format("$saves:VPPDebugMissions/Debug.%1", terrain);
		if (FileExist(missionPath) && !forceOverride)
		{
			Print("Default Mission: " + missionPath + " exists!");
			m_VPPDebugMissions.Insert(terrain, path); //exists already
			return;
		}

		if (forceOverride)
			FileProcessingFuncs.DeleteRecursive(missionPath, true); //Delete existing

		MakeDirectory(missionPath);
		CopyFile("VPPAdminTools/Extra/init.c", path + "/init.c");

		string fileName;
		FileAttr fileAttributes;
		FindFileFlags flags = FindFileFlags.ARCHIVES;
		
		FindFileHandle fHandle = FindFile(string.Format("dz/worlds/%1/ce/*", terrain), fileName, fileAttributes, flags);
		CreateDirectoriesFromPath(path, fileName);
		CopyFile(string.Format("dz/worlds/%1/ce/%2", terrain, fileName), path + "/" + fileName);

		int i = 0;
		while(FindNextFile(fHandle, fileName, fileAttributes))
		{
			CreateDirectoriesFromPath(path, fileName);
			CopyFile(string.Format("dz/worlds/%1/ce/%2", terrain, fileName), path + "/" + fileName);
			i++;
		}

		m_VPPDebugMissions.Insert(terrain, path);
	}

	bool SearchForCustomDebugMission(out array<string> paths)
	{
		paths = {};
		string fileName;
		FileAttr fileAtts;
		FindFileFlags flags = FindFileFlags.ARCHIVES;

		FindFileHandle fHandle = FindFile("$CurrentDir:/*", fileName, fileAtts, flags);
		if (FileExist(string.Format("$CurrentDir:%1/init.c", fileName)) && fileAtts == FileAttr.DIRECTORY)
			paths.Insert(fileName);

		int i = 0;
		while(FindNextFile(fHandle, fileName, fileAtts))
		{
			if (FileExist(string.Format("$CurrentDir:%1/init.c", fileName)) && fileAtts == FileAttr.DIRECTORY)
				paths.Insert(fileName);

			i++;
		}

		//Append paths to vanilla missions:
		foreach(string terrain, string path: m_VPPDebugMissions)
		{
			paths.Insert(path);
		}
		return true;
	}

	void PlayCustomMission(string path)
	{
		Print("PlayCustomMission: " + path);
		SetGameState(DayZGameState.IN_GAME);
		SetLoadState(DayZLoadState.MISSION_START);
		path.Replace("/", "\\");
		PlayMission(path);
	}

	override bool OnInitialize()
	{
		string value;
		if (CommandlineGetParam("CEDebugMode", value))
		{
			value.ToLower();
			if (GetLoadState() == DayZLoadState.UNDEFINED)
			{
				ParticleList.PreloadParticles();
				InitNotifications();

				switch(value)
				{
					case "chernarusplus":
						PlayCustomMission(m_VPPDebugMissions["chernarusplus"]);
					break;

					case "enoch":
					case "livonia":
						PlayCustomMission(m_VPPDebugMissions["enoch"]);
					break;

					default:
						//Search for a custom mission
						Print("[VPPAdminTools] FAILED TO LOAD DEBUG MISSION: " + value + " unknown terrain map");
						DebugModeDialog diag;
						GetGame().GetWorkspace().CreateWidgets("VPPAdminTools\\GUI\\Layouts\\VPPNotificationDebug.layout").GetScript(diag);
						string errorMsg = string.Format("Unable to find a mission folder for world: %1\n", value);
						errorMsg += "\n-Make sure the world name is correct according to the config of the world.";
						errorMsg += "\n-Copy a valid mission folder to the root directory of DayZ Client and click 'SEARCH AGAIN'.";
						errorMsg += "\n\n-Missions under '$saves' are created by default directly from game data (CLE PBOs)";
						errorMsg += "\nYou can find those missions in:";
						errorMsg += "\n\n[when '-profiles' param is NOT used]:\nC:\\Users\\(PC_NAME)\\Documents\\DayZ\\VPPDebugMissions\\";
						errorMsg += "\n\n[when '-profiles' param is used]: $profile:VPPDebugMissions\\";
						errorMsg += "\n\n'$profile' is the path you set with the startup parameter '-profiles'";
						errorMsg += "\n\nDouble-click a mission from the list below to start!";
						diag.SetError("ERROR: No Debug mission files found!", errorMsg);
						diag.RefreshSearch();
					break;
				}
			}
		}
		else
		{
			return super.OnInitialize();
		}
		return false;
	}

	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		if ( key == KeyCode.KC_LSHIFT ) 
		{
			m_IsLShiftHolding = true;
		}

		if ((IsLeftAltHolding() || IsLeftAltHolding()) && key == KeyCode.KC_F4)
		{
			LogoutRequestTime();
			GetMission().Pause();
			GetCallQueue(CALL_CATEGORY_GUI).Call(GetMission().CreateLogoutMenu, GetGame().GetUIManager().GetMenu());
			GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetMission().AbortMission, 1000);
			GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestExit, 500, false, 0);
		}
	}
	
	override void OnKeyRelease(int key)
	{
		super.OnKeyRelease(key);
		if ( key == KeyCode.KC_LSHIFT ) 
		{
			m_IsLShiftHolding = false;
		}
	}

	override void DeferredInit()
	{
		super.DeferredInit();
		//Register custom option 
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_SPEED, "vppat_cam_speed", VPPATProfileConstants.DEFAULT_CAM_SPEED);
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_BOOST, "vppat_cam_boost", VPPATProfileConstants.DEFAULT_CAM_BOOST);
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_MOVE_DRAG, "vppat_cam_move_drag", VPPATProfileConstants.DEFAULT_CAM_MOVE_DRAG);
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_MOUSE_SENSE, "vppat_cam_mouse_sense", VPPATProfileConstants.DEFAULT_CAM_MOUSE_SENSE);
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_SMOOTHNESS, "vppat_cam_smoothness", VPPATProfileConstants.DEFAULT_CAM_SMOOTHNESS);
		m_VPPATProfileOptions.RegisterProfileOption(EVPPATProfileOptions.CAM_FOV, "vppat_cam_fov", VPPATProfileConstants.DEFAULT_CAM_FOV);
	}

	//Credit to philip from CFTools ;)
	override void OnUpdate(bool doSim, float timeslice)
	{
        super.OnUpdate(doSim, timeslice);

        m_Vticks++;
        if(m_Vtps_time + 1 < GetGame().GetTickTime()) {
            m_Vtps_time = GetGame().GetTickTime();
            m_Vtps = m_Vticks / 2;
            m_Vticks = 0;
        }
    }

    int GetServerFPS() return m_Vtps;

    string GetServerName()
	{
		return m_ServerName;
	}

	void SetServerName(string str)
	{
		m_ServerName = str;
	}

	void VSetServerIP(string ip)
	{
		m_VServerIP = ip;
	}

	string VGetServerIP()
	{
		return m_VServerIP;
	}

	VPPEventHandler VPPATGetEventHandler()
	{
		return vppatEventHandler;
	}

	//Register early RPCs after game is running.
	void RegisterRPCs()
	{
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "KickClientHandle", this, SingleplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "GetConnectedSession", this, SingleplayerExecutionType.Client );
	}
	
	void KickClientHandle( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<string> data;
        if ( !ctx.Read( data ) || sender != null ) return;

        if (type == CallType.Client)
        {
			bool done = false;
			while( !done )
			{
				if (GetDayZGame().GetGameState() != DayZGameState.CONNECT || GetDayZGame().GetGameState() != DayZGameState.CONNECTING || g_Game.GetLoadState() != DayZLoadState.CONNECT_START)
				{
					GetGame().GetUIManager().ShowDialog("#VSTR_SESSION_DISCONNECT", "V++ AdminTools: "+data.param1, 1, DBT_OK, DBB_OK, DMT_WARNING, g_Game.GetUIManager().GetMenu());
	        		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().DisconnectSessionForce, 100, false);
					done = true;
					break;
				}
			}
        }
	}

	void GetConnectedSession( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if (type == CallType.Client)
        {
        	string address;
        	int port;
        	GetGame().GetHostAddress( address, port );
        	GetRPCManager().VSendRPC( "RPC_MissionGameplay", "ConnectedSession", new Param1<string>(address), true);
        }
	}
	
	void SetSpectateMode(bool state)
	{
		m_SpectateStatus = state;
	}
	
	float GetVPPATProfileVal(EVPPATProfileOptions option)
	{
		return m_VPPATProfileOptions.GetProfileOptionVal(option);
	}
	
	void SetVPPATProfileVal(EVPPATProfileOptions option, float value)
	{
		m_VPPATProfileOptions.SetProfileOptionVal(option, value);
	}

	bool IsSpectateMode()
	{
		return m_SpectateStatus;
	}
	
	bool IsRightAltHolding()
	{
		return m_IsRightAltHolding;
	}
	
	bool IsLeftAltHolding()
	{
		return m_IsLeftAltHolding;
	}
	
	bool IsLShiftHolding()
	{
		return m_IsLShiftHolding;
	}

	int GetFailedLoginAttempts()
	{
		return m_FailedLoginCount;
	}

	void IncrementFailedLoginAttempts()
	{
		m_FailedLoginCount++;
	}

	void SetAdminPasswordHash(string data)
	{
		m_AdminPassword = data;
	}

	void DisablePasswordProtection(bool state)
	{
		m_DisablePasswordProtection = state;
	}

	bool IsPasswordProtectionDisabled()
	{
		return m_DisablePasswordProtection;
	}

	string GetAdminPasswordHash()
	{
		return m_AdminPassword;
	}

	string GetSessionPassword()
	{
		string password;
		if (GetCLIParam("password", password))
		{
			return password;
		}
		return m_ConnectPassword;
	}
	
	string VPPAT_GetSteam64Id()
	{
		if ( !m_VPPATOnlineServicesInit )
		{
			OnlineServices.Init();
			m_VPPATOnlineServicesInit = true;
		}
        
		BiosUserManager user_manager = GetGame().GetUserManager();
		if( user_manager )
		{
		    if( user_manager.GetTitleInitiator() )
		    {
		        user_manager.SelectUserEx( user_manager.GetTitleInitiator() );
		    }
		}

		if ( user_manager && user_manager.GetSelectedUser() )
		    return user_manager.GetSelectedUser().GetUid();

		return "-1";
	}

	/*
		Reconnects player to same session
	*/
	void ReconnectToCurrentSession()
	{
		string portStr, ip, password;
		int port;
		password = GetSessionPassword();
		if( !GetCLIParam("connect", ip) )
		{
			ip = m_ConnectAddress;
		}
		
		if ( !GetCLIParam("port", portStr) )
		{
			port = m_ConnectPort;
		}else{
			port = portStr.ToInt();
		}
		GetGame().Connect( null, ip, port, password );
	}

	//-=-=-=-=-=-=-=-=-=-=-=-
	string GetDefaultMagazine(EntityAI weapon)
	{
		TStringArray mag_names = new TStringArray;
		string cfg_path = "cfgWeapons " + weapon.GetType() + " magazines";
		GetGame().ConfigGetTextArray(cfg_path, mag_names);	
		
		if (mag_names.Count() > 0)
		{
			return mag_names[0];
		}
		
		mag_names = new TStringArray;
		cfg_path = "cfgWeapons " + weapon.GetType() + " chamberableFrom";
		GetGame().ConfigGetTextArray(cfg_path, mag_names);
		
		if (mag_names.Count() > 0)
		{
			return mag_names[0];
		}
			
		return "";
	}
	
	string GetDate(bool fullStamp)
	{
		int year, month, day, hour, minute, second;
		GetYearMonthDay(year, month, day);
		GetHourMinuteSecond(hour, minute, second);
		string date;
		if (fullStamp)
		{
			date = "[" + "Hr: " + hour.ToStringLen(2) + " - "+ "Min: " + minute.ToStringLen(2) +"] " + " || " + "[Mth: " +month.ToStringLen(2) + " -- " + "Day: " + day.ToStringLen(2) + "]";
			return date;
		}
		date = "[" + "Hr: " + hour.ToStringLen(2) + " - "+ "Min: " + minute.ToStringLen(2) +"] ";
		return date;
	}
	
	vector SwapYZ(vector vec)
	{
		vector tmp;
		tmp[0] = vec[0];
		tmp[1] = vec[2];
		tmp[2] = vec[1];
	
		return tmp;
	}

	vector StringToVector(string str)
	{
		vector oV = "0 0 0";
		TStringArray strs = new TStringArray;
		str.Replace("<","");
		str.Replace(">","");
		str.Replace(","," ");

		str.Split( " ", strs );

		oV = Vector(strs.Get(0).ToFloat(),strs.Get(1).ToFloat(),strs.Get(2).ToFloat());
		return oV;
	}

	string VectorToString(vector vctr)
	{
		string oStr;
		oStr = vctr[0].ToString();
		oStr += "," + vctr[1].ToString();
		oStr += "," + vctr[2].ToString();
		return oStr;
	}

	vector SnapToGround(vector pos)
	{
		float pos_x = pos[0];
		float pos_z = pos[2];
		float pos_y = GetGame().SurfaceY(pos_x, pos_z);
		vector tmp_pos = Vector(pos_x, pos_y, pos_z);
		tmp_pos[1] = tmp_pos[1] + pos[1];
	
		return tmp_pos;
	}

	vector GetPosByCursor( float distance = 1000.0, Object ignore = NULL )
	{
	    if ( !GetPlayer() )
	        return "0 0 0";

	    vector dir = GetGame().GetPointerDirection();
	    vector from = GetGame().GetCurrentCameraPosition();
	    vector to = from + ( dir * distance );
	    vector rayStart = from;
	    vector rayEnd = to;
	    vector hitPos;
	    vector hitNormal;
	    int hitComponentIndex;
	    DayZPhysics.RaycastRV( rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, ignore );
	    return hitPos;
	}

	vector GetCursorPos()
	{
	    if ( !GetPlayer() )
	        return "0 0 0";

	    vector rayStart = GetGame().GetCurrentCameraPosition();
	    vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
	    vector hitPos;
	    vector hitNormal;
	    int hitComponentIndex;
	    DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, GetPlayer());
	    return hitPos;
	}

	set< Object > GetObjectsAt( vector from, vector to, Object ignore = NULL, float radius = 0.0, Object with = NULL )
	{
	    vector contact_pos;
	    vector contact_dir;
	    int contact_component;

	    set< Object > geom = new set< Object >;
	    set< Object > view = new set< Object >;

	    DayZPhysics.RaycastRV( from, to, contact_pos, contact_dir, contact_component, geom, with, ignore, false, false, ObjIntersectGeom, radius );
	    DayZPhysics.RaycastRV( from, to, contact_pos, contact_dir, contact_component, view, with, ignore, false, false, ObjIntersectView, radius );

	    for ( int i = 0; i < geom.Count(); i++ )
	        view.Insert( geom[i] );

	    if ( view.Count() > 0 ) 
	        return view;
		
	    return NULL;
	}

	Object getObjectAtCrosshair(float distance = 1000.0, float radius = 0.0, Object with = NULL)
	{
	    vector rayStart = GetGame().GetCurrentCameraPosition();
	    vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * distance;
	    auto objs = GetObjectsAt( rayStart, rayEnd, GetGame().GetPlayer(), radius, with );

	    if( objs != null && objs.Count() > 0 )
	        return objs[ 0 ];

	    return NULL;
	}
};

VPPEventHandler VPPATGetEventHandler()
{
	return g_Game.VPPATGetEventHandler();
};