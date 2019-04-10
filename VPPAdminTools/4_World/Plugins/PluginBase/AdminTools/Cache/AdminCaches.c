class AdminCaches extends VPPConfig
{
	private ref array<ref AdminCache> adminCaches;
    static const string SETTINGS_FILE = "$profile:\\AdminsCache.json";

	void AdminCaches()
	{
		adminCaches = new ref array<ref AdminCache>;
	}

	void ~AdminCaches()
	{

	}

	override void save()
	{
		JsonFileLoader<ref AdminCaches>.JsonSaveFile(SETTINGS_FILE, this);
		load();
	}

	override void load()
	{
		adminCaches = new ref array<ref AdminCache>;
		if (FileExist(SETTINGS_FILE)) {
			JsonFileLoader<ref AdminCaches>.JsonLoadFile(SETTINGS_FILE, this);
			Print("Loading AdminCache.");
		}else{
			Print("AdminCache not found. Creating...");

			ref array<ref VPPPermission> permissions = new array<ref VPPPermission>;
			ref array<string> PermTypes = {
				"PlayerManager",
				"ItemManager",
				"ServerManager",
				"ObjectEditor",
				"WeatherManager",
			};

			foreach(string type : PermTypes){
				permissions.Insert(new VPPPermission(type, 1));	
			}

			addCache("76561198420222029", "DaOne", permissions);
			addCache("76561198040739393", "GravityWolf", permissions);
		}
	}

	ref array<string> getPermissions(string GUID)
	{
		ref array<string> Perms = new array<string>;
		foreach(AdminCache cache : adminCaches)
		{
			if (cache.getGUID() == GUID)
			{
				return cache.getAllPermsByGUID(GUID);
			}
		}
		return NULL;
	}

	void addCache(string guid, string savedname, ref array<ref VPPPermission> Perms){
		adminCaches.Insert(new ref AdminCache(guid, savedname, Perms));
		save();
	}

	void removeCacheByName(string name){
		foreach(ref AdminCache cache : adminCaches){
			if(cache.getName() == name){
				adminCaches.RemoveItem(cache);
				save();
			}
		}
	}

	void removeCacheByGUID(string guid){
		foreach(ref AdminCache cache : adminCaches){
			if(cache.getGUID() == guid){
				adminCaches.RemoveItem(cache);
				save();
			}
		}
	}

	bool containsCacheByName(string name){
		foreach(ref AdminCache cache : adminCaches){
			if(cache.getName() == name){
				return true;
			}
		}
		return false;
	}

	bool containsCacheByGUID(string guid){
		foreach(ref AdminCache cache : adminCaches){
			if(cache.getGUID() == guid){
				return true;
			}
		}
		return false;
	}

	void emptyCache(){
		foreach(ref AdminCache cache : adminCaches){
			adminCaches.RemoveItem(cache);
			save();
		}
	}
}