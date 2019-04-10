class PlayerInfoHolder
{
    private string m_name;
    private string m_GUID;
    private int    m_PosInMapArray;
    private vector m_LastPos;

    private float  m_Health;
    private float  m_Blood;
    private float  m_Shock;

    void PlayerInfoHolder(string player_name, string SteamID, int MapPos){
        this.m_name          = player_name;
        this.m_GUID          = SteamID;
        this.m_PosInMapArray = MapPos;
    }

    void ~PlayerInfoHolder(){
        
    }

    string GetPlayerName(){
        return this.m_name;
    }

    string GetGUID(){
        return this.m_GUID;
    }

    int GetPosInMapArray(){
        return this.m_PosInMapArray;
    }

    vector GetLastPosition(){
        return m_LastPos;
    }

    void SetLastPosition(vector lastPos){
        this.m_LastPos = lastPos;
    }

    void SetHealthCache(float health)
    {
    	m_Health = health;
    }

    void SetBloodCache(float blood){
    	m_Blood = blood;
    }

    void SetShockCache(float shock){
    	m_Shock = shock;
    }

    float GetHealthCache(){
    	return m_Health;
    }

    float GetBloodCache(){
    	return m_Blood;
    }

    float GetShockCache(){
    	return m_Shock;
    }
}

class PlayerManagerUI
{
	ref array<ref PlayerInfoHolder> m_PlayerCacheArray = new array<ref PlayerInfoHolder>;
	ref PlayerInfoHolder oCache;
	ref array<string> m_SelectedPlayers = new array<string>;
	protected ref map<int,ref Param2<string,string>> m_PlayersMap; //Player name then GUID
	protected ref map<string, vector> m_TPLocations;

	protected VPPAdminHud m_MainFrameClass;
	protected Widget M_MAIN_ROOT;
	protected Widget M_SUB_ROOT;

	protected Widget m_MessageInputUI;
	protected Widget M_PANEL_TELEPORT;
	protected Widget M_PANEL_PERMISSIONS;

	private SliderWidget m_SliderHealth;
	private SliderWidget m_SliderBlood;
	private SliderWidget m_SliderShock;
	private TextWidget m_TxtHealth;
	private TextWidget m_TxtBlood;
	private TextWidget m_TxtShock;

	private ButtonWidget m_BtnSetHealth;
	private ButtonWidget m_BtnSetBlood;
	private ButtonWidget m_BtnSetShock;

	private TextWidget      m_TxtGUID;
	private TextWidget      m_TxtName;
	private XComboBoxWidget m_ComboWeatherManager;
	private XComboBoxWidget m_ComboServerSettings;
	private XComboBoxWidget m_ComboPlayerManager;
	private XComboBoxWidget m_ComboItemManager;
	private XComboBoxWidget m_ComboObjectEditor;
	private CheckBoxWidget  m_ToggleESP;
	private CheckBoxWidget  m_ESPType;

	private ButtonWidget m_BtnShowPlayerOnMap;
	private ButtonWidget m_BtnSendMessage;
	private ButtonWidget m_BtnHeal;
	private ButtonWidget m_BtnKill;
	private ButtonWidget m_BtnTpMeTo;
	private ButtonWidget m_BtnTpTo;
	private ButtonWidget m_BtnCopyID;
	private ButtonWidget m_BtnCopyName;
	private ButtonWidget m_BtnSelectAll;
	private ButtonWidget m_BtnRefreshList;
	private ButtonWidget m_BtnPermissionsManager;
	private ButtonWidget m_BtnApplyPerms;
	private ButtonWidget m_BtnTeleportManager;
	private ButtonWidget m_BtnTPMe;
	private ButtonWidget m_BtnTPSelectedP;
	private ButtonWidget m_BtnKick;
	private ButtonWidget m_BtnRemoveAdmin;
	private TextListboxWidget m_PlayersList;

	private CheckBoxWidget m_ChkBoxTPSelected;

	private TextListboxWidget m_TeleportList;
	private EditBoxWidget     m_InputX;
	private EditBoxWidget     m_InputZ;
	private EditBoxWidget     m_InputY;

	private EditBoxWidget m_BoxInputTitle;
	private EditBoxWidget m_BoxInputMessage;
	private ButtonWidget  m_BtnConfirmMessage;

	private TextWidget m_TxtSelectedNum;
	private MultilineEditBoxWidget m_TxtPlayerGUID;
	private MultilineEditBoxWidget m_TxtPlayerName;

	private ButtonWidget      m_BtnMapTP;
	protected Widget 		  m_Panel_MapTeleport;
	protected ButtonWidget    m_BtnRefreshMap;
	protected MapWidget		  m_TPMapWidget;

	protected bool m_HasSuperAdmin;
	protected bool m_updateMapWidget;

