class KillDeathMessage: WebHookMessageBase
{
	string killerName, killerGUID;
	string victimName, victimGUID;
	string details;

	void KillDeathMessage()
	{
		
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed = new WbEmbed("Kill / Death Report:", 16711680, "", "", "");
		WbField field;

		if (victimName != "" || victimGUID != "")
		{
			field = embed.AddField();
			field.SetName("Victim:");
			field.SetValue(victimName+ "\n[" + victimGUID + "]\n[Steam Profile](https://steamcommunity.com/profiles/" + victimGUID + ")");
			field.Inline(true);
		}
		//---
		if (killerGUID != "" || killerName != "")
		{
			field = embed.AddField();
			field.SetName("Killer:");
			if (killerName == "Infected")
				field.SetValue(killerName);
			else if (killerGUID == victimGUID || killerName == victimName)
				field.SetValue("Suicide");
			else
				field.SetValue(killerName+ "\n[" + killerGUID + "]\n[Steam Profile](https://steamcommunity.com/profiles/" + killerGUID + ")");

			field.Inline(true);
		}
		//---
		field = embed.AddField();
		field.SetName("Details:");
		field.SetValue("**" + details + "**");
		field.Inline(false);

		embeds.Insert( embed );
		return embed;
	}
};