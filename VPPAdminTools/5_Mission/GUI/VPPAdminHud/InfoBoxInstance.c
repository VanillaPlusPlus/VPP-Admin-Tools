class InfoBoxInstance extends UIScriptedMenu
{
	private Widget          m_Root;    
	private Widget 			m_InfoPanelRoot;
	private TextWidget		m_InfoLabel;
	private RichTextWidget	m_InfoText;
	private ImageWidget     m_InfoImage;

	private string m_InnerText;
	private string m_TitleText;

	void InfoBoxInstance(Widget RootWidget, string TargetWidgetName, string TitleText, string InnerText)
	{
		//m_Root			= GetGame().GetWorkspace().CreateWidgets( "gui/layouts/DZTest/VPPInfoBox.layout", NULL );
		m_Root			= GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/VPPInfoBox.layout", NULL );

		m_InfoPanelRoot = Widget.Cast( m_Root.FindAnyWidget( "Info_Panel" ) );
		m_InfoLabel	    = TextWidget.Cast( m_InfoPanelRoot.FindAnyWidget( "LabelText"  ) );
		m_InfoText	    = RichTextWidget.Cast( m_InfoPanelRoot.FindAnyWidget( "Info_Content" ) );

		m_InnerText     = InnerText;
		m_TitleText     = TitleText;

		m_InfoImage     = ImageWidget.Cast( RootWidget.FindAnyWidget( TargetWidgetName ) );
    	m_InfoImage.LoadImageFile(0,"set:ccgui_enforce image:FilterCurrentObjectivesFocus");

    	WidgetEventHandler.GetInstance().RegisterOnMouseEnter( m_InfoImage,  this, "MouseEnter" );
    	WidgetEventHandler.GetInstance().RegisterOnMouseLeave( m_InfoImage,  this, "MouseLeave" );
	}

	void ~InfoBoxInstance()
	{

	}

	void SetTitle(string NewTitle)
	{
		m_TitleText = NewTitle;
	}

	string GetTitle()
	{
		return m_TitleText;
	}

	void SetInnerText(string NewInnerText)
	{
		m_InnerText = NewInnerText;
	}

	string GetInnerText()
	{
		return m_InnerText;
	}

	bool MouseEnter( Widget w, int x, int y )
	{
		if (w == m_InfoImage)
		{
			int m_x;
			int m_y;
			g_Game.GetMousePos(m_x, m_y);
	    	m_Root.SetPos(m_x,m_y);

			m_InfoPanelRoot.Show(true);
			m_InfoLabel.SetText( m_TitleText );
			m_InfoText.SetText( m_InnerText );

			m_Root.Update();
			m_InfoPanelRoot.Update();
			m_InfoLabel.Update();
			m_InfoText.Update();
			return true;
		}
		return false;
	}
	
	bool MouseLeave( Widget w, Widget s, int x, int y)
	{
		if (w == m_InfoImage) m_InfoPanelRoot.Show(false); return true;
		return false;
	}
}