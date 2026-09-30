//VPP: find a stable state by its declared flags and enter it directly (same sequence as vanilla OnStoreLoad / RandomizeFSMStateEx and CF_FindBestStableState)
modded class WeaponFSM
{
	//First stable state with matching jam/magazine flags whose per-muzzle chamber states equal muzzleStates.
	//allowAnyMuzzle: without an exact match return the first flag match (used while jammed: ValidateAndRepair skips chamber checks then).
	//Single-state machines (Magnum) answer every NON-jammed query whatever hasMagazine is, like vanilla RandomizeFSMStateEx. Do not add a magazine filter there.
	//They never answer a jammed query: that state has no unjam transition, so the jam would be permanent.
	WeaponStableState VPPFindStableState(bool isJammed, bool hasMagazine, array<MuzzleState> muzzleStates, bool allowAnyMuzzle)
	{
		WeaponStableState fallback = null;
		foreach (WeaponTransition trans : m_Transitions)
		{
			WeaponStableState st = WeaponStableState.Cast(trans.m_srcState);
			if (!st)
				continue;

			if (st.IsSingleState())
			{
				if (!isJammed)
					return st;

				continue;
			}

			if (st.IsJammed() != isJammed || st.HasMagazine() != hasMagazine)
				continue;

			if (VPPMuzzlesEqual(st, muzzleStates))
				return st;

			if (allowAnyMuzzle && !fallback)
				fallback = st;
		}

		return fallback;
	}

	protected bool VPPMuzzlesEqual(WeaponStableState st, array<MuzzleState> muzzleStates)
	{
		if (!st || !muzzleStates)
			return false;

		int stateCount = st.GetMuzzleStateCount();
		if (muzzleStates.Count() != stateCount)
			return false;

		for (int i = 0; i < stateCount; i++)
		{
			if (muzzleStates[i] != st.GetMuzzleState(i))
				return false;
		}

		return true;
	}

	//IsSingleState is checked before IsJammed: the Magnum state mirrors the weapon flag
	bool VPPHasJammedState()
	{
		foreach (WeaponTransition jamTrans : m_Transitions)
		{
			WeaponStableState jamSt = WeaponStableState.Cast(jamTrans.m_srcState);
			if (!jamSt || jamSt.IsSingleState())
				continue;

			if (jamSt.IsJammed())
				return true;
		}

		return false;
	}

	//OnEntry re-applies SetJammed / SetCharged / SetWeaponOpen from the state; it skips SyncAnimState for a null event, so call it here
	void VPPForceStableState(WeaponStableState st)
	{
		if (!st)
			return;

		Terminate();
		m_State = st;
		Start(null, true);
		st.SyncAnimState();
	}
};