	void PlayerManagerUI( VPPAdminHud MainFrameClass )
	{
		//----RPC's----
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "SortTeleportPositions", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "SortPlayerList", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "SortPlayerPosition", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "SortPlayerPermissions", this, SingeplayerExecutionType.Server );
		//-------------
		m_MainFrameClass  = MainFrameClass;
		m_PlayersMap      = new map<int,ref Param2<string,string>>;

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.onUpdate, 700, true);
	}

	void ~PlayerManagerUI()
	{
		m_MainFrameClass = NULL;

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.onUpdate);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.ButtonCoolDown);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.UpdateCacheClass);
	}

	void Init(Widget RootLayout, Widget SubRootLayout, bool HasSuperAdmin)
	{
		M_MAIN_ROOT  = RootLayout;
		M_SUB_ROOT   = SubRootLayout;

		M_PANEL_TELEPORT     = Widget.Cast( M_MAIN_ROOT.FindAnyWidget( "Panel_Teleport" ) );
		M_PANEL_PERMISSIONS  = Widget.Cast( M_MAIN_ROOT.FindAnyWidget( "Panel_Permissions" ) );
		ref InfoBoxInstance m_InfoBoxInstance = new InfoBoxInstance(M_PANEL_PERMISSIONS,"ImageHelpPerms","Permissions:","If a player is already on the admin list, the button 'Remove' will show up to COMPLETELY remove them as an admin! NOTE: Does not work for super admins! You must remove them manually! Player needs to relog afet applying permissions (temp)");
		ref InfoBoxInstance m_InfoBoxInstance_playerManager = new InfoBoxInstance(M_SUB_ROOT,"PlayerManagerInfoImg","Help:","2x CLICK on a player name in the list to select them, 2x CLICK again to deselect. You have to select your player if you wish to use Command buttons on YOU. Refresh button will deselect and refresh.");

		m_MainFrameClass.m_DraggableWindows.Insert("Panel_Teleport","true");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_PANEL_TELEPORT,  m_MainFrameClass, "IsDraggingWindow" );
		m_MainFrameClass.m_DraggableWindows.Insert("Panel_Permissions","true");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_PANEL_PERMISSIONS,  m_MainFrameClass, "IsDraggingWindow" );
		m_MainFrameClass.m_DraggableWindows.Insert("Panel_PlayerManager","true");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_SUB_ROOT,  m_MainFrameClass, "IsDraggingWindow" );
 
		m_BtnPermissionsManager = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnPermissions" ) );
		if (HasSuperAdmin)
		{
			m_BtnPermissionsManager.Enable(true);
			m_BtnPermissionsManager.Show(true);
			m_HasSuperAdmin = HasSuperAdmin;
		}
		else if (!HasSuperAdmin)
		{
			m_BtnPermissionsManager.Show(false);
			m_BtnPermissionsManager.Enable(false);
			m_HasSuperAdmin = HasSuperAdmin;
		}

		m_BtnRemoveAdmin        = ButtonWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "BtnRemoveAdmin" ) );
		m_BtnRemoveAdmin.Enable(false);
		m_BtnApplyPerms         = ButtonWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "BtnApplyPerms" ) );
		m_TxtGUID               = TextWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "TxtGUID" ) );
		m_TxtName               = TextWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "TxtName" ) );
		m_ComboWeatherManager   = XComboBoxWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "ComboWeatherManager" ) );
		m_ComboServerSettings   = XComboBoxWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "ComboServerSettings" ) );
		m_ComboPlayerManager    = XComboBoxWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "ComboPlayerManager" ) );
		m_ComboItemManager      = XComboBoxWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "ComboItemManager" ) );
		m_ComboObjectEditor     = XComboBoxWidget.Cast( M_PANEL_PERMISSIONS.FindAnyWidget( "ComboObjectEditor" ) );

		m_BtnShowPlayerOnMap    = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnShowPlayerOnMap" ) );
		m_BtnTeleportManager    = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnTeleportManager" ) );
		m_BtnRefreshList		= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnRefreshList" ) );
		m_BtnSelectAll		    = ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnSelectAll" ) );
		m_BtnTpTo 				= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnTpTo" ) );
		m_BtnTpMeTo  			= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnTpMeTo" ) );
		m_BtnKill 				= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnKill" ) );
		m_BtnHeal 				= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnHeal" ) );
		m_BtnSendMessage 		= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnSendMessage" ) );
		m_BtnKick				= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnKick" ) );
		//Tp Panel
		
		m_BtnMapTP				= ButtonWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "BtnMapTP" ) );
		m_BtnTPSelectedP		= ButtonWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "BtnTPSelectedP" ) );
		m_BtnTPMe     		    = ButtonWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "BtnTPMe" ) );
		m_TeleportList 			= TextListboxWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "TeleportList" ) );
		m_InputX				= EditBoxWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "InputX" ) );
		m_InputZ				= EditBoxWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "InputZ" ) );
		m_InputY				= EditBoxWidget.Cast( M_PANEL_TELEPORT.FindAnyWidget( "InputY" ) );

		m_MessageInputUI		= Widget.Cast( M_MAIN_ROOT.FindAnyWidget( "MessageInputUI" ) );
		m_BoxInputTitle			= EditBoxWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "BoxInputTitle" ) );
		m_BoxInputMessage		= EditBoxWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "BoxInputMessage" ) );
		m_BtnConfirmMessage		= ButtonWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "BtnConfirmMessage" ) );

		m_Panel_MapTeleport     = Widget.Cast( M_MAIN_ROOT.FindAnyWidget( "Panel_MapTeleport" ) );
		m_TPMapWidget			= MapWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "TPMapWidget" ) );
		m_ChkBoxTPSelected		= CheckBoxWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "ChkBoxTPSelected" ) );
		m_BtnRefreshMap		    = ButtonWidget.Cast( M_MAIN_ROOT.FindAnyWidget( "BtnRefreshMap" ) );
		WidgetEventHandler.GetInstance().RegisterOnDoubleClick( m_TPMapWidget, this, "MapDoubleClick" );

		//----
		m_SliderHealth          = SliderWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "SliderHealth" ) );
		m_SliderBlood			= SliderWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "SliderBlood" ) );
		m_SliderShock			= SliderWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "SliderShock" ) );
		m_TxtHealth             = TextWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "TxtHealth" ) );
		m_TxtBlood			    = TextWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "TxtBlood" ) );
		m_TxtShock			    = TextWidget.Cast(  M_SUB_ROOT.FindAnyWidget( "TxtShock" ) );

		m_BtnSetHealth			= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnSetHealth" ) );
		m_BtnSetBlood			= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnSetBlood" ) );
		m_BtnSetShock			= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnSetShock" ) );
		//---

		m_ToggleESP      	    = CheckBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ToggleESP" ) );
		m_ESPType			    = CheckBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "ESPType" ) );
		m_ToggleESP.SetChecked(g_Game.CheckESP());
		m_ESPType.SetChecked(g_Game.ESPType());

		m_BtnCopyID				= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnCopyID" ) );
		m_BtnCopyName 			= ButtonWidget.Cast( M_SUB_ROOT.FindAnyWidget( "BtnCopyName" ) );
		m_TxtPlayerGUID         = MultilineEditBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "TxtPlayerGUID" ) );
		m_TxtPlayerName         = MultilineEditBoxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "TxtPlayerName" ) );
		m_TxtSelectedNum        = TextWidget.Cast( M_SUB_ROOT.FindAnyWidget( "TxtSelectedNum" ) );
		m_PlayersList	        = TextListboxWidget.Cast( M_SUB_ROOT.FindAnyWidget( "PlayerList" ) );
		WidgetEventHandler.GetInstance().RegisterOnDoubleClick( m_PlayersList, this, "PlayerListDoubleClick" );
		GetPlayerList();
		GetTPLocations();
	}


	void onUpdate()
	{
		if (M_SUB_ROOT != NULL && M_SUB_ROOT.IsVisible())
		{
			m_TxtSelectedNum.SetText("Selected: "+m_SelectedPlayers.Count());

			if (m_SelectedPlayers.Count() == 1)
			{
				m_BtnPermissionsManager.Enable(true);

				int SelectedRow = m_PlayersList.GetSelectedRow();
				string SelectedName;
				if (SelectedRow != -1)
				{
					m_PlayersList.GetItemText(SelectedRow, 0, SelectedName);
					for (int i = 0; i < m_PlayerCacheArray.Count(); ++i)
				    {
				    	oCache = m_PlayerCacheArray.Get(i);
				    	if (SelectedName == oCache.GetPlayerName())
				    	{
				    		m_TxtPlayerGUID.SetText("GUID: "+oCache.GetGUID());
							m_TxtPlayerName.SetText("NAME: "+SelectedName);
				    	}
				    }
				}
			}
			else
			{
				m_BtnPermissionsManager.Enable(false);
			}

			if (m_Panel_MapTeleport != NULL && m_Panel_MapTeleport.IsVisible() && m_updateMapWidget)
			{
				m_TPMapWidget.ClearUserMarks();
				PlayerBase playerAdmin;
				playerAdmin = PlayerBase.Cast( GetGame().GetPlayer() );
				m_TPMapWidget.AddUserMark(playerAdmin.GetPosition(), "Me(Admin)", ARGB(255,255,255,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
			}
		}
	}

	void ButtonCoolDown(ButtonWidget button)
	{
		button.Enable(true);
		button.Update();
	}

	void RequestUpdateCacheClass()
	{
		//Get Player Positions to cache for selected player(s)
	    Param1<ref array<string>> param = new Param1<ref array<string>>(m_SelectedPlayers);
		GetRPCManager().SendRPC( "RPC_AdminTools", "GetPlayerPosition", param, true);
	}

	void RefreshPlayersMap()
	{
		m_TPMapWidget.ClearUserMarks();
	    for (int i = 0; i < m_PlayerCacheArray.Count(); ++i)
	    {
	    	oCache = m_PlayerCacheArray.Get(i);
			m_TPMapWidget.AddUserMark(oCache.GetLastPosition(), oCache.GetPlayerName(), ARGB(255,255,255,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
	    }
	}

	void UpdateCacheClass()
	{
		if (m_SelectedPlayers.Count() == 1)
		{
			string selectedGUID = m_SelectedPlayers.Get(0);
			for (int i = 0; i < m_PlayerCacheArray.Count(); ++i)
		    {
		    	oCache = m_PlayerCacheArray.Get(i);
		    	if (selectedGUID == oCache.GetGUID())
		    	{
	    			m_SliderHealth.SetCurrent(oCache.GetHealthCache());
					m_SliderBlood.SetCurrent(oCache.GetBloodCache());
					m_SliderShock.SetCurrent(oCache.GetShockCache());
					m_TxtHealth.SetText(oCache.GetHealthCache().ToString());
					m_TxtBlood.SetText(oCache.GetBloodCache().ToString());
					m_TxtShock.SetText(oCache.GetShockCache().ToString());
		    	}
		    }
		}
	}

	void KickPlayer()
	{
		Param1<ref array<string>> map_param = new Param1<ref array<string>>(m_SelectedPlayers);
        GetRPCManager().SendRPC( "RPC_AdminTools", "KickPlayer", map_param, true );
	}

	void SendMessage()
	{
		string TitleText,Message;
		TitleText = m_BoxInputTitle.GetText();
		Message   = m_BoxInputMessage.GetText();

		if (TitleText != "" && Message != "")
		{
			Param3<string,string,ref array<string>> map_param = new Param3<string,string,ref array<string>>(TitleText,Message,m_SelectedPlayers);
        	GetRPCManager().SendRPC( "RPC_AdminTools", "SendMessage", map_param, true );

        	m_MessageInputUI.Show(false);
		}
	}

	void HealPlayer()
	{
		Param1<ref array<string>> map_param = new Param1<ref array<string>>(m_SelectedPlayers);
        GetRPCManager().SendRPC( "RPC_AdminTools", "HealPlayer", map_param, true );
	}

	void KillPlayer()
	{
		Param1<ref array<string>> map_param = new Param1<ref array<string>>(m_SelectedPlayers);
        GetRPCManager().SendRPC( "RPC_AdminTools", "KillPlayer", map_param, true );
	}

	void PlayersTeleport(bool TPPlayerToMe)
	{
		Param2<bool,ref array<string>> map_param;
		if (TPPlayerToMe)
		{
			map_param = new Param2<bool,ref array<string>>(true,m_SelectedPlayers);
	        GetRPCManager().SendRPC( "RPC_AdminTools", "PlayersTeleport", map_param, true );
		}
		else
		{
			map_param = new Param2<bool,ref array<string>>(false,m_SelectedPlayers);
	        GetRPCManager().SendRPC( "RPC_AdminTools", "PlayersTeleport", map_param, true );
		}
	}

	void MapDoubleClick(Widget w, int x, int y, int button)
	{
		vector world_pos = ScreenToWorld();
		Param3<vector,ref array<string>,bool> map_param;
		if (world_pos)
		{
			if (m_ChkBoxTPSelected.IsChecked())
			{
				if (m_SelectedPlayers.Count() >= 1)
				{
					map_param = new Param3<vector,ref array<string>,bool>(world_pos,m_SelectedPlayers,false);
	        		GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", map_param, true );
				}
			}
			else
			{
				map_param = new Param3<vector,ref array<string>,bool>(world_pos,NULL,true);
	        	GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", map_param, true );
			}
		}
		RefreshPlayersMap();
	}

	vector ScreenToWorld()
	{
		vector world_pos,ScreenToMap,dir,from,to;

		dir = GetGame().GetPointerDirection();
	    from = GetGame().GetCurrentCameraPosition();
	    to = from + ( dir * 1000 );

		world_pos = GetGame().GetScreenPos( to );
	    ScreenToMap = m_TPMapWidget.ScreenToMap( world_pos );
	    return ScreenToMap;
	}

	void AlterStatus(string WhichOne)
	{
		Param3<float,string,ref array<string>> map_param; //value,type
		if (m_SelectedPlayers != NULL && m_SelectedPlayers.Count() >= 1)
		{
			switch(WhichOne)
			{
				case "Health":
				float fltHealth = m_SliderHealth.GetCurrent();
				map_param = new Param3<float,string,ref array<string>>(fltHealth,WhichOne,m_SelectedPlayers);
	        	GetRPCManager().SendRPC( "RPC_AdminTools", "AlterStatus", map_param, true );
				break;

				case "Blood":
				float fltBlood = m_SliderBlood.GetCurrent();
				map_param = new Param3<float,string,ref array<string>>(fltBlood,WhichOne,m_SelectedPlayers);
	        	GetRPCManager().SendRPC( "RPC_AdminTools", "AlterStatus", map_param, true );
				break;

				case "Shock":
				float fltShock = m_SliderShock.GetCurrent();
				map_param = new Param3<float,string,ref array<string>>(fltShock,WhichOne,m_SelectedPlayers);
	        	GetRPCManager().SendRPC( "RPC_AdminTools", "AlterStatus", map_param, true );
				break;
			}
		}
	}

	void GetPlayerPermissions()
	{
		Param1<ref array<string>> map_param = new Param1<ref array<string>>(m_SelectedPlayers);
	    GetRPCManager().SendRPC( "RPC_AdminTools", "GetPlayerPermissions", map_param, true );
	}

	void ApplyPlayerPermissions(string AdminName, string GUID)
	{ 
		int ComboPlayerManager,ComboServerSettings,ComboWeatherManager,ComboItemManager,ComboObjectEditor;
		ComboPlayerManager  = m_ComboPlayerManager.GetCurrentItem();
		ComboServerSettings = m_ComboServerSettings.GetCurrentItem();
		ComboWeatherManager = m_ComboWeatherManager.GetCurrentItem();
		ComboItemManager    = m_ComboItemManager.GetCurrentItem();
		ComboObjectEditor   = m_ComboObjectEditor.GetCurrentItem();
		ref array<string> m_PermsData = new array<string>;
		if (ComboPlayerManager  == 1) m_PermsData.Insert("PlayerManager"); 
		if (ComboServerSettings == 1) m_PermsData.Insert("ServerManager");
		if (ComboWeatherManager == 1) m_PermsData.Insert("WeatherManager");
		if (ComboItemManager    == 1) m_PermsData.Insert("ItemManager");
		if (ComboObjectEditor   == 1) m_PermsData.Insert("ObjectEditor");

		Param3<string,string,ref array<string>> map_param = new Param3<string,string,ref array<string>>(AdminName,GUID,m_PermsData);
	    GetRPCManager().SendRPC( "RPC_PermitManager", "SaveAdminPermissions", map_param, true );
	}

	void SortPlayerPermissions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		//UID, PERMS
		Param2<string, ref array<string>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	ref array<string> m_Perms = new array<string>;
        	m_Perms.Copy(data.param2);
        	m_TxtGUID.SetText("GUID: "+data.param1);
        	for (int i = 0; i < m_PlayerCacheArray.Count(); ++i)
		    {
		    	oCache = m_PlayerCacheArray.Get(i);
		    	if (data.param1 == oCache.GetGUID())
		    	{
		    		m_TxtName.SetText("Name: "+oCache.GetPlayerName());
		    	}
		    }

		    m_BtnRemoveAdmin.Enable(true);

	        for (int x = 0; x < m_Perms.Count(); ++x)
	        {
	        	string line_content = m_Perms.Get(x);
	        	switch(line_content)
	        	{
	        		case "PlayerManager":
	        		m_ComboPlayerManager.SetCurrentItem(1);
	        		break;

	        		case "ServerManager":
	        		m_ComboServerSettings.SetCurrentItem(1);
	        		break;

	        		case "WeatherManager":
	        		m_ComboWeatherManager.SetCurrentItem(1);
	        		break;

	        		case "ItemManager":
	        		m_ComboItemManager.SetCurrentItem(1);
	        		break;

	        		case "ObjectEditor":
	        		m_ComboObjectEditor.SetCurrentItem(1);
	        		break;
	        	}
	        }
        }
	}

	void SortTeleportPositions( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref map<string, vector>> data;
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	m_TPLocations = new map<string, vector>;
        	m_TPLocations.Copy(data.param1);
        	m_TPLocations.Sort(m_TPLocations,m_TPLocations.Count());
        	for (int i = 0; i < m_TPLocations.Count(); ++i)
		    {
		    	string PosName = m_TPLocations.GetKey(i);
		    	vector PosVec  = m_TPLocations.Get(PosName);
		    	string StrPos  = g_Game.VectorToString(PosVec);
		    	m_TeleportList.AddItem(PosName, NULL, 0 );	
				m_TeleportList.SetItem( i, StrPos, NULL, 1 );
		    }
        }
	}

	void TeleportToPosition(bool SelectionType)
	{
		string PosX,PosZ,PosY,Final;
		PosX = m_InputX.GetText();
		PosZ = m_InputZ.GetText();
		PosY = m_InputY.GetText();

		Final = PosX;
		Final += ","+PosZ;
		Final += ","+PosY;
		vector TPPos = g_Game.StringToVector(Final);

		Param3<vector,ref array<string>,bool> map_param;

		if (SelectionType)
		{
			ref array<string> oGuids = new array<string>;
		    for (int i = 0; i < m_SelectedPlayers.Count(); ++i)
		    {
		    	string oID = m_SelectedPlayers.Get(i);
		    	oGuids.Insert(oID);
		    }

		    map_param = new Param3<vector,ref array<string>,bool>(TPPos,oGuids,false);
	        GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", map_param, true );
		}
		else if (!SelectionType)
		{
			map_param = new Param3<vector,ref array<string>,bool>(TPPos,NULL,true);
	        GetRPCManager().SendRPC( "RPC_AdminTools", "TeleportToPosition", map_param, true );
		}
	}

	void GetTPLocations()
	{
	    Param1<string> map_param = new Param1<string>("CuntHole");
        GetRPCManager().SendRPC( "RPC_AdminTools", "LoadTeleportPositions", map_param, true );
	}

	//Removes dupes if there is any
	bool CheckForDuplicate(string GUID)
	{
	    for (int i = 0; i < m_SelectedPlayers.Count(); ++i)
	    {
	    	string gettedID = m_SelectedPlayers.Get(i);
	    	if (GUID == gettedID)
	    	{
	    		m_SelectedPlayers.Remove(i);
	    		Print("[CheckForDuplicate] Removed:: "+gettedID);
	    		return true;
	    	}
	    }
		return false;
	}

	void SelectAllPlayers()
	{
		m_SelectedPlayers  = new array<string>;
	    for (int i = 0; i < m_PlayersMap.Count(); ++i)
	    {
	    	int mapPos = m_PlayersMap.GetKey(i);
	    	ref Param2<string,string> oData = m_PlayersMap.Get(mapPos);
	    	m_SelectedPlayers.Insert(oData.param2);
	    }
	}

	void PlayerListDoubleClick(Widget w, int x, int y, int button)
	{
		string SelectedName;
		int SelectedRow;
		int i = 0;
		if (w == m_PlayersList)
		{
			//add player
			SelectedRow = m_PlayersList.GetSelectedRow();
			if (SelectedRow != -1)
			{
				SelectedName;
				m_PlayersList.GetItemText(SelectedRow, 0, SelectedName);
				for (i = 0; i < m_PlayerCacheArray.Count(); ++i)
			    {
			    	oCache = m_PlayerCacheArray.Get(i);
			    	if (SelectedName == oCache.GetPlayerName())
			    	{
			    		if (!CheckForDuplicate(oCache.GetGUID()))
			    		{
			    			m_SelectedPlayers.Insert(oCache.GetGUID());
			    			m_SliderHealth.SetCurrent(oCache.GetHealthCache());
							m_SliderBlood.SetCurrent(oCache.GetBloodCache());
							m_SliderShock.SetCurrent(oCache.GetShockCache());
			    			Print("[Add] m_SelectedPlayers:: "+oCache.GetGUID());
			    		}
			    	}
			    }
			}
		}
	}

	void SortPlayerList( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
    	Param1<ref map<string,string>> data;
        if ( !ctx.Read( data ) ) return;

        ref map<string,string> CopyTmp = new map<string,string>;
        ref array<string> m_GuidsStrArr = new array<string>;
        CopyTmp.Copy(data.param1);

        if (type == CallType.Client)
        {
        	if (CopyTmp != NULL)
        	{
    		    for (int i = 0; i < CopyTmp.Count(); ++i)
    		    {
    		    	string PlayerName = CopyTmp.GetKey(i);
    		    	string GUID  	  = CopyTmp.Get(PlayerName);
    		    	m_PlayersList.AddItem(PlayerName, NULL, 0, -1 );

    		    	ref Param2<string,string> oData = new Param2<string,string>(PlayerName,GUID);
    		    	m_PlayersMap.Insert(i,oData);

    		    	m_GuidsStrArr.Insert(GUID); //Used as a temp for 'GetPlayerPosition'

    		    	oCache = new PlayerInfoHolder(PlayerName,GUID,i);
					m_PlayerCacheArray.Insert(oCache);
    		    }

    		    //Get Player Positions to cache
    		    Param1<ref array<string>> param = new Param1<ref array<string>>(m_GuidsStrArr);
       			GetRPCManager().SendRPC( "RPC_AdminTools", "GetPlayerPosition", param, true);
        	}
        }
    }

    //Also does blood,health,shock cache
    void SortPlayerPosition( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<ref map<string,vector>> data; //GUID, Position
        if ( !ctx.Read( data ) ) return;

        if (type == CallType.Client)
        {
        	ref map<string,vector> mPosData = new map<string,vector>;
        	mPosData.Copy(data.param1);
        	if (mPosData)
        	{
        		for (int i = 0; i < mPosData.Count(); ++i)
	    	    {
	    	    	string GUID     = mPosData.GetKey(i);
	    	    	vector cachePos = mPosData.Get(GUID);
	    	    	for (int x = 0; x < m_PlayerCacheArray.Count(); ++x)
				    {
				    	TStringArray strs = new TStringArray; // 0 == Guid || 1 == health || 2 == blood || 3 == shock
				    	GUID.Split("|",strs);

				    	oCache = m_PlayerCacheArray.Get(x);
				    	if (strs.Get(0) == oCache.GetGUID())
				    	{
				    		oCache.SetLastPosition(cachePos);
				    		oCache.SetHealthCache(strs.Get(1).ToFloat());
							oCache.SetBloodCache(strs.Get(2).ToFloat());
							oCache.SetShockCache(strs.Get(3).ToFloat());
				    	}
				    }
	    	    }
        	}
        }
	}

    void GetPlayerList()
    {
    	Param1<string> param = new Param1<string>("BitchAssHOE!");
       	GetRPCManager().SendRPC( "RPC_AdminTools", "GetPlayerList", param, true);

       	m_PlayersMap       = new map<int,ref Param2<string,string>>;
       	m_PlayerCacheArray = new array<ref PlayerInfoHolder>;
       	m_SelectedPlayers  = new array<string>;
    }

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    bool OnChange(Widget w, int x, int y, bool finished)
    {
    	return false;
    }

	bool OnClick( Widget w, int x, int y, int button )
	{
		string tmpStr;
		if (w == m_BtnTeleportManager)
		{
			if (M_PANEL_TELEPORT.IsVisible())
			{
				M_PANEL_TELEPORT.Show(false);
			}
			else
			{
				M_PANEL_TELEPORT.Show(true);
			}
			return true;
		}

		if (w == m_ESPType)
		{
			g_Game.SetESPType(m_ESPType.IsChecked());
			return true;
		}

		if (w == m_ToggleESP)
		{
			if (m_ToggleESP.IsChecked() && !g_Game.CheckESP())
			{
				g_Game.setESPStatus(true);
			}else{
				g_Game.setESPStatus(false);
			}
			return true;
		}

		switch(w)
		{
			case m_BtnPermissionsManager:
			if (M_PANEL_PERMISSIONS.IsVisible())
			{
				M_PANEL_PERMISSIONS.Show(false);
			}
			else
			{
				GetPlayerPermissions();
				M_PANEL_PERMISSIONS.Show(true);
			}
			return true;
			break;

			case m_BtnRemoveAdmin:
			//Do remove
			Param1<ref array<string>> remove_param = new Param1<ref array<string>>(m_SelectedPlayers);
        	GetRPCManager().SendRPC( "RPC_AdminTools", "RemoveAdminByGUID", remove_param, true );
			m_BtnRemoveAdmin.Enable(false);
			M_PANEL_PERMISSIONS.Show(false);
			return true;
			break;

			case m_BtnApplyPerms:
			string PlayerGUID,PlayerName;
			for (int i = 0; i < m_PlayerCacheArray.Count(); ++i)
		    {
		    	oCache = m_PlayerCacheArray.Get(i);
		    	string SelectedGUID = m_SelectedPlayers.Get(0);
		    	if (SelectedGUID == oCache.GetGUID())
		    	{
		    		PlayerGUID = oCache.GetGUID();
		    		PlayerName = oCache.GetPlayerName();
		    	}
		    }
			ApplyPlayerPermissions(PlayerName,PlayerGUID);
			M_PANEL_PERMISSIONS.Show(false);
			return true;
			break;

			case m_BtnRefreshList:
			m_PlayersList.ClearItems();
			GetPlayerList();
			m_BtnRefreshList.Enable(false);
			m_BtnRefreshList.Update();
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 5000, false, m_BtnRefreshList);
			return true;
			break;

			case m_BtnSelectAll:
			SelectAllPlayers();
			return true;
			break;

			case m_BtnCopyID:
			m_TxtPlayerGUID.GetText(tmpStr);
			GetGame().CopyToClipboard( tmpStr );
			return true;
			break;

			case m_BtnCopyName:
			m_TxtPlayerName.GetText(tmpStr);
			GetGame().CopyToClipboard( tmpStr );
			return true;
			break;

			case m_TeleportList:
			int oRow = m_TeleportList.GetSelectedRow();
			string Coords;
			if (oRow != -1)
			{
				m_TeleportList.GetItemText(oRow, 1, Coords);
				TStringArray strs = new TStringArray;
				Coords.Split(",",strs);

				m_InputX.SetText(strs.Get(0));
				m_InputZ.SetText(strs.Get(1));
				m_InputY.SetText(strs.Get(2));
			}
			return true;
			break;

			case m_BtnTPMe:
			string PosX,PosZ,PosY;
			PosX = m_InputX.GetText();
			PosZ = m_InputZ.GetText();
			PosY = m_InputY.GetText();
			if (PosX != "" && PosZ != "" && PosY != "")
			{
				TeleportToPosition(false);
			}
			return true;
			break;

			case m_BtnTPSelectedP:
			if (m_SelectedPlayers.Count() >= 1)
			{
				TeleportToPosition(true);
				return true;
			}
			break;

			case m_BtnMapTP:
			if (m_Panel_MapTeleport != NULL && m_Panel_MapTeleport.IsVisible())
			{
				m_Panel_MapTeleport.Show(false);
				m_ChkBoxTPSelected.Show(false);
				m_updateMapWidget = false;
			}
			else if (m_Panel_MapTeleport != NULL)
			{
				m_Panel_MapTeleport.Show(true);
				m_ChkBoxTPSelected.Show(true);
				m_updateMapWidget = true;

				PlayerBase playerAdmin;
				playerAdmin = PlayerBase.Cast( GetGame().GetPlayer() );
				m_TPMapWidget.AddUserMark(playerAdmin.GetPosition(), "Me(Admin)", ARGB(255,255,255,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
			}
			return true;
			break;

			//--Health stuff
			case m_BtnSetHealth:
				AlterStatus("Health");
				m_BtnSetHealth.Enable(false);
				m_BtnSetHealth.Update();
				RequestUpdateCacheClass();
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 3000, false, m_BtnSetHealth);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdateCacheClass, 2000, false);
				return true;
			break;

			case m_BtnSetBlood:
				AlterStatus("Blood");
				m_BtnSetBlood.Enable(false);
				m_BtnSetBlood.Update();
				RequestUpdateCacheClass();
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 3000, false, m_BtnSetBlood);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdateCacheClass, 2000, false);
				return true;
			break;

			case m_BtnSetShock:
				AlterStatus("Shock");
				m_BtnSetShock.Enable(false);
				m_BtnSetShock.Update();
				RequestUpdateCacheClass();
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 3000, false, m_BtnSetShock);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.UpdateCacheClass, 2000, false);
				return true;
			break;
			//---

			case m_BtnTpTo:
			if (m_SelectedPlayers.Count() >= 1)
			{
				PlayersTeleport(true); //true = tp him to me
				return true;
			}
			break;

			case m_BtnTpMeTo:
			if (m_SelectedPlayers.Count() == 1)
			{
				PlayersTeleport(false); //false = tp me to him
				return true;
			}
			break;

			case m_BtnKill:
			if (m_SelectedPlayers.Count() >= 1)
			{
				KillPlayer();
				return true;
			}
			break;

			case m_BtnHeal:
			if (m_SelectedPlayers.Count() >= 1)
			{
				HealPlayer();
				Param1<string> testparam = new Param1<string>("CuntHole");
        		GetRPCManager().SendRPC( "RPC_AdminTools", "PhysicsTest", testparam, true );
				return true;
			}
			break;

			case m_BtnSendMessage:
			if (m_SelectedPlayers.Count() >= 1)
			{
				if (m_MessageInputUI.IsVisible())
				{
					m_MessageInputUI.Show(false);
				}
				else
				{
					m_MessageInputUI.Show(true);
				}
				return true;
			}
			break;

			case m_BtnConfirmMessage:
			if (m_SelectedPlayers.Count() >= 1)
			{
				SendMessage();
				return true;
			}
			break;

			case m_BtnKick:
			if (m_SelectedPlayers.Count() >= 1)
			{
				KickPlayer();
				return true;
			}
			break;

			case m_BtnShowPlayerOnMap:
			if (m_Panel_MapTeleport != NULL && m_Panel_MapTeleport.IsVisible())
			{
				m_Panel_MapTeleport.Show(false);
				m_BtnRefreshMap.Show(false);
				m_updateMapWidget = true;
			}
			else if (m_Panel_MapTeleport != NULL)
			{
				m_Panel_MapTeleport.Show(true);
				m_BtnRefreshMap.Show(true);
				m_updateMapWidget = false;
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.RefreshPlayersMap, 1200, false);
			}
			break;

			case m_BtnRefreshMap:
			m_PlayersList.ClearItems();
			GetPlayerList();
			m_BtnRefreshMap.Enable(false);
			m_BtnRefreshMap.Update();
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ButtonCoolDown, 5000, false, m_BtnRefreshMap);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.RefreshPlayersMap, 1200, false);
			return true;
			break;
		}

		return false;
	}
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}