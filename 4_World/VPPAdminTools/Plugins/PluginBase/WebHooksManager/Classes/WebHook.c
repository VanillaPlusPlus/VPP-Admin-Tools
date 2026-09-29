enum VPPWebHookServerStatsTime
{
	ONE_MINUTE 		= 60,
	FIVE_MINUTES    = 300,
	TEN_MINUTES     = 600,
	FIFTEEN_MINUTES = 900,
};

// Per event settings of a webhook: on/off and what gets through.
class VPPWebhookEventCfg : Managed
{
	bool Enabled;
	// kill / hit: only player vs player
	bool PvPOnly;
	// kill / hit: skip infected and animals
	bool SkipAI;
	// kill / hit: minimum distance in meters (0 = any)
	float MinDistance;
	// admin.action: only these modules (lower case, empty = all) / never these
	ref array<string> Modules;
	ref array<string> ExcludedModules;

	void VPPWebhookEventCfg()
	{
		Modules = new array<string>();
		ExcludedModules = new array<string>();
	}
};

/*
	One webhook. The legacy fields (the per-category switches and "simplified messages") are kept so old configs load
	and the current menu keeps working: on load a v0 config is migrated to events + a Discord preset with the same
	switches (Migrate). Templates live in files (Templates/<m_Id>/<event>.tpl), runtime state in VPPWebhookSender.
*/
class WebHook : Managed
{
	const static int VERSION = 2;

	private string 			  m_Name;
	private string 			  m_URL;
	bool   					  m_deathKillLogs;
	bool   					  m_adminActivityLog;
	bool   					  m_admHitLog;
	bool   					  m_joinLeaveLog;
	bool   					  m_serverStatusLog;
	VPPWebHookServerStatsTime m_serverStatsInterval;
	bool 					  m_simplifiedMessages;

	// v2
	int m_Version;
	// stable id (template folder name)
	string m_Id;
	bool m_Disabled;
	string m_Preset;
	string m_ContentType;
	// messages per minute (0 = the preset default)
	int m_RateLimit;
	bool m_HideIds;
	bool m_HideServerAddress;
	// "hook.<key>" template variables (Discord username / avatar, Telegram chat_id, role mentions, ...)
	ref map<string, string> m_Vars;
	ref map<string, ref VPPWebhookEventCfg> m_Events;
	// per event: hash of the preset template text the server wrote to disk. A file still matching it is untouched
	// and follows preset updates; a file an admin edited no longer matches and is never overwritten.
	ref map<string, int> m_PresetHashes;

	void WebHook(string name, string url)
	{
		m_Name = name;
		m_URL  = url;
		m_Vars = new map<string, string>();
		m_Events = new map<string, ref VPPWebhookEventCfg>();
		m_PresetHashes = new map<string, int>();
	}

	// ---------------------------------------------------------------- migration

	// v0 (switches only) -> v2: an id, a Discord preset and one event config per event, enabled from the old switches.
	bool Migrate(int serial)
	{
		if (!m_Vars)
		{
			m_Vars = new map<string, string>();
		}

		if (!m_Events)
		{
			m_Events = new map<string, ref VPPWebhookEventCfg>();
		}

		if (!m_PresetHashes)
		{
			m_PresetHashes = new map<string, int>();
		}

		bool changed = false;
		if (m_Id == "")
		{
			m_Id = NewId(serial);
			changed = true;
		}

		if (m_Version < VERSION)
		{
			if (m_Preset == "")
			{
				m_Preset = VPPWebhookPresets.DISCORD_EMBED;
				if (m_simplifiedMessages)
				{
					m_Preset = VPPWebhookPresets.DISCORD_SIMPLE;
				}
			}

			SyncEventsFromSwitches();
			m_Version = VERSION;
			changed = true;
		}

		if (!VPPWebhookPresets.IsPreset(m_Preset))
		{
			m_Preset = VPPWebhookPresets.DISCORD_EMBED;
			changed = true;
		}

		if (m_ContentType == "")
		{
			m_ContentType = VPPWebhookPresets.ContentTypeOf(m_Preset);
			changed = true;
		}

		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			if (!m_Events.Contains(def.Id))
			{
				VPPWebhookEventCfg added = new VPPWebhookEventCfg();
				added.Enabled = SwitchOf(def.LegacyGroup);
				m_Events.Set(def.Id, added);
				changed = true;
			}
		}

		if (m_serverStatsInterval <= 0)
		{
			m_serverStatsInterval = VPPWebHookServerStatsTime.FIVE_MINUTES;
			changed = true;
		}

