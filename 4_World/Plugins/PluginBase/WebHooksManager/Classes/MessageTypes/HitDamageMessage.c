class HitDamageMessage: WebHookMessageBase
{
	string victimName, sourceName, victimId, sourceId, details;

	void HitDamageMessage()
	{
		
	}

	override ref WbEmbed AddEmbed()
	{
		ref WbEmbed embed = new WbEmbed("Hit Report:", 16711680, "", "", "");
		ref WbField field;
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
			field.SetValue(string.Format("Self Harm, call the authorities on this psycho, give him/her a hug or something"));
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