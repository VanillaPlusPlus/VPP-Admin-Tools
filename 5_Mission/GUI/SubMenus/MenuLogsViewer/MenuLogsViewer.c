class MenuLogsViewer extends AdminHudSubMenu
{
	private HtmlWidget 			   m_HtmlWidget;
	private ButtonWidget 		   m_btnDeleteSelected;
	private ButtonWidget           m_BtnRefreshLogList;
	private CanvasWidget           m_Canvas;
	private bool 				   m_loaded = false;
	
	protected TextWidget   m_page;
	protected ButtonWidget m_BtnNextPG;
	protected ButtonWidget m_BtnPrevPG;
	protected float 	   m_page_height;
	protected float 	   m_content_total_height;
	protected float 	   m_content_pos;
	
	void MenuLogsViewer()
	{
		//------RPCs-------
		GetRPCManager().AddRPC( "RPC_MenuLogsViewer", "SortLogsList", this);
		//-----------------
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);

		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/LogsViewerUI/MenuLogsViewer.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );

		m_HtmlWidget  	 = HtmlWidget.Cast(M_SUB_WIDGET.FindAnyWidget("HtmlWidget"));
		m_BtnNextPG      = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnNextPG" ) );
		m_BtnPrevPG   	 = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnPrevPG" ) );
		m_page           = TextWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PageCounter" ) );
		
		m_Canvas	  = CanvasWidget.Cast(M_SUB_WIDGET.FindAnyWidget("Canvas"));
		
		m_btnDeleteSelected = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("btnDeleteSelected"));
		GetVPPUIManager().HookConfirmationDialog(m_btnDeleteSelected, M_SUB_WIDGET,this,"DeleteLog", DIAGTYPE.DIAG_YESNO, "Delete Log?", "Are you sure you wish to delete selected log files? Note: log file that is used during runtime will not be deleted!");
		m_BtnRefreshLogList = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnRefreshLogList"));
		
		float width;
		m_HtmlWidget.GetScreenSize(width, m_page_height);
		m_loaded = true;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return; //Update cancels if sub menu is not visible.
		
		//DrawLine(float x1, float y1, float x2, float y2, float width, int color);
		//m_Canvas.DrawLine(Math.RandomInt(1, 100),Math.RandomInt(1,100),Math.RandomInt(1,100),Math.RandomInt(1,100),3,ARGB(255,255,0,0));
		//m_Canvas.Update();
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_BtnRefreshLogList:
			//TODO: Reload Script List
			break;
			
			case m_BtnNextPG:
			NextPrevPage(true);
			break;

			case m_BtnPrevPG:
			NextPrevPage(false);
			break;
		}
		return false;
	}
	
	void DeleteLog(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			//TODO: Remove selected Log
		}
	}
	
	void LoadConent(ref array<string> m_data)
	{
		ref array<string> m_cache = new array<string>;
		string Final = "";
		for (int i = 0; i < m_data.Count(); ++i)
	    {
	    	Final += m_data.Get(i);
	    	if (Final.Length() >= 1028)
	    	{
	    		m_cache.Insert(Final);
	    		Final = "";
	    	}
	    }
		
	    switch(m_cache.Count())
	    {
	    	case 1:
	    	m_HtmlWidget.SetText(m_cache.Get(0) + Final);
	    	break;

	    	case 2:
	    	m_HtmlWidget.SetText(m_cache.Get(0) + m_cache.Get(1) + Final);
	    	break;

	    	case 3:
	    	m_HtmlWidget.SetText(m_cache.Get(0) + m_cache.Get(1) + m_cache.Get(2) + Final);
	    	break;

	    	case 4:
	    	m_HtmlWidget.SetText(m_cache.Get(0) + m_cache.Get(1) + m_cache.Get(2) + m_cache.Get(3) + Final);
	    	break;

	    	case 5:
	    	m_HtmlWidget.SetText(m_cache.Get(0) + m_cache.Get(1) + m_cache.Get(2) + m_cache.Get(3) + m_cache.Get(4) + Final);
	    	break;

	    	default : {
	    		m_HtmlWidget.SetText(Final);
	    	}
	    }

		m_content_total_height = m_HtmlWidget.GetContentHeight();
		m_content_pos = 0;
		NextPrevPage(false);
	}
	
	void NextPrevPage(bool next)
	{
		if (next)
			m_content_pos	= m_content_pos + m_page_height;
		else
			m_content_pos	= m_content_pos - m_page_height;
		
		float maxOffset = 0;
		if (m_content_total_height > m_page_height)
		{
			maxOffset = m_content_total_height - m_page_height;
		}		
		
		if (m_content_pos < 0)
		{
			m_content_pos = 0;
		}

		if (m_content_pos > maxOffset)
		{
			m_content_pos = maxOffset;
		}
		
		m_HtmlWidget.SetContentOffset(m_content_pos, true);
		
		float pagesTotal = Math.Ceil(m_content_total_height / m_page_height);
		float currPage = Math.Round(m_content_pos / m_page_height) + 1;

		m_page.SetText( currPage.ToString() + " / " + pagesTotal.ToString() );
	}
	
	void SortLogsList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if (!ctx.Read(data)) return;
		
		if ( type == CallType.Client )
		{
			ref array<string> temp = data.param1;
			temp.Copy(data.param1);
			//m_HtmlWidget.SetText("");
			/*
			FileHandle tmp = OpenFile("$profile:tmptext.html", FileMode.WRITE);

			FPrintln(tmp,"<p><span style='color: #ff0000;'>");
			foreach(string line : temp)
			{
				FPrintln(tmp,line);
			}
			FPrintln(tmp,"</span></p>");
			
			CloseFile(tmp);
			*/
			LoadConent(temp);
		}
	}
};