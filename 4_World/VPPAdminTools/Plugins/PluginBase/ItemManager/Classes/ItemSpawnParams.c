class ItemSpawnParams
{
	string presetName;
	vector position;
	int quantity, condition, placementType;
	ref array<string> targets;
	
	void ItemSpawnParams(string presetname, vector pos,int quant, int cond, int placType, array<string> trgIDs)
	{
		presetName    = presetname;
		position      = pos;
		quantity      = quant;
		condition     = cond;
		placementType = placType;
		targets       = trgIDs;
	}
}