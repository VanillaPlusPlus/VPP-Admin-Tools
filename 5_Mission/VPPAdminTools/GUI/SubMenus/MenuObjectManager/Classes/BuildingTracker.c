class BuildingTracker: ScriptedWidgetEventHandler 
{
    protected Widget           m_RootWidget;
    protected Widget       	   m_ParentWidget;
    protected TextWidget       m_ItemNameWidget;
    protected TextWidget       m_ItemDistanceWidget;
	protected GridSpacerWidget m_GridYPR;
	protected GridSpacerWidget m_GridXYZ;
	protected EditBoxWidget    m_EditYaw;
	protected EditBoxWidget    m_EditPitch;
	protected EditBoxWidget    m_EditRoll;
	protected EditBoxWidget    m_EditX;
	protected EditBoxWidget    m_EditZ;
	protected EditBoxWidget    m_EditY;
    protected string           m_ItemName;
	protected bool             m_TrackerStatus;
	Object 					   m_TrackerEntity;
	vector                     m_Orientation;
	vector                     m_Position;

    void BuildingTracker( Widget parentWidget, string itemName, Object trackedEntity, bool show) 
	{
		m_ParentWidget = parentWidget;
        m_RootWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.BuildingTracker, parentWidget);
		
        m_ItemNameWidget     = TextWidget.Cast( m_RootWidget.FindAnyWidget( "TypeInput" ) );
        m_ItemDistanceWidget = TextWidget.Cast( m_RootWidget.FindAnyWidget( "DistanceInput" ) );
		m_EditYaw			 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditYaw" ) );
		m_EditPitch			 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditPitch" ) );
		m_EditRoll			 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditRoll" ) );
		m_EditX			 	 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditX" ) );
		m_EditY			 	 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditY" ) );
		m_EditZ			 	 = EditBoxWidget.Cast( m_RootWidget.FindAnyWidget( "EditZ" ) );
		
		m_GridYPR 			 = GridSpacerWidget.Cast(m_RootWidget.FindAnyWidget("GridYPR"));
		m_GridXYZ 			 = GridSpacerWidget.Cast(m_RootWidget.FindAnyWidget("GridXZY"));
		
        m_TrackerEntity    = trackedEntity;
        m_ItemName  	   = itemName;
		if (m_TrackerEntity)
		{
			m_Orientation = m_TrackerEntity.GetOrientation();
			m_Position    = m_TrackerEntity.GetPosition();
		}
		
        m_ItemNameWidget.SetText( m_ItemName );
        m_RootWidget.Update();
        m_ItemNameWidget.Update();
        m_ItemDistanceWidget.Update();
		
		UpdateDataBoxes();
		ShowTracker(show);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.DoUpdate, 1, true);
		m_RootWidget.SetHandler(this);
    }

    void ~BuildingTracker() 
	{
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
        if (m_RootWidget != null) 
			m_RootWidget.Unlink();
    }
	
	void ForceDeleteLocalObj()
	{
		if (m_TrackerEntity != null)
		{
			//GetGame().ObjectDelete(m_TrackerEntity); Does not work locally on MP objects? some protection behind it??
			
			//Hack
			m_TrackerEntity.SetPosition("0 -5000 0"); //Burry it away
		}
	}


    float CalcDistance() 
	{
		vector startPos    = m_TrackerEntity.GetPosition();
        vector SnappedPos  = g_Game.SnapToGround( Vector(startPos[0], startPos[1], startPos[2]) );
        vector fn    	   = Vector(SnappedPos[0], SnappedPos[1], SnappedPos[2]);
		
        return vector.Distance( GetGame().GetPlayer().GetPosition(), startPos );
    }
	
	void UpdateDataBoxes(bool useObject = false)
	{
		if (useObject)
		{
			vector pos = m_TrackerEntity.GetPosition();
			vector orientation = m_TrackerEntity.GetOrientation();
			
			m_EditYaw.SetText(orientation[0].ToString());
			m_EditPitch.SetText(orientation[1].ToString());
			m_EditRoll.SetText(orientation[2].ToString());
			m_EditX.SetText(pos[0].ToString());
			m_EditZ.SetText(pos[1].ToString());
			m_EditY.SetText(pos[2].ToString());
			return;
		}
		m_EditYaw.SetText(m_Orientation[0].ToString());
		m_EditPitch.SetText(m_Orientation[1].ToString());
		m_EditRoll.SetText(m_Orientation[2].ToString());
		m_EditX.SetText(m_Position[0].ToString());
		m_EditZ.SetText(m_Position[1].ToString());
		m_EditY.SetText(m_Position[2].ToString());
	}

    void DoUpdate()
	{
		if (m_TrackerEntity == null){
			delete this;
			return;
		}
		
		vector position    = Vector(m_EditX.GetText().ToFloat(),m_EditZ.GetText().ToFloat(),m_EditY.GetText().ToFloat());
		vector orientation = Vector(m_EditYaw.GetText().ToFloat(),m_EditPitch.GetText().ToFloat(),m_EditRoll.GetText().ToFloat());
		if (position != m_Position)
		{
			//Update by Boxes
			m_TrackerEntity.SetPosition(position);
			m_Position = m_TrackerEntity.GetPosition();
			UpdateDataBoxes();
		}
		else if (m_TrackerEntity.GetPosition() != m_Position)
		{
			//Update by drag
			m_Position = m_TrackerEntity.GetPosition();
			UpdateDataBoxes();
		}
		
		EditBoxWidget underCuror = EditBoxWidget.Cast(GetWidgetUnderCursor());
		if (orientation != m_Orientation && underCuror != null && underCuror == m_EditYaw || underCuror == m_EditPitch || underCuror == m_EditRoll)
		{
			//Update yaw pitch roll
			m_TrackerEntity.SetYawPitchRoll(Vector(orientation[0],orientation[1],orientation[2]));
			m_Orientation = m_TrackerEntity.GetOrientation();
			UpdateDataBoxes();
		}
		
		if (IsCursorOnEditorBoxs()) return; //Dont update widget pos since its being used
		
        if (m_TrackerStatus && m_RootWidget != NULL && GetGame().GetPlayer()) 
		{
			vector startPos    = m_TrackerEntity.GetPosition();
	        vector SnappedPos  = g_Game.SnapToGround( Vector(startPos[0], startPos[1], startPos[2]) );
	        vector fn    	   = Vector(SnappedPos[0], SnappedPos[1] , SnappedPos[2]);
			
            vector ScreenPosRelative = GetGame().GetScreenPosRelative(startPos);
            if( ScreenPosRelative[0] >= 1 || ScreenPosRelative[0] == 0 || ScreenPosRelative[1] >= 1 || ScreenPosRelative[1] == 0 ) {
                m_RootWidget.Show( false );
                return;
            } else if( ScreenPosRelative[2] < 0 ) {
                m_RootWidget.Show( false );
                return;
            } else {
                m_RootWidget.Show( true );
            }

            float pos_x,pos_y;

            vector ScreenPos = GetGame().GetScreenPos(startPos);
            pos_x = ScreenPos[0];
            pos_y = ScreenPos[1];

            pos_x = Math.Ceil(pos_x);
            pos_y = Math.Ceil(pos_y);

            m_RootWidget.SetPos(pos_x,pos_y);
            m_ItemDistanceWidget.SetText( "[" + Math.Floor(CalcDistance()).ToString() + "m]" );
			
			//Size
			float width, height;
			m_RootWidget.GetSize(width, height);
			
			vector centerPos = vector.Zero;
				
			if (GetGame().GetPlayer()){
				centerPos = GetGame().GetPlayer().GetPosition();
			}else{
				centerPos = startPos;
			}
			
			vector adminPos = GetGame().GetPlayer().GetPosition();
			if (IsFreeCamActive())
				adminPos = VPPGetCurrentCameraPosition();
			
			float offset = vector.Distance(adminPos, centerPos);
			float distSize = centerPos[2];
			
			float current_FOV = Camera.GetCurrentFOV();
			float config_FOV = GetDayZGame().GetUserFOVFromConfig();
			float FOV_scale = current_FOV / config_FOV;
			
			width  = (offset / (distSize * 2)) * FOV_scale;
			if (width > 0.125)
				width = 0.125;
			if (width < 0.100)
				width = 0.100;
			
			height = (offset / (distSize * 2)) * FOV_scale;
			if (height > 0.065)
				height = 0.065;
			if (height < 0.043)
				height = 0.043;
	
			m_RootWidget.SetSize(width, height);
			
			m_GridYPR.Update();
			m_GridXYZ.Update();
            m_RootWidget.Update();
            m_ItemNameWidget.Update();
            m_ItemDistanceWidget.Update();
        }
		else if (m_RootWidget != NULL) 
		{
            m_RootWidget.Show(false);
        }
    }
	
	bool IsCursorOnEditorBoxs()
	{
		EditBoxWidget underCuror = EditBoxWidget.Cast(GetWidgetUnderCursor());
		if (underCuror == null)
			return false;
		
		switch(underCuror)
		{
			case m_EditYaw:
			case m_EditPitch:
			case m_EditRoll:
			case m_EditX:
			case m_EditZ:
			case m_EditY:
			return true;
			break;
			
			default: {
				return false;
			}
		}
		return false;
	}
	
	void Highlight(bool state)
	{
		if (state)
			m_RootWidget.SetColor(ARGB(200,0,0,110));
		else
			m_RootWidget.SetColor(ARGB(145,0,0,0));
	}
	
	void ShowTracker(bool state)
	{
		if (state)
		{
			m_RootWidget.Show(false);
		}else{
			m_RootWidget.Show(true);
		}
		m_TrackerStatus = state;
	}
	
	bool IsTrackerVisible()
	{
		return m_TrackerStatus;
	}

	Object GetTrackingObject()
	{
		return m_TrackerEntity;
	}
};