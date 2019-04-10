class VPPAdminHud extends UIScriptedMenu
{
    private Widget 			m_InfoPanelRoot;
	private TextWidget		m_InfoLabel;
	private RichTextWidget	m_InfoText;

	//--Windows--
	protected Widget M_PANEL_WEATHER;
	protected Widget M_PANEL_SETTINGS;
	protected Widget M_PANEL_PLAYER_MANAGER;
	protected Widget M_PANEL_ITEM_MANAGER;

	//--Hud Bar--
	private ButtonWidget m_SettingBtn;
	private ButtonWidget m_PlayerManagerBtn;
	private ButtonWidget m_WeatherManagerBtn;
	private ButtonWidget m_ObjectEditorBtn;
	private ButtonWidget m_ItemManagerBtn;

	//X buttons
	private ButtonWidget m_BtnCloseWM;
	private ButtonWidget m_BtnCloseSM;
	private ButtonWidget m_BtnClosePM;
	private ButtonWidget m_BtnCloseTPM;
	private ButtonWidget m_BtnCloseIM;
	private ButtonWidget m_BtnClosePERM;
	private ButtonWidget m_BtnCloseTPMap;
	private ButtonWidget m_BtnCloseMSG;

	//-------------SubClasses-------------
	ref PlayerManagerUI  m_PlayerManagerUI;
	ref ServerSettingsUI m_ServerSettingsUI;
	ref WeatherUI        m_WeatherUI;
	ref ItemManagerUI    m_ItemManagerUI;
	//------------------------------------
	private bool         m_HudStatus;

	private bool m_IsDraggingWindow;
	private string m_FocusedWindow;
	ref map<string,string> m_DraggableWindows;
	ref map<string, ref Param2<string,string>> m_InfoBoxTxtMap;
	//protected int m_MouseX, m_MouseY, m_PosX, m_PosY;
	float m_PosX,m_PosY;
	protected bool isAdmin, isSuperAdmin;
	ref map<string,string> m_Permissions;

	void VPPAdminHud()
	{
		GetRPCManager().AddRPC( "RPC_VPPAdminHud", "VerifyAdminStatus", this, SingeplayerExecutionType.Server );

		//Ask server if this client is admin
		Param1<string> param = new Param1<string>("CopyPastaMyShitHUh?");
        GetRPCManager().SendRPC( "RPC_PermitManager", "VerifyAdminStatus", param, true);
        //-------------------------------------------
        m_Permissions = new map<string,string>;
	}

	void ~VPPAdminHud()
	{
		m_PlayerManagerUI  = NULL;
		m_ServerSettingsUI = NULL;
		m_WeatherUI        = NULL;
		m_ItemManagerUI    = NULL;

		GetGame().GetMission().GetHud().Show( true );
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.Construct);
		Print("~VPPAdminHud");
	}

	void HideAdminHud(bool trigger)
	{
		m_HudStatus = trigger;
		if (trigger)
		{
			layoutRoot.Show(false);
		}
		else
		{
			layoutRoot.Show(true);
		}
	}

	bool isHudVisible()
	{
		return m_HudStatus;
	}

	override Widget Init()
    {
    	//layoutRoot = GetGame().GetWorkspace().CreateWidgets( "gui/layouts/DZTest/VPPAdminHud.layout" );
    	layoutRoot = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/VPPAdminHud.layout" );
    	
    	m_InfoPanelRoot = Widget.Cast( layoutRoot.FindAnyWidget( "Info_Panel" ) );
		m_InfoLabel	= TextWidget.Cast( m_InfoPanelRoot.FindAnyWidget( "LabelText" ) );
		m_InfoText	= RichTextWidget.Cast( m_InfoPanelRoot.FindAnyWidget( "Info_Content" ) );

		m_SettingBtn		 = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "SettingBtn" ) );
		m_PlayerManagerBtn   = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "PlayerManagerBtn" ) );
		m_WeatherManagerBtn  = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "WeatherManagerBtn" ) );
		m_ObjectEditorBtn    = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "ObjectEditorBtn" ) );
		m_ItemManagerBtn     = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "ItemManagerBtn" ) );

		//--Windows--
		M_PANEL_WEATHER        = Widget.Cast( layoutRoot.FindAnyWidget( "Panel_Weather" ) );
		M_PANEL_SETTINGS       = Widget.Cast( layoutRoot.FindAnyWidget( "Panel_Settings" ) );
		M_PANEL_PLAYER_MANAGER = Widget.Cast( layoutRoot.FindAnyWidget( "Panel_PlayerManager" ) );
		M_PANEL_ITEM_MANAGER   = Widget.Cast( layoutRoot.FindAnyWidget( "Panel_ItemManager" ) );

		//X buttons
		m_BtnCloseWM     = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseWM" ) );
		m_BtnCloseSM     = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseSM" ) );
		m_BtnClosePM   	 = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnClosePM" ) );
		m_BtnCloseTPM 	 = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseTPM" ) );
		m_BtnCloseIM 	 = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseIM" ) );
		m_BtnClosePERM   = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnClosePERM" ) );
		m_BtnCloseTPMap  = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseTPMap" ) );
		m_BtnCloseMSG    = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "BtnCloseMSG" ) );

		FillInfoMap();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.Construct, 1200, false);
    	return layoutRoot;
    }

    void VerifyAdminStatus( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
    {
    	Param3<ref array<string>,bool,bool> data; //admin, superadmin
        if ( !ctx.Read( data ) ) return;

        ref array<string> m_PermsStrArray = new array<string>;
        m_PermsStrArray.Copy(data.param1);

        if (type == CallType.Client)
        {
        	if (data.param2 && m_PermsStrArray != NULL)
        	{
        		isAdmin = true;
        		//Sort Permissions
    		    for (int i = 0; i < m_PermsStrArray.Count(); ++i)
    		    {
    		    	string p_line = m_PermsStrArray.Get(i);
    		    	m_Permissions.Insert(p_line,"true"); //permType, boolstr
    		    }
        	}
        	else if (data.param3)
        	{
        		isSuperAdmin = true;
        	}
        }
    }

    void FillInfoMap()
    {
    	m_InfoBoxTxtMap = new map<string, ref Param2<string,string>>;
    	m_InfoBoxTxtMap.Insert("m_SettingBtn", new Param2<string,string>("Server Settings:","Here you can edit everything about Vanilla++ witout having to touch any files!") );
    	m_InfoBoxTxtMap.Insert("m_PlayerManagerBtn", new Param2<string,string>("Player Manager:","Manage all players on the server! (Kick,Kill,Heal and more!)") );
    	m_InfoBoxTxtMap.Insert("m_WeatherManagerBtn", new Param2<string,string>("Weather Settings:","Change time,date,Weather conditions") );
    	m_InfoBoxTxtMap.Insert("m_ObjectEditorBtn", new Param2<string,string>("Object Editor:","This tool allows you to place structures on the world map.(W.I.P Coming soon!)") );
    	m_InfoBoxTxtMap.Insert("m_ItemManagerBtn", new Param2<string,string>("Item Manager:","This allows you to spawn in any object in the game.") );
    }

    //----------------
    void Construct()
    {
    	m_DraggableWindows = new map<string,string>;
    	if (isAdmin) //Big ole phat tits!
        {
    	    for (int i = 0; i < m_Permissions.Count(); ++i)
    	    {
    	    	string PermType = m_Permissions.GetKey(i);
    	    	string Status   = m_Permissions.Get(PermType);

    	    	switch(PermType)
    	    	{
    	    		case "PlayerManager":
    	    		if (Status == "true")
    	    		{
    	    			m_PlayerManagerUI  = new PlayerManagerUI(this);
    	    			m_PlayerManagerUI.Init(layoutRoot,M_PANEL_PLAYER_MANAGER,false); //true if superadmin
    	    		}
    	    		break;

    	    		case "ServerManager":
    	    		if (Status == "true")
    	    		{
						m_ServerSettingsUI = new ServerSettingsUI(this);
						m_ServerSettingsUI.Init(layoutRoot,M_PANEL_SETTINGS,true);
    	    		}
    	    		break;

    	    		case "WeatherManager":
    	    		if (Status == "true")
    	    		{
						m_WeatherUI        = new WeatherUI(this);
						m_WeatherUI.Init(layoutRoot,M_PANEL_WEATHER,true);
    	    		}
    	    		break;

    	    		case "ItemManager":
    	    		if (Status == "true")
    	    		{
						m_ItemManagerUI    = new ItemManagerUI(this);
						m_ItemManagerUI.Init(layoutRoot,M_PANEL_ITEM_MANAGER,true);
    	    		}
    	    		break;
    	    	}
    	    }
        }
        else if (isSuperAdmin)
        {
        	m_PlayerManagerUI  = new PlayerManagerUI(this);
			m_ServerSettingsUI = new ServerSettingsUI(this);
			m_WeatherUI        = new WeatherUI(this);
			m_ItemManagerUI    = new ItemManagerUI(this);

			m_PlayerManagerUI.Init(layoutRoot,M_PANEL_PLAYER_MANAGER,true); //true if superadmin
			m_ServerSettingsUI.Init(layoutRoot,M_PANEL_SETTINGS,true);
			m_ItemManagerUI.Init(layoutRoot,M_PANEL_ITEM_MANAGER,true);
			m_WeatherUI.Init(layoutRoot,M_PANEL_WEATHER,true);
        }
        else
        {
        	this.Close();
			g_Game.GetUIManager().ShowCursor(false);
            g_Game.GetUIManager().ShowUICursor(false);
            GetGame().GetInput().ResetGameFocus();
			GetGame().GetMission().PlayerControlEnable();
			GetGame().GetUIManager().Back();
			GetGame().GetMission().GetHud().Show( true );
        }
    }

	bool IsDraggingWindow()
	{
		GetGame().GetDragQueue().Call( this, "UpdateWindowPosition" );
		return true;
	}

	void UpdateWindowPosition(int mouse_x, int mouse_y, bool is_dragging )
	{
		if (!is_dragging)
		{
			layoutRoot.FindAnyWidget(m_FocusedWindow).SetPos( m_PosX, m_PosY);
		}
		else
		{
			layoutRoot.FindAnyWidget(m_FocusedWindow).GetPos( m_PosX, m_PosY);
		}
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if ( m_ItemManagerUI != NULL && m_ItemManagerUI.OnMouseButtonDown(w,x,y,button))
		{
			return true;
		}
		return false;
	}

	override bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
	{
		if ( m_ItemManagerUI != NULL && m_ItemManagerUI.OnMouseWheel(w,x,y,wheel))
		{
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if ( m_PlayerManagerUI != NULL && m_PlayerManagerUI.OnChange(w,x,y,finished))
		{
			return true;
		}
		else if ( m_ItemManagerUI != NULL && m_ItemManagerUI.OnChange(w,x,y,finished))
		{
			return true;
		}
		else if ( m_ServerSettingsUI != NULL && m_ServerSettingsUI.OnChange(w,x,y,finished))
		{
			return true;
		}
		return false;
	}

    override bool OnClick( Widget w, int x, int y, int button )
	{
		protected Widget oDaddy;
		if ( m_PlayerManagerUI != NULL && m_PlayerManagerUI.OnClick(w,x,y,button))
		{
			return true;
		}
		else if (m_ServerSettingsUI != NULL && m_ServerSettingsUI.OnClick(w,x,y,button))
		{
			return true;
		}
		else if ( m_ItemManagerUI != NULL && m_ItemManagerUI.OnClick(w,x,y,button))
		{
			return true;
		}
		else if (m_WeatherUI != NULL && m_WeatherUI.OnClick(w,x,y,button))
		{
			return true;
		}
		
		switch(w)
		{
			case m_SettingBtn:
			if (M_PANEL_SETTINGS.IsVisible() && m_ServerSettingsUI != NULL)
			{
				M_PANEL_SETTINGS.Show(false);
			}
			else if (m_ServerSettingsUI != NULL)
			{
				M_PANEL_SETTINGS.Show(true);
			}
			return true;
			break;

			case m_PlayerManagerBtn:
			if (M_PANEL_PLAYER_MANAGER.IsVisible() && m_PlayerManagerUI != NULL)
			{
				M_PANEL_PLAYER_MANAGER.Show(false);
			}
			else if (m_PlayerManagerUI != NULL)
			{
				M_PANEL_PLAYER_MANAGER.Show(true);
			}
			return true;
			break;

			case m_WeatherManagerBtn:
			if (M_PANEL_WEATHER.IsVisible() && m_WeatherUI != NULL)
			{
				M_PANEL_WEATHER.Show(false);
			}
			else if (m_WeatherUI != NULL)
			{
				M_PANEL_WEATHER.Show(true);
			}
			return true;
			break;

			case m_ObjectEditorBtn:

			return true;
			break;

			case m_ItemManagerBtn:
			if (M_PANEL_ITEM_MANAGER.IsVisible() && m_ItemManagerUI != NULL)
			{
				M_PANEL_ITEM_MANAGER.Show(false);
			}
			else if (m_ItemManagerUI != NULL)
			{
				M_PANEL_ITEM_MANAGER.Show(true);
			}
			return true;
			break;

			//X Buttons
			case m_BtnCloseWM:
			if (M_PANEL_WEATHER.IsVisible() && m_WeatherUI != NULL)
			{
				M_PANEL_WEATHER.Show(false);
				return true;
			}
			break;

			case m_BtnCloseSM:
			if (M_PANEL_SETTINGS.IsVisible() && m_ServerSettingsUI != NULL)
			{
				M_PANEL_SETTINGS.Show(false);
				return true;
			}
			break;

			case m_BtnClosePM:
			if (M_PANEL_PLAYER_MANAGER.IsVisible() && m_PlayerManagerUI != NULL)
			{
				M_PANEL_PLAYER_MANAGER.Show(false);
				return true;
			}
			break;

			case m_BtnCloseTPM:
			if (m_PlayerManagerUI != NULL)
			{
				oDaddy = m_BtnCloseTPM.GetParent();
				if (oDaddy != NULL) { oDaddy.Show(false); return true; }
			}
			break;

			case m_BtnCloseIM:
			if (M_PANEL_ITEM_MANAGER.IsVisible() && m_ItemManagerUI != NULL)
			{
				M_PANEL_ITEM_MANAGER.Show(false);
				return true;
			}
			break;

			case m_BtnClosePERM:
			if (m_PlayerManagerUI != NULL)
			{
				oDaddy = m_BtnClosePERM.GetParent();
				if (oDaddy != NULL) { oDaddy.Show(false); return true;}
			}
			break;

			case m_BtnCloseTPMap:
			oDaddy = m_BtnCloseTPMap.GetParent();
			if (oDaddy != NULL) { oDaddy.Show(false); return true; }
			break;

			case m_BtnCloseMSG:
			if (m_PlayerManagerUI != NULL)
			{
				oDaddy = m_BtnCloseMSG.GetParent();
				if (oDaddy != NULL) { oDaddy.Show(false); return true; }
			}
			break;
		}

		return false;
	}

    override bool OnMouseEnter( Widget w, int x, int y )
	{
		if (w != NULL && m_DraggableWindows != NULL && m_DraggableWindows.Contains( w.GetName() ) && !m_IsDraggingWindow)
		{
			m_FocusedWindow = w.GetName();
		}

		Param2<string,string> Info;
		switch(w)
		{
			case m_SettingBtn:
			Info = m_InfoBoxTxtMap.Get("m_SettingBtn");
			if (Info)
			{
				m_InfoPanelRoot.Show(true);
				m_InfoLabel.SetText( Info.param1 );
				m_InfoText.SetText( Info.param2 );

				m_InfoPanelRoot.Update();
				m_InfoLabel.Update();
				m_InfoText.Update();
				return true;
			}
			break;

			case m_PlayerManagerBtn:
			Info = m_InfoBoxTxtMap.Get("m_PlayerManagerBtn");
			if (Info)
			{
				m_InfoPanelRoot.Show(true);
				m_InfoLabel.SetText( Info.param1 );
				m_InfoText.SetText( Info.param2 );

				m_InfoPanelRoot.Update();
				m_InfoLabel.Update();
				m_InfoText.Update();
				return true;
			}
			break;

			case m_WeatherManagerBtn:
			Info = m_InfoBoxTxtMap.Get("m_WeatherManagerBtn");
			if (Info)
			{
				m_InfoPanelRoot.Show(true);
				m_InfoLabel.SetText( Info.param1 );
				m_InfoText.SetText( Info.param2 );

				m_InfoPanelRoot.Update();
				m_InfoLabel.Update();
				m_InfoText.Update();
				return true;
			}
			return true;
			break;

			case m_ObjectEditorBtn:
			Info = m_InfoBoxTxtMap.Get("m_ObjectEditorBtn");
			if (Info)
			{
				m_InfoPanelRoot.Show(true);
				m_InfoLabel.SetText( Info.param1 );
				m_InfoText.SetText( Info.param2 );

				m_InfoPanelRoot.Update();
				m_InfoLabel.Update();
				m_InfoText.Update();
				return true;
			}
			return true;
			break;

			case m_ItemManagerBtn:
			Info = m_InfoBoxTxtMap.Get("m_ItemManagerBtn");
			if (Info)
			{
				m_InfoPanelRoot.Show(true);
				m_InfoLabel.SetText( Info.param1 );
				m_InfoText.SetText( Info.param2 );

				m_InfoPanelRoot.Update();
				m_InfoLabel.Update();
				m_InfoText.Update();
				return true;
			}
			return true;
			break;
		}
		return false;
	}
	
	override bool OnMouseLeave( Widget w, Widget enterW, int x, int y )
	{
		if (w == m_SettingBtn || w == m_PlayerManagerBtn || w == m_WeatherManagerBtn || w == m_ObjectEditorBtn || w == m_ItemManagerBtn)
		{
			m_InfoPanelRoot.Show(false);
			return true;
		}
		return false;
	}

	/*override bool OnKeyPress( Widget w, int x, int y, int key )
	{
		super.OnKeyPress(w,x,y,key);
		if ( m_PlayerManagerUI != NULL && m_PlayerManagerUI.OnKeyPress(w,x,y,key))
		{
			return true;
		}
		return true;
	}
	*/
	//----------------
}