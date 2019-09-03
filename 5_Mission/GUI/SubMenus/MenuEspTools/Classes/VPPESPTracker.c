class VPPESPTracker: ScriptedWidgetEventHandler 
{
    protected Widget           m_RootWidget;
    protected ref Widget       m_ParentWidget;
    protected GridSpacerWidget m_SpacerGrid;
	
    protected CheckBoxWidget   m_CheckBox;
    protected TextWidget       m_ItemNameWidget;
    protected TextWidget       m_ItemDistanceWidget;
	protected SliderWidget     m_HealthInput;
	protected SliderWidget     m_BloodInput;
	protected TextWidget       m_GUIDInput;
	protected TextWidget       m_GUID64Input;

    protected string           m_ItemName;
    protected vector	       m_MarkerPositon;
    protected bool             m_IsMarkerVisible;
	private ref PlayerStatsData m_playerInfo;
	Object 					   m_TrackerEntity;

    void VPPESPTracker( Widget parentWidget, string itemName, Object trackedEntity, bool detailed, bool visible = true ) 
	{
		GetRPCManager().AddRPC( "RPC_VPPESPPlayerTracker", "HandleData", this, SingeplayerExecutionType.Server );
	
		m_ParentWidget = parentWidget;
        m_RootWidget = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTracker.layout", parentWidget);
		m_RootWidget.SetHandler(this);
        m_SpacerGrid       = GridSpacerWidget.Cast( m_RootWidget.FindAnyWidget( "SpacerGrid" ) );

        m_CheckBox         = CheckBoxWidget.Cast( m_RootWidget.FindAnyWidget( "CheckBox" ) );
        m_ItemNameWidget   = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "ItemName" ) );
        m_ItemDistanceWidget = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "ItemDistance" ) );

        m_TrackerEntity    = trackedEntity;
        m_ItemName  	   = itemName;
        m_IsMarkerVisible  = visible;

        m_ItemNameWidget.SetText( m_ItemName );
        m_SpacerGrid.Update();
        m_ItemNameWidget.Update();
        m_ItemDistanceWidget.Update();

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.DoUpdate, 1, true);
    }

    void ~VPPESPTracker() 
	{
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
        if (m_RootWidget != null) 
			m_RootWidget.Unlink();
    }
	
	void InitPlayerEspWidget()
	{
		if (m_RootWidget != null)
			m_RootWidget.Unlink();

		m_RootWidget = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/EspToolsUI/EspTrackerDetailed.layout", m_ParentWidget);
		m_RootWidget.SetHandler(this);
        m_SpacerGrid  = GridSpacerWidget.Cast( m_RootWidget );
		
        m_ItemNameWidget   	 = TextWidget.Cast( m_RootWidget.FindAnyWidget( "NameInput" ) );
        m_ItemDistanceWidget = TextWidget.Cast( m_RootWidget.FindAnyWidget( "DistanceInput" ) );
		m_HealthInput		 = SliderWidget.Cast( m_RootWidget.FindAnyWidget( "HealthInput" ) );
	    m_BloodInput		 = SliderWidget.Cast( m_RootWidget.FindAnyWidget( "BloodInput" ) );
		//m_GUIDInput			 = TextWidget.Cast( m_RootWidget.FindAnyWidget( "GUIDInput" ) );
		m_GUID64Input		 = TextWidget.Cast( m_RootWidget.FindAnyWidget( "GUID64Input" ) );
        m_RootWidget.Update();
	}
	
	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref PlayerStatsData,Object> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Client)
		{
			if (data.param2.GetNetworkIDString() == this.m_TrackerEntity.GetNetworkIDString())
			{
				InitPlayerEspWidget();
				m_playerInfo = data.param1;
				m_ItemNameWidget.SetText( m_playerInfo.GetStat("Name") );
				m_HealthInput.SetCurrent(m_playerInfo.GetStat("Health").ToFloat());
				m_BloodInput.SetCurrent(m_playerInfo.GetStat("Blood").ToFloat());
				//m_GUIDInput.SetText(m_playerInfo.GetStat("GUID"));
				m_GUID64Input.SetText(m_playerInfo.GetStat("Steam64"));
			}
		}
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_CheckBox)
		{
			autoptr EspToolsMenu espManager = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (m_CheckBox.IsChecked())
			{
				//Color update
				m_CheckBox.SetColor(ARGB(150,0,255,21));
				//Add
				if (espManager)
					espManager.AddEntry(m_ItemName, m_TrackerEntity);
			}else{
				//Color update
				m_CheckBox.SetColor(ARGB(150,255,0,0));
				//Remove
				if (espManager)
					espManager.RemoveEntryByObject(m_TrackerEntity);
			}
			
			return true;
		}
		return false;
	}
	
    float CalcDistance() 
	{
		vector startPos    = m_TrackerEntity.GetPosition();
        vector SnappedPos  = g_Game.SnapToGround( Vector(startPos[0], startPos[1], startPos[2]) );
        vector fn    	   = Vector(SnappedPos[0], SnappedPos[1], SnappedPos[2]);
		
        return vector.Distance( GetGame().GetPlayer().GetPosition(), startPos );
    }

    void DoUpdate()
	{
		if (m_TrackerEntity == null)
		{
			delete this;
			return;
		}
		
        if (IsMarkerVisible() && m_RootWidget != NULL && GetGame().GetPlayer()) 
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

            m_SpacerGrid.Update();
            m_ItemNameWidget.Update();
            m_ItemDistanceWidget.Update();
        }
		else if (m_RootWidget != NULL) 
		{
            m_RootWidget.Show(false);
        }
    }

    bool IsMarkerVisible() 
	{
        return m_IsMarkerVisible;
    }
	
	void SetChecked(bool state)
	{
		if (m_CheckBox != null)
			m_CheckBox.SetChecked(state);
	}
	
	bool IsChecked()
	{
		if (m_CheckBox != null)
			return m_CheckBox.IsChecked();
		
		return false;
	}

    void SetMarkerVisible(bool visible) 
	{
        m_IsMarkerVisible = visible;
    }
	
	Object GetTrackingObject()
	{
		return m_TrackerEntity;
	}
};