		return changed;
	}

	static string NewId(int serial)
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		int stamp = ((day * 24 + hour) * 60 + minute) * 60 + second;
		int random = Math.RandomInt(1000, 9999);
		return "wh" + stamp.ToString() + serial.ToString() + random.ToString();
	}

	// The old per-category switches decide which events are on (old menu and old configs).
	void SyncEventsFromSwitches()
	{
		if (!m_Events)
		{
			m_Events = new map<string, ref VPPWebhookEventCfg>();
		}

		array<ref VPPWebhookEventDef> eventDefs = VPPWebhookDefs.All();
		foreach (VPPWebhookEventDef def : eventDefs)
		{
			VPPWebhookEventCfg cfg = m_Events.Get(def.Id);
			if (!cfg)
			{
				cfg = new VPPWebhookEventCfg();
				m_Events.Set(def.Id, cfg);
			}

			cfg.Enabled = SwitchOf(def.LegacyGroup);
		}
	}

	protected bool SwitchOf(string legacyGroup)
	{
		if (legacyGroup == VPPWebhookDefs.GROUP_DEATH)
		{
			return m_deathKillLogs;
		}

		if (legacyGroup == VPPWebhookDefs.GROUP_ADMIN)
		{
			return m_adminActivityLog;
		}

		if (legacyGroup == VPPWebhookDefs.GROUP_HIT)
		{
			return m_admHitLog;
		}

		if (legacyGroup == VPPWebhookDefs.GROUP_JOINLEAVE)
		{
			return m_joinLeaveLog;
		}

		if (legacyGroup == VPPWebhookDefs.GROUP_STATUS)
		{
			return m_serverStatusLog;
		}

		return false;
	}

	// ---------------------------------------------------------------- client copy

	// What an admin client gets: the URL masked unless showUrl (a webhook URL is a secret), no runtime state.
	WebHook CopyForClient(bool showUrl)
	{
		WebHook copy = new WebHook(m_Name, m_URL);
		if (!showUrl)
		{
			copy.SetURL(MaskUrl(m_URL));
		}

		copy.m_deathKillLogs = m_deathKillLogs;
		copy.m_adminActivityLog = m_adminActivityLog;
		copy.m_admHitLog = m_admHitLog;
		copy.m_joinLeaveLog = m_joinLeaveLog;
		copy.m_serverStatusLog = m_serverStatusLog;
		copy.m_serverStatsInterval = m_serverStatsInterval;
		copy.m_simplifiedMessages = m_simplifiedMessages;
		copy.m_Version = m_Version;
		copy.m_Id = m_Id;
		copy.m_Disabled = m_Disabled;
		copy.m_Preset = m_Preset;
		copy.m_ContentType = m_ContentType;
		copy.m_RateLimit = m_RateLimit;
		copy.m_HideIds = m_HideIds;
		copy.m_HideServerAddress = m_HideServerAddress;
		copy.m_Vars.Copy(m_Vars);
		for (int i = 0; i < m_Events.Count(); i++)
		{
			string eventId = m_Events.GetKey(i);
			VPPWebhookEventCfg source = m_Events.GetElement(i);
			VPPWebhookEventCfg cloned = new VPPWebhookEventCfg();
			cloned.Enabled = source.Enabled;
			cloned.PvPOnly = source.PvPOnly;
			cloned.SkipAI = source.SkipAI;
			cloned.MinDistance = source.MinDistance;
			cloned.Modules.Copy(source.Modules);
			cloned.ExcludedModules.Copy(source.ExcludedModules);
			copy.m_Events.Set(eventId, cloned);
		}

		return copy;
	}

	// "https://discord.com/api/webhooks/…Ab3F": scheme and host, then … and the last 4 characters.
	static string MaskUrl(string url)
	{
		if (url == "")
		{
			return "";
		}

		int schemeEnd = url.IndexOf("://");
		int hostEnd = -1;
		if (schemeEnd >= 0)
		{
			hostEnd = url.IndexOfFrom(schemeEnd + 3, "/");
		}

		string head = url;
		if (hostEnd > 0)
		{
			head = url.Substring(0, hostEnd + 1);
		}

		int total = url.Length();
		string tail = "";
		if (total > 4)
		{
			tail = url.Substring(total - 4, 4);
		}

		return head + MASK_MARK + tail;
	}

	const static string MASK_MARK = "…";

	static bool IsMasked(string url)
	{
		return url.Contains(MASK_MARK);
	}

	// ---------------------------------------------------------------- accessors

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

	bool IsEventEnabled(string eventId)
	{
		if (m_Disabled || !m_Events)
		{
			return false;
		}

		VPPWebhookEventCfg cfg = m_Events.Get(eventId);
		return cfg && cfg.Enabled;
	}

	VPPWebhookEventCfg GetEventCfg(string eventId)
	{
		if (!m_Events)
		{
			return null;
		}

		return m_Events.Get(eventId);
	}

	// Messages per minute this webhook may send (Discord allows about 30 per webhook).
	int GetRateLimit()
	{
		if (m_RateLimit > 0)
		{
			return m_RateLimit;
		}

		if (VPPWebhookPresets.IsDiscord(m_Preset))
		{
			return 25;
		}

		return 60;
	}

	// The URL requests go to: Discord gets ?wait=true so a delivered message answers with a body (the engine reports a
	// bodyless 204 as a timeout, memory dayz-restapi-json-internals).
	string GetSendUrl()
	{
		string url = m_URL;
		string lower = url;
		lower.ToLower();
		bool discord = lower.Contains("discord.com/api/webhooks") || lower.Contains("discordapp.com/api/webhooks");
		if (!discord || lower.Contains("wait="))
		{
			return url;
		}

		if (url.Contains("?"))
		{
			return url + "&wait=true";
		}

		return url + "?wait=true";
	}

	bool ExpectsReply()
	{
		string sendUrl = GetSendUrl();
		return sendUrl.Contains("wait=true");
	}
};
