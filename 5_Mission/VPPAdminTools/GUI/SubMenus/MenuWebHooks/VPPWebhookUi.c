/*
	Shared helpers of the webhooks menu: localized event / preset names, the "#" display mapping and text utilities.

	Text set on a widget loses the "#" of every "#word" (the engine reads it as a stringtable key). The XML Editor
	MESSAGES tab proved in game that a full-width "＃" survives, so every text shown in an editor or preview is mapped
	"#" -> "＃" and back when read (per line: the engine Replace is exact only up to 2560 bytes).
*/
class VPPWebhookUi
{
	const static string FULLWIDTH_HASH = "＃";
	const static int CLR_OK = 0xFF4CAF50;
	const static int CLR_FAIL = 0xFFC24545;
	const static int CLR_OFF = 0xFF5C6166;
	const static int CLR_IDLE = 0xFFCCD0D4;
	const static int CLR_NEW = 0xFFE8A33D;
	const static int CLR_WHITE = 0xFFFFFFFF;
	const static int CLR_MUTED = 0xFFA3A8AD;
	const static int CLR_ORANGE = 0xFFE8A33D;
	const static int CLR_SECONDARY = 0xFFCCD0D4;

	static string Tr(string key)
	{
		string translated = Widget.TranslateString(key);
		return translated;
	}

	// Stringtable suffix of an event id ("server.status" -> "STATUS").
	static string EventKey(string eventId)
	{
		if (eventId == VPPWebhookDefs.EV_ADMIN)
		{
			return "ADMIN";
		}

		if (eventId == VPPWebhookDefs.EV_KILL)
		{
			return "KILL";
		}

		if (eventId == VPPWebhookDefs.EV_HIT)
		{
			return "HIT";
		}

		if (eventId == VPPWebhookDefs.EV_JOIN)
		{
			return "JOIN";
		}

		if (eventId == VPPWebhookDefs.EV_LEAVE)
		{
			return "LEAVE";
		}

		if (eventId == VPPWebhookDefs.EV_LEAVE_EARLY)
		{
			return "LEAVE_EARLY";
		}

		if (eventId == VPPWebhookDefs.EV_LOGOUT_START)
		{
			return "LOGOUT_START";
		}

		if (eventId == VPPWebhookDefs.EV_LOGOUT_CANCEL)
		{
			return "LOGOUT_CANCEL";
		}

		if (eventId == VPPWebhookDefs.EV_STATUS)
		{
			return "STATUS";
		}

		if (eventId == VPPWebhookDefs.EV_BOOT)
		{
			return "BOOT";
		}

		return "";
	}

	static string EventLabel(string eventId)
	{
		string suffix = EventKey(eventId);
		if (suffix == "")
		{
			return eventId;
		}

		string label = Tr("#VSTR_WH_EVN_" + suffix);
		return label;
	}

	static string EventDesc(string eventId)
	{
		string suffix = EventKey(eventId);
		if (suffix == "")
		{
			return "";
		}

		string desc = Tr("#VSTR_WH_EVD_" + suffix);
		return desc;
	}

	static string PresetLabel(string presetId)
	{
		string upperId = presetId;
		upperId.ToUpper();
		string label = Tr("#VSTR_WH_PRESET_" + upperId);
		return label;
	}

	// One line for a widget: "#" -> "＃".
	static string ToDisplay(string line)
	{
		string shown = line;
		if (shown.IndexOf("#") >= 0)
		{
			shown.Replace("#", FULLWIDTH_HASH);
		}

		return shown;
	}

	// One line read from a widget: "＃" -> "#".
	static string FromDisplay(string line)
	{
		string raw = line;
		if (raw.IndexOf(FULLWIDTH_HASH) >= 0)
		{
			raw.Replace(FULLWIDTH_HASH, "#");
		}

		return raw;
	}

	// Splits on LF (a CR before it is dropped); an empty text gives one empty line.
	static void SplitLines(string text, array<string> outLines)
	{
		int total = text.Length();
		int pos = 0;
		int newline = 0;
		int lineEnd = 0;
		string line = "";
		string carriage = "";
		int carriageCode = 13;
		carriage = carriageCode.AsciiToString();
		outLines.Clear();
		while (pos <= total)
		{
			newline = text.IndexOfFrom(pos, "\n");
			if (newline < 0)
			{
				newline = total;
			}

			lineEnd = newline;
			line = "";
			if (lineEnd > pos)
			{
				line = text.Substring(pos, lineEnd - pos);
			}

			if (line.Length() > 0 && line.Substring(line.Length() - 1, 1) == carriage)
			{
				line = line.Substring(0, line.Length() - 1);
			}

			outLines.Insert(line);
			pos = newline + 1;
		}
	}

