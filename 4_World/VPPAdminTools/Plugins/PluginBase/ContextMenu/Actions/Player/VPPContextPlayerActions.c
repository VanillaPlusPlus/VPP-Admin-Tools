class VPPCA_PlayerActions
{
	static void Register(VPPContextActionManager mgr)
	{
		if (!mgr)
			return;

		mgr.AddAction(new VPPCA_PlayerHeal());
		mgr.AddAction(new VPPCA_PlayerStopBleeding());
		mgr.AddAction(new VPPCA_PlayerSpectate());
		mgr.AddAction(new VPPCA_PlayerInHands());
		mgr.AddAction(new VPPCA_PlayerStatusGroup());
		mgr.AddAction(new VPPCA_PlayerGodmode());
		mgr.AddAction(new VPPCA_PlayerUnlimitedAmmo());
		mgr.AddAction(new VPPCA_PlayerInvisible());
		mgr.AddAction(new VPPCA_PlayerFrozen());
		mgr.AddAction(new VPPCA_PlayerTeleportGroup());
		mgr.AddAction(new VPPCA_PlayerTeleportGoto());
		mgr.AddAction(new VPPCA_PlayerTeleportBring());
		mgr.AddAction(new VPPCA_PlayerTeleportReturn());
		mgr.AddAction(new VPPCA_PlayerModerationGroup());
		mgr.AddAction(new VPPCA_PlayerMessage());
		mgr.AddAction(new VPPCA_PlayerKick());
		mgr.AddAction(new VPPCA_PlayerBan());
		mgr.AddAction(new VPPCA_PlayerCopyId());
		mgr.AddAction(new VPPCA_PlayerClearInventory());
		mgr.AddAction(new VPPCA_PlayerKill());
	}
};

/*
	Shared base for built-in player actions. These only forward to the existing
	RPC_PlayerManager handlers, which own permission checks, logging and webhooks.
*/
class VPPCA_PlayerActionBase : VPPContextAction
{
	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || !target.IsPlayer())
			return false;

		if (target.IsMulti() && !SupportsMultiTarget())
			return false;

		return true;
	}

	//PlayerManager:* permissions are registered by PermissionManager itself
	override bool AutoRegisterPermission()
	{
		return false;
	}

	protected array<string> CopyIds(VPPContextTarget t)
	{
		array<string> ids = new array<string>;
		if (t && t.GetPlayerIds())
			ids.Copy(t.GetPlayerIds());

		return ids;
	}

	protected array<int> CopySessions(VPPContextTarget t)
	{
		array<int> sessions = new array<int>;
		if (!t || !t.GetSessionIds())
			return sessions;

		array<int> src = t.GetSessionIds();
		for (int i = 0; i < src.Count(); i++)
		{
			if (src[i] >= 0)
				sessions.Insert(src[i]);
		}

		return sessions;
	}

	protected bool IsSelf(VPPContextTarget t)
	{
		if (!t)
			return false;

		return t.GetPlayerId() == g_Game.VPPAT_GetSteam64Id();
	}

	protected EntityAI HandsOf(VPPContextTarget t)
	{
		if (!t)
			return null;

		PlayerBase pb = t.GetPlayer();
		if (pb)
			return pb.GetHumanInventory().GetEntityInHands();

		return null;
	}
};

class VPPCA_PlayerStatusGroup : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.status";
	}

	override int GetOrder()
	{
		return 1000;
	}

	override int GetKind()
	{
		return EVPPContextKind.SUBMENU;
	}

	override string GetPermission()
	{
		return "";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_STATUS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_ACTIVITY;
	}
};

class VPPCA_PlayerTeleportGroup : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.teleport";
	}

	override int GetOrder()
	{
		return 1010;
	}

	override int GetKind()
	{
		return EVPPContextKind.SUBMENU;
	}

	override string GetPermission()
	{
		return "";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_TELEPORT";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_MOVE;
	}
};

class VPPCA_PlayerModerationGroup : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.moderation";
	}

	override int GetOrder()
	{
		return 1020;
	}

	override int GetKind()
	{
		return EVPPContextKind.SUBMENU;
	}

	override string GetPermission()
	{
		return "";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_MODERATION";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_GAVEL;
	}
};
