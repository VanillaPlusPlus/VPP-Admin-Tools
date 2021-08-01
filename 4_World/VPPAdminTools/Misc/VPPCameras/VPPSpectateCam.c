class VPPSpectateCam extends Camera
{
	private vector m_y_p_r_previous;
	private vector u_d_p_r_previous;
	private float m_bblUpdateInterval = 0.0;
	PlayerBase m_FollowObject;

	override void EOnFrame( IEntity other, float timeSlice )
	{
		if (m_FollowObject == NULL) return;
			FollowTarget3PP(timeSlice);
	}
	
	void VPPSpectateCam()
	{
		SetEventMask( EntityEvent.FRAME );
	}
	
	void SetTargetObj(PlayerBase target)
	{
		m_FollowObject = target;
		PPEffects.OverrideDOF(false, 0, 0, 0, 0, 1);
	}
	
	protected void FollowTargetFront()
	{
		if ( m_FollowObject == null ) return;
		
		vector m_player_dir = m_FollowObject.GetDirection();
		m_player_dir.Normalize();
		
		vector m_cameraTrans[4];
		vector player_pos = Vector(m_FollowObject.GetPosition()[0],m_FollowObject.GetPosition()[1] + 1.7,m_FollowObject.GetPosition()[2]);
		Math3D.MatrixIdentity4(m_cameraTrans);
		Math3D.DirectionAndUpMatrix(m_player_dir, "0 0 0", m_cameraTrans);
		m_cameraTrans[3] = player_pos;
		vector m_demoPos = Vector(0,0, 3.68823).Multiply4(m_cameraTrans);

		vector m_demoRot = "0 0 0";
		SetPosition(m_demoPos);
		m_demoRot[0] = Math.Atan2(-m_player_dir[0], -m_player_dir[2]) * Math.RAD2DEG;
		SetOrientation(m_demoRot);
		
	}

	protected void FollowTarget3PP(float timeSlice)
	{
		if ( m_FollowObject == null ) return;
		
		float ud = m_FollowObject.GetAimingAngleUD();
		vector aimOrientation = m_FollowObject.GetOrientation();
		aimOrientation[0] = aimOrientation[0];
		aimOrientation[1] = aimOrientation[1] + ud;
		
		SetOrientation(Vector(m_FollowObject.GetOrientation()[0], aimOrientation.VectorToAngles()[1] ,m_FollowObject.GetOrientation()[2]));

		float f = GetOrientation()[0] * 0.017453292;
        float motionX  = (double)(Math.Sin(f) * 1.0);
        float motionZ = (double)(Math.Cos(f) * 1.0); 
		
		vector camPOS = m_FollowObject.GetPosition();
		camPOS[0] = camPOS[0] - motionX;
		camPOS[1] = m_FollowObject.GetPosition()[1] + 1.6;
		camPOS[2] = camPOS[2] - motionZ;

		SetPosition(camPOS);
		
		if( m_bblUpdateInterval > 0.5 )
		{
			GetGame().UpdateSpectatorPosition(GetPosition());
			m_bblUpdateInterval = 0;
		}
		m_bblUpdateInterval = m_bblUpdateInterval + timeSlice;
	}
	
	vector GetHeadingPosition(int distance, Human hmn)
	{
		float heading = MiscGameplayFunctions.GetHeadingAngle(DayZPlayerImplement.Cast(hmn));
		vector posModifier = Vector(-(distance * Math.Sin(heading)), 0, distance * Math.Cos(heading));
		return posModifier + hmn.GetPosition();
	}
	
	
	vector SmoothMovement( vector y_p_r, float timeslice )
	{
		if ( m_y_p_r_previous )
		{					
			if ( Math.AbsFloat( y_p_r[0] - m_y_p_r_previous[0] ) > 100 )
			{												
				if ( y_p_r[0] > 0 )
				{		
					m_y_p_r_previous[0] = m_y_p_r_previous[0] + 360; 
				}
				
				if ( y_p_r[0] < 0 )
				{		
					m_y_p_r_previous[0] = m_y_p_r_previous[0] - 360;
				}
			}
			
			y_p_r[0] = Math.Lerp( m_y_p_r_previous[0], y_p_r[0], 15 * timeslice );
			y_p_r[1] = Math.Lerp( m_y_p_r_previous[1], y_p_r[1], 15 * timeslice );
			y_p_r[2] = Math.Lerp( m_y_p_r_previous[2], y_p_r[2], 15 * timeslice );
		}
		
		m_y_p_r_previous = y_p_r;
		
		return y_p_r;
	}
};