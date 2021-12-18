modded class TransferValues 
{
	const float VALUE_CHECK_INTERVAL = 3;

	//include admin clients to receive the RPC
	override void SendValue(int value_type, float value)
	{
		super.SendValue(value_type, value);

		array<Man> players = {};
		EnScript.GetClassVar(GetGame().GetMission(), "m_Players", -1, players);

		foreach(Man player : players)
		{
			PlayerBase pb = PlayerBase.Cast(player);
			if (!pb || !pb.IsAlive())
				continue;

			if (!GetPermissionManager().HasUserGroup(pb.VPlayerGetSteamId()))
				continue;

			GetGame().RPCSingleParam(player, VPPATRPCs.RPC_SYNC_TRANSFER_VALUES, new Param2<int,float>(value_type, value), true, player.GetIdentity());
		}
	}
};