class VPPCA_EntityActions
{
	static void Register(VPPContextActionManager mgr)
	{
		if (!mgr)
			return;

		mgr.AddAction(new VPPCA_CommonCopyGroup());
		mgr.AddAction(new VPPCA_CommonCopyPosition());
		mgr.AddAction(new VPPCA_CommonCopyType());
		mgr.AddAction(new VPPCA_CommonDelete());
		mgr.AddAction(new VPPCA_ItemRepair());
		mgr.AddAction(new VPPCA_ItemRuin());
		mgr.AddAction(new VPPCA_ItemFill());
		mgr.AddAction(new VPPCA_ItemEmpty());
		mgr.AddAction(new VPPCA_VehicleRepair());
		mgr.AddAction(new VPPCA_VehicleRepairParts());
		mgr.AddAction(new VPPCA_VehicleRefuel());
		mgr.AddAction(new VPPCA_CreatureHeal());
		mgr.AddAction(new VPPCA_CreatureKill());
		mgr.AddAction(new VPPCA_EntityAttachments());
		mgr.AddAction(new VPPCA_AttachSpawn());
		mgr.AddAction(new VPPCA_WeaponLoadMagazine());
		mgr.AddAction(new VPPCA_WeaponLoadAmmo());
	}
};

//Log text shared by the entity actions: "<type> @ x, y, z"
class VPPCA_EntityDetail
{
	static string Describe(Object obj)
	{
		if (!obj)
			return "";

		string detail = obj.GetType() + " @ ";
		detail += VPPContextUtils.FormatPosition(obj.GetPosition());
		return detail;
	}
};

//"Copy >" group. Visible for any target; the children decide what is shown (P2 adds vpp.player.copy_id here).
class VPPCA_CommonCopyGroup : VPPContextAction
{
	override string GetId()
	{
		return "vpp.common.copy";
	}

	override int GetOrder()
	{
		return VPPContextConstants.ORDER_INFO;
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
		return "#VSTR_CTX_COPY";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_COPY;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		return target != null;
	}
};

//Base for generic item operations. Items inside another player's inventory are owner-level checked by the dispatcher.
class VPPCA_ItemActionBase : VPPContextAction
{
	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return target.GetItem() != null;
	}
};
