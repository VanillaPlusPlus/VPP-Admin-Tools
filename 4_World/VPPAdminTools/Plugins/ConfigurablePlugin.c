class ConfigurablePlugin : PluginBase
{
	[NonSerialized()]
	protected string JSONPATH;
	
	void ConfigurablePlugin()
	{
		JSONPATH = "";
	}
	
	override void OnInit();
	void Load();
	void Save();

	//Currently used only for the building manager
	protected void GetFilePaths(inout array<string> m_Files)
	{
		FindFileHandle FileSearch;
		string FileName;
		string FullPath;
		string Filter = ".vpp";
		int file_attr;
		m_Files = new array<string>;
		
		if (FileExist("$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager"))
		{
			string path_find_pattern = "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager" + "\\*";
			FileSearch = FindFile(path_find_pattern, FileName, file_attr, FindFileFlags.ALL);
	
			bool found = true;
			while (found)
			{
				if ( file_attr != FileAttr.DIRECTORY )
				{
					FullPath = "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager/" + FileName;
					if (FileName != "")
					{
						string fileType = FileName.Substring((FileName.Length() - Filter.Length()), Filter.Length());
						if (fileType == Filter)
						{
							m_Files.Insert(FullPath);
						}
					}
				}
				found = FindNextFile(FileSearch, FileName, file_attr);
			}
			GetSimpleLogger().Log("Building Sets Loaded: " + m_Files.Count());
			CloseFindFile(FileSearch);
		}
	}
};