class LocalPlayer
{
	private Object m_PlayerVehicle;
	private int m_UniqueID;
	
	void LocalPlayer(ref Object playerVeh, int PID){
		m_PlayerVehicle = playerVeh;
		m_UniqueID      = PID;
	}

	void ~LocalPlayer(){
		
	}
	
	int GetUniqueID(){
		return m_UniqueID;
	}

	ref Object GetPlayerVehicle(){
		return m_PlayerVehicle;
	}

	void SetPlayerVehicle(ref Object vehObj){
		m_PlayerVehicle = vehObj;
	}
}

class VPPSpawnPoint
{
	private string name;
	private vector spawnLocation;
	private float cooldownTime;
	void VPPSpawnPoint(string name, vector spawnLocation, float cooldownTime = 1800){
		this.spawnLocation = spawnLocation;
		this.name = name;
		this.cooldownTime = cooldownTime;
	}

	void ~VPPSpawnPoint(){

	}

	string getSpawnName(){
		return name;
	}

	vector getSpawnLocation(){
		return spawnLocation;
	}

	float getCooldownTime(){
		return cooldownTime;
	}
	
	void setCooldown(float newCooldown)
	{
		cooldownTime = newCooldown;
	}
	
	void setName(string NewName)
	{
		name = NewName;
	}
	
	void setLocation(vector newLocation)
	{
		spawnLocation = newLocation;
	}
}

