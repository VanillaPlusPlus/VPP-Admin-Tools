modded class Grenade_Base
{
	private int m_activatorSessionID = -1;

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if(GetGame().IsServer())
		{
			PlayerBase activator;
			if(GetHealth("", "") <= 0.0)
			{
				if(activator == null)
				{
					if(source.IsInherited(PlayerBase))
					{
						activator = PlayerBase.Cast(source);
					}

					if(activator == null)
					{
						activator = PlayerBase.Cast(source.GetHierarchyRootPlayer());
					}
				}

				if(activator)
					m_activatorSessionID = activator.VPlayerGetSessionId();
			}
		}
	}
	
	void SetActivator(PlayerBase player)
	{
		if(player)
			m_activatorSessionID = player.VPlayerGetSessionId();

		if(!player)
			m_activatorSessionID = -1;
	}

	PlayerBase GetActivator()
	{
		if(m_activatorSessionID == -1) return null;

		int low, high;
		GetGame().GetPlayerNetworkIDByIdentityID(m_activatorSessionID, low, high);
		return PlayerBase.Cast(GetGame().GetObjectByNetworkId(low, high));
	}
}
