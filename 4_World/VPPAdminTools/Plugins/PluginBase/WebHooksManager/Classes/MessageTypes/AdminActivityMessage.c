class AdminActivityMessage: WebHookMessageBase
{
	private string m_uid;
	private string m_msg;
	private string m_name;

	void AdminActivityMessage(string guid, string name, string msg)
	{
		m_uid  = guid;
		m_msg  = "**" + msg + "**";
		m_name = name;
		AddEmbed();
		SetContent();
	}

	/*
	* We use this for simplified versions of the webhook messages (without embeds)
	*/
	override void SetContent(string str = string.Empty)
	{
		content = string.Format("**Admin Activity:** [%1](<https://steamcommunity.com/profiles/%2>) Details: %3", m_name, m_uid, m_msg);
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed = new WbEmbed("Admin Activity Report:", 16766720, "", "", "");
		WbField field;

		field = embed.AddField();
		field.SetName("Name:");
		field.SetValue(m_name);
		field.Inline(true);
		//---
		field = embed.AddField();
		field.SetName("Steam64ID:");
		field.SetValue(m_uid + " [Steam Profile](https://steamcommunity.com/profiles/" + m_uid + ")");
		field.Inline(true);
		//---
		field = embed.AddField();
		field.SetName("Details:");
		field.SetValue(m_msg);
		field.Inline(false);

		embeds.Insert( embed );
		return embed;
	}
};