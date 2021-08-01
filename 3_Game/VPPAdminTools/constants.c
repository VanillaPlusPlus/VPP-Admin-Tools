const int VPP_ADMIN_HUD 		  = 3215465487;
const int DRAG_TEST = 45454545454;


static bool VPPAT_IsModLoadded(string name)
{
	name.ToLower();
	int mod_count = GetGame().ConfigGetChildrenCount("CfgMods");
	for( int i = 2; i < mod_count; i++ )
	{
		string mod_name;
		GetGame().ConfigGetChildName("CfgMods", i, mod_name);
		mod_name.ToLower();
		if (mod_name == name)
			return true;
	}
	return false;
};