class DayZProjectManager: WorkbenchPlugin
{
	static string DefualtClientStartBatch = "P:\\DevEnvironment\\@StartClient.bat";
	static string DefualtServerStartBatch = "P:\\DevEnvironment\\@StartServer.bat";

	static string DefualtKillClientBatch = "P:\\DevEnvironment\\@KillClient.bat";
	static string DefualtKillServerBatch = "P:\\DevEnvironment\\@KillServer.bat";

	override void Configure()
	{
		Workbench.ScriptDialog("Edit Command Settings", "", this);
	}
	
	void RunDayZBat(string filepath, bool wait = false)
	{
		if (filepath.Length() < 2) return;
		
		filepath.Replace("\\", "/");
		
		if (filepath[1] != ":")
		{
			string cwd;
			Workbench.GetCwd(cwd);
			filepath = cwd + "/" + filepath;
		}
		
		int index = filepath.IndexOf("/");
		int last_index = index;
		
		while(index != -1)
		{
			last_index = index;
			index = filepath.IndexOfFrom(last_index + 1, "/");
		}
		
		if (last_index == -1) return;
		
		string path = filepath.Substring(0, last_index);
		string bat = "\"" + filepath.Substring(last_index + 1, filepath.Length() - last_index - 1) + "\"";
		
		string fullCmd = "cmd /c \"chdir /D " + path + " & call " + bat + "\"";
		Workbench.RunCmd(fullCmd, wait);
	}
	
	[ButtonAttribute("Apply")]
	void DialogOk()
	{
		Workbench.Dialog("Success!", "Settings saved!");
	}
	
	void RunDayZBatList(array<string> filepaths)
	{
		foreach (string file: filepaths)
		{
			RunDayZBat(file, true);
		}
	}
}
