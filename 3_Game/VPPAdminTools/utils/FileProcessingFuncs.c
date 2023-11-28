class FileProcessingFuncs
{
	/*
	* Delete files in either $saves or $profile
	* works with folders, recursively deletes a directory
	* returns count of deleted files & directories
	*/
	static int DeleteRecursive(string path, bool deleteRoot = false, string pattern = "/*")
	{
		if (!FileExist(path))
		{
			Print("FileProcessingFuncs::DeleteRecursive() path not found: " + path);
			return 0;
		}

		array<string> paths = {};
		AssemblePathStructure(path, paths, pattern);

		int total = 0;
		if (paths && paths.Count() > 0)
		{
			for (int i = 0; i < paths.Count(); ++i)
			{
				if (DeleteFile(paths[i]))
					total++;
			}
		}

		if (deleteRoot)
		{
			if (DeleteFile(path))
				total++;
		}
		return total;
	}

	/*
	* Fetches full structure of a directory recursively 
	* including all child files and dir hierarchy
	* output array with paths in order of folder structure
	* Works with PBOs too ;)
	*/
	static void AssemblePathStructure(string path, out array<string> paths, string pattern = "/*")
	{
		paths = {};
		string fileName;
		FileAttr fileAtts;
		FindFileFlags flags = FindFileFlags.ARCHIVES;

		FindFileHandle fHandle;
		bool complete;

		while(!complete)
		{
			if (!fHandle)
				fHandle = FindFile(path + pattern, fileName, fileAtts, flags); //init find file

			if (fileName != string.Empty || FileExist(fileName))
			{
				if (fileAtts == FileAttr.DIRECTORY)
				{
					array<string> subDirs = {};
					AssemblePathStructure(string.Format("%1/%2", path, fileName), subDirs); //recursive
					if (subDirs && subDirs.Count() > 0)
					{
						paths.InsertAll(subDirs); //we want these to be in order of AFTER the contents of themselves
					}
				}
				paths.Insert(string.Format("%1/%2", path, fileName));
			}
			complete = !FindNextFile(fHandle, fileName, fileAtts);
		}
	}

	/*
	* Helper functions to find users drive paths
	* Credit to: @wrdg#2323 Enfusion Modders Discord.
	*/
	static string GetAbsoluteProfileDirectory() // $profile
	{
	    string defined;
	    if (GetGame().CommandlineGetParam("profiles", defined))
	        return defined;

	    string directory = GetAbsoluteUserDirectory();
	    if (directory)
	        return directory + "AppData\\Local\\DayZ\\";

	    return string.Empty;
	}

	static string GetAbsoluteSavesDirectory() // $saves
	{
	    string defined, directory;
	    if (GetGame().CommandlineGetParam("profiles", defined))
	    {
	    	string pName;
	    	if (!GetGame().CommandlineGetParam("name", pName))
	    		pName = "Survior";
	    	
	        directory = GetAbsoluteUserDirectory(string.Format("%1\\Users\\%2\\", defined, pName));
	        if (directory)
	            return directory;
	    }
	    else
	    {
	        directory = GetAbsoluteUserDirectory();
	        if (directory)
	            return directory + "Documents\\DayZ\\";
	    }

	    return string.Empty;
	}

	static string GetAbsoluteUserDirectory(string rootDirectory = "$saves:")
	{
	    string fileName;
	    FileAttr fileAttr;
	    Print("GetAbsoluteUserDirectory:: " + rootDirectory + "*.core.xml");
	    FindFileHandle file = FindFile(rootDirectory + "*.core.xml", fileName, fileAttr, 0); // just get the first instance (cross fingers)

	    if (file)
	    {
	        string username = fileName.Substring(0, fileName.IndexOf(".core.xml"));
	        CloseFindFile(file);
	        return string.Format("C:\\Users\\%1\\", username);
	    }

	    CloseFindFile(file);
	    return string.Empty;
	}
};