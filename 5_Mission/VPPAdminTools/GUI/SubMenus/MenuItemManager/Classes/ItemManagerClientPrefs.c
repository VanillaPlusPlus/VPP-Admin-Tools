/*
	Client-side Item Manager preferences (favorites + recent spawns).
	Stored locally per player profile, never sent to the server.
*/
class ItemManagerClientPrefs
{
	private const static string DIRPATH  = "$profile:VPPAdminTools";
	private const static string JSONPATH = "$profile:VPPAdminTools/ItemManagerClientPrefs.json";
	private const static int    MAX_RECENTS = 20;

	ref array<string> Favorites = new array<string>;
	ref array<string> Recents   = new array<string>;

	static ref ItemManagerClientPrefs Load()
	{
		ItemManagerClientPrefs prefs = new ItemManagerClientPrefs();
		if (FileExist(JSONPATH))
			JsonFileLoader<ItemManagerClientPrefs>.JsonLoadFile(JSONPATH, prefs);

		if (prefs.Favorites == null) prefs.Favorites = new array<string>;
		if (prefs.Recents == null)   prefs.Recents   = new array<string>;
		return prefs;
	}

	void Save()
	{
		if (!FileExist(DIRPATH))
			MakeDirectory(DIRPATH);

		JsonFileLoader<ItemManagerClientPrefs>.JsonSaveFile(JSONPATH, this);
	}

	bool IsFavorite(string typeName)
	{
		return Favorites.Find(typeName) > -1;
	}

	//returns the new favorite state
	bool ToggleFavorite(string typeName)
	{
		int idx = Favorites.Find(typeName);
		if (idx > -1)
		{
			Favorites.Remove(idx);
			Save();
			return false;
		}
		Favorites.Insert(typeName);
		Save();
		return true;
	}

	//newest first, deduped, capped at MAX_RECENTS
	void RecordRecent(string typeName)
	{
		int idx = Recents.Find(typeName);
		if (idx > -1)
			Recents.Remove(idx);

		Recents.InsertAt(typeName, 0);

		while (Recents.Count() > MAX_RECENTS)
			Recents.Remove(Recents.Count() - 1);

		Save();
	}
}