modded class DayZGame
{
	private bool   m_ESPToggle;
	private bool   m_ESPType;
	private ref map<int,string>   m_KeyBindsMap;
	private ref VPPKeybindsConfig     m_VPPKeybindsConfig;

	void DayZGame()
	{
		GetRPCManager().AddRPC( "RPC_VanillaPlusPlus", "KickClientHandle", this, SingeplayerExecutionType.Server );
		Print("DayZ Vanilla PlusPlus!");

		m_VPPKeybindsConfig = new VPPKeybindsConfig();

		m_KeyBindsMap = new map<int,string>;
		m_KeyBindsMap.Insert(KeyCode.KC_ESCAPE,"KC_ESCAPE");
		m_KeyBindsMap.Insert(KeyCode.KC_1,"KC_1");
		m_KeyBindsMap.Insert(KeyCode.KC_2,"KC_2");
		m_KeyBindsMap.Insert(KeyCode.KC_3,"KC_3");
		m_KeyBindsMap.Insert(KeyCode.KC_4,"KC_4");
		m_KeyBindsMap.Insert(KeyCode.KC_5,"KC_5");
		m_KeyBindsMap.Insert(KeyCode.KC_6,"KC_6");
		m_KeyBindsMap.Insert(KeyCode.KC_7,"KC_7");
		m_KeyBindsMap.Insert(KeyCode.KC_8,"KC_8");
		m_KeyBindsMap.Insert(KeyCode.KC_9,"KC_9");
		m_KeyBindsMap.Insert(KeyCode.KC_0,"KC_0");
		m_KeyBindsMap.Insert(KeyCode.KC_MINUS,"KC_MINUS");
		m_KeyBindsMap.Insert(KeyCode.KC_EQUALS,"KC_EQUALS");
		m_KeyBindsMap.Insert(KeyCode.KC_BACK,"KC_BACK");
		m_KeyBindsMap.Insert(KeyCode.KC_TAB,"KC_TAB");
		m_KeyBindsMap.Insert(KeyCode.KC_Q,"KC_Q");
		m_KeyBindsMap.Insert(KeyCode.KC_W,"KC_W");
		m_KeyBindsMap.Insert(KeyCode.KC_E,"KC_E");
		m_KeyBindsMap.Insert(KeyCode.KC_R,"KC_R");
		m_KeyBindsMap.Insert(KeyCode.KC_T,"KC_T");
		m_KeyBindsMap.Insert(KeyCode.KC_Y,"KC_Y");
		m_KeyBindsMap.Insert(KeyCode.KC_U,"KC_U");
		m_KeyBindsMap.Insert(KeyCode.KC_I,"KC_I");
		m_KeyBindsMap.Insert(KeyCode.KC_O,"KC_O");
		m_KeyBindsMap.Insert(KeyCode.KC_P,"KC_P");
		m_KeyBindsMap.Insert(KeyCode.KC_LBRACKET,"KC_LBRACKET");
		m_KeyBindsMap.Insert(KeyCode.KC_RBRACKET,"KC_RBRACKET");
		m_KeyBindsMap.Insert(KeyCode.KC_RETURN,"KC_RETURN");
		m_KeyBindsMap.Insert(KeyCode.KC_LCONTROL,"KC_LCONTROL");
		m_KeyBindsMap.Insert(KeyCode.KC_A,"KC_A");
		m_KeyBindsMap.Insert(KeyCode.KC_S,"KC_S");
		m_KeyBindsMap.Insert(KeyCode.KC_D,"KC_D");
		m_KeyBindsMap.Insert(KeyCode.KC_F,"KC_F");
		m_KeyBindsMap.Insert(KeyCode.KC_G,"KC_G");
		m_KeyBindsMap.Insert(KeyCode.KC_H,"KC_H");
		m_KeyBindsMap.Insert(KeyCode.KC_J,"KC_J");
		m_KeyBindsMap.Insert(KeyCode.KC_K,"KC_K");
		m_KeyBindsMap.Insert(KeyCode.KC_L,"KC_L");
		m_KeyBindsMap.Insert(KeyCode.KC_SEMICOLON,"KC_SEMICOLON");
		m_KeyBindsMap.Insert(KeyCode.KC_APOSTROPHE,"KC_APOSTROPHE");
		m_KeyBindsMap.Insert(KeyCode.KC_GRAVE,"KC_GRAVE");
		m_KeyBindsMap.Insert(KeyCode.KC_LSHIFT,"KC_LSHIFT");
		m_KeyBindsMap.Insert(KeyCode.KC_BACKSLASH,"KC_BACKSLASH");
		m_KeyBindsMap.Insert(KeyCode.KC_Z,"KC_Z");
		m_KeyBindsMap.Insert(KeyCode.KC_X,"KC_X");
		m_KeyBindsMap.Insert(KeyCode.KC_C,"KC_C");
		m_KeyBindsMap.Insert(KeyCode.KC_V,"KC_V");
		m_KeyBindsMap.Insert(KeyCode.KC_B,"KC_B");
		m_KeyBindsMap.Insert(KeyCode.KC_N,"KC_N");
		m_KeyBindsMap.Insert(KeyCode.KC_M,"KC_M");
		m_KeyBindsMap.Insert(KeyCode.KC_COMMA,"KC_COMMA");
		m_KeyBindsMap.Insert(KeyCode.KC_PERIOD,"KC_PERIOD");
		m_KeyBindsMap.Insert(KeyCode.KC_SLASH,"KC_SLASH");
		m_KeyBindsMap.Insert(KeyCode.KC_RSHIFT,"KC_RSHIFT");
		m_KeyBindsMap.Insert(KeyCode.KC_MULTIPLY,"KC_MULTIPLY");
		m_KeyBindsMap.Insert(KeyCode.KC_LMENU,"KC_LMENU");
		m_KeyBindsMap.Insert(KeyCode.KC_SPACE,"KC_SPACE");
		m_KeyBindsMap.Insert(KeyCode.KC_CAPITAL,"KC_CAPITAL");
		m_KeyBindsMap.Insert(KeyCode.KC_F1,"KC_F1");
		m_KeyBindsMap.Insert(KeyCode.KC_F2,"KC_F2");
		m_KeyBindsMap.Insert(KeyCode.KC_F3,"KC_F3");
		m_KeyBindsMap.Insert(KeyCode.KC_F4,"KC_F4");
		m_KeyBindsMap.Insert(KeyCode.KC_F5,"KC_F5");
		m_KeyBindsMap.Insert(KeyCode.KC_F6,"KC_F6");
		m_KeyBindsMap.Insert(KeyCode.KC_F7,"KC_F7");
		m_KeyBindsMap.Insert(KeyCode.KC_F8,"KC_F8");
		m_KeyBindsMap.Insert(KeyCode.KC_F9,"KC_F9");
		m_KeyBindsMap.Insert(KeyCode.KC_F10,"KC_F10");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMLOCK,"KC_NUMLOCK");
		m_KeyBindsMap.Insert(KeyCode.KC_SCROLL,"KC_SCROLL");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD7,"KC_NUMPAD7");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD8,"KC_NUMPAD8");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD9,"KC_NUMPAD9");
		m_KeyBindsMap.Insert(KeyCode.KC_SUBTRACT,"KC_SUBTRACT");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD4,"KC_NUMPAD4");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD5,"KC_NUMPAD5");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD6,"KC_NUMPAD6");
		m_KeyBindsMap.Insert(KeyCode.KC_ADD,"KC_ADD");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD1,"KC_NUMPAD1");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD2,"KC_NUMPAD2");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD3,"KC_NUMPAD3");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPAD0,"KC_NUMPAD0");
		m_KeyBindsMap.Insert(KeyCode.KC_DECIMAL,"KC_DECIMAL");
		m_KeyBindsMap.Insert(KeyCode.KC_OEM_102,"KC_OEM_102");
		m_KeyBindsMap.Insert(KeyCode.KC_F11,"KC_F11");
		m_KeyBindsMap.Insert(KeyCode.KC_F12,"KC_F12");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPADEQUALS,"KC_NUMPADEQUALS");
		m_KeyBindsMap.Insert(KeyCode.KC_PREVTRACK,"KC_PREVTRACK");
		m_KeyBindsMap.Insert(KeyCode.KC_AT,"KC_AT");
		m_KeyBindsMap.Insert(KeyCode.KC_COLON,"KC_COLON");
		m_KeyBindsMap.Insert(KeyCode.KC_UNDERLINE,"KC_UNDERLINE");
		m_KeyBindsMap.Insert(KeyCode.KC_STOP,"KC_STOP");
		m_KeyBindsMap.Insert(KeyCode.KC_AX,"KC_AX");
		m_KeyBindsMap.Insert(KeyCode.KC_UNLABELED,"KC_UNLABELED");
		m_KeyBindsMap.Insert(KeyCode.KC_NEXTTRACK,"KC_NEXTTRACK");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPADENTER,"KC_NUMPADENTER");
		m_KeyBindsMap.Insert(KeyCode.KC_RCONTROL,"KC_RCONTROL");
		m_KeyBindsMap.Insert(KeyCode.KC_MUTE,"KC_MUTE");
		m_KeyBindsMap.Insert(KeyCode.KC_CALCULATOR,"KC_CALCULATOR");
		m_KeyBindsMap.Insert(KeyCode.KC_PLAYPAUSE,"KC_PLAYPAUSE");
		m_KeyBindsMap.Insert(KeyCode.KC_MEDIASTOP,"KC_MEDIASTOP");
		m_KeyBindsMap.Insert(KeyCode.KC_VOLUMEDOWN,"KC_VOLUMEDOWN");
		m_KeyBindsMap.Insert(KeyCode.KC_VOLUMEUP,"KC_VOLUMEUP");
		m_KeyBindsMap.Insert(KeyCode.KC_WEBHOME,"KC_WEBHOME");
		m_KeyBindsMap.Insert(KeyCode.KC_NUMPADCOMMA,"KC_NUMPADCOMMA");
		m_KeyBindsMap.Insert(KeyCode.KC_DIVIDE,"KC_DIVIDE");
		m_KeyBindsMap.Insert(KeyCode.KC_SYSRQ,"KC_SYSRQ");
		m_KeyBindsMap.Insert(KeyCode.KC_RMENU,"KC_RMENU");
		m_KeyBindsMap.Insert(KeyCode.KC_PAUSE,"KC_PAUSE");
		m_KeyBindsMap.Insert(KeyCode.KC_HOME,"KC_HOME");
		m_KeyBindsMap.Insert(KeyCode.KC_UP,"KC_UP");
		m_KeyBindsMap.Insert(KeyCode.KC_PRIOR,"KC_PRIOR");
		m_KeyBindsMap.Insert(KeyCode.KC_LEFT,"KC_LEFT");
		m_KeyBindsMap.Insert(KeyCode.KC_RIGHT,"KC_RIGHT");
		m_KeyBindsMap.Insert(KeyCode.KC_END,"KC_END");
		m_KeyBindsMap.Insert(KeyCode.KC_DOWN,"KC_DOWN");
		m_KeyBindsMap.Insert(KeyCode.KC_NEXT,"KC_NEXT");
		m_KeyBindsMap.Insert(KeyCode.KC_INSERT,"KC_INSERT");
		m_KeyBindsMap.Insert(KeyCode.KC_DELETE,"KC_DELETE");
		m_KeyBindsMap.Insert(KeyCode.KC_LWIN,"KC_LWIN");
		m_KeyBindsMap.Insert(KeyCode.KC_RWIN,"KC_RWIN");
		m_KeyBindsMap.Insert(KeyCode.KC_APPS,"KC_APPS");
		m_KeyBindsMap.Insert(KeyCode.KC_POWER,"KC_POWER");
		m_KeyBindsMap.Insert(KeyCode.KC_SLEEP,"KC_SLEEP");
		m_KeyBindsMap.Insert(KeyCode.KC_WAKE,"KC_WAKE");
		m_KeyBindsMap.Insert(KeyCode.KC_MEDIASELECT,"KC_MEDIASELECT");
	}

	ref VPPKeybindsConfig getKeyBindsConfig()
	{
		return m_VPPKeybindsConfig;
	}

	ref map<int,string> getKeyBindsMap()
	{
		return m_KeyBindsMap;
	}

	void KickClientHandle( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<string> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	GetGame().GetUIManager().ShowDialog("Kicked From Server", data.param1, 1, DBT_OK, DBB_OK, DMT_WARNING, g_Game.GetUIManager().FindMenu(MENU_INGAME));
        	g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().DisconnectSessionForce, 1200, false);
        }
	}

	bool CheckESP()
	{
		return m_ESPToggle;
	}

	bool ESPType()
	{
		return m_ESPType;
	}

	void SetESPType(bool trigger)
	{
		m_ESPType = trigger; //true == detailed
	}

	void setESPStatus(bool trigger)
	{
		m_ESPToggle = trigger;
	}

	//-=-=-=-=-=-=-=-=-=-=-=-
	static string GetDate(bool fullStamp)
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
	
	
	static vector SwapYZ(vector vec)
	{
		vector tmp;
		tmp[0] = vec[0];
		tmp[1] = vec[2];
		tmp[2] = vec[1];
	
		return tmp;
	}

	static vector StringToVector(string str)
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

	static string VectorToString(vector vctr)
	{
		string oStr;
		oStr = vctr[0].ToString();
		oStr += "," + vctr[1].ToString();
		oStr += "," + vctr[2].ToString();
		return oStr;
	}

	static vector SnapToGround(vector pos)
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
	    {
	        return "0 0 0";
	    }
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
	    {
	        return "0 0 0";
	    }

	    vector rayStart = GetGame().GetCurrentCameraPosition();
	    vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * 10000;
	    vector hitPos;
	    vector hitNormal;
	    int hitComponentIndex;
	    DayZPhysics.RaycastRV(rayStart, rayEnd, hitPos, hitNormal, hitComponentIndex, NULL, NULL, GetPlayer());
	    return hitPos;
	}

	set< Object > GetObjectsAt( vector from, vector to, Object ignore = NULL, float radius = 1.5, Object with = NULL )
	{
	    vector contact_pos;
	    vector contact_dir;
	    int contact_component;

	    set< Object > geom = new set< Object >;
	    set< Object > view = new set< Object >;

	    DayZPhysics.RaycastRV( from, to, contact_pos, contact_dir, contact_component, geom, with, ignore, false, false, ObjIntersectGeom, radius );
	    DayZPhysics.RaycastRV( from, to, contact_pos, contact_dir, contact_component, view, with, ignore, false, false, ObjIntersectView, radius );

	    for ( int i = 0; i < geom.Count(); i++ )
	    {
	        view.Insert( geom[i] );
	    }

	    if ( view.Count() > 0 ) 
	    {
	        return view;
	    }
	    return NULL;
	}

	Object getObjectAtCrosshair(float distance = 1000.0, float radius = 1.5, Object with = NULL)
	{
	    vector rayStart = GetGame().GetCurrentCameraPosition();
	    vector rayEnd = rayStart + GetGame().GetCurrentCameraDirection() * distance;

	    auto objs = GetObjectsAt( rayStart, rayEnd, GetGame().GetPlayer(), radius, with );

	    if( objs.Count() > 0 )
	    {
	        return objs[ 0 ];
	    }

	    return NULL;
	}
}