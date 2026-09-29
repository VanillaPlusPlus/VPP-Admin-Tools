class JoinLeaveMessage: WebHookMessageBase
{
	private string playerName;
	private string guid;
	private string details;
	private string m_EventId;

	// eventId: VPPWebhookDefs.EV_JOIN / EV_LEAVE / ... ("" = guessed from the text, for older callers)
	void JoinLeaveMessage(string pname, string lguid, string ldetails, string eventId = "")
	{
		this.playerName   = pname;
		this.details 	  = ldetails;
		this.guid    	  = lguid;
		m_EventId = eventId;
		AddEmbed();
		SetContent();
	}

	/*
	* We use this for simplified versions of the webhook messages (without embeds)
	*/
	override void SetContent(string str = string.Empty)
	{
		content = string.Format("[%1](<https://steamcommunity.com/profiles/%2>) %3 ", playerName, guid, details);
		content += string.Format("**%1**",GetSteamAPIManager().GetTotalPlayerCount());
	}

	override VPPWebhookEvent ToEvent()
	{
		string eventId = m_EventId;
		if (eventId == "")
		{
			eventId = GuessEvent(details);
		}

		VPPWebhookEvent webhookEvent = new VPPWebhookEvent(eventId);
		webhookEvent.Set("player.name", playerName);
		webhookEvent.Set("player.id", guid);
		webhookEvent.Set("action", details);
		AddExtras(webhookEvent);
		return webhookEvent;
	}

	protected string GuessEvent(string text)
	{
		string lower = text;
		lower.ToLower();
		if (lower.Contains("joined"))
		{
			return VPPWebhookDefs.EV_JOIN;
		}

		if (lower.Contains("early"))
		{
			return VPPWebhookDefs.EV_LEAVE_EARLY;
		}

		if (lower.Contains("canceled") || lower.Contains("cancelled"))
		{
			return VPPWebhookDefs.EV_LOGOUT_CANCEL;
		}

		if (lower.Contains("initiated"))
		{
			return VPPWebhookDefs.EV_LOGOUT_START;
		}

		return VPPWebhookDefs.EV_LEAVE;
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed = new WbEmbed("Player: " + playerName + " " + details, 255, "", "", "");
		WbField field;

		//---
		field = embed.AddField();
		field.SetName("Steam64ID:");
		field.SetValue(guid + "\n[Steam Profile](https://steamcommunity.com/profiles/" + guid + ")");
		field.Inline(true);

		field = embed.AddField();
		field.SetName("Players:");
		field.SetValue(GetSteamAPIManager().GetTotalPlayerCount());
		field.Inline(true);

		embeds.Insert( embed );
		return embed;
	}
};