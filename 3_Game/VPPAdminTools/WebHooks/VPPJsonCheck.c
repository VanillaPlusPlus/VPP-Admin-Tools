/*
	JSON validation with the game's own parser (JsonSerializer.ReadFromString = RapidJSON, memory:
	dayz-restapi-json-internals): the whole text is parsed before it is mapped to the target, so any target works as a
	strict syntax check. The engine's message says what is wrong but not where; VPPJsonLocator finds the line / column.
	Discord payloads are also read into typed classes to check Discord's limits.
*/
class VPPJsonProbe
{
};

class VPPJsonCheckResult : Managed
{
	bool Valid;
	// the engine's message ("Missing a comma or '}' after an object member.") when not valid
	string Message;
	int Line;
	int Col;

	void VPPJsonCheckResult()
	{
		Valid = true;
		Message = "";
	}
};

class VPPJsonCheck
{
	// RapidJSON's parse error texts: a ReadFromString error without one of them is a mapping error after a good parse.
	protected static ref array<string> s_SyntaxMessages;

	static VPPJsonCheckResult Check(string text)
	{
		VPPJsonCheckResult result = new VPPJsonCheckResult();
		JsonSerializer serializer = new JsonSerializer();
		VPPJsonProbe probe = new VPPJsonProbe();
		string error = "";
		if (serializer.ReadFromString(probe, text, error))
		{
			return result;
		}

		string syntaxMessage = SyntaxMessageOf(error);
		if (syntaxMessage == "")
		{
			// parsed fine, only the probe mapping complained (e.g. the root is an array)
			return result;
		}

		result.Valid = false;
		result.Message = syntaxMessage;
		int errorPos = VPPJsonLocator.FirstError(text);
		result.Line = VPPWebhookText.LineOf(text, errorPos);
		result.Col = VPPWebhookText.ColOf(text, errorPos);
		return result;
	}

	protected static string SyntaxMessageOf(string error)
	{
		if (!s_SyntaxMessages)
		{
			s_SyntaxMessages = new array<string>();
			s_SyntaxMessages.Insert("The document is empty.");
			s_SyntaxMessages.Insert("The document root must not be followed by other values.");
			s_SyntaxMessages.Insert("Invalid value.");
			s_SyntaxMessages.Insert("Missing a name for object member.");
			s_SyntaxMessages.Insert("Missing a colon after a name of object member.");
			s_SyntaxMessages.Insert("Missing a comma or '}' after an object member.");
			s_SyntaxMessages.Insert("Missing a comma or ']' after an array element.");
			s_SyntaxMessages.Insert("Incorrect hex digit after \\u escape in string.");
			s_SyntaxMessages.Insert("The surrogate pair in string is invalid.");
			s_SyntaxMessages.Insert("Invalid escape character in string.");
			s_SyntaxMessages.Insert("Missing a closing quotation mark in string.");
			s_SyntaxMessages.Insert("Invalid encoding in string.");
			s_SyntaxMessages.Insert("Number too big to be stored in double.");
			s_SyntaxMessages.Insert("Miss fraction part in number.");
			s_SyntaxMessages.Insert("Miss exponent in number.");
			s_SyntaxMessages.Insert("Unspecific syntax error.");
		}

		foreach (string known : s_SyntaxMessages)
		{
			if (error.Contains(known))
			{
				return known;
			}
		}

		return "";
	}
};

// Where the first JSON error is (byte position): a small strict scanner, used only to point at the problem the engine
// reported.
class VPPJsonLocator
{
	protected string m_Text;
	protected int m_Pos;
	protected int m_Len;
	protected bool m_Failed;
	protected int m_FailPos;

	static int FirstError(string text)
	{
		VPPJsonLocator locator = new VPPJsonLocator();
		return locator.Run(text);
	}

	const static int STATE_VALUE = 0;
	const static int STATE_KEY = 1;
	const static int STATE_AFTER = 2;
	const static int IN_OBJECT = 1;
	const static int IN_ARRAY = 2;
	const static int MAX_DEPTH = 64;

