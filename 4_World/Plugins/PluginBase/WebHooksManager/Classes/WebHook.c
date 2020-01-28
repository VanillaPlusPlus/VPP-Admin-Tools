class WebHook
{
	private string m_Name;
	private string m_URL;
	bool   m_deathKillLogs; 
	bool   m_adminActivityLog;
	bool   m_admHitLog;
	bool   m_joinLeaveLog;

	void WebHook(string name, string url)
	{
		m_Name = name;
		m_URL  = url;
	}

	void SetURL(string url)
	{
		m_URL = url;
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
};