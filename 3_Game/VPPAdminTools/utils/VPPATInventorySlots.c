/*
	used as container class for all game inventory objects types
*/
class VPPATInventorySlots
{
	static ref map<string, ref array<string>> SlotsItems = new map <string, ref array<string>>;

	//context-menu spawnable index: separate from SlotsItems (owned by the refuel/repair code), created on first use only
	protected static ref map<string, ref array<string>> s_VPPSpawnable;   //lowercase slot -> classnames
	protected static ref map<string, ref array<string>> s_VPPSorted;      //lowercase slot -> classnames sorted by display name
	protected static ref map<string, string> s_VPPDisplayNames;

	void VPPATInventorySlots()
	{
		VPPATInventorySlots.DumpInventorySlots();
	}

	static void DumpInventorySlots()
	{
		//Fetch all possible attachments / inventory items for literally everything
		for(int i = 0; i < GetGame().ConfigGetChildrenCount("CfgVehicles"); i++)
		{
			string className;
			if (!GetGame().ConfigGetChildName(CFG_VEHICLESPATH, i, className))
				continue;
						
			if (IsBaseClass(className, CFG_VEHICLESPATH))
				continue;
			
			int cfgType = GetGame().ConfigGetType(CFG_VEHICLESPATH + " " + className + " inventorySlot");
			if (cfgType == CT_ARRAY)
			{
				array<string> attachments = {};
				GetGame().ConfigGetTextArray(CFG_VEHICLESPATH + " " + className + " inventorySlot", attachments);
				foreach(string att : attachments)
				{
					if (att == string.Empty)
						break;
					
					att.ToLower();
					if (SlotsItems[att] == NULL){
						SlotsItems[att] = new array<string>;
					}
					
					if (SlotsItems[att].Find(className) == -1)
						SlotsItems[att].Insert(className);
				}
			}
			else if (cfgType == CT_STRING)
			{
				string invSlot = string.Empty;
				if (GetGame().ConfigGetText("CfgVehicles " + className + " inventorySlot", invSlot) && invSlot != string.Empty)
				{
					invSlot.ToLower();
					if (SlotsItems[invSlot] == NULL){
						SlotsItems[invSlot] = new array<string>;
					}
					
					if (SlotsItems[invSlot].Find(className) == -1)
						SlotsItems[invSlot].Insert(className);
				}
			}
		}
	}

	static bool IsBaseClass(string type, string cfg)
	{
		int scope = GetGame().ConfigGetInt(cfg + " " + type + " scope");
		if (scope == 0)
			return true;
		
		if (type.Length() > "_Base".Length() && type.Substring(type.Length() - "_Base".Length(), "_Base".Length()) == "_Base")
			return true;

		return false;
	}

	//scope 2, no spaces, not *_base, not *_ruined (case-insensitive)
	static bool IsSpawnableClass(string root, string className)
	{
		if (root == "" || className == "")
			return false;

		if (className.Contains(" "))
			return false;

		string scopePath = root + " " + className + " scope";
		if (GetGame().ConfigGetInt(scopePath) != 2)
			return false;

		string lowName = className;
		lowName.ToLower();
		if (lowName.Contains("_ruined"))
			return false;

		int len = lowName.Length();
		if (len >= 5 && lowName.Substring(len - 5, 5) == "_base")
			return false;

		return true;
	}

	//"CfgVehicles" | "CfgWeapons" | "CfgMagazines" | ""
	static string FindConfigRoot(string className)
	{
		if (className == "" || className.Contains(" "))
			return "";

		string vehiclesPath = "CfgVehicles " + className;
		if (GetGame().ConfigIsExisting(vehiclesPath))
			return "CfgVehicles";

		string weaponsPath = "CfgWeapons " + className;
		if (GetGame().ConfigIsExisting(weaponsPath))
			return "CfgWeapons";

		string magazinesPath = "CfgMagazines " + className;
		if (GetGame().ConfigIsExisting(magazinesPath))
			return "CfgMagazines";

		return "";
	}

	protected static void BuildSpawnableIndex()
	{
		if (s_VPPSpawnable)
			return;

		s_VPPSpawnable = new map<string, ref array<string>>;
		IndexConfigRoot("CfgVehicles");
		IndexConfigRoot("CfgWeapons");
		IndexConfigRoot("CfgMagazines");
	}

