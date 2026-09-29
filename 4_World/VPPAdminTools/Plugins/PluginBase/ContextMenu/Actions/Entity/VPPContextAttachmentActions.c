//"Equipment >" on Man, "Attachments >" on everything else. Every sub-page is a drill-in on a CloneTarget carrying
//EXTRA_ATT_* Extras and is built here; the manager hides the row by itself when OnBuildChildren adds nothing.
class VPPCA_EntityAttachments : VPPContextAction
{
	override string GetId()
	{
		return "vpp.entity.attachments";
	}

	override int GetOrder()
	{
		return VPPContextConstants.ORDER_ATTACHMENTS;
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
		if (!target)
			return "#VSTR_CTX_ATT_ATTACHMENTS";

		EntityAI ent = target.GetEntity();
		if (ent && ent.IsMan())
			return "#VSTR_CTX_ATT_EQUIPMENT";

		if (target.GetWeapon())
			return "#VSTR_CTX_WPN_ATTACHMENTS";

		return "#VSTR_CTX_ATT_ATTACHMENTS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_BOXES;
	}

	//runs on the server too: side-effect free
	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsMulti())
			return false;

		EntityAI ent = target.GetEntity();
		if (!ent)
			return false;

		return ent.GetInventory() != null;
	}

	override string GetHint(VPPContextTarget target)
	{
		if (!target)
			return "";

		EntityAI ent = target.GetEntity();
		if (!ent || !ent.GetInventory())
			return "";

		int present = 0;
		Weapon_Base wpn = Weapon_Base.Cast(ent);
		if (wpn)
		{
			int muzzles = wpn.GetMuzzleCount();
			for (int mi = 0; mi < muzzles; mi++)
			{
				if (wpn.GetMagazine(mi))
					present++;
			}
		}

		GameInventory inv = ent.GetInventory();
		int slotCount = inv.GetAttachmentSlotsCount();
		for (int i = 0; i < slotCount; i++)
		{
			int slotId = inv.GetAttachmentSlotId(i);
			if (VPPCA_AttachOps.IsHiddenSlot(ent, slotId))
				continue;

			EntityAI att = inv.FindAttachment(slotId);
			if (!att)
				continue;

			if (wpn && VPPCA_AttachOps.IsMuzzleMagazine(wpn, att))
				continue;

			present++;
		}

		if (present <= 0)
			return "";

		return present.ToString();
	}

	protected VPPContextTarget PageTarget(VPPContextTarget source, string mode, string extraKey, string extraValue)
	{
		VPPContextTarget pageClone = source.CloneTarget();
		pageClone.SetExtra(VPPContextConstants.EXTRA_ATT_MODE, mode);
		if (extraKey != "")
			pageClone.SetExtra(extraKey, extraValue);

		return pageClone;
	}

	override void OnBuildChildren(VPPContextTarget target, VPPContextItems items)
	{
		if (!target || !items)
			return;

		EntityAI ent = target.GetEntity();
		if (!ent || !ent.GetInventory())
			return;

		string mode = target.GetExtra(VPPContextConstants.EXTRA_ATT_MODE);
		if (mode == "")
			BuildOverview(target, ent, items);
		else if (mode == VPPContextConstants.ATT_MODE_SLOT)
			BuildSlotPage(target, ent, items);
		else if (mode == VPPContextConstants.ATT_MODE_MAG)
			BuildMagazinePage(target, ent, items);
		else if (mode == VPPContextConstants.ATT_MODE_AMMO)
			BuildAmmoPage(target, ent, items);
	}

	//MUST stay cheap: HasVisibleChildrenAt runs it on every root build. No index, no config scans, native slot calls only.
	protected void BuildOverview(VPPContextTarget target, EntityAI ent, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr)
			return;

		GameInventory inv = ent.GetInventory();
		if (!inv)
			return;

		bool canSpawn = mgr.HasPermissionHint(VPPContextConstants.PERM_SPAWN_ATTACHMENT);
		bool canLoad = mgr.HasPermissionHint(VPPContextConstants.PERM_WEAPON_LOAD);
		Weapon_Base wpn = Weapon_Base.Cast(ent);

		if (wpn && VPPCA_WeaponOps.SupportsLoading(wpn))
		{
			int muzzles = wpn.GetMuzzleCount();
			int magMuzzles = 0;
			for (int mc = 0; mc < muzzles; mc++)
			{
				if (wpn.GetMagazineTypeCount(mc) > 0)
					magMuzzles++;
			}

			bool anyAmmo = false;
			for (int mi = 0; mi < muzzles; mi++)
			{
				if (wpn.GetRandomChamberableAmmoTypeName(mi) != "")
					anyAmmo = true;

				if (wpn.GetMagazineTypeCount(mi) <= 0)
					continue;

				string magLabel = "#VSTR_CTX_WPN_MAGAZINE";
				if (magMuzzles > 1)
				{
					int muzzleNo = mi + 1;
					magLabel = Widget.TranslateString("#VSTR_CTX_WPN_MAGAZINE") + " " + muzzleNo.ToString();
				}

				string magRowId = "vpp.entity.attachments.mag." + mi.ToString();
				Magazine mag = wpn.GetMagazine(mi);
				string magHint = "#VSTR_CTX_EMPTY_SLOT";
				if (mag)
				{
					int magAmmo = mag.GetAmmoCount();
					int magMax = mag.GetAmmoMax();
					magHint = magAmmo.ToString() + "/";
					magHint += magMax.ToString();
				}

				if (!mag && !canLoad)
				{
					items.AddInfo(magRowId, magLabel, VPPContextConstants.ICON_PACKAGE, 10 + mi, magHint);
					continue;
				}

				VPPContextTarget magPage = PageTarget(target, VPPContextConstants.ATT_MODE_MAG, VPPContextConstants.EXTRA_ATT_MUZZLE, mi.ToString());
				VPPContextItem magRow = items.AddSubmenu(magRowId, magLabel, VPPContextConstants.ICON_PACKAGE, 10 + mi, magPage, VPPContextConstants.ID_ATTACHMENTS);
				if (magRow)
					magRow.Hint = magHint;
			}

			if (anyAmmo && canLoad)
			{
				VPPContextTarget ammoPage = PageTarget(target, VPPContextConstants.ATT_MODE_AMMO, "", "");
				VPPContextItem ammoRow = items.AddSubmenu("vpp.entity.attachments.ammo", "#VSTR_CTX_WPN_AMMO", VPPContextConstants.ICON_CROSSHAIR, 20, ammoPage, VPPContextConstants.ID_ATTACHMENTS);
				int cm = wpn.GetCurrentMuzzle();
				if (ammoRow && wpn.HasInternalMagazine(cm))
				{
					int innerCount = wpn.GetInternalMagazineCartridgeCount(cm);
					int innerMax = wpn.GetInternalMagazineMaxCartridgeCount(cm);
					string ammoHint = innerCount.ToString() + "/";
					ammoHint += innerMax.ToString();
					ammoRow.Hint = ammoHint;
				}
			}
		}

		int slotCount = inv.GetAttachmentSlotsCount();
		for (int i = 0; i < slotCount; i++)
		{
			int slotId = inv.GetAttachmentSlotId(i);
			if (VPPCA_AttachOps.IsHiddenSlot(ent, slotId))
				continue;

			EntityAI att = inv.FindAttachment(slotId);
			if (wpn && att && VPPCA_AttachOps.IsMuzzleMagazine(wpn, att))
				continue;

			string slotName = InventorySlots.GetSlotName(slotId);
			string slotLabel = VPPCA_AttachOps.SlotLabel(slotId);
			string slotRowId = "vpp.entity.attachments.slot." + slotName;
			string slotHint = "#VSTR_CTX_EMPTY_SLOT";
			if (att)
				slotHint = VPPContextUtils.GetObjectDisplayName(att);

			//read-only look of the old weapon submenu for admins who cannot spawn
			if (!att && !canSpawn)
			{
				items.AddInfo(slotRowId, slotLabel, VPPContextConstants.ICON_BOXES, 100 + i, slotHint);
				continue;
			}

			VPPContextTarget slotPage = PageTarget(target, VPPContextConstants.ATT_MODE_SLOT, VPPContextConstants.EXTRA_ATT_SLOT, slotName);
			VPPContextItem slotRow = items.AddSubmenu(slotRowId, slotLabel, VPPContextConstants.ICON_BOXES, 100 + i, slotPage, VPPContextConstants.ID_ATTACHMENTS);
			if (slotRow)
				slotRow.Hint = slotHint;
		}
	}

	protected void BuildSlotPage(VPPContextTarget target, EntityAI ent, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr)
			return;

		GameInventory inv = ent.GetInventory();
		string slotName = target.GetExtra(VPPContextConstants.EXTRA_ATT_SLOT);
		int slotId = InventorySlots.GetSlotIdFromString(slotName);
		if (slotId == InventorySlots.INVALID || !inv || !inv.HasAttachmentSlot(slotId))
			return;

		EntityAI occupant = inv.FindAttachment(slotId);
		string currentType = "";
		if (occupant)
			currentType = occupant.GetType();

		if (occupant && !target.HasExtra(VPPContextConstants.EXTRA_ATT_PAGE))
			AddCurrentRows(target, occupant, items);

		if (VPPCA_AttachOps.IsLockedOccupied(ent, slotId))
		{
			items.AddInfo("vpp.entity.attachments.locked", "#VSTR_CTX_ATT_LOCKED", VPPContextConstants.ICON_INFO, 1000, "");
			return;
		}

		VPPContextArgs baseArgs = new VPPContextArgs();
		baseArgs.Set(VPPContextConstants.ARG_ATT_SLOT, slotName);

		array<string> types = VPPATInventorySlots.GetSpawnableForSlot(slotName);
		AddTypeRows(target, types, currentType, occupant != null, mgr.GetAction(VPPContextConstants.ID_ATTACH_SPAWN), baseArgs, items);
	}

	protected void BuildMagazinePage(VPPContextTarget target, EntityAI ent, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr)
			return;

		Weapon_Base wpn = Weapon_Base.Cast(ent);
		if (!wpn || !VPPCA_WeaponOps.SupportsLoading(wpn))
			return;

		string muzzleText = target.GetExtra(VPPContextConstants.EXTRA_ATT_MUZZLE);
		int mi = muzzleText.ToInt();
		if (mi < 0 || mi >= wpn.GetMuzzleCount())
			return;

		Magazine mag = wpn.GetMagazine(mi);
		string currentType = "";
		if (mag)
			currentType = mag.GetType();

		if (mag && !target.HasExtra(VPPContextConstants.EXTRA_ATT_PAGE))
			AddCurrentRows(target, mag, items);

		VPPContextArgs baseArgs = new VPPContextArgs();
		baseArgs.Set(VPPContextConstants.ARG_ATT_MUZZLE, mi.ToString());

		array<string> magTypes = VPPCA_WeaponOps.GetMagazineTypes(wpn, mi);
		array<string> types = VPPATInventorySlots.SortByDisplayName(magTypes);
		AddTypeRows(target, types, currentType, mag != null, mgr.GetAction(VPPContextConstants.ID_WEAPON_LOAD_MAG), baseArgs, items);
	}

	protected void BuildAmmoPage(VPPContextTarget target, EntityAI ent, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr)
			return;

		Weapon_Base wpn = Weapon_Base.Cast(ent);
		if (!wpn || !VPPCA_WeaponOps.SupportsLoading(wpn))
			return;

		array<string> ammoTypes = VPPCA_WeaponOps.GetChamberableTypes(wpn, -1);
		array<string> types = VPPATInventorySlots.SortByDisplayName(ammoTypes);
		int cm = wpn.GetCurrentMuzzle();
		string currentType = wpn.GetChamberedCartridgeMagazineTypeName(cm);

		VPPContextArgs noArgs = new VPPContextArgs();
		AddTypeRows(target, types, currentType, false, mgr.GetAction(VPPContextConstants.ID_WEAPON_LOAD_AMMO), noArgs, items);
	}

	//"Current" retargets to the occupant's own root page; "Remove Current" is the registered delete action
	//(Danger, Confirm and the DeleteObjectAtCrosshair permission come from the action)
	protected void AddCurrentRows(VPPContextTarget pageTarget, EntityAI occupant, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr || !occupant)
			return;

		VPPContextTarget occTarget = pageTarget.Retarget(occupant);
		string occName = VPPContextUtils.GetObjectDisplayName(occupant);
		VPPContextItem curRow = items.AddSubmenu("vpp.entity.attachments.current", occName, VPPContextConstants.ICON_EYE, 10, occTarget, "");
		if (curRow)
			curRow.Hint = "#VSTR_CTX_ATT_CURRENT";

		VPPContextAction del = mgr.GetAction(VPPContextConstants.ID_COMMON_DELETE);
		if (!del || !del.IsTargetType(occTarget) || !del.CanShow(occTarget))
			return;

		if (!mgr.HasPermissionHint(del.GetPermission()))
			return;

		VPPContextItem rm = items.AddAction(del, occTarget);
		if (!rm)
			return;

		rm.Id = "vpp.entity.attachments.remove";
		rm.Label = "#VSTR_CTX_ATT_REMOVE";
		rm.Order = 20;
	}

	protected void AddTypeRows(VPPContextTarget pageTarget, array<string> types, string currentType, bool confirm, VPPContextAction action, VPPContextArgs baseArgs, VPPContextItems items)
	{
		VPPContextActionManager mgr = GetVPPContextActionManager();
		if (!mgr || !action || !types || !baseArgs)
			return;

		if (!mgr.HasPermissionHint(action.GetPermission()))
			return;

		//the server rejects a target the action does not take (rule A4)
		if (!action.IsTargetType(pageTarget))
			return;

		int total = types.Count();
		if (total == 0)
		{
			items.AddInfo("vpp.entity.attachments.none", "#VSTR_CTX_ATT_NO_ITEMS", VPPContextConstants.ICON_INFO, 1000, "");
			return;
		}

		//the view creates a widget per row on every rebuild: long lists get page-chooser rows
		int size = VPPContextConstants.ATT_PAGE_SIZE;
		int start = 0;
		int stop = total;
		if (total > size)
		{
			if (!pageTarget.HasExtra(VPPContextConstants.EXTRA_ATT_PAGE))
			{
				AddPageRows(pageTarget, types, items);
				return;
			}

			string pageText = pageTarget.GetExtra(VPPContextConstants.EXTRA_ATT_PAGE);
			int pageIdx = pageText.ToInt();
			int pages = (total + size - 1) / size;
			if (pageIdx < 0 || pageIdx >= pages)
				pageIdx = 0;

			start = pageIdx * size;
			stop = start + size;
			if (stop > total)
				stop = total;
		}

		string lowCurrent = currentType;
		lowCurrent.ToLower();
		for (int i = start; i < stop; i++)
		{
			string cls = types.Get(i);
			VPPContextItem row = items.AddAction(action, pageTarget);
			if (!row)
				continue;

			string lowCls = cls;
			lowCls.ToLower();
			int orderOffset = i - start;

			row.Id = "vpp.entity.attachments.type." + cls;
			row.Label = VPPATInventorySlots.GetClassDisplayName(cls);
			row.IconPath = VPPContextConstants.ICON_PACKAGE_PLUS;
			row.Order = 1000 + orderOffset;
			//replacing destroys the occupant and its cargo
			row.Confirm = confirm;
			row.Hint = cls;
			if (lowCurrent != "" && lowCls == lowCurrent)
				row.Hint = "#VSTR_CTX_ATT_CURRENT";

			VPPContextArgs rowArgs = VPPContextArgs.FromMap(baseArgs.GetMap());
			rowArgs.Set(VPPContextConstants.ARG_ATT_TYPE, cls);
			row.Args = rowArgs;
		}
	}

	protected void AddPageRows(VPPContextTarget pageTarget, array<string> types, VPPContextItems items)
	{
		if (!pageTarget || !types || !items)
			return;

		int size = VPPContextConstants.ATT_PAGE_SIZE;
		int total = types.Count();
		int pages = (total + size - 1) / size;
		for (int p = 0; p < pages; p++)
		{
			int first = p * size;
			int last = first + size;
			if (last > total)
				last = total;

			string firstName = ShortName(VPPATInventorySlots.GetClassDisplayName(types.Get(first)));
			string lastName = ShortName(VPPATInventorySlots.GetClassDisplayName(types.Get(last - 1)));
			string pageLabel = firstName + " - " + lastName;

			VPPContextTarget pageT = pageTarget.CloneTarget();
			pageT.SetExtra(VPPContextConstants.EXTRA_ATT_PAGE, p.ToString());

			string pageRowId = "vpp.entity.attachments.page." + p.ToString();
			VPPContextItem pageRow = items.AddSubmenu(pageRowId, pageLabel, VPPContextConstants.ICON_LIST, 1000 + p, pageT, VPPContextConstants.ID_ATTACHMENTS);
			if (!pageRow)
				continue;

			int firstNo = first + 1;
			string pageHint = firstNo.ToString() + "-" + last.ToString();
			pageRow.Hint = pageHint;
		}
	}

	protected string ShortName(string text)
	{
		if (text.LengthUtf8() <= 10)
			return text;

		string cut = text.SubstringUtf8(0, 10);
		return cut + "..";
	}
};

