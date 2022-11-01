class KillDeathMessage: WebHookMessageBase
{
	string killerName, killerGUID;
	string victimName, victimGUID;
	string details;

	void KillDeathMessage()
	{
	}

	/*
	* We use this for simplified versions of the webhook messages (without embeds)
	*/
	override void SetContent(string str = string.Empty)
	{
		if (victimName != "" || victimGUID != "")
		{
			content = string.Format("**Player Activity:** [%1](<https://steamcommunity.com/profiles/%2>) ", victimName, victimGUID);
		}

		if (killerGUID != "" || killerName != "")
		{
			if (killerName == "Infected")
			{
				content += string.Format("died to an **Infected**.");
			}
			else if (killerGUID == victimGUID || killerName == victimName)
			{
				content += string.Format("killed **themselves**.");
			}else{
				content += string.Format("**killed by** [%1](<https://steamcommunity.com/profiles/%2>) ", killerName, killerGUID);
			}
		}
		content += " " + details;
		content.Replace("\n", " ");
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