class WebHookMessageBase
{
	protected ref array<ref WbEmbed> embeds;
	ref JsonSerializer serializer;
	string content;
	// structured values the producer adds for templates (weapon, distance, positions, ...)
	[NonSerialized()]
	ref map<string, string> m_Extra;

	void WebHookMessageBase()
	{
		embeds = new array<ref WbEmbed>;
		serializer = new JsonSerializer;
	}

	void ~WebHookMessageBase()
	{
		if (serializer)
			delete serializer;
	}

	WbEmbed AddEmbed()
	{
		return null;
	}

	void SetContent(string str = string.Empty)
	{
		content = str;
	}

	string GetContent()
	{
		return content;
	}

	// The event this message becomes for templated webhooks (null = this message type has no event).
	VPPWebhookEvent ToEvent()
	{
		return null;
	}

	void SetVar(string name, string value)
	{
		if (!m_Extra)
		{
			m_Extra = new map<string, string>();
		}

		m_Extra.Set(name, value);
	}

	string GetVar(string name)
	{
		string value = "";
		if (m_Extra && m_Extra.Contains(name))
		{
			value = m_Extra.Get(name);
		}

		return value;
	}

	protected void AddExtras(VPPWebhookEvent webhookEvent)
	{
		if (!m_Extra)
		{
			return;
		}

		for (int i = 0; i < m_Extra.Count(); i++)
		{
			string extraName = m_Extra.GetKey(i);
			string extraValue = m_Extra.GetElement(i);
			webhookEvent.Set(extraName, extraValue);
		}
	}

	string BuildMessage(bool simplified = false)
	{
		string result = "";
		serializer.WriteToString(embeds, true, result);
		if (!simplified)
			return "{" + "\"embeds\": " + result + "}";

		return "{ \"content\": \""+ GetContent() + "\", \"embeds\": null, \"username\": \"VPPAdminTools\", \"avatar_url\": \"https://i.imgur.com/oSEhCJV.png\", \"attachments\": [] }";
	}
};

class WbEmbed
{
	string title;
	int color;
	ref WbAuthor author;
	ref array<ref WbField> fields;
	string timestamp;
	
	void WbEmbed(string ltitle = "WebHook Title", int lcolor = 25500, string authorName = "", string authorURL = "", string authorImg = "")
	{
		fields = new array<ref WbField>;
		this.title = ltitle;
		this.color = lcolor;
		author = new WbAuthor();
		author.SetName(authorName);
		author.SetURL(authorURL);
		author.SetIconURL(authorImg);

		int year, month, day, hour, minute, second;
		GetHourMinuteSecondUTC(hour, minute, second);
		GetYearMonthDayUTC(year, month, day);

		string strHour = hour.ToString(), strMin = minute.ToString();
		if (hour.ToString().Length() == 1)
			strHour = string.Format("%2%1", hour, 0);

		if (minute.ToString().Length() == 1)
			strMin = string.Format("%2%1", minute, 0);

		timestamp = string.Format( "%3-%2-%1T%4:%5:00.000Z", day, month, year, strHour, strMin);
	}

	WbField AddField()
	{
		ref WbField field = new WbField();
		fields.Insert( field );
		return field;
	}
};

class WbField
{
	string name  = "field";
	string value = "content";
	bool inline  = false;

	void SetName(string n)
	{
		if (n == "") return;
		name = n;
	}

	void SetValue(string v)
	{
		if (v == "") return;
		value = v;
	}

	void Inline(bool state)
	{
		inline = state;
	}
};

class WbAuthor
{
	string name     = "VPPAdminTools";
	string url      = "https://steamcommunity.com/sharedfiles/filedetails/?id=1828439124";
	string icon_url = "https://i.imgur.com/oSEhCJV.png";
	
	void SetName(string n)
	{
		if (n == "") return;
		name = n;
	}

	void SetURL(string u)
	{
		if (u == "") return;
		url = u;
	}

	void SetIconURL(string iconurl)
	{
		if (iconurl == "") return;
		icon_url = iconurl;
	}
};