class VPPCA_WeaponActions
{
	static void Register(VPPContextActionManager mgr)
	{
		if (!mgr)
			return;

		mgr.AddAction(new VPPCA_WeaponJammed());
		mgr.AddAction(new VPPCA_WeaponEjectRound());
		mgr.AddAction(new VPPCA_WeaponRefill());
	}
};

//Base for weapon rows. A weapon held by another player is owner-level checked by the dispatcher.
class VPPCA_WeaponActionBase : VPPContextAction
{
	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return target.GetWeapon() != null;
	}
};

class VPPCA_WeaponJammed : VPPCA_WeaponActionBase
{
	override string GetId()
	{
		return "vpp.weapon.jammed";
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
		return VPPContextConstants.PERM_WEAPON_JAM;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_WPN_JAMMED";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_DOT;
	}

	//Hidden for families whose FSM has no jammed stable state it can leave again
	override bool CanShow(VPPContextTarget target)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return false;

		if (wpn.IsJammed())
			return true;

		return wpn.VPPCanForceJam();
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return false;

		if (wpn.IsJammed())
			return true;

		return wpn.VPPHasRoundForJam();
	}

	override string GetHint(VPPContextTarget target)
	{
		if (!target)
			return "";

		Weapon_Base wpn = target.GetWeapon();
		if (!wpn || wpn.IsJammed())
			return "";

		if (!wpn.VPPHasRoundForJam())
			return "#VSTR_CTX_HINT_NO_ROUND";

		return "";
	}

	//The jam flag follows the FSM state, which Synchronize() pushes to clients; the server state reply wins over the local flag.
	override bool IsChecked(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		Weapon_Base wpn = target.GetWeapon();
		if (wpn)
			return wpn.IsJammed();

		return false;
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return true;
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (wpn)
			outState.Set(GetId(), VPPContextUtils.BoolToState(wpn.IsJammed()));
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		result.LogDetail = wpn.GetType();

		if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_BUSY");
			result.SetState(wpn.IsJammed());
			return;
		}

		if (wpn.IsJammed())
		{
			if (VPPCA_WeaponOps.Unjam(wpn))
			{
				result.Ok("#VSTR_CTX_RESULT_UNJAMMED");
				result.SetState(false);
			}
			else
			{
				result.Fail("#VSTR_CTX_RESULT_INVALID");
				result.SetState(true);
			}
		}
		else if (!VPPCA_WeaponOps.CanJam(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_CANNOT_JAM");
			result.SetState(false);
		}
		else if (!wpn.VPPHasRoundForJam())
		{
			result.Fail("#VSTR_CTX_RESULT_NO_ROUND");
			result.SetState(false);
		}
		else if (VPPCA_WeaponOps.TryJam(wpn))
		{
			result.Ok("#VSTR_CTX_RESULT_JAMMED");
			result.SetState(true);
		}
		else
		{
			result.Fail("#VSTR_CTX_RESULT_CANNOT_JAM");
			result.SetState(wpn.IsJammed());
		}

		VPPCA_WeaponOps.SyncWeaponMagazines(wpn);
	}
};

class VPPCA_WeaponEjectRound : VPPCA_WeaponActionBase
{
	override string GetId()
	{
		return "vpp.weapon.eject_round";
	}

	override int GetOrder()
	{
		return 110;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_WEAPON_EJECT;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_WPN_EJECT";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_EJECT;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return false;

		if (wpn.IsJammed())
			return true;

		int muzzles = wpn.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			if (!wpn.IsChamberEmpty(mi))
				return true;
		}

		return false;
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		result.LogDetail = wpn.GetType();

		if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_BUSY");
			return;
		}

		int ejected = VPPCA_WeaponOps.EjectChambered(wpn);
		VPPCA_WeaponOps.SyncWeaponMagazines(wpn);
		if (ejected > 0)
			result.Ok("#VSTR_CTX_RESULT_EJECTED");
		else
			result.Fail("#VSTR_CTX_RESULT_CHAMBER_EMPTY");
	}
};

class VPPCA_WeaponRefill : VPPCA_WeaponActionBase
{
	override string GetId()
	{
		return "vpp.weapon.refill_magazine";
	}

	override int GetOrder()
	{
		return 120;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_WEAPON_REFILL;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_WPN_REFILL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PACKAGE_PLUS;
	}

	//Hidden for the Magnum cylinder, flare guns and anything else without a magazine or an internal magazine
	override bool CanShow(VPPContextTarget target)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return false;

		return VPPCA_WeaponOps.HasMagazineCapacity(wpn);
	}

	//enabled state and hint come from the server when available: the client copy of an attached magazine can lag the server
	override bool IsEnabled(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return false;

		return VPPCA_WeaponOps.CanRefill(wpn);
	}

	override string GetHint(VPPContextTarget target)
	{
		if (!target)
			return "";

		string hintKey = GetId() + VPPCA_WeaponOps.STATE_HINT_SUFFIX;
		if (target.HasState(hintKey))
			return target.GetState(hintKey);

		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return "";

		return VPPCA_WeaponOps.BuildAmmoHint(wpn);
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return true;
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
			return;

		outState.Set(GetId(), VPPContextUtils.BoolToState(VPPCA_WeaponOps.CanRefill(wpn)));
		outState.Set(GetId() + VPPCA_WeaponOps.STATE_HINT_SUFFIX, VPPCA_WeaponOps.BuildAmmoHint(wpn));
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		string before = VPPCA_WeaponOps.BuildAmmoHint(wpn);
		result.LogDetail = wpn.GetType() + " " + before;

		if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_BUSY");
			return;
		}

		if (!VPPCA_WeaponOps.HasMagazineCapacity(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_NO_MAGAZINE");
			return;
		}

		if (!VPPCA_WeaponOps.CanRefill(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_ALREADY_FULL");
			return;
		}

		int refilled = VPPCA_WeaponOps.Refill(wpn);
		string after = VPPCA_WeaponOps.BuildAmmoHint(wpn);
		result.LogDetail = wpn.GetType() + " " + before + " -> " + after;
		if (refilled > 0)
			result.Ok("#VSTR_CTX_RESULT_REFILLED");
		else
			result.Fail("#VSTR_CTX_RESULT_INVALID");
	}
};
