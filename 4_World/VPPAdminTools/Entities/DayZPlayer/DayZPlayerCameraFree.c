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
		m_startOri[1] = 0.0;
		m_startOri[2] = 0.0;

		more_camspeed = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		more_fov	  = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV);
	}

	void ~DayZPlayerCameraFree()
	{
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
		m_startOri[1] = 0.0;
		m_startOri[2] = 0.0;
	}
	
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult)
	{
		//settings & vars
		float CAM_BOOST 		= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST);
		float CAM_MOVE_DRAG 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG);
		float CAM_MOUSE_SENSE 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE);
		float CAM_SMOOTHNESS 	= g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS);

		vector mat[4];
		GetCamera(0, mat);
		
		Input input = GetGame().GetInput();
		float forward, strafe, altitude, yawDiff, pitchDiff, speedInc = 0.0;

		if (CanControl())
		{
			forward  = input.LocalValue("UACamForward") - input.LocalValue("UACamBackward");
			strafe   = input.LocalValue("UACamRight") - input.LocalValue("UACamLeft");
			altitude = input.LocalValue("UACamUp") - input.LocalValue("UACamDown");
			yawDiff = input.LocalValue("UACamShiftLeft") - input.LocalValue("UACamShiftRight");
			pitchDiff = input.LocalValue("UACamShiftDown") - input.LocalValue("UACamShiftUp");
			speedInc = 0.0;
		}
		
		if (CanControl() && input.LocalValue("UACamFOV") == 0)
		{
			speedInc = (input.LocalValue("UACamSpeedAdd") - input.LocalValue("UACamSpeedDeduct")) * 1.5;
		}
		bool increaseSpeeds = input.LocalValue("UACamTurbo");

		//Keyboard Control
		local_camspeed  = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		more_camspeed 	+= (more_camspeed * speedInc / CAM_BOOST);
		local_camspeed  += more_camspeed;

		if(increaseSpeeds)
			local_camspeed = local_camspeed * CAM_BOOST;

		if (local_camspeed < VPPATProfileConstants.MIN_CAM_SPEED){
			local_camspeed = VPPATProfileConstants.MIN_CAM_SPEED;
			more_camspeed  = VPPATProfileConstants.MIN_CAM_SPEED;
		}
		
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
		if (input.LocalDbl("UACamFOV") && CanControl())
			m_forceFovOverride = !m_forceFovOverride;

		if ((input.LocalValue("UACamFOV") > 0 && CanControl()) || m_forceFovOverride)
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
		pOutResult.m_fUseHeading 		 = 0.0;
		pOutResult.m_fInsideCamera 		 = 0.0;
		pOutResult.m_fShootFromCamera	 = 0.0;

		pOutResult.m_iDirectBone 		 = -1;
		pOutResult.m_iDirectBoneMode 	 = 0.0;
		
		pOutResult.m_fPositionModelSpace   = 1.0;
		pOutResult.m_CollisionIgnoreEntity = m_pPlayer;
		
		pOutResult.m_fIgnoreParentRoll  = 1.0;
		pOutResult.m_fIgnoreParentPitch = 1.0;
		pOutResult.m_fIgnoreParentYaw 	= 0.0;
	}

	vector GetCurrentPosition()
	{
		return m_startPos;
	}

	bool CanControl()
	{
		bool isTyping;

		if (GetGame().GetUIManager().GetMenu() == null)
			isTyping = false;

		Widget underCursor = GetWidgetUnderCursor();
		UIScriptedMenu chatInput = GetGame().GetUIManager().FindMenu(MENU_CHAT_INPUT);
				
		if (underCursor && GetGame().GetUIManager().IsCursorVisible() && (underCursor.GetTypeID() == EditBoxWidgetTypeID || underCursor.GetTypeID() == MultilineEditBoxWidgetTypeID))
			isTyping = true;
		
		if (chatInput != null && chatInput.IsVisible())
			isTyping = true;
		
		return (!isTyping && !GetGame().GetUIManager().IsCursorVisible());
	}

	override string GetCameraName()
	{
		return "DayZPlayerCameraFree";
	}
};