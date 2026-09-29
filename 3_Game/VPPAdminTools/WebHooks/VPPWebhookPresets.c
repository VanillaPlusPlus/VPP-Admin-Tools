/*
	Webhook presets: complete template sets for common services, built from one set of pieces per event (title, color,
	fields, a Discord markdown line and a plain line). The Discord presets reproduce the messages VPP sent before
	templates existed, so migrated webhooks look the same until an admin edits them.

	Piece texts are written as they appear inside a JSON string (\n = newline). Webhook custom variables used here:
	hook.username, hook.avatar (Discord, Lolka), hook.chat_id (Telegram).

	Lolka (lolka.app/developers/webhooks) takes Discord's webhook body (content, username, avatar_url, embeds with the
	same fields and limits); its presets are the Discord ones with plain markdown links (no Discord "(<url>)" preview
	suppression) and HTTPS links, as Lolka requires external URLs to be HTTPS.
*/
class VPPWebhookPieceField : Managed
{
	string Name;
	string Value;
	bool Inline;
	// the field is only sent when this variable is not empty ("" = always)
	string OnlyIf;
};

class VPPWebhookPiece : Managed
{
	string Title;
	int Color;
	string Line;
	string PlainLine;
	ref array<ref VPPWebhookPieceField> Fields;

	void VPPWebhookPiece(string title, int color)
	{
		Title = title;
		Color = color;
		Line = "";
		PlainLine = "";
		Fields = new array<ref VPPWebhookPieceField>();
	}

	void AddField(string name, string value, bool inline, string onlyIf)
	{
		VPPWebhookPieceField field = new VPPWebhookPieceField();
		field.Name = name;
		field.Value = value;
		field.Inline = inline;
		field.OnlyIf = onlyIf;
		Fields.Insert(field);
	}
};

class VPPWebhookPresets
{
	const static string DISCORD_EMBED = "discord_embed";
	const static string DISCORD_SIMPLE = "discord_simple";
	const static string LOLKA_EMBED = "lolka_embed";
	const static string LOLKA_SIMPLE = "lolka_simple";
	const static string SLACK = "slack";
	const static string TEAMS = "teams";
	const static string TELEGRAM = "telegram";
	const static string GENERIC_JSON = "generic_json";
	const static string PLAIN_TEXT = "plain_text";

	const static string CT_JSON = "application/json";
	const static string CT_TEXT = "text/plain";

	protected static ref map<string, ref VPPWebhookPiece> s_Pieces;

	static void Ids(array<string> outIds)
	{
		outIds.Clear();
		outIds.Insert(DISCORD_EMBED);
		outIds.Insert(DISCORD_SIMPLE);
		outIds.Insert(LOLKA_EMBED);
		outIds.Insert(LOLKA_SIMPLE);
		outIds.Insert(SLACK);
		outIds.Insert(TEAMS);
		outIds.Insert(TELEGRAM);
		outIds.Insert(GENERIC_JSON);
		outIds.Insert(PLAIN_TEXT);
	}

	static bool IsPreset(string presetId)
	{
		array<string> ids = new array<string>();
		Ids(ids);
		return ids.Find(presetId) >= 0;
	}

	static string ContentTypeOf(string presetId)
	{
		if (presetId == PLAIN_TEXT)
		{
			return CT_TEXT;
		}

		return CT_JSON;
	}

	static bool IsDiscord(string presetId)
	{
		return presetId == DISCORD_EMBED || presetId == DISCORD_SIMPLE;
	}

	static bool IsLolka(string presetId)
	{
		return presetId == LOLKA_EMBED || presetId == LOLKA_SIMPLE;
	}

	// Presets whose body follows Discord's webhook format and limits (checked with VPPDiscordLint).
	static bool UsesDiscordLimits(string presetId)
	{
		return IsDiscord(presetId) || IsLolka(presetId);
	}

