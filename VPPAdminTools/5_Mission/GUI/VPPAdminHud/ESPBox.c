class ESPBox extends UIScriptedMenu
{
	private float            m_LifeTime   = 5;
	private float            m_FadeLength = 0.1; //Trash
	private Widget           m_MarkerRootWidget;
	private GridSpacerWidget m_SpacerGrid;

	private TextWidget       m_NameInput;
	private TextWidget       m_HealthInput;
	private TextWidget       m_BloodInput;
	private TextWidget       m_GUIDInput;
	private TextWidget       m_GUID64Input;
	private TextWidget       m_DistanceInput;

	private TextWidget       m_GUIDtxt;
	private TextWidget       m_GUID64txt;

	private bool             m_Holding_UAUISelectPlayer = false;
	private bool             isMarkerVisible;
	private Object           m_TargetVehicle;

	private string m_Name;
	private string m_Steam64;
	private string m_GUID;
	private float m_Health;
	private float m_Blood;
	private int   m_DefaultColor;

	void ESPBox( Object TargetVehicle, string playerName, string Steam64ID, string GUID, float currentHealth, float currentBlood )
	{
		//m_MarkerRootWidget = GetGame().GetWorkspace().CreateWidgets( "gui/layouts/DZTest/3DPlayerBox.layout" );
		m_MarkerRootWidget = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/3DPlayerBox.layout" );

		m_SpacerGrid       = GridSpacerWidget.Cast( m_MarkerRootWidget.FindAnyWidget( "SpacerGrid" ) );

		m_DefaultColor     = m_SpacerGrid.GetColor();
		m_NameInput        = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "NameInput" ) );
		m_HealthInput      = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "HealthInput" ) );
		m_BloodInput       = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "BloodInput" ) );
		m_GUIDInput        = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "GUIDInput" ) );
		m_GUID64Input      = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "GUID64Input" ) );
		m_DistanceInput    = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "DistanceInput" ) );

		//Static
		m_GUIDtxt  	       = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "GUID" ) );
		m_GUID64txt		   = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "GUID64" ) );
		//----

		m_TargetVehicle    = TargetVehicle;
		m_Name             = playerName;
		m_Steam64          = Steam64ID;
		m_GUID             = GUID;
		m_Health           = currentHealth;
		m_Blood            = currentBlood;

		if (!g_Game.ESPType())
		{
			m_GUIDInput.SetText(m_GUID);
			m_GUID64Input.SetText(Steam64ID);
		}else{
			m_GUIDInput.Show(false); 
			m_GUID64Input.Show(false); 
			m_GUIDtxt.Show(false); 
			m_GUID64txt.Show(false);
		}

		m_NameInput.SetText(m_Name);
		m_HealthInput.SetText(m_Health.ToString());
		m_BloodInput.SetText(m_Blood.ToString());

		m_SpacerGrid.Update();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.DoUpdate, 1, true);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.LifeTime, 1000, true);
	}
	
	void ~ESPBox()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.doFade);
		m_MarkerRootWidget.Show( false );
	}

	float CalcDistance()
	{
		return vector.Distance( GetGame().GetPlayer().GetPosition(), m_TargetVehicle.GetPosition() );
	}

	void doFade()
	{
		if (m_MarkerRootWidget == NULL) return;
		
		float Current_alpha = m_SpacerGrid.GetAlpha();
		if (m_FadeLength >= 1)
		{
			m_SpacerGrid.SetAlpha( Current_alpha - 0.2 );
			m_FadeLength = m_FadeLength - 0.1;
		}
		else if ( m_FadeLength <= 0.900001 || Current_alpha <= 0.1)
		{
			//Destroy
			m_MarkerRootWidget.Show( false );
			delete this;
		}
	}

	void LifeTime()
	{
		if (m_LifeTime >= 1)
		{
			m_LifeTime--;
		}
		else if ( m_LifeTime <= 0 )
		{
			//Do fade
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.LifeTime);
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.doFade, 100, true);
		}
	}
	
	void DoUpdate()
	{
		if (m_MarkerRootWidget != NULL)
		{
			/*if (GetWidgetUnderCursor().GetName() == m_MarkerRootWidget.GetName())
			{
				Print("underCursor: "+GetWidgetUnderCursor().GetName());
			}*/

			UAInput inputUA = GetUApi().GetInputByName( "UAUISelectPlayer" );
			if ( inputUA.LocalHold() )
			{
				m_Holding_UAUISelectPlayer = true;
				m_SpacerGrid.SetColor( ARGBF(0.8, 1, 0, 0) );
			}else{
				m_Holding_UAUISelectPlayer = false;
				m_SpacerGrid.SetColor( m_DefaultColor );
			}

			vector TargetVehiclePosition = m_TargetVehicle.GetPosition();
			vector ScreenPosRelative = GetGame().GetScreenPosRelative(Vector(TargetVehiclePosition[0] + 0.2, TargetVehiclePosition[1] + 1.6,TargetVehiclePosition[2]));
			if( ScreenPosRelative[0] >= 1 || ScreenPosRelative[0] == 0 || ScreenPosRelative[1] >= 1 || ScreenPosRelative[1] == 0 )
			{
				m_MarkerRootWidget.Show( false );
				return;
			}
			else if( ScreenPosRelative[2] < 0 )
			{
				m_MarkerRootWidget.Show( false );
				return;
			}
			else
			{
				m_MarkerRootWidget.Show( true );
			}

			float pos_x,pos_y;

			vector ScreenPos = GetGame().GetScreenPos(Vector(TargetVehiclePosition[0] + 0.2, TargetVehiclePosition[1] + 1.6,TargetVehiclePosition[2]));
			pos_x = ScreenPos[0];
			pos_y = ScreenPos[1];

			pos_x = Math.Ceil(pos_x);
			pos_y = Math.Ceil(pos_y);

			m_MarkerRootWidget.SetPos(pos_x,pos_y);
			m_DistanceInput.SetText( Math.Floor(CalcDistance()).ToString() + "m" );

			m_SpacerGrid.Update();
			m_DistanceInput.Update();
		}
	}
}