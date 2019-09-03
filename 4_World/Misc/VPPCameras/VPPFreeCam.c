class VPPFreeCam extends Camera
{
	//---------
	protected float CAM_SPEED = 5.0;
	protected float CAM_BOOST = 10.0;
	protected float CAM_MOVE_DRAG = 0.9;
	protected float CAM_MOUSE_SENSE = 0.4;
	protected float CAM_SMOOTHNESS = 0.8;
	protected float CAM_FOV = 1.0;
	//---------
	
	protected vector localVelocity;
	protected float  localyawVelocity;
	protected float  localpitchVelocity;
	protected float  local_camspeed;
	
	private float m_SendUpdateAcc = 0.0;

	void VPPFreeCam()
	{
		SetEventMask( EntityEvent.FRAME );
	}
	
	void ~VPPFreeCam()
	{

	}
	
	override void EOnFrame( IEntity other, float timeSlice )
	{
		FreeFly(timeSlice);
	}
	
	protected void FreeFly(float timeSlice)
	{
		vector OldOrientation = GetOrientation();
		vector oldPos    = GetPosition();
		vector oldDir    = GetDirection();

		Input input = GetGame().GetInput();
		
		float forward  = input.LocalValue( "UAMoveForward" ) - input.LocalValue( "UAMoveBack" );
		float strafe   = input.LocalValue( "UAMoveRight" ) - input.LocalValue( "UAMoveLeft" );
		float altitude = input.LocalValue( "UALeanLeft" ) - input.LocalValue( "UACamDown" );
		float yawDiff = input.LocalValue( "UAAimLeft" ) - input.LocalValue( "UAAimRight" );
		float pitchDiff = input.LocalValue( "UAAimDown" ) - input.LocalValue( "UAAimUp" );
		float speedInc = 0;
		autoptr Widget underCursor = GetWidgetUnderCursor();
		if (underCursor != null && underCursor.GetTypeID() != EditBoxWidgetTypeID)
		{
			speedInc = input.LocalValue( "UACamSpeedAdd" ) - input.LocalValue( "UACamSpeedDeduct" );
		}
		bool increaseSpeeds = input.LocalValue( "UACamTurbo" );

		//Keyboard Control
		local_camspeed = CAM_SPEED;
		if ( CAM_BOOST > 0 )
		{
			CAM_SPEED = CAM_SPEED + ( CAM_SPEED * speedInc / CAM_BOOST );
			
			if ( CAM_SPEED < 0.01 ) {
				CAM_SPEED = 0.01;
			}
			
			local_camspeed = CAM_SPEED;

			if( increaseSpeeds )
				local_camspeed = local_camspeed * CAM_BOOST;
		}

		vector up = vector.Up;
		vector direction = GetDirection();
		vector directionAside = vector.Up * direction;
		
		up = up * altitude * local_camspeed;
		direction = direction * forward * local_camspeed;
		directionAside = directionAside * strafe * local_camspeed;

		localVelocity = localVelocity * CAM_MOVE_DRAG;
		localVelocity = localVelocity + (direction + directionAside + up) * timeSlice;

		vector newPos = oldPos + localVelocity;

		float surfaceY = GetGame().SurfaceY( newPos[0], newPos[2] ) + 0.15; //0.25
		if ( newPos[1] < surfaceY )
			newPos[1] = surfaceY;

		//Mouse Control
		localyawVelocity =  (localyawVelocity + yawDiff * CAM_MOUSE_SENSE);
		localpitchVelocity = (localpitchVelocity + pitchDiff * CAM_MOUSE_SENSE);

		vector OrientNew = OldOrientation;
		OrientNew[0] = OldOrientation[0] - Math.RAD2DEG * localyawVelocity * timeSlice;
		OrientNew[1] = OldOrientation[1] - Math.RAD2DEG * localpitchVelocity * timeSlice;

		localyawVelocity   *= CAM_SMOOTHNESS;
		localpitchVelocity *= CAM_SMOOTHNESS;

		if( OrientNew[1] < -89 )
			OrientNew[1] = -89;
			
		if( OrientNew[1] > 89 )
			OrientNew[1] = 89;
			
		//Update Position & rotation
		SetPosition(newPos);
		SetOrientation(OrientNew);
		
		if( m_SendUpdateAcc > 0.5 )
		{
			GetGame().UpdateSpectatorPosition(GetPosition());
			m_SendUpdateAcc = 0;
		}
		
		m_SendUpdateAcc = m_SendUpdateAcc + timeSlice;
	}
	
	void SnapToAngle(float angle)
	{
		vector OldOrientation = GetOrientation();
		SetOrientation(Vector(OldOrientation[0],angle,OldOrientation[2]));
	}
	
	void SetYaw(float yaw)
	{
		vector OldOrientation = GetOrientation();
		SetOrientation(Vector(yaw,OldOrientation[1],OldOrientation[2]));
	}
	
	void SetPitch(float pitch)
	{
		vector OldOrientation = GetOrientation();
		SetOrientation(Vector(OldOrientation[0],pitch,OldOrientation[2]));
	}
	
	void SetRoll(float roll)
	{
		vector OldOrientation = GetOrientation();
		SetOrientation(Vector(OldOrientation[0],OldOrientation[1],roll));
	}
	
	void SetFov(float fov)
	{
		CAM_FOV = fov;
		SetFOV(CAM_FOV);
	}
	
	float GetFov()
	{
		return CAM_FOV;
	}
};