	// A flat state machine with an explicit container stack (recursion is unreliable in Enforce). Returns the byte
	// position of the first error, or the text length when the text is valid.
	int Run(string text)
	{
		int state = STATE_VALUE;
		int top = 0;
		string ch = "";
		array<int> containers = new array<int>();
		m_Text = text;
		m_Pos = 0;
		m_Len = text.Length();
		m_Failed = false;
		m_FailPos = 0;
		while (!m_Failed)
		{
			SkipSpace();
			if (state == STATE_VALUE)
			{
				ch = Peek();
				if (ch == "{" || ch == "[")
				{
					m_Pos++;
					SkipSpace();
					if ((ch == "{" && Peek() == "}") || (ch == "[" && Peek() == "]"))
					{
						m_Pos++;
						state = STATE_AFTER;
						continue;
					}

					if (containers.Count() >= MAX_DEPTH)
					{
						Fail();
						break;
					}

					if (ch == "{")
					{
						containers.Insert(IN_OBJECT);
						state = STATE_KEY;
					}
					else
					{
						containers.Insert(IN_ARRAY);
						state = STATE_VALUE;
					}

					continue;
				}

				ReadScalar(ch);
				state = STATE_AFTER;
				continue;
			}

			if (state == STATE_KEY)
			{
				if (Peek() != "\"")
				{
					Fail();
					break;
				}

				ReadString();
				SkipSpace();
				if (Peek() != ":")
				{
					Fail();
					break;
				}

				m_Pos++;
				state = STATE_VALUE;
				continue;
			}

			if (containers.Count() == 0)
			{
				break;
			}

			top = containers[containers.Count() - 1];
			ch = Peek();
			if (ch == ",")
			{
				m_Pos++;
				if (top == IN_OBJECT)
				{
					state = STATE_KEY;
				}
				else
				{
					state = STATE_VALUE;
				}

				continue;
			}

			if ((top == IN_OBJECT && ch == "}") || (top == IN_ARRAY && ch == "]"))
			{
				m_Pos++;
				containers.RemoveOrdered(containers.Count() - 1);
				continue;
			}

			Fail();
		}

		if (!m_Failed)
		{
			SkipSpace();
			if (m_Pos < m_Len)
			{
				Fail();
			}
		}

		if (!m_Failed)
		{
			return m_Len;
		}

		return m_FailPos;
	}

	protected void Fail()
	{
		if (!m_Failed)
		{
			m_Failed = true;
			m_FailPos = m_Pos;
		}
	}

	protected string Peek()
	{
		if (m_Pos >= m_Len)
		{
			return "";
		}

		return m_Text.Get(m_Pos);
	}

	protected void SkipSpace()
	{
		while (m_Pos < m_Len)
		{
			string ch = m_Text.Get(m_Pos);
			int code = ch.ToAscii();
			if (code != 32 && code != 9 && code != 10 && code != 13)
			{
				return;
			}

			m_Pos++;
		}
	}

	// A string, true / false / null or a number; anything else is an error.
	protected void ReadScalar(string ch)
	{
		if (ch == "\"")
		{
			ReadString();
		}
		else if (ch == "t")
		{
			ReadWord("true");
		}
		else if (ch == "f")
		{
			ReadWord("false");
		}
		else if (ch == "n")
		{
			ReadWord("null");
		}
		else if (ch == "-" || IsDigit(ch))
		{
			ReadNumber();
		}
		else
		{
			Fail();
		}
	}

	protected void ReadString()
	{
		m_Pos++;
		while (m_Pos < m_Len)
		{
			string ch = m_Text.Get(m_Pos);
			int code = ch.ToAscii();
			if (ch == "\"")
			{
				m_Pos++;
				return;
			}

			if (code >= 0 && code < 32)
			{
				Fail();
				return;
			}

			if (ch == "\\")
			{
				m_Pos++;
				string escaped = Peek();
				if (escaped == "u")
				{
					m_Pos += 5;
					continue;
				}

				if (escaped != "\"" && escaped != "\\" && escaped != "/" && escaped != "b" && escaped != "f" && escaped != "n" && escaped != "r" && escaped != "t")
				{
					Fail();
					return;
				}
			}

			m_Pos++;
		}

		Fail();
	}

	protected void ReadWord(string word)
	{
		int wordLen = word.Length();
		if (m_Pos + wordLen > m_Len || m_Text.Substring(m_Pos, wordLen) != word)
		{
			Fail();
			return;
		}

		m_Pos += wordLen;
	}

