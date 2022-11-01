class JoinLeaveMessage: WebHookMessageBase
{
	private string playerName;
	private string guid;
	private string details;

	void JoinLeaveMessage(string pname, string guid, string details)
	{
		this.playerName   = pname;
		this.details 	  = details;
		this.guid    	  = guid;
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