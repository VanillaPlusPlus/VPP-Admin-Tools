/*
	Context action menu controller (owned by MissionGameplay.m_VPPContextMenu).

	Runs from the GUI update queue whether or not a menu is open, and drives one
	workspace-root VPPContextMenuView in two modes:
	- LOOK   : tools on + look toggle on (UAVPPContextMenuToggle, saved per client), hold the context key on a crosshair target; no cursor, input captured.
	- CURSOR : OpenAt / OpenForPlayers from VPP menus; Windows-like popup at the cursor.
	Pages drill down through a frame stack; every row comes from VPPContextActionManager.

	Never toggles the mission-wide player control lock: re-enabling it would drop the freecam's own excludes.
*/
enum EVPPContextMenuPhase
{
	IDLE = 0,
	ARMED = 1,
	OPEN = 2,
	WAIT_RELEASE = 3
};

enum EVPPContextMenuMode
{
	NONE = 0,
	LOOK = 1,
	CURSOR = 2
};

class VPPContextMenuFrame : Managed
{
	ref VPPContextTarget Target;
	string ParentId;
	string Title;
	string TitleIcon;
	int    Focus;
};

class VPPContextMenuController : Managed
{
	protected ref VPPContextMenuView                m_View;
	protected ref VPPContextInputCapture            m_Capture;
	protected ref array<ref VPPContextMenuFrame>    m_Frames;
	protected ref VPPContextItems                   m_Display;

	protected int   m_Phase;
	protected int   m_Mode;
	protected bool  m_LookEnabled;

	protected float m_ArmTimer;
	protected ref VPPContextTarget m_ArmTarget;

	protected float m_NavAccum;
	protected float m_NavIdle;
	protected float m_NavDebugTimer;

	protected int   m_ConfirmIndex = -1;
	protected float m_ConfirmTimer;

	protected int   m_PrevMouseMask;
	protected bool  m_RequiresObject;

	protected ref array<string>    m_StateRequested;
	protected ref map<int,string>  m_PendingResults;

	protected ref VPPContextAction m_PromptAction;
	protected ref VPPContextTarget m_PromptTarget;
	protected ref VPPContextArgs   m_PromptArgs;
	protected VPPDialogBox        m_PromptDialog;   //weak: the dialog deletes itself
	protected bool m_PromptFromLook;
	protected bool m_PromptPending;
	protected bool m_PromptOpen;
	protected bool m_DialogAddedMenuExclude;
	protected bool m_DialogLockedKeybinds;
	protected bool m_DialogChangedFocus;

	protected VPPContextActionManager m_SubscribedMgr;   //weak

	protected static VPPContextMenuController s_Instance;   //weak

