class WebHookConnectionManager
{
	private float 			   currentTime;
	ref WebHookCallBack  	   webHookCB;

	void WebHookConnectionManager(string contextUrl)
	{
		webHookCB  = new WebHookCallBack();
	}
	
	void Post(string address, string data)
	{
		if (data && address != string.Empty)
		{
			RestContext cURLCtx = GetRestApi().GetRestContext(address);
			cURLCtx.SetHeader( "application/json" );
			cURLCtx.POST(webHookCB, "", data);
		}
	}
};