class VPPCA_PlayerMessage : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.moderation.message";
	}

	override string GetParentId()
	{
		return "vpp.player.moderation";
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return "PlayerManager:SendMessage";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_MESSAGE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_MESSAGE;
	}

	override string GetInputPrompt(VPPContextTarget target)
	{
		return "#VSTR_TOOLTIP_SEND_MSG";
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		string msg = args.GetInput();
		if (msg == "")
			return true;

		GetRPCManager().VSendRPC("RPC_PlayerManager", "SendMessage", new Param3<string,string,ref array<string>>("Server Admin:", msg, CopyIds(target)), true);
		return true;
	}
};

class VPPCA_PlayerKick : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.moderation.kick";
	}

	override string GetParentId()
	{
		return "vpp.player.moderation";
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return "PlayerManager:KickPlayer";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	//the reason prompt already acts as the confirmation step
	override bool RequiresConfirm(VPPContextTarget target)
	{
		return false;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_KICK";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_KICK;
	}

	override string GetInputPrompt(VPPContextTarget target)
	{
		return "#VSTR_TOOLTIP_KICK_REASON";
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		string reason = args.GetInput();
		if (reason == "")
			reason = "#VSTR_NOTIFY_KICK_MESSAGE_PLAYER";

		GetRPCManager().VSendRPC("RPC_PlayerManager", "KickPlayer", new Param2<ref array<string>,string>(CopyIds(target), reason), true);
		return true;
	}
};

class VPPCA_PlayerBan : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.moderation.ban";
	}

	override string GetParentId()
	{
		return "vpp.player.moderation";
	}

	override int GetOrder()
	{
		return 120;
	}

	override string GetPermission()
	{
		return "PlayerManager:BanPlayer";
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
		return "#VSTR_CTX_PLR_BAN";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_BAN;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "BanPlayer", new Param1<ref array<string>>(CopyIds(target)), true);
		return true;
	}
};
