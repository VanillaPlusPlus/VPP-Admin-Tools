class MenuServerManager extends AdminHudSubMenu
{
	private Widget 				 				 m_ScriptsDropDownWidget;
	protected ref VPPDropDownMenu 	 			 m_ScriptsDropDown;
	protected ref map<string,ref array<string>>  m_CustomScripts;
	/*-Server Status Monitor-*/
	private TextWidget m_ServerFPSInput, m_PlayerCountInput, m_UpTimeInput, m_MemoryUsedInput, m_NetworkOutInput, m_NetworkInInput, m_ActiveAIInput;
	/*-----------------------*/
	
	private ref VPPTextEditor m_TextEditor;
	
	private MapWidget      m_ActivityMap;
	private ButtonWidget   m_RefreshActivityMap;
	private ButtonWidget   m_RestartServer;
	private ButtonWidget   m_BtnKickAll;
	private ButtonWidget   m_BtnViewAdmLog;
	private CheckBoxWidget m_ToggleServerMonitor;
	private CheckBoxWidget m_ChkBoxUpdateActivity;
	private EditBoxWidget  m_InputAdminPassword;
	private ButtonWidget   m_BtnAdminLogin;
	private ButtonWidget   m_BtnLoadScript;
	private ButtonWidget   m_BtnWriteScript;
	private ButtonWidget   m_BtnViewScript;
	private ButtonWidget   m_BtnLockServer;
	private ButtonWidget   m_BtnRefreshScriptList;
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
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "UpdateServerMonitor", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "UpdateActivityMap", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "SortScriptList", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_MenuServerManager", "HandleLogViewer", this, SingeplayerExecutionType.Server );
		/*-----*/
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
				GetRPCManager().SendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
			}
			
			if (m_ToggleServerMonitor.IsChecked())
			{
				GetRPCManager().SendRPC("RPC_ServerManager", "RequestServerMonitor", NULL, true);
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

		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/ServerManagerUI/MenuServerManager.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_TextEditor = new VPPTextEditor(m_RootWidget,this); //Script/text editor
		
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
		m_BtnViewAdmLog      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnViewAdmLog") );
		
		m_BtnLoadScript      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnLoadScript") );
		m_BtnWriteScript 	 = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnWriteScript") );
		m_BtnViewScript      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnViewScript") );
		m_BtnRefreshScriptList   = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnRefreshScriptList") );
		
		m_ScriptsDropDownWidget  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "ScriptsDropDown") );
		m_ScriptsDropDown  	     = new VPPDropDownMenu( m_ScriptsDropDownWidget, "Found Scripts...($profile:)" );
		m_ScriptsDropDown.m_OnSelectItem.Insert( OnSelectScript );
		
		m_CustomScripts = new map<string,ref array<string>>;
		
		GetRPCManager().SendRPC("RPC_ServerManager", "RequestScriptList", NULL, true); //Get Scripts list
		GetRPCManager().SendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
		m_loaded = true;
	}
		
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_RefreshActivityMap:
			GetRPCManager().SendRPC("RPC_ServerManager", "RequestActivityMap", NULL, true);
			break;
			
			case m_BtnViewAdmLog:
			//GetRPCManager().SendRPC("RPC_ServerManager", "RequestLogViewer", NULL, true);
			GetRPCManager().SendRPC("RPC_LogManager", "GetLogData", new Param1<string>("Log_[2019-7-21]--[13-35-25].txt"), true);
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
			ToggleProcessMonitor(m_ToggleServerMonitor.IsChecked());
			break;
			
			case m_BtnRefreshScriptList:
			GetRPCManager().SendRPC("RPC_ServerManager", "RequestScriptList", NULL, true); //Get Scripts list
			break;
			
			case m_BtnLoadScript:
			GetRPCManager().SendRPC("RPC_ServerManager", "LoadScript", new Param1<string>(m_ScriptsDropDown.GetText()), true);
			break;
			
			case m_BtnWriteScript:
			HideSubMenu(); //Hide this menu cuz MapWidget overlaps :(
			m_TextEditor.ClearText();
			m_TextEditor.ShowEditor(true);
			break;
			
			case m_BtnViewScript:
			HideSubMenu(); //Hide this menu cuz MapWidget overlaps :(
			m_TextEditor.UpdateText(m_CustomScripts.Get(m_CustomScripts.GetKey(m_ScriptsDropDown.GetIndex())));
			m_TextEditor.ShowEditor(true);
			break;
		}
		return false;
	}
	
	void LockServer(int result)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().SendRPC("RPC_MissionServer", "RequestLockServer", null, true);
		}
	}
	
	void RestartServer(int result, string input)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.OK && input != "")
		{
			GetRPCManager().SendRPC("RPC_ServerManager", "RequestRestartServer", new Param1<int>(input.ToInt()), true);
		}
	}
	
	void KickAllPlayers(int result)
	{
		ToggleMapWidget(true);
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().SendRPC("RPC_ServerManager", "RequestKickAllPlayers", null, true);
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
	
	void ConfirmLogin()
	{
	 	MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
		if (mission == null) return;
		
		if (mission.m_Chat.IsLoggedIn()){
			m_BtnAdminLogin.SetColor( ARGBF(1.0, 0, 1.0, 0) ); 
			ToggleProcessMonitor(m_ToggleServerMonitor.IsChecked());
			m_BtnAdminLogin.SetText("Logout");
			m_AdminloggedIn = true;
		}else{
			m_BtnAdminLogin.SetColor( ARGBF(1.0, 1.0, 0, 0) );
			m_AdminloggedIn = false;
			m_BtnAdminLogin.SetText("Login");
		}
	}
	
	void OnSelectScript(int index)
	{
		if (m_CustomScripts == NULL){
			m_BtnLoadScript.Enable(false); 
			m_BtnViewScript.Enable(false);
			return;
		}
		
		m_ScriptsDropDown.SetText( m_CustomScripts.GetKey(index) );
		m_ScriptsDropDown.Close();
		m_BtnLoadScript.Enable(true);
		m_BtnViewScript.Enable(true);
	}
	
	void PopulateScriptDropDown(string displayName)
	{
		m_ScriptsDropDown.AddElement(displayName);
	}
	
	void HandleLogViewer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if( !ctx.Read( data ) ) return;
		
		if( type == CallType.Client )
		{
			autoptr array<string> temp = data.param1;
			temp.Copy(data.param1);
			HideSubMenu(); //Hide this menu cuz MapWidget overlaps :(
			m_TextEditor.ClearText();
			m_TextEditor.UpdateText(temp);
			m_TextEditor.ShowEditor(true);
		}
	}
	
	void SortScriptList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		autoptr Param2<string,ref array<string>> data;
		if( !ctx.Read( data ) ) return;
		
		if( type == CallType.Client )
		{
			m_ScriptsDropDown.RemoveAllElements();
			m_CustomScripts = new map<string,ref array<string>>;
			m_CustomScripts.Insert(data.param1,data.param2);
			PopulateScriptDropDown(data.param1);
		}
	}
	
	void UpdateActivityMap(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<ref VPPPlayerData>> data;
		if( !ctx.Read( data ) ) return;
		
		autoptr array<ref VPPPlayerData> temp = data.param1;
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
			m_UpTimeInput.SetText(string.Format("%1d %2h %3m %4s", days, hours, minutes, secs));
			
			MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
			
			if (mission){
				autoptr TStringArray messages = new TStringArray;
				mission.m_Chat.GetSystemMessage().Split(",",messages);
				foreach(string msg : messages){
					msg.Replace(" ","");
					if (msg.Contains("Serverload:FPS")){
						msg.Replace("Serverload:FPS","");
						m_ServerFPSInput.SetText(msg);
					}else if (msg.Contains("memoryused:")){
						msg.Replace("memoryused:","");
						m_MemoryUsedInput.SetText(msg);
					}else if (msg.Contains("out:")){
						msg.Replace("out:","");
						m_NetworkOutInput.SetText(msg);
					}else if (msg.Contains("in:")){
						msg.Replace("in:","");
						m_NetworkInInput.SetText(msg);
					}else if(msg.Contains("ActivePlayers:")){
						msg.Replace("ActivePlayers:", "");
						m_PlayerCountInput.SetText(msg);
					}else if (msg.Contains("ActiveAIs:")){
						msg.Replace("ActiveAIs:","");
						m_ActiveAIInput.SetText(msg);
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