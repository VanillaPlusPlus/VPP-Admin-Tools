class AdminHudSubMenu: ScriptedWidgetEventHandler
{
	protected bool         m_IsVisible;
	protected string       m_layoutPath;
	ref Widget   	  	   M_SUB_WIDGET;
	protected ButtonWidget m_closeButton;
	protected Widget       m_TitlePanel;
	protected Widget       m_RootWidget;
	
	void OnCreate(Widget RootW)
	{
		m_RootWidget = RootW;
	}
	
	/*
		Update is always called even if window is hidden.
	*/
	void OnUpdate(float timeslice)
	{
		
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
	}
	
	void HideSubMenu()
	{
		m_IsVisible = false;
		M_SUB_WIDGET.Show(false);
		M_SUB_WIDGET.Update();
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
		if ( w == m_TitlePanel)
		{
			GetGame().GetDragQueue().Call( this, "UpdateWindowPosition" );
			return true;
		}
		return false;
	}
	
	void UpdateWindowPosition(int mouse_x, int mouse_y, bool is_dragging )
	{
		M_SUB_WIDGET.SetPos( mouse_x, mouse_y);
		M_SUB_WIDGET.Update();
	}
};