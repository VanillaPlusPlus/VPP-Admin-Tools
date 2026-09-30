class VPPCA_PlayerHeal : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.heal";
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return "PlayerManager:HealPlayers";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_HEAL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_HEART;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "HealPlayers", new Param1<ref array<string>>(CopyIds(target)), true);
		return true;
	}
};

class VPPCA_PlayerStopBleeding : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.stop_bleeding";
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return "PlayerManager:StopBleeding";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_STOP_BLEEDING";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_BANDAGE;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "StopBleedingPlayers", new Param1<ref array<string>>(CopyIds(target)), true);
		return true;
	}
};

//class name frozen: the 5_Mission VPPContextSpectateAction subclasses it and swaps it in via ReplaceAction
class VPPCA_PlayerSpectate : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.spectate";
	}

	override int GetOrder()
	{
		return 120;
	}

	override string GetPermission()
	{
		return "PlayerManager:SpectatePlayer";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_SPECTATE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_EYE;
	}

	override bool CanShow(VPPContextTarget target)
	{
		if (g_Game.IsSpectateMode())
			return false;

		return !IsSelf(target);
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "SpectatePlayer", new Param1<string>(target.GetPlayerId()), true);
		return true;
	}
};

class VPPCA_PlayerInHands : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.in_hands";
	}

	override int GetOrder()
	{
		return 130;
	}

	override int GetKind()
	{
		return EVPPContextKind.SUBMENU;
	}

	override string GetPermission()
	{
		return "";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_IN_HANDS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_SWORD;
	}

	override string GetHint(VPPContextTarget target)
	{
		return VPPContextUtils.GetObjectDisplayName(HandsOf(target));
	}

	override bool CanShow(VPPContextTarget target)
	{
		return HandsOf(target) != null;
	}

	override VPPContextTarget GetSubmenuTarget(VPPContextTarget target)
	{
		EntityAI hands = HandsOf(target);
		if (!hands)
			return null;

		return target.Retarget(hands);
	}

	//drills into the held item's own root page
	override string GetSubmenuId(VPPContextTarget target)
	{
		return "";
	}
};

class VPPCA_PlayerCopyId : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.copy_id";
	}

	override string GetParentId()
	{
		return VPPContextConstants.GROUP_COPY;
	}

	override int GetOrder()
	{
		return 90;
	}

	override string GetPermission()
	{
		return "";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_COPY_ID";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_COPY;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetGame().CopyToClipboard(target.GetPlayerId());
		VPPContextActionManager.ClientNotify("#VSTR_CTX_RESULT_COPIED", target.GetPlayerId());
		return true;
	}
};

class VPPCA_PlayerClearInventory : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.clear_inventory";
	}

	override int GetOrder()
	{
		return 9000;
	}

	override string GetPermission()
	{
		return "PlayerManager:ClearInventory";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_CLEAR_INVENTORY";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PACKAGE_X;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "ClearInventory", new Param1<ref array<string>>(CopyIds(target)), true);
		return true;
	}
};

class VPPCA_PlayerKill : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.kill";
	}

	override int GetOrder()
	{
		return 9100;
	}

	override string GetPermission()
	{
		return "PlayerManager:KillPlayers";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_KILL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_SKULL;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "KillPlayers", new Param1<ref array<string>>(CopyIds(target)), true);
		return true;
	}
};
