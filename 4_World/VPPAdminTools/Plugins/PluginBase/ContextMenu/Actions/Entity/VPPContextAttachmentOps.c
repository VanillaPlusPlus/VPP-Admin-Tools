enum EVPPAttachOutcome
{
	FAILED = 0,
	ATTACHED = 1,
	QUEUED = 2
};

//Server helpers for the attachment actions; SlotLabel / IsHiddenSlot / IsLockedOccupied are also used by the client pages
class VPPCA_AttachOps
{
	const static int RETRY_MS = 150;
	const static int RETRY_COUNT = 10;

	static string SlotLabel(int slotId)
	{
		string label = InventorySlots.GetSlotDisplayName(slotId);
		g_Game.FormatRawConfigStringKeys(label);
		label = Widget.TranslateString(label);
		if (label == "")
			label = InventorySlots.GetSlotName(slotId);

		return label;
	}

	static bool IsMagazineSlot(Weapon_Base wpn, int slotId)
	{
		if (!wpn)
			return false;

		int muzzles = wpn.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			if (wpn.GetSlotFromMuzzleIndex(mi) == slotId)
				return true;
		}

		return false;
	}

	static bool IsMuzzleMagazine(Weapon_Base wpn, EntityAI att)
	{
		if (!wpn || !att)
			return false;

		int muzzles = wpn.GetMuzzleCount();
		for (int k = 0; k < muzzles; k++)
		{
			EntityAI muzzleMag = wpn.GetMagazine(k);
			if (muzzleMag && muzzleMag == att)
				return true;
		}

		return false;
	}

	//hidden: not displayable in the inventory UI, the character head placeholder (any entity), or a weapon's magazine slot (magazines have their own rows)
	static bool IsHiddenSlot(EntityAI ent, int slotId)
	{
		if (!ent)
			return true;

		if (!ent.CanDisplayAttachmentSlot(slotId))
			return true;

		string lowName = InventorySlots.GetSlotName(slotId);
		lowName.ToLower();
		if (lowName == "head")
			return true;

		Weapon_Base wpn = Weapon_Base.Cast(ent);
		if (wpn && IsMagazineSlot(wpn, slotId))
			return true;

		return false;
	}

	//occupied AND locked (mounted barbed wire, combination lock, armed explosive, fireplace parts): never replaced
	static bool IsLockedOccupied(EntityAI ent, int slotId)
	{
		if (!ent || !ent.GetInventory() || slotId == InventorySlots.INVALID)
			return false;

		if (!ent.GetInventory().FindAttachment(slotId))
			return false;

		return ent.GetInventory().GetSlotLock(slotId);
	}

	//the lock is NOT checked here: OnExecuteServer reports it with its own message
	static bool ValidateSlotSpawn(EntityAI ent, string slotName, string typeName)
	{
		if (!ent || !ent.GetInventory())
			return false;

		if (slotName == "" || typeName == "")
			return false;

		int slotId = InventorySlots.GetSlotIdFromString(slotName);
		if (slotId == InventorySlots.INVALID)
			return false;

		if (!ent.GetInventory().HasAttachmentSlot(slotId))
			return false;

		if (IsHiddenSlot(ent, slotId))
			return false;

		return VPPATInventorySlots.CanSpawnInSlot(typeName, slotName);
	}

	//mirrors EntityAI.DeleteSafe
	static bool IsUnderLivePlayer(EntityAI item)
	{
		if (!item)
			return false;

		Man rootPlayer = item.GetHierarchyRootPlayer();
		if (!rootPlayer)
			return false;

		return rootPlayer.IsAlive();
	}

	static EntityAI GetOccupant(EntityAI parent, int slotId, int muzzle)
	{
		if (!parent)
			return null;

		if (muzzle >= 0)
		{
			Weapon_Base wpn = Weapon_Base.Cast(parent);
			if (!wpn || muzzle >= wpn.GetMuzzleCount())
				return null;

			return wpn.GetMagazine(muzzle);
		}

		if (!parent.GetInventory())
			return null;

		return parent.GetInventory().FindAttachment(slotId);
	}

	static bool CreateNow(EntityAI parent, int slotId, string typeName, int muzzle)
	{
		if (!parent)
			return false;

		if (muzzle >= 0)
		{
			Weapon_Base wpn = Weapon_Base.Cast(parent);
			if (!wpn)
				return false;

			//settles the FSM or rolls back completely
			return VPPCA_WeaponOps.AttachMagazineNow(wpn, muzzle, typeName);
		}

		if (!parent.GetInventory())
			return false;

		//null = script conditions or exclusions refused it
		EntityAI created = parent.GetInventory().CreateAttachmentEx(typeName, slotId);
		return created != null;
	}

	static int SpawnIntoSlot(EntityAI parent, int slotId, string typeName, int muzzle, string senderId)
	{
		if (!parent)
			return EVPPAttachOutcome.FAILED;

		EntityAI occupant = GetOccupant(parent, slotId, muzzle);
		if (occupant)
		{
			//DeleteSafe = juncture delete, the slot frees a few frames later
			//ObjectDelete = immediate (AdminTools car-part precedent)
			if (IsUnderLivePlayer(occupant))
				occupant.DeleteSafe();
			else
				GetGame().ObjectDelete(occupant);
		}

		if (GetOccupant(parent, slotId, muzzle))
		{
			int low;
			int high;
			parent.GetNetworkID(low, high);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(VPPCA_AttachOps.DeferredCreate, RETRY_MS, false, low, high, slotId, typeName, muzzle, senderId, RETRY_COUNT);
			return EVPPAttachOutcome.QUEUED;
		}

		if (CreateNow(parent, slotId, typeName, muzzle))
			return EVPPAttachOutcome.ATTACHED;

		return EVPPAttachOutcome.FAILED;
	}

	static void DeferredCreate(int parentLow, int parentHigh, int slotId, string typeName, int muzzle, string senderId, int attemptsLeft)
	{
		EntityAI parent = EntityAI.Cast(GetGame().GetObjectByNetworkId(parentLow, parentHigh));
		if (!parent)
		{
			NotifyFailed(senderId, typeName);
			return;
		}

		Weapon_Base wpn = Weapon_Base.Cast(parent);
		bool mustWait = GetOccupant(parent, slotId, muzzle) != null;
		if (wpn && VPPCA_WeaponOps.IsBusy(wpn))
			mustWait = true;

		if (mustWait)
		{
			if (attemptsLeft <= 0)
			{
				NotifyFailed(senderId, typeName);
				return;
			}

			int nextAttempts = attemptsLeft - 1;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(VPPCA_AttachOps.DeferredCreate, RETRY_MS, false, parentLow, parentHigh, slotId, typeName, muzzle, senderId, nextAttempts);
			return;
		}

		//loading always starts from an unjammed weapon (a jam may have come back while waiting)
		if (wpn && muzzle >= 0 && wpn.IsJammed())
			VPPCA_WeaponOps.Unjam(wpn);

		if (!CreateNow(parent, slotId, typeName, muzzle))
			NotifyFailed(senderId, typeName);
	}

	static void ReportOutcome(int outcome, string typeName, VPPContextResult result)
	{
		if (!result)
			return;

		if (outcome == EVPPAttachOutcome.ATTACHED)
		{
			result.Ok("#VSTR_CTX_RESULT_ATTACHED", typeName);
			return;
		}

		if (outcome == EVPPAttachOutcome.QUEUED)
		{
			result.Ok("#VSTR_CTX_RESULT_ATTACH_QUEUED", typeName);
			string queuedDetail = result.LogDetail + " (queued)";
			result.LogDetail = queuedDetail;
			return;
		}

		result.Fail("#VSTR_CTX_RESULT_CANNOT_ATTACH", typeName);
	}

	protected static void NotifyFailed(string senderId, string typeName)
	{
		Print("[VPPContextMenu] deferred attach failed: " + typeName);

		PermissionManager pm = GetPermissionManager();
		if (pm)
			pm.NotifyPlayer(senderId, "#VSTR_CTX_RESULT_ATTACH_FAILED", NotifyTypes.ERROR);
	}
};
