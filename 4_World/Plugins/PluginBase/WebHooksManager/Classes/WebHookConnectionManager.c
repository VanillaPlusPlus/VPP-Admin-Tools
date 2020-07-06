class WebHookConnectionManager
{
	private float 			   currentTime;
	RestContext    		   	   cURLCtx;
	ref WebHookCallBack  	   webHookCB;

	void WebHookConnectionManager(string contextUrl)
	{
		RestApi cURLCore = CreateRestApi();
		cURLCore.EnableDebug(false);
		webHookCB  = new WebHookCallBack();
		cURLCtx    = cURLCore.GetRestContext( contextUrl ); //Discord only for now
		cURLCtx.SetHeader( "application/json" );
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