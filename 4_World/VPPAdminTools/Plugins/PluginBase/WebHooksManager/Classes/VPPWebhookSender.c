// An event to send: its id and variables (VPPWebhookDefs lists the built-in ones). Other mods can emit their own ids
// with any variables through GetWebHooksManager().Emit(); templates for them are not name-checked.
class VPPWebhookEvent : Managed
{
	string Id;
	ref map<string, string> Vars;

	void VPPWebhookEvent(string eventId)
	{
		Id = eventId;
		Vars = new map<string, string>();
	}

	void Set(string name, string value)
	{
		Vars.Set(name, value);
	}

	string Get(string name)
	{
		string value = "";
		if (Vars.Contains(name))
		{
			value = Vars.Get(name);
		}

		return value;
	}
};

// Delivery numbers of one webhook (since the server started).
class VPPWebhookStats : Managed
{
	int Queued;
	int Delivered;
	// accepted without an answer (a 2xx without body is reported by the engine like a timeout)
	int NoReply;
	int Failed;
	int Dropped;
	int ConsecutiveFailures;
	string LastError;
	string LastErrorTime;
	string LastSuccessTime;
};

// A request in flight or waiting in the queue.
class VPPWebhookJob : Managed
{
	int Id;
	string EventId;
	string Body;
	int Attempts;
	// GetGame().GetTime() before which it is not sent (retry backoff)
	int NotBefore;
	// a test send: who asked (plain id) and their request id
	string TestBy;
	int TestReqId;
};

class VPPWebhookRequestCallback : RestCallback
{
	protected VPPWebhookSender m_Owner;
	protected int m_JobId;

	void VPPWebhookRequestCallback(VPPWebhookSender owner, int jobId)
	{
		m_Owner = owner;
		m_JobId = jobId;
	}

	override void OnSuccess(string data, int dataSize)
	{
		if (m_Owner)
		{
			m_Owner.OnRequestDone(m_JobId, VPPWebhookSender.RESULT_SUCCESS);
		}
	}

	override void OnError(int errorCode)
	{
		if (m_Owner)
		{
			m_Owner.OnRequestDone(m_JobId, errorCode);
		}
	}

	override void OnTimeout()
	{
		if (m_Owner)
		{
			m_Owner.OnRequestDone(m_JobId, VPPWebhookSender.RESULT_NO_REPLY);
		}
	}
};

/*
	Runtime of one webhook (server): its compiled templates, the send queue (rate limited per minute, retried with
	backoff) and the delivery stats. Request callbacks stay referenced until the engine calls them: the engine keeps a
	raw pointer, so they are never freed early.
*/
class VPPWebhookSender : Managed
{
	// Result codes as the engine really reports them (DayZServer 1.30 sub_1408B07C0 / sub_1408C9260). The script
	// ERestResultState names are one step off (its EREST_ERROR is 5, the engine's 4xx code), so they are not used.
	const static int RESULT_SUCCESS = 3;
	// HTTP 4xx (bad body, bad / deleted webhook URL, rate limited)
	const static int RESULT_HTTP_4XX = 5;
	// HTTP 5xx
	const static int RESULT_HTTP_5XX = 6;
	// any other curl failure: connect, DNS, TLS, or a 3xx redirect (redirects are not followed)
	const static int RESULT_NETWORK = 7;
	// curl timeout, or a 2xx without a body (Discord without ?wait=true answers 204)
	const static int RESULT_NO_REPLY = 8;

	const static int MAX_QUEUE = 100;
	const static int MAX_IN_FLIGHT = 50;
	const static int MAX_ATTEMPTS = 4;

	protected WebHook m_Hook;
	protected ref map<string, ref VPPWebhookTemplate> m_Templates;
	protected ref array<ref VPPWebhookJob> m_Queue;
	protected ref map<int, ref VPPWebhookJob> m_InFlight;
	protected ref map<int, ref VPPWebhookRequestCallback> m_Callbacks;
	protected ref array<int> m_SentTimes;
	// callbacks the engine already answered, released a while later (never inside their own OnSuccess / OnError)
	protected ref array<int> m_DoneIds;
	protected ref array<int> m_DoneTimes;
	protected int m_NextJobId;
	ref VPPWebhookStats Stats;

