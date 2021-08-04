/*
	used as container class for all game inventory objects types
*/
class VPPATInventorySlots
{
	static ref map<string, ref array<string>> SlotsItems = new map <string, ref array<string>>;

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
};