modded class DrowningMdfr
{
	override bool ActivateCondition(PlayerBase player)
	{
		if (player && player.GodModeStatus())
			return false;
		return super.ActivateCondition(player);
	}

	override bool DeactivateCondition(PlayerBase player)
	{
		//force an already-active drowning to end the moment godmode turns on
		if (player && player.GodModeStatus())
			return true;
		return super.DeactivateCondition(player);
	}

	override void OnTick(PlayerBase player, float deltaT)
	{
		if (player && player.GodModeStatus())
			return;
		super.OnTick(player, deltaT);
	}
};
