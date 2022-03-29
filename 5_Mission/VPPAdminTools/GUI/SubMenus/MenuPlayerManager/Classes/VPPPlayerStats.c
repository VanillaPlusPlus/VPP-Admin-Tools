class VPPPlayerStats : VPPPlayerTemplate
{
    private ref map<string, TextWidget> m_Stats;
    private ImageWidget  m_IconHealth;
    private ImageWidget  m_IconBlood;
	private ButtonWidget m_btnCopyInfo;
    private string ownerId;
	private string m_CopyText;
	private ref map<string, string> m_stats;
    const ref array<string> m_StatName = {"Name", "SteamID", "Guid", "Health", "Blood", "Shock", "Weapon","UserGroup"};

    
    void VPPPlayerStats(GridSpacerWidget grid, map<string, string> stats)
    {
        m_Stats = new map<string, TextWidget>;
        
        m_LayoutPath  = VPPATUIConstants.VPPPlayerInfoBox;
		m_stats 	  = stats;
        m_EntryBox	  = GetGame().GetWorkspace().CreateWidgets(m_LayoutPath, grid);
	    m_IconHealth  = ImageWidget.Cast(m_EntryBox.FindAnyWidget("ImgHealth"));
		m_IconBlood   = ImageWidget.Cast(m_EntryBox.FindAnyWidget("ImgBlood"));
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
			
			if (stat == "Health"){
				int value = statValue.ToInt();
				if (value == 100 && value >= 75)
				{
					m_IconHealth.LoadImageFile(0,"set:dayz_gui image:iconHealth0");
				}else if (value < 75 && value >= 50){
					m_IconHealth.LoadImageFile(0,"set:dayz_gui image:iconHealth1");
				}else if (value < 50 && value >= 25){
					m_IconHealth.LoadImageFile(0,"set:dayz_gui image:iconHealth2");
				}else if (value < 25 && value >= 1){
					m_IconHealth.LoadImageFile(0,"set:dayz_gui image:iconHealth3");
				}else if (value == 0){
					m_IconHealth.LoadImageFile(0,"set:dayz_gui image:iconHealth4");
				}
			}
			
			if (stat == "Blood"){
				if (value == 5000 && value >= 4000)
				{
					m_IconBlood.LoadImageFile(0,"set:dayz_gui image:iconBlood0");
				}else if (value < 4000 && value >= 3000){
					m_IconBlood.LoadImageFile(0,"set:dayz_gui image:iconBlood1");
				}else if (value < 3000 && value >= 2000){
					m_IconBlood.LoadImageFile(0,"set:dayz_gui image:iconBlood2");
				}else if (value < 2000 && value >= 1000){
					m_IconBlood.LoadImageFile(0,"set:dayz_gui image:iconBlood3");
				}else if (value == 0){
					m_IconBlood.LoadImageFile(0,"set:dayz_gui image:iconBlood4");
				}
			}
            
            if (stat == "Weapon" && statValue == ""){
                statText.SetText("None");
                continue;
            }

            if(statText)
                statText.SetText(statValue);
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
    
    private void CreateStatsWidgets()
    {
        foreach(string str : m_StatName)
        {
            m_Stats.Insert(str, TextWidget.Cast(m_EntryBox.FindAnyWidget(str)));
        }
    }
}