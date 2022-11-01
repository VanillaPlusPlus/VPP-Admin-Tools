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
	bool 					  m_simplifiedMessages;

	
	[NonSerialized()]
	private ref Timer 		  m_MessageTimer;

	void WebHook(string name, string url)
	{
		m_Name = name;
		m_URL  = url;
	}

	/*
	* Called on all WebHook types, only interval based ones will have the timer started.
	*/
	void InitTimer()
	{
		if (!GetGame().IsDedicatedServer() || !GetGame().IsServer())
			return;
		if (!ReportServerStatus())
			return;

		m_MessageTimer = new Timer();
		m_MessageTimer.Run(GetServerStatsInterval(), this, "PostServerStatus", NULL, true);
	}

	void ~WebHook()
	{
		WebHooksManager.m_Invoker.Remove(PostMessage);
		if (m_MessageTimer)
		{
			m_MessageTimer.Stop();
			m_MessageTimer = NULL;
		}
	}

	void PostServerStatus()
	{
		PostMessage(ServerStatusMessage, new ServerStatusMessage());
	}

	/*
	* Server side only
	*/
	void PostMessage(typename messageType, WebHookMessageBase dataClass)
	{
		if (!GetGame().IsDedicatedServer() || !GetGame().IsServer())
			return;

		switch(messageType)
		{
			case AdminActivityMessage:
			if (SendAdminActivityLogs())
			{
				AdminActivityMessage aam;
				Class.CastTo(aam, dataClass);
				GetWebHooksManager().GetConnectionManager().Post(GetURL(), aam.BuildMessage(SimplifyMessages()));
			}
			break;

			case KillDeathMessage:
			if (SendDeathKillLogs())
			{
				KillDeathMessage kdm;
				Class.CastTo(kdm, dataClass);
				GetWebHooksManager().GetConnectionManager().Post(GetURL(), kdm.BuildMessage(SimplifyMessages()));
			}
			break;

			case JoinLeaveMessage:
			if (SendJoinLeaveLogs())
			{
				JoinLeaveMessage jlm;
				Class.CastTo(jlm, dataClass);
				GetWebHooksManager().GetConnectionManager().Post(GetURL(), jlm.BuildMessage(SimplifyMessages()));
			}
			break;

			case HitDamageMessage:
			if (SendHitLogs())
			{
				HitDamageMessage hdm;
				Class.CastTo(hdm, dataClass);
				GetWebHooksManager().GetConnectionManager().Post(GetURL(), hdm.BuildMessage(SimplifyMessages()));
			}
			break;

			case ServerStatusMessage:
			if (ReportServerStatus())
			{
				ServerStatusMessage stm;
				Class.CastTo(stm, dataClass);
				GetWebHooksManager().GetConnectionManager().Post(GetURL(), stm.BuildMessage(SimplifyMessages()));
			}
			break;
		}
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

	bool SimplifyMessages()
	{
		return m_simplifiedMessages;
	}
};