	protected void ReadNumber()
	{
		if (Peek() == "-")
		{
			m_Pos++;
		}

		if (!IsDigit(Peek()))
		{
			Fail();
			return;
		}

		while (IsDigit(Peek()))
		{
			m_Pos++;
		}

		if (Peek() == ".")
		{
			m_Pos++;
			if (!IsDigit(Peek()))
			{
				Fail();
				return;
			}

			while (IsDigit(Peek()))
			{
				m_Pos++;
			}
		}

		string exponent = Peek();
		if (exponent == "e" || exponent == "E")
		{
			m_Pos++;
			string sign = Peek();
			if (sign == "+" || sign == "-")
			{
				m_Pos++;
			}

			if (!IsDigit(Peek()))
			{
				Fail();
				return;
			}

			while (IsDigit(Peek()))
			{
				m_Pos++;
			}
		}
	}

	protected bool IsDigit(string ch)
	{
		if (ch == "")
		{
			return false;
		}

		int code = ch.ToAscii();
		return code >= 48 && code <= 57;
	}
};

// ---------------------------------------------------------------- Discord payload limits

class VPPDiscordField
{
	string name;
	string value;
};

class VPPDiscordFooter
{
	string text;
};

class VPPDiscordAuthor
{
	string name;
};

class VPPDiscordEmbed
{
	string title;
	string description;
	ref array<ref VPPDiscordField> fields;
	ref VPPDiscordFooter footer;
	ref VPPDiscordAuthor author;
};

class VPPDiscordPayload
{
	string content;
	string username;
	ref array<ref VPPDiscordEmbed> embeds;
};

class VPPDiscordLint
{
	// Discord's documented limits; problems are English lines (codes for the UI later).
	static void Check(string body, array<string> outProblems)
	{
		outProblems.Clear();
		JsonSerializer serializer = new JsonSerializer();
		VPPDiscordPayload payload = new VPPDiscordPayload();
		string error = "";
		if (!serializer.ReadFromString(payload, body, error))
		{
			outProblems.Insert("Discord could not read this payload: a field has the wrong type (" + error.Trim() + ").");
			return;
		}

		int contentLen = payload.content.LengthUtf8();
		if (contentLen > 2000)
		{
			outProblems.Insert("content is " + contentLen.ToString() + " characters (Discord allows 2000).");
		}

		int embedCount = 0;
		if (payload.embeds)
		{
			embedCount = payload.embeds.Count();
		}

		if (contentLen == 0 && embedCount == 0)
		{
			outProblems.Insert("Discord needs content or at least one embed.");
		}

		if (embedCount > 10)
		{
			outProblems.Insert("There are " + embedCount.ToString() + " embeds (Discord allows 10).");
		}

		int total = 0;
		for (int e = 0; e < embedCount; e++)
		{
			VPPDiscordEmbed embed = payload.embeds[e];
			if (!embed)
			{
				continue;
			}

			total += CheckText(embed.title, 256, "embed title", outProblems);
			total += CheckText(embed.description, 4096, "embed description", outProblems);
			if (embed.author)
			{
				total += CheckText(embed.author.name, 256, "embed author name", outProblems);
			}

			if (embed.footer)
			{
				total += CheckText(embed.footer.text, 2048, "embed footer", outProblems);
			}

			if (!embed.fields)
			{
				continue;
			}

			if (embed.fields.Count() > 25)
			{
				outProblems.Insert("An embed has more than 25 fields.");
			}

			foreach (VPPDiscordField field : embed.fields)
			{
				if (!field)
				{
					continue;
				}

				if (field.name == "" || field.value == "")
				{
					outProblems.Insert("An embed field has an empty name or value (Discord rejects it).");
				}

				total += CheckText(field.name, 256, "field name", outProblems);
				total += CheckText(field.value, 1024, "field value", outProblems);
			}
		}

		if (total > 6000)
		{
			outProblems.Insert("The embeds hold " + total.ToString() + " characters in total (Discord allows 6000).");
		}
	}

	protected static int CheckText(string text, int limit, string label, array<string> outProblems)
	{
		int textLen = text.LengthUtf8();
		if (textLen > limit)
		{
			outProblems.Insert("A " + label + " is " + textLen.ToString() + " characters (Discord allows " + limit.ToString() + ").");
		}

		return textLen;
	}
};