//Spawns a class into an attachment slot. Placed only by VPPCA_EntityAttachments (per-row Args {slot, type}).
class VPPCA_AttachSpawn : VPPContextAction
{
	override string GetId()
	{
		return "vpp.entity.attach_spawn";
	}

	override string GetParentId()
	{
		return VPPContextConstants.PARENT_DYNAMIC;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_SPAWN_ATTACHMENT;
	}

	//never shown: the rows override it
	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_ATT_ATTACHMENTS";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PACKAGE_PLUS;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsMulti())
			return false;

		EntityAI ent = target.GetEntity();
		if (!ent)
			return false;

		return ent.GetInventory() != null;
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		if (!target || !args)
			return false;

		string slotName = args.Get(VPPContextConstants.ARG_ATT_SLOT);
		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		return VPPCA_AttachOps.ValidateSlotSpawn(target.GetEntity(), slotName, typeName);
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		EntityAI ent = target.GetEntity();
		if (!ent)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		string slotName = args.Get(VPPContextConstants.ARG_ATT_SLOT);
		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		string detail = typeName + " -> " + slotName;
		result.LogDetail = detail;

		int slotId = InventorySlots.GetSlotIdFromString(slotName);
		if (VPPCA_AttachOps.IsLockedOccupied(ent, slotId))
		{
			result.Fail("#VSTR_CTX_ATT_LOCKED");
			return;
		}

