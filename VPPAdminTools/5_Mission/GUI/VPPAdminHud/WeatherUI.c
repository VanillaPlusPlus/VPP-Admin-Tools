class WeatherUI extends UIScriptedMenu
{
	protected VPPAdminHud m_MainFrameClass;

	protected Widget M_MAIN_ROOT;
	protected Widget M_SUB_ROOT;
	protected Widget M_PANEL_ITEM_MANAGER;
	private ref array<ref InfoBoxInstance>  m_InfoBoxClass;

	protected SliderWidget m_SliderTime;
	protected SliderWidget m_SliderDate;
	protected SliderWidget m_SliderOvercast;
	protected SliderWidget m_SliderWindForce;
	protected SliderWidget m_SliderFog;

	private int      m_Year;
    private int      m_Month;
    private int      m_Day;
    private int      m_Hour;
    private int      m_Minute;
    private float    m_Overcast;
    private float    m_Rain;
    private float    m_Fog;
    private float    m_WindForce;

    private int      m_CurrentYear;
    private int      m_CurrentMonth;
    private int      m_CurrentDay;
    private int      m_CurrentHour;
    private int      m_CurrentMinute;
    private float    m_CurrentOvercast;
    private float    m_CurrentRain;
    private float    m_CurrentFog;
    private float    m_CurrentWindForce;

	void WeatherUI(  VPPAdminHud MainFrameClass  )
	{
		m_MainFrameClass = MainFrameClass;

		GetGame().GetWorld().GetDate( m_Year, m_Month, m_Day, m_Hour, m_Minute );

        Weather weather = GetGame().GetWeather();
        m_Overcast  = weather.GetOvercast().GetActual();
        m_Rain      = weather.GetRain().GetActual();
        m_Fog       = weather.GetFog().GetActual();
        m_WindForce = weather.GetWindSpeed();
	}

	void ~WeatherUI()
	{
		m_MainFrameClass = NULL;
	}
    
	void Init(Widget RootLayout, Widget SubRootLayout, bool HasPerm)
	{
		if (HasPerm)
		{
			M_MAIN_ROOT  = RootLayout;
			M_SUB_ROOT   = SubRootLayout; //Panel_Weather

			m_MainFrameClass.m_DraggableWindows.Insert("Panel_Weather","true");
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( M_SUB_ROOT,  m_MainFrameClass, "IsDraggingWindow" );

			m_SliderTime      = SliderWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SliderTime" ) );
			m_SliderDate      = SliderWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SliderDate" ) );
			m_SliderOvercast  = SliderWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SliderOvercast" ) );
			m_SliderWindForce = SliderWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SliderWindForce" ) );
			m_SliderFog		  = SliderWidget.Cast( M_SUB_ROOT.FindAnyWidget( "SliderFog" ) );

			m_InfoBoxClass = new array<ref InfoBoxInstance>;
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImgTime","Time:","...") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImgDate","Date:","...") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImgOvercast","Overcast:","...") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImgWind","Wind:","...") );
		    m_InfoBoxClass.Insert( new InfoBoxInstance(RootLayout,"ImgFog","Fog:","...") );
		}
	}

	void InitSliders()
    {
        
    }

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
	override bool OnClick( Widget w, int x, int y, int button )
	{
		return false;
	}

	override bool OnChange( Widget w, int x, int y, bool finished )
    {
    	return false;
    }
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
}