	protected static void IndexConfigRoot(string root)
	{
		int childCount = GetGame().ConfigGetChildrenCount(root);
		for (int i = 0; i < childCount; i++)
		{
			string className = "";
			if (!GetGame().ConfigGetChildName(root, i, className))
				continue;

			string slotPath = root + " " + className;
			slotPath += " inventorySlot";
			int cfgType = GetGame().ConfigGetType(slotPath);

			//most classes have no inventorySlot: reject on the type BEFORE any scope or name work
			if (cfgType != CT_ARRAY && cfgType != CT_STRING)
				continue;

			if (!IsSpawnableClass(root, className))
				continue;

			if (cfgType == CT_ARRAY)
			{
				array<string> slotNames = new array<string>;
				GetGame().ConfigGetTextArray(slotPath, slotNames);
				foreach (string oneSlot : slotNames)
				{
					AddSpawnable(oneSlot, className);
				}
			}
			else
			{
				string arrSlot = "";
				GetGame().ConfigGetText(slotPath, arrSlot);
				AddSpawnable(arrSlot, className);
			}
		}
	}

	protected static void AddSpawnable(string slotName, string className)
	{
		if (slotName == "")
			return;

		string key = slotName;
		key.ToLower();

		array<string> bucket;
		if (!s_VPPSpawnable.Find(key, bucket))
		{
			bucket = new array<string>;
			s_VPPSpawnable.Set(key, bucket);
		}

		if (bucket.Find(className) == -1)
			bucket.Insert(className);
	}

	//localized, cached, falls back to the classname
	static string GetClassDisplayName(string className)
	{
		if (!s_VPPDisplayNames)
			s_VPPDisplayNames = new map<string, string>;

		string cached;
		if (s_VPPDisplayNames.Find(className, cached))
			return cached;

		string disp = "";
		string root = FindConfigRoot(className);
		if (root != "")
		{
			string dispPath = root + " " + className + " displayName";
			disp = GetGame().ConfigGetTextOut(dispPath);
			if (GetGame().FormatRawConfigStringKeys(disp))
				disp = Widget.TranslateString(disp);
		}

		if (disp == "")
			disp = className;

		s_VPPDisplayNames.Set(className, disp);
		return disp;
	}

	//returns a new array; null-safe
	static array<string> SortByDisplayName(array<string> classNames)
	{
		array<string> sorted = new array<string>;
		if (!classNames)
			return sorted;

		map<string, string> byKey = new map<string, string>;
		array<string> keys = new array<string>;
		foreach (string cls : classNames)
		{
			string sortKey = GetClassDisplayName(cls);
			sortKey.ToLower();
			sortKey = sortKey + "|" + cls;
			if (byKey.Contains(sortKey))
				continue;

			byKey.Set(sortKey, cls);
			keys.Insert(sortKey);
		}

		keys.Sort();
		foreach (string k : keys)
		{
			sorted.Insert(byKey.Get(k));
		}
		return sorted;
	}

	//client; never null; callers must not modify the result. The first call runs a one-time config scan.
	static array<string> GetSpawnableForSlot(string slotName)
	{
		string key = slotName;
		key.ToLower();

		if (!s_VPPSorted)
			s_VPPSorted = new map<string, ref array<string>>;

		array<string> cachedSorted;
		if (s_VPPSorted.Find(key, cachedSorted) && cachedSorted)
			return cachedSorted;

		BuildSpawnableIndex();
		array<string> bucket = null;
		s_VPPSpawnable.Find(key, bucket);
		array<string> sortedBucket = SortByDisplayName(bucket);
		s_VPPSorted.Set(key, sortedBucket);
		return sortedBucket;
	}

	//server validation straight from config (no index)
	static bool CanSpawnInSlot(string className, string slotName)
	{
		if (slotName == "" || slotName.Contains(" "))
			return false;

		string root = FindConfigRoot(className);
		if (!IsSpawnableClass(root, className))
			return false;

		string wanted = slotName;
		wanted.ToLower();

		string slotPath = root + " " + className;
		slotPath += " inventorySlot";
		int cfgType = GetGame().ConfigGetType(slotPath);
		if (cfgType == CT_ARRAY)
		{
			array<string> slotNames = new array<string>;
			GetGame().ConfigGetTextArray(slotPath, slotNames);
			foreach (string oneSlot : slotNames)
			{
				string lowSlot = oneSlot;
				lowSlot.ToLower();
				if (lowSlot == wanted)
					return true;
			}
			return false;
		}

		if (cfgType == CT_STRING)
		{
			string arrSlot = "";
			GetGame().ConfigGetText(slotPath, arrSlot);
			arrSlot.ToLower();
			return arrSlot == wanted;
		}

		return false;
	}
};