		Weapon_Base wpn = Weapon_Base.Cast(ent);
		if (wpn)
		{
			if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
			{
				result.Fail("#VSTR_CTX_RESULT_BUSY");
				return;
			}
		}

		string senderId = "";
		if (sender)
			senderId = sender.GetPlainId();

		int outcome = VPPCA_AttachOps.SpawnIntoSlot(ent, slotId, typeName, -1, senderId);
		VPPCA_AttachOps.ReportOutcome(outcome, typeName, result);
	}
};

//Spawns a full magazine (plus one chambered round) into a muzzle. Placed only by VPPCA_EntityAttachments (Args {muzzle, type}).
class VPPCA_WeaponLoadMagazine : VPPContextAction
{
	override string GetId()
	{
		return "vpp.weapon.load_magazine";
	}

	override string GetParentId()
	{
		return VPPContextConstants.PARENT_DYNAMIC;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_WEAPON_LOAD;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_WPN_MAGAZINE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_PACKAGE_PLUS;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return target.GetWeapon() != null;
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		if (!target || !args)
			return false;

		Weapon_Base wpn = target.GetWeapon();
		if (!wpn || !VPPCA_WeaponOps.SupportsLoading(wpn))
			return false;

		int mi = args.GetInt(VPPContextConstants.ARG_ATT_MUZZLE, -1);
		if (mi < 0 || mi >= wpn.GetMuzzleCount())
			return false;

		array<string> magTypes = VPPCA_WeaponOps.GetMagazineTypes(wpn, mi);
		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		return VPPCA_WeaponOps.ListContainsNoCase(magTypes, typeName);
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		int mi = args.GetInt(VPPContextConstants.ARG_ATT_MUZZLE, -1);
		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		string detail = typeName + " -> muzzle " + mi.ToString();
		result.LogDetail = detail;

		if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_BUSY");
			return;
		}

