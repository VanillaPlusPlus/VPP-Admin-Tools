class EspFilterProperties
{
	string filterType;
	int    color;
	int    comboIndex;
	bool slowSearchNeeded;
	
	void EspFilterProperties(string type, int c, bool slowSeachReq = false)
	{
		filterType = type;
		color = c;
		if (color <= 0)
		{
			color = ARGB(255,255,255,255);
		}
		slowSearchNeeded = slowSeachReq;
	}
};