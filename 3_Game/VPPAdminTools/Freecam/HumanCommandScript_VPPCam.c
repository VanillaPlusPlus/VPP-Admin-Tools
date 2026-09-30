class HumanCommandScript_VPPCam : HumanCommandScript
{
	DayZPlayer 				m_pPlayer;
	HumanInputController 	m_Input;
	bool					m_bNeedFinish;

	//! the engine calls this constructor (1st parameter must be the Human)
	void HumanCommandScript_VPPCam(Human pHuman)
	{
		m_pPlayer = DayZPlayer.Cast(pHuman);
		if (m_pPlayer)
		{
			m_Input = m_pPlayer.GetInputController();
		}
	}

	override void OnActivate()
	{
		if (!m_pPlayer || !m_Input)
		{
			return;
		}

		dBodyEnableGravity(m_pPlayer, false);
		m_Input.SetDisabled(true);

		GetGame().GetMission().AddActiveInputExcludes({"movement", "aiming", "menu"});
	}

	override void OnDeactivate()
	{
		if (!m_pPlayer || !m_Input)
		{
			return;
		}

		dBodyEnableGravity(m_pPlayer, true);
		m_Input.SetDisabled(false);

		if (!GetGame().IsDedicatedServer())
		{
			GetGame().GetMission().PlayerControlEnable(true);
			GetGame().GetUIManager().ShowUICursor(false);
			GetGame().GetMission().RemoveActiveInputExcludes({"menu", "movement", "aiming"}, true);
			GetGame().GetMission().RefreshExcludes();
		}
	}

	//! called to set values to animation graph processing
	override void PreAnimUpdate(float pDt)
	{
	}

	override void PrePhysUpdate(float pDt)
	{
		vector trans = vector.Zero;
		PrePhys_SetTranslation(trans);
	}

	//! called when all animation / pre phys update is handled
	override bool PostPhysUpdate(float pDt)
	{
		if (m_bNeedFinish)
		{
			SetFlagFinished(true);
			if (m_Input)
			{
				m_Input.SetDisabled(false);
			}

			return false;
		}
		return true;
	}
}
