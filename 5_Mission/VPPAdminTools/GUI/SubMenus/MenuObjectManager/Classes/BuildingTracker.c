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
	protected ButtonWidget	   m_BtnSelect;
    protected string           m_ItemName;
	protected bool             m_TrackerStatus;
	bool 					   m_IsExpanded;
	Object 					   m_TrackerEntity;
	vector                     m_Orientation;
	vector                     m_Position;
	bool   					   m_IsSelected;
	MenuObjectManager 		   objManager; //weak ptr
	ref ScriptInvoker 		   OnTrackerSelected = new ScriptInvoker();

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
		
		m_BtnSelect			 = ButtonWidget.Cast(m_RootWidget.FindAnyWidget("BtnSelect"));

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
		m_RootWidget.SetUserData(this);
		VPPUIManager.WIDGET_PTRs.Insert(m_RootWidget); //For drag-selection

		objManager = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
    }

    void ~BuildingTracker() 
	{
		VPPUIManager.WIDGET_PTRs.RemoveItem(m_RootWidget); //For drag-selection
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
        if (m_RootWidget != null) 
			m_RootWidget.Unlink();

		OnTrackerSelected.Clear();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
    	if (w == m_BtnSelect)
    	{
    		if (!objManager)
    			return false;

    		if (IsSelected())
    		{
    			Highlight(false);
    			if (objManager.GetSelectedParent() == this)
    			{
    				objManager.SetSelectedParent(NULL);
    			}
    		}
    		else
    		{
    			objManager.SetSelectedObject(m_TrackerEntity);
    		}
    		return true;
    	}
    	return super.OnClick(w, x, y, button);
    }
	
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnSelect)
		{
			//Expand/Hide
			ExpandDetails(!m_IsExpanded);
			return true;
		}
		return super.OnDoubleClick(w, x, y, button);
	}

	bool shouldHideCalled = false;

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if ((w == m_RootWidget || w == m_GridYPR || w == m_GridXYZ) && enterW && enterW.GetName() == "rootFrame")
		{
			if (!shouldHideCalled && m_IsExpanded)
			{
				int _x, _y;
	        	GetMousePos(_x, _y);
				thread _ShouldHideDetails(_x, _y);
				shouldHideCalled = true;
				return true;
			}
		}
		return super.OnMouseLeave(w, enterW, x, y);
	}

	//Acts as a "bounds" checks if mouse moved far away enough from widget, if too far hide it :)
	private void _ShouldHideDetails(int x, int y)
	{
		int current_x, current_y;
		bool complete;
		while (!complete)
		{
			GetMousePos(current_x, current_y);
			if (Math.AbsInt(x - current_x) > 65 || Math.AbsInt(y - current_y) > 65) 
            {
	            map<Widget, int> validUnderMouse = new map<Widget, int>();
	            DumpWidget(m_RootWidget, validUnderMouse);

	            //Still inside
				if (!validUnderMouse.Contains(GetWidgetUnderCursor()))
				{
					ExpandDetails(false); //hide details popup
					shouldHideCalled = false;
					complete = true;
					break;
				}
            }
            Sleep(10);
		}
	}

	void DumpWidget(Widget w, inout map<Widget, int> widgets)
	{
		if ( !w )
			return;
	
		widgets.Insert(w, 0);
		DumpWidget(w.GetChildren(), widgets);		
		DumpWidget(w.GetSibling(), widgets);
	}

	//Callback for when give widget (WIDGET_PTRs) is selected
	void OnWidgetDragSelect(bool state)
	{
		if (!IsTrackerVisible())
			return;
		if (IsSelected() && state)
			return;
		if (!objManager)
			return;

		if (state)
		{
			objManager.SetSelectedObject(m_TrackerEntity, false, true); //select
		}

		//deselect
		if (!state && IsSelected())
		{
			Highlight(false);
			if (objManager.GetSelectedParent() == this)
			{
				objManager.SetSelectedParent(NULL);
			}
		}
	}

	void ExpandDetails(bool expand)
	{
		m_IsExpanded = expand;
		m_GridYPR.Show(expand);
		m_GridXYZ.Show(expand);
		m_ItemNameWidget.Show(expand);
		m_ItemDistanceWidget.Show(expand);
		m_BtnSelect.Show(!expand);
	}

	void ForceDeleteLocalObj()
	{
		if (m_TrackerEntity != null)
			m_TrackerEntity.SetPosition("0 -5000 0"); //Burry it away
	}

    float CalcDistance() 
	{
		vector startPos    = m_TrackerEntity.GetPosition();
        vector SnappedPos  = g_Game.SnapToGround( Vector(startPos[0], startPos[1], startPos[2]) );
        vector fn    	   = Vector(SnappedPos[0], SnappedPos[1], SnappedPos[2]);
		
		vector adminPos = GetGame().GetPlayer().GetPosition();
		if (IsFreeCamActive())
			adminPos = VPPGetCurrentCameraPosition();

        return vector.Distance(adminPos, startPos);
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
		
		vector position    = Vector(m_EditX.GetText().ToFloat(), m_EditZ.GetText().ToFloat(), m_EditY.GetText().ToFloat());
		vector orientation = Vector(m_EditYaw.GetText().ToFloat(), m_EditPitch.GetText().ToFloat(), m_EditRoll.GetText().ToFloat());
		if (position != m_Position)
		{
			//Update by Boxes
			m_TrackerEntity.SetPosition(position);
			m_Position = m_TrackerEntity.GetPosition();
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
		SetSelected(state);
		if (state)
		{
			m_RootWidget.SetColor(ARGB(200,0,0,110));
			m_BtnSelect.SetColor(ARGB(255,94,94,248));
		}
		else
		{
			m_RootWidget.SetColor(ARGB(145,0,0,0));
			m_BtnSelect.SetColor(ARGB(255,230,230,243));
		}
	}

	bool IsSelected()
	{
		return (m_IsSelected && GetTrackingObject() != NULL);
	}

	void SetSelected(bool state)
	{
		m_IsSelected = state;
		OnTrackerSelected.Invoke(state);
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