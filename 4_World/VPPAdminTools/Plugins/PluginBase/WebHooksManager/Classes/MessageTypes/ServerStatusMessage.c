class ServerStatusMessage: WebHookMessageBase
{
	bool m_BoottUp;

	void ServerStatusMessage(bool bootUp = false)
	{
		m_BoottUp = bootUp;
		AddEmbed();
		SetContent();
	}

	int RGB(int r, int g, int b)
	{
		r = r << 16;
		g = g << 8;
		return r | g | b;
	}

	int ConvertTime(int ms, int div, bool floor = true)
	{
		if (floor){
		return Math.Floor((ms / div));
		}
		return (ms / div);
	}

	/*
	* We use this for simplified versions of the webhook messages (without embeds)
	*/
	override void SetContent(string str = string.Empty)
	{
		int queryPort = GetGame().ServerConfigGetInt("steamQueryPort");
		if (m_BoottUp)
		{
			content = string.Format("Server [**%1**](<http://dayzsalauncher.com/#/servercheck/%3:%2>) successfully started, player connect enabled.", g_Game.GetServerName(), queryPort.ToString(), g_Game.VGetServerIP());
		}
		else
		{
			content = string.Format("**Server Status:** [%1](<http://dayzsalauncher.com/#/servercheck/%3:%2>) ", g_Game.GetServerName(), queryPort.ToString(), g_Game.VGetServerIP());
			content += string.Format("Up-Time: **%1** ", FormatUpTime());
			content += string.Format("Server FPS: **%1** ", g_Game.GetServerFPS());
			content += string.Format("Players: **%1**", GetSteamAPIManager().GetTotalPlayerCount());
		}
	}

	string FormatUpTime()
	{
		int secs = ConvertTime(GetGame().GetTime(), 1000);
	   	int days = secs / (24 * 3600);
        	secs = secs % (24 * 3600);

       	int hours = secs / 3600;
        	secs = secs % 3600;

        int minutes = secs / 60;
        	secs = secs % 60;
		
		return string.Format("%1d %2h %3m %4s", days, hours, minutes, secs);
	}

	override WbEmbed AddEmbed()
	{
		WbEmbed embed;
		WbField field;
		int queryPort = GetGame().ServerConfigGetInt("steamQueryPort");

		if ( m_BoottUp )
		{
			embed = new WbEmbed("**SERVER BOOT-UP REPORT**", RGB(0, 255, 0), "", "", "");
			field;
			//---
			field = embed.AddField();
			field.SetName("Server Online!");
			field.SetValue(string.Format("Server [**%1**](<http://dayzsalauncher.com/#/servercheck/%3:%2>) has **booted-up** successfully!", g_Game.GetServerName(), queryPort.ToString(), g_Game.VGetServerIP()));
			field.Inline(true);

			embeds.Insert( embed );
			return embed;
		}
		else
		{
			embed = new WbEmbed(string.Format("**SERVER STATUS REPORT:**\n**%1**", g_Game.GetServerName()), 16766720, "", "", "");
			field;
			//---
			field = embed.AddField();
			field.SetName("Server Status:");

	        string final;
	        final 			 += string.Format("[**%1**](<http://dayzsalauncher.com/#/servercheck/%3:%2>)", g_Game.GetServerName(), queryPort.ToString(), g_Game.VGetServerIP());
			final 			 += string.Format("\nUp-Time: %1", FormatUpTime());
			final 			 += string.Format("\nServer FPS: %1", g_Game.GetServerFPS());

			final 			 += string.Format("\nPlayers: %1", GetSteamAPIManager().GetTotalPlayerCount());

			field.SetValue(final);
			field.Inline(false);
			embeds.Insert( embed );
			return embed;
		}
		return NULL;
	}
};