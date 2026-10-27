/*
	Building set files (server).

	Sets are saved as JSON, one file per set in the set folder (<name>.json), so they can be edited by hand:
	{
		"Version": 2,
		"Name": "MyBase",
		"Active": true,
		"Objects": [
			{ "TypeName": "Land_Mil_Tower_Small", "Position": [3700.5, 402.1, 5960.2], "Orientation": [90.0, 0.0, 0.0],
			  "Active": true, "Scale": 1.0 }
		]
	}
	A field left out keeps its default (Active true, Scale 1). Every save writes .tmp first and keeps the previous file
	as .json.bak.

	Versions
	- 1: the old binary FileSerializer dump (<name>.vpp). Converted once at boot: the set is written as JSON, the .vpp
	  is moved to $profile:VPPAdminTools/Backups/BuildingSets/ and the journal points at the JSON from then on.
	- 2: the first JSON version (adds Scale).
	A JSON file older than CURRENT_VERSION is upgraded in VPPBuildingSetFile.Upgrade() and saved again (previous file
	kept as .json.bak). To change the format: add the field (with its default in the constructor), bump
	CURRENT_VERSION and add an "if (Version < N)" step to Upgrade() that fills it for older files.
*/
class VPPBuildingSetFileObject : Managed
{
	string TypeName;
	vector Position;
	vector Orientation;
	bool Active;
	float Scale;

	void VPPBuildingSetFileObject()
	{
		TypeName = "";
		Active = true;
		Scale = 1.0;
	}
};

class VPPBuildingSetFile : Managed
{
	const static int CURRENT_VERSION = 2;
	const static int LEGACY_VERSION = 1;

	int Version;
	string Name;
	bool Active;
	ref array<ref VPPBuildingSetFileObject> Objects;

	void VPPBuildingSetFile()
	{
		Version = CURRENT_VERSION;
		Name = "";
		Active = false;
		Objects = new array<ref VPPBuildingSetFileObject>;
	}

	// Brings a file of an older version up to CURRENT_VERSION; true when anything changed (then it is saved again).
	bool Upgrade()
	{
		bool changed = false;
		if (!Objects)
		{
			Objects = new array<ref VPPBuildingSetFileObject>;
			changed = true;
		}

		// a hand-made file without "Version" reads as 0: the fields it has are the version 2 fields
		if (Version < 2)
		{
			Version = 2;
			changed = true;
		}

		// future: if (Version < 3) { fill the new field of every object; Version = 3; changed = true; }

		for (int i = Objects.Count() - 1; i >= 0; i--)
		{
			VPPBuildingSetFileObject entry = Objects[i];
			if (!entry || entry.TypeName == "")
			{
				Objects.RemoveOrdered(i);
				changed = true;
				continue;
			}

			float clamped = Math.Clamp(entry.Scale, VPPBuildingSetStore.SCALE_MIN, VPPBuildingSetStore.SCALE_MAX);
			if (entry.Scale <= 0)
			{
				clamped = 1.0;
			}

			if (clamped != entry.Scale)
			{
				entry.Scale = clamped;
				changed = true;
			}
		}

		return changed;
	}

	BuildingSet ToBuildingSet()
	{
		BuildingSet bSet = new BuildingSet(Name, Active);
		array<ref SpawnedBuilding> buildings = bSet.GetBuildings();
		foreach (VPPBuildingSetFileObject entry : Objects)
		{
			SpawnedBuilding building = new SpawnedBuilding(entry.TypeName, entry.Position, entry.Orientation, false);
			building.LoadState(entry.Active, entry.Scale);
			buildings.Insert(building);
		}

		return bSet;
	}

