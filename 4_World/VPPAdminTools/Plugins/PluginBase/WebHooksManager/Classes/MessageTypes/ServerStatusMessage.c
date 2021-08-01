class ServerStatusMessage: WebHookMessageBase
{
	private bool m_BoottUp;

	void ServerStatusMessage(bool bootUp = true)
	{
		m_BoottUp = bootUp;
		AddEmbed();
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

	override WbEmbed AddEmbed()
	{
		WbEmbed embed;
		WbField field;

		if ( m_BoottUp )
		{
			embed = new WbEmbed("**SERVER BOOT-UP REPORT**", RGB(0, 255, 0), "", "", "");
			field;
			//---
			field = embed.AddField();
			field.SetName("Server Online!");
			field.SetValue(string.Format("The DayZ Server **%1** has **booted-up** successfully!", g_Game.GetServerName()));
			field.Inline(true);

			embeds.Insert( embed );
			return embed;
		}
		else
		{
			embed = new WbEmbed(string.Format("**SERVER STATUS REPORT:**\n%1", g_Game.GetServerName()), 16766720, "", "", "");
			field;
			//---
			field = embed.AddField();
			field.SetName("Server Status:");

			int secs = ConvertTime(GetGame().GetTime(), 1000);
		   	int days = secs / (24 * 3600);
	        	secs = secs % (24 * 3600);

	       	int hours = secs / 3600;
	        	secs = secs % 3600;

	        int minutes = secs / 60;
	        	secs = secs % 60;

	        string final;
	        string strUptime  = string.Format("%1d %2h %3m %4s", days, hours, minutes, secs);
			final 			 += string.Format("Up-Time: %1", strUptime);
			final 			 += string.Format("\nServer FPS: %1", g_Game.GetServerFPS());

			array<Man> players = new array<Man>;
			GetGame().GetWorld().GetPlayerList(players);
			final 			 += string.Format("\nOnline Players: %1", players.Count());

			field.SetValue(final);
			field.Inline(false);
			embeds.Insert( embed );
			return embed;
		}
		return NULL;
	}
};