	void VPPContextMenuController()
	{
		m_Frames         = new array<ref VPPContextMenuFrame>;
		m_StateRequested = new array<string>;
		m_PendingResults = new map<int,string>;
		m_Display        = new VPPContextItems();
		m_Capture        = new VPPContextInputCapture();
		m_View           = new VPPContextMenuView(this);

		m_Phase = EVPPContextMenuPhase.IDLE;
		m_Mode  = EVPPContextMenuMode.NONE;

		m_LookEnabled = VPPContextMenuTuning.LOOK_ENABLED_DEFAULT;
		string savedLook;
		if (GetGame().GetProfileString(VPPContextMenuTuning.PROFILE_LOOK_ENABLED, savedLook))
		{
			if (savedLook == "1")
				m_LookEnabled = true;
			else if (savedLook == "0")
				m_LookEnabled = false;
		}

		s_Instance = this;

		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.DoUpdate);
		VPPAdminHud.m_OnPermissionsChanged.Insert(this.OnHudPermissionsChanged);
	}

	void ~VPPContextMenuController()
	{
		Shutdown();

		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
		VPPAdminHud.m_OnPermissionsChanged.Remove(this.OnHudPermissionsChanged);
		UnsubscribeManager();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.ReconcileState);

		if (s_Instance == this)
			s_Instance = null;
	}

	static VPPContextMenuController GetInstance()
	{
		return s_Instance;
	}

	/*
		Public API
	*/
	//cursor-mode prefab entry point for any menu / mod
	bool OpenAt(VPPContextTarget target, int x, int y)
	{
		if (!target)
			return false;

		if (m_Mode == EVPPContextMenuMode.LOOK && m_Phase != EVPPContextMenuPhase.IDLE)
			return false;

		//one prompt at a time: its callback resolves against the stored action/target
		if (m_PromptOpen || m_PromptPending)
			return false;

		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr || !mgr.HasAnyAction(target))
			return false;

		if (m_Phase == EVPPContextMenuPhase.OPEN)
			Close();

		m_Mode = EVPPContextMenuMode.CURSOR;
		OpenPage(target, false, x, y);
		m_PrevMouseMask = CurrentMouseMask();
		mgr.SendPermissionRequest(false);
		return true;
	}

	bool OpenForPlayers(array<string> steam64s, array<int> sessionIds, string primaryId, int primarySessionId, string primaryName, int x, int y)
	{
		VPPContextTarget target;
		if (steam64s && steam64s.Count() > 1)
			target = VPPContextTarget.ForPlayers(steam64s, sessionIds, primaryId, primarySessionId, primaryName, EVPPContextSource.PLAYER_LIST);
		else
			target = VPPContextTarget.ForPlayer(primaryId, primarySessionId, primaryName, EVPPContextSource.PLAYER_LIST);

		return OpenAt(target, x, y);
	}

	void Close()
	{
		if (m_Phase != EVPPContextMenuPhase.OPEN && m_Phase != EVPPContextMenuPhase.ARMED)
			return;

		if (m_Mode == EVPPContextMenuMode.LOOK)
		{
			EnterWaitRelease();
			return;
		}

		m_View.Show(false);
		DisarmConfirm();
		m_Frames.Clear();
		m_Phase = EVPPContextMenuPhase.IDLE;
		m_Mode  = EVPPContextMenuMode.NONE;
	}

	bool IsOpen()
	{
		return m_Phase == EVPPContextMenuPhase.OPEN;
	}

	bool IsLookEnabled()
	{
		return m_LookEnabled;
	}

	void SetLookEnabled(bool enabled)
	{
		m_LookEnabled = enabled;

		string lookValue = "0";
		if (enabled)
			lookValue = "1";

		GetGame().SetProfileString(VPPContextMenuTuning.PROFILE_LOOK_ENABLED, lookValue);
		GetGame().SaveProfile();

		//the held context key stays swallowed until release; no ForceReset
		if (!enabled && m_Mode == EVPPContextMenuMode.LOOK && (m_Phase == EVPPContextMenuPhase.ARMED || m_Phase == EVPPContextMenuPhase.OPEN))
			EnterWaitRelease();
	}

	//returns the new state
	bool ToggleLookEnabled()
	{
		SetLookEnabled(!m_LookEnabled);
		return m_LookEnabled;
	}

	int GetMode()
	{
		return m_Mode;
	}

	int GetPhase()
	{
		return m_Phase;
	}

	//called first thing in MissionGameplay.OnKeyPress; true = consumed
	bool OnKeyPress(int key)
	{
		if (!IsOpen())
			return false;

		if (key == KeyCode.KC_ESCAPE)
		{
			if (m_Frames.Count() > 1)
				PopFrame();
			else
				Close();

			return true;
		}
		return false;
	}

	void Shutdown()
	{
		if (m_Capture)
			m_Capture.End();

		if (m_PromptOpen && m_PromptFromLook)
			EndDialogHandoff();

		m_PromptOpen = false;
		ClearPrompt();

		if (m_View)
			m_View.Show(false);

		m_ConfirmIndex = -1;
		m_ConfirmTimer = 0;
		m_Phase = EVPPContextMenuPhase.IDLE;
		m_Mode  = EVPPContextMenuMode.NONE;
		m_ArmTarget = null;

		if (m_Frames)
			m_Frames.Clear();

		m_PromptPending = false;
	}

	void DoUpdate(float dt)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player)
		{
			if (m_Phase != EVPPContextMenuPhase.IDLE)
				ForceReset();

			return;
		}

		EnsureSubscribed();
		PromptWatchdog();

		if (m_Phase == EVPPContextMenuPhase.IDLE)
		{
			TryArmLook();
		}
		else if (m_Phase == EVPPContextMenuPhase.ARMED)
		{
			UpdateArmed(dt);
		}
		else if (m_Phase == EVPPContextMenuPhase.OPEN)
		{
			if (m_Mode == EVPPContextMenuMode.LOOK)
				UpdateLookOpen(dt);
			else
				UpdateCursorOpen(dt);
		}
		else if (m_Phase == EVPPContextMenuPhase.WAIT_RELEASE)
		{
			UpdateWaitRelease();
		}

		if (m_ConfirmTimer > 0)
		{
			m_ConfirmTimer -= dt;
			if (m_ConfirmTimer <= 0)
				DisarmConfirm();
		}
	}

	/*
		View callbacks (cursor mode only; in look mode the cursor is hidden)
	*/
	void OnViewRowHovered(int index)
	{
		if (m_Mode != EVPPContextMenuMode.CURSOR || !IsOpen())
			return;

		if (!IsFocusable(index) || index == m_View.GetFocusIndex())
			return;

		DisarmConfirm();
		m_View.SetFocusIndex(index);
	}

	//LMB and RMB both activate, as on Windows
	void OnViewRowClicked(int index, int button)
	{
		if (m_Mode != EVPPContextMenuMode.CURSOR || !IsOpen())
			return;

		if (IsFocusable(index) && index != m_View.GetFocusIndex())
		{
			DisarmConfirm();
			m_View.SetFocusIndex(index);
		}

		ActivateIndex(index);
	}

	void OnViewWheel(int direction)
	{
		if (m_Mode != EVPPContextMenuMode.CURSOR || !IsOpen())
			return;

		MoveFocus(direction);
	}

	/*
		Manager events
	*/
	void OnActionResult(int seq, string actionId, bool success, string messageKey, string messageArg, int newState)
	{
		//the manager has already shown the toast
		string targetKey = m_PendingResults.Get(seq);
		m_PendingResults.Remove(seq);
		if (targetKey == "")
			return;

		if (newState >= 0)
		{
			bool stateOn = false;
			if (newState == 1)
				stateOn = true;

			foreach (VPPContextMenuFrame stateFrame : m_Frames)
			{
				if (stateFrame && stateFrame.Target && stateFrame.Target.GetKey() == targetKey)
					stateFrame.Target.SetState(actionId, VPPContextUtils.BoolToState(stateOn));
			}

			//other rows of a kept-open page may read server state too (e.g. Refill after a forced jam): refresh it once
			RefreshServerState(targetKey);
		}
		else if (!success)
		{
			//a rejected optimistic toggle is reverted by the authoritative state
			VPPContextActionManager mgr = GetVPPContextActionManager();
			if (mgr)
			{
				foreach (VPPContextMenuFrame failFrame : m_Frames)
				{
					if (failFrame && failFrame.Target && failFrame.Target.GetKey() == targetKey)
					{
						if (mgr.NeedsServerState(failFrame.Target))
							mgr.SendStateRequest(failFrame.Target);

						break;
					}
				}
			}
		}

		RebuildIfCurrentKey(targetKey);
	}

	void OnStateReceived(int seq, string targetKey, map<string,string> state)
	{
		if (!state)
			return;

		foreach (VPPContextMenuFrame f : m_Frames)
		{
			if (f && f.Target && f.Target.GetKey() == targetKey)
				f.Target.MergeState(state);
		}

		RebuildIfCurrentKey(targetKey);
	}

	void OnPermissionsReceived()
	{
		if (IsOpen())
			RebuildCurrentPage(GetFocusedItemId());
	}

	//subscribed to VPPAdminHud.m_OnPermissionsChanged
	void OnHudPermissionsChanged(map<string,bool> perms)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (mgr)
			mgr.SendPermissionRequest(true);
	}

	//VPPDialogBox callback (cancel: CallFunction(int); OK: CallFunctionParams Param2<int,string>)
	void OnPromptResult(int result, string input = "")
	{
		m_PromptOpen = false;
		if (m_PromptFromLook)
			EndDialogHandoff();

		if (result == DIAGRESULT.OK && m_PromptAction && m_PromptTarget)
		{
			VPPContextArgs args = new VPPContextArgs();
			if (m_PromptArgs)
				args = VPPContextArgs.FromMap(m_PromptArgs.GetMap());

			args.Set(VPPContextConstants.ARG_INPUT, input);
			Dispatch(m_PromptAction, m_PromptTarget, args, null, -1);
		}

		ClearPrompt();
	}

	/*
		Manager subscription + spectate bridge install
	*/
	protected void EnsureSubscribed()
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr || mgr == m_SubscribedMgr)
			return;

		UnsubscribeManager();

		mgr.Event_OnResult.Insert(this.OnActionResult);
		mgr.Event_OnState.Insert(this.OnStateReceived);
		mgr.Event_OnPermissions.Insert(this.OnPermissionsReceived);
		m_SubscribedMgr = mgr;

		//only replace the stock action: a third-party replacement is left alone
		VPPContextAction existing = mgr.GetAction(VPPContextConstants.ID_PLAYER_SPECTATE);
		if (existing && existing.Type() == VPPCA_PlayerSpectate)
		{
			VPPContextSpectateAction spectateAction = new VPPContextSpectateAction();
			mgr.ReplaceAction(VPPContextConstants.ID_PLAYER_SPECTATE, spectateAction);
		}
	}

	protected void UnsubscribeManager()
	{
		if (!m_SubscribedMgr)
			return;

		//only touch the manager while it is still the live plugin (plugins die in ~MissionBase)
		if (GetVPPContextActionManager() != m_SubscribedMgr)
		{
			m_SubscribedMgr = null;
			return;
		}

		if (m_SubscribedMgr.Event_OnResult)
			m_SubscribedMgr.Event_OnResult.Remove(this.OnActionResult);

		if (m_SubscribedMgr.Event_OnState)
			m_SubscribedMgr.Event_OnState.Remove(this.OnStateReceived);

		if (m_SubscribedMgr.Event_OnPermissions)
			m_SubscribedMgr.Event_OnPermissions.Remove(this.OnPermissionsReceived);

		m_SubscribedMgr = null;
	}

	/*
		Look mode
	*/
	protected bool CanUseLookMode()
	{
		if (!m_LookEnabled)
			return false;

		MissionBaseWorld mission = MissionBaseWorld.Cast(GetGame().GetMission());
		if (!mission || !mission.VPPAT_AdminToolsToggled())
			return false;

		//no UIScriptedMenu at all (incl. the admin HUD with game focus): avoids vanilla's per-frame mouse DisableKey
		if (GetGame().GetUIManager().GetMenu() != null)
			return false;

		if (GetGame().GetUIManager().IsCursorVisible())
			return false;

		VPPUIManager uiMgr = GetVPPUIManager();
		if (!uiMgr || uiMgr.GetKeybindsStatus() || uiMgr.IsTyping())
			return false;

		if (g_Game.IsSpectateMode())
			return false;

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.IsAlive() || player.IsUnconscious())
			return false;

		if (m_PromptOpen || m_PromptPending)
			return false;

		return true;
	}

	protected VPPContextTarget PickCrosshairTarget()
	{
		Object obj = g_Game.getObjectAtCrosshair(VPPContextMenuTuning.PICK_DISTANCE, 0.0, NULL);
		if (!obj || obj.GetType() == "")
			return null;

		if (!VPPContextMenuTuning.LOOK_ALLOW_STATIC_OBJECTS && !EntityAI.Cast(obj))
			return null;

		return VPPContextTarget.ForObject(obj, EVPPContextSource.CROSSHAIR);
	}

	protected void TryArmLook()
	{
		Input input = GetGame().GetInput();
		if (!input.LocalPress(VPPContextMenuTuning.INPUT_MENU, false))
			return;

		if (!CanUseLookMode())
			return;

		//nothing actionable under the crosshair: vanilla RMB is left untouched
		VPPContextTarget t = PickCrosshairTarget();
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!t || !mgr || !mgr.HasAnyAction(t))
			return;

		m_ArmTarget = t;
		m_ArmTimer = 0;
		m_Capture.Begin();
		m_Mode = EVPPContextMenuMode.LOOK;
		m_Phase = EVPPContextMenuPhase.ARMED;

		mgr.SendPermissionRequest(false);
	}

	protected void UpdateArmed(float dt)
	{
		m_Capture.Keep();

		Input input = GetGame().GetInput();
		bool released = input.LocalRelease(VPPContextMenuTuning.INPUT_MENU, false);
		if (input.LocalValue(VPPContextMenuTuning.INPUT_MENU, false) <= 0)
			released = true;

		bool targetGone = false;
		if (!m_ArmTarget)
			targetGone = true;
		else if (TargetNeedsObject(m_ArmTarget) && !m_ArmTarget.HasObject())
			targetGone = true;

		//released before the open delay: the click is swallowed until both buttons are up
		if (released || GetGame().GetUIManager().GetMenu() != null || targetGone || !m_LookEnabled)
		{
			m_Phase = EVPPContextMenuPhase.WAIT_RELEASE;
			return;
		}

		m_ArmTimer += dt;
		if (m_ArmTimer >= VPPContextMenuTuning.LOOK_OPEN_DELAY)
			OpenPage(m_ArmTarget, true, 0, 0);
	}

	protected bool ShouldCloseLook()
	{
		Input input = GetGame().GetInput();
		if (input.LocalRelease(VPPContextMenuTuning.INPUT_MENU, false))
			return true;

		if (input.LocalValue(VPPContextMenuTuning.INPUT_MENU, false) <= 0)
			return true;

		if (GetGame().GetUIManager().GetMenu() != null)
			return true;

		if (!m_LookEnabled)
			return true;

		MissionBaseWorld mission = MissionBaseWorld.Cast(GetGame().GetMission());
		if (!mission || !mission.VPPAT_AdminToolsToggled())
			return true;

		if (g_Game.IsSpectateMode())
			return true;

		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !player.IsAlive())
			return true;

		if (m_RequiresObject && !RootTargetHasObject())
			return true;

		return false;
	}

	protected void UpdateLookOpen(float dt)
	{
		m_Capture.Keep();
		if (ShouldCloseLook())
		{
			EnterWaitRelease();
			return;
		}

		PopGoneFrames();

		Input input = GetGame().GetInput();

		//mouse-axis navigation; the idle reset stops slow creep from walking the highlight
		float d = input.LocalValue(VPPContextMenuTuning.INPUT_NAV_DOWN, false) - input.LocalValue(VPPContextMenuTuning.INPUT_NAV_UP, false);
		if (VPPContextMenuTuning.LOOK_NAV_INVERT)
			d = -d;

		float maxAccum = VPPContextMenuTuning.LOOK_NAV_MAX_ACCUM;
		float step = VPPContextMenuTuning.LOOK_NAV_STEP;
		if (Math.AbsFloat(d) <= VPPContextMenuTuning.LOOK_NAV_DEADZONE)
		{
			m_NavIdle += dt;
			if (m_NavIdle >= VPPContextMenuTuning.LOOK_NAV_IDLE_RESET)
				m_NavAccum = 0;
		}
		else
		{
			m_NavIdle = 0;
			m_NavAccum = Math.Clamp(m_NavAccum + (d * dt), -maxAccum, maxAccum);
		}

		while (m_NavAccum >= step)
		{
			MoveFocus(1);
			m_NavAccum -= step;
		}

		while (m_NavAccum <= -step)
		{
			MoveFocus(-1);
			m_NavAccum += step;
		}

		if (VPPContextMenuTuning.LOOK_NAV_DEBUG)
		{
			m_NavDebugTimer += dt;
			if (m_NavDebugTimer >= 0.5)
			{
				m_NavDebugTimer = 0;
				string dbg = "[VPPContextMenu] nav d=" + d.ToString();
				dbg += " accum=" + m_NavAccum.ToString();
				Print(dbg);
			}
		}

		if (input.LocalPress(VPPContextMenuTuning.INPUT_WHEEL_DOWN, false))
			MoveFocus(1);

		if (input.LocalPress(VPPContextMenuTuning.INPUT_WHEEL_UP, false))
			MoveFocus(-1);

		PollKeyboard();
		if (!IsOpen())
			return;

		if (input.LocalPress(VPPContextMenuTuning.INPUT_SELECT, false))
			ActivateIndex(m_View.GetFocusIndex());
	}

	protected void EnterWaitRelease()
	{
		m_View.Show(false);
		DisarmConfirm();
		m_Frames.Clear();
		m_Phase = EVPPContextMenuPhase.WAIT_RELEASE;
	}

	//capture is kept until the context key, select and both raw buttons are up (no raise/fire afterwards)
	protected void UpdateWaitRelease()
	{
		m_Capture.Keep();

		Input input = GetGame().GetInput();
		bool up = true;
		if (input.LocalValue(VPPContextMenuTuning.INPUT_MENU, false) > 0)
			up = false;

		if (input.LocalValue(VPPContextMenuTuning.INPUT_SELECT, false) > 0)
			up = false;

		if ((GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0)
			up = false;

		if ((GetMouseState(MouseState.RIGHT) & MB_PRESSED_MASK) != 0)
			up = false;

		if (!up)
			return;

		m_Capture.End();
		m_Phase = EVPPContextMenuPhase.IDLE;
		m_Mode = EVPPContextMenuMode.NONE;
		m_ArmTarget = null;

		if (m_PromptPending)
		{
			m_PromptPending = false;
			OpenPrompt(true);
		}
	}

	protected void ForceReset()
	{
		m_Capture.End();
		m_View.Show(false);
		DisarmConfirm();
		m_Frames.Clear();
		m_Phase = EVPPContextMenuPhase.IDLE;
		m_Mode = EVPPContextMenuMode.NONE;
		m_ArmTarget = null;
		if (m_PromptPending)
		{
			m_PromptPending = false;
			if (!m_PromptOpen)
			{
				m_PromptAction = null;
				m_PromptTarget = null;
				m_PromptFromLook = false;
			}
		}
	}

	/*
		Cursor mode
	*/
	protected int CurrentMouseMask()
	{
		int mask = 0;
		if ((GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0)
			mask = mask | 1;

		if ((GetMouseState(MouseState.RIGHT) & MB_PRESSED_MASK) != 0)
			mask = mask | 2;

		if ((GetMouseState(MouseState.MIDDLE) & MB_PRESSED_MASK) != 0)
			mask = mask | 4;

		return mask;
	}

	//no click-away catcher: an outside press closes the popup and still reaches the widget below
	protected void UpdateCursorOpen(float dt)
	{
		if (!GetGame().GetUIManager().IsCursorVisible())
		{
			Close();
			return;
		}

		if (m_RequiresObject && !RootTargetHasObject())
		{
			Close();
			return;
		}

		PopGoneFrames();

		PollKeyboard();
		if (!IsOpen())
			return;

		int mask = CurrentMouseMask();
		int rising = mask & ~m_PrevMouseMask;
		m_PrevMouseMask = mask;

		if (rising != 0 && !m_View.IsWidgetInside(GetWidgetUnderCursor()))
			Close();
	}

	/*
		Pages
	*/
	protected void OpenPage(VPPContextTarget target, bool look, int x, int y)
	{
		m_Frames.Clear();
		m_StateRequested.Clear();

		VPPContextMenuFrame root = new VPPContextMenuFrame();
		root.Target = target;
		root.ParentId = "";
		root.Title = TitleFor(target);
		root.TitleIcon = VPPContextUtils.GetTargetIcon(target);
		root.Focus = -1;
		m_Frames.Insert(root);

		m_RequiresObject = false;
		if (target.HasObject() && TargetNeedsObject(target))
			m_RequiresObject = true;

		m_NavAccum = 0;
		m_NavIdle = 0;

		RebuildCurrentPage("");

		if (look)
			m_View.PlaceAtCrosshair();
		else
			m_View.PlaceAtCursor(x, y);

		m_View.Show(true);
		m_Phase = EVPPContextMenuPhase.OPEN;
		RequestStateOnce(target);
	}

	protected string TitleFor(VPPContextTarget target)
	{
		if (!target)
			return "";

		if (target.IsMulti())
		{
			string fmt = Widget.TranslateString("#VSTR_CTX_MULTI_PLAYERS");
			return string.Format(fmt, target.GetPlayerCount().ToString());
		}

		return target.GetDisplayName();
	}

	protected void RequestStateOnce(VPPContextTarget target)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr || !target)
			return;

		if (!mgr.NeedsServerState(target))
			return;

		string key = target.GetKey();
		if (m_StateRequested.Find(key) != -1)
			return;

		mgr.SendStateRequest(target);
		m_StateRequested.Insert(key);
	}

	protected void RebuildCurrentPage(string keepFocusId)
	{
		VPPContextMenuFrame f = GetCurrentFrame();
		if (!f)
			return;

		//remember an armed confirmation so a background rebuild (state/permission/result reply) keeps it
		string armedId = "";
		float armedTimer = 0;
		if (m_ConfirmIndex >= 0 && m_ConfirmTimer > 0)
		{
			VPPContextItem armedPrev = GetDisplayItem(m_ConfirmIndex);
			if (armedPrev)
			{
				armedId = armedPrev.Id;
				armedTimer = m_ConfirmTimer;
			}
		}

		m_Display = new VPPContextItems();

		if (m_Frames.Count() > 1)
		{
			VPPContextItem back = new VPPContextItem();
			back.Id = VPPContextConstants.ID_UI_BACK;
			back.Kind = EVPPContextKind.BACK;
			back.Label = "#VSTR_CTX_BACK";
			back.IconPath = VPPContextConstants.ICON_CHEVRON;
			back.Target = f.Target;
			m_Display.Insert(back);
		}

		VPPContextItems page = new VPPContextItems();
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (mgr)
			mgr.BuildPage(f.Target, f.ParentId, page);

		for (int i = 0; i < page.Count(); i++)
		{
			VPPContextItem pageItem = page.Get(i);
			if (pageItem)
				m_Display.Insert(pageItem);
		}

		//only a truly empty page (nothing but separators) gets the placeholder row
		bool hasRealRow = false;
		for (int m = 0; m < page.Count(); m++)
		{
			VPPContextItem realCand = page.Get(m);
			if (realCand && realCand.Kind != EVPPContextKind.SEPARATOR)
			{
				hasRealRow = true;
				break;
			}
		}

		if (!hasRealRow)
			m_Display.AddInfo(VPPContextConstants.ID_UI_NONE, "#VSTR_CTX_NO_ACTIONS", VPPContextConstants.ICON_INFO, 0, "");

		m_View.Render(f.Title, f.TitleIcon, m_Display, IsLookMode());

		//focus: kept row, then the frame's saved row, then the first real row, then Back
		int focus = -1;
		if (keepFocusId != "")
		{
			int keepIdx = m_Display.IndexOf(keepFocusId);
			if (IsFocusable(keepIdx))
				focus = keepIdx;
		}

		if (focus < 0 && IsFocusable(f.Focus))
			focus = f.Focus;

		if (focus < 0)
		{
			for (int j = 0; j < m_Display.Count(); j++)
			{
				VPPContextItem cand = m_Display.Get(j);
				if (cand && cand.Kind != EVPPContextKind.BACK && IsFocusable(j))
				{
					focus = j;
					break;
				}
			}
		}

		if (focus < 0)
		{
			for (int k = 0; k < m_Display.Count(); k++)
			{
				VPPContextItem backCand = m_Display.Get(k);
				if (backCand && backCand.Kind == EVPPContextKind.BACK)
				{
					focus = k;
					break;
				}
			}
		}

		m_View.SetFocusIndex(focus);
		m_ConfirmIndex = -1;
		m_ConfirmTimer = 0;

		//restore the armed row only on an in-place rebuild that kept the same row focused
		if (armedId != "" && armedId == keepFocusId)
		{
			int armedIdx = m_Display.IndexOf(armedId);
			VPPContextItem armedNow = GetDisplayItem(armedIdx);
			if (armedNow && armedIdx == focus && armedNow.Confirm && armedNow.Enabled)
			{
				m_ConfirmIndex = armedIdx;
				m_ConfirmTimer = armedTimer;
				m_View.SetConfirmArmed(armedIdx, true);
			}
		}

		if (IsLookMode())
			m_View.PlaceAtCrosshair();
		else
			m_View.Reclamp();
	}

	protected void RebuildIfCurrentKey(string targetKey)
	{
		if (!IsOpen())
			return;

		VPPContextMenuFrame cur = GetCurrentFrame();
		if (cur && cur.Target && cur.Target.GetKey() == targetKey)
			RebuildCurrentPage(GetFocusedItemId());
	}

	protected void PushFrame(VPPContextItem item)
	{
		VPPContextMenuFrame cur = GetCurrentFrame();
		if (!cur || !item)
			return;

		cur.Focus = m_View.GetFocusIndex();

		VPPContextMenuFrame f = new VPPContextMenuFrame();
		f.Target = item.Target;
		if (!f.Target)
			f.Target = cur.Target;

		f.ParentId = item.SubmenuId;
		f.Focus = -1;

		//a different entity (key) is a retarget; the same key, including a CloneTarget carrying Extras, is a drill-in titled with the row label
		if (!f.Target.IsSameTarget(cur.Target))
		{
			f.Title = TitleFor(f.Target);
			f.TitleIcon = VPPContextUtils.GetTargetIcon(f.Target);
			RequestStateOnce(f.Target);
		}
		else
		{
			f.Title = item.Label;
			f.TitleIcon = item.IconPath;
		}

		m_Frames.Insert(f);
		RebuildCurrentPage("");
	}

	//RebuildCurrentPage restores the parent frame's saved focus
	protected void PopFrame()
	{
		if (m_Frames.Count() <= 1)
			return;

		m_Frames.Remove(m_Frames.Count() - 1);
		RebuildCurrentPage("");
	}

	protected void MoveFocus(int dir)
	{
		if (!m_Display || dir == 0)
			return;

		int count = m_Display.Count();
		int cur = m_View.GetFocusIndex();
		int nextIdx = -1;
		int idx = cur + dir;
		while (idx >= 0 && idx < count)
		{
			if (IsFocusable(idx))
			{
				nextIdx = idx;
				break;
			}
			idx += dir;
		}

		//clamped at the ends, no wrap
		if (nextIdx < 0 || nextIdx == cur)
			return;

		DisarmConfirm();
		m_View.SetFocusIndex(nextIdx);
	}

	//kLeft (UAVPPCtxBack) is the guaranteed back key in look mode; Esc is best effort there
	protected void PollKeyboard()
	{
		if (GetVPPUIManager() && GetVPPUIManager().IsTyping())
			return;

		Input input = GetGame().GetInput();
		if (input.LocalPress(VPPContextMenuTuning.INPUT_ARROW_UP, false))
			MoveFocus(-1);

		if (input.LocalPress(VPPContextMenuTuning.INPUT_ARROW_DOWN, false))
			MoveFocus(1);

		if (input.LocalPress(VPPContextMenuTuning.INPUT_ENTER, false))
		{
			ActivateIndex(m_View.GetFocusIndex());
			if (!IsOpen())
				return;
		}

		if (input.LocalPress(VPPContextMenuTuning.INPUT_BACK, false))
		{
			if (m_Frames.Count() > 1)
			{
				PopFrame();
			}
			else
			{
				//back at the root closes the menu (mirrors OnKeyPress Esc)
				Close();
				return;
			}
		}

		if (input.LocalPress(VPPContextMenuTuning.INPUT_FORWARD, false))
		{
			VPPContextItem focused = GetDisplayItem(m_View.GetFocusIndex());
			if (focused && focused.Kind == EVPPContextKind.SUBMENU)
				ActivateIndex(m_View.GetFocusIndex());
		}
	}

	/*
		Activation / execution
	*/
	protected void ActivateIndex(int index)
	{
		if (!IsOpen())
			return;

		VPPContextItem item = GetDisplayItem(index);
		if (!item)
			return;

		if (item.Kind == EVPPContextKind.SEPARATOR || item.Kind == EVPPContextKind.INFO)
			return;

		if (item.Kind == EVPPContextKind.BACK)
		{
			PopFrame();
			return;
		}

		if (!item.Enabled)
			return;

		if (item.Kind == EVPPContextKind.SUBMENU)
		{
			PushFrame(item);
			return;
		}

		//click-again confirmation: the first activation only arms the row
		if (item.Confirm)
		{
			bool confirmed = false;
			if (m_ConfirmIndex == index && m_ConfirmTimer > 0)
				confirmed = true;

			if (!confirmed)
			{
				ArmConfirm(index);
				return;
			}
		}

		DisarmConfirm();
		Execute(item, index);
	}

	protected void ArmConfirm(int index)
	{
		m_ConfirmIndex = index;
		m_ConfirmTimer = VPPContextMenuTuning.CONFIRM_WINDOW;
		m_View.SetConfirmArmed(index, true);
	}

	protected void DisarmConfirm()
	{
		if (m_ConfirmIndex >= 0 && m_View)
			m_View.SetConfirmArmed(-1, false);

		m_ConfirmIndex = -1;
		m_ConfirmTimer = 0;
	}

	protected void Execute(VPPContextItem item, int index)
	{
		VPPContextAction action = item.Action;
		VPPContextTarget target = item.Target;

		//rows without their own target (e.g. VPPContextItems.AddCallback) execute on the page target
		if (!target)
		{
			VPPContextMenuFrame execFrame = GetCurrentFrame();
			if (execFrame)
				target = execFrame.Target;
		}

		//dynamic row without an action: provider callback
		if (!action)
		{
			if (item.CallbackInst && item.CallbackFunc != "")
			{
				//Param2 members are weak: keep the args alive in a local for the duration of the call
				VPPContextArgs cbArgs = new VPPContextArgs();
				if (item.Args)
					cbArgs = VPPContextArgs.FromMap(item.Args.GetMap());

				Param2<VPPContextTarget, VPPContextArgs> cbParams = new Param2<VPPContextTarget, VPPContextArgs>(target, cbArgs);
				GetGame().GameScript.CallFunctionParams(item.CallbackInst, item.CallbackFunc, null, cbParams);
			}

			if (!item.KeepOpen)
				Close();

			return;
		}

		//text input: the menu closes first; look mode defers the dialog until the buttons are up
		if (action.GetInputPrompt(target) != "")
		{
			m_PromptAction = action;
			m_PromptTarget = target;
			m_PromptArgs = null;
			if (item.Args)
				m_PromptArgs = VPPContextArgs.FromMap(item.Args.GetMap());

			if (IsLookMode())
			{
				m_PromptFromLook = true;
				m_PromptPending = true;
				Close();
			}
			else
			{
				m_PromptFromLook = false;
				Close();
				OpenPrompt(false);
			}
			return;
		}

		VPPContextArgs args = new VPPContextArgs();
		if (item.Args)
			args = VPPContextArgs.FromMap(item.Args.GetMap());

		Dispatch(action, target, args, item, index);
	}

	protected void Dispatch(VPPContextAction action, VPPContextTarget target, VPPContextArgs args, VPPContextItem item, int index)
	{
		if (!action || !target)
			return;

		bool handledLocally = action.OnExecuteClient(target, args);
		if (!handledLocally)
		{
			VPPContextActionManager mgr = GetVPPContextActionManager();
			if (mgr)
			{
				int seq = mgr.SendExecute(action, target, args);
				m_PendingResults.Set(seq, target.GetKey());
			}
		}

		//optimistic toggle flip; server actions reconcile from HandleResult, client ones from a delayed RequestState
		if (item && item.Kind == EVPPContextKind.TOGGLE)
		{
			item.Checked = !item.Checked;
			target.SetState(action.GetId(), VPPContextUtils.BoolToState(item.Checked));
			m_View.RefreshRow(index);

			if (handledLocally)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.ReconcileState);
				GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ReconcileState, VPPContextMenuTuning.TOGGLE_RECONCILE_MS, false);
			}
		}

		bool keep = false;
		if (item)
			keep = item.KeepOpen;

		if (!keep)
			Close();
	}

	//bypasses RequestStateOnce on purpose
	protected void ReconcileState()
	{
		if (!IsOpen())
			return;

		VPPContextMenuFrame f = GetCurrentFrame();
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!f || !f.Target || !mgr)
			return;

		mgr.SendStateRequest(f.Target);
	}

	//Re-requests the open page's server state after a kept-open server action changed its target.
	//Bypasses RequestStateOnce on purpose; the server throttles state requests per sender and target.
	protected void RefreshServerState(string targetKey)
	{
		if (!IsOpen())
			return;

		VPPContextMenuFrame f = GetCurrentFrame();
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!f || !f.Target || !mgr)
			return;

		if (f.Target.GetKey() != targetKey)
			return;

		if (mgr.NeedsServerState(f.Target))
			mgr.SendStateRequest(f.Target);
	}

	/*
		Text prompt dialog
	*/
	protected void OpenPrompt(bool fromLook)
	{
		if (!m_PromptAction)
		{
			ClearPrompt();
			return;
		}

		VPPUIManager uiMgr = GetVPPUIManager();
		if (!uiMgr)
		{
			ClearPrompt();
			return;
		}

		VPPDialogBox dlg = uiMgr.CreateDialogBox(null, true);
		if (!dlg)
		{
			ClearPrompt();
			return;
		}

		m_PromptDialog = dlg;
		m_PromptFromLook = fromLook;
		if (fromLook)
			BeginDialogHandoff();

		//the edit box is never pre-filled
		string title = Widget.TranslateString(m_PromptAction.GetLabel(m_PromptTarget));
		string body = Widget.TranslateString(m_PromptAction.GetInputPrompt(m_PromptTarget));
		dlg.InitDiagBox(DIAGTYPE.DIAG_OK_CANCEL_INPUT, title, body, this, "OnPromptResult");
		if (!m_PromptAction.IsInputNumeric())
			dlg.AllowCharInput();

		m_PromptOpen = true;
	}

	//DeleteObjCrosshair recipe minus the mission-wide control lock, so freecam excludes survive
	protected void BeginDialogHandoff()
	{
		VPPUIManager uiMgr = GetVPPUIManager();
		if (uiMgr && !uiMgr.GetKeybindsStatus())
		{
			uiMgr.SetKeybindsStatus(true);
			m_DialogLockedKeybinds = true;
		}

		GetGame().GetInput().ChangeGameFocus(1);
		m_DialogChangedFocus = true;

		GetGame().GetUIManager().ShowUICursor(true);

		Mission mission = GetGame().GetMission();
		if (mission && !mission.IsInputExcludeActive("menu"))
		{
			mission.AddActiveInputExcludes({"menu"});
			m_DialogAddedMenuExclude = true;
		}
	}

	protected void EndDialogHandoff()
	{
		Mission mission = GetGame().GetMission();
		if (m_DialogAddedMenuExclude && mission)
			mission.RemoveActiveInputExcludes({"menu"}, true);

		if (m_DialogChangedFocus)
			GetGame().GetInput().ChangeGameFocus(-1);

		if (GetGame().GetUIManager().GetMenu() == null)
			GetGame().GetUIManager().ShowUICursor(false);

		if (m_DialogLockedKeybinds && GetVPPUIManager())
			GetVPPUIManager().SetKeybindsStatus(false);

		m_DialogAddedMenuExclude = false;
		m_DialogChangedFocus = false;
		m_DialogLockedKeybinds = false;
	}

	//the dialog was destroyed without calling back (teardown, HUD changes)
	protected void PromptWatchdog()
	{
		if (!m_PromptOpen || m_PromptDialog)
			return;

		m_PromptOpen = false;
		if (m_PromptFromLook)
			EndDialogHandoff();

		ClearPrompt();
	}

	protected void ClearPrompt()
	{
		m_PromptAction = null;
		m_PromptTarget = null;
		m_PromptArgs = null;
		m_PromptDialog = null;
		m_PromptFromLook = false;
	}

	/*
		Helpers
	*/
	protected bool IsLookMode()
	{
		if (m_Mode == EVPPContextMenuMode.LOOK)
			return true;

		return false;
	}

	protected VPPContextMenuFrame GetCurrentFrame()
	{
		if (!m_Frames || m_Frames.Count() == 0)
			return null;

		return m_Frames[m_Frames.Count() - 1];
	}

	protected bool RootTargetHasObject()
	{
		if (!m_Frames || m_Frames.Count() == 0)
			return false;

		VPPContextMenuFrame root = m_Frames[0];
		if (!root || !root.Target)
			return false;

		return root.Target.HasObject();
	}

	//object targets and crosshair-picked players are bound to a live entity; player-list players may be outside the bubble
	protected bool TargetNeedsObject(VPPContextTarget t)
	{
		if (!t)
			return false;

		if (t.GetKind() == EVPPContextTargetKind.OBJECT)
			return true;

		if (t.GetKind() == EVPPContextTargetKind.PLAYER && t.GetSource() == EVPPContextSource.CROSSHAIR)
			return true;

		return false;
	}

	//a drill-down page whose retargeted entity vanished (dropped, deleted, left the bubble) falls back to its parent
	protected void PopGoneFrames()
	{
		while (m_Frames && m_Frames.Count() > 1)
		{
			VPPContextMenuFrame cur = GetCurrentFrame();
			VPPContextMenuFrame parent = m_Frames[m_Frames.Count() - 2];
			if (!cur || !cur.Target || !parent || cur.Target == parent.Target)
				return;

			if (!TargetNeedsObject(cur.Target) || cur.Target.HasObject())
				return;

			PopFrame();
		}
	}

	protected VPPContextItem GetDisplayItem(int index)
	{
		if (!m_Display || index < 0 || index >= m_Display.Count())
			return null;

		return m_Display.Get(index);
	}

	protected bool IsFocusable(int index)
	{
		VPPContextItem item = GetDisplayItem(index);
		if (!item)
			return false;

		if (item.Kind == EVPPContextKind.SEPARATOR || item.Kind == EVPPContextKind.INFO)
			return false;

		return true;
	}

	protected string GetFocusedItemId()
	{
		VPPContextItem item = GetDisplayItem(m_View.GetFocusIndex());
		if (!item)
			return "";

		return item.Id;
	}
};

VPPContextMenuController GetVPPContextMenu()
{
	return VPPContextMenuController.GetInstance();
}