	static VPPBuildingSetFile FromBuildingSet(BuildingSet bSet)
	{
		VPPBuildingSetFile setFile = new VPPBuildingSetFile();
		setFile.Name = bSet.GetName();
		setFile.Active = bSet.GetActive();
		array<ref SpawnedBuilding> buildings = bSet.GetBuildings();
		foreach (SpawnedBuilding building : buildings)
		{
			if (!building)
			{
				continue;
			}

			VPPBuildingSetFileObject entry = new VPPBuildingSetFileObject();
			entry.TypeName = building.GetTypeName();
			entry.Position = building.GetPosition();
			entry.Orientation = building.GetOrientation();
			entry.Active = building.GetActive();
			entry.Scale = building.GetScale();
			setFile.Objects.Insert(entry);
		}

		return setFile;
	}
};

/*
	Version 1 readers: exact copies of the 2019-2026 BuildingSet / SpawnedBuilding member lists. The binary reader
	matches members by position and first checks that the stored member count equals the class's count (NonSerialized
	and const members included), so these lists, their types, order and modifiers must NEVER change (checked by
	tools/verify_object_manager.py). Read only, never written.
*/
class VPPSpawnedBuildingLegacy : Managed
{
	private string m_Name;
	private string m_ObjectName;
	private vector m_Position;
	private vector m_Direction;
	private vector m_Orientation;
	private bool   m_IsActive;
	[NonSerialized()]
	private Object m_Building;
	private string m_NetWorkId;

	string GetObjectName()
	{
		return m_ObjectName;
	}

	vector GetPosition()
	{
		return m_Position;
	}

	vector GetOrientation()
	{
		return m_Orientation;
	}

	bool GetActive()
	{
		return m_IsActive;
	}
};

class VPPBuildingSetLegacy : Managed
{
	private string m_Name;
	private bool   m_IsActive;
	private ref array<ref VPPSpawnedBuildingLegacy> m_Buildings;

	string GetName()
	{
		return m_Name;
	}

	bool GetActive()
	{
		return m_IsActive;
	}

	array<ref VPPSpawnedBuildingLegacy> GetBuildings()
	{
		return m_Buildings;
	}
};

// What a read found (filled by VPPBuildingSetStore.ReadJson).
class VPPBuildingSetReadInfo : Managed
{
	int FileVersion;
	bool Upgraded;
	string Error;
};

class VPPBuildingSetStore
{
	const static string JSON_EXT = ".json";
	const static string LEGACY_EXT = ".vpp";
	const static string LEGACY_BACKUP_DIR = "$profile:VPPAdminTools/Backups/BuildingSets/";
	// scale limits of a placed object (file, runtime and the Object Builder attributes window)
	const static float SCALE_MIN = 0.01;
	const static float SCALE_MAX = 100.0;

	static bool HasExtension(string path, string ext)
	{
		string lower = path;
		lower.ToLower();
		int extLength = ext.Length();
		if (lower.Length() <= extLength)
		{
			return false;
		}

		return lower.Substring(lower.Length() - extLength, extLength) == ext;
	}

	static bool IsLegacyPath(string path)
	{
		return HasExtension(path, LEGACY_EXT);
	}

	// <folder>/<name>.vpp -> <folder>/<name>.json
	static string JsonPathOf(string legacyPath)
	{
		int extLength = LEGACY_EXT.Length();
		string stem = legacyPath.Substring(0, legacyPath.Length() - extLength);
		return stem + JSON_EXT;
	}

	// A JSON set file; null when it cannot be read. info.Upgraded = it was older than CURRENT_VERSION and was brought
	// up to date in memory (the caller saves it again).
	static BuildingSet ReadJson(string path, VPPBuildingSetReadInfo info)
	{
		VPPBuildingSetFile setFile = null;
		string error = "";
		if (!JsonFileLoader<VPPBuildingSetFile>.LoadFile(path, setFile, error) || !setFile)
		{
			if (info)
			{
				info.Error = error;
			}

			return null;
		}

		int fileVersion = setFile.Version;
		bool upgraded = setFile.Upgrade();
		if (info)
		{
			info.FileVersion = fileVersion;
			info.Upgraded = upgraded;
		}

		if (fileVersion > VPPBuildingSetFile.CURRENT_VERSION)
		{
			GetSimpleLogger().Log("[BuildingSetManager] " + path + " was written by a newer version (format " + fileVersion.ToString() + "); fields this version does not know are dropped when it is saved");
		}

		return setFile.ToBuildingSet();
	}