	// The template of an event in a preset ("" when the event is unknown).
	static string TemplateFor(string presetId, string eventId)
	{
		EnsurePieces();
		VPPWebhookPiece piece = s_Pieces.Get(eventId);
		VPPWebhookEventDef def = VPPWebhookDefs.Get(eventId);
		if (!piece || !def)
		{
			return "";
		}

		if (presetId == DISCORD_SIMPLE)
		{
			return DiscordSimple(piece);
		}

		if (presetId == LOLKA_EMBED)
		{
			string lolkaEmbed = DiscordEmbed(piece);
			return ForLolka(lolkaEmbed);
		}

		if (presetId == LOLKA_SIMPLE)
		{
			string lolkaSimple = DiscordSimple(piece);
			return ForLolka(lolkaSimple);
		}

		if (presetId == SLACK)
		{
			return "{\n  \"text\": \"" + piece.PlainLine + "\"\n}\n";
		}

		if (presetId == TEAMS)
		{
			return Teams(piece);
		}

		if (presetId == TELEGRAM)
		{
			return "{\n  \"chat_id\": \"{{hook.chat_id}}\",\n  \"text\": \"" + piece.PlainLine + "\",\n  \"disable_web_page_preview\": true\n}\n";
		}

		if (presetId == GENERIC_JSON)
		{
			return GenericJson(def);
		}

		if (presetId == PLAIN_TEXT)
		{
			return piece.PlainLine + "\n";
		}

		return DiscordEmbed(piece);
	}

	protected static string DiscordEmbed(VPPWebhookPiece piece)
	{
		string colorText = piece.Color.ToString();
		string text = "{\n  \"embeds\": [\n    {\n      \"title\": \"" + piece.Title + "\",\n      \"color\": " + colorText + ",\n";
		text = text + "      \"author\": {\n        \"name\": \"{{hook.username|default:VPPAdminTools}}\",\n";
		text = text + "        \"url\": \"https://steamcommunity.com/sharedfiles/filedetails/?id=1828439124\",\n";
		text = text + "        \"icon_url\": \"{{hook.avatar|default:https://i.imgur.com/oSEhCJV.png}}\"\n      },\n";
		text = text + "      \"fields\": [\n";
		for (int i = 0; i < piece.Fields.Count(); i++)
		{
			VPPWebhookPieceField field = piece.Fields[i];
			string inlineText = "false";
			if (field.Inline)
			{
				inlineText = "true";
			}

			string entry = "        { \"name\": \"" + field.Name + "\", \"value\": \"" + field.Value + "\", \"inline\": " + inlineText + " }";
			text = text + FieldSeparator(i, field, entry);
		}

		text = text + "\n      ],\n      \"timestamp\": \"{{time.iso}}\"\n    }\n  ]\n}\n";
		return text;
	}

	protected static string DiscordSimple(VPPWebhookPiece piece)
	{
		string text = "{\n  \"content\": \"" + piece.Line + "\",\n";
		text = text + "  \"username\": \"{{hook.username|default:VPPAdminTools}}\",\n";
		text = text + "  \"avatar_url\": \"{{hook.avatar|default:https://i.imgur.com/oSEhCJV.png}}\"\n}\n";
		return text;
	}

	// Discord text -> Lolka: "[x](<url>)" becomes "[x](url)" and the server check link uses HTTPS.
	protected static string ForLolka(string text)
	{
		string converted = text;
		converted.Replace("(<", "(");
		converted.Replace(">)", ")");
		converted.Replace("http://dayzsalauncher.com", "https://dayzsalauncher.com");
		return converted;
	}

	protected static string Teams(VPPWebhookPiece piece)
	{
		string colorHex = HexColor(piece.Color);
		string text = "{\n  \"@type\": \"MessageCard\",\n  \"@context\": \"https://schema.org/extensions\",\n";
		text = text + "  \"themeColor\": \"" + colorHex + "\",\n  \"summary\": \"" + piece.Title + "\",\n  \"title\": \"" + piece.Title + "\",\n";
		text = text + "  \"sections\": [\n    {\n      \"facts\": [\n";
		for (int i = 0; i < piece.Fields.Count(); i++)
		{
			VPPWebhookPieceField field = piece.Fields[i];
			string entry = "        { \"name\": \"" + field.Name + "\", \"value\": \"" + field.Value + "\" }";
			text = text + FieldSeparator(i, field, entry);
		}

		text = text + "\n      ]\n    }\n  ]\n}\n";
		return text;
	}

