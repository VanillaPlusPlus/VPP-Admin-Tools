modded class AreaExposureMdfr
{
	override void OnActivate(PlayerBase player)
	{
		if (player && player.GodModeStatus()){
			Deactivate(); //incase super was modded before hand
			return;
		}

		super.OnActivate(player);
	}

	override void Activate()
	{
		if (m_Player && m_Player.GodModeStatus())
			return; //stop it from activating in godmode

		//call super
		super.Activate();
	}
}