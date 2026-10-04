/*
	Building set journal (server). DayZ 1.30 broke FindFile, so the Building Set Manager can no longer list its
	folder to find the saved sets. The journal is a JSON file next to the sets that records the full path of every
	set file the server writes; loading reads exactly those paths.

	BuildingSetJournal.json
	{
		"Version": 1,
		"ScanFolder": 1,          // also try FindFile on boot (capped); finds sets added by hand, harmless when broken
		"Sets": [ { "Name": "MyBase", "Path": "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager/MyBase.vpp" } ],
		"History": [ { "Time": "2026-10-04 12:00:00", "Action": "save", "Name": "MyBase", "Path": "..." } ]
	}

	Writes go to .tmp first; the previous journal is kept as .bak and is used when the main file cannot be read (an
	unreadable main file is kept as .corrupt). Sets can also be added by hand: a "Path" may be a full path, a file
	name or just the set name, or list them one per line in BuildingSetImport.txt (merged on boot, then renamed .done).
*/
class VPPBuildingSetJournalEntry : Managed
{
	string Name;
	string Path;

	void VPPBuildingSetJournalEntry()
	{
		Name = "";
		Path = "";
	}
};

class VPPBuildingSetJournalEvent : Managed
{
	string Time;
	string Action;
	string Name;
	string Path;
};

class VPPBuildingSetJournal : Managed
{
	const static int VERSION = 1;
	const static int HISTORY_MAX = 100;
	const static string FILE_NAME = "BuildingSetJournal.json";
	const static string IMPORT_NAME = "BuildingSetImport.txt";
	const static string HELPER_NAME = "ImportBuildingSets.bat";
	const static string SET_EXT = ".vpp";

	int Version;
	int ScanFolder;
	ref array<ref VPPBuildingSetJournalEntry> Sets;
	ref array<ref VPPBuildingSetJournalEvent> History;

	[NonSerialized()]
	protected string m_Dir;
	[NonSerialized()]
	protected string m_File;
	[NonSerialized()]
	protected bool m_Dirty;

	void VPPBuildingSetJournal()
	{
		Version = VERSION;
		ScanFolder = 1;
		Sets = new array<ref VPPBuildingSetJournalEntry>();
		History = new array<ref VPPBuildingSetJournalEvent>();
		m_Dir = "";
		m_File = "";
	}

	// dir: the set folder, ending with "/"
	void SetFolder(string dir)
	{
		m_Dir = dir;
		m_File = dir + FILE_NAME;
	}

	string GetFolder()
	{
		return m_Dir;
	}

	bool IsDirty()
	{
		return m_Dirty;
	}

	// ---------------------------------------------------------------- paths

	// The file of a set name, as the manager writes it.
	string PathFor(string setName)
	{
		return m_Dir + setName + SET_EXT;
	}

	// A hand-written entry made usable: "MyBase", "MyBase.vpp" or a full path ("$profile:..." / "C:/...").
	string NormalizePath(string text)
	{
		string path = text.Trim();
		string lower = "";
		if (path == "")
		{
			return "";
		}

		path.Replace("\\", "/");
		if (path.IndexOf(":") < 0 && path.IndexOf("/") < 0)
		{
			path = m_Dir + path;
		}

		lower = path;
		lower.ToLower();
		if (lower.Length() < 4 || lower.Substring(lower.Length() - 4, 4) != SET_EXT)
		{
			path = path + SET_EXT;
		}

		return path;
	}

	protected int IndexOfPath(string path)
	{
		string wanted = path;
		string other = "";
		int total = Sets.Count();
		int i = 0;
		wanted.ToLower();
		for (i = 0; i < total; i++)
		{
			if (!Sets[i])
			{
				continue;
			}

			other = Sets[i].Path;
			other.ToLower();
			if (other == wanted)
			{
				return i;
			}
		}

		return -1;
	}

	bool HasPath(string path)
	{
		return IndexOfPath(path) >= 0;
	}

	// The paths to load, in journal order.
	void GetPaths(array<string> outPaths)
	{
		outPaths.Clear();
		foreach (VPPBuildingSetJournalEntry entry : Sets)
		{
			if (entry && entry.Path != "")
			{
				outPaths.Insert(entry.Path);
			}
		}
	}

	// ---------------------------------------------------------------- changes

	// Records (or refreshes) a set file. action ("" = no history line): save, rename, import, scan.
	void Put(string setName, string path, string action)
	{
		int index = IndexOfPath(path);
		VPPBuildingSetJournalEntry entry = null;
		if (index >= 0)
		{
			entry = Sets[index];
			if (entry.Name != setName)
			{
				entry.Name = setName;
				m_Dirty = true;
			}
		}
		else
		{
			entry = new VPPBuildingSetJournalEntry();
			entry.Name = setName;
			entry.Path = path;
			Sets.Insert(entry);
			m_Dirty = true;
		}

		if (action != "")
		{
			Record(action, setName, path);
		}

		// one entry per set: an older path of the same set (e.g. registered by hand elsewhere, now saved into the
		// set folder) is dropped so the set is not loaded twice
		if (setName == "")
		{
			return;
		}

		for (int i = Sets.Count() - 1; i >= 0; i--)
		{
			VPPBuildingSetJournalEntry other = Sets[i];
			if (other && other != entry && other.Name == setName)
			{
				string otherPath = other.Path;
				Sets.RemoveOrdered(i);
				Record("moved", setName, otherPath);
			}
		}
	}

