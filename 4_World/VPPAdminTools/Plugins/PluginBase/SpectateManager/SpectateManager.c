class SpectateManager extends PluginBase
{
	private ref map<string, ref SpectateSession> m_Sessions;
	private ref Timer 						 	 m_FollowTimer;
	private ref Timer 						 	 m_ADSTimer;        //fast sight-mode poll (edge-triggered pushes)
	private ref VPPSpectateRecovery 			 m_Recovery;
	private ref map<string, int> 				 m_DeathGracePrefs; //adminId -> seconds (absent = default)
	private int 								 m_DbgLastLookRelay; //throttle for the look-relay debug log

	private const static string RECOVERY_PATH = "$profile:VPPAdminTools/SpectateRecovery.json";

	void SpectateManager()
	{
		m_Sessions = new map<string, ref SpectateSession>;
		m_Recovery = new VPPSpectateRecovery();
		m_DeathGracePrefs = new map<string, int>;

		GetRPCManager().AddRPC( "RPC_SpectateManager", "RequestSpectate", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_SpectateManager", "RequestExitSpectate", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_SpectateManager", "RequestInitRetry", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_SpectateManager", "ClientReadyAck", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC( "RPC_SpectateManager", "SetDeathGrace", this, SingeplayerExecutionType.Server );
		//NOTE: the look stream arrives via the vanilla ENTITY RPC (PlayerBase.OnRPC
		//-> RelayLookData), NOT the CF channel — the CF client->server path rejects
		//RPCs from non-admin senders (no admin password)
	}

	override void OnInit()
	{
		super.OnInit();
		if (FileExist(RECOVERY_PATH))
		{
			JsonFileLoader<VPPSpectateRecovery>.JsonLoadFile(RECOVERY_PATH, m_Recovery);
			if (m_Recovery == null || m_Recovery.positions == null)
				m_Recovery = new VPPSpectateRecovery();
		}
	}

	//---------------------------------------------------------------------------------
	// RPC handlers
	//---------------------------------------------------------------------------------
	void RequestSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || sender == null)
			return;

		Param1<string> data; //target steam64
		if (!ctx.Read(data))
			return;

		RequestSpectateDirect(sender, data.param1);
	}

	//Public entry shared by the legacy PlayerManager::SpectatePlayer reroute.
	void RequestSpectateDirect(PlayerIdentity sender, string targetId)
	{
		if (sender == null)
			return;

		string adminId = sender.GetPlainId();

		if (!GetPermissionManager().VerifyPermission(adminId, "PlayerManager:SpectatePlayer"))
			return;

		if (!GetPermissionManager().VerifyPermission(adminId, "PlayerManager:SpectatePlayer", targetId))
			return;

		if (adminId == targetId)
		{
			GetPermissionManager().NotifyPlayer(adminId, "#VSTR_NOTIFY_SPECTATE_SELF", NotifyTypes.ERROR);
			return;
		}

		PlayerBase targetPb = GetPermissionManager().GetPlayerBaseByID(targetId);
		PlayerBase adminPb  = GetPermissionManager().GetPlayerBaseByID(adminId);
		if (targetPb == null || adminPb == null)
			return;

		if (!targetPb.IsAlive())
		{
			GetPermissionManager().NotifyPlayer(adminId, "#VSTR_NOTIFY_SPECTATE_TARGET_LOST", NotifyTypes.ERROR);
			return;
		}

		//no mutual/chained spectate: a spectating admin's body is frozen 40m under
		//THEIR target — following it would drag both bodies into the void
		if (m_Sessions.Contains(targetId))
		{
			GetPermissionManager().NotifyPlayer(adminId, "#VSTR_NOTIFY_SPECTATE_TARGET_BUSY", NotifyTypes.ERROR);
			return;
		}

		if (adminPb.GetCommand_Vehicle())
		{
			GetPermissionManager().NotifyPlayer(adminId, "#VSTR_NOTIFY_SPECTATE_IN_VEHICLE", NotifyTypes.ERROR);
			return;
		}

		SpectateSession existing = m_Sessions.Get(adminId);
		if (existing != null)
		{
			SwitchTarget(existing, targetPb, targetId);
		}
		else
		{
			BeginSpectate(adminPb, targetPb, adminId, targetId);
		}

		GetSimpleLogger().Log(string.Format("\"%1\" (steamid=%2) started to spectate player: \"%3\"", sender.GetName(), adminId, targetId));
		GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(adminId, sender.GetName(), "[SpectateManager] Spectating player: " + targetId));
	}

	void RequestExitSpectate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || sender == null)
			return;

		EndSpectate(sender.GetPlainId(), EVPPSpectateEndReason.ADMIN_REQUEST, true);
	}

	void RequestInitRetry(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || sender == null)
			return;

		SpectateSession session = m_Sessions.Get(sender.GetPlainId());
		if (session == null)
			return;

		session.m_ResendCount = session.m_ResendCount + 1;
		if (session.m_ResendCount > VPPSpectateConstants.INIT_MAX_RETRIES)
		{
			EndSpectate(session.m_AdminId, EVPPSpectateEndReason.INIT_FAILED, true);
			return;
		}

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendClientStart, VPPSpectateConstants.STREAM_GRACE_SEC * 1000, false, session.m_AdminId);
	}

	void ClientReadyAck(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || sender == null)
			return;

		SpectateSession session = m_Sessions.Get(sender.GetPlainId());
		if (session != null)
			session.m_ClientReady = true;
	}

	//per-admin death-grace preference from the menu dropdown (0 = exit immediately)
	void SetDeathGrace(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || sender == null)
			return;

		Param1<int> data;
		if (!ctx.Read(data))
			return;

		int seconds = Math.Clamp(data.param1, 0, VPPSpectateConstants.DEATH_GRACE_MAX);
		m_DeathGracePrefs.Set(sender.GetPlainId(), seconds);
	}

	protected int GetDeathGraceFor(string adminId)
	{
		if (m_DeathGracePrefs.Contains(adminId))
			return m_DeathGracePrefs.Get(adminId);
		return VPPSpectateConstants.DEATH_GRACE_DEFAULT;
	}

	//---------------------------------------------------------------------------------
	// Session lifecycle
	//---------------------------------------------------------------------------------
	protected void BeginSpectate(PlayerBase adminPb, PlayerBase targetPb, string adminId, string targetId)
	{
		SpectateSession session = new SpectateSession(adminId, targetId);

		//capture return state BEFORE any mutation
		session.m_ReturnPos    = adminPb.GetPosition();
		session.m_ReturnOri    = adminPb.GetOrientation();
		session.m_HadGodmode   = adminPb.GodModeStatus();
		session.m_WasInvisible = adminPb.InvisibilityStatus();

		//persist crash recovery BEFORE moving the body
		m_Recovery.positions.Set(adminId, session.m_ReturnPos);
		SaveRecovery();

		//protect + hide + freeze (DEBUG: ADS_DEBUG_SHOW_BODY keeps the body visible
		//so the client-local link attachment can be inspected in-world)
		if (!session.m_HadGodmode)
			adminPb.setGodMode(true);
		if (!session.m_WasInvisible && !VPPSpectateConstants.ADS_DEBUG_SHOW_BODY)
			adminPb.VPPSetInvisibility(true, InvisToggleType.SCRIPT);
		FreezeAdminBody(adminPb, true);

		//initial placement, then SERVER-SIDE link: the body rides the target's head
		//continuously — no follow re-teleports (their client-side corrections caused
		//visible jitter against the client link), and the network bubble streams the
		//target's area off the composed transform
		TeleportBodyUnder(adminPb, targetPb);
		adminPb.VPPATAttachToSpectateTarget(targetPb);

		//flip the target's look-stream flag (net-synced; their client starts
		//sampling its true camera direction for the 1PP view)
		targetPb.VPPATSetLookStream(true);

		m_Sessions.Set(adminId, session);

		//give the client a moment to stream the target in, then hand over
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendClientStart, VPPSpectateConstants.STREAM_GRACE_SEC * 1000, false, adminId);

		StartFollowTimer();
	}

	protected void SwitchTarget(SpectateSession session, PlayerBase newTarget, string newTargetId)
	{
		string oldTargetId = session.m_TargetId;

		//keep the ORIGINAL return state — only the target changes
		session.m_TargetId    = newTargetId;
		session.m_ClientReady = false;
		session.m_ResendCount = 0;
		session.m_TargetDead  = false;
		session.m_DeathGraceLeft = 0.0;
		session.m_LastSightMode  = EVPPSpectateSightMode.NONE;

		PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(session.m_AdminId);
		if (adminPb != null)
		{
			//re-link to the new target (detach first — SetPosition on a linked entity
			//would fight the old link)
			adminPb.VPPATDetachFromSpectateTarget();
			TeleportBodyUnder(adminPb, newTarget);
			adminPb.VPPATAttachToSpectateTarget(newTarget);
		}

		//look-stream handover: stop the old target (unless another admin still
		//spectates them), start the new one
		StopLookStreamIfUnwatched(oldTargetId);
		newTarget.VPPATSetLookStream(true);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SendClientStart, VPPSpectateConstants.STREAM_GRACE_SEC * 1000, false, session.m_AdminId);
	}

	protected void SendClientStart(string adminId)
	{
		SpectateSession session = m_Sessions.Get(adminId);
		if (session == null)
			return;

		PlayerBase adminPb  = GetPermissionManager().GetPlayerBaseByID(adminId);
		PlayerBase targetPb = GetPermissionManager().GetPlayerBaseByID(session.m_TargetId);
		if (adminPb == null || targetPb == null || adminPb.GetIdentity() == null)
			return;

		GetRPCManager().VSendRPC("RPC_SpectateClient", "StartSpectate", new Param2<Object, string>(targetPb, session.m_TargetId), true, adminPb.GetIdentity());
	}

	protected void EndSpectate(string adminId, EVPPSpectateEndReason reason, bool notifyClient)
	{
		SpectateSession session = m_Sessions.Get(adminId);
		if (session == null)
			return;

		PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(adminId);
		if (adminPb != null)
		{
			//ORDER MATTERS: detach the server link FIRST (SetPosition on a linked
			//entity fights the link), then restore position while STILL frozen
			//(no gravity -> no fall), then unfreeze, then restore visibility/
			//protection to their PRIOR state.
			adminPb.VPPATDetachFromSpectateTarget();
			adminPb.SetPosition(session.m_ReturnPos);
			adminPb.SetOrientation(session.m_ReturnOri);

			FreezeAdminBody(adminPb, false);

			//the link CORRUPTS the scripted freeze command (negative instance-leak
			//in server logs = double-free), so conditional command checks can't be
			//trusted — force fresh locomotion after the command winds down
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.ForceRestoreLocomotion, 300, false, adminId);

			if (!session.m_WasInvisible)
				adminPb.VPPSetInvisibility(false, InvisToggleType.SCRIPT);
			if (!session.m_HadGodmode)
				adminPb.setGodMode(false);

			if (notifyClient && adminPb.GetIdentity() != null)
				GetRPCManager().VSendRPC("RPC_SpectateClient", "EndSpectate", new Param1<int>(reason), true, adminPb.GetIdentity());

			//only consume the crash-recovery entry when the restore actually ran —
			//if the admin is offline it stays, so OnPlayerConnectRecovery can still rescue them
			m_Recovery.positions.Remove(adminId);
			SaveRecovery();
		}

		string endedTargetId = "";
		if (session != null)
			endedTargetId = session.m_TargetId;

		m_Sessions.Remove(adminId);
		if (m_Sessions.Count() == 0)
			StopFollowTimer();

		//stop the target's look stream unless another admin still spectates them
		if (endedTargetId != "")
			StopLookStreamIfUnwatched(endedTargetId);

		GetSimpleLogger().Log(string.Format("Spectate session ended for steamid=%1 (reason=%2)", adminId, reason));
	}

	//clears the look-stream flag on a target ONLY when no remaining session
	//references them (multiple admins can spectate the same player)
	protected void StopLookStreamIfUnwatched(string targetId)
	{
		foreach (string adminId, SpectateSession session : m_Sessions)
		{
			if (session.m_TargetId == targetId)
				return;
		}

		PlayerBase targetPb = GetPermissionManager().GetPlayerBaseByID(targetId);
		if (targetPb != null)
			targetPb.VPPATSetLookStream(false);
	}

	//high-frequency relay: the spectated player's client ships its true camera
	//angles (mouse-look/freelook never replicate) via the entity RPC ingested in
	//PlayerBase.OnRPC; forward to every admin currently spectating that player.
	void RelayLookData(string senderId, float lookYaw, float lookPitch)
	{
		foreach (string adminId, SpectateSession session : m_Sessions)
		{
			if (session.m_TargetId != senderId)
				continue;
			if (!session.m_ClientReady)
				continue;

			PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(adminId);
			if (adminPb && adminPb.GetIdentity())
			{
				GetRPCManager().VSendRPC("RPC_SpectateClient", "TargetLook", new Param2<float, float>(lookYaw, lookPitch), true, adminPb.GetIdentity());

				if (VPPSpectateConstants.ADS_DEBUG)
				{
					int now = GetGame().GetTime();
					if (now - m_DbgLastLookRelay > 5000)
					{
						m_DbgLastLookRelay = now;
						Print("[VPPADS] look stream: server relaying yaw=" + lookYaw.ToString() + " to admin=" + adminId);
					}
				}
			}
		}
	}

	//---------------------------------------------------------------------------------
	// Follow + vitals tick
	//---------------------------------------------------------------------------------
	protected void OnFollowTick()
	{
		if (m_Sessions.Count() == 0)
			return;

		//collect ended sessions after iteration (can't mutate map mid-foreach)
		array<string> toEndDisconnected = {};
		array<string> toEndDied = {};
		array<string> toEndForced = {};

		foreach (string adminId, SpectateSession session : m_Sessions)
		{
			PlayerBase targetPb = GetPermissionManager().GetPlayerBaseByID(session.m_TargetId);
			if (targetPb == null)
			{
				toEndDisconnected.Insert(adminId);
				continue;
			}

			if (!targetPb.IsAlive())
			{
				//death grace: linger on the corpse for the admin's configured delay
				//(the admin can still switch/exit manually during the countdown)
				if (!session.m_TargetDead)
				{
					session.m_TargetDead = true;
					session.m_DeathGraceLeft = GetDeathGraceFor(adminId);
				}

				if (session.m_DeathGraceLeft <= 0)
				{
					toEndDied.Insert(adminId);
					continue;
				}

				session.m_DeathGraceLeft = session.m_DeathGraceLeft - VPPSpectateConstants.FOLLOW_TICK_SEC;
				//fall through: keep the body anchored to the corpse + vitals flowing
			}

			PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(adminId);
			if (adminPb == null)
				continue;

			//the frozen admin body died (godmode toggled off + killed, scripted kill...):
			//terminate so a respawn doesn't get dragged under the target
			if (!adminPb.IsAlive())
			{
				toEndForced.Insert(adminId);
				continue;
			}

			//link-health failsafe: the server-side link keeps the body riding the
			//target, so drift should never grow. If the link silently broke, re-link
			//(the old bare re-teleport would itself break a healthy link — SetPosition
			//on a linked entity). A healthy link sits ~2m from the target.
			if (vector.Distance(adminPb.GetPosition(), targetPb.GetPosition()) > VPPSpectateConstants.FOLLOW_RETELEPORT_DIST)
			{
				adminPb.VPPATDetachFromSpectateTarget();
				TeleportBodyUnder(adminPb, targetPb);
				adminPb.VPPATAttachToSpectateTarget(targetPb);
			}

			//vitals sample to the spectating client
			if (session.m_ClientReady && adminPb.GetIdentity() != null)
			{
				float hpPct = 0.0;
				float maxHp = targetPb.GetMaxHealth("", "Health");
				if (maxHp > 0)
					hpPct = (targetPb.GetHealth("", "Health") / maxHp) * 100.0;
				float blood = targetPb.GetHealth("", "Blood");

				GetRPCManager().VSendRPC("RPC_SpectateClient", "TargetVitals", new Param2<float, float>(hpPct, blood), true, adminPb.GetIdentity());
			}
		}

		foreach (string endId : toEndDisconnected)
		{
			EndSpectate(endId, EVPPSpectateEndReason.TARGET_DISCONNECTED, true);
		}
		foreach (string endId2 : toEndDied)
		{
			EndSpectate(endId2, EVPPSpectateEndReason.TARGET_DIED, true);
		}
		foreach (string endId3 : toEndForced)
		{
			EndSpectate(endId3, EVPPSpectateEndReason.SERVER_FORCED, true);
		}
	}

	//---------------------------------------------------------------------------------
	// Mission hooks (called from missionServer.c)
	//---------------------------------------------------------------------------------
	void OnAdminDisconnect(string uid)
	{
		//restore the body BEFORE logout persists its position
		EndSpectate(uid, EVPPSpectateEndReason.ADMIN_REQUEST, false);
	}

	void OnPlayerConnectRecovery(PlayerBase player, PlayerIdentity identity)
	{
		if (player == null || identity == null)
			return;

		string uid = identity.GetPlainId();
		if (!m_Recovery.positions.Contains(uid))
			return;

		//session still ACTIVE (e.g. logout started then canceled while spectating):
		//don't consume the recovery entry or fight the follow tick
		if (m_Sessions.Contains(uid))
			return;

		vector restorePos = m_Recovery.positions.Get(uid);
		m_Recovery.positions.Remove(uid);
		SaveRecovery();

		//delayed restore: let the login flow settle first, guard against fall/damage briefly
		player.SetAllowDamage(false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RecoverPlayerPosition, 1000, false, uid, restorePos);
	}

	protected void RecoverPlayerPosition(string uid, vector pos)
	{
		PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(uid);
		if (pb == null)
			return;

		pb.SetPosition(pos);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RestoreDamageState, 5000, false, uid);
		GetSimpleLogger().Log(string.Format("Spectate crash-recovery restored steamid=%1 to %2", uid, pos.ToString()));
	}

	protected void RestoreDamageState(string uid)
	{
		PlayerBase pb = GetPermissionManager().GetPlayerBaseByID(uid);
		if (pb == null)
			return;

		//only re-enable damage if they don't have godmode active
		if (!pb.GodModeStatus())
			pb.SetAllowDamage(true);
	}

	void RestoreAllSessions()
	{
		//graceful shutdown/restart: put every spectating admin back
		array<string> ids = {};
		foreach (string adminId, SpectateSession session : m_Sessions)
		{
			ids.Insert(adminId);
		}
		foreach (string id : ids)
		{
			EndSpectate(id, EVPPSpectateEndReason.SERVER_FORCED, true);
		}
	}

	//---------------------------------------------------------------------------------
	// Internals
	//---------------------------------------------------------------------------------
	protected void TeleportBodyUnder(PlayerBase adminPb, PlayerBase targetPb)
	{
		vector tPos = targetPb.GetPosition();
		adminPb.SetPosition(Vector(tPos[0], tPos[1] - VPPSpectateConstants.FOLLOW_Y_OFFSET, tPos[2]));
	}

	//deferred post-exit kick (belt-and-suspenders): re-enable the server-side
	//input controller and force fresh locomotion
	protected void ForceRestoreLocomotion(string adminId)
	{
		PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(adminId);
		if (adminPb == null)
			return;
		if (!adminPb.IsAlive())
			return;

		HumanInputController hic = adminPb.GetInputController();
		if (hic)
			hic.SetDisabled(false);

		if (adminPb.GetCommand_Vehicle() == null)
			adminPb.StartCommand_Move();
	}

	//NO scripted freeze command for spectate (the body LINK corrupts
	//HumanCommandScript_VPPCam — double-free instance leak — and exits could
	//never cleanly hand locomotion back). VPPBR precedent: linked spectators
	//keep their ordinary Move command; only the input controller is held off.
	protected void FreezeAdminBody(PlayerBase adminPb, bool freeze)
	{
		HumanInputController hic = adminPb.GetInputController();
		if (hic)
			hic.SetDisabled(freeze);

		//safety: finish any leftover scripted command (e.g. lingering freecam)
		if (!freeze)
		{
			HumanCommandScript_VPPCam hcs = HumanCommandScript_VPPCam.Cast(adminPb.GetCommand_Script());
			if (hcs)
			{
				hcs.SetFlagFinished(true);
				hcs.m_bNeedFinish = true;
			}
		}
	}

	protected void StartFollowTimer()
	{
		if (m_FollowTimer == null)
			m_FollowTimer = new Timer(CALL_CATEGORY_SYSTEM);

		if (!m_FollowTimer.IsRunning())
			m_FollowTimer.Run(VPPSpectateConstants.FOLLOW_TICK_SEC, this, "OnFollowTick", null, true);

		if (m_ADSTimer == null)
			m_ADSTimer = new Timer(CALL_CATEGORY_SYSTEM);

		if (!m_ADSTimer.IsRunning())
			m_ADSTimer.Run(VPPSpectateConstants.ADS_TICK_SEC, this, "OnADSTick", null, true);
	}

	protected void StopFollowTimer()
	{
		if (m_FollowTimer != null && m_FollowTimer.IsRunning())
			m_FollowTimer.Stop();
		if (m_ADSTimer != null && m_ADSTimer.IsRunning())
			m_ADSTimer.Stop();
	}

	//fast poll of the targets' server-maintained sight state; pushes only on change.
	//HandleADS runs unconditionally in the CommandHandler, so the SERVER knows every
	//player's IsInIronsights/IsInOptics (incl. handheld optics via ActionViewOptics).
	protected void OnADSTick()
	{
		if (m_Sessions.Count() == 0)
			return;

		foreach (string adminId, SpectateSession session : m_Sessions)
		{
			if (!session.m_ClientReady)
				continue;

			int mode = EVPPSpectateSightMode.NONE;
			PlayerBase targetPb = GetPermissionManager().GetPlayerBaseByID(session.m_TargetId);
			if (targetPb && targetPb.IsAlive() && !targetPb.IsUnconscious())
			{
				if (targetPb.IsInOptics())
					mode = EVPPSpectateSightMode.OPTICS;
				else if (targetPb.IsInIronsights())
					mode = EVPPSpectateSightMode.IRONSIGHTS;
			}

			if (mode == session.m_LastSightMode)
				continue; //edge-trigger only

			//commit the edge only after a successful send — a transiently null admin
			//body/identity must not swallow the transition for the rest of the session
			PlayerBase adminPb = GetPermissionManager().GetPlayerBaseByID(adminId);
			if (adminPb && adminPb.GetIdentity())
			{
				GetRPCManager().VSendRPC("RPC_SpectateClient", "TargetADS", new Param1<int>(mode), true, adminPb.GetIdentity());
				session.m_LastSightMode = mode;
			}
		}
	}

	protected void SaveRecovery()
	{
		JsonFileLoader<VPPSpectateRecovery>.JsonSaveFile(RECOVERY_PATH, m_Recovery);
	}

	bool HasSession(string adminId)
	{
		return m_Sessions.Contains(adminId);
	}
};

/* Global getter */
SpectateManager GetSpectateManager()
{
	return SpectateManager.Cast(GetPluginManager().GetPluginByType(SpectateManager));
}
