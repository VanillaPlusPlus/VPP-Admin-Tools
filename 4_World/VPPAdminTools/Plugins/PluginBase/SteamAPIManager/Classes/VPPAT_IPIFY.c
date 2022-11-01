class VPPAT_IPIFY: RestCallback
{
	override void OnSuccess(string data, int dataSize)
	{
		g_Game.VSetServerIP(data);
	}
};