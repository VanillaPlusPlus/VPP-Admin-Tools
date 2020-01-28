class WebHooksManager: ConfigurablePlugin
{
	private ref WebHookConnectionManager m_ConnectionManager;
	private ref array<ref WebHook> 		 M_DATA;

	void WebHooksManager()
	{
		M_DATA = new array<ref WebHook>;
		m_ConnectionManager = new WebHookConnectionManager( "https://discordapp.com/api/webhooks/" );

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
	}

	override void Load()
	{
		if (FileExist( JSONPATH ))
		{
			Print("[WebHooksManager] Loading Saved Webhooks configuration");
			JsonFileLoader<WebHooksManager>.JsonLoadFile(JSONPATH, this);
		}
	}

	override void Save()
	{
		JsonFileLoader<WebHooksManager>.JsonSaveFile(JSONPATH, this);
	}

	/*
		Called from many other instances such as admin logs and vppat plugins
	*/
	void PostData(typename messageType, ref WebHookMessageBase dataClass)
	{
		if (M_DATA.Count() > 0)
		{
			foreach(ref WebHook hook: M_DATA)
			{
				if (hook)
				{
					//Post sent data according to each webhooks configuration
					switch(messageType)
					{
						case AdminActivityMessage:
						if (hook.SendAdminActivityLogs())
						{
							autoptr AdminActivityMessage aam;
							Class.CastTo( aam, dataClass);

							m_ConnectionManager.Post(hook.GetURL(), aam.BuildMessage());
						}
						break;

						case KillDeathMessage:
						if (hook.SendDeathKillLogs())
						{
							autoptr KillDeathMessage kdm;
							Class.CastTo( kdm, dataClass);

							m_ConnectionManager.Post(hook.GetURL(), kdm.BuildMessage());
						}
						break;

						case JoinLeaveMessage:
						if (hook.SendJoinLeaveLogs())
						{
							autoptr JoinLeaveMessage jlm;
							Class.CastTo( jlm, dataClass)
							
							m_ConnectionManager.Post(hook.GetURL(), jlm.BuildMessage());
						}
						break;

						case HitDamageMessage:
						if (hook.SendHitLogs())
						{
							autoptr HitDamageMessage hdm;
							Class.CastTo( hdm, dataClass);

							m_ConnectionManager.Post(hook.GetURL(), hdm.BuildMessage());
						}
						break;
					}
				}
			}
		}
	}

	void GetWebHooks(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server && sender)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWebHooks"))
				return;

			GetRPCManager().SendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender);
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
				M_DATA.RemoveOrdered( data.param1 );
				Save();
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: "+data.param2 + " was successfully deleted!", NotifyTypes.NOTIFY);
				GetRPCManager().SendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
			}else{
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

			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Created new WebHook: " + data.param1.GetName()));
			M_DATA.Insert( data.param1 );
			GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"New WebHook: " + data.param1.GetName() + " successfully created and saved!", NotifyTypes.NOTIFY);
			Save();
			GetRPCManager().SendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
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

			ref WebHook old = M_DATA.Get( data.param1 );
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[WebHooksManager] Edited WebHook: " + old.GetName()));
			if (old && old.GetName() == data.param2)
			{
				M_DATA.Set(data.param1, data.param3);
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"WebHook: " + data.param3.GetName() + " was successfully edited & saved!", NotifyTypes.NOTIFY);
				Save();
				GetRPCManager().SendRPC("RPC_MenuWebHooks", "PopulateList", new Param1<ref array<ref WebHook>>(M_DATA), true, sender); //Reload data for client
			}else{
				//Failed
				GetPermissionManager().NotifyPlayer(sender.GetPlainId(),"Failed to save changes to webhook: " + data.param3.GetName() + "\n\nCould not find old instance, another admin deleted it?", NotifyTypes.NOTIFY);
			}
		}
	}
};

ref WebHooksManager GetWebHooksManager()
{
	return WebHooksManager.Cast(GetPluginManager().GetPluginByType(WebHooksManager));
};