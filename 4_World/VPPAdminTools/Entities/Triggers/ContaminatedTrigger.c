modded class ContaminatedTrigger
{
	override void OnEnterServerEvent(TriggerInsider insider)
	{
		if (insider)
		{
			PlayerBase playerInsider = PlayerBase.Cast(insider.GetObject());
			if (playerInsider && playerInsider.GodModeStatus())
				return;
		}
		super.OnEnterServerEvent(insider);
	}
};