// XML Editor MESSAGES tab: wire data and the rules of the server messages files (db/messages.xml and every
// <ce type="messages"> file), taken from the 1.30 server's own loader and scheduler:
// - a <message> reads delay, repeat, deadline, onconnect and shutdown (integers, minutes; anything unreadable is 0),
//   time and text. Text longer than 1151 bytes is cropped.
// - the server skips a message without text or without a mode: onconnect = 1, repeat > 0, deadline > 0 or a time.
//   shutdown = 1 only counts together with a deadline or a time.
// - onconnect: sent once, privately, `delay` minutes after each player connects (delay 0 = 1 second).
// - repeat: broadcast every `repeat` minutes from the server start (delay does not apply).
// - deadline: a countdown from the server start, announced at 90, 60, 45, 30, 20, 15, 10, 5, 4, 3, 2 and 1 minutes
//   left (#tmin = minutes left); with shutdown the server shuts down when it ends.
// - time: "HH:MM[:SS]" then optionally " DD.MM[.YYYY]": time only = every day, with the day = every month, with
//   day.month = every year, a full date = once (skipped at startup when already past). With shutdown it counts down
//   to that time and shuts the server down.
// - #name (server name) and #port are filled in when the file loads, #tmin at every countdown announcement.
// The wire classes derive from Managed: rows and edits are held by more than one array (chunk + working copy, part +
// assembled save), and only Managed objects are reference counted; a plain class held by two refs is freed twice.

class VPPXEMessageRow : Managed
{
	int Line;
	int Delay;
	int Repeat;
	int Deadline;
	int OnConnect;
	int Shutdown;
	string Time;
	string Text;

	void VPPXEMessageRow()
	{
		Time = "";
		Text = "";
	}

	void CopyFrom(VPPXEMessageRow other)
	{
		if (!other)
		{
			return;
		}

		Line = other.Line;
		Delay = other.Delay;
		Repeat = other.Repeat;
		Deadline = other.Deadline;
		OnConnect = other.OnConnect;
		Shutdown = other.Shutdown;
		Time = other.Time;
		Text = other.Text;
	}

	// Same message content (the line number is ignored).
	bool SameAs(VPPXEMessageRow other)
	{
		if (!other)
		{
			return false;
		}

		if (Delay != other.Delay || Repeat != other.Repeat || Deadline != other.Deadline)
		{
			return false;
		}

		if (OnConnect != other.OnConnect || Shutdown != other.Shutdown)
		{
			return false;
		}

		return Time == other.Time && Text == other.Text;
	}
};

class VPPXEMessagesChunk : Managed
{
	int ReqId;
	int FileIdx;
	int FileCount;
	int ChunkIdx;
	int ChunkCount;
	string FileKey;
	int Revision;
	bool Editable;
	string ErrorKey;
	ref array<ref VPPXEMessageRow> Rows;

	void VPPXEMessagesChunk()
	{
		FileKey = "";
		ErrorKey = "";
		Rows = new array<ref VPPXEMessageRow>;
	}
};

class VPPXEMessageEdit : Managed
{
	int Op;
	int Index;
	ref VPPXEMessageRow Row;

	void VPPXEMessageEdit()
	{
		Row = new VPPXEMessageRow();
	}
};

class VPPXEMessagesSave : Managed
{
	int ReqId;
	int PartIdx;
	int PartCount;
	string FileKey;
	int BaseRevision;
	ref array<ref VPPXEMessageEdit> Edits;

	void VPPXEMessagesSave()
	{
		FileKey = "";
		Edits = new array<ref VPPXEMessageEdit>;
	}
};

class VPPXEMessageRules
{
	const static int MAX_TEXT_BYTES = 1151;
	const static int MAX_MINUTES = 525600;
	const static int MAX_ROWS = 2000;
	const static int CHUNK_BYTES = 12000;
	const static int EDITS_PER_PART = 40;
	const static string DIGITS = "0123456789";

	// Minutes left at which a countdown is announced, largest first.
	static void CountdownSteps(array<int> outSteps)
	{
		outSteps.Clear();
		outSteps.Insert(90);
		outSteps.Insert(60);
		outSteps.Insert(45);
		outSteps.Insert(30);
		outSteps.Insert(20);
		outSteps.Insert(15);
		outSteps.Insert(10);
		outSteps.Insert(5);
		outSteps.Insert(4);
		outSteps.Insert(3);
		outSteps.Insert(2);
		outSteps.Insert(1);
	}

	// The server keeps the message (text and at least one mode).
	static bool IsActive(VPPXEMessageRow row)
	{
		if (!row || row.Text == "")
		{
			return false;
		}

		return row.OnConnect == 1 || row.Repeat > 0 || row.Deadline > 0 || row.Time != "";
	}

