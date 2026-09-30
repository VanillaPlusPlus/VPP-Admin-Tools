/*
	Context menu input: tuning constants + look-mode input capture.

	The capture only ever removes what it added itself (tracked flags), so excludes owned
	by other systems (freecam "movement"/"aiming"/"menu", spectate, gestures) survive.
*/
class VPPContextMenuTuning
{
	const static float  LOOK_OPEN_DELAY           = 0.20;
	const static float  LOOK_NAV_STEP             = 0.08;
	const static float  LOOK_NAV_MAX_ACCUM        = 0.24;
	const static float  LOOK_NAV_DEADZONE         = 0.0001;
	const static float  LOOK_NAV_IDLE_RESET       = 0.25;   //seconds of |d| <= deadzone before the accumulator resets to 0
	const static bool   LOOK_NAV_INVERT           = false;
	const static bool   LOOK_NAV_DEBUG            = false;
	const static bool   LOOK_ALLOW_STATIC_OBJECTS = false;  //true = static map props (trees, rocks) also arm look-mode; the 1000m pick almost always hits one, so RMB would rarely reach vanilla aim
	const static float  PICK_DISTANCE             = 1000.0;
	const static float  CONFIRM_WINDOW            = 3.0;
	const static int    TOGGLE_RECONCILE_MS       = 700;
	const static string INPUT_MENU                = "UAVPPContextMenu";
	const static string INPUT_SELECT              = "UAVPPContextSelect";
	const static string INPUT_NAV_UP              = "UAVPPCtxNavUp";
	const static string INPUT_NAV_DOWN            = "UAVPPCtxNavDown";
	const static string INPUT_WHEEL_UP            = "UAVPPCtxWheelUp";
	const static string INPUT_WHEEL_DOWN          = "UAVPPCtxWheelDown";
	const static string INPUT_BACK                = "UAVPPCtxBack";
	const static string INPUT_FORWARD             = "UAVPPCtxForward";
	const static string INPUT_ARROW_UP            = "UAUPCommand";
	const static string INPUT_ARROW_DOWN          = "UADOWNCommand";
	const static string INPUT_ENTER               = "UAExecuteCommand";
	const static bool   LOOK_ENABLED_DEFAULT      = false;  //first-run state of the hold-to-open look menu; the admin's toggle is saved in the profile afterwards
	const static string PROFILE_LOOK_ENABLED      = "vppat_ctxmenu_look";
};

class VPPContextInputCapture : Managed
{
	protected bool m_Active;
	protected bool m_AddedVanilla;
	protected bool m_AddedOwn;
	protected bool m_RaiseForced;
	protected bool m_RaiseOverridden;
	protected bool m_LockedKeybinds;
	protected int  m_SelfCheckFrames;

	protected static bool s_SelfCheckLogged;

	void Begin()
	{
		if (m_Active)
			return;

		Mission mission = GetGame().GetMission();
		if (!mission)
			return;

		if (!mission.IsInputExcludeActive("radialmenu"))
		{
			mission.AddActiveInputExcludes({"radialmenu"});
			m_AddedVanilla = true;
		}

		if (!mission.IsInputExcludeActive("VPPContextMenu"))
		{
			mission.AddActiveInputExcludes({"VPPContextMenu"});
			m_AddedOwn = true;
		}

		//excludes apply one frame late; ForceDisable + OverrideRaise stop the RMB raise right away
		UAInput raise = GetUApi().GetInputByID(UATempRaiseWeapon);
		if (raise)
		{
			raise.ForceDisable(true);
			m_RaiseForced = true;
		}

		HumanInputController hic = GetPlayerInputController();
		if (hic)
		{
			hic.OverrideRaise(HumanInputControllerOverrideType.ENABLED, false);
			m_RaiseOverridden = true;
		}

		VPPContextMenuState.SetCapturing(true);

		VPPUIManager uiMgr = GetVPPUIManager();
		if (uiMgr && !uiMgr.GetKeybindsStatus())
		{
			uiMgr.SetKeybindsStatus(true);
			m_LockedKeybinds = true;
		}

		m_SelfCheckFrames = 3;
		m_Active = true;
	}

