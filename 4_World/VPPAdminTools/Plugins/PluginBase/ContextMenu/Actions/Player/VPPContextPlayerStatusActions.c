class VPPCA_PlayerGodmode : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.status.godmode";
	}

	override string GetParentId()
	{
		return "vpp.player.status";
	}

	override int GetOrder()
	{
		return 100;
	}

	override int GetKind()
	{
		return EVPPContextKind.TOGGLE;
	}

	override string GetPermission()
	{
		return "PlayerManager:GiveGodmode";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_GODMODE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_ACTIVITY;
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		PlayerBase pb = target.GetPlayer();
		if (pb)
			outState.Set(GetId(), VPPContextUtils.BoolToState(pb.GodModeStatus()));
	}

	override bool IsChecked(VPPContextTarget target)
	{
		if (target.IsMulti())
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		PlayerBase pb = target.GetPlayer();
		if (pb)
			return pb.GodModeStatus();

		return false;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GiveGodmode", new Param1<string>(target.GetPlayerId()), true);
		return true;
	}
};

class VPPCA_PlayerUnlimitedAmmo : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.status.unlimited_ammo";
	}

	override string GetParentId()
	{
		return "vpp.player.status";
	}

	override int GetOrder()
	{
		return 110;
	}

	override int GetKind()
	{
		return EVPPContextKind.TOGGLE;
	}

	override string GetPermission()
	{
		return "PlayerManager:GiveUnlimitedAmmo";
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_UNLIMITED_AMMO";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PACKAGE_PLUS;
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		PlayerBase pb = target.GetPlayer();
		if (pb)
			outState.Set(GetId(), VPPContextUtils.BoolToState(pb.VPPIsUnlimitedAmmo()));
	}

	override bool IsChecked(VPPContextTarget target)
	{
		if (target.IsMulti())
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		PlayerBase pb = target.GetPlayer();
		if (pb)
			return pb.VPPIsUnlimitedAmmo();

		return false;
	}

	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "GiveUnlimitedAmmo", new Param1<string>(target.GetPlayerId()), true);
		return true;
	}
};

class VPPCA_PlayerInvisible : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.status.invisible";
	}

	override string GetParentId()
	{
		return "vpp.player.status";
	}

	override int GetOrder()
	{
		return 120;
	}

	override int GetKind()
	{
		return EVPPContextKind.TOGGLE;
	}

	override string GetPermission()
	{
		return "PlayerManager:SetPlayerInvisible";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_INVISIBLE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_EYE;
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		PlayerBase pb = target.GetPlayer();
		if (pb)
			outState.Set(GetId(), VPPContextUtils.BoolToState(pb.InvisibilityStatus()));
	}

	override bool IsChecked(VPPContextTarget target)
	{
		if (target.IsMulti())
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		PlayerBase pb = target.GetPlayer();
		if (pb)
			return pb.InvisibilityStatus();

		return false;
	}

	//RequestInvisibility takes SESSION ids, not steam64s
	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		array<int> sessions = CopySessions(target);
		if (sessions.Count() == 0)
			return true;

		GetRPCManager().VSendRPC("RPC_PlayerManager", "RequestInvisibility", new Param1<ref array<int>>(sessions), true);
		return true;
	}
};

class VPPCA_PlayerFrozen : VPPCA_PlayerActionBase
{
	override string GetId()
	{
		return "vpp.player.status.frozen";
	}

	override string GetParentId()
	{
		return "vpp.player.status";
	}

	override int GetOrder()
	{
		return 130;
	}

	override int GetKind()
	{
		return EVPPContextKind.TOGGLE;
	}

	override string GetPermission()
	{
		return "PlayerManager:FreezePlayers";
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_PLR_FROZEN";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_ACTIVITY;
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		PlayerBase pb = target.GetPlayer();
		if (pb)
			outState.Set(GetId(), VPPContextUtils.BoolToState(pb.VPPIsFreezeControls()));
	}

	override bool IsChecked(VPPContextTarget target)
	{
		if (target.IsMulti())
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		PlayerBase pb = target.GetPlayer();
		if (pb)
			return pb.VPPIsFreezeControls();

		return false;
	}

	//state -1 = toggle per player on the server
	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		GetRPCManager().VSendRPC("RPC_PlayerManager", "FreezePlayers", new Param2<ref array<string>,int>(CopyIds(target), -1), true);
		return true;
	}
};
