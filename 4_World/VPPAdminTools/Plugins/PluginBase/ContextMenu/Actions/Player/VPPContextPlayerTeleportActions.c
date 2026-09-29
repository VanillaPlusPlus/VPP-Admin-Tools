/*
	PlayerManager.TeleportHandle (the reused server handler) requires BOTH
	PlayerManager:TeleportToPlayer and PlayerManager:TeleportPlayerTo for every
	teleport type. Each row keeps its contract permission string and also hides
	itself when the client permission hint denies the other one.
*/
class VPPCA_PlayerTeleportActionBase : VPPCA_PlayerActionBase
{
	protected bool HasBothTeleportPermissions()
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr)
			return true;

		if (!mgr.HasPermissionHint("PlayerManager:TeleportToPlayer"))
			return false;

		return mgr.HasPermissionHint("PlayerManager:TeleportPlayerTo");
	}

	override bool CanShow(VPPContextTarget target)
	{
		return HasBothTeleportPermissions();
	}
};

class VPPCA_PlayerTeleportGoto : VPPCA_PlayerTeleportActionBase
{
	override string GetId()
	{
		return "vpp.player.teleport.goto";
	}

	override string GetParentId()
	{
		return "vpp.player.teleport";
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return "PlayerManager:TeleportToPlayer";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_TP_GOTO";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_LOCATE;
	}

	override bool CanShow(VPPContextTarget target)
	{
		if (IsSelf(target))
			return false;

		return super.CanShow(target);
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Param2<VPPAT_TeleportType,ref array<string>> data = new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.GOTO, CopyIds(target));
		GetRPCManager().VSendRPC("RPC_PlayerManager", "TeleportHandle", data, true);
		return true;
	}
};

class VPPCA_PlayerTeleportBring : VPPCA_PlayerTeleportActionBase
{
	override string GetId()
	{
		return "vpp.player.teleport.bring";
	}

	override string GetParentId()
	{
		return "vpp.player.teleport";
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return "PlayerManager:TeleportPlayerTo";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_TP_BRING";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_BRING;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Param2<VPPAT_TeleportType,ref array<string>> data = new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.BRING, CopyIds(target));
		GetRPCManager().VSendRPC("RPC_PlayerManager", "TeleportHandle", data, true);
		return true;
	}
};

class VPPCA_PlayerTeleportReturn : VPPCA_PlayerTeleportActionBase
{
	override string GetId()
	{
		return "vpp.player.teleport.return";
	}

	override string GetParentId()
	{
		return "vpp.player.teleport";
	}

	override int GetOrder()
	{
		return 120;
	}

	override string GetPermission()
	{
		return "PlayerManager:TeleportPlayerTo";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_TP_RETURN";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_RETURN;
	}

	override bool RequiresConfirm(VPPContextTarget target)
	{
		return true;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		Param2<VPPAT_TeleportType,ref array<string>> data = new Param2<VPPAT_TeleportType,ref array<string>>(VPPAT_TeleportType.RETURN, CopyIds(target));
		GetRPCManager().VSendRPC("RPC_PlayerManager", "TeleportHandle", data, true);
		return true;
	}
};