		//a magazine change on a jammed FSM has no matching stable state: loading always starts from an unjammed weapon, BEFORE the old magazine is deleted
		if (wpn.IsJammed() && !VPPCA_WeaponOps.Unjam(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_CANNOT_ATTACH", typeName);
			return;
		}

		string senderId = "";
		if (sender)
			senderId = sender.GetPlainId();

		int magSlot = wpn.GetSlotFromMuzzleIndex(mi);
		int outcome = VPPCA_AttachOps.SpawnIntoSlot(wpn, magSlot, typeName, mi, senderId);
		VPPCA_AttachOps.ReportOutcome(outcome, typeName, result);
	}
};

//Refills the internal magazine and the chamber with one chamberableFrom type. Placed only by VPPCA_EntityAttachments (Args {type}).
class VPPCA_WeaponLoadAmmo : VPPContextAction
{
	override string GetId()
	{
		return "vpp.weapon.load_ammo";
	}

	override string GetParentId()
	{
		return VPPContextConstants.PARENT_DYNAMIC;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_WEAPON_LOAD;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_WPN_AMMO";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_CROSSHAIR;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return target.GetWeapon() != null;
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		if (!target || !args)
			return false;

		Weapon_Base wpn = target.GetWeapon();
		if (!wpn || !VPPCA_WeaponOps.SupportsLoading(wpn))
			return false;

		array<string> ammoTypes = VPPCA_WeaponOps.GetChamberableTypes(wpn, -1);
		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		return VPPCA_WeaponOps.ListContainsNoCase(ammoTypes, typeName);
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		Weapon_Base wpn = target.GetWeapon();
		if (!wpn)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		string typeName = args.Get(VPPContextConstants.ARG_ATT_TYPE);
		result.LogDetail = typeName;

		if (!VPPCA_WeaponOps.CheckCooldown(wpn) || VPPCA_WeaponOps.IsBusy(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_BUSY");
			return;
		}

		if (wpn.IsJammed() && !VPPCA_WeaponOps.Unjam(wpn))
		{
			result.Fail("#VSTR_CTX_RESULT_CANNOT_LOAD", typeName);
			return;
		}

		int loaded = VPPCA_WeaponOps.LoadAmmo(wpn, typeName);
		string detail = typeName + " x" + loaded.ToString();
		result.LogDetail = detail;

		if (loaded > 0)
			result.Ok("#VSTR_CTX_RESULT_AMMO_LOADED", typeName);
		else
			result.Fail("#VSTR_CTX_RESULT_CANNOT_LOAD", typeName);
	}
};
