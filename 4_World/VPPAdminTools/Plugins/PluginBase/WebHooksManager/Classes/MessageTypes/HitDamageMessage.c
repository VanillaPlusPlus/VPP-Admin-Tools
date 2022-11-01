class HitDamageMessage: WebHookMessageBase
{
	string victimName, sourceName, victimId, sourceId, details;

	void HitDamageMessage()
	{
	}

	/*
	* We use this for simplified versions of the webhook messages (without embeds)
	*/
	override void SetContent(string str = string.Empty)
	{
		content = string.Format("**Hit Damage:** [%1](<https://steamcommunity.com/profiles/%2>) ", victimName, victimId);
	
		if (sourceId == "_obj")
		{
			content += string.Format("got hit by: **%1**", sourceName);
		}
		else if (sourceId == victimId || sourceName == victimName)
		{
			content += "caused damaged to themselves.";
		}
		else if (sourceId == "Explosion")
		{
			content += string.Format("got hit by an explosion of type: **%1**", sourceName);
		}
		else
		{
			content += string.Format("got hit by: [**%1**](<https://steamcommunity.com/profiles/%2>) ", sourceName, sourceId);
		}

		content += " " + details;
		content.Replace("\n", " ");
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed = new WbEmbed("Hit Report:", 16711680, "", "", "");
		WbField field;
		//--
		field = embed.AddField();
		field.SetName("Victim:");
		field.SetValue( string.Format("%1\n[%2]\n[Steam Profile](https://steamcommunity.com/profiles/%2)", victimName, victimId) );
		field.Inline(true);
		//--
		field = embed.AddField();
		field.SetName("Source:");
		if (sourceId == "_obj")
			field.SetValue(string.Format("%1", sourceName));
		else if (sourceId == victimId || sourceName == victimName)
			field.SetValue(string.Format("Self Harm"));
		else if ( sourceId == "Explosion" )
			field.SetValue(string.Format("Explosion of Type: [ %1 ]", sourceName));
		else
			field.SetValue(string.Format("%1\n[%2]\n[Steam Profile](https://steamcommunity.com/profiles/%2)", sourceName, sourceId));
		
		field.Inline(true);
		//--
		field = embed.AddField();
		field.SetName("Details:");
		field.SetValue("**" + details + "**");
		field.Inline(false);

		embeds.Insert( embed );
		return embed;
	}
};