	void VPPWebhookSender(WebHook hook)
	{
		m_Hook = hook;
		m_Templates = new map<string, ref VPPWebhookTemplate>();
		m_Queue = new array<ref VPPWebhookJob>();
		m_InFlight = new map<int, ref VPPWebhookJob>();
		m_Callbacks = new map<int, ref VPPWebhookRequestCallback>();
		m_SentTimes = new array<int>();
		m_DoneIds = new array<int>();
		m_DoneTimes = new array<int>();
		Stats = new VPPWebhookStats();
	}

	WebHook GetHook()
	{
		return m_Hook;
	}

	void SetTemplate(string eventId, VPPWebhookTemplate tpl)
	{
		m_Templates.Set(eventId, tpl);
	}

	VPPWebhookTemplate GetTemplate(string eventId)
	{
		return m_Templates.Get(eventId);
	}

	bool IsJson()
	{
		return m_Hook.m_ContentType.Contains("json");
	}

	// Render with the webhook's variables added (hook.*) and its redaction applied; "" when there is no usable template.
	string Render(VPPWebhookEvent webhookEvent)
	{
		VPPWebhookTemplate tpl = m_Templates.Get(webhookEvent.Id);
		if (!tpl || tpl.HasErrors())
		{
			return "";
		}

		map<string, string> vars = new map<string, string>();
		vars.Copy(webhookEvent.Vars);
		for (int i = 0; i < m_Hook.m_Vars.Count(); i++)
		{
			string hookKey = m_Hook.m_Vars.GetKey(i);
			string hookValue = m_Hook.m_Vars.GetElement(i);
			vars.Set("hook." + hookKey, hookValue);
		}

		Redact(vars);
		bool jsonBody = IsJson();
		return tpl.Render(vars, jsonBody);
	}

	protected void Redact(map<string, string> vars)
	{
		if (m_Hook.m_HideServerAddress)
		{
			vars.Set("server.ip", "hidden");
			vars.Set("server.port", "hidden");
		}

		if (!m_Hook.m_HideIds)
		{
			return;
		}

		for (int i = 0; i < vars.Count(); i++)
		{
			string varName = vars.GetKey(i);
			if (varName.Length() > 3 && varName.Substring(varName.Length() - 3, 3) == ".id")
			{
				vars.Set(varName, "hidden");
			}
		}
	}

	// ---------------------------------------------------------------- queue

	void Enqueue(string eventId, string body, string testBy, int testReqId)
	{
		if (body == "")
		{
			return;
		}

		if (m_Queue.Count() >= MAX_QUEUE)
		{
			m_Queue.RemoveOrdered(0);
			Stats.Dropped++;
		}

		VPPWebhookJob job = new VPPWebhookJob();
		m_NextJobId++;
		job.Id = m_NextJobId;
		job.EventId = eventId;
		job.Body = body;
		job.TestBy = testBy;
		job.TestReqId = testReqId;
		m_Queue.Insert(job);
		Stats.Queued++;
	}

	// Called every second by the manager: sends what the rate limit and the in-flight cap allow.
	void Pump()
	{
		int now = GetGame().GetTime();
		ReleaseDoneCallbacks(now);
		if (m_Queue.Count() == 0 || !m_Hook)
		{
			return;
		}

		for (int i = m_SentTimes.Count() - 1; i >= 0; i--)
		{
			if (now - m_SentTimes[i] > 60000)
			{
				m_SentTimes.Remove(i);
			}
		}

		int budget = m_Hook.GetRateLimit() - m_SentTimes.Count();
		int index = 0;
		while (budget > 0 && index < m_Queue.Count() && m_InFlight.Count() < MAX_IN_FLIGHT)
		{
			VPPWebhookJob job = m_Queue[index];
			if (job.NotBefore > now)
			{
				index++;
				continue;
			}

			m_Queue.RemoveOrdered(index);
			Send(job);
			m_SentTimes.Insert(now);
			budget--;
		}
	}

