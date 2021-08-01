class VPPPlacementCam extends Camera
{
	//---------
	protected float CAM_SPEED = 5.0;
	protected float CAM_ROTATE_SPEED = 1.0;
	protected float CAM_BOOST = 10.0;
	protected float CAM_MOVE_DRAG = 0.9;
	protected float CAM_MOUSE_SENSE = 0.8;
	protected float CAM_SMOOTHNESS = 0.8;
	
	protected vector localVelocity;
	protected float  localyawVelocity;
	protected float  localpitchVelocity;
	protected float  local_camspeed;
	protected float  local_camRotateSpeed;
	protected float  staticAngleYaw   = 0;
	protected float  staticAnglePitch = -89;
	
	protected vector offSetPos;
	protected float camHeight;
	protected float rotTickCounter = 0;
	protected float rotSpeed;
	protected float rotWidthMax;
	protected float rotHeightMax;
	protected float motionX = 0.0;
	protected float motionY = 0.0;
	//---------
	
	Object		 		m_ObjectToPlace;
	bool 				m_Initialized;
	vector 				m_cam_pos;
	vector 				m_camera_dir;
	vector 				m_cameraTrans[4];
	vector 				m_cameraRotTrans[4];
	vector 				m_demoPos;
	
	const float 		FOV = 1.0236;
	
	void VPPPlacementCam()
	{
		m_ObjectToPlace = GetGame().CreateObject("land_container_1moh", GetPosition(), true, false/*ai*/);
		SetFOV(FOV);

		SetEventMask( EntityEvent.FRAME );
		m_Initialized = true;

	}
	
	override void EOnFrame( IEntity other, float timeSlice )
	{
		UpdateProjection(timeSlice);
	}
	
	vector CalculatePos()
	{
		Math3D.MatrixIdentity4(m_cameraTrans);
		Math3D.DirectionAndUpMatrix(m_camera_dir/* - SnapToGround(GetPosition())*/, "0 1 0", m_cameraTrans);
		m_cameraTrans[3] = GetPosition();
		m_demoPos = Vector(0,0, 3.0).Multiply4(m_cameraTrans);
		m_demoPos[1] = GetGame().SurfaceY(m_demoPos[0], m_demoPos[2]);
		
		return m_demoPos;
	}
	
	protected void UpdateProjection(float timeSlice)
	{
		if(!m_Initialized) return;
		if(m_ObjectToPlace == null) return;
		
		vector oldPos = GetPosition();
		m_camera_dir = GetDirection();
		m_camera_dir.Normalize();

		Input input = GetGame().GetInput();
		
		float forward  		= input.LocalValue( "UACamForward" ) - input.LocalValue( "UACamBackward" );
		float strafe   	 	= input.LocalValue( "UACamRight" ) - input.LocalValue( "UACamLeft" );
		float speedInc	    = input.LocalValue( "UACamSpeedAdd" ) - input.LocalValue( "UACamSpeedDeduct" );
		bool increaseSpeeds = input.LocalValue( "UACamTurbo" );
		float yawDiff = input.LocalValue( "UACamShiftLeft" ) - input.LocalValue( "UACamShiftRight" );
		float pitchDiff = input.LocalValue( "UACamShiftDown" ) - input.LocalValue( "UACamShiftUp" );
		float camRollShifter  = input.LocalValue( "UAFire" ) - input.LocalValue( "UATempRaiseWeapon" );
		
		//Cam Speed Control
		local_camspeed = CAM_SPEED;
		if ( CAM_BOOST > 0 && !g_Game.IsLeftCtrlDown())
		{
			CAM_SPEED = CAM_SPEED + ( CAM_SPEED * speedInc / CAM_BOOST );
			
			if ( CAM_SPEED < 0.01 ) {
				CAM_SPEED = 0.01;
			}
			
			local_camspeed = CAM_SPEED;

			if( increaseSpeeds )
				local_camspeed = local_camspeed * CAM_BOOST;
		}
		
		//Cam Rotation Speed Control
		local_camRotateSpeed = CAM_ROTATE_SPEED;
		if ( CAM_BOOST > 0 )
		{
			CAM_ROTATE_SPEED = CAM_ROTATE_SPEED + ( CAM_ROTATE_SPEED * speedInc / CAM_BOOST );
			
			if ( CAM_ROTATE_SPEED < 0.01 ) {
				CAM_ROTATE_SPEED = 0.01;
			}
			
			local_camRotateSpeed = CAM_ROTATE_SPEED;

			if( increaseSpeeds )
				local_camRotateSpeed = local_camRotateSpeed * CAM_BOOST;
		}

		vector direction = GetDirection();
		vector directionAside = vector.Up * direction;
		
		direction = direction * forward * local_camspeed;
		directionAside = directionAside * strafe * local_camspeed;

		localVelocity = localVelocity * CAM_MOVE_DRAG;
		localVelocity = localVelocity + (direction + directionAside) * timeSlice;
		
	
		vector projectionPos = oldPos + localVelocity;
	
		vector camAngle = GetOrientation();
		if (input.LocalRelease( "UACamRelease" )) //LeftControl
		{
			SetPosition(m_ObjectToPlace.GetPosition());
			return;
		}
		
		if (g_Game.IsLeftCtrlDown())
		{
			//Rotate In place
			localyawVelocity   = (localyawVelocity + yawDiff * local_camRotateSpeed);
			localpitchVelocity = (localpitchVelocity + pitchDiff * local_camRotateSpeed);
			
			localyawVelocity   *= CAM_SMOOTHNESS;
			localpitchVelocity *= CAM_SMOOTHNESS;
			
			m_ObjectToPlace.SetOrientation(Vector(m_ObjectToPlace.GetOrientation()[0] - Math.RAD2DEG * localyawVelocity * timeSlice,0,m_ObjectToPlace.GetOrientation()[2]));
			
			rotWidthMax  = 15.0; //TODO: make size based on object world size (make this optional angle view)
			rotHeightMax = 15.0; //TODO: make size based on object world size (make this optional angle view)
			
			float turnZ	= camRollShifter * 45.0 * timeSlice;
			float roll = GetOrientation()[2];
			float oldHeight = GetPosition()[1];
			
			//Rotate Cam around object
			if (input.LocalPress( "UACamLeft", false ) || input.LocalHold( "UACamLeft", false ))
			{
				rotTickCounter += timeSlice * local_camRotateSpeed;
				motionX = (double)(Math.Cos(rotTickCounter) * rotWidthMax);
	       		motionY = (double)(Math.Sin(rotTickCounter) * rotHeightMax);
			}
			else if (input.LocalPress( "UACamRight", false ) || input.LocalHold( "UACamRight", false ))
			{
				rotTickCounter += timeSlice * local_camRotateSpeed;
				motionX = (double)(Math.Cos(rotTickCounter) * rotWidthMax);
	       		motionY = (double)(Math.Sin(rotTickCounter) * rotHeightMax);
			}
			offSetPos = Vector(motionX,motionY,0).Multiply4(m_cameraTrans);
			
			vector fn = Vector(offSetPos[0],projectionPos[1],offSetPos[2]);
			vector lookDir = m_ObjectToPlace.GetPosition() - GetPosition();

			//Cam Height
			camHeight = oldHeight + turnZ * local_camRotateSpeed; //TODO: HEIGHT ADJUSTMENT BASED ON OBJECT WORLD SIZE
			if (camHeight > (GetGame().SurfaceY( projectionPos[0], projectionPos[2] )))
				SetPosition(Vector(fn[0],camHeight,fn[2]));
			
			SetDirection(lookDir);
			return;
		}
		else
		{
			//TODO: make this angle an option| topdown view
			projectionPos[1] = GetGame().SurfaceY( projectionPos[0], projectionPos[2] ) + 10; //TODO: HEIGHT ADJUSTMENT BASED ON OBJECT WORLD SIZE
		
			SetPosition(projectionPos);
			SetYawPitchRoll(Vector(0,staticAnglePitch,0));
			
			if (!g_Game.IsLeftCtrlDown() || input.LocalPress( "UACamForward", false ) || input.LocalPress( "UACamBackward", false ) || input.LocalPress( "UACamRight", false ) || input.LocalPress( "UACamLeft", false ) || input.LocalHold( "UACamForward", false ) || input.LocalHold( "UACamBackward", false ) || input.LocalHold( "UACamRight", false ) || input.LocalHold( "UACamLeft", false ))
			{
				m_ObjectToPlace.SetPosition(CalculatePos());
			}
		}
	}
};