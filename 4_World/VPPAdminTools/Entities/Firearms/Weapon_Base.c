modded class Weapon_Base
{
	protected static ref map<string, bool> s_VPPJamCapable;

	override void EEFired(int muzzleType, int mode, string ammoType)
	{
		super.EEFired (muzzleType, mode, ammoType);
		PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
		if ( player )
		{
			player.UnlimitedAmmoCheck(this);
		}
	}

	WeaponFSM VPPGetWpnFSM()
	{
		return m_fsm;
	}

	void VPPForceShowBarrel()
	{
		if (!g_Game.IsDedicatedServer() && m_weaponHideBarrelIdx != -1)
			SetSimpleHiddenSelectionState(m_weaponHideBarrelIdx, true);
	}

	//true when the FSM has a jammed stable state it can leave again; cached per type; client and server
	bool VPPCanForceJam()
	{
		if (!m_fsm)
			return false;

		if (!s_VPPJamCapable)
			s_VPPJamCapable = new map<string, bool>;

		string typeKey = GetType();
		bool cached;
		if (s_VPPJamCapable.Find(typeKey, cached))
			return cached;

		bool capable = m_fsm.VPPHasJammedState();
		s_VPPJamCapable.Set(typeKey, capable);
		return capable;
	}

	//a jam needs a cartridge, either in the chamber or one that can be fed; client and server
	bool VPPHasRoundForJam()
	{
		int mi = GetCurrentMuzzle();
		if (!IsChamberEmpty(mi))
			return true;

		Magazine feedMag = GetMagazine(mi);
		if (feedMag && feedMag.GetAmmoCount() > 0)
			return true;

		if (HasInternalMagazine(mi) && GetInternalMagazineCartridgeCount(mi) > 0)
			return true;

		return false;
	}

	//server: moves one cartridge from the attached or internal magazine into the empty chamber, putting it back on failure
	protected bool VPPChamberFromFeed(int mi)
	{
		float dmg;
		string bullet;
		Magazine feedMag = GetMagazine(mi);
		if (feedMag && feedMag.GetAmmoCount() > 0)
		{
			if (!feedMag.ServerAcquireCartridge(dmg, bullet))
				return false;

			if (PushCartridgeToChamber(mi, dmg, bullet))
				return true;

			feedMag.ServerStoreCartridge(dmg, bullet);
			return false;
		}

		if (HasInternalMagazine(mi) && GetInternalMagazineCartridgeCount(mi) > 0)
		{
			if (!PopCartridgeFromInternalMagazine(mi, dmg, bullet))
				return false;

			if (PushCartridgeToChamber(mi, dmg, bullet))
				return true;

			PushCartridgeToInternalMagazine(mi, dmg, bullet);
			return false;
		}

		return false;
	}

	//server
	//Enters a jammed stable state directly. RandomizeFSMState cannot: it needs an exact chamber match, and nearly every vanilla jammed state declares a fired casing (F) while a live round reports L.
	//The chosen state may declare F while the chamber holds a live round. Validation skips chamber checks while jammed, and vanilla WeaponUnjamming_Cartridge handles a live round when the player clears the jam.
	bool VPPForceJam()
	{
		if (!m_fsm || IsJammed())
			return false;

		//muzzle 0 decides the magazine flag on purpose: WeaponFSM.ValidateAndRepair checks GetMagazine(0)
		bool hasMag = GetMagazine(0) != null;
		WeaponStableState jamState = m_fsm.VPPFindStableState(true, hasMag, GetMuzzleStates(), true);
		if (!jamState)
			return false;

		int mi = GetCurrentMuzzle();
		if (IsChamberEmpty(mi) && !VPPChamberFromFeed(mi))
			return false;

		//re-pick with the new chamber state (Pistol has an exact live-round jammed state)
		jamState = m_fsm.VPPFindStableState(true, hasMag, GetMuzzleStates(), true);
		if (!jamState)
			return false;

		m_fsm.VPPForceStableState(jamState);
		ForceSyncSelectionState();
		Synchronize();
		return IsJammed();
	}

	//server: leaves a jam through an exact non-jammed stable state. Returns false when none fits; only a fired casing may have been ejected by then.
	bool VPPForceUnjam()
	{
		if (!m_fsm)
			return false;

		int mi = GetCurrentMuzzle();
		if (IsChamberFiredOut(mi))
		{
			EjectCasing(mi);
			EffectBulletHide(mi);
			HideBullet(mi);
		}

		bool hasMag = GetMagazine(0) != null;
		WeaponStableState freeState = m_fsm.VPPFindStableState(false, hasMag, GetMuzzleStates(), false);
		if (!freeState)
			return false;

		//single-state machines (Magnum) mirror the weapon flag in OnEntry, so clear it first
		SetJammed(false);
		m_fsm.VPPForceStableState(freeState);
		ForceSyncSelectionState();
		Synchronize();
		return !IsJammed();
	}

	//server
	//Every VPP weapon mutation ends here instead of a bare RandomizeFSMState.
	//Enters the stable state matching the weapon: jam flag, magazine on muzzle 0 (what ValidateAndRepair checks), chamber per muzzle. While jammed, any jammed state with the right magazine flag fits.
	//false = no stable state fits this layout (e.g. an open-bolt weapon cannot hold a chambered round without a magazine). Vanilla RandomizeFSMState is still tried for modded multi-muzzle layouts, and the caller rolls its change back.
	bool VPPSettleFSM()
	{
		if (!m_fsm)
			return false;

		bool jammed = IsJammed();
		bool hasMag = GetMagazine(0) != null;
		WeaponStableState st = m_fsm.VPPFindStableState(jammed, hasMag, GetMuzzleStates(), jammed);
		if (st)
		{
			m_fsm.VPPForceStableState(st);
			ForceSyncSelectionState();
			Synchronize();
			return true;
		}

		RandomizeFSMState();
		Synchronize();
		return false;
	}
};