modded class LandMineTrap
{
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
					SetActivator(activator);
			}
		}
	}
};