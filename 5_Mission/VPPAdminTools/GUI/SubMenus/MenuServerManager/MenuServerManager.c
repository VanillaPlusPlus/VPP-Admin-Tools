class MenuServerManager extends AdminHudSubMenu
{
	/*-Server Status Monitor-*/
	private TextWidget m_ServerFPSInput, m_PlayerCountInput, m_UpTimeInput, m_MemoryUsedInput, m_NetworkOutInput, m_NetworkInInput, m_ActiveAIInput;
	/*-----------------------*/

	private MapWidget      m_ActivityMap;
	private ButtonWidget   m_RefreshActivityMap;
	private ButtonWidget   m_RestartServer;
	private ButtonWidget   m_BtnKickAll;
	private CheckBoxWidget m_ToggleServerMonitor;
	private CheckBoxWidget m_ChkBoxUpdateActivity;
	private EditBoxWidget  m_InputAdminPassword;
	private ButtonWidget   m_BtnAdminLogin;
	private ImageWidget    m_ImgLoginInfo;
	private ButtonWidget   m_BtnLockServer;
	/*-Pinned (floating) server monitor-*/
	private ButtonWidget   m_BtnPinMonitor;
	private ImageWidget    m_ImgPinMonitor;
	private bool           m_MonitorPinned;
	private Widget         m_MonitorPinWidget;
	private TextWidget     m_PinFPS, m_PinPlayers, m_PinUpTime, m_PinMemory, m_PinNetOut, m_PinNetIn, m_PinAI;
	private float          m_MonitorFeedTick;
	/*----------------------------------*/
	private string 	   	   m_AdminPassword;
	private bool 		   m_loaded;
	private bool           m_AdminloggedIn;
	private ref Timer timer;
	private string m_pwinput;
	
	float UpdateInterval = 1.0;
	float UpdateTick;
	
	void MenuServerManager()
	{
		/*RPCs*/
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "UpdateServerMonitor", this, SingeplayerExecutionType.Client );
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "UpdateActivityMap", this, SingeplayerExecutionType.Client );
		/*-----*/
	}

	void ~MenuServerManager()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.MonitorFeedUpdate);
		if (m_MonitorPinWidget)
			m_MonitorPinWidget.Unlink();
	}

	override void HideBrokenWidgets(bool state)
	{
		m_ActivityMap.Show(!state);
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return; //Update cancels if sub menu is not visible.
		
		//Update Up-Time & player count
		UpdateTick += timeslice;
		if (UpdateTick >= UpdateInterval)
		{
			if (m_ChkBoxUpdateActivity.IsChecked())
			{
				GetRPCManager().VSendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
			}
			UpdateTick = 0.0;
		}
		
		//Password cover
		if (m_pwinput != m_InputAdminPassword.GetText())
		{
			string hashed;
			string raw = m_InputAdminPassword.GetText();
			int inputLength = raw.Length();
			raw.Replace("*","");
			
			if (inputLength > m_AdminPassword.Length()){
				m_AdminPassword += raw;
			}else{
				m_AdminPassword = m_AdminPassword.Substring( 0, inputLength);
			}
			
			for(int i = 0; i < inputLength; i++){
				hashed += "*";
			}
			m_InputAdminPassword.SetText(hashed);
			m_pwinput = hashed;
		}
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuServerManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

		m_ToggleServerMonitor  = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ToggleServerMonitor") );
		m_ChkBoxUpdateActivity = CheckBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ChkBoxUpdateActivity") );
		m_InputAdminPassword  = EditBoxWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "InputAdminPassword") );
		m_BtnAdminLogin 	  = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnAdminLogin") );
		
		m_ServerFPSInput   = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ServerFPSInput") );
		m_PlayerCountInput = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PlayerCountInput") );
		m_UpTimeInput      = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "UpTimeInput") );
		m_MemoryUsedInput  = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "MemoryUsedInput") );
		m_NetworkInInput   = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "NetworkInInput") );
		m_NetworkOutInput  = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "NetworkOutInput") );
		m_ActiveAIInput    = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ActiveAIInput") );
		
		m_ActivityMap        = MapWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ActivityMap") );
		m_RefreshActivityMap = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "RefreshMapActivity") );
		m_RestartServer      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnRestartServer") );
		GetVPPUIManager().HookConfirmationDialog(m_RestartServer, M_SUB_WIDGET,this,"RestartServer", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TOOLTIP_TITLE_RESTARTSERVER", "#VSTR_TOOLTIP_RESTARTSERVER");
		m_BtnLockServer 	 = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnLockServer") );
		GetVPPUIManager().HookConfirmationDialog(m_BtnLockServer, M_SUB_WIDGET,this,"LockServer", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_LOCKSERVER", "#VSTR_TOOLTIP_LOCKSERVER");
		m_BtnKickAll     	 = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnKickAll") );
		GetVPPUIManager().HookConfirmationDialog(m_BtnKickAll, M_SUB_WIDGET,this,"KickAllPlayers", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_KICKALLPLAYERS", "#VSTR_TOOLTIP_KICKALLPLAYERS");
		m_ImgLoginInfo = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgLoginInfo") );
		ToolTipHandler loginTip;
		m_ImgLoginInfo.GetScript(loginTip);
		if (loginTip)
		{
			loginTip.SetTitle("#VSTR_TOOLTIP_TITLE_ADMINLOGIN");
			loginTip.SetContentText("#VSTR_TOOLTIP_ADMINLOGIN");
		}

		m_BtnPinMonitor = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnPinMonitor") );
		m_ImgPinMonitor = ImageWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "ImgPinMonitor") );
		UpdatePinButtonVisual();

		m_ChkBoxUpdateActivity.SetChecked(true); //Auto-update activity map: on by default
		m_ToggleServerMonitor.SetChecked(true);  //Server monitor: on by default

		//Persistent feed: drives the monitor RPC for both the in-panel view and the pinned
		//overlay, and keeps running even while the hud / sub-menu are closed (see MonitorFeedUpdate).
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.MonitorFeedUpdate);

		GetRPCManager().VSendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
		m_loaded = true;
	}
		
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_RefreshActivityMap:
			GetRPCManager().VSendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
			break;

			case m_BtnAdminLogin:
			if (m_AdminloggedIn)
			{
				GetGame().ChatPlayer("#logout");
				timer = new Timer;
				timer.Run(1.0,this,"ConfirmLogin",null,false);
				m_AdminPassword = "";
			}else{
				GetGame().ChatPlayer("#login "+m_AdminPassword);
				timer = new Timer;
				timer.Run(1.0,this,"ConfirmLogin",null,false);
				m_AdminPassword = "";
			}
			break;
			
			case m_ToggleServerMonitor:
			ToggleProcessMonitor(m_ToggleServerMonitor.IsChecked() || m_MonitorPinned);
			break;

			case m_BtnPinMonitor:
			ToggleMonitorPin();
			break;
		}
		return false;
	}
	
	void LockServer(int result)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC("RPC_MissionServer", "RequestLockServer", null, true);
		}
	}
	
	void RestartServer(int result, string input)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.OK && input != "")
		{
			GetRPCManager().VSendRPC("RPC_ServerManager", "RequestRestartServer", new Param1<int>(input.ToInt()), true);
		}
	}
	
	void KickAllPlayers(int result)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().VSendRPC("RPC_ServerManager", "RequestKickAllPlayers", null, true);
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_KICKINGPLAYERS");
		}
	}
	
	override void ShowSubMenu()
	{
		super.ShowSubMenu();
		ConfirmLogin();
	}
	
	void ToggleMapWidget(bool state)
	{
		if (m_ActivityMap == null) return;
		m_ActivityMap.Show(state);
	}
	
	void ToggleProcessMonitor(bool state)
	{
		if (state)
			GetGame().ChatPlayer("#monitor 1");
		else{
			GetGame().ChatPlayer("#monitor 0");
			//Clear display data
			m_UpTimeInput.SetText("null");
			m_ServerFPSInput.SetText("null");
			m_MemoryUsedInput.SetText("null");
			m_NetworkOutInput.SetText("null");
			m_NetworkInInput.SetText("null");
			m_ActiveAIInput.SetText("null");
		}
	}

	//Writes a monitor value to the in-panel widget and (when active) the pinned overlay.
	void SetMonitorField(TextWidget menuW, TextWidget pinW, string val)
	{
		if (menuW) menuW.SetText(val);
		if (m_MonitorPinned && pinW) pinW.SetText(val);
	}

	/*
		Persistent per-frame feed (registered on the GUI update queue in OnCreate).
		Runs regardless of menu/hud visibility so the pinned overlay keeps updating.
		Requests a fresh monitor sample once per UpdateInterval while monitoring is
		"active": pinned, OR (hud open AND the monitor checkbox is checked).
	*/
	void MonitorFeedUpdate(float tDelta)
	{
		m_MonitorFeedTick += tDelta;
		if (m_MonitorFeedTick < UpdateInterval) return;
		m_MonitorFeedTick = 0.0;

		bool active = m_MonitorPinned;
		if (!active)
		{
			VPPAdminHud hud = GetToolbarMenu();
			active = (hud && hud.IsShowing() && m_ToggleServerMonitor && m_ToggleServerMonitor.IsChecked());
		}
		if (!active) return;

		GetRPCManager().VSendRPC("RPC_ServerManager", "RequestServerMonitor", NULL, true);
	}

	//Pin button: spawn or tear down the floating overlay. The in-panel monitor is left untouched.
	void ToggleMonitorPin()
	{
		if (!m_MonitorPinned)
			CreateMonitorPin();
		else
			DestroyMonitorPin();

		//Keep the server-side monitor broadcast on while pinned or the checkbox is set.
		ToggleProcessMonitor(m_ToggleServerMonitor.IsChecked() || m_MonitorPinned);
		UpdatePinButtonVisual();

		//Populate the overlay immediately on pin.
		if (m_MonitorPinned)
			GetRPCManager().VSendRPC("RPC_ServerManager", "RequestServerMonitor", NULL, true);
	}

	void CreateMonitorPin()
	{
		if (m_MonitorPinWidget) return;

		//Parented to the workspace root (null) so it survives sub-menu / hud close.
		m_MonitorPinWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ServerMonitorPin, null);
		m_MonitorPinWidget.SetSort(1000, true); //above the gameplay HUD when menus are closed

		m_PinFPS     = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinFPSInput") );
		m_PinPlayers = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinPlayersInput") );
		m_PinUpTime  = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinUpTimeInput") );
		m_PinMemory  = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinMemoryInput") );
		m_PinNetOut  = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinNetOutInput") );
		m_PinNetIn   = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinNetInInput") );
		m_PinAI      = TextWidget.Cast( m_MonitorPinWidget.FindAnyWidget("PinAIInput") );

		m_MonitorPinned = true;
	}

	void DestroyMonitorPin()
	{
		m_MonitorPinned = false;
		if (m_MonitorPinWidget)
		{
			m_MonitorPinWidget.Unlink();
			m_MonitorPinWidget = null;
		}
		m_PinFPS = null; m_PinPlayers = null; m_PinUpTime = null; m_PinMemory = null;
		m_PinNetOut = null; m_PinNetIn = null; m_PinAI = null;
	}

	void UpdatePinButtonVisual()
	{
		if (!m_ImgPinMonitor) return;
		if (m_MonitorPinned)
			m_ImgPinMonitor.SetColor( ARGB(255, 232, 163, 61) );   //accent-orange = pinned
		else
			m_ImgPinMonitor.SetColor( ARGB(255, 154, 160, 166) );  //secondary-gray = unpinned
	}
	
	void ConfirmLogin()
	{
	 	MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
		if (mission == null) return;
		
		if (mission.IsLoggedInAsAdmin()){
			m_BtnAdminLogin.SetColor( ARGBF(1.0, 0, 1.0, 0) );
			ToggleProcessMonitor(m_ToggleServerMonitor.IsChecked() || m_MonitorPinned);
			TextWidget.Cast(m_BtnAdminLogin.FindAnyWidget("LblLogin")).SetText(Widget.TranslateString("#VSTR_LBL_LOGOUT"));
			m_AdminloggedIn = true;
		}else{
			m_BtnAdminLogin.SetColor( ARGBF(1.0, 1.0, 0, 0) );
			m_AdminloggedIn = false;
			TextWidget.Cast(m_BtnAdminLogin.FindAnyWidget("LblLogin")).SetText(Widget.TranslateString("#VSTR_LBL_LOGIN"));
		}
	}

	void UpdateActivityMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref VPPPlayerData>> data;
		if( !ctx.Read( data ) ) return;
		
		array<ref VPPPlayerData> temp = data.param1;
		if( type == CallType.Client )
		{
			m_ActivityMap.ClearUserMarks();
			foreach(VPPPlayerData info : temp)
			{
				m_ActivityMap.AddUserMark(info.m_PlayerPos, info.m_PlayerName, ARGB(255,0,255,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
			}
		}
	 }
	
	void UpdateServerMonitor(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) ) return;
		
		if( type == CallType.Client )
		{
			int secs = ConvertTime(data.param1,1000);
		   	int days = secs / (24 * 3600); 
	        	secs = secs % (24 * 3600); 
	       	int hours = secs / 3600; 
	        	secs = secs % 3600;
	        	int minutes = secs / 60; 
	        	secs = secs % 60; 
			SetMonitorField(m_UpTimeInput, m_PinUpTime, string.Format("%1d %2h %3m %4s", days, hours, minutes, secs));

			MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());

			if (mission){
				TStringArray messages = new TStringArray;
				mission.GetSystemMessage().Split(",",messages);
				foreach(string msg : messages){
					msg.Replace(" ","");
					if (msg.Contains("Serverload:FPS")){
						msg.Replace("Serverload:FPS","");
						SetMonitorField(m_ServerFPSInput, m_PinFPS, msg);
					}else if (msg.Contains("memoryused:")){
						msg.Replace("memoryused:","");
						SetMonitorField(m_MemoryUsedInput, m_PinMemory, msg);
					}else if (msg.Contains("out:")){
						msg.Replace("out:","");
						SetMonitorField(m_NetworkOutInput, m_PinNetOut, msg);
					}else if (msg.Contains("in:")){
						msg.Replace("in:","");
						SetMonitorField(m_NetworkInInput, m_PinNetIn, msg);
					}else if(msg.Contains("ActivePlayers:")){
						msg.Replace("ActivePlayers:", "");
						SetMonitorField(m_PlayerCountInput, m_PinPlayers, msg);
					}else if (msg.Contains("ActiveAIs:")){
						msg.Replace("ActiveAIs:","");
						SetMonitorField(m_ActiveAIInput, m_PinAI, msg);
					}
				}
			}
		}
	}
	
	int ConvertTime(int ms, int div, bool floor = true)
	{
		if (floor){
		return Math.Floor((ms / div));
		}
		return (ms / div);
	}
	
	bool IsLoggedInAsAdmin()
	{
		return m_AdminloggedIn;
	}
};