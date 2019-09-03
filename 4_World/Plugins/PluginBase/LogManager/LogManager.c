class LogManager : PluginBase
{
    private string 	   m_Logpath;
	private FileHandle m_LogFile;
	   
    void LogManager()
    {
		//-----RPC's-----
		GetRPCManager().AddRPC("RPC_LogManager", "GetLogData", this, SingeplayerExecutionType.Server );
		//---------------
		
		MakeDirectory("$profile:VPPAdminTools/Logging");
		
		int hour, minute, second;
		int year, month, day;
		
		GetHourMinuteSecondUTC(hour,minute,second);
		GetYearMonthDayUTC(year,month,day);
		
		string timeStamp = string.Format("[%1-%2-%3]--[%4-%5-%6]",year.ToString(),month.ToString(),day.ToString(),hour.ToString(),minute.ToString(),second.ToString());
		m_Logpath = string.Format("$profile:VPPAdminTools/Logging/Log_%1.txt",timeStamp);
		
    	m_LogFile = OpenFile(m_Logpath, FileMode.WRITE);
		FPrintln(m_LogFile, "=========================Vanilla++ Admin Tools Session Logs=========================");
		FPrintln(m_LogFile, "Logs Started: "+timeStamp);
		FPrintln(m_LogFile, "====================================================================================");
		CloseFile(m_LogFile);
    }
	
	void ~LogManager()
	{
		CloseFile(m_LogFile);
	}
	
	void Log(string str)
	{
		m_LogFile = OpenFile(m_Logpath, FileMode.APPEND);
		FPrintln(m_LogFile, str);
		CloseFile(m_LogFile);
	}
	
	//--------RPC Calls--------
	void GetLogData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server )
        {
			if(!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "LogsViewer:ViewLogs")) return;
			
			autoptr array<string> paths;
			CollectLogPaths(paths);
			
			foreach(string logPath : paths)
			{
				if (logPath.Contains(data.param1))
				{
					autoptr array<string> logData;
					LogToArray(logPath,logData);
					GetRPCManager().SendRPC("RPC_MenuLogsViewer", "SortLogsList", new Param1<ref array<string>>(logData), true, sender);
				}
			}
		}
	}
	//------------------------
	
	private void LogToArray(string path, out array<string> ContentLines)
	{
		FileHandle logFile = OpenFile( path,FileMode.READ );
		ContentLines = new array<string>;
		
		if (logFile != 0)
		{
			string line_content;
			while ( FGets( logFile, line_content ) > 0 )
			{
				ContentLines.Insert(line_content);
			}
			CloseFile(logFile);
		}
	}
	
	private void CollectLogPaths(out ref array<string> m_Files)
	{
		FindFileHandle FileSearch;
		string FileName;
		string FullPath;
		string Filter = ".txt";
		int file_attr;
		m_Files = new array<string>;
		
		if (FileExist("$profile:"))
		{
			string path_find_pattern = "$profile:VPPAdminTools/Logging" + "\\*";
	
			FileSearch = FindFile(path_find_pattern, FileName, file_attr, FindFileFlags.ALL);
	
			bool found = true;
			while (found)
			{
				if ( file_attr != FileAttr.DIRECTORY )
				{
					FullPath = "$profile:VPPAdminTools/Logging" + "/" + FileName;
					
					string fileType = FileName.Substring((FileName.Length() - Filter.Length()), Filter.Length());
					if (fileType == Filter)
					{
						m_Files.Insert(FullPath);
						Print(FullPath);
					}
				}
				else if ( file_attr == FileAttr.DIRECTORY )
				{
					FullPath = "$profile:VPPAdminTools/Logging" + "/" + FileName;
					GetFileFromFolder(FullPath,m_Files,".txt");
				}
				found = FindNextFile(FileSearch, FileName, file_attr);
			}
			CloseFindFile(FileSearch);
		}
	}
	
	private void GetFileFromFolder(string FolderPath, array<string> m_Files,string filter)
	{
		FindFileHandle FileSearch;
	
		string FileName;
		string FullPath;
		string Filter = filter;
		int 	file_attr;
		int	flags;
	
		if (FileExist(FolderPath))
		{
			string path_find_pattern = FolderPath + "\\*";
	
			FileSearch = FindFile(path_find_pattern, FileName, file_attr, FindFileFlags.ALL);
	
			bool found = true;
			while (found)
			{
				if ( file_attr != FileAttr.DIRECTORY )
				{
					FullPath = FolderPath + "/" + FileName;
					
					string fileType = FileName.Substring((FileName.Length() - Filter.Length()), Filter.Length());
					if (fileType == Filter)
					{
						m_Files.Insert(FullPath);
					}
				}
				else if ( file_attr == FileAttr.DIRECTORY )
				{
					FullPath = FolderPath + "/" + FileName;
					GetFileFromFolder(FullPath,m_Files,".txt");
				}
				found = FindNextFile(FileSearch, FileName, file_attr);
			}
			CloseFindFile(FileSearch);
		}
	}
};

LogManager GetSimpleLogger()
{
	return LogManager.Cast(GetPluginManager().GetPluginByType(LogManager));
}