	// action: delete, rename, missing, unreadable
	void Remove(string path, string action)
	{
		int index = IndexOfPath(path);
		string setName = "";
		if (index < 0)
		{
			return;
		}

		setName = Sets[index].Name;
		Sets.RemoveOrdered(index);
		m_Dirty = true;
		Record(action, setName, path);
	}

	protected void Record(string action, string setName, string path)
	{
		VPPBuildingSetJournalEvent change = new VPPBuildingSetJournalEvent();
		change.Time = TimeText();
		change.Action = action;
		change.Name = setName;
		change.Path = path;
		History.Insert(change);
		while (History.Count() > HISTORY_MAX)
		{
			History.RemoveOrdered(0);
		}

		m_Dirty = true;
	}

	protected static string TimeText()
	{
		int year = 0;
		int month = 0;
		int day = 0;
		int hour = 0;
		int minute = 0;
		int second = 0;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		string dateText = year.ToString() + "-" + Pad(month) + "-" + Pad(day);
		string clockText = Pad(hour) + ":" + Pad(minute) + ":" + Pad(second);
		return dateText + " " + clockText;
	}

	protected static string Pad(int value)
	{
		if (value < 10)
		{
			return "0" + value.ToString();
		}

		return value.ToString();
	}

	// ---------------------------------------------------------------- load / save

	// Reads the journal (the .bak when the main file cannot be read). False when there is none yet.
	bool Load()
	{
		bool loaded = false;
		string backupPath = m_File + ".bak";
		if (FileExist(m_File))
		{
			loaded = LoadFrom(m_File);
			if (!loaded)
			{
				string corruptPath = m_File + ".corrupt";
				if (FileExist(corruptPath))
				{
					DeleteFile(corruptPath);
				}

				CopyFile(m_File, corruptPath);
				GetSimpleLogger().Log("[BuildingSetManager] The building set journal could not be read; kept as " + corruptPath);
			}
		}

		if (!loaded && FileExist(backupPath))
		{
			loaded = LoadFrom(backupPath);
			if (loaded)
			{
				GetSimpleLogger().Log("[BuildingSetManager] Building set journal restored from " + backupPath);
				m_Dirty = true;
			}
		}

		return loaded;
	}

	protected bool LoadFrom(string path)
	{
		VPPBuildingSetJournal stored = null;
		string error = "";
		if (!JsonFileLoader<VPPBuildingSetJournal>.LoadFile(path, stored, error) || !stored)
		{
			GetSimpleLogger().Log("[BuildingSetManager] " + error);
			return false;
		}

		Sets.Clear();
		History.Clear();
		ScanFolder = stored.ScanFolder;
		if (stored.Sets)
		{
			foreach (VPPBuildingSetJournalEntry entry : stored.Sets)
			{
				if (!entry)
				{
					continue;
				}

				string entryPath = NormalizePath(entry.Path);
				if (entryPath == "" && entry.Name != "")
				{
					entryPath = NormalizePath(entry.Name);
				}

				if (entryPath == "" || HasPath(entryPath))
				{
					m_Dirty = true;
					continue;
				}

				if (entryPath != entry.Path)
				{
					m_Dirty = true;
				}

				VPPBuildingSetJournalEntry kept = new VPPBuildingSetJournalEntry();
				kept.Name = entry.Name;
				kept.Path = entryPath;
				Sets.Insert(kept);
			}
		}

		if (stored.History)
		{
			foreach (VPPBuildingSetJournalEvent change : stored.History)
			{
				if (change)
				{
					History.Insert(change);
				}
			}
		}

		if (stored.Version != VERSION)
		{
			m_Dirty = true;
		}

		return true;
	}

	// Writes .tmp, keeps the previous journal as .bak, then swaps the new one in.
	bool Save()
	{
		string tmpPath = m_File + ".tmp";
		string backupPath = m_File + ".bak";
		string error = "";
		Version = VERSION;
		if (!JsonFileLoader<VPPBuildingSetJournal>.SaveFile(tmpPath, this, error))
		{
			GetSimpleLogger().Log("[BuildingSetManager] FAILED to write the building set journal: " + error);
			return false;
		}

		if (FileExist(m_File))
		{
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			CopyFile(m_File, backupPath);
			DeleteFile(m_File);
		}

		if (!CopyFile(tmpPath, m_File))
		{
			GetSimpleLogger().Log("[BuildingSetManager] FAILED to replace the building set journal; the previous one is " + backupPath);
			if (FileExist(backupPath) && !FileExist(m_File))
			{
				CopyFile(backupPath, m_File);
			}

			return false;
		}

		DeleteFile(tmpPath);
		m_Dirty = false;
		return true;
	}

	void SaveIfDirty()
	{
		if (m_Dirty)
		{
			Save();
		}
	}

