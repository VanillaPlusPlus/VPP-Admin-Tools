// The freecam command (HumanCommandScript_VPPCam) lives in 3_Game/VPPAdminTools/Freecam: it must share a script
// module with its native base HumanCommandScript (see the note there).

// *************************************************************************************
// ! DayZPlayerImplementScriptCommand - this is called by Player's Command Handler
// ! purpose: to start scripted swimming command
// *************************************************************************************
modded class DayZPlayerImplement
{
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

		if (pCurrentCommandID != DayZPlayerConstants.COMMANDID_SCRIPT)
		{
			return false;
		}

		HumanCommandScript activeCmd = GetCommand_Script();
		if (!HumanCommandScript_VPPCam.Cast(activeCmd))
		{
			return false;
		}

		if (pCurrentCommandFinished)
		{
			//handled
			StartCommand_Move();
			return true;
		}

		//the freecam command turned gravity off: skip falling while it holds the body
		if (PhysicsIsFalling(true))
		{
			return true;
		}
		return false;
	}
};
