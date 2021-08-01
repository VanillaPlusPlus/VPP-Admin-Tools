class PlayerStatsData
{
	private ref map<string,string> m_Stats;
	
	void PlayerStatsData(map<string,string> stats)
	{
		m_Stats	  = stats;
	}
	
	string GetStat(string StatType)
	{
		foreach(string type, string value : m_Stats)
		{
			if (StatType == type)
			{
				return value;
			}
		}
		return "0";
	}
	
	map<string,string> GetMap()
	{
		return m_Stats;
	}
};