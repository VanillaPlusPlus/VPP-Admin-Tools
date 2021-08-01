class WebHookMessageBase
{
	protected ref array<ref WbEmbed> embeds;
	ref JsonSerializer serializer;

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

	string BuildMessage()
	{
		string result = "";
		serializer.WriteToString( embeds, false, result );
		return "{" + "\"embeds\": " + result + "}";
	}
};

class WbEmbed
{
	string title;
	int color;
	ref WbAuthor author;
	ref array<ref WbField> fields;
	string timestamp;
	
	void WbEmbed(string title = "WebHook Title", int color = 25500, string authorName = "", string authorURL = "", string authorImg = "")
	{
		fields = new array<ref WbField>;
		this.title = title;
		this.color = color;
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
	string url      = "https://steamcommunity.com/sharedfiles/filedetails/?id=1708571078";
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