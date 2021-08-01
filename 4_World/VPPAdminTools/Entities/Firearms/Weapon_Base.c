modded class Weapon_Base
{
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
};