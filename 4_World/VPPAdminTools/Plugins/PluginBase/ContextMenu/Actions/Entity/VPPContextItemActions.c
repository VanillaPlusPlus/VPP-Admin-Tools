class VPPCA_ItemRepair : VPPCA_ItemActionBase
{
	override string GetId()
	{
		return "vpp.item.repair";
	}

	override int GetOrder()
	{
		return 5000;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_ITEM_HEALTH;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_ITEM_REPAIR";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_HAMMER;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		ItemBase item = target.GetItem();
		if (!item)
			return false;

		//0 = pristine
		return item.GetHealthLevel("") > 0;
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		ItemBase item = target.GetItem();
		if (!item)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		item.SetFullHealth();
		result.Ok("#VSTR_CTX_RESULT_REPAIRED");
		result.LogDetail = VPPCA_EntityDetail.Describe(item);
	}
};

class VPPCA_ItemFill : VPPCA_ItemActionBase
{
	override string GetId()
	{
		return "vpp.item.fill";
	}

	override int GetOrder()
	{
		return 5100;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_ITEM_QUANTITY;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_ITEM_FILL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PLUS;
	}

	protected bool CanFillItem(ItemBase item)
	{
		if (!item)
			return false;

		Magazine mag = Magazine.Cast(item);
		if (mag)
			return mag.GetAmmoCount() < mag.GetAmmoMax();

		if (!item.HasQuantity())
			return false;

		return !item.IsFullQuantity();
	}

	//magazine rows stay visible: an attached magazine's client count can lag the server, so the server state decides enabled and hint
	override bool CanShow(VPPContextTarget target)
	{
		ItemBase item = target.GetItem();
		if (Magazine.Cast(item))
			return true;

		return CanFillItem(item);
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		if (!target)
			return false;

		ItemBase item = target.GetItem();
		if (!Magazine.Cast(item))
			return item != null;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		return CanFillItem(item);
	}

	override string GetHint(VPPContextTarget target)
	{
		if (!target)
			return "";

		Magazine mag = Magazine.Cast(target.GetItem());
		if (!mag)
			return "";

		string hintKey = GetId() + VPPCA_WeaponOps.STATE_HINT_SUFFIX;
		if (target.HasState(hintKey))
			return target.GetState(hintKey);

		return BuildMagazineHint(mag);
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		if (!target)
			return false;

		return Magazine.Cast(target.GetItem()) != null;
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		Magazine mag = Magazine.Cast(target.GetItem());
		if (!mag)
			return;

		outState.Set(GetId(), VPPContextUtils.BoolToState(mag.GetAmmoCount() < mag.GetAmmoMax()));
		outState.Set(GetId() + VPPCA_WeaponOps.STATE_HINT_SUFFIX, BuildMagazineHint(mag));
	}

	protected string BuildMagazineHint(Magazine mag)
	{
		if (!mag)
			return "";

		string hint = mag.GetAmmoCount().ToString() + "/";
		hint += mag.GetAmmoMax().ToString();
		return hint;
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		ItemBase item = target.GetItem();
		if (!item)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		Magazine mag = Magazine.Cast(item);
		if (mag)
		{
			if (mag.GetAmmoCount() >= mag.GetAmmoMax())
			{
				result.Fail("#VSTR_CTX_RESULT_ALREADY_FULL");
				return;
			}

			mag.ServerSetAmmoMax();
			VPPCA_WeaponOps.SyncMagazineAmmo(mag);
		}
		else
		{
			if (!item.HasQuantity())
			{
				result.Fail("#VSTR_CTX_RESULT_INVALID");
				return;
			}

			if (item.IsFullQuantity())
			{
				result.Fail("#VSTR_CTX_RESULT_ALREADY_FULL");
				return;
			}

			item.SetQuantityMax();
		}

		result.Ok("#VSTR_CTX_RESULT_FILLED");
		result.LogDetail = VPPCA_EntityDetail.Describe(item);
	}
};

class VPPCA_ItemEmpty : VPPCA_ItemActionBase
{
	override string GetId()
	{
		return "vpp.item.empty";
	}

	override int GetOrder()
	{
		return 5110;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_ITEM_QUANTITY;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_ITEM_EMPTY";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_MINUS;
	}

	//ammo piles are excluded: an empty pile is not a valid item
	protected bool CanEmptyItem(ItemBase item)
	{
		if (!item)
			return false;

		Magazine mag = Magazine.Cast(item);
		if (mag)
		{
			if (Ammunition_Base.Cast(item))
				return false;

			return mag.GetAmmoCount() > 0;
		}

		if (!item.HasQuantity())
			return false;

		return item.GetQuantity() > item.GetQuantityMin();
	}

	override bool CanShow(VPPContextTarget target)
	{
		return CanEmptyItem(target.GetItem());
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		ItemBase item = target.GetItem();
		if (!CanEmptyItem(item))
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		Magazine mag = Magazine.Cast(item);
		if (mag)
		{
			mag.ServerSetAmmoCount(0);
			VPPCA_WeaponOps.SyncMagazineAmmo(mag);
		}
		else
		{
			item.SetQuantity(item.GetQuantityMin(), false);	//destroy_config false: varQuantityDestroyOnMin items are kept
		}

		result.Ok("#VSTR_CTX_RESULT_EMPTIED");
		result.LogDetail = VPPCA_EntityDetail.Describe(item);
	}
};

//Danger band (9800) so destructive rows cluster at the bottom above Delete.
class VPPCA_ItemRuin : VPPCA_ItemActionBase
{
	override string GetId()
	{
		return "vpp.item.ruin";
	}

	override int GetOrder()
	{
		return 9800;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_ITEM_HEALTH;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_ITEM_RUIN";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_FLAME;
	}

	override bool CanShow(VPPContextTarget target)
	{
		return !VPPContextUtils.IsExplosive(target.GetItem());
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		ItemBase item = target.GetItem();
		if (!item)
			return false;

		return !item.IsDamageDestroyed();
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		ItemBase item = target.GetItem();
		if (!item)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		//health 0 detonates ExplosivesBase / Grenade_Base
		if (VPPContextUtils.IsExplosive(item))
		{
			result.Fail("#VSTR_CTX_RESULT_EXPLOSIVE");
			return;
		}

		item.SetHealth("", "", 0);
		result.Ok("#VSTR_CTX_RESULT_RUINED");
		result.LogDetail = VPPCA_EntityDetail.Describe(item);
	}
};
