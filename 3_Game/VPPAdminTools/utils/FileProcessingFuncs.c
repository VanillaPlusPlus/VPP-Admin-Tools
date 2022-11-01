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
};