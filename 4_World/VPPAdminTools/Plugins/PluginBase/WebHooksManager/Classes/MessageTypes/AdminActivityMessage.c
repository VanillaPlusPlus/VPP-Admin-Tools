class AdminActivityMessage: WebHookMessageBase
{
	private string m_uid;
	private string m_msg;
	private string m_name;
	private string m_raw;

	void AdminActivityMessage(string guid, string name, string msg)
	{
		m_uid  = guid;
		m_raw  = msg;
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

	// action = the text as given; module = the "[Module]" prefix most callers use, action.text = the rest.
	override VPPWebhookEvent ToEvent()
	{
		VPPWebhookEvent webhookEvent = new VPPWebhookEvent(VPPWebhookDefs.EV_ADMIN);
		webhookEvent.Set("admin.name", m_name);
		webhookEvent.Set("admin.id", m_uid);
		webhookEvent.Set("action", m_raw);
		string moduleName = "";
		string actionText = m_raw;
		string trimmed = m_raw.Trim();
		if (trimmed.IndexOf("[") == 0)
		{
			int close = trimmed.IndexOf("]");
			if (close > 1)
			{
				moduleName = trimmed.Substring(1, close - 1);
				string rest = trimmed.Substring(close + 1, trimmed.Length() - close - 1);
				actionText = rest.Trim();
				if (actionText.IndexOf("::") == 0)
				{
					string afterColons = actionText.Substring(2, actionText.Length() - 2);
					actionText = afterColons.Trim();
				}
			}
		}

		webhookEvent.Set("module", moduleName);
		webhookEvent.Set("action.text", actionText);
		AddExtras(webhookEvent);
		return webhookEvent;
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