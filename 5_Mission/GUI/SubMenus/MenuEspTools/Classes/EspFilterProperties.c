class EspFilterProperties
{
	string filterType;
	int    color;
	int    comboIndex;
	
	void EspFilterProperties(string type, int c)
	{
		filterType = type;
		color = c;
		if (color <= 0)
		{
			color = ARGB(255,255,255,255);
		}
	}
};