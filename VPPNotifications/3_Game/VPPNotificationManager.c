#ifndef VPPNOTIFICATIONS
class VPPNotificationManager
{
	private static ref VPPNotificationManager instance;

	void VPPNotificationManager()
	{
		instance = this;
		Print("VPPNotificationManager()");
	}

	static void SendMessage(VPPMessageProps message, PlayerIdentity targetIdentity = null, Object targetObject = null)
	{
		if (GetGame().IsServer() && !GetGame().IsClient())
		{
			GetGame().RPCSingleParam(targetObject, RPC_VPP_ADD_NOTIFICATION, new Param1<ref VPPMessageProps>(message), true, targetIdentity);
		}
	}

	static void SendMessages(array<ref VPPMessageProps> messages, PlayerIdentity targetIdentity = null, Object targetObject = null)
	{
		if (GetGame().IsServer() && !GetGame().IsClient())
		{
			GetGame().RPCSingleParam(targetObject, RPC_VPP_ADD_NOTIFICATIONS, new Param1<ref array<ref VPPMessageProps>>(messages), true, targetIdentity);
		}
	}
};
#endif