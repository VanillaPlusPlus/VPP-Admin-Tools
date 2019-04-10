class CustomMapWidget extends UIScriptedMenu
{
	private Widget               m_Root;
	private ButtonWidget         m_CloseButton;
	private ButtonWidget         m_BtnApply;
	private ButtonWidget 		 m_BtnClearAll;
	private MapWidget            m_MapWidget;
	private EditBoxWidget        m_InputNewPos;
	private TextWidget           m_TxtXMLStatus;
	private ImageWidget          m_ImageHelp;
	private ref ServerSettingsUI m_RootClass;
	private string 				 m_WidgetType;

	private ref map<string, vector> m_Markers;

	void CustomMapWidget(string nameType,ref ServerSettingsUI RootClass, Widget RootWidget, ref map<string, vector> Markers)
	{
		m_WidgetType     = nameType;
		m_RootClass      = RootClass;
		//m_Root           = GetGame().GetWorkspace().CreateWidgets( "gui/layouts/DZTest/CustomMapWidget.layout", NULL );
		m_Root           = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/CustomMapWidget.layout", NULL );
		m_CloseButton    = ButtonWidget.Cast( m_Root.FindAnyWidget( "BackBtn1" ) );
		m_BtnApply       = ButtonWidget.Cast( m_Root.FindAnyWidget( "BtnApply" ) );
		m_BtnClearAll	 = ButtonWidget.Cast( m_Root.FindAnyWidget( "BtnClearAll" ) );
		m_MapWidget      = MapWidget.Cast( m_Root.FindAnyWidget( "LeMapWidget" ) );
		m_InputNewPos    = EditBoxWidget.Cast( m_Root.FindAnyWidget( "InputNewPos" ) );
		m_TxtXMLStatus   = TextWidget.Cast( m_Root.FindAnyWidget( "TxtXMLStatus" ) );
		m_ImageHelp		 = ImageWidget.Cast( m_Root.FindAnyWidget( "ImageHelp" ) );

		m_Markers         = Markers;
		DrawMarkers();

		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_CloseButton, this, "onClick" );
		
		if (m_WidgetType == "DisplayPosition"){
			m_InputNewPos.Show(false);
			m_BtnApply.Show(false);
			m_BtnClearAll.Show(false);
			m_TxtXMLStatus.SetText("Total Scanned Items: "+Markers.Count().ToString());
			m_ImageHelp.Show(false);
		}else{
			ref InfoBoxInstance m_InfoBoxInstance = new InfoBoxInstance(m_Root,"ImageHelp","Information:","DOUBLE-RIGHT Click on a point you wish to remove.\nDOUBLE-LEFT Click anywhere on map to add a location.\nClick Apply to save changes.\n  ");
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_BtnClearAll, this, "onClick" );
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_BtnApply, this, "onClick" );
			WidgetEventHandler.GetInstance().RegisterOnDoubleClick( m_MapWidget, this, "MapDoubleClick" );
		}
		
		g_Game.GetUIManager().ShowUICursor(true);
	}

	void ~CustomMapWidget()
	{

	}

	void DrawMarkers()
	{
		if (m_Markers != NULL)
		{
			for (int i = 0; i < m_Markers.Count(); ++i)
		    {
		    	string MarkerName = m_Markers.GetKey(i);
		    	vector MarkerPos  = m_Markers.Get(MarkerName);

		    	m_MapWidget.AddUserMark(MarkerPos, MarkerName, ARGB(255,50,205,50), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypointeditor_CA.paa");
		    }
		}
	}

	void MapDoubleClick(Widget w, int x, int y, int button)
	{
		if (w == m_MapWidget)
		{
    		//2x Right click to remove
			if (button == MouseState.RIGHT)
			{
				CheckDistance(ScreenToWorld());
			}
			else
			{
				//Add new point
	    		if (m_Markers.Contains(m_InputNewPos.GetText()) && m_InputNewPos.GetText() == "") return;

	    		vector world_pos = ScreenToWorld();
	    		m_Markers.Insert(m_InputNewPos.GetText(),world_pos);
			}
    		//Reload markers
	    	m_MapWidget.ClearUserMarks();
			DrawMarkers();
		}
	}

	void CheckDistance(vector posToCheck)
	{
	    for (int i = 0; i < m_Markers.Count(); ++i)
	    {
	    	string LocationName = m_Markers.GetKey(i);
	    	vector LocationPos  = m_Markers.Get(LocationName);

	    	float Distance = vector.Distance( posToCheck, LocationPos);
	    	if (Distance <= 85)
	    	{
	    		UpdatePositions(LocationPos);
	    	}
	    }
	}

	void UpdatePositions(vector selectedPos)
	{
		for (int i = 0; i < m_Markers.Count(); ++i)
	    {
	    	string PositionName   = m_Markers.GetKey(i);
	    	vector PositionVector = m_Markers.Get(PositionName);
	    	if (selectedPos == PositionVector)
	    	{
	    		//Remove selected pos
	    		m_Markers.Remove(PositionName);

	    		//Reload markers
		    	m_MapWidget.ClearUserMarks();
				DrawMarkers();
	    	}
	    }
	}

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

	void onClick( Widget w, int x, int y, int button )
	{
		if (button == MouseState.LEFT && w == m_BtnClearAll)
		{
			m_Markers = new map<string, vector>;
			m_MapWidget.ClearUserMarks();
			DrawMarkers();
		}

		if ( button == MouseState.LEFT && w == m_CloseButton)
		{
			m_Root.Unlink();
			delete this;
		}

		if (button == MouseState.LEFT && w == m_BtnApply)
		{
			switch(m_WidgetType)
			{
				case "InfectedHordes":
				m_RootClass.SaveInfectedHordesPositions(m_Markers);
				break;

				case "SpawnPointsEditor":
				m_RootClass.UpdateSpawnPointsPositions(m_Markers);
				break;
			}
			m_Root.Unlink();
			delete this;
		}
	}
}