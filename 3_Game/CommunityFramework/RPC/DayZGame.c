modded class DayZGame
{
	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == RPCManager.VPPAT_FRAMEWORK_RPC_ID)
		{
			GetRPCManager().VOnRPC(sender, target, rpc_type, ctx);
			return;
		}
		super.OnRPC(sender, target, rpc_type, ctx);
	}
};