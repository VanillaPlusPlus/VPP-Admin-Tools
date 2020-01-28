class WebHookConnectionManager
{
	private float 			   currentTime;
	CURLContext    		   	   cURLCtx;
	ref WebHookCallBack  	   webHookCB;

	void WebHookConnectionManager(string contextUrl)
	{
		CURLCore cURLCore = CreateCURLCore();
		cURLCore.EnableDebug(false);
		webHookCB  = new WebHookCallBack();
		cURLCtx    = cURLCore.GetCURLContext( contextUrl ); //Discord only for now
		cURLCtx.SetHeader( "application/json" );
	}

	void ~WebHookConnectionManager()
	{
		delete cURLCtx;
		delete webHookCB;
	}
	
	void Post(string address, string data)
	{
		if (data)
		{
			address.Replace("https://discordapp.com/api/webhooks/", "");
			cURLCtx.POST( webHookCB, address, data);
		}
	}
};