	// "" when the row can be saved, else the #key of the first problem.
	static string CheckRow(VPPXEMessageRow row)
	{
		int hour = 0;
		int minute = 0;
		int second = 0;
		int day = 0;
		int month = 0;
		int year = 0;
		if (!row)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (!InRange(row.Delay) || !InRange(row.Repeat) || !InRange(row.Deadline))
		{
			return "#VSTR_XMLE_MSG_ERR_MINUTES";
		}

		if (row.OnConnect < 0 || row.OnConnect > 1 || row.Shutdown < 0 || row.Shutdown > 1)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (row.Text == "")
		{
			return "#VSTR_XMLE_MSG_ERR_TEXT_EMPTY";
		}

		if (row.Text.Length() > MAX_TEXT_BYTES)
		{
			return "#VSTR_XMLE_MSG_ERR_TEXT_LONG";
		}

		if (row.Time != "" && ParseTime(row.Time, hour, minute, second, day, month, year) < 0)
		{
			return "#VSTR_XMLE_MSG_ERR_TIME";
		}

		return "";
	}

	protected static bool InRange(int minutes)
	{
		return minutes >= 0 && minutes <= MAX_MINUTES;
	}

	// -1 = invalid, else how many date parts follow the time: 0 = every day, 1 = day (every month), 2 = day.month
	// (every year), 3 = day.month.year (once).
	static int ParseTime(string text, out int hour, out int minute, out int second, out int day, out int month, out int year)
	{
		hour = 0;
		minute = 0;
		second = 0;
		day = 0;
		month = 0;
		year = 0;
		string trimmed = text.Trim();
		if (trimmed == "" || trimmed.Length() > 32)
		{
			return -1;
		}

		string timePart = trimmed;
		string datePart = "";
		int space = trimmed.IndexOf(" ");
		if (space >= 0)
		{
			timePart = trimmed.Substring(0, space);
			datePart = trimmed.Substring(space + 1, trimmed.Length() - space - 1);
			datePart = datePart.Trim();
		}

		array<string> timeTokens = new array<string>();
		if (!SplitTokens(timePart, timeTokens) || timeTokens.Count() < 2 || timeTokens.Count() > 3)
		{
			return -1;
		}

		string hourToken = timeTokens[0];
		string minuteToken = timeTokens[1];
		if (!ParseNumber(hourToken, 0, 23, hour) || !ParseNumber(minuteToken, 0, 59, minute))
		{
			return -1;
		}

		if (timeTokens.Count() == 3)
		{
			string secondToken = timeTokens[2];
			if (!ParseNumber(secondToken, 0, 59, second))
			{
				return -1;
			}
		}

		if (datePart == "")
		{
			return 0;
		}

		array<string> dateTokens = new array<string>();
		if (!SplitTokens(datePart, dateTokens) || dateTokens.Count() < 1 || dateTokens.Count() > 3)
		{
			return -1;
		}

		string dayToken = dateTokens[0];
		if (!ParseNumber(dayToken, 1, 31, day))
		{
			return -1;
		}

		if (dateTokens.Count() >= 2)
		{
			string monthToken = dateTokens[1];
			if (!ParseNumber(monthToken, 1, 12, month))
			{
				return -1;
			}
		}

		if (dateTokens.Count() == 3)
		{
			string yearToken = dateTokens[2];
			if (!ParseNumber(yearToken, 2000, 2999, year))
			{
				return -1;
			}
		}

		return dateTokens.Count();
	}

	// Splits on ":" "." and "/" (the server's separators); false on an empty token.
	protected static bool SplitTokens(string text, array<string> tokens)
	{
		tokens.Clear();
		string current = "";
		int len = text.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = text.Get(i);
			if (ch == ":" || ch == "." || ch == "/")
			{
				if (current == "")
				{
					return false;
				}

				tokens.Insert(current);
				current = "";
				continue;
			}

			current = current + ch;
		}

		if (current == "")
		{
			return false;
		}

		tokens.Insert(current);
		return true;
	}

	protected static bool ParseNumber(string token, int minValue, int maxValue, out int value)
	{
		value = 0;
		int len = token.Length();
		if (len == 0 || len > 4)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			string ch = token.Get(i);
			if (DIGITS.IndexOf(ch) < 0)
			{
				return false;
			}
		}

		value = token.ToInt();
		return value >= minValue && value <= maxValue;
	}

	// A whole-minutes form field: "" = 0; false when it is not a number from 0 to MAX_MINUTES.
	static bool ParseMinutes(string text, out int minutes)
	{
		minutes = 0;
		string trimmed = text.Trim();
		if (trimmed == "")
		{
			return true;
		}

		return ParseLong(trimmed, minutes);
	}

	protected static bool ParseLong(string token, out int minutes)
	{
		minutes = 0;
		int len = token.Length();
		if (len == 0 || len > 6)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			if (DIGITS.IndexOf(token.Get(i)) < 0)
			{
				return false;
			}
		}

		minutes = token.ToInt();
		return minutes <= MAX_MINUTES;
	}

	// Byte estimate of a row on the wire (chunking).
	static int EstimateRow(VPPXEMessageRow row)
	{
		return 48 + row.Time.Length() + row.Text.Length();
	}
};