	// A version 1 (.vpp) set; null when it cannot be read or its body does not match the version 1 layout.
	static BuildingSet ReadLegacy(string path)
	{
		if (!FileExist(path))
		{
			return null;
		}

		FileSerializer file = new FileSerializer();
		if (!file.Open(path, FileMode.READ))
		{
			return null;
		}

		VPPBuildingSetLegacy legacy = null;
		file.Read(legacy);
		file.Close();
		if (!legacy || !legacy.GetBuildings())
		{
			return null;
		}

		string setName = legacy.GetName();
		bool setActive = legacy.GetActive();
		BuildingSet bSet = new BuildingSet(setName, setActive);
		array<ref SpawnedBuilding> buildings = bSet.GetBuildings();
		array<ref VPPSpawnedBuildingLegacy> oldBuildings = legacy.GetBuildings();
		foreach (VPPSpawnedBuildingLegacy old : oldBuildings)
		{
			if (!old)
			{
				continue;
			}

			string typeName = old.GetObjectName();
			if (typeName == "")
			{
				continue;
			}

			vector position = old.GetPosition();
			vector orientation = old.GetOrientation();
			bool active = old.GetActive();
			SpawnedBuilding building = new SpawnedBuilding(typeName, position, orientation, false);
			building.LoadState(active, 1.0);
			buildings.Insert(building);
		}

		return bSet;
	}

	// Writes .tmp, keeps the previous file as .bak, then swaps the new one in.
	static bool Write(BuildingSet bSet, string path)
	{
		if (!bSet)
		{
			return false;
		}

		VPPBuildingSetFile setFile = VPPBuildingSetFile.FromBuildingSet(bSet);
		string tmpPath = path + ".tmp";
		string backupPath = path + ".bak";
		string error = "";
		if (!JsonFileLoader<VPPBuildingSetFile>.SaveFile(tmpPath, setFile, error))
		{
			GetSimpleLogger().Log("[BuildingSetManager] FAILED to write " + tmpPath + ": " + error);
			return false;
		}

		if (FileExist(path))
		{
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			CopyFile(path, backupPath);
			DeleteFile(path);
		}

		if (!CopyFile(tmpPath, path))
		{
			GetSimpleLogger().Log("[BuildingSetManager] FAILED to replace " + path + "; the previous file is " + backupPath);
			if (FileExist(backupPath) && !FileExist(path))
			{
				CopyFile(backupPath, path);
			}

			return false;
		}

		DeleteFile(tmpPath);
		return true;
	}

	// Moves a converted (or superseded) .vpp out of the set folder so it is never read again. The file stays where
	// it is when the copy fails.
	static bool ArchiveLegacy(string legacyPath)
	{
		if (!FileExist(legacyPath))
		{
			return true;
		}

		MakeDirectory("$profile:VPPAdminTools/Backups");
		MakeDirectory(LEGACY_BACKUP_DIR);
		string fileName = legacyPath;
		fileName.Replace("\\", "/");
		int slash = fileName.LastIndexOf("/");
		if (slash >= 0)
		{
			fileName = fileName.Substring(slash + 1, fileName.Length() - slash - 1);
		}

		string target = LEGACY_BACKUP_DIR + fileName;
		int copyNumber = 1;
		while (FileExist(target) && copyNumber < 100)
		{
			target = LEGACY_BACKUP_DIR + fileName + "." + copyNumber.ToString();
			copyNumber++;
		}

		if (!CopyFile(legacyPath, target))
		{
			GetSimpleLogger().Log("[BuildingSetManager] Could not back up " + legacyPath + " to " + target + "; it is left in place");
			return false;
		}

		DeleteFile(legacyPath);
		GetSimpleLogger().Log("[BuildingSetManager] Old set file moved to " + target);
		return true;
	}
};
