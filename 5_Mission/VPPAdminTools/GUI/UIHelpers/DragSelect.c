class DragSelect extends UIScriptedMenu
{
	private Widget    	m_root;
	private Widget      m_drag_widget;
	private int 		m_mouse_x;
 	private int 		m_mouse_y;
	private float 		m_size_w;
	private float       m_size_h;	
	private int 		MAX_WIDTH;
	private int 		MAX_HEIGHT;
	private  float      WIDTH_STEP  = 12.0;
	private  float      HEIGHT_STEP = 12.0;
	
	void DragSelect()
	{
		m_root  	  = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPDragSelect, NULL);
		m_drag_widget = m_root.FindAnyWidget("DragPanel");
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_root, this, "StartClick" );
		
		GetGame().GetScreenSize(MAX_WIDTH, MAX_HEIGHT);
		GetGame().GetMousePos( m_mouse_x, m_mouse_y );
		m_drag_widget.SetPos(m_mouse_x,m_mouse_y - 45);
		m_drag_widget.Update();
	}
	
	override void Update( float timeslice )
	{
		bool ctrl = g_Game.IsLeftCtrlDown();
		if (GetGame().GetMouseState(MouseState.LEFT) & 0x80000000)
		{
			int x,y;
			m_drag_widget.GetSize(m_size_w,m_size_h);
			GetGame().GetMousePos(x,y);
			
			//Width
			if (x > m_mouse_x)
			{
				m_mouse_x = x;
				if (m_size_w <= MAX_WIDTH)
					m_size_w = m_size_w + WIDTH_STEP;
			}
			
			if (x < m_mouse_x && !ctrl)
			{
				m_mouse_x = x;
				if (m_size_w >= 5.0)
					m_size_w = m_size_w - WIDTH_STEP;
			}
			//Height
			if (y > m_mouse_y)
			{
				m_mouse_y = y;
				if (m_size_h <= MAX_HEIGHT)
					m_size_h = m_size_h + HEIGHT_STEP;
			}
			
			if (y < m_mouse_y && !ctrl)
			{
				m_mouse_y = y;
				if (m_size_h >= 5.0)
					m_size_h = m_size_h - HEIGHT_STEP;
			}
			m_drag_widget.SetSize(m_size_w,m_size_h);
			m_drag_widget.Update();
		}else{
			if (!ctrl)
			ResetDrag();
		}
	}
	
	void StartClick( Widget w, int x, int y, int button )
	{
		if (button == MouseState.LEFT)
		{
			GetGame().GetMousePos( m_mouse_x, m_mouse_y );
			m_drag_widget.SetPos(m_mouse_x,m_mouse_y - 45);
			m_drag_widget.Update();
		}
	}
	
	void ResetDrag()
	{
		m_drag_widget.SetPos(0,0);
		m_drag_widget.SetSize(0,0);
		m_drag_widget.Update();
	}
};