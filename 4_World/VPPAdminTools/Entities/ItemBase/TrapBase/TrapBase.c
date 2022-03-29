modded class TrapBase
{
	protected int m_activatorSessionID = -1;

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

	override void OnPlacementComplete( Man player, vector position = "0 0 0", vector orientation = "0 0 0" ) 
	{
		super.OnPlacementComplete(player, position, orientation);
		if(GetGame().IsServer())
		{
			if(player)
			{
				m_activatorSessionID = PlayerBase.Cast(player).VPlayerGetSessionId();
			}
		}
	}
};