	protected void ReleaseDoneCallbacks(int now)
	{
		for (int i = m_DoneIds.Count() - 1; i >= 0; i--)
		{
			if (now - m_DoneTimes[i] < 30000)
			{
				continue;
			}

			int doneId = m_DoneIds[i];
			m_Callbacks.Remove(doneId);
			m_DoneIds.RemoveOrdered(i);
			m_DoneTimes.RemoveOrdered(i);
		}
	}

	// Requests still waiting for the engine (the manager keeps a sender alive until this is 0).
	int PendingRequests()
	{
		return m_InFlight.Count() + m_DoneIds.Count();
	}

	protected void Send(VPPWebhookJob job)
	{
		string url = m_Hook.GetSendUrl();
		if (url == "" || !GetRestApi())
		{
			Finish(job, false, "no URL or no RestApi");
			return;
		}

		job.Attempts++;
		VPPWebhookRequestCallback callback = new VPPWebhookRequestCallback(this, job.Id);
		m_Callbacks.Set(job.Id, callback);
		m_InFlight.Set(job.Id, job);
		RestContext context = GetRestApi().GetRestContext(url);
		context.SetHeader(m_Hook.m_ContentType);
		context.POST(callback, "", job.Body);
	}

	void OnRequestDone(int jobId, int state)
	{
		VPPWebhookJob job = m_InFlight.Get(jobId);
		m_InFlight.Remove(jobId);
		m_DoneIds.Insert(jobId);
		int doneAt = GetGame().GetTime();
		m_DoneTimes.Insert(doneAt);
		// a retired sender (its webhook was deleted / replaced) only waits for its callbacks
		if (!job || !m_Hook)
		{
			return;
		}

		if (state == RESULT_SUCCESS)
		{
			Finish(job, true, "");
			return;
		}

		// a bodyless 2xx and a real timeout look the same; with Discord's ?wait=true a delivered message has a body
		if (state == RESULT_NO_REPLY && !m_Hook.ExpectsReply())
		{
			Stats.NoReply++;
			Stats.ConsecutiveFailures = 0;
			string noReplyTime = TimeText();
			Stats.LastSuccessTime = noReplyTime;
			ReportTest(job, true, "NO_REPLY");
			return;
		}

		string reason = StateName(state);
		// 4xx is usually a bad payload or URL: one retry (covers a 429 rate limit); server / network errors retry more
		int maxAttempts = MAX_ATTEMPTS;
		if (state == RESULT_HTTP_4XX)
		{
			maxAttempts = 2;
		}

		if (job.Attempts < maxAttempts && job.TestBy == "")
		{
			int delay = 10000 * job.Attempts * job.Attempts;
			job.NotBefore = GetGame().GetTime() + delay;
			m_Queue.InsertAt(job, 0);
			return;
		}

		Finish(job, false, reason);
	}

	protected void Finish(VPPWebhookJob job, bool ok, string reason)
	{
		string now = TimeText();
		if (ok)
		{
			Stats.Delivered++;
			Stats.ConsecutiveFailures = 0;
			Stats.LastSuccessTime = now;
			ReportTest(job, true, "DELIVERED");
			return;
		}

		Stats.Failed++;
		Stats.ConsecutiveFailures++;
		Stats.LastError = reason;
		Stats.LastErrorTime = now;
		string hookName = m_Hook.GetName();
		Print("[WebHooksManager] Webhook \"" + hookName + "\" failed to deliver " + job.EventId + ": " + reason);
		if (reason.IndexOf("HTTP_4XX") == 0)
		{
			DiagnoseRejected(job);
		}

		ReportTest(job, false, reason);
	}

	protected void ReportTest(VPPWebhookJob job, bool ok, string reason)
	{
		if (job.TestBy == "")
		{
			return;
		}

		WebHooksManager manager = GetWebHooksManager();
		if (manager)
		{
			manager.OnTestResult(job.TestBy, job.TestReqId, m_Hook.m_Id, job.EventId, ok, reason);
		}
	}

