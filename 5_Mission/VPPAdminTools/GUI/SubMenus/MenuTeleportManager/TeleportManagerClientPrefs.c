/*
	One recorded recent teleport (name + serialized world position).
	Coords are stored as a string so the JSON store stays simple/portable.
*/
class TeleportRecentEntry
{
	string Name;
	string Coords;

	void TeleportRecentEntry(string name = "", string coords = "0 0 0")
	{
		Name   = name;
		Coords = coords;
	}

	vector GetLocation()
	{
		return g_Game.StringToVector(Coords);
	}
}

/*
	Client-side Teleport Manager preferences (recent teleports history).
	Stored locally per player profile, never sent to the server.
	Modeled on ItemManagerClientPrefs.
*/
class TeleportManagerClientPrefs
{
	private const static string DIRPATH     = "$profile:VPPAdminTools";
	private const static string JSONPATH    = "$profile:VPPAdminTools/TeleportManagerClientPrefs.json";
	private const static int    MAX_RECENTS = 15;

	ref array<ref TeleportRecentEntry> Recents = new array<ref TeleportRecentEntry>;

	static ref TeleportManagerClientPrefs Load()
	{
		TeleportManagerClientPrefs prefs = new TeleportManagerClientPrefs();
		if (FileExist(JSONPATH))
			JsonFileLoader<TeleportManagerClientPrefs>.JsonLoadFile(JSONPATH, prefs);

		if (prefs.Recents == null)
			prefs.Recents = new array<ref TeleportRecentEntry>;
		return prefs;
	}

	void Save()
	{
		if (!FileExist(DIRPATH))
			MakeDirectory(DIRPATH);

		JsonFileLoader<TeleportManagerClientPrefs>.JsonSaveFile(JSONPATH, this);
	}

	array<ref TeleportRecentEntry> GetRecents()
	{
		return Recents;
	}

	//newest first, deduped by name, capped at MAX_RECENTS
	void RecordRecent(string name, vector position)
	{
		if (name == "")
			name = "Custom Point";

		for (int i = Recents.Count() - 1; i >= 0; --i)
		{
			if (Recents[i] != null && Recents[i].Name == name)
				Recents.Remove(i);
		}

		Recents.InsertAt(new TeleportRecentEntry(name, g_Game.VectorToString(position)), 0);

		while (Recents.Count() > MAX_RECENTS)
			Recents.Remove(Recents.Count() - 1);

		Save();
	}

	void RemoveRecentAt(int index)
	{
		if (index >= 0 && index < Recents.Count())
		{
			Recents.Remove(index);
			Save();
		}
	}

	void ClearRecents()
	{
		Recents.Clear();
		Save();
	}
}