	// Every variable of the event as one flat JSON object (for services that do their own formatting).
	protected static string GenericJson(VPPWebhookEventDef def)
	{
		string text = "{\n";
		int total = def.Vars.Count();
		for (int i = 0; i < total; i++)
		{
			VPPWebhookVar webhookVar = def.Vars[i];
			string comma = ",";
			if (i == total - 1)
			{
				comma = "";
			}

			text = text + "  \"" + webhookVar.Name + "\": \"{{" + webhookVar.Name + "}}\"" + comma + "\n";
		}

		return text + "}\n";
	}

	// The first field is always sent; a later conditional field carries its own leading comma inside the condition.
	protected static string FieldSeparator(int index, VPPWebhookPieceField field, string entry)
	{
		if (index == 0)
		{
			return entry;
		}

		if (field.OnlyIf == "")
		{
			return ",\n" + entry;
		}

		return "{{#if " + field.OnlyIf + "}},\n" + entry + "{{/if}}";
	}

	static string HexColor(int color)
	{
		string digits = "0123456789ABCDEF";
		string hex = "";
		int remaining = color;
		for (int i = 0; i < 6; i++)
		{
			int nibble = remaining % 16;
			hex = digits.Get(nibble) + hex;
			remaining = remaining / 16;
		}

		return hex;
	}

