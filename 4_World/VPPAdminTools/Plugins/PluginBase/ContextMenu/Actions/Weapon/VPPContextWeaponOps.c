//Server-side weapon mutations for the weapon context actions, plus the config readers the attachment pages share (those also run on the client).
//The jam flag follows the FSM state. Jam and unjam enter a stable state directly. Every other chamber or magazine mutation ends with Weapon_Base.VPPSettleFSM (which synchronizes).
class VPPCA_WeaponOps
{
	//state key suffix of the server-side ammo hint (Refill and Fill rows)
	const static string STATE_HINT_SUFFIX = ":hint";

	protected static ref map<string,int> s_LastOp;

	//FSM-driven: DoubleBarrel, SingleShotPistol, Flaregun, Archery, Magnum and modded FSMs without jammed states return false automatically
	static bool CanJam(Weapon_Base w)
	{
		if (!w)
			return false;

		return w.VPPCanForceJam();
	}

	static bool IsBusy(Weapon_Base w)
	{
		if (!w)
			return true;

		if (w.CanProcessWeaponEvents() && !w.IsIdle())
			return true;

		if (GetGame().HasInventoryJunctureItem(w))
			return true;

		PlayerBase holder = PlayerBase.Cast(w.GetHierarchyRootPlayer());
		if (holder && holder.GetWeaponManager() && holder.GetWeaponManager().IsRunning())
			return true;

		return false;
	}

	//Per-weapon cooldown shared by every weapon action. Returns false while the weapon is still cooling down.
	static bool CheckCooldown(Weapon_Base w)
	{
		if (!w)
			return false;

		//GetNetworkIDString concatenates high+low with no separator, so distinct ids can collide
		int netLow;
		int netHigh;
		w.GetNetworkID(netLow, netHigh);
		string key = netHigh.ToString() + ":" + netLow.ToString();
		if (netLow == 0 && netHigh == 0)
		{
			key = w.GetType() + "@";
			key += VPPContextUtils.FormatPosition(w.GetPosition());
		}

		if (!s_LastOp)
			s_LastOp = new map<string,int>;

		int now = GetGame().GetTime();
		int last;
		if (s_LastOp.Find(key, last) && now - last < VPPContextConstants.WEAPON_OP_COOLDOWN_MS)
			return false;

		if (s_LastOp.Count() > 256)
			PruneCooldowns(now);

		s_LastOp.Set(key, now);
		return true;
	}

	protected static void PruneCooldowns(int now)
	{
		if (!s_LastOp)
			return;

		array<string> stale = new array<string>;
		foreach (string opKey, int stamp : s_LastOp)
		{
			if (now - stamp >= VPPContextConstants.WEAPON_OP_COOLDOWN_MS)
				stale.Insert(opKey);
		}

		foreach (string staleKey : stale)
		{
			s_LastOp.Remove(staleKey);
		}
	}

	//true when the weapon is no longer jammed
	static bool Unjam(Weapon_Base w)
	{
		if (!w)
			return false;

		if (!w.IsJammed())
			return true;

		if (w.VPPForceUnjam())
			return true;

		//no free state keeps the live round in this layout: store the round and retry with an empty chamber
		int mi = w.GetCurrentMuzzle();
		if (!w.IsChamberEmpty(mi))
			PopChamberedRound(w, mi);

		if (w.VPPForceUnjam())
			return true;

		//no free stable state fits (modded FSM): VPPSettleFSM's search is a superset of vanilla's, so this settle is expected to fail; the jam is restored so the flag and the FSM never disagree
		w.SetJammed(false);
		if (w.VPPSettleFSM())
			return true;

		w.SetJammed(true);
		w.VPPSettleFSM();
		return false;
	}

	static bool TryJam(Weapon_Base w)
	{
		if (!w)
			return false;

		return w.VPPForceJam();
	}

