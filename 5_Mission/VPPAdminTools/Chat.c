/*!
	channel type, possible values
	CCSystem(1)
	CCAdmin(2)
	CCDirect(4)
	CCMegaphone(8)
	CCTransmitter(16)
	CCPublicAddressSystem(32)

modded class Chat
{
	//TODO: make this less shitty.
	private string systemMessage;
	private bool   isLoggedIn;
	override void Add(ChatMessageEventParams params)
	{
		if (params.param2 == "System")
		{
			systemMessage = params.param3;
			
			if(systemMessage.Contains("Server load:"))
			{
				return;
			}
			
			if (systemMessage == "Logged in as admin")
				isLoggedIn = true;
			else if (systemMessage == "Logged out")
				isLoggedIn = false;
		}
		super.Add(params);
	}
	
	string GetSystemMessage()
	{
		return systemMessage;
	}
	
	bool IsLoggedIn()
	{
		return isLoggedIn;
	}
};
*/