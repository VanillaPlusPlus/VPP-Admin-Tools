class VPPDayZPlayerCameraSpectate extends DayZPlayerCameraBase
{
	protected PlayerBase m_Target;
	protected int        m_HeadBoneIdx;
	protected int        m_DbgLastDriftLog;  //throttle for the link-drift log
	//single source of truth for the HB lever so the probe log can never drift
	//from what EmitResult actually writes
	static const bool    ADS_UPDATE_EVERY_FRAME = true;
	//FOLLOW-branch HB lever (July 9 round 3's one behavior change): same engine
	//flag, SEPARATE constant so reverting one branch can never silently flip the other
	static const bool    FOLLOW_UPDATE_EVERY_FRAME = true;
	protected int        m_DbgLastSrcLog;    //g_Game.GetTime() of the last ADS probe BURST start
	protected int        m_DbgBurstLeft;     //frames remaining in the current unthrottled probe burst
	protected int        m_DbgBurstPrevTime; //timestamp of the previous burst frame (continuity guard)
	protected float      m_DbgBurstPrevYaw;  //previous burst frame: sampled sight-line yaw (signed)
	protected float      m_DbgBurstPrevBody; //previous burst frame: target body yaw (signed)
	//follow-branch burst probe state (separate from the ADS burst so an ADS
	//enter/exit can never tangle the two burst windows)
	protected int        m_DbgFolLastBurst;  //g_Game.GetTime() of the last follow burst start
	protected int        m_DbgFolBurstLeft;  //frames remaining in the current follow burst
	protected int        m_DbgFolPrevTime;   //timestamp of the previous follow burst frame
	protected vector     m_DbgFolPrevHead;   //previous frame: RAW head-bone WS position
	protected vector     m_DbgFolPrevOut;    //previous frame: emitted world cam position
	protected float      m_DbgFolPrevBody;   //previous frame: target body yaw (signed)
	protected float      m_DbgFolPrevYaw;    //previous frame: emitted cam yaw (signed)
	protected int        m_View;

	//third person
	protected float      m_Dist;
	protected float      m_OrbitYaw;
	protected float      m_OrbitPitch;

	//angle smoothing state (VPPBR filter)
	protected float      m_YawCurrent;
	protected float      m_YawVelocity;
	protected float      m_PitchCurrent;
	protected float      m_PitchVelocity;

	//first person position smoothing (per-axis SmoothCD velocities, world space)
	protected vector     m_SmoothedHeadPos;
	protected float      m_PosVelX[1];
	protected float      m_PosVelY[1];
	protected float      m_PosVelZ[1];


	//ADS sight state (EVPPSpectateSightMode, server-pushed)
	protected int         m_TargetSightMode;
	protected EntityAI    m_SightInHands;   //in-hands entity at resolve time (re-resolution trigger)
	protected Weapon_Base m_SightWeapon;
	protected ItemOptics  m_SightOptic;
	protected EntityAI    m_SightEntity;    //resolved sight source (weapon or optic)
	protected vector      m_SightCamPosMS;
	protected vector      m_SightCamDirMS;  //normalized, model space
	protected bool        m_SightValid;

	//FOV blend state (single restore path for every exit)
	protected float      m_FovCurrent;
	protected float      m_FovVel[1];

	protected float      m_StreamLookYaw;
	protected float      m_StreamLookPitch;
	protected int        m_StreamLookTime; //g_Game.GetTime() of last sample (0 = never)
	protected float      m_CorrYawApplied;

	protected vector     m_OutCamPos;
	protected vector     m_OutCamAngles;
	protected vector     m_LastCamPosWS;
	protected vector     m_LastCamAngles;
	protected bool       m_HasLastFrame;
	protected float      m_NearPlaneCurrent;

	//optic visuals borrowed from the vanilla enter-optics path, applied to OUR
	//instance of the target's optic (client-visual only). Tracked separately from
	//the resolve state so a re-resolve can restore exactly what was touched.
	protected ItemOptics  m_VisualOptic;
	protected Weapon_Base m_VisualWeapon;
	protected bool        m_VisualsApplied;
	protected bool        m_VisualEnteredOptics;
	protected bool        m_VisualSwitchedEM;
	protected bool        m_VisualModelsHidden; //2D scope: weapon+attachments hidden
	protected bool        m_DbgFightLogged;  //throttle for the per-frame upkeep log
	protected int         m_DbgLastEmitLog;  //throttle for the alignment probe
	protected PPERequester_CameraADS m_RequesterADS;

	//previous camera, for GetBaseAngles seeding at exit (freecam pattern)
	ref DayZPlayerCameraBase m_pPrevCamera;

	//diagnostics: whether the engine polls OnDrawOptics2D under this camera
	protected static bool s_DbgOverlayPollLogged;
	protected static int  s_DbgLastServeLog;
	protected static int  s_DbgLastHeartbeat;

	//the one live spectate cam (consumed by modded PlayerBase.OnDrawOptics2D)
	protected static VPPDayZPlayerCameraSpectate s_ActiveInstance;

	protected static PlayerBase s_PendingTarget;
	protected static int        s_PendingView;
	protected static int        s_PendingSightMode;

	void VPPDayZPlayerCameraSpectate(DayZPlayer pPlayer, HumanInputController pInput)
	{
		s_ActiveInstance = this;
		m_View = EVPPSpectateView.THIRD_PERSON;
		m_Dist = VPPSpectateConstants.CAM_3PP_DISTANCE;
		m_TargetSightMode = EVPPSpectateSightMode.NONE;
		m_NearPlaneCurrent = 0.07;

		m_FovCurrent = g_Game.GetUserFOV();
		m_FovVel[0]  = 0.0;

		SetTarget(s_PendingTarget);
		SetView(s_PendingView);
		SetTargetADS(s_PendingSightMode);
	}

	void ~VPPDayZPlayerCameraSpectate()
	{
		ClearOpticVisuals();
		if (s_ActiveInstance == this)
			s_ActiveInstance = null;
	}

	//-----------------------------------------------------------------------------
	// Handler -> camera push protocol (statics; instance may not exist yet)
	//-----------------------------------------------------------------------------
	static void PushTarget(PlayerBase target)
	{
		s_PendingTarget = target;
		if (s_ActiveInstance != null)
			s_ActiveInstance.SetTarget(target);
	}

	static void PushView(int view)
	{
		s_PendingView = view;
		if (s_ActiveInstance != null)
			s_ActiveInstance.SetView(view);
	}

	static void PushSightMode(int mode)
	{
		s_PendingSightMode = mode;
		if (s_ActiveInstance != null)
			s_ActiveInstance.SetTargetADS(mode);
	}

	//20Hz true-look sample from the target's client (via server relay)
	static void PushLook(float yaw, float pitch)
	{
		if (s_ActiveInstance != null)
			s_ActiveInstance.SetStreamedLook(yaw, pitch);
	}

	void SetStreamedLook(float yaw, float pitch)
	{
		if (VPPSpectateConstants.ADS_DEBUG && m_StreamLookTime == 0)
			DbgADS("look stream: first sample received yaw=" + yaw.ToString() + " pitch=" + pitch.ToString());

		//fold VectorToAngles' [0..360) into the signed range our filters expect
		if (yaw > 180.0)
			yaw = yaw - 360.0;
		if (pitch > 180.0)
			pitch = pitch - 360.0;

		m_StreamLookYaw   = yaw;
		m_StreamLookPitch = pitch;
		m_StreamLookTime  = g_Game.GetTime();
	}

	protected bool HasFreshStreamedLook()
	{
		if (m_StreamLookTime == 0)
			return false;
		return (g_Game.GetTime() - m_StreamLookTime) < VPPSpectateConstants.LOOK_STREAM_STALE_MS;
	}

	protected float ComputeStreamCorrTargetYaw(float weaponYawSigned, out float outErr, out float outAlpha)
	{
		float err = m_StreamLookYaw - weaponYawSigned;
		if (err > 180.0)
			err = err - 360.0;
		if (err < -180.0)
			err = err + 360.0;
		outErr = err;
		outAlpha = 0.0;

		if (!HasFreshStreamedLook())
			return 0.0;

		float absErr = Math.AbsFloat(err);
		float fullB = VPPSpectateConstants.CAM_ADS_CORR_FULL_DEG;
		float zeroB = VPPSpectateConstants.CAM_ADS_CORR_ZERO_DEG;
		float alpha = 0.0;
		if (absErr <= fullB)
		{
			alpha = 1.0;
		}
		else if (absErr >= zeroB)
		{
			alpha = 0.0;
		}
		else
		{
			alpha = 1.0 - ((absErr - fullB) / (zeroB - fullB));
		}
		alpha = alpha * VPPSpectateConstants.CAM_ADS_CORR_STRENGTH;
		outAlpha = alpha;
		return alpha * err;
	}

	//called by the handler at teardown BEFORE the camera flag flips, so no frame
	//renders scope PPE over the returning vanilla camera and OnDrawOptics2D stops
	//serving instantly. The engine destroys the instance afterwards on its own.
	static void OnTeardown()
	{
		if (s_ActiveInstance != null)
		{
			//force the instance INERT (target null + sight mode NONE + visuals
			//cleared): the engine may still run its OnUpdate for a frame before the
			//camera swap, and a live sight state would RE-APPLY the optic visuals
			//through the ADS re-resolution trigger right after this cleared them
			s_ActiveInstance.SetTarget(null);
			s_ActiveInstance = null;
		}
		s_PendingTarget    = null;
		s_PendingView      = EVPPSpectateView.THIRD_PERSON;
		s_PendingSightMode = EVPPSpectateSightMode.NONE;
	}

	//-----------------------------------------------------------------------------
	// DayZPlayerCamera lifecycle overrides
	//-----------------------------------------------------------------------------
	override void OnActivate(DayZPlayerCamera pPrevCamera, DayZPlayerCameraResult pPrevCameraResult)
	{
		super.OnActivate(pPrevCamera, pPrevCameraResult);
		m_pPrevCamera = DayZPlayerCameraBase.Cast(pPrevCamera);
	}

	//seeds the vanilla camera's view direction when spectate exits
	override vector GetBaseAngles()
	{
		if (m_pPrevCamera != null)
			return m_pPrevCamera.GetBaseAngles();
		return vector.Zero;
	}

	//no super: the base implementation (scheduled via CallLater from OnActivate)
	//stops PPERequester_CameraADS and un-hides the weapon barrel — it would stomp
	//the spectate ADS scope PPE inside the activation window. BUT when no spectate
	//visuals are applied, DO stop the requester: the admin may have entered
	//spectate while ADS (the latch never processes its exit edge — input frozen)
	//and their own ironsights DOF / scope mask PPE would otherwise leak over the
	//whole session.
	override void SetCameraPP(bool state, DayZPlayerCamera launchedFrom)
	{
		if (!m_VisualsApplied)
		{
			PPERequester_CameraADS req = PPERequester_CameraADS.Cast(PPERequesterBank.GetRequester(PPERequester_CameraADS));
			if (req)
				req.Stop();
		}
	}

	//EMPTY on purpose: NV must follow the SPECTATED view, not the admin's own
	//NVG state; the previous scripted camera never managed NV either.
	override void UpdateCameraNV(PlayerBase player)
	{
	}

	override string GetCameraName()
	{
		return "VPPDayZPlayerCameraSpectate";
	}

	//-----------------------------------------------------------------------------
	void SetTarget(PlayerBase target)
	{
		m_Target = target;
		PPEffects.OverrideDOF(false, 0, 0, 0, 0, 1);

		//new target: sight state belongs to the old one (server re-evaluates within one ADS tick)
		m_TargetSightMode = EVPPSpectateSightMode.NONE;
		ClearSight();

		//streamed look belongs to the old target too — invalidate until the new
		//target's stream arrives (fallback covers the gap); the applied correction
		//belongs to the old target as well (this coincides with a full reseed)
		m_StreamLookTime = 0;
		m_CorrYawApplied = 0.0;

		//restore the view-appropriate near plane (an old target's optic override
		//must not leak onto the new target's plain view)
		if (m_View == EVPPSpectateView.FIRST_PERSON)
			m_NearPlaneCurrent = VPPSpectateConstants.CAM_1PP_NEARPLANE;
		else
			m_NearPlaneCurrent = 0.07;

		if (m_Target != null)
		{
			m_HeadBoneIdx = m_Target.GetBoneIndexByName("Head");
			SeedFromTarget();
		}
	}

	PlayerBase GetTarget()
	{
		return m_Target;
	}

	void SetView(int view)
	{
		m_View = view;
		ResetOrbit();

		if (m_View == EVPPSpectateView.FIRST_PERSON)
		{
			m_NearPlaneCurrent = VPPSpectateConstants.CAM_1PP_NEARPLANE;
		}
		else
		{
			m_NearPlaneCurrent = 0.07;
			ClearOpticVisuals(); //scope PPE/reticle must never ride into 3PP
		}

		SeedFromTarget();

		//entering 1PP while the target is already on sights: land directly in the ADS view
		if (m_View == EVPPSpectateView.FIRST_PERSON && m_TargetSightMode != EVPPSpectateSightMode.NONE)
		{
			ResolveSight();
			if (m_SightValid)
				SeedADS();
		}
	}

	int GetView()
	{
		return m_View;
	}

	void ToggleView()
	{
		if (m_View == EVPPSpectateView.FIRST_PERSON)
			SetView(EVPPSpectateView.THIRD_PERSON);
		else
			SetView(EVPPSpectateView.FIRST_PERSON);
	}

	//server-pushed sight mode (EVPPSpectateSightMode)
	void SetTargetADS(int mode)
	{
		if (mode == m_TargetSightMode)
			return;

		DbgADS("edge: mode=" + mode.ToString() + " view=" + m_View.ToString());

		m_TargetSightMode = mode;

		if (mode == EVPPSpectateSightMode.NONE)
		{
			ClearSight();
			if (m_View == EVPPSpectateView.FIRST_PERSON)
			{
				m_NearPlaneCurrent = VPPSpectateConstants.CAM_1PP_NEARPLANE;
				if (m_Target && m_Target.IsAlive())
					SeedFromTarget();
			}
			return;
		}

		ResolveSight();
		if (m_View == EVPPSpectateView.FIRST_PERSON && m_SightValid)
			SeedADS();
	}

	//-----------------------------------------------------------------------------
	// Per-frame update (engine-driven)
	//-----------------------------------------------------------------------------
	override void OnUpdate(float pDt, out DayZPlayerCameraResult pOutResult)
	{
		//deliberately NO super.OnUpdate: the base StdFovUpdate would fight our
		//FOV blend and the NV path must stay off

		//stale-instance guard (vanilla base OnUpdate has the same): the engine can
		//tick an outgoing camera instance after the swap — it must never resolve
		//sight state or re-apply optic visuals from beyond the grave
		if (!m_pPlayer || !PlayerBase.Cast(m_pPlayer))
			return;
		if (PlayerBase.Cast(m_pPlayer).GetCurrentCamera() != this)
			return;

		if (m_Target != null)
		{
			PlayerBase ownBody = PlayerBase.Cast(m_pPlayer);
			if (ownBody != null)
			{
				float linkDrift = vector.Distance(m_pPlayer.GetPosition(), m_Target.GetPosition());
				if (linkDrift > 0.05)
				{
					ownBody.VPPATReassertSpectateLink(m_Target);

					if (VPPSpectateConstants.ADS_DEBUG)
					{
						int nowDrift = g_Game.GetTime();
						if (nowDrift - m_DbgLastDriftLog > 2000)
						{
							m_DbgLastDriftLog = nowDrift;
							DbgADS("link: drift=" + linkDrift.ToString() + "m - identity re-asserted");
						}
					}
				}
			}
		}

		if (m_Target == null || !m_Target.IsAlive())
			ClearOpticVisuals();

		if (m_Target == null)
		{
			if (!m_HasLastFrame)
			{
				//ctor race fallback: at most one frame before the server force-ends
				m_OutCamPos    = m_pPlayer.GetPosition() + Vector(0, 1.6, 0);
				m_OutCamAngles = vector.Zero;
			}
			else
			{
				m_OutCamPos    = m_LastCamPosWS;
				m_OutCamAngles = m_LastCamAngles;
			}
			EmitResult(pOutResult);
			return;
		}

		if (m_View == EVPPSpectateView.FIRST_PERSON)
		{
			if (m_TargetSightMode != EVPPSpectateSightMode.NONE && m_Target.IsAlive())
				UpdateFirstPersonADS(pDt);
			else
				UpdateFirstPerson(pDt);
		}
		else
		{
			UpdateThirdPerson(pDt);
		}

		UpdateFOV(pDt);
		EmitResult(pOutResult);
	}

	protected void EmitResult(out DayZPlayerCameraResult pOutResult)
	{
		bool adsRendering = false;
		if (m_View == EVPPSpectateView.FIRST_PERSON && m_TargetSightMode != EVPPSpectateSightMode.NONE && m_SightValid && m_Target != null && m_Target.IsAlive())
			adsRendering = true;

		if (adsRendering)
		{
			vector adsOwnPos = m_pPlayer.GetPosition();
			pOutResult.m_OwnerTM[0] = "1 0 0";
			pOutResult.m_OwnerTM[1] = "0 1 0";
			pOutResult.m_OwnerTM[2] = "0 0 1";
			pOutResult.m_OwnerTM[3] = adsOwnPos;
			pOutResult.m_bOwnerTMOverride = true;
			Math3D.YawPitchRollMatrix(m_OutCamAngles, pOutResult.m_CameraTM);
			pOutResult.m_CameraTM[3] = m_OutCamPos - adsOwnPos;
			pOutResult.m_fIgnoreParentRoll  = 1.0;
			pOutResult.m_fIgnoreParentPitch = 1.0;
			pOutResult.m_fIgnoreParentYaw   = 1.0;
			pOutResult.m_bUpdateEveryFrame = ADS_UPDATE_EVERY_FRAME;
		}
		else if (m_Target != null)
		{
			vector baseTM[4];
			m_pPlayer.GetTransform(baseTM);

			vector camWorld[4];
			Math3D.YawPitchRollMatrix(m_OutCamAngles, camWorld);

			pOutResult.m_CameraTM[0] = camWorld[0].InvMultiply3(baseTM);
			pOutResult.m_CameraTM[1] = camWorld[1].InvMultiply3(baseTM);
			pOutResult.m_CameraTM[2] = camWorld[2].InvMultiply3(baseTM);
			pOutResult.m_CameraTM[3] = m_OutCamPos.InvMultiply4(baseTM);

			pOutResult.m_bOwnerTMOverride    = false;
			//full composition — that is exactly what the inverse above is for
			pOutResult.m_fIgnoreParentRoll   = 0.0;
			pOutResult.m_fIgnoreParentPitch  = 0.0;
			pOutResult.m_fIgnoreParentYaw    = 0.0;
			pOutResult.m_bUpdateEveryFrame   = FOLLOW_UPDATE_EVERY_FRAME;

			if (VPPSpectateConstants.ADS_DEBUG)
			{
				int nowFol = g_Game.GetTime();
				if (m_DbgFolBurstLeft <= 0 && nowFol - m_DbgFolLastBurst > 10000)
				{
					m_DbgFolLastBurst = nowFol;
					m_DbgFolBurstLeft = 60;
					DbgADS("folSRC: burst start view=" + m_View.ToString() + " updEveryFrame=" + FOLLOW_UPDATE_EVERY_FRAME.ToString() + " extrap=" + g_Game.IsPhysicsExtrapolationEnabled().ToString());
				}
				if (m_DbgFolBurstLeft > 0)
				{
					m_DbgFolBurstLeft = m_DbgFolBurstLeft - 1;
					vector folHead = m_Target.GetBonePositionWS(m_HeadBoneIdx);
					float folBody = m_Target.GetOrientation()[0];
					if (folBody > 180.0)
						folBody = folBody - 360.0;
					float folYaw = m_OutCamAngles[0];
					if (folYaw > 180.0)
						folYaw = folYaw - 360.0;
					int folDtMs = nowFol - m_DbgFolPrevTime;
					float folDHead = 0.0;
					float folDCam = 0.0;
					float folDBody = 0.0;
					float folDYaw = 0.0;
					//continuity guard: only difference against the PREVIOUS RENDERED
					//frame (a view switch or hitch would otherwise fake one huge delta)
					if (folDtMs < 200)
					{
						folDHead = vector.Distance(folHead, m_DbgFolPrevHead);
						folDCam = vector.Distance(m_OutCamPos, m_DbgFolPrevOut);
						folDBody = folBody - m_DbgFolPrevBody;
						if (folDBody > 180.0)
							folDBody = folDBody - 360.0;
						if (folDBody < -180.0)
							folDBody = folDBody + 360.0;
						folDYaw = folYaw - m_DbgFolPrevYaw;
						if (folDYaw > 180.0)
							folDYaw = folDYaw - 360.0;
						if (folDYaw < -180.0)
							folDYaw = folDYaw + 360.0;
					}
					m_DbgFolPrevTime = nowFol;
					m_DbgFolPrevHead = folHead;
					m_DbgFolPrevOut = m_OutCamPos;
					m_DbgFolPrevBody = folBody;
					m_DbgFolPrevYaw = folYaw;
					string folLine = "folBURST: view=" + m_View.ToString() + " dtms=" + folDtMs.ToString() + " dHead=" + folDHead.ToString() + " dCam=" + folDCam.ToString();
					folLine = folLine + " body=" + folBody.ToString() + " dBody=" + folDBody.ToString() + " dYaw=" + folDYaw.ToString();
					DbgADS(folLine);
				}
			}
		}
		else
		{
			//freeze-frame (target gone, body unlinked/orphaned): no live base to
			//invert against — pass the cached world transform through the
			//identity-owner override instead (proven-aligned static recipe)
			vector ownPos = m_pPlayer.GetPosition();
			pOutResult.m_OwnerTM[0] = "1 0 0";
			pOutResult.m_OwnerTM[1] = "0 1 0";
			pOutResult.m_OwnerTM[2] = "0 0 1";
			pOutResult.m_OwnerTM[3] = ownPos;
			pOutResult.m_bOwnerTMOverride = true;
			Math3D.YawPitchRollMatrix(m_OutCamAngles, pOutResult.m_CameraTM);
			pOutResult.m_CameraTM[3] = m_OutCamPos - ownPos;
			pOutResult.m_fIgnoreParentRoll   = 1.0;
			pOutResult.m_fIgnoreParentPitch  = 1.0;
			pOutResult.m_fIgnoreParentYaw    = 1.0;
		}

		pOutResult.m_fDistance             = -0.075;
		pOutResult.m_fUseHeading           = 0.0;
		pOutResult.m_fShootFromCamera      = 0.0;
		pOutResult.m_iDirectBone           = -1;
		pOutResult.m_iDirectBoneMode       = 0;
		pOutResult.m_fPositionModelSpace   = 1.0;
		pOutResult.m_bUpdateWhenBlendOut   = false;
		pOutResult.m_CollisionIgnoreEntity = m_pPlayer;

		pOutResult.m_fNearPlane   = m_NearPlaneCurrent;
		pOutResult.m_fFovAbsolute = m_FovCurrent; //always set — never leave the -1 default

		//inside LOD in 1PP/ADS (VPPBR's reticle-proven config), outside in 3PP
		if (m_View == EVPPSpectateView.FIRST_PERSON)
			pOutResult.m_fInsideCamera = 1.0;
		else
			pOutResult.m_fInsideCamera = 0.0;

		//alignment probe + linked-transform semantics capture: tmYaw (GetTransform)
		//vs bodyYaw (GetOrientation) vs the body's distance to the target tell us
		//exactly what a linked entity reports — needed to ever trust base-relative
		//composition again
		if (VPPSpectateConstants.ADS_DEBUG)
		{
			int now = g_Game.GetTime();
			if (now - m_DbgLastEmitLog > 3000)
			{
				m_DbgLastEmitLog = now;
				float bodyYaw = m_pPlayer.GetOrientation()[0];
				vector probeTM[4];
				m_pPlayer.GetTransform(probeTM);
				vector probeAng = Math3D.MatrixToAngles(probeTM);
				string tgtInfo = "none";
				string distInfo = "none";
				if (m_Target != null)
				{
					tgtInfo = m_Target.GetOrientation()[0].ToString() + "+" + m_Target.GetAimingAngleLR().ToString();
					distInfo = vector.Distance(m_pPlayer.GetPosition(), m_Target.GetPosition()).ToString();
				}
				//physDelta: visual (linked) position vs PHYSICS/sim position of the
				//body — a persistent nonzero delta is the composition-base mismatch
				//behind the heading-dependent ADS misalignment
				float physDelta = vector.Distance(m_pPlayer.GetPosition(), m_pPlayer.PhysicsGetPositionWS());
				DbgADS("emit: view=" + m_View.ToString() + " outYaw=" + m_OutCamAngles[0].ToString() + " bodyYaw=" + bodyYaw.ToString() + " tmYaw=" + probeAng[0].ToString() + " tgtYaw=" + tgtInfo + " bodyDist=" + distInfo + " physDelta=" + physDelta.ToString());
			}
		}

		m_LastCamPosWS  = m_OutCamPos;
		m_LastCamAngles = m_OutCamAngles;
		m_HasLastFrame  = true;
	}

	//---------------------------------------------------------------------------------
	protected void SeedFromTarget()
	{
		if (m_Target == null)
			return;

		//seed the smoothing state from the target's current pose — no swing-in.
		//1PP tracks orientation + aim LR (matching UpdateFirstPerson's target),
		//3PP tracks bare orientation — seed accordingly to avoid an exit-yaw pop.
		float seedYaw = m_Target.GetOrientation()[0];
		if (m_View == EVPPSpectateView.FIRST_PERSON)
			seedYaw = seedYaw + m_Target.GetAimingAngleLR();

		m_YawCurrent    = seedYaw;
		m_YawVelocity   = 0.0;
		m_PitchCurrent  = m_Target.GetAimingAngleUD();
		m_PitchVelocity = 0.0;
		ResetOrbit();

		m_SmoothedHeadPos = m_Target.GetBonePositionWS(m_HeadBoneIdx);
		m_PosVelX[0] = 0.0;
		m_PosVelY[0] = 0.0;
		m_PosVelZ[0] = 0.0;
	}

	void ResetOrbit()
	{
		m_OrbitYaw   = 0.0;
		m_OrbitPitch = 0.0;
	}

	//VPPBR-proven velocity + acceleration angle filter (360-degree wrap safe).
	//coef <= 0 uses the default follow coefficient.
	protected float SmoothAngle(float current, float target, inout float velocity, float dt, float coef = 0)
	{
		if (coef <= 0)
			coef = VPPSpectateConstants.CAM_SMOOTH_COEF;

		float prevVel = velocity;

		if (current > 180.0)
			current = current - 360.0;
		if (target > 180.0)
			target = target - 360.0;

		float diff = target - current;
		if (diff > 180.0)
			diff = diff - 360.0;
		if (diff < -180.0)
			diff = diff + 360.0;

		velocity = diff * coef;
		float accel = prevVel - velocity;

		current = current + (velocity * dt) + (accel * dt * dt);

		//return in SIGNED range (-180..180]. NormalizeAngle alone yields [0..360),
		//which breaks the downstream pitch clamp (e.g. -10 deg would become 350).
		current = Math.NormalizeAngle(current);
		if (current > 180.0)
			current = current - 360.0;
		return current;
	}

	//mouse orbit (3PP). NOTE: the UAAim* inputs are DEAD while spectating (the
	//"aiming" input exclude covers exactly them), so orbit reads the VPP mouse-axis
	//inputs (UACamShift*, bound to mLeft/mRight/mUp/mDown) like the freecam does.
	protected void HandleOrbitInput(float dt)
	{
		//don't orbit while a menu/cursor is up (e.g. admin hud reopened mid-spectate)
		if (GetGame().GetUIManager().GetMenu() != null)
			return;

		Input input = GetGame().GetInput();
		float yawDiff   = input.LocalValue("UACamShiftLeft") - input.LocalValue("UACamShiftRight");
		float pitchDiff = input.LocalValue("UACamShiftDown") - input.LocalValue("UACamShiftUp");

		m_OrbitYaw   = m_OrbitYaw - (Math.RAD2DEG * yawDiff * dt);
		m_OrbitPitch = m_OrbitPitch - (Math.RAD2DEG * pitchDiff * dt);
		m_OrbitPitch = Math.Clamp(m_OrbitPitch, -80.0, 80.0);
	}

	//mouse-wheel zoom (3PP) via the existing VPP cam-speed inputs
	protected void HandleZoomInput(float dt)
	{
		if (GetGame().GetUIManager().GetMenu() != null)
			return;

		Input input = GetGame().GetInput();
		if (input.LocalPress("UACamSpeedAdd", false))
			m_Dist = m_Dist - VPPSpectateConstants.CAM_3PP_ZOOM_STEP;
		if (input.LocalPress("UACamSpeedDeduct", false))
			m_Dist = m_Dist + VPPSpectateConstants.CAM_3PP_ZOOM_STEP;

		m_Dist = Math.Clamp(m_Dist, 0.75, VPPSpectateConstants.CAM_3PP_MAX_DIST);
	}

	//---------------------------------------------------------------------------------
	// ADS sight resolution (mode-aware vanilla priority, all off the REMOTE target)
	//---------------------------------------------------------------------------------
	protected void ClearSight()
	{
		ClearOpticVisuals();
		m_SightInHands = null;
		m_SightWeapon  = null;
		m_SightOptic   = null;
		m_SightEntity  = null;
		m_SightValid   = false;
	}

	protected void DbgADS(string msg)
	{
		if (VPPSpectateConstants.ADS_DEBUG)
			Print("[VPPADS] " + msg);
	}

	//-----------------------------------------------------------------------------
	// Optic visuals: make OUR instance of the target's optic behave as if it were
	// being used — the whole vanilla pipeline is script-gated, so it can be driven
	// for a remote entity (reddot texture, enter-optics state, scope-tube PPE).
	//-----------------------------------------------------------------------------
	protected void ApplyOpticVisuals()
	{
		ClearOpticVisuals(); //never stack two applications

		if (!m_SightValid || m_SightOptic == null)
		{
			bool hasOptic = (m_SightOptic != null);
			DbgADS("apply: skipped (valid=" + m_SightValid.ToString() + " optic=" + hasOptic.ToString() + ")");
			return;
		}

		m_VisualOptic  = m_SightOptic;
		m_VisualWeapon = m_SightWeapon;
		m_VisualsApplied = true;
		m_DbgFightLogged = false;

		//red dot / illuminated reticle: FORCED on — the remote optic's energy state
		//isn't reliably replicated, so the vanilla IsWorking() gate stays dark for
		//spectators. Also feeds the 2D reticle texture path (OnDrawOptics2D).
		m_VisualOptic.ShowReddot(true);

		DbgADS("apply: optic=" + m_VisualOptic.GetType() + " 2D=" + m_VisualOptic.IsUsingOptics2DModel().ToString() + " reddotIdx=" + m_VisualOptic.m_reddot_index.ToString() + " tex=" + m_VisualOptic.m_optic_sight_texture + " displayed=" + m_VisualOptic.m_reddot_displayed.ToString() + " working=" + m_VisualOptic.IsWorking().ToString());

		if (m_TargetSightMode != EVPPSpectateSightMode.OPTICS || m_VisualOptic.IsSightOnly())
		{
			DbgADS("apply: reddot-only (mode=" + m_TargetSightMode.ToString() + " sightOnly=" + m_VisualOptic.IsSightOnly().ToString() + ")");
			return;
		}

		//vanilla SwitchOptics parity: power the optic so energy-driven visuals react
		if (m_VisualOptic.HasEnergyManager() && !m_VisualOptic.GetCompEM().IsSwitchedOn())
		{
			m_VisualOptic.GetCompEM().SwitchOn();
			m_VisualSwitchedEM = true;
		}

		//mirror DayZPlayerImplement.SwitchOptics(enter):
		//native in-optics state + the hook that hides lens covers
		bool entered = m_VisualOptic.EnterOptics();
		m_VisualOptic.OnOpticEnter();
		m_VisualEnteredOptics = true;

		DbgADS("apply: EnterOptics=" + entered.ToString() + " IsInOptics=" + m_VisualOptic.IsInOptics().ToString() + " allowsDOF=" + m_VisualOptic.AllowsDOF().ToString() + " EMon=" + m_VisualSwitchedEM.ToString());

		if (m_VisualOptic.IsUsingOptics2DModel())
		{
			if (m_VisualWeapon)
				m_VisualWeapon.SetInvisibleRecursive(true);
			else
				m_VisualOptic.SetInvisibleRecursive(true);
			m_VisualModelsHidden = true;
		}

		//magnifying scopes: vanilla scope-tube mask + lens distortion + edge blur
		//(DayZPlayerCameraOptics.SetCameraPP magnifying branch; NV post kept off)
		if (!m_VisualOptic.AllowsDOF())
		{
			if (!m_RequesterADS)
				Class.CastTo(m_RequesterADS, PPERequesterBank.GetRequester(PPERequester_CameraADS));

			if (m_RequesterADS)
			{
				array<float> maskValues = new array<float>;
				array<float> lensValues = new array<float>;
				float gauss = 0.0;

				if (m_VisualOptic.GetOpticsPPMask() && m_VisualOptic.GetOpticsPPMask().Count() == 4)
					maskValues = m_VisualOptic.GetOpticsPPMask();
				if (m_VisualOptic.GetOpticsPPLens() && m_VisualOptic.GetOpticsPPLens().Count() == 4)
					lensValues = m_VisualOptic.GetOpticsPPLens();
				if (m_VisualOptic.GetOpticsPPBlur() != 0)
					gauss = m_VisualOptic.GetOpticsPPBlur();

				m_RequesterADS.Start();
				m_RequesterADS.SetValuesOptics(maskValues, lensValues, gauss);

				DbgADS("apply: PPE mask=" + maskValues.Count().ToString() + " lens=" + lensValues.Count().ToString() + " gauss=" + gauss.ToString());
			}
		}

		if (m_VisualWeapon)
			m_VisualWeapon.HideWeaponBarrel(true); //self-guards on optic && !AllowsDOF
	}

	protected void ClearOpticVisuals()
	{
		if (!m_VisualsApplied)
			return;
		m_VisualsApplied = false;

		DbgADS("clear: restoring optic visuals");

		//entity refs may already be auto-nulled (deleted mid-ADS) — the PPE stop
		//and flag resets must run regardless
		if (m_VisualOptic)
		{
			if (m_VisualEnteredOptics)
			{
				m_VisualOptic.ExitOptics();
				m_VisualOptic.OnOpticExit(); //restores the "hide"/cover selections
			}
			if (m_VisualSwitchedEM && m_VisualOptic.HasEnergyManager())
				m_VisualOptic.GetCompEM().SwitchOff();
			m_VisualOptic.ShowReddot(false); //back to remote-normal state
		}
		m_VisualEnteredOptics = false;
		m_VisualSwitchedEM = false;

		//restore the 2D-scope model hiding (back to remote-normal visible state)
		if (m_VisualModelsHidden)
		{
			if (m_VisualWeapon)
				m_VisualWeapon.SetInvisibleRecursive(false);
			else if (m_VisualOptic)
				m_VisualOptic.SetInvisibleRecursive(false);
			m_VisualModelsHidden = false;
		}

		//forced restore, not HideWeaponBarrel(false): the vanilla call re-checks the
		//attached optic and silently skips when the scope was detached mid-ADS
		if (m_VisualWeapon)
			m_VisualWeapon.VPPForceShowBarrel();

		m_VisualOptic  = null;
		m_VisualWeapon = null;

		if (m_RequesterADS)
			m_RequesterADS.Stop(); //OnStop resets the PP mask + material params
	}

	//the 2D scope overlay the engine should draw for the spectator right now, if
	//any (consumed by the modded PlayerBase.OnDrawOptics2D on the controlled player)
	static ItemOptics GetActive2DOverlayOptic()
	{
		if (s_ActiveInstance == null)
			return null;
		return s_ActiveInstance.Get2DOverlayOptic();
	}

	//diagnostics: logs the first engine poll of OnDrawOptics2D
	static void DbgMarkOverlayPoll()
	{
		if (!VPPSpectateConstants.ADS_DEBUG)
			return;
		if (!s_DbgOverlayPollLogged)
		{
			s_DbgOverlayPollLogged = true;
			Print("[VPPADS] OnDrawOptics2D IS polled by the engine (first call logged)");
		}
	}

	static void DbgMarkOverlayServe(ItemOptics optic)
	{
		if (!VPPSpectateConstants.ADS_DEBUG)
			return;
		int now = g_Game.GetTime();
		if (now - s_DbgLastServeLog > 2000)
		{
			s_DbgLastServeLog = now;
			Print("[VPPADS] OnDrawOptics2D serving spectate optic: " + optic.GetType());
		}
	}

	//diagnostics: throttled heartbeat logged only WHILE a spectate cam exists —
	//proves the engine keeps polling OnDrawOptics2D under this camera
	static void DbgOverlayHeartbeat(bool controlled)
	{
		if (!VPPSpectateConstants.ADS_DEBUG)
			return;
		if (s_ActiveInstance == null)
			return;
		int now = g_Game.GetTime();
		if (now - s_DbgLastHeartbeat > 2000)
		{
			s_DbgLastHeartbeat = now;
			Print("[VPPADS] OnDrawOptics2D poll during spectate, controlled=" + controlled.ToString());
		}
	}

	protected ItemOptics Get2DOverlayOptic()
	{
		if (m_View != EVPPSpectateView.FIRST_PERSON)
			return null;
		if (m_TargetSightMode != EVPPSpectateSightMode.OPTICS)
			return null;
		if (!m_SightValid || m_SightOptic == null)
			return null;
		if (m_Target == null || !m_Target.IsAlive())
			return null;
		if (!m_SightOptic.IsUsingOptics2DModel())
			return null;
		return m_SightOptic;
	}

	protected bool ResolveSight()
	{
		ClearSight();
		if (m_Target == null)
			return false;

		EntityAI inHands = m_Target.GetHumanInventory().GetEntityInHands();
		m_SightInHands = inHands;
		if (inHands == null)
			return false;

		m_SightWeapon = Weapon_Base.Cast(inHands);
		if (m_SightWeapon)
			m_SightOptic = m_SightWeapon.GetAttachedOptics();
		else
			m_SightOptic = ItemOptics.Cast(inHands); //handheld binocs/scope path (vanilla)

		vector posMS;
		vector dirMS;

		if (m_TargetSightMode == EVPPSpectateSightMode.OPTICS && m_SightOptic)
		{
			//vanilla optics camera: the optic's TRUE sight point, not its backup irons
			m_SightOptic.UseWeaponIronsightsOverride(false);
			m_SightOptic.GetCameraPoint(posMS, dirMS);
			m_SightEntity = m_SightOptic;
		}
		else if (m_TargetSightMode == EVPPSpectateSightMode.IRONSIGHTS)
		{
			if (m_SightOptic && m_SightOptic.HasWeaponIronsightsOverride())
			{
				//optic replaces the weapon's iron sight view with its own override point
				m_SightOptic.UseWeaponIronsightsOverride(true);
				m_SightOptic.GetCameraPoint(posMS, dirMS);
				m_SightEntity = m_SightOptic;
			}
			else if (m_SightWeapon)
			{
				int mi = m_SightWeapon.GetCurrentMuzzle();
				if (mi < 0 || mi >= m_SightWeapon.GetMuzzleCount())
					mi = 0; //remote muzzle index may not be synced
				m_SightWeapon.GetCameraPoint(mi, posMS, dirMS);
				m_SightEntity = m_SightWeapon;
			}
		}

		if (m_SightEntity == null)
		{
			DbgADS("resolve: no sight entity (mode=" + m_TargetSightMode.ToString() + " inHands=" + inHands.GetType() + ")");
			return false;
		}

		if (dirMS.Length() < 0.0001)
		{
			DbgADS("resolve: zero-length camera dir on " + m_SightEntity.GetType());
			return false; //no camera point configured on this item
		}

		m_SightCamPosMS = posMS;
		m_SightCamDirMS = dirMS.Normalized();
		m_SightValid = true;

		if (m_View == EVPPSpectateView.FIRST_PERSON)
		{
			ApplyADSNearPlane();
			ApplyOpticVisuals();
		}
		return true;
	}

	protected void ApplyADSNearPlane()
	{
		if (m_TargetSightMode == EVPPSpectateSightMode.OPTICS && m_SightOptic && !m_SightOptic.IsUsingOptics2DModel())
		{
			//3D optics carry a config near-plane override (scope housings)
			m_NearPlaneCurrent = Math.Max(m_SightOptic.GetNearPlaneValue(), VPPSpectateConstants.CAM_1PP_NEARPLANE);
		}
		else
		{
			m_NearPlaneCurrent = VPPSpectateConstants.CAM_1PP_NEARPLANE;
		}
	}

	protected float GetSightFOV()
	{
		if (m_TargetSightMode == EVPPSpectateSightMode.OPTICS && m_SightOptic)
		{
			//GetZoomInit is pure config -> remote-safe.
			//NOTE: the target's dialed zoom STEP is not synced; this is entry zoom.
			float f = m_SightOptic.GetZoomInit();
			if (f > 0.001)
				return f;
		}
		return GameConstants.DZPLAYER_CAMERA_FOV_IRONSIGHTS; //0.5236 rad (60 deg)
	}

	protected void SeedADS()
	{
		if (!m_SightValid)
			return;

		//seed from the weapon's sight point + line (the same source the per-frame
		//tracking uses for 3D sights — no first-frame swing)
		vector p0 = m_SightEntity.ModelToWorld(m_SightCamPosMS);
		vector p1 = m_SightEntity.ModelToWorld(m_SightCamPosMS + m_SightCamDirMS);
		vector dirWS = p1 - p0;
		dirWS.Normalize();
		vector rawAngles = dirWS.VectorToAngles();

		float yaw = rawAngles[0];
		float pitch = rawAngles[1];
		if (yaw > 180.0)
			yaw = yaw - 360.0;
		if (pitch > 180.0)
			pitch = pitch - 360.0;

		//2D scopes seed straight onto the streamed TRUE aim instead — the per-frame
		//tracking keeps the stream for them (weapon model hidden, fullscreen overlay)
		bool seedIs2D = false;
		if (m_TargetSightMode == EVPPSpectateSightMode.OPTICS && m_SightOptic && m_SightOptic.IsUsingOptics2DModel())
			seedIs2D = true;
		if (seedIs2D && HasFreshStreamedLook())
		{
			yaw   = m_StreamLookYaw;
			pitch = m_StreamLookPitch;
		}

		//3D sights: apply the same stream correction as the per-frame rigid path,
		//SNAPPED — the seed is already a snap point, and the per-frame low-pass
		//then continues from this exact value (no first-frame pop)
		if (!seedIs2D)
		{
			float seedErr = 0.0;
			float seedAlpha = 0.0;
			m_CorrYawApplied = ComputeStreamCorrTargetYaw(yaw, seedErr, seedAlpha);
			yaw = yaw + m_CorrYawApplied;
			if (yaw > 180.0)
				yaw = yaw - 360.0;
			if (yaw < -180.0)
				yaw = yaw + 360.0;
		}
		else
		{
			m_CorrYawApplied = 0.0;
		}

		m_YawCurrent    = yaw;
		m_YawVelocity   = 0.0;
		m_PitchCurrent  = pitch;
		m_PitchVelocity = 0.0;

		m_SmoothedHeadPos = p0;
		m_PosVelX[0] = 0.0;
		m_PosVelY[0] = 0.0;
		m_PosVelZ[0] = 0.0;
	}

	//---------------------------------------------------------------------------------
	protected void UpdateThirdPerson(float dt)
	{
		HandleZoomInput(dt);
		HandleOrbitInput(dt);

		//death grace: hold the current angles (a corpse reports junk
		//orientation) but keep the pivot tracking the body
		float yawT;
		float pitchT;
		if (m_Target.IsAlive())
		{
			yawT   = m_Target.GetOrientation()[0];
			pitchT = m_Target.GetAimingAngleUD();
		}
		else
		{
			yawT   = m_YawCurrent;
			pitchT = m_PitchCurrent;
		}

		m_YawCurrent   = SmoothAngle(m_YawCurrent, yawT, m_YawVelocity, dt);
		m_PitchCurrent = SmoothAngle(m_PitchCurrent, pitchT, m_PitchVelocity, dt);

		float finalYaw   = m_YawCurrent + m_OrbitYaw;
		float finalPitch = m_PitchCurrent + m_OrbitPitch;
		finalPitch = Math.Clamp(finalPitch, -VPPSpectateConstants.CAM_PITCH_CLAMP, VPPSpectateConstants.CAM_PITCH_CLAMP);

		vector pivot = m_Target.GetBonePositionWS(m_HeadBoneIdx) + Vector(0, VPPSpectateConstants.CAM_3PP_PIVOT_UP, 0);

		vector angles = Vector(finalYaw, finalPitch, 0);
		vector fwd    = angles.AnglesToVector();
		vector right  = vector.Up * fwd;

		vector camPos = pivot - (fwd * m_Dist) + (right * VPPSpectateConstants.CAM_3PP_SHOULDER);

		Object ignoreObj = m_Target;
		HumanCommandVehicle hcv = m_Target.GetCommand_Vehicle();
		if (hcv && hcv.GetTransport())
			ignoreObj = hcv.GetTransport();

		vector hitPos;
		vector hitNormal;
		int hitComp;
		set<Object> results = new set<Object>;
		if (DayZPhysics.RaycastRV(pivot, camPos, hitPos, hitNormal, hitComp, results, null, ignoreObj, false, false))
		{
			camPos = hitPos + (fwd * 0.1);
			if (vector.Distance(pivot, camPos) < VPPSpectateConstants.CAM_3PP_MIN_DIST)
				camPos = pivot - (fwd * VPPSpectateConstants.CAM_3PP_MIN_DIST);
		}

		m_OutCamPos    = camPos;
		m_OutCamAngles = angles;
	}

	protected void UpdateFirstPerson(float dt)
	{
		//death grace: hold angles, keep following the (ragdolled) head bone
		float yawT;
		float pitchT;
		if (m_Target.IsAlive())
		{
			if (HasFreshStreamedLook())
			{
				//the target's TRUE camera direction (streamed from their client at
				//20Hz) — carries mouse-look and freelook, which never replicate;
				//the fallback below only knows body yaw + raised-weapon aim
				yawT   = m_StreamLookYaw;
				pitchT = m_StreamLookPitch;
			}
			else
			{
				yawT   = m_Target.GetOrientation()[0] + m_Target.GetAimingAngleLR();
				pitchT = m_Target.GetAimingAngleUD();
			}
		}
		else
		{
			yawT   = m_YawCurrent;
			pitchT = m_PitchCurrent;
		}

		m_YawCurrent   = SmoothAngle(m_YawCurrent, yawT, m_YawVelocity, dt);
		m_PitchCurrent = SmoothAngle(m_PitchCurrent, pitchT, m_PitchVelocity, dt);

		vector angles = Vector(m_YawCurrent, m_PitchCurrent, 0);
		vector fwd    = angles.AnglesToVector();

		//per-axis SmoothCD kills bone micro-jitter while keeping natural walk bob
		vector headPos = m_Target.GetBonePositionWS(m_HeadBoneIdx);
		m_SmoothedHeadPos[0] = Math.SmoothCD(m_SmoothedHeadPos[0], headPos[0], m_PosVelX, VPPSpectateConstants.CAM_1PP_POS_SMOOTH, 1000, dt);
		m_SmoothedHeadPos[1] = Math.SmoothCD(m_SmoothedHeadPos[1], headPos[1], m_PosVelY, VPPSpectateConstants.CAM_1PP_POS_SMOOTH, 1000, dt);
		m_SmoothedHeadPos[2] = Math.SmoothCD(m_SmoothedHeadPos[2], headPos[2], m_PosVelZ, VPPSpectateConstants.CAM_1PP_POS_SMOOTH, 1000, dt);

		float camFwdOffset = VPPSpectateConstants.CAM_1PP_FWD_OFFSET;
		HumanMovementState	hms = new HumanMovementState();
		m_Target.GetMovementState(hms);
		if (hms.m_iMovement == DayZPlayerConstants.MOVEMENTIDX_SPRINT)
		{
			camFwdOffset = VPPSpectateConstants.CAM_1PP_FWD_OFFSET_SPRINT;
		}

		m_OutCamPos    = m_SmoothedHeadPos + (fwd * camFwdOffset) + Vector(0, VPPSpectateConstants.CAM_1PP_UP_OFFSET, 0);
		m_OutCamAngles = angles;
	}

	//ADS: camera on the target's weapon/optic sight line, all in world space
	protected void UpdateFirstPersonADS(float dt)
	{
		//re-resolution triggers: hands content changed, the optic was (de)attached,
		//or the resolved sight entity was DELETED (Enforce auto-nulls refs to deleted
		//entities, which makes the equality checks alone blind to deletion)
		EntityAI inHandsNow = m_Target.GetHumanInventory().GetEntityInHands();
		bool opticChanged = false;
		if (m_SightWeapon && m_SightWeapon.GetAttachedOptics() != m_SightOptic)
			opticChanged = true;
		if (inHandsNow != m_SightInHands || opticChanged || m_SightEntity == null)
		{
			bool wasValid = m_SightValid;
			ResolveSight();
			if (m_SightValid && !wasValid)
				SeedADS();
		}

		//no resolvable sight: plain 1PP this frame (server will edge NONE shortly anyway)
		if (!m_SightValid)
		{
			UpdateFirstPerson(dt);
			return;
		}

		//ADS sources: POSITION from the weapon's sight point (rides the rendered
		//model — same entity basis as the sight the admin sees); DIRECTION from the
		//weapon's own sight line for 3D sights, streamed TRUE aim for 2D scopes.
		bool sightIs2D = false;
		if (m_TargetSightMode == EVPPSpectateSightMode.OPTICS && m_SightOptic && m_SightOptic.IsUsingOptics2DModel())
			sightIs2D = true;

		vector p0 = m_SightEntity.ModelToWorld(m_SightCamPosMS);
		vector p1 = m_SightEntity.ModelToWorld(m_SightCamPosMS + m_SightCamDirMS);
		vector dirWS = p1 - p0;
		dirWS.Normalize();

		vector wpnAngles = dirWS.VectorToAngles();
		float wpnYaw = wpnAngles[0];
		float wpnPitch = wpnAngles[1];
		if (wpnYaw > 180.0)
			wpnYaw = wpnYaw - 360.0;
		if (wpnPitch > 180.0)
			wpnPitch = wpnPitch - 360.0;

		vector rawAngles;
		bool usedStream = false;
		if (sightIs2D && HasFreshStreamedLook())
		{
			rawAngles[0] = m_StreamLookYaw;
			rawAngles[1] = m_StreamLookPitch;
			rawAngles[2] = 0;
			usedStream = true;
		}
		else
		{
			rawAngles = wpnAngles;
		}

		float corrErr = 0.0;
		float corrAlpha = 0.0;
		float corrTarget = ComputeStreamCorrTargetYaw(wpnYaw, corrErr, corrAlpha);
		if (!sightIs2D)
		{
			float corrK = Math.Clamp(VPPSpectateConstants.CAM_ADS_SMOOTH_COEF * dt, 0.0, 1.0);
			m_CorrYawApplied = m_CorrYawApplied + ((corrTarget - m_CorrYawApplied) * corrK);
		}
		else
		{
			m_CorrYawApplied = 0.0;
		}

		if (VPPSpectateConstants.ADS_DEBUG)
		{
			string dbgSrc = "rigid";
			if (usedStream)
				dbgSrc = "stream";
			if (sightIs2D && !usedStream)
				dbgSrc = "2Dfallback";
			int nowSrc = g_Game.GetTime();
			if (m_DbgBurstLeft <= 0 && nowSrc - m_DbgLastSrcLog > 10000)
			{
				m_DbgLastSrcLog = nowSrc;
				m_DbgBurstLeft = 60;
				DbgADS("adsSRC: burst start src=" + dbgSrc + " updEveryFrame=" + ADS_UPDATE_EVERY_FRAME.ToString() + " extrap=" + g_Game.IsPhysicsExtrapolationEnabled().ToString());
			}
			if (m_DbgBurstLeft > 0)
			{
				m_DbgBurstLeft = m_DbgBurstLeft - 1;
				float prbYaw = rawAngles[0];
				if (prbYaw > 180.0)
					prbYaw = prbYaw - 360.0;
				float prbBody = m_Target.GetOrientation()[0];
				if (prbBody > 180.0)
					prbBody = prbBody - 360.0;
				float prbDY = 0.0;
				float prbDB = 0.0;
				//continuity guard: only difference against the PREVIOUS RENDERED frame
				//(an ADS exit/re-enter inside a burst would otherwise fake one huge delta)
				if (nowSrc - m_DbgBurstPrevTime < 200)
				{
					prbDY = prbYaw - m_DbgBurstPrevYaw;
					if (prbDY > 180.0)
						prbDY = prbDY - 360.0;
					if (prbDY < -180.0)
						prbDY = prbDY + 360.0;
					prbDB = prbBody - m_DbgBurstPrevBody;
					if (prbDB > 180.0)
						prbDB = prbDB - 360.0;
					if (prbDB < -180.0)
						prbDB = prbDB + 360.0;
				}
				m_DbgBurstPrevTime = nowSrc;
				m_DbgBurstPrevYaw = prbYaw;
				m_DbgBurstPrevBody = prbBody;
				string prbLine = "adsBURST: src=" + dbgSrc + " dt=" + dt.ToString() + " yaw=" + prbYaw.ToString() + " dYaw=" + prbDY.ToString();
				prbLine = prbLine + " body=" + prbBody.ToString() + " dBody=" + prbDB.ToString();
				prbLine = prbLine + " strYaw=" + m_StreamLookYaw.ToString() + " strPitch=" + m_StreamLookPitch.ToString() + " wpnPitch=" + wpnPitch.ToString();
				prbLine = prbLine + " err=" + corrErr.ToString() + " alpha=" + corrAlpha.ToString() + " corr=" + m_CorrYawApplied.ToString();
				DbgADS(prbLine);
			}
		}

		if (sightIs2D)
		{
			m_YawCurrent   = SmoothAngle(m_YawCurrent, rawAngles[0], m_YawVelocity, dt, VPPSpectateConstants.CAM_ADS_SMOOTH_COEF);
			m_PitchCurrent = SmoothAngle(m_PitchCurrent, rawAngles[1], m_PitchVelocity, dt, VPPSpectateConstants.CAM_ADS_SMOOTH_COEF);
			m_SmoothedHeadPos[0] = Math.SmoothCD(m_SmoothedHeadPos[0], p0[0], m_PosVelX, VPPSpectateConstants.CAM_ADS_POS_SMOOTH, 1000, dt);
			m_SmoothedHeadPos[1] = Math.SmoothCD(m_SmoothedHeadPos[1], p0[1], m_PosVelY, VPPSpectateConstants.CAM_ADS_POS_SMOOTH, 1000, dt);
			m_SmoothedHeadPos[2] = Math.SmoothCD(m_SmoothedHeadPos[2], p0[2], m_PosVelZ, VPPSpectateConstants.CAM_ADS_POS_SMOOTH, 1000, dt);

			m_OutCamPos    = m_SmoothedHeadPos;
			m_OutCamAngles = Vector(m_YawCurrent, m_PitchCurrent, 0);
		}
		else
		{
			float rigYaw   = wpnYaw;
			float rigPitch = wpnPitch;

			rigYaw = rigYaw + m_CorrYawApplied;
			if (rigYaw > 180.0)
				rigYaw = rigYaw - 360.0;
			if (rigYaw < -180.0)
				rigYaw = rigYaw + 360.0;

			m_YawCurrent      = rigYaw;
			m_YawVelocity     = 0.0;
			m_PitchCurrent    = rigPitch;
			m_PitchVelocity   = 0.0;
			m_SmoothedHeadPos = p0;
			m_PosVelX[0] = 0.0;
			m_PosVelY[0] = 0.0;
			m_PosVelZ[0] = 0.0;

			vector adsRight = vector.Up * dirWS;
			if (adsRight.LengthSq() < 0.000001)
			{
				//PRE-correction yaw: the trim basis must never depend on alpha
				vector adsFallbackAngles = Vector(wpnYaw + 90.0, 0, 0);
				adsRight = adsFallbackAngles.AnglesToVector();
			}
			adsRight.Normalize();
			vector adsUp = dirWS * adsRight;
			adsUp.Normalize();
			m_OutCamPos    = p0 - (dirWS * VPPSpectateConstants.CAM_ADS_BACK_OFFSET) + (adsRight * VPPSpectateADSTuning.s_RightOffset) + (adsUp * VPPSpectateADSTuning.s_UpOffset);
			m_OutCamAngles = Vector(rigYaw, rigPitch, 0);
		}

		if (m_VisualsApplied && m_VisualOptic)
		{
			if (!m_VisualOptic.m_reddot_displayed)
			{
				if (!m_DbgFightLogged)
				{
					DbgADS("upkeep: reddot flipped off externally - re-forcing (fight detected)");
					m_DbgFightLogged = true;
				}
				m_VisualOptic.ShowReddot(true);
			}
			if (m_VisualEnteredOptics && !m_VisualOptic.IsInOptics())
			{
				if (!m_DbgFightLogged)
				{
					DbgADS("upkeep: native optics state dropped - re-entering (fight detected)");
					m_DbgFightLogged = true;
				}
				m_VisualOptic.EnterOptics();
			}
		}
	}

	//single FOV path for ALL modes and ALL exits (view toggle, target switch,
	//ADS end, death, weapon dropped) — blends toward the correct target each frame
	protected void UpdateFOV(float dt)
	{
		float fovTarget = g_Game.GetUserFOV();

		if (m_View == EVPPSpectateView.FIRST_PERSON && m_TargetSightMode != EVPPSpectateSightMode.NONE && m_SightValid && m_Target.IsAlive())
			fovTarget = GetSightFOV();

		m_FovCurrent = Math.SmoothCD(m_FovCurrent, fovTarget, m_FovVel, VPPSpectateConstants.CAM_ADS_FOV_SMOOTH, 1000, dt);
	}
};
