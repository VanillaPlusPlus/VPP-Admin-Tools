class ItemScanResultScreen: ScriptedWidgetEventHandler
{
    protected Widget    	m_EntryWidget;
    protected TextWidget    m_SomeText;
    protected ButtonWidget  m_BtnClose;
    protected MapWidget     m_map;
	
    void ItemScanResultScreen()
    {
		m_EntryWidget = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/XMLEditorUI/itemscanresultscreen.layout", null);
		m_SomeText      = TextWidget.Cast(m_EntryWidget.FindAnyWidget("SomeText"));
		m_BtnClose      = ButtonWidget.Cast(m_EntryWidget.FindAnyWidget("BtnClose"));
		m_map      		= MapWidget.Cast(m_EntryWidget.FindAnyWidget("map"));
		m_EntryWidget.SetHandler(this);
		m_EntryWidget.SetSort(110,true);
		m_map.SetSort(115,true);
    }
	
	void ~ItemScanResultScreen()
	{
		if (m_EntryWidget != null)
			m_EntryWidget.Unlink();
	}
	
	void DrawMarker(string name, vector pos)
	{
		m_map.AddUserMark(pos, name, ARGB(255,0,215,0), "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa");
	}
	
	void ClearMarkers()
	{
		m_map.ClearUserMarks();
	}
	
	void SetResultText(string txt)
	{
		m_SomeText.SetText(txt);
	}
	
	vector GetPointerWorldPos()
	{
		vector world_pos,ScreenToMap,dir,from,to;
		dir  = GetGame().GetPointerDirection();
	    from = GetGame().GetCurrentCameraPosition();
	    to = from + ( dir * 1000 );
		world_pos = GetGame().GetScreenPos( to );
	    ScreenToMap = m_map.ScreenToMap( world_pos );
	    return ScreenToMap;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnClose)
		{
			delete this;
			return true;
		}
		return false;
	}
	
	override bool OnKeyPress(Widget w, int x, int y, int key)
	{
		if (key == KeyCode.KC_ESCAPE)
		{
			delete this;
			return true;
		}
		return false;
	}
};