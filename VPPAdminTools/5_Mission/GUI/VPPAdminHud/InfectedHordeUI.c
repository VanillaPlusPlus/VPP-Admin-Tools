class InfectedHordeUI extends UIScriptedMenu
{
	private ref ServerSettingsUI m_RootClass;
	private Widget               m_RootWidget;
	private Widget               m_InfectedHordePanel;

	private TextListboxWidget    m_PositionsList;
	private ButtonWidget         m_BtnApplyIH;
	private ButtonWidget         m_BtnShowOnMap;
	private TextWidget           m_TxtIHStatus;
	private EditBoxWidget        m_InputMaxZeds;
	private EditBoxWidget        m_InputMinZombies;
	private EditBoxWidget        m_InputLifeTimeIH;
	private EditBoxWidget        m_InputPauseTime;
	private EditBoxWidget        m_InputMaxHordes;
	private CheckBoxWidget       m_ChkShowMessage;
	private CheckBoxWidget       m_ChkSpecialZeds;

	private int INT_MAX_ZOMBIES;
	private int INT_MIN_ZOMBIES;
	private int INT_INFECTEDHORDE_TIME;
	private int INT_INFECTEDHORDE_DESPAWNTIME;
	private int MAX_HORDES;
	private bool SENDBROADCASTMESSAGE;
	private bool CANSPAWNSPECIALINFECTED;
	private ref map<string, vector> m_HordePositions;

	void InfectedHordeUI(ref ServerSettingsUI RootClass, Widget RootWidget, Widget InfectedHordePanel)
	{
		/*=====RPC's====*/
		GetRPCManager().AddRPC("RPC_InfectHordeClient", "handleInfectedJson", this, SingeplayerExecutionType.Server);
		//--------------------
		m_RootWidget         = RootWidget;
		m_RootClass          = RootClass;
		m_InfectedHordePanel = InfectedHordePanel;

		m_BtnShowOnMap    = ButtonWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "BtnShowOnMap" ) );
		m_BtnApplyIH      = ButtonWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "BtnApplyIH" ) );
		m_TxtIHStatus     = TextWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "TxtIHStatus" ) );
		m_PositionsList   = TextListboxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "ListPosIH" ) );
		m_InputMaxZeds    = EditBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "InputMaxZeds" ) );
		m_InputMinZombies = EditBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "InputMinZombies" ) );
		m_InputLifeTimeIH = EditBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "InputLifeTimeIH" ) );
		m_InputPauseTime  = EditBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "InputPauseTime" ) );
		m_InputMaxHordes  = EditBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "InputMaxHordes" ) );
		m_ChkShowMessage  = CheckBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "ChkShowMessage" ) );
		m_ChkSpecialZeds  = CheckBoxWidget.Cast( m_InfectedHordePanel.FindAnyWidget( "ChkSpecialZeds" ) );

		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_BtnApplyIH, this, "onClick" );
		WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_BtnShowOnMap, this, "onClick" );
		GetRPCManager().SendRPC("RPC_InfectHordeServer", "getInfectedJson", NULL, true);
	}

	void onClick( Widget w, int x, int y, int button )
	{
		if ( button != MouseState.LEFT) return;

		switch(w)
		{
			case m_BtnApplyIH:
			saveInfectedJson();
			break;

			case m_BtnShowOnMap:
			ref CustomMapWidget m_CustomMapWidget = new CustomMapWidget("InfectedHordes",m_RootClass,m_RootWidget,m_HordePositions);
			break;
		}
	}

	void handleInfectedJson(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		ref VPPParam8<int, int, int, int, int, bool, bool, ref map<string, vector>> data;
		if(!ctx.Read(data)) return;

		if(type == CallType.Client)
		{
			INT_MAX_ZOMBIES 			  = data.param1;
			INT_MIN_ZOMBIES               = data.param2;
			INT_INFECTEDHORDE_TIME        = data.param4;
			INT_INFECTEDHORDE_DESPAWNTIME = data.param3;
			MAX_HORDES					  = data.param5;
			SENDBROADCASTMESSAGE		  = data.param6;
			CANSPAWNSPECIALINFECTED		  = data.param7;

			m_InputMaxZeds.SetText(INT_MAX_ZOMBIES.ToString());
			m_InputMinZombies.SetText(INT_MIN_ZOMBIES.ToString());
			m_InputLifeTimeIH.SetText(INT_INFECTEDHORDE_TIME.ToString());
			m_InputPauseTime.SetText(INT_INFECTEDHORDE_DESPAWNTIME.ToString());
			m_InputMaxHordes.SetText(MAX_HORDES.ToString());
			m_ChkShowMessage.SetChecked(SENDBROADCASTMESSAGE);
			m_ChkSpecialZeds.SetChecked(CANSPAWNSPECIALINFECTED);

			m_HordePositions = new map<string, vector>;
			m_HordePositions.Copy(data.param8);
		    for (int i = 0; i < m_HordePositions.Count(); ++i)
		    {
		    	string town    = m_HordePositions.GetKey(i);
		    	vector postown = m_HordePositions.Get(town);

		    	string StrPos  = g_Game.VectorToString(postown);
		    	m_PositionsList.AddItem(town, NULL, 0 );	
				m_PositionsList.SetItem( i, StrPos, NULL, 1 );
		    }
		}
	}

	void UpdatePositions(ref map<string, vector> newPositions)
	{
		m_HordePositions = newPositions;
		saveInfectedJson();

		//Update the Textlist
		m_PositionsList.ClearItems();
		for (int i = 0; i < m_HordePositions.Count(); ++i)
	    {
	    	string town    = m_HordePositions.GetKey(i);
	    	vector postown = m_HordePositions.Get(town);

	    	string StrPos  = g_Game.VectorToString(postown);
	    	m_PositionsList.AddItem(town, NULL, 0 );	
			m_PositionsList.SetItem( i, StrPos, NULL, 1 );
	    }
	}

	void saveInfectedJson()
	{
		ref VPPParam8<int, int, int, int, int, bool, bool, ref map<string, vector>> jsonData = new VPPParam8<int,int,int,int,int,bool,bool, ref map<string, vector>>(
		m_InputMaxZeds.GetText().ToInt(),
		m_InputMinZombies.GetText().ToInt(),
		m_InputPauseTime.GetText().ToInt(),
		m_InputLifeTimeIH.GetText().ToInt(),
		m_InputMaxHordes.GetText().ToInt(),
		m_ChkShowMessage.IsChecked(),
		m_ChkSpecialZeds.IsChecked(),
		m_HordePositions);
		GetRPCManager().SendRPC("RPC_InfectHordeServer", "saveInfectedJson", jsonData, true);
		m_TxtIHStatus.SetText("Successfuly Applied Changes!");
	}
}