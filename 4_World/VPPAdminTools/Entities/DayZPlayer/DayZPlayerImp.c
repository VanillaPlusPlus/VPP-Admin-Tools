class HumanCommandScript_VPPCameraMode
{
	void HumanCommandScript_VPPCameraMode(Human pHuman)
	{
		HumanAnimInterface 	hai = pHuman.GetAnimInterface();
	
		m_VarSpeed			= hai.BindVariableFloat("MovementSpeed");
		m_VarDirection		= hai.BindVariableFloat("MovementDirection");
	}	

	int 					m_VarSpeed;				//! float variable - 0,1,2,3 speed
	int 					m_VarDirection;			//! float variable - angle 
}

class HumanCommandScript_VPPCam : HumanCommandScript
{
	void 	HumanCommandScript_VPPCam(DayZPlayer pPlayer, HumanCommandScript_VPPCameraMode pTable)
	{
		m_pPlayer 	= pPlayer;
		m_Table 	= pTable;
		m_Input		= pPlayer.GetInputController();
	}

	//! 
	override void 	OnActivate()
	{
		dBodyEnableGravity(m_pPlayer, false);
		DayZPlayerImplement.Cast(m_pPlayer).m_pGravtiyToggle = false;
		m_Input.SetDisabled(true);

		GetGame().GetMission().AddActiveInputExcludes({"movement", "aiming", "menu"});
	}

	override void 	OnDeactivate()
	{
		dBodyEnableGravity(m_pPlayer, true);
		DayZPlayerImplement.Cast(m_pPlayer).m_pGravtiyToggle = true;
		m_Input.SetDisabled(false);

		if (!GetGame().IsDedicatedServer())
		{
	        GetGame().GetMission().PlayerControlEnable(true);
	        GetGame().GetUIManager().ShowUICursor(false);
	        GetGame().GetMission().RemoveActiveInputExcludes({"menu", "movement", "aiming"},true);
	        GetGame().GetMission().RefreshExcludes();
		}
	}

	// called to set values to animation graph processing 
	override void 	PreAnimUpdate(float pDt)
	{
		//float 	heading;
		//heading = m_Input.GetHeadingAngle();
		//PreAnim_SetFilteredHeading(heading, 0.1, 360);
	}
	
	override void 	PrePhysUpdate(float pDt)
	{
		vector trans = vector.Zero;
		PrePhys_SetTranslation(trans);
	}

	//! called when all animation / pre phys update is handled
	override bool	PostPhysUpdate(float pDt)
	{
		if (m_bNeedFinish)
		{
			SetFlagFinished(true);
			m_Input.SetDisabled(false);
			return false;
		}
		return true;
	}

	DayZPlayer 							m_pPlayer;
	HumanCommandScript_VPPCameraMode 	m_Table;
	HumanInputController 				m_Input;
	bool								m_bNeedFinish;
}

// *************************************************************************************
// ! DayZPlayerImplementScriptCommand - this is called by Player's Command Handler
// ! purpose: to start scripted swimming command 
// *************************************************************************************
modded class DayZPlayerImplement
{
	ref HumanCommandScript_VPPCameraMode m_VPPCamHmnCmd;
	bool m_pGravtiyToggle = true; //when true, has gravity

	void DayZPlayerImplement()
	{
	}
	
	void InitTablesCmd()
	{
		if (m_VPPCamHmnCmd == null)
		{
			m_VPPCamHmnCmd = new HumanCommandScript_VPPCameraMode(this);
		}
	}

	override bool ModCommandHandlerInside(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (super.ModCommandHandlerInside(pDt, pCurrentCommandID, pCurrentCommandFinished))
		{
			return true;
		}

		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_SCRIPT)
		{
			HumanCommandScript hcs = GetCommand_Script();
			if (HumanCommandScript_VPPCam.Cast(hcs) != null)
			{
				return true;
			}
			return false;
		}
		return false;
	}

	override bool ModCommandHandlerBefore(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (super.ModCommandHandlerBefore(pDt, pCurrentCommandID, pCurrentCommandFinished))
		{
			return true;
		}

		if (pCurrentCommandFinished && pCurrentCommandID == DayZPlayerConstants.COMMANDID_SCRIPT)
		{
			HumanCommandScript hcs = GetCommand_Script();
			if (HumanCommandScript_VPPCam.Cast(hcs))
			{
				//handled
				StartCommand_Move();
				return true;
			}
		}

		//skip falling when gravity disabled.
		if (PhysicsIsFalling(true) && !m_pGravtiyToggle)
		{
			return true; //skip falling when gravity disabled.
		}
		return false;
	}
};