class VPPPlayerStats : VPPPlayerTemplate
{
    private ref map<string, TextWidget> m_Stats;
	private ButtonWidget m_btnCopyInfo;
    private string ownerId;
	private string m_CopyText;
	private ref map<string, string> m_stats;
    const ref array<string> m_StatName = {"Name", "SteamID", "Guid", "Health", "Blood", "Shock", "Water", "Energy", "Weapon", "UserGroup"};

    void VPPPlayerStats(GridSpacerWidget grid, map<string, string> stats)
    {
        m_Stats = new map<string, TextWidget>;
        
        m_LayoutPath  = VPPATUIConstants.VPPPlayerInfoBox;
		m_stats 	  = stats;
        m_EntryBox	  = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
		m_btnCopyInfo = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnCopyInfo"));
        WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown( m_btnCopyInfo, this, "ButtonClick" );
		CreateStatsWidgets();
        
        foreach(string stat, string statValue : stats)
        {
            TextWidget statText = m_Stats[stat];
			
			switch(stat)
			{
				case "Name":
				m_CopyText += "Name: "+statValue;
				SetCardTitle(statValue);
				break;
				
				case "SteamID":
				m_CopyText += "\nSteam64 ID: " + statValue;
				ownerId = statValue;
				break;
				
				case "Guid":
				m_CopyText += "\nGUID: "+statValue;
				m_CopyText += "\n\nEnjoy our tools? Please consider supporting us!\nDaOnes's      PayPal: paypal.me/duhonez\nGravityWolf's PayPal: paypal.me/GravityWolf";
				break;
			}
			
            if (stat == "Weapon" && statValue == ""){
                statText.SetText("None");
                continue;
            }

            if(statText)
            {
                //vitals come in as raw floats (e.g. "559.906") - display rounded
                if (stat == "Health" || stat == "Blood" || stat == "Shock" || stat == "Water" || stat == "Energy")
                {
                    int rounded = Math.Round(statValue.ToFloat());
                    statText.SetText(rounded.ToString());
                }
                else
                    statText.SetText(statValue);
            }
        }
        
        UpdateVital("Health", 100.0);
        UpdateVital("Blood",  5000.0);
        UpdateVital("Shock",  100.0);
        UpdateVital("Water",  5000.0);
        UpdateVital("Energy", 5000.0);
        
        if (m_stats.Contains("UserGroup"))
        {
            TextWidget groupTitle = TextWidget.Cast(m_EntryBox.FindAnyWidget("CardTitleGroup"));
            if (groupTitle)
                groupTitle.SetText(m_stats["UserGroup"]);
        }
        
        UpdateFlag("GodMode");
        UpdateFlag("UnlimitedAmmo");
        UpdateFlag("Invisible");
        UpdateFlag("Frozen");
    }
	
	private void SetCardTitle(string playerName)
	{
		TextWidget title = TextWidget.Cast(m_EntryBox.FindAnyWidget("CardTitleName"));
		if (title)
			title.SetText(playerName);
	}
	
	/*
		Renders an admin flag ("1"/"0" from server) as Enabled/Disabled
		using design tokens positive-green / text-muted
	*/
	private void UpdateFlag(string flagName)
	{
		TextWidget flagText = TextWidget.Cast(m_EntryBox.FindAnyWidget("Flag" + flagName));
		if (!flagText) return;
		
		if (m_stats.Contains(flagName) && m_stats[flagName] == "1")
		{
			flagText.SetText("Enabled");
			flagText.SetColor(ARGB(255, 76, 175, 80));
		}
		else
		{
			flagText.SetText("Disabled");
			flagText.SetColor(ARGB(255, 92, 97, 102));
		}
	}
	
	void ButtonClick( Widget w, int x, int y, int button )
	{
		if (w == m_btnCopyInfo) GetGame().CopyToClipboard(m_CopyText);
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SUCCESS_COPY_TOCLIPBOARD");
	}
    
    void ~VPPPlayerStats()
    {
        if (m_EntryBox != null)
        	m_EntryBox.Unlink();
    }
    
    string GetID()
    {
        return ownerId;
    }
	
	string GetStatValue(string name)
	{
		return m_stats[name];
	}
	
	bool HasStat(string name)
	{
		return m_stats.Contains(name);
	}
	
	/*
		Tints the vital icon by threshold and fills the underline bar
		proportionally (design tokens: GUI/Styles/DesignSystem.md)
	*/
	private void UpdateVital(string stat, float maxValue)
	{
		if (!m_stats.Contains(stat)) return;
		
		float value = m_stats[stat].ToFloat();
		float frac  = Math.Clamp(value / maxValue, 0.0, 1.0);
		
		int tint;
		if (frac >= 0.75)
			tint = ARGB(255, 76, 175, 80);   //positive-green
		else if (frac >= 0.5)
			tint = ARGB(255, 217, 178, 61);  //warning-yellow
		else if (frac >= 0.25)
			tint = ARGB(255, 232, 163, 61);  //accent-orange
		else
			tint = ARGB(255, 194, 69, 69);   //danger-red
		
		ImageWidget icon = ImageWidget.Cast(m_EntryBox.FindAnyWidget("Img" + stat));
		if (icon)
			icon.SetColor(tint);
		
		Widget bar = m_EntryBox.FindAnyWidget("Bar" + stat);
		if (bar)
		{
			bar.SetColor(tint);
			bar.SetSize(frac, 1.0);
		}
	}
    
    private void CreateStatsWidgets()
    {
        foreach(string str : m_StatName)
        {
            m_Stats.Insert(str, TextWidget.Cast(m_EntryBox.FindAnyWidget(str)));
        }
    }
}