	// A 4xx: the full body goes to Rejected/<webhook id>_<event>.txt (the newest one per event), and the log says
	// whether the game's JSON parser accepts it (with line / column) and which Discord limits it breaks. Valid JSON
	// with no limit problems points at the URL or the service's rate limit instead.
	protected void DiagnoseRejected(VPPWebhookJob job)
	{
		string dir = "$profile:VPPAdminTools/ConfigurablePlugins/WebHooksManager/Rejected";
		string path = "";
		string verdictText = "";
		string lineText = "";
		string colText = "";
		string problem = "";
		VPPJsonCheckResult verdict = null;
		array<string> problems = new array<string>();
		if (!FileExist(dir))
		{
			MakeDirectory(dir);
		}

		path = dir + "/" + m_Hook.m_Id + "_" + job.EventId + ".txt";
		VPPXmlText.WriteAll(path, job.Body);
		Print("[WebHooksManager] Rejected body saved to " + path);
		if (!IsJson())
		{
			return;
		}

		verdict = VPPJsonCheck.Check(job.Body);
		if (!verdict.Valid)
		{
			lineText = verdict.Line.ToString();
			colText = verdict.Col.ToString();
			verdictText = "[WebHooksManager] The body is NOT valid JSON at line " + lineText + ", column " + colText + ": " + verdict.Message;
			Print(verdictText);
			return;
		}

		if (VPPWebhookPresets.IsDiscord(m_Hook.m_Preset))
		{
			VPPDiscordLint.Check(job.Body, problems);
		}

		if (problems.Count() == 0)
		{
			Print("[WebHooksManager] The body is valid JSON within Discord limits: check the URL, or the service is rate limiting");
			return;
		}

		foreach (string lintProblem : problems)
		{
			problem = "[WebHooksManager] Discord limit: " + lintProblem;
			Print(problem);
		}
	}

	static string StateName(int state)
	{
		if (state == RESULT_HTTP_4XX)
		{
			return "HTTP_4XX (bad body, wrong or deleted URL, or rate limited)";
		}

		if (state == RESULT_HTTP_5XX)
		{
			return "HTTP_5XX (the service had an error)";
		}

		if (state == RESULT_NETWORK)
		{
			return "NETWORK_ERROR (connect, DNS, TLS, or a redirecting URL)";
		}

		if (state == RESULT_NO_REPLY)
		{
			return "TIMEOUT (or an empty reply)";
		}

		return "ERROR_" + state.ToString();
	}

	static string TimeText()
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		string dateText = VPPWebhookClock.Pad(year, 4) + "-" + VPPWebhookClock.Pad(month, 2) + "-" + VPPWebhookClock.Pad(day, 2);
		return dateText + " " + VPPWebhookClock.Pad(hour, 2) + ":" + VPPWebhookClock.Pad(minute, 2) + ":" + VPPWebhookClock.Pad(second, 2) + " UTC";
	}
};

// UTC time texts for the common template variables.
class VPPWebhookClock
{
	static string Pad(int value, int width)
	{
		string text = value.ToString();
		while (text.Length() < width)
		{
			text = "0" + text;
		}

		return text;
	}

	// Seconds since 1970-01-01 UTC (days-from-civil, no CF dependency).
	static int UnixTime(int year, int month, int day, int hour, int minute, int second)
	{
		int y = year;
		if (month <= 2)
		{
			y = y - 1;
		}

		int era = y / 400;
		if (y < 0)
		{
			era = (y - 399) / 400;
		}

		int yoe = y - era * 400;
		int shiftedMonth = month + 9;
		if (month > 2)
		{
			shiftedMonth = month - 3;
		}

		int doy = (153 * shiftedMonth + 2) / 5 + day - 1;
		int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		int days = era * 146097 + doe - 719468;
		return days * 86400 + hour * 3600 + minute * 60 + second;
	}

	static void Fill(map<string, string> vars)
	{
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		GetYearMonthDayUTC(year, month, day);
		GetHourMinuteSecondUTC(hour, minute, second);
		string dateText = Pad(year, 4) + "-" + Pad(month, 2) + "-" + Pad(day, 2);
		string timeText = Pad(hour, 2) + ":" + Pad(minute, 2) + ":" + Pad(second, 2);
		int unix = UnixTime(year, month, day, hour, minute, second);
		vars.Set("time.date", dateText);
		vars.Set("time.time", timeText);
		vars.Set("time.iso", dateText + "T" + timeText + ".000Z");
		vars.Set("time.unix", unix.ToString());
	}
};
