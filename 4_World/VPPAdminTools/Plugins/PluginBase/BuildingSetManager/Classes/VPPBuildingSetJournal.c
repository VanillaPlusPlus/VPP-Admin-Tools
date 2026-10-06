/*
	Building set journal (server). DayZ 1.30 broke FindFile ($ prefixes and backslashes), so the saved sets are found
	with CF.FindFileEx, Community Framework's workaround (it falls back to the engine's FindFile once that is fixed).
	CF can only list folders inside the game directory, so the journal also records the full path of every set file
	the server writes: those sets load even on a server whose profile folder is elsewhere.

	BuildingSetJournal.json
	{
		"Version": 1,
		"Sets": [ { "Name": "MyBase", "Path": "$profile:VPPAdminTools/ConfigurablePlugins/BuildingSetManager/MyBase.vpp" } ],
		"History": [ { "Time": "2026-10-06 12:00:00", "Action": "save", "Name": "MyBase", "Path": "..." } ]
	}

	Writes go to .tmp first; the previous journal is kept as .bak and is used when the main file cannot be read (an
	unreadable main file is kept as .corrupt). A "Path" may also be written by hand as a full path, a file name or
	just the set name.
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
	const static string SET_EXT = ".vpp";

	int Version;
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

	// Records (or refreshes) a set file. action ("" = no history line): save, scan.
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

	// ---------------------------------------------------------------- folder scan

	// Every *.vpp in the set folder that the journal does not list yet is added (sets saved before the journal, or
	// copied in by hand). CF.FindFileEx resolves "$profile:" itself while the engine's FindFile is broken. Capped so
	// endless or garbage results cannot hang the boot.
	int ScanForSets()
	{
		string fileName = "";
		string path = "";
		string lower = "";
		FileAttr attributes = 0;
		int added = 0;
		int steps = 0;
		bool more = true;
		string pattern = m_Dir + "*";
		FindFileHandle finder = CF.FindFileEx(pattern, fileName, attributes, FindFileFlags.ALL);
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
