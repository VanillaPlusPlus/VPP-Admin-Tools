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
		m_CmdTooltips.Insert( "/sph", new Param2<string,string>("spawn an item into hands", "<class name>") );
		m_CmdTooltips.Insert( "/spg", new Param2<string,string>("spawn an item on ground", "<class name>") );
		m_CmdTooltips.Insert( "/spawncar", new Param2<string,string>("spawn a civilian sedan nearby", "") );
		m_CmdTooltips.Insert( "/refuel", new Param2<string,string>("refuel & repair vehicle you are in", "") );
		m_CmdTooltips.Insert( "/spi", new Param2<string,string>("spawn an item in your inventory", "<class name>") );
		m_CmdTooltips.Insert( "/ammo", new Param2<string,string>("repair and reload weapon in hand", "") );
		m_CmdTooltips.Insert( "/tpp", new Param2<string,string>("teleport to coordinates", "<7500,0,7500>") );
		m_CmdTooltips.Insert( "/tpt", new Param2<string,string>("teleport to a position saved in your presets", "<PresetName>") );
		m_CmdTooltips.Insert( "/goto", new Param2<string,string>("teleport to a player by name", "<PlayerName>") );
		m_CmdTooltips.Insert( "/return", new Param2<string,string>("teleport player by name to previously cached position", "<PlayerName>") );
		m_CmdTooltips.Insert( "/bring", new Param2<string,string>("teleport player by name to you", "<PlayerName>") );
		m_CmdTooltips.Insert( "/heal", new Param2<string,string>("heal player by name or self", "<PlayerName> OR self") );
		m_CmdTooltips.Insert( "/kill", new Param2<string,string>("kill player by name or self", "<PlayerName> OR self") );
		m_CmdTooltips.Insert( "/strip", new Param2<string,string>("strip player gear by name or self", "<PlayerName> OR self") );
		m_CmdTooltips.Insert( "/clear", new Param2<string,string>("clears commands history", "") );
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/CommandsConsoleUI/CommandsConsoleUI.layout");
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
		autoptr ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("Information:");
		toolTip.SetContentText("Arrow key UP/DOWN to cycle between previously executed commands");
		
		
		foreach(string command, ref Param2<string,string> params: m_CmdTooltips)
		{
			ref Widget sgElement = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/CommandsConsoleUI/TextElement.layout", m_GridSuggest);
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
		
		ref Widget historyWidget = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/CommandsConsoleUI/TextElement.layout", m_GridFeed);
		TextWidget.Cast(historyWidget.FindAnyWidget("Message")).SetText(input);
		m_HistoryWidgets.Insert(historyWidget);
		m_GridFeed.Update();
		m_ScrollerFeed.VScrollToPos(m_ScrollerFeed.GetContentHeight() * 1000);
		m_ScrollerFeed.Update();
	}
	
	void ClearHistory()
	{
		foreach(ref Widget w: m_HistoryWidgets)
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
						GetRPCManager().SendRPC( "RPC_MissionServer", "HandleChatCommand", new Param1<string>(cmdInpt), true);

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
		foreach(ref Widget filterW, string command: m_FilterWidgets)
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