	// A whole text for a widget: every line mapped with ToDisplay (lines stay under the Replace limit).
	static string TextToDisplay(string text)
	{
		array<string> lines = new array<string>();
		string shown = "";
		string mapped = "";
		int total = 0;
		int i = 0;
		SplitLines(text, lines);
		total = lines.Count();
		for (i = 0; i < total; i++)
		{
			mapped = ToDisplay(lines[i]);
			if (i > 0)
			{
				shown = shown + "\n";
			}

			shown = shown + mapped;
		}

		return shown;
	}

	static string JoinList(array<string> items, string separator)
	{
		string joined = "";
		int total = 0;
		int i = 0;
		if (!items)
		{
			return joined;
		}

		total = items.Count();
		for (i = 0; i < total; i++)
		{
			if (i > 0)
			{
				joined = joined + separator;
			}

			joined = joined + items[i];
		}

		return joined;
	}

	// "ItemManager, playermanager" -> itemmanager, playermanager (trimmed, lower case, no duplicates).
	static void ParseModules(string text, array<string> outModules)
	{
		array<string> parts = new array<string>();
		string part = "";
		int total = 0;
		int i = 0;
		outModules.Clear();
		SplitOn(text, ",", parts);
		total = parts.Count();
		for (i = 0; i < total; i++)
		{
			part = parts[i];
			part = part.Trim();
			part.ToLower();
			if (part == "" || outModules.Find(part) >= 0)
			{
				continue;
			}

			outModules.Insert(part);
		}
	}

	// Splits on a separator without string.Split (a script function with an out parameter).
	static void SplitOn(string text, string separator, array<string> outParts)
	{
		int total = text.Length();
		int sepLen = separator.Length();
		int pos = 0;
		int found = 0;
		string part = "";
		outParts.Clear();
		while (pos <= total)
		{
			found = text.IndexOfFrom(pos, separator);
			if (found < 0)
			{
				found = total;
			}

			part = "";
			if (found > pos)
			{
				part = text.Substring(pos, found - pos);
			}

			outParts.Insert(part);
			pos = found + sepLen;
		}
	}

	// A template problem in the admin's language ("Line 3, column 7: ...").
	static string IssueText(VPPWebhookIssueWire issue)
	{
		string body = Tr("#VSTR_WH_ISSUE_" + issue.Code);
		string lineText = issue.Line.ToString();
		string colText = issue.Col.ToString();
		string where = "";
		if (body == "" || body.IndexOf("VSTR_") >= 0)
		{
			body = issue.Message;
		}
		else
		{
			body = string.Format(body, issue.Arg);
		}

		string atFormat = Tr("#VSTR_WH_ISSUE_AT");
		where = string.Format(atFormat, lineText, colText);
		return where + " " + body;
	}

	static int IntervalIndex(VPPWebHookServerStatsTime interval)
	{
		if (interval == VPPWebHookServerStatsTime.ONE_MINUTE)
		{
			return 0;
		}

		if (interval == VPPWebHookServerStatsTime.TEN_MINUTES)
		{
			return 2;
		}

		if (interval == VPPWebHookServerStatsTime.FIFTEEN_MINUTES)
		{
			return 3;
		}

		return 1;
	}

	static VPPWebHookServerStatsTime IntervalOf(int index)
	{
		if (index == 0)
		{
			return VPPWebHookServerStatsTime.ONE_MINUTE;
		}

		if (index == 2)
		{
			return VPPWebHookServerStatsTime.TEN_MINUTES;
		}

		if (index == 3)
		{
			return VPPWebHookServerStatsTime.FIFTEEN_MINUTES;
		}

		return VPPWebHookServerStatsTime.FIVE_MINUTES;
	}

	static void IntervalLabels(array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert(Tr("#VSTR_LBL_INTERVAL_60S"));
		outLabels.Insert(Tr("#VSTR_LBL_INTERVAL_5M"));
		outLabels.Insert(Tr("#VSTR_LBL_INTERVAL_10M"));
		outLabels.Insert(Tr("#VSTR_LBL_INTERVAL_15M"));
	}
};