	//Pops the chambered cartridge and stores it with the holder, or drops it as a pile next to the weapon.
	static bool PopChamberedRound(Weapon_Base w, int mi)
	{
		if (!w)
			return false;

		string magType = w.GetChamberedCartridgeMagazineTypeName(mi);
		float dmg;
		string ammo;
		if (!w.PopCartridgeFromChamber(mi, dmg, ammo))
			return false;

		PlayerBase holder = PlayerBase.Cast(w.GetHierarchyRootPlayer());
		if (holder)
		{
			DayZPlayerUtils.HandleStoreCartridge(holder, w, mi, dmg, ammo, magType);
		}
		else if (magType != "")
		{
			Magazine pile = Magazine.Cast(GetGame().CreateObjectEx(magType, w.GetPosition(), ECE_PLACE_ON_SURFACE));
			if (pile)
			{
				pile.ServerSetAmmoCount(0);
				pile.ServerStoreCartridge(dmg, ammo);
			}
		}

		return true;
	}

	//Returns the number of chambers ejected. Never uses Weapon_Base.EjectCartridge: it pops the internal magazine when the chamber is not ejectable.
	static int EjectChambered(Weapon_Base w)
	{
		if (!w)
			return 0;

		int count = 0;
		bool needSync = false;
		int current = w.GetCurrentMuzzle();
		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			bool unjammedHere = false;
			//unjam first and fall through: a forced jam can leave a live round in the chamber
			if (w.IsJammed() && mi == current)
			{
				if (Unjam(w))
				{
					count++;
					unjammedHere = true;
				}
			}

			bool ejected = false;
			if (w.IsChamberFiredOut(mi))
			{
				w.EjectCasing(mi);
				ejected = true;
			}
			else if (!w.IsChamberEmpty(mi))
			{
				if (PopChamberedRound(w, mi))
					ejected = true;
			}

			if (ejected)
			{
				w.EffectBulletHide(mi);
				w.HideBullet(mi);
				if (!unjammedHere)
					count++;

				needSync = true;
			}
		}

		if (needSync)
			w.VPPSettleFSM();

		//The revolver cylinder visuals are driven separately from the FSM
		if (count > 0)
		{
			Magnum_Base revolver = Magnum_Base.Cast(w);
			if (revolver)
				revolver.SyncCylinderRotation();
		}

