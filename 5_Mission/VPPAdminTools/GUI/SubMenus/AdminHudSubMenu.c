class AdminHudSubMenu: ScriptedWidgetEventHandler
{
	protected bool         m_IsVisible;
	protected string       m_layoutPath;
	ref Widget   	  	   M_SUB_WIDGET;
	protected ButtonWidget m_closeButton;
	protected Widget       m_TitlePanel;
	protected Widget       m_RootWidget;
	protected float        m_posX, m_posY;
	
	void AdminHudSubMenu()
	{
	}

	void OnCreate(Widget RootW)
	{
		m_RootWidget = RootW;
	}

	void ~AdminHudSubMenu()
	{
	}

	/*
	*called when admin toolbar/hud is created and opened each time
	*/
	void OnAdminHudOpened()
	{
	}
	
	/*
		Update is always called even if window is hidden.
	*/
	void OnUpdate(float timeslice)
	{
	}
	
	void OnWidgetScriptInit(Widget w)
	{
		m_RootWidget.SetHandler(this);
	}
	
	override bool OnUpdate(Widget w)
	{
		if ( w == m_TitlePanel )
			m_TitlePanel.SetPos(0, 0, true);

		return true;
	}
	
	override bool OnDrag( Widget w, int x, int y )
	{
		if ( w == m_TitlePanel )
		{
			M_SUB_WIDGET.GetPos(m_posX, m_posY);

			m_posX = x - m_posX;
			m_posY = y - m_posY;
			m_TitlePanel.SetPos( 0, 0, true );
			m_TitlePanel.SetPos( 0, 0, false );
			
			return false;
		}
		return true;
	}

	override bool OnDragging( Widget w, int x, int y, Widget reciever )
	{
		if ( w == m_TitlePanel )
		{
			SetWindowPos(x - m_posX, y - m_posY);
			return false;
		}
		return true;
	}

	override bool OnDrop( Widget w, int x, int y, Widget reciever )
	{
		if ( w == m_TitlePanel )
		{
			SetWindowPos(x - m_posX, y - m_posY);
			return false;
		}
		return true;
	}
	
	void SetWindowPos( int x, int y )
	{
		M_SUB_WIDGET.SetPos(x, y, true);
		m_TitlePanel.SetPos(0, 0, true);
	}
	
	protected Widget CreateWidgets(string path)
	{
		return GetGame().GetWorkspace().CreateWidgets( path, m_RootWidget.FindAnyWidget("Panel_Content"));
	}
	
	bool IsSubMenuVisible()
	{
		return m_IsVisible;
	}
	
	void ShowSubMenu()
	{
		m_IsVisible = true;
		M_SUB_WIDGET.Show(true);
		M_SUB_WIDGET.Update();
		VPPAdminHud rootHud = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		rootHud.SetWindowPriorty(this);
	}
	
	void HideSubMenu()
	{
		m_IsVisible = false;
		M_SUB_WIDGET.Show(false);
		M_SUB_WIDGET.Update();
	}
	
	//TEMP: use it to hide scrollbars and map widgets when the window is not in focus. Stupid dayz bug
	void HideBrokenWidgets(bool state)
	{
		//true == hide scroll bars etc...
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_closeButton)
		{
			HideSubMenu();
			return true;
		}
		return false;
	}
	
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		VPPAdminHud rootHud = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		if (rootHud && Type() != MenuObjectManager){
			rootHud.SetWindowPriorty(this);
			return true;
		}
		return false;
	}
};