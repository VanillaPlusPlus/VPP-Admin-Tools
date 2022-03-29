modded class ActionPin
{
	override void OnStartServer( ActionData action_data )
	{
		super.OnStartServer(action_data);
		
		if(GetGame().IsServer())
		{
			Grenade_Base grenade = Grenade_Base.Cast(action_data.m_MainItem);
			if(grenade)
			{
				grenade.SetActivator(null);
			}
		}
	}
}