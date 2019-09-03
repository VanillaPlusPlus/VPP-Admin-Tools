class MapWidgetUI 
{
	private MapWidget 	 m_MapWidget;
	private ButtonWidget m_CloseButton;
	void MapWidgetUI()
	{
		
	}

	void ~MapWidgetUI()
	{

	}

	//m_MapWidget.AddUserMark(MarkerPos, MarkerName, ARGB(255,50,205,50), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
	vector ScreenToWorld()
	{
		vector world_pos,ScreenToMap,dir,from,to;
		dir = GetGame().GetPointerDirection();
	    from = GetGame().GetCurrentCameraPosition();
	    to = from + ( dir * 1000 );
		world_pos = GetGame().GetScreenPos( to );
	    ScreenToMap = m_MapWidget.ScreenToMap( world_pos );
	    return ScreenToMap;
	}

	void OnClick( Widget w, int x, int y, int button )
	{
		if (button == MouseState.LEFT){
			
		}

		if ( button == MouseState.LEFT && w == m_CloseButton){
			m_MapWidget.Unlink();
			delete this;
		}
	}
}