		return count;
	}

	static bool HasMagazineCapacity(Weapon_Base w)
	{
		if (!w)
			return false;

		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			if (w.GetMagazine(mi))
				return true;

			if (w.HasInternalMagazine(mi))
				return true;
		}

		return false;
	}

	static bool CanRefill(Weapon_Base w)
	{
		if (!w)
			return false;

		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			Magazine mag = w.GetMagazine(mi);
			if (mag)
			{
				if (mag.GetAmmoCount() < mag.GetAmmoMax())
					return true;

				continue;
			}

			if (w.HasInternalMagazine(mi) && w.GetInternalMagazineCartridgeCount(mi) < w.GetInternalMagazineMaxCartridgeCount(mi))
				return true;
		}

		return false;
	}

	//Server. Pushes the magazine's server ammo count to every client that has the entity.
	//Clients track an attached magazine's ammo by replaying weapon events, so a Server* change made outside the weapon FSM (refill, forced jam, eject, fill, empty) never reaches them.
	//Same idea as vanilla MiscGameplayFunctions.UnlimitedAmmoDebugCheck (server ServerSetAmmoMax + client LocalSetAmmoMax). Nothing is detached or re-created.
	//Never call it for a magazine created in the same frame: its creation already carries the count, and the RPC can arrive before the entity exists on a client.
	static void SyncMagazineAmmo(Magazine mag)
	{
		if (!mag)
			return;

		if (!GetGame().IsServer() || !GetGame().IsMultiplayer())
			return;

		Param1<int> data = new Param1<int>(mag.GetAmmoCount());
		GetRPCManager().VSendRPC("RPC_VPPContextMenuClient", "SyncMagazineAmmo", data, true, null, mag);
	}

	//Server. SyncMagazineAmmo for every magazine attached to the weapon.
	static void SyncWeaponMagazines(Weapon_Base w)
	{
		if (!w)
			return;

		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			Magazine mag = w.GetMagazine(mi);
			if (mag)
				SyncMagazineAmmo(mag);
		}
	}

	//true when the muzzle has an attached or an internal magazine; outputs that magazine's count and capacity. Client and server.
	static bool GetMuzzleAmmo(Weapon_Base w, int mi, out int count, out int capacity)
	{
		count = 0;
		capacity = 0;
		if (!w || mi < 0 || mi >= w.GetMuzzleCount())
			return false;

		Magazine mag = w.GetMagazine(mi);
		if (mag)
		{
			count = mag.GetAmmoCount();
			capacity = mag.GetAmmoMax();
			return true;
		}

		if (!w.HasInternalMagazine(mi))
			return false;

		count = w.GetInternalMagazineCartridgeCount(mi);
		capacity = w.GetInternalMagazineMaxCartridgeCount(mi);
		return true;
	}

	//"count/max" of the first muzzle that is not full, else of the current muzzle; "" without magazine capacity. Client and server.
	static string BuildAmmoHint(Weapon_Base w)
	{
		if (!w)
			return "";

		int pick = -1;
		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			int count;
			int capacity;
			if (!GetMuzzleAmmo(w, mi, count, capacity))
				continue;

			if (count < capacity)
			{
				pick = mi;
				break;
			}
		}

		if (pick < 0)
			pick = w.GetCurrentMuzzle();

		int pickCount;
		int pickCapacity;
		if (!GetMuzzleAmmo(w, pick, pickCount, pickCapacity))
			return "";

		string hint = pickCount.ToString() + "/";
		hint += pickCapacity.ToString();
		return hint;
	}

	//Server. Fills every attached and internal magazine, chambers a round where the chamber was empty, then pushes the new counts to clients.
	//Returns the number of muzzles that gained rounds.
	static int Refill(Weapon_Base w)
	{
		if (!w)
			return 0;

		array<int> refilled = new array<int>;
		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			Magazine mag = w.GetMagazine(mi);
			if (mag)
			{
				if (mag.GetAmmoCount() < mag.GetAmmoMax())
				{
					mag.ServerSetAmmoMax();
					refilled.Insert(mi);
				}

				continue;
			}

			if (!w.HasInternalMagazine(mi))
				continue;

			int capacity = w.GetInternalMagazineMaxCartridgeCount(mi);
			int loaded = w.GetInternalMagazineCartridgeCount(mi);
			if (loaded >= capacity)
				continue;

			float dmg;
			string ammo = "";
			if (loaded > 0)
				w.GetInternalMagazineCartridgeInfo(mi, 0, dmg, ammo);

			if (ammo == "")
				ammo = w.GetRandomChamberableAmmoTypeName(mi);

			if (ammo == "")
				continue;

			int before = loaded;
			while (loaded < capacity)
			{
				if (!w.PushCartridgeToInternalMagazine(mi, 0, ammo))
					break;

				loaded++;
			}

			if (loaded > before)
				refilled.Insert(mi);
		}

		int n = refilled.Count();
		if (n == 0)
			return 0;

		if (!ChamberAfterRefill(w, refilled))
			w.Synchronize();

		SyncWeaponMagazines(w);
		return n;
	}

	//Server. After a refill, puts one extra round of the feed's own type into each refilled muzzle whose chamber is empty (vanilla SpawnAttachedMagazine does the same), so a weapon fired dry can shoot straight away.
	//Skipped while jammed. Rolls the rounds back out when no stable state holds them (open bolt). Returns true when VPPSettleFSM ran (it synchronizes).
	protected static bool ChamberAfterRefill(Weapon_Base w, array<int> refilledMuzzles)
	{
		if (!w || !refilledMuzzles || w.IsJammed())
			return false;

		array<int> chambered = new array<int>;
		foreach (int mi : refilledMuzzles)
		{
			if (!w.IsChamberEmpty(mi) || w.IsChamberFiredOut(mi))
				continue;

			float chDmg;
			string chType = "";
			bool found = false;
			Magazine feed = w.GetMagazine(mi);
			if (feed)
				found = feed.GetCartridgeAtIndex(0, chDmg, chType);
			else if (w.HasInternalMagazine(mi) && w.GetInternalMagazineCartridgeCount(mi) > 0)
				found = w.GetInternalMagazineCartridgeInfo(mi, 0, chDmg, chType);

			if (!found || chType == "")
				continue;

			if (w.PushCartridgeToChamber(mi, 0, chType))
				chambered.Insert(mi);
		}

		if (chambered.Count() == 0)
			return false;

		if (w.VPPSettleFSM())
			return true;

		//no stable state holds a chambered round here (open bolt): take the rounds back out and settle again
		foreach (int back : chambered)
		{
			float backDmg;
			string backType;
			w.PopCartridgeFromChamber(back, backDmg, backType);
			w.EffectBulletHide(back);
			w.HideBullet(back);
		}

		w.VPPSettleFSM();
		return true;
	}

	//Archery has no chamber or magazine semantics the loading pages can use
	static bool SupportsLoading(Weapon_Base w)
	{
		if (!w)
			return false;

		if (!w.VPPGetWpnFSM())
			return false;

		if (w.IsInherited(Archery_Base))
			return false;

		return true;
	}

	//Muzzle 0 lives on the weapon class itself; the others are the muzzles[] subclasses ("this" means the weapon class).
	static string GetMuzzleConfigPath(Weapon_Base w, int mi)
	{
		if (!w)
			return "";

		string root = "CfgWeapons " + w.GetType();
		if (mi <= 0)
			return root;

		string muzzlesPath = root + " muzzles";
		array<string> muzzleNames = new array<string>;
		GetGame().ConfigGetTextArray(muzzlesPath, muzzleNames);
		if (mi < muzzleNames.Count())
		{
			string muzzleName = muzzleNames.Get(mi);
			string lowMuzzle = muzzleName;
			lowMuzzle.ToLower();
			if (muzzleName != "" && lowMuzzle != "this")
				return root + " " + muzzleName;
		}

		return root;
	}

	static bool ListContainsNoCase(array<string> names, string value)
	{
		if (!names)
			return false;

		string lowValue = value;
		lowValue.ToLower();
		foreach (string entryName : names)
		{
			string lowEntry = entryName;
			lowEntry.ToLower();
			if (lowEntry == lowValue)
				return true;
		}

		return false;
	}

	//Spawnable CfgMagazines classes of one muzzle's config list. A muzzle subclass without the list falls back to the weapon's root list.
	protected static array<string> ReadWeaponList(Weapon_Base w, int mi, string entry)
	{
		array<string> result = new array<string>;
		if (!w || mi < 0 || mi >= w.GetMuzzleCount())
			return result;

		array<string> rawNames = new array<string>;
		string listPath = GetMuzzleConfigPath(w, mi) + " " + entry;
		GetGame().ConfigGetTextArray(listPath, rawNames);
		if (rawNames.Count() == 0 && mi > 0)
		{
			string rootPath = "CfgWeapons " + w.GetType();
			rootPath += " " + entry;
			GetGame().ConfigGetTextArray(rootPath, rawNames);
		}

		foreach (string cfgName : rawNames)
		{
			if (!VPPATInventorySlots.IsSpawnableClass("CfgMagazines", cfgName))
				continue;

			if (ListContainsNoCase(result, cfgName))
				continue;

			result.Insert(cfgName);
		}

		return result;
	}

	static array<string> GetMagazineTypes(Weapon_Base w, int mi)
	{
		return ReadWeaponList(w, mi, "magazines");
	}

	//mi = -1: the union over every muzzle
	static array<string> GetChamberableTypes(Weapon_Base w, int mi)
	{
		if (mi >= 0)
			return ReadWeaponList(w, mi, "chamberableFrom");

		array<string> merged = new array<string>;
		if (!w)
			return merged;

		int muzzles = w.GetMuzzleCount();
		for (int k = 0; k < muzzles; k++)
		{
			array<string> perMuzzle = ReadWeaponList(w, k, "chamberableFrom");
			foreach (string chamberName : perMuzzle)
			{
				if (!ListContainsNoCase(merged, chamberName))
					merged.Insert(chamberName);
			}
		}

		return merged;
	}

	//Server. The caller guarantees the slot is free and the type is valid.
	//Do not use vanilla SpawnAttachedMagazine: it ErrorEx's on failure and only knows muzzle 0.
	static bool AttachMagazineNow(Weapon_Base w, int mi, string magType)
	{
		if (!w || mi < 0 || mi >= w.GetMuzzleCount())
			return false;

		if (w.GetMagazine(mi))
			return false;

		int magSlot = w.GetSlotFromMuzzleIndex(mi);
		EntityAI created = w.GetInventory().CreateAttachmentEx(magType, magSlot);
		if (!created && mi == 0)
			created = w.GetInventory().CreateAttachment(magType);

		Magazine newMag = Magazine.Cast(created);
		//not a magazine, or CreateAttachment picked another free slot
		if (!newMag || w.GetMagazine(mi) != newMag)
		{
			if (created)
				GetGame().ObjectDelete(created);

			return false;
		}

		newMag.ServerSetAmmoMax();

		//ready to fire: one extra round of the magazine's own type in the empty chamber, like vanilla SpawnAttachedMagazine
		bool chambered = false;
		if (w.IsChamberEmpty(mi))
		{
			float chDmg;
			string chType;
			if (newMag.GetCartridgeAtIndex(0, chDmg, chType))
				chambered = w.PushCartridgeToChamber(mi, 0, chType);
		}

		if (w.VPPSettleFSM())
			return true;

		//no stable state holds a chambered round with this magazine (open bolt): take the extra round out and settle again
		if (chambered)
		{
			float backDmg;
			string backType;
			w.PopCartridgeFromChamber(mi, backDmg, backType);
			w.EffectBulletHide(mi);
			w.HideBullet(mi);
			if (w.VPPSettleFSM())
				return true;
		}

		//still no state: remove the magazine (EEItemDetached runs ValidateAndRepair) and settle what is left
		GetGame().ObjectDelete(newMag);
		w.VPPSettleFSM();
		return false;
	}

	//Server. The caller unjams first. Replaces what was loaded before: refills every internal magazine that takes ammoType and chambers one round per muzzle.
	//Returns the number of rounds loaded.
	static int LoadAmmo(Weapon_Base w, string ammoType)
	{
		if (!w || !SupportsLoading(w))
			return 0;

		string bulletType;
		if (!AmmoTypesAPI.MagazineTypeToAmmoType(ammoType, bulletType))
			return 0;

		int loaded = 0;
		array<int> chamberedMuzzles = new array<int>;
		int muzzles = w.GetMuzzleCount();
		for (int mi = 0; mi < muzzles; mi++)
		{
			array<string> chamberable = GetChamberableTypes(w, mi);
			if (!ListContainsNoCase(chamberable, ammoType))
				continue;

			if (w.HasInternalMagazine(mi))
			{
				float oldDmg;
				string oldType;
				while (w.GetInternalMagazineCartridgeCount(mi) > 0)
				{
					if (!w.PopCartridgeFromInternalMagazine(mi, oldDmg, oldType))
						break;
				}

				int capacity = w.GetInternalMagazineMaxCartridgeCount(mi);
				for (int k = 0; k < capacity; k++)
				{
					if (!w.PushCartridgeToInternalMagazine(mi, 0, bulletType))
						break;

					loaded++;
				}
			}

			if (w.IsChamberFiredOut(mi))
			{
				w.EjectCasing(mi);
			}
			else if (!w.IsChamberEmpty(mi))
			{
				float chDmg;
				string chType;
				w.PopCartridgeFromChamber(mi, chDmg, chType);
			}

			if (w.PushCartridgeToChamber(mi, 0, bulletType))
			{
				loaded++;
				chamberedMuzzles.Insert(mi);
			}
		}

		if (loaded == 0)
		{
			w.VPPSettleFSM();
			return 0;
		}

		if (!w.VPPSettleFSM())
		{
			//e.g. an open-bolt weapon without a magazine cannot hold a chambered round: pop the chambered rounds back out
			foreach (int back : chamberedMuzzles)
			{
				float bDmg;
				string bType;
				if (w.PopCartridgeFromChamber(back, bDmg, bType))
					loaded--;

				w.EffectBulletHide(back);
				w.HideBullet(back);
			}

			w.VPPSettleFSM();
		}

		Magnum_Base revolver = Magnum_Base.Cast(w);
		if (revolver)
			revolver.SyncCylinderRotation();

		return loaded;
	}
};
