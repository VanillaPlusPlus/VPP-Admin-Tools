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

		embeds.Insert( embed );
		return embed;
	}
};