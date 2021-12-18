class DayZPlayerCameraFree extends DayZPlayerCameraBase
{
	protected vector localVelocity;
	protected float  localyawVelocity;
	protected float  localpitchVelocity;
	protected float  local_camspeed, more_camspeed;
	protected float  local_fov, more_fov;

	protected vector m_startPos;
	protected vector m_startOri;
	protected bool   m_forceFovOverride;

	ref DayZPlayerCameraBase m_pPrevCamera;

	void DayZPlayerCameraFree(DayZPlayer pPlayer, HumanInputController pInput)
	{
		vector mat[4];
		GetGame().GetPlayer().GetTransform(mat);
		m_startPos = mat[3];
		m_startOri = Math3D.MatrixToAngles(mat);
		m_startOri[2] = 0.0;
		pInput.SetDisabled(true);
		more_camspeed = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		more_fov	  = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV);
	}

	void ~DayZPlayerCameraFree()
	{
		HumanInputController hic = m_pPlayer.GetInputController();
		hic.SetDisabled(false);
	}

	override vector GetBaseAngles()
	{
		return m_pPrevCamera.GetBaseAngles();
	}

	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult)
	{
		super.OnActivate(pPrevCamera, pPrevCameraResult);
		
		m_pPrevCamera = DayZPlayerCameraBase.Cast(pPrevCamera);
		m_startPos = GetGame().ObjectModelToWorld(m_pPlayer, pPrevCameraResult.m_CameraTM[3]);
		m_startOri = Math3D.MatrixToAngles(pPrevCameraResult.m_CameraTM);
		m_startOri[2] = 0.0;
	}
	
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult)
	{
		//settings & vars
		float CAM_BOOST 		= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST);
		float CAM_MOVE_DRAG 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG);
		float CAM_MOUSE_SENSE 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE);
		float CAM_SMOOTHNESS 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS);

		HumanInputController hic = m_pPlayer.GetInputController();
		if (hic)
			hic.SetDisabled(true);

		vector mat[4];
		GetCamera(0, mat);
		
		Input input = GetGame().GetInput();
		float forward  = input.LocalValue("UACamForward") - input.LocalValue("UACamBackward");
		float strafe   = input.LocalValue("UACamRight") - input.LocalValue("UACamLeft");
		float altitude = input.LocalValue("UACamUp") - input.LocalValue("UACamDown");
		float yawDiff = input.LocalValue("UACamShiftLeft") - input.LocalValue("UACamShiftRight");
		float pitchDiff = input.LocalValue("UACamShiftDown") - input.LocalValue("UACamShiftUp");
		float speedInc = 0.0;
		Widget underCursor = GetWidgetUnderCursor();
		if ((underCursor == NULL) || (underCursor && underCursor.GetTypeID() != EditBoxWidgetTypeID) && input.LocalValue("UACamFOV") == 0)
		{
			speedInc = input.LocalValue("UACamSpeedAdd") - input.LocalValue("UACamSpeedDeduct");
		}
		bool increaseSpeeds = input.LocalValue("UACamTurbo");

		//Keyboard Control
		local_camspeed  = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		more_camspeed 	+= (more_camspeed * speedInc / CAM_BOOST);
		local_camspeed  += more_camspeed;

		if (local_camspeed < VPPATProfileConstants.MIN_CAM_SPEED)
			local_camspeed = VPPATProfileConstants.MIN_CAM_SPEED;
		
		if(increaseSpeeds)
			local_camspeed = local_camspeed * CAM_BOOST;
		
		vector up = vector.Up;
		vector direction = mat[2];
		vector directionAside = vector.Up * direction;
		
		up = up * altitude * local_camspeed;
		direction = direction * forward * local_camspeed;
		directionAside = directionAside * strafe * local_camspeed;

		localVelocity = localVelocity * CAM_MOVE_DRAG;
		localVelocity = localVelocity + (direction + directionAside + up) * pDt;
		
		m_startPos = m_startPos + localVelocity;

		//FOV override toggle
		if (input.LocalDbl("UACamFOV"))
			m_forceFovOverride = !m_forceFovOverride;

		if (input.LocalValue("UACamFOV") > 0 || m_forceFovOverride)
		{
			CAM_MOUSE_SENSE = 0.1;
			float camFov = input.LocalValue("UACamSpeedAdd") - input.LocalValue("UACamSpeedDeduct");
			float fovVelocity = (fovVelocity + camFov * 0.01);

			more_fov  += fovVelocity;
			local_fov = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV) + more_fov;
			pOutResult.m_fFovAbsolute = local_fov;
		}
		else
		{
			CAM_MOUSE_SENSE = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE);
		}

		//Mouse Control
		localyawVelocity =  (localyawVelocity + yawDiff * CAM_MOUSE_SENSE);
		localpitchVelocity = (localpitchVelocity + pitchDiff * CAM_MOUSE_SENSE);

		m_startOri[0] = m_startOri[0] - Math.RAD2DEG * localyawVelocity * pDt;
		m_startOri[1] = m_startOri[1] - Math.RAD2DEG * localpitchVelocity * pDt;

		localyawVelocity   *= CAM_SMOOTHNESS;
		localpitchVelocity *= CAM_SMOOTHNESS;

		if(m_startOri[1] < -89)
			m_startOri[1] = -89;
			
		if(m_startOri[1] > 89)
			m_startOri[1] = 89;
			
		//Update Position & rotation
		Math3D.YawPitchRollMatrix(m_startOri, pOutResult.m_CameraTM);
		pOutResult.m_CameraTM[3] = m_pPlayer.CoordToLocal(m_startPos);
		//-----
		pOutResult.m_fDistance 			 = -0.075;
		pOutResult.m_fUseHeading 		 = 0;
		pOutResult.m_fInsideCamera 		 = 0.0;
		pOutResult.m_fShootFromCamera	 = 0.0;

		pOutResult.m_iDirectBone 		 = -1;
		pOutResult.m_iDirectBoneMode 	 = 0.0;

		pOutResult.m_fPositionModelSpace   = 1.0;
		pOutResult.m_CollisionIgnoreEntity = m_pPlayer;
		
		pOutResult.m_fIgnoreParentRoll  = 0.0;
		pOutResult.m_fIgnoreParentPitch = 0.0;
		pOutResult.m_fIgnoreParentYaw 	= 0.0;
	}

	override string GetCameraName()
	{
		return "DayZPlayerCameraFree";
	}
};