	void Keep()
	{
		if (!m_Active)
			return;

		//EnableAllInputs or a closing gestures/quickbar menu can drop the groups we added
		Mission mission = GetGame().GetMission();
		if (mission)
		{
			if (m_AddedVanilla && !mission.IsInputExcludeActive("radialmenu"))
				mission.AddActiveInputExcludes({"radialmenu"});

			if (m_AddedOwn && !mission.IsInputExcludeActive("VPPContextMenu"))
				mission.AddActiveInputExcludes({"VPPContextMenu"});
		}

		HumanInputController hic = GetPlayerInputController();
		if (hic)
		{
			hic.OverrideRaise(HumanInputControllerOverrideType.ENABLED, false);
			m_RaiseOverridden = true;
		}

		if (m_SelfCheckFrames > 0)
		{
			m_SelfCheckFrames--;
			if (m_SelfCheckFrames == 0 && !s_SelfCheckLogged)
			{
				LogSelfCheck();
				s_SelfCheckLogged = true;
			}
		}
	}

	void End()
	{
		if (!m_Active)
			return;

		Mission mission = GetGame().GetMission();
		if (mission)
		{
			if (m_AddedVanilla)
				mission.RemoveActiveInputExcludes({"radialmenu"}, true);

			if (m_AddedOwn)
				mission.RemoveActiveInputExcludes({"VPPContextMenu"}, true);
		}

		//never lift the vanilla surrender-transition raise block
		if (m_RaiseForced)
		{
			bool surrenderActive = false;
			if (mission && mission.IsInputRestrictionActive(EInputRestrictors.SURRENDER_TRANSITION))
				surrenderActive = true;

			if (!surrenderActive)
			{
				UAInput raise = GetUApi().GetInputByID(UATempRaiseWeapon);
				if (raise)
					raise.ForceDisable(false);
			}
		}

		if (m_RaiseOverridden)
		{
			HumanInputController hic = GetPlayerInputController();
			if (hic)
				hic.OverrideRaise(HumanInputControllerOverrideType.DISABLED, false);
		}

		SupressInput(UAFire);
		SupressInput(UADefaultAction);
		SupressInput(UATempRaiseWeapon);

		VPPContextMenuState.SetCapturing(false);

		if (m_LockedKeybinds && GetVPPUIManager())
			GetVPPUIManager().SetKeybindsStatus(false);

		m_Active          = false;
		m_AddedVanilla    = false;
		m_AddedOwn        = false;
		m_RaiseForced     = false;
		m_RaiseOverridden = false;
		m_LockedKeybinds  = false;
		m_SelfCheckFrames = 0;
	}

	bool IsActive()
	{
		return m_Active;
	}

	protected HumanInputController GetPlayerInputController()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
			return null;

		return player.GetInputController();
	}

	protected void SupressInput(int inputId)
	{
		UAInput uaInput = GetUApi().GetInputByID(inputId);
		if (uaInput)
			uaInput.Supress();
	}

	//modded <exclude> groups are unproven at runtime: log once whether they actually locked
	protected void LogSelfCheck()
	{
		bool fireLocked = false;
		UAInput fireInput = GetUApi().GetInputByID(UAFire);
		if (fireInput)
			fireLocked = fireInput.IsLocked();

		bool nextLocked = false;
		UAInput nextInput = GetUApi().GetInputByID(UANextAction);
		if (nextInput)
			nextLocked = nextInput.IsLocked();

		string msg = "[VPPContextMenu] exclude self-check: UAFire locked=";
		msg += BoolText(fireLocked);
		msg += " UANextAction locked=";
		msg += BoolText(nextLocked);
		Print(msg);
	}

	protected string BoolText(bool state)
	{
		if (state)
			return "true";

		return "false";
	}
};