	protected static void EnsurePieces()
	{
		if (s_Pieces)
		{
			return;
		}

		s_Pieces = new map<string, ref VPPWebhookPiece>();
		string steamLink = "[Steam Profile]({{admin.id|steamurl}})";
		string victimValue = "{{victim.name}}\\n[{{victim.id}}]\\n[Steam Profile]({{victim.id|steamurl}})";
		string serverLink = "(<http://dayzsalauncher.com/#/servercheck/{{server.ip}}:{{server.port}}>)";

		VPPWebhookPiece piece = new VPPWebhookPiece("Admin Activity Report:", 16766720);
		piece.AddField("Name:", "{{admin.name|default:Unknown}}", true, "");
		piece.AddField("Steam64ID:", "{{admin.id}} " + steamLink, true, "");
		piece.AddField("Details:", "**{{action}}**", false, "");
		piece.Line = "**Admin Activity:** [{{admin.name}}](<{{admin.id|steamurl}}>) Details: **{{action}}**";
		piece.PlainLine = "Admin activity: {{admin.name}} ({{admin.id}}): {{action}}";
		s_Pieces.Set(VPPWebhookDefs.EV_ADMIN, piece);

		piece = new VPPWebhookPiece("Kill / Death Report:", 16711680);
		piece.AddField("Victim:", victimValue, true, "");
		string killerValue = "{{#ifeq cause suicide}}Suicide{{else}}{{#if killer.id}}{{killer.name}}\\n[{{killer.id}}]\\n[Steam Profile]({{killer.id|steamurl}}){{else}}{{killer.name}}{{/if}}{{/ifeq}}";
		piece.AddField("Killer:", killerValue, true, "killer.name");
		piece.AddField("Details:", "**{{details}}**", false, "");
		string killLine = "**Player Activity:** [{{victim.name}}](<{{victim.id|steamurl}}>) ";
		killLine = killLine + "{{#ifeq cause infected}}died to an **Infected**.{{else}}{{#ifeq cause suicide}}killed **themselves**.{{else}}";
		killLine = killLine + "{{#if killer.id}}**killed by** [{{killer.name}}](<{{killer.id|steamurl}}>){{/if}}{{/ifeq}}{{/ifeq}} {{details}}";
		piece.Line = killLine;
		string killPlain = "{{victim.name}} died{{#ifeq cause suicide}} (suicide){{/ifeq}}{{#if killer.id}}, killed by {{killer.name}}{{/if}}";
		killPlain = killPlain + "{{#ifeq cause infected}}, killed by infected{{/ifeq}}{{#if weapon}} with {{weapon}}{{/if}}{{#if distance}} from {{distance}} m{{/if}}.";
		piece.PlainLine = killPlain;
		s_Pieces.Set(VPPWebhookDefs.EV_KILL, piece);

		piece = new VPPWebhookPiece("Hit Report:", 16711680);
		piece.AddField("Victim:", victimValue, true, "");
		string sourceValue = "{{#ifeq source.kind object}}{{source.name|default:Unknown}}{{else}}{{#ifeq source.kind self}}Self Harm{{else}}";
		sourceValue = sourceValue + "{{#ifeq source.kind explosion}}Explosion of Type: [ {{source.name}} ]{{else}}";
		sourceValue = sourceValue + "{{#if source.id}}{{source.name}}\\n[{{source.id}}]\\n[Steam Profile]({{source.id|steamurl}}){{else}}{{source.name|default:Unknown}}{{/if}}{{/ifeq}}{{/ifeq}}{{/ifeq}}";
		piece.AddField("Source:", sourceValue, true, "");
		piece.AddField("Details:", "**{{details}}**", false, "");
		string hitLine = "**Hit Damage:** [{{victim.name}}](<{{victim.id|steamurl}}>) {{#ifeq source.kind object}}got hit by: **{{source.name}}**{{else}}";
		hitLine = hitLine + "{{#ifeq source.kind self}}caused damaged to themselves.{{else}}{{#ifeq source.kind explosion}}got hit by an explosion of type: **{{source.name}}**{{else}}";
		hitLine = hitLine + "got hit by: {{#if source.id}}[**{{source.name}}**](<{{source.id|steamurl}}>){{else}}**{{source.name}}**{{/if}}{{/ifeq}}{{/ifeq}}{{/ifeq}} {{details}}";
		piece.Line = hitLine;
		piece.PlainLine = "{{victim.name}} was hit by {{source.name}}{{#if weapon}} with {{weapon}}{{/if}}{{#if zone}} in the {{zone}}{{/if}}.";
		s_Pieces.Set(VPPWebhookDefs.EV_HIT, piece);

		array<string> playerEvents = new array<string>();
		playerEvents.Insert(VPPWebhookDefs.EV_JOIN);
		playerEvents.Insert(VPPWebhookDefs.EV_LEAVE);
		playerEvents.Insert(VPPWebhookDefs.EV_LEAVE_EARLY);
		playerEvents.Insert(VPPWebhookDefs.EV_LOGOUT_START);
		playerEvents.Insert(VPPWebhookDefs.EV_LOGOUT_CANCEL);
		foreach (string playerEvent : playerEvents)
		{
			piece = new VPPWebhookPiece("Player: {{player.name}} {{action}}", 255);
			piece.AddField("Steam64ID:", "{{player.id}}\\n[Steam Profile]({{player.id|steamurl}})", true, "");
			piece.AddField("Players:", "{{players|default:0}}", true, "");
			piece.Line = "[{{player.name}}](<{{player.id|steamurl}}>) {{action}} **{{players}}**";
			piece.PlainLine = "{{player.name}} {{action}} ({{players}} online)";
			s_Pieces.Set(playerEvent, piece);
		}

		piece = new VPPWebhookPiece("**SERVER STATUS REPORT:**\\n**{{server.name}}**", 16766720);
		string statusValue = "[**{{server.name}}**]" + serverLink + "\\nUp-Time: {{uptime}}\\nServer FPS: {{server.fps}}\\nPlayers: {{players}}";
		piece.AddField("Server Status:", statusValue, false, "");
		piece.Line = "**Server Status:** [{{server.name}}]" + serverLink + " Up-Time: **{{uptime}}** Server FPS: **{{server.fps}}** Players: **{{players}}**";
		piece.PlainLine = "{{server.name}}: up {{uptime}}, {{server.fps}} FPS, {{players}} players";
		s_Pieces.Set(VPPWebhookDefs.EV_STATUS, piece);

		piece = new VPPWebhookPiece("**SERVER BOOT-UP REPORT**", 65280);
		piece.AddField("Server Online!", "Server [**{{server.name}}**]" + serverLink + " has **booted-up** successfully!", true, "");
		piece.Line = "Server [**{{server.name}}**]" + serverLink + " successfully started, player connect enabled.";
		piece.PlainLine = "{{server.name}} started, players can connect.";
		s_Pieces.Set(VPPWebhookDefs.EV_BOOT, piece);
	}
};
