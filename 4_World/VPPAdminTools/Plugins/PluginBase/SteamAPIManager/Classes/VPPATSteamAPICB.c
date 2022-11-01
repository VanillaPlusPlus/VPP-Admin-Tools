class VPPATSteamAPICB: RestCallback
{
	void VPPATSteamAPICB()
	{
	}

	override void OnSuccess(string data, int dataSize)
	{
		FileHandle file = OpenFile(string.Format("$profile:GameServer_%1.json", g_Game.VGetServerIP()), FileMode.WRITE);
		if (file != 0)
		{
			FPrintln(file, data);
		}
		CloseFile(file);
		GetSteamAPIManager().HandleSteamQueryCb();
	}

	override void OnError(int errorCode)
	{
		Print("[VPPATSteamAPICB] !!! OnError() " + typename.EnumToString(ERestResultState, errorCode));
	}
};