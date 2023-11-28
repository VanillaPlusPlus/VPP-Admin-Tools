class ItemSpawnParams
{
	string presetName;
	vector position;
	int condition, placementType;
	float quantity;
	ref array<string> targets;
	bool useCEDef = false; //Indicator to setup item according to CE/XML's setup (attachments, loot)
	
	void ItemSpawnParams(string presetname, vector pos, float quant, int cond, int placType, array<string> trgIDs, bool _useCEDef = false)
	{
		presetName    = presetname;
		position      = pos;
		quantity      = quant;
		condition     = cond;
		placementType = placType;
		targets       = trgIDs;
		useCEDef 	  = _useCEDef;
	}
}