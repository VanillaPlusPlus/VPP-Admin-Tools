/*
 Custom Tooltip window handler
*/
class ToolTipHandler extends ScriptedWidgetEventHandler
{
	protected Widget     	 m_root;
	protected ref Widget     m_ToolTipRoot;
	protected TextWidget 	 m_TitleWidget;
	protected MultilineTextWidget m_ContentWidget;
	
	protected string     m_TitleText;
	protected string     m_ToolTipText;
	protected bool       m_SnapLeft;
	
	
	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
		
		m_TitleText   = "Test Title";
		m_ToolTipText = "This is a tool tip.";
		
		m_ToolTipRoot   = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPInfoBox, GetVPPUIManager().GetMenuByType(VPPAdminHud).layoutRoot);
		m_TitleWidget   = TextWidget.Cast(m_ToolTipRoot.FindAnyWidget("Title"));
		m_ContentWidget = MultilineTextWidget.Cast(m_ToolTipRoot.FindAnyWidget("ContentText"));
		
		m_TitleWidget.SetText(m_TitleText);
		m_ContentWidget.SetText(m_ToolTipText);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w == m_root)
		{
			int m_x;
			int m_y;
			g_Game.GetMousePos(m_x, m_y);
			if (m_SnapLeft)
			{
				float width,height;
				m_ContentWidget.GetSize(width,height);
				m_ToolTipRoot.SetPos(m_x - ( 400 * width ),m_y);
			}else{
				m_ToolTipRoot.SetPos(m_x,m_y);
			}
			m_ToolTipRoot.Show(true);
			m_ToolTipRoot.Update();
			m_TitleWidget.Update();
			m_ContentWidget.Update();
			return true;
		}
		return false;
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w == m_root)
		{
			m_ToolTipRoot.Show(false);
			return true;
		}
		return false;
	}
	
	void SetTitle(string title)
	{
		m_TitleText = title;
		m_TitleWidget.SetText(m_TitleText);
		m_TitleWidget.Update();
	}
	
	void SetContentText(string text)
	{
		m_ToolTipText = text;
		m_ContentWidget.SetText(m_ToolTipText);
		m_ContentWidget.Update();
	}
	
	void SetLeftSide(bool state)
	{
		//For the main icons panel
		m_SnapLeft = state;
	}
};