modded class DayZGame
{
	private ref VPPEventHandler vppatEventHandler;
	private bool   m_SpectateStatus;
	private bool   m_IsLShiftHolding;

	private int    m_FailedLoginCount = 0;
	private string m_AdminPassword;

	private string 			  m_ServerName;
	private int 			  m_Vtps = 0;
    private int 			  m_Vtps_time = 0;
    private int 			  m_Vticks = 0;

	void DayZGame()
	{
		Print("[DayZ Game]:: DayZGame(): Initializing V++ Admin Tools.");
		vppatEventHandler = new VPPEventHandler();
		GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RegisterRPCs, 500, false); //Register RPCs after 10 seconds of game boot
	}
	
	override void OnKeyPress(int key)
	{
		super.OnKeyPress(key);
		if ( key == KeyCode.KC_LSHIFT ) 
		{
			m_IsLShiftHolding = true;
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

	VPPEventHandler VPPATGetEventHandler()
	{
		return vppatEventHandler;
	}

	//Register early RPCs after game is running.
	void RegisterRPCs()
	{
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "KickClientHandle", this );
		GetRPCManager().AddRPC( "RPC_MissionGameplay", "GetConnectedSession", this );
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