	// ---------------------------------------------------------------- adding sets by hand / scanning

	// BuildingSetImport.txt: one set name, file name or path per line ("//" comments allowed). Renamed to .done.
	int ImportList()
	{
		string importPath = m_Dir + IMPORT_NAME;
		string line = "";
		string path = "";
		int added = 0;
		int lines = 0;
		int length = 0;
		int emptyRun = 0;
		if (!FileExist(importPath))
		{
			return 0;
		}

		FileHandle handle = OpenFile(importPath, FileMode.READ);
		if (handle == 0)
		{
			GetSimpleLogger().Log("[BuildingSetManager] Cannot open " + importPath);
			return 0;
		}

		while (lines < 2000)
		{
			line = "";
			length = FGets(handle, line);
			if (length < 0)
			{
				break;
			}

			lines++;
			line = line.Trim();
			// FGets may report the end of the file as endless empty lines
			if (line == "")
			{
				emptyRun++;
				if (emptyRun >= 50)
				{
					break;
				}

				continue;
			}

			emptyRun = 0;
			if (line.IndexOf("//") == 0)
			{
				continue;
			}

			path = NormalizePath(line);
			if (path == "" || HasPath(path))
			{
				continue;
			}

			Put("", path, "import");
			added++;
		}

		CloseFile(handle);
		string donePath = importPath + ".done";
		if (FileExist(donePath))
		{
			DeleteFile(donePath);
		}

		CopyFile(importPath, donePath);
		DeleteFile(importPath);
		GetSimpleLogger().Log("[BuildingSetManager] Imported " + added.ToString() + " building set path(s) from " + IMPORT_NAME);
		return added;
	}

	// Writes ImportExistingBuildingSets.bat into the set folder (once; delete it to get a fresh copy). Run on the
	// server machine, it appends the file name of every *.vpp in its own folder to BuildingSetImport.txt, so sets
	// saved before the journal existed are imported on the next restart (already listed ones are skipped).
	bool WriteImportHelper()
	{
		string helperPath = m_Dir + HELPER_NAME;
		int carriageCode = 13;
		string crlf = carriageCode.AsciiToString() + "\n";
		array<string> lines = new array<string>();
		if (FileExist(helperPath))
		{
			return false;
		}

		lines.Insert("@echo off");
		lines.Insert("rem VPP Admin Tools - building set import helper, written by the server.");
		lines.Insert("rem DayZ 1.30 can not list this folder, so the server only loads the sets named in BuildingSetJournal.json.");
		lines.Insert("rem Run this file once: it adds every building set (*.vpp) in this folder to " + IMPORT_NAME + ",");
		lines.Insert("rem and the server imports them into the journal on the next restart.");
		lines.Insert("rem chcp 65001: names are written as UTF-8, as the server reads them");
		lines.Insert("chcp 65001 >nul");
		lines.Insert("setlocal");
		lines.Insert("pushd \"%~dp0\"");
		lines.Insert("set COUNT=0");
		lines.Insert("for %%F in (*.vpp) do (");
		lines.Insert("    >>\"" + IMPORT_NAME + "\" echo %%~nxF");
		lines.Insert("    set /a COUNT+=1");
		lines.Insert(")");
		lines.Insert("popd");
		lines.Insert("echo Added %COUNT% building set(s) to " + IMPORT_NAME + ".");
		lines.Insert("echo Restart the server to import them. Sets that are already in the journal are skipped.");
		lines.Insert("pause");
		FileHandle handle = OpenFile(helperPath, FileMode.WRITE);
		if (handle == 0)
		{
			GetSimpleLogger().Log("[BuildingSetManager] Cannot write " + helperPath);
			return false;
		}

		foreach (string line : lines)
		{
			FPrint(handle, line + crlf);
		}

		CloseFile(handle);
		GetSimpleLogger().Log("[BuildingSetManager] Wrote " + helperPath + ": run it once to import building sets saved before the journal");
		return true;
	}

	// The pre-1.30 folder scan, kept as a helper: every *.vpp FindFile returns is added. Capped so a broken FindFile
	// (endless or garbage results) cannot hang the boot; ScanFolder 0 in the journal turns it off.
	int ScanForSets()
	{
		string fileName = "";
		string path = "";
		string lower = "";
		FileAttr attributes = 0;
		int added = 0;
		int steps = 0;
		bool more = true;
		if (ScanFolder == 0)
		{
			return 0;
		}

		string pattern = m_Dir + "*";
		FindFileHandle finder = FindFile(pattern, fileName, attributes, FindFileFlags.ALL);
		while (more && steps < 5000)
		{
			steps++;
			lower = fileName;
			lower.ToLower();
			if (fileName != "" && attributes != FileAttr.DIRECTORY && lower.Length() > 4 && lower.Substring(lower.Length() - 4, 4) == SET_EXT)
			{
				path = m_Dir + fileName;
				if (!HasPath(path) && FileExist(path))
				{
					Put("", path, "scan");
					added++;
				}
			}

			fileName = "";
			more = FindNextFile(finder, fileName, attributes);
		}

		CloseFindFile(finder);
		return added;
	}
};
