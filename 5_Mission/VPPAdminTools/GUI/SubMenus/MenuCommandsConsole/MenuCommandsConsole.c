class MenuCommandsConsole: AdminHudSubMenu
{
	private bool             	   m_Init;
	private ScrollWidget 	 	   m_ScrollerFeed;
	private GridSpacerWidget 	   m_GridFeed;
	private GridSpacerWidget 	   m_GridSuggest;
	private ScrollWidget 	 	   m_ScrollerSuggest;
	private MultilineEditBoxWidget m_CommandInput;
	private ImageWidget      	   m_ImgInfo;
	private bool   				   inputFocused;
	private ref array<string> 	   m_PrevCommands;
	private int index = 0;
	
	/* You can insert into these arrays your own custom filters by modding this class! */
	ref array<ref Widget> 					  m_HistoryWidgets;
	ref map<ref Widget, string> 			  m_FilterWidgets;
	ref map<string,ref Param2<string,string>> m_CmdTooltips;

	void MenuCommandsConsole()
	{
		m_HistoryWidgets = new array<ref Widget>;
		m_FilterWidgets = new map<ref Widget, string>;
		m_PrevCommands = new array<string>;
		m_CmdTooltips  = new map<string,ref Param2<string,string>>;
		m_CmdTooltips.Insert("/sph", new Param2<string,string>("#VSTR_TOOLTIP_SPH", "<classname>"));
		m_CmdTooltips.Insert("/spg", new Param2<string,string>("#VSTR_TOOLTIP_SPG", "<classname>"));
		m_CmdTooltips.Insert("/spawncar", new Param2<string,string>("#VSTR_TOOLTIP_SPAWNCAR", ""));
		m_CmdTooltips.Insert("/refuel", new Param2<string,string>("#VSTR_TOOLTIP_REFUEL", ""));
		m_CmdTooltips.Insert("/spi", new Param2<string,string>("#VSTR_TOOLTIP_SPI", "<classname>"));
		m_CmdTooltips.Insert("/ammo", new Param2<string,string>("#VSTR_TOOLTIP_AMMO", ""));
		m_CmdTooltips.Insert("/tpp", new Param2<string,string>("#VSTR_TOOLTIP_TPP", "<7500,0,7500>"));
		m_CmdTooltips.Insert("/tpt", new Param2<string,string>("#VSTR_TOOLTIP_TPT", "<PresetName>"));
		m_CmdTooltips.Insert("/goto", new Param2<string,string>("#VSTR_TOOLTIP_GOTO", "<PlayerName>"));
		m_CmdTooltips.Insert("/return", new Param2<string,string>("#VSTR_TOOLTIP_RETURN", "<PlayerName>"));
		m_CmdTooltips.Insert("/bring", new Param2<string,string>("#VSTR_TOOLTIP_BRING", "<PlayerName>"));
		m_CmdTooltips.Insert("/heal", new Param2<string,string>("#VSTR_TOOLTIP_HEAL", "<PlayerName> OR self"));
		m_CmdTooltips.Insert("/kill", new Param2<string,string>("#VSTR_TOOLTIP_KILL", "<PlayerName> OR self"));
		m_CmdTooltips.Insert("/strip", new Param2<string,string>("#VSTR_TOOLTIP_STRIP", "<PlayerName> OR self"));
		m_CmdTooltips.Insert("/clear", new Param2<string,string>("#VSTR_TOOLTIP_CLEAR", ""));
		if (VPPAT_IsModLoadded("NAMALSK_SURVIVAL")){
			m_CmdTooltips.Insert("/runevent", new Param2<string,string>("Force start a Namalsk global event, types of events from init.c", "/runevent EVRStorm"));
		}
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.CommandsConsoleUI);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		M_SUB_WIDGET.SetHandler(this);

		m_ImgInfo      = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfo"));
		m_ScrollerFeed = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerFeed"));
		m_GridFeed     = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget("GridFeed"));
		m_GridSuggest  = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget("GridSuggest"));
		m_ScrollerSuggest = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollerSuggest"));
		m_CommandInput    = MultilineEditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("CommandInput"));
		
		//tooltip
		ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_TOOLTIP_COMMANDCONSOLE_KEYS");
		
		
		foreach(string command, Param2<string,string> params: m_CmdTooltips)
		{
			Widget sgElement = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.TextElement, m_GridSuggest);
			MultilineTextWidget.Cast(sgElement.FindAnyWidget("Message")).SetText("Command: " + command + "\n" + "Action: " + params.param1 + "\nExample: " + params.param2);
			sgElement.Show(false);
			
			m_ScrollerSuggest.Update();
			m_ScrollerSuggest.VScrollStep(1);
			m_GridSuggest.Update();
			m_FilterWidgets.Insert( sgElement, command);
		}
		
		m_Init = true;
	}
	
	void InsertHistory(string input)
	{
		if (input == string.Empty) return;
		
		m_PrevCommands.Insert(input);
		if (m_HistoryWidgets.Count() >= 99)
			ClearHistory();
		
		Widget historyWidget = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.TextElement, m_GridFeed);
		MultilineTextWidget txtWidget = MultilineTextWidget.Cast(historyWidget.FindAnyWidget("Message"));
		txtWidget.SetText(input);

		m_HistoryWidgets.Insert(historyWidget);
		m_GridFeed.Update();
		m_ScrollerFeed.VScrollToPos(m_ScrollerFeed.GetContentHeight() * 1000);
		m_ScrollerFeed.Update();
	}
	
	void ClearHistory()
	{
		foreach(Widget w: m_HistoryWidgets)
			w.Unlink();
		
		m_HistoryWidgets = new array<ref Widget>;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		Input input = GetGame().GetInput();
		if (inputFocused)
		{
			if (input.LocalPress("UAExecuteCommand", false))
			{
				string cmdInpt;
				m_CommandInput.GetText(cmdInpt);
				cmdInpt.Trim();
				cmdInpt.Replace("\n", "");
				Print(cmdInpt);
				if (cmdInpt.Contains( "/clear" ))
				{
					ClearHistory();
					m_CommandInput.SetText("");
					return;
				}
				
				if (cmdInpt != string.Empty || cmdInpt != "")
				{
					//send off command 
					if (cmdInpt != "" && (cmdInpt.Substring(0,1)) == "/")
						GetRPCManager().VSendRPC( "RPC_MissionServer", "HandleChatCommand", new Param1<string>(cmdInpt), true);

					InsertHistory(cmdInpt);
				}
				m_CommandInput.SetText("");
			}
			
			if (m_PrevCommands.Count() > 0)
			{
				//Cycle thru history
				if (input.LocalPress("UAUPCommand", false))
				{
					SetFocus(null);
					index++;
					if (index > m_PrevCommands.Count() - 1)
						index = 0;
					
					m_CommandInput.SetText("");
					m_CommandInput.SetText(m_PrevCommands.Get(index));
				}
				
				if (input.LocalPress("UADOWNCommand", false))
				{
					SetFocus(null);
					index--;
					if (index < 0)
						index = m_PrevCommands.Count() - 1;
					
					m_CommandInput.SetText("");
					m_CommandInput.SetText(m_PrevCommands.Get(index));
				}
			}
		}
		
		//Suggest filter update
		if (m_Init && m_ScrollerSuggest)
		{
			if (m_CommandInput)
			{
				string searchText;
				m_CommandInput.GetText(searchText);
				if (searchText == string.Empty || searchText == "")
				{
					//Hide filter widget
					m_ScrollerSuggest.Show(false);
				}else{
					m_ScrollerSuggest.Show(true);
					UpdateFiler(searchText);
				}
			}
		}
	}
	
	void UpdateFiler(string input)
	{
		foreach(Widget filterW, string command: m_FilterWidgets)
		{
			if (filterW)
			{
				command.ToLower();
				input.ToLower();
				if (command.Contains(input))
					filterW.Show(true);
				else
					filterW.Show(false);
				
				m_ScrollerSuggest.Update();
				m_GridSuggest.Update();
			}
		}
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == m_CommandInput)
		{
			inputFocused = true;
			SetFocus(m_CommandInput);
			return true;
		}
		return false;
	}

    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
    	if (w == m_CommandInput)
		{
			inputFocused = false;
			SetFocus(null);
			return true;
		}
		return false;
    }
};