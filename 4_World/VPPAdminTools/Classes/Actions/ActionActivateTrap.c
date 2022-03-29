modded class ActionActivateTrap
{
	override void OnFinishProgressServer( ActionData action_data )
	{
		super.OnFinishProgressServer(action_data);

		if(GetGame().IsServer())
		{
			TrapBase trap = TrapBase.Cast(action_data.m_Target.GetObject());
			if(trap)
			{
				trap.SetActivator(action_data.m_Player);
			}
		}
	}
}