enum VPPWebHookServerStatsTime
{
	ONE_MINUTE 		= 60,
	FIVE_MINUTES    = 300,
	TEN_MINUTES     = 600,
	FIFTEEN_MINUTES = 900,
};

class WebHook
{
	private string 			  m_Name;
	private string 			  m_URL;
	bool   					  m_deathKillLogs; 
	bool   					  m_adminActivityLog;
	bool   					  m_admHitLog;
	bool   					  m_joinLeaveLog;
	bool   					  m_serverStatusLog;
	VPPWebHookServerStatsTime m_serverStatsInterval;

	void WebHook(string name, string url)
	{
		m_Name = name;
		m_URL  = url;
	}

	void SetURL(string url)
	{
		m_URL = url;
	}

	void SetServerStatsInterval(VPPWebHookServerStatsTime interval)
	{
		m_serverStatsInterval = interval;
	}

	void SetName(string name)
	{
		m_Name = name;
	}

	string GetName()
	{
		return m_Name;
	}

	string GetURL()
	{
		return m_URL;
	}

	bool SendDeathKillLogs()
	{
		return m_deathKillLogs;
	}

	bool SendAdminActivityLogs()
	{
		return m_adminActivityLog;
	}

	bool SendHitLogs()
	{
		return m_admHitLog;
	}

	bool SendJoinLeaveLogs()
	{
		return m_joinLeaveLog;
	}

	bool ReportServerStatus()
	{
		return m_serverStatusLog;
	}

	VPPWebHookServerStatsTime GetServerStatsInterval()
	{
		return m_serverStatsInterval;
	}
};