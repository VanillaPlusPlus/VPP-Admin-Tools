class WebHooksManager: ConfigurablePlugin
{
	[NonSerialized()]
	private ref WebHookConnectionManager m_ConnectionManager;
	[NonSerialized()]
	private bool m_ReportServerStatus;
	[NonSerialized()]
	static ref ScriptInvoker m_Invoker = new ScriptInvoker();

	private ref array<ref WebHook> 		 M_DATA;

	void WebHooksManager()
	{
		M_DATA = new array<ref WebHook>;
		m_ConnectionManager = new WebHookConnectionManager("");

		if (!FileExist("$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager"))
			MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager");

		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager/WebHooks.json";

		/*
			-RPCs-
		*/
		GetRPCManager().AddRPC("RPC_WebHooksManager", "GetWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "DeleteWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "CreateWebHooks", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WebHooksManager", "EditWebHooks", this, SingleplayerExecutionType.Server);
	}

	override void OnInit()
	{
		Load();
		SetupInvokerQueue();
	}

	override void Load()
	{
		if (FileExist( JSONPATH ))
		{
			JsonFileLoader<WebHooksManager>.JsonLoadFile(JSONPATH, this);
			Print("[WebHooksManager] Loading Saved Webhooks configuration");
		}
	}

	override void Save()
	{
		JsonFileLoader<WebHooksManager>.JsonSaveFile(JSONPATH, this);
	}

	void SetupInvokerQueue()
	{
		if (!M_DATA || M_DATA.Count() <= 0)
			return;

		foreach(WebHook hook: M_DATA)
		{
			if (!hook)
				continue;

			hook.InitTimer();
			WebHooksManager.m_Invoker.Insert(hook.PostMessage);
		}
	}

	void PostServerBootup()
	{
		PostData(ServerStatusMessage, new ServerStatusMessage(true));
	}

	/*
		Called from many other instances such as admin logs and vppat plugins
	*/
	void PostData(typename messageType, WebHookMessageBase dataClass)
	{
		if (!M_DATA || M_DATA.Count() <= 0)
			return;

		WebHooksManager.m_Invoker.Invoke(messageType, dataClass);
	}

	void GetWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks", "", false))
				return;

			GetRPCManager().VSendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender);
		}
	}

	void DeleteWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param2<int,string> data;
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Delete")) 
				return;

			if (!ctx.Read(data))
				return;

			if (data.param1 != -1 && M_DATA.Get(data.param1).GetName() == data.param2)
			{
				GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Deleted WebHook: " + M_DATA.Get(data.param1).GetName()));
				M_DATA.RemoveOrdered(data.param1);
				Save();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: "+data.param2 + " was successfully deleted!", NotifyTypes.NOTIFY);
				GetRPCManager().VSendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
			}
			else
			{
				//Failed
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Error deleting webhook: "+data.param2 + "\nobject not found in array!", NotifyTypes.NOTIFY);
			}
		}
	}

	void CreateWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param1<ref WebHook> data;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Create")) 
				return;

			if (!ctx.Read(data))
				return;

			WebHook webHook = data.param1;
			M_DATA.Insert(webHook);
			webHook.InitTimer();
			WebHooksManager.m_Invoker.Insert(webHook.PostMessage);

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Created new WebHook: " + data.param1.GetName()));
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"New WebHook: " + data.param1.GetName() + " successfully created and saved!", NotifyTypes.NOTIFY);
			Save();
			GetRPCManager().VSendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
		}
	}

	void EditWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			Param3<int,string,ref WebHook> data;

			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks:Edit")) 
				return;

			if (!ctx.Read(data))
				return;

			WebHook old = M_DATA.Get(data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Edited WebHook: " + old.GetName()));
			if (old && old.GetName() == data.param2)
			{
				delete old; //call destructor to stop timers n invokers
				WebHooksManager.m_Invoker.Clear();
				M_DATA.RemoveOrdered(data.param1);
				M_DATA.Insert(data.param3);
				SetupInvokerQueue();

				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: " + data.param3.GetName() + " was successfully edited & saved!", NotifyTypes.NOTIFY);
				Save();
				GetRPCManager().VSendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
			}
			else
			{
				//Failed
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Failed to save changes to webhook: " + data.param3.GetName() + "\n\nCould not find old instance, another admin deleted it?", NotifyTypes.NOTIFY);
			}
		}
	}

	WebHookConnectionManager GetConnectionManager()
	{
		return m_ConnectionManager;
	}
};

WebHooksManager GetWebHooksManager()
{
	return WebHooksManager.Cast(GetPluginManager().GetPluginByType(WebHooksManager));
};