/*
	Design-system helper: collapsible section.

	Binds a header (button/panel), a content widget and an optional chevron
	ImageWidget. Clicking the header toggles the content; parent spacers with
	"Ignore invisible" enabled will reflow the remaining sections.

	Chevron image slots (image0 = expanded/up, image1 = collapsed/down) are loaded
	here via LoadImageFile - declaring image1 in the .layout does NOT register the
	second slot, so layouts only carry image0 for the initial look.
*/
class VPPCollapsibleSection
{
	protected Widget 	  m_Root;
	protected Widget 	  m_Header;
	protected Widget 	  m_Content;
	protected ImageWidget m_Chevron;
	protected bool 		  m_Expanded;

	void VPPCollapsibleSection(Widget root, string headerName, string contentName, string chevronName = "", bool startExpanded = true)
	{
		m_Root 	   = root;
		m_Header   = root.FindAnyWidget(headerName);
		m_Content  = root.FindAnyWidget(contentName);
		m_Expanded = startExpanded;

		if (chevronName != "")
		{
			m_Chevron = ImageWidget.Cast(root.FindAnyWidget(chevronName));
			m_Chevron.LoadImageFile(0, "set:vpp_icons image:chevron_up");
			m_Chevron.LoadImageFile(1, "set:vpp_icons image:chevron_down");
		}

		if (m_Header)
			WidgetEventHandler.GetInstance().RegisterOnMouseButtonDown(m_Header, this, "OnHeaderClick");

		Apply();
	}

	void OnHeaderClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
			return;

		SetExpanded(!m_Expanded);
	}

	void SetExpanded(bool state)
	{
		m_Expanded = state;
		Apply();
	}

	bool IsExpanded()
	{
		return m_Expanded;
	}

	protected void Apply()
	{
		if (m_Content)
		{
			m_Content.Show(m_Expanded);
			Widget parent = m_Content.GetParent();
			if (parent)
				parent.Update();
		}

		if (m_Chevron)
		{
			if (m_Expanded)
				m_Chevron.SetImage(0);
			else
				m_Chevron.SetImage(1);
		}

		if (m_Root)
			m_Root.Update();
	}
};
