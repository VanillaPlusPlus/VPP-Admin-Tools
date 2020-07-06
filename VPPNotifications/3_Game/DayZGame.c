#ifndef VPPNOTIFICATIONS
modded class DayZGame
{
	void DayZGame()
	{
		new VPPNotificationManager;
		Print("[VPPAT] Internal VPPNotifications module loaded!");
	}

	/*
		For older versions support. We all know retards wont update.
	*/
	void SendMessage(bool Global, PlayerIdentity DirectIdentity, string Title, string Message, int msgLength, int FadeInTime, /*notused*/bool ForceShow = false, /*notused*/bool DoFadeIn = false, string ImagePath = "", float SizeX = 1.0, float SizeY = 1.0)
	{
		if (GetGame().IsServer() && !GetGame().IsClient())
		{
			if (Global)
			{
				VPPNotificationManager.SendMessage(new VPPMessageProps(Title, Message, msgLength, FadeInTime, ImagePath, SizeX, SizeY));
				return;
			}

			VPPNotificationManager.SendMessage(new VPPMessageProps(Title, Message, msgLength, FadeInTime, ImagePath, SizeX, SizeY), DirectIdentity);
		}
	}
}

const int RPC_VPP_ADD_NOTIFICATION  = 47496254;
const int RPC_VPP_ADD_NOTIFICATIONS = 49496254;
#endif