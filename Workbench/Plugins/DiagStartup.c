[WorkbenchPluginAttribute("Start Diag Client", "Opens diag client", "Ctrl+ALT+D", "", {"ResourceManager", "ScriptEditor"})]
class StartClient: DayZProjectManager
{
	[Attribute("", "editbox")]
	string ClientStartBatch;

	override void Configure()
	{
		if (ClientStartBatch == string.Empty)
			ClientStartBatch = DefualtClientStartBatch;

		super.Configure();
	}

	override void Run()
	{
		if (ClientStartBatch == string.Empty)
			ClientStartBatch = DefualtClientStartBatch;

		RunDayZBat(ClientStartBatch, true);
	}
}

[WorkbenchPluginAttribute("Start Diag Server", "Opens diag server", "Ctrl+Shift+D", "", {"ResourceManager", "ScriptEditor"})]
class StartServer: DayZProjectManager
{
	[Attribute("", "editbox")]
	string ServerStartBatch;

	override void Configure()
	{
		if (ServerStartBatch == string.Empty)
			ServerStartBatch = DefualtServerStartBatch;

		super.Configure();
	}

	override void Run()
	{
		if (ServerStartBatch == string.Empty)
			ServerStartBatch = DefualtServerStartBatch;

		RunDayZBat(ServerStartBatch, true);
	}
}

[WorkbenchPluginAttribute("Kill DayZ Client", "Stops running diag client", "Ctrl+ALT+X", "", {"ResourceManager", "ScriptEditor"})]
class KillClient: DayZProjectManager
{
	[Attribute("", "editbox")]
	string ClientKillBatch;

	override void Configure()
	{
		if (ClientKillBatch == string.Empty)
			ClientKillBatch = DefualtKillClientBatch;

		super.Configure();
	}

	override void Run()
	{
		if (ClientKillBatch == string.Empty)
			ClientKillBatch = DefualtKillClientBatch;

		RunDayZBat(ClientKillBatch, true);
	}
}

[WorkbenchPluginAttribute("Kill DayZ Server", "Stop running diag server", "Ctrl+Shift+X", "", {"ResourceManager", "ScriptEditor"})]
class KillServer: DayZProjectManager
{
	[Attribute("", "editbox")]
	string ServerKillBatch;

	override void Configure()
	{
		if (ServerKillBatch == string.Empty)
			ServerKillBatch = DefualtKillServerBatch;

		super.Configure();
	}

	override void Run()
	{
		if (ServerKillBatch == string.Empty)
			ServerKillBatch = DefualtKillServerBatch;

		RunDayZBat(ServerKillBatch, true);
	}
}