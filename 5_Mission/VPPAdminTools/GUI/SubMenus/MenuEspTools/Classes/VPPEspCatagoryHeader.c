class VPPEspCatagoryHeader: ScriptedWidgetEventHandler
{
	private ref Widget m_root;
	private ImageWidget m_IconCollapse;
	private ButtonWidget m_BtnCollapse;
	private RichTextWidget m_CatagoryTitle;
	private GridSpacerWidget m_SpacerGrid; //children content grid
	private ButtonWidget m_BtnClose;
	private VPPESPTracker m_TrackerBase;
	private bool m_Collapsed;
	private bool m_RootCatagory;

	void VPPEspCatagoryHeader()
	{
	}
	
	void Destroy()
	{
		if (m_root){
			m_root.Unlink();
		}
	}

	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
		m_IconCollapse  = ImageWidget.Cast(w.FindAnyWidget("IconCollapse"));
		if (m_IconCollapse)
		{
			m_IconCollapse.LoadImageFile(0, "set:vpp_icons image:chevron_down"); //collapsed
			m_IconCollapse.LoadImageFile(1, "set:vpp_icons image:chevron_up");   //expanded
		}
		m_CatagoryTitle = RichTextWidget.Cast(w.FindAnyWidget("CatagoryTitle"));
		m_BtnCollapse   = ButtonWidget.Cast(w.FindAnyWidget("BtnCollapse"));
		m_BtnClose   = ButtonWidget.Cast(w.FindAnyWidget("BtnClose"));
	}

	void BuildCatagory(string layoutPath, string title, bool hideDefault = true)
	{
		if (layoutPath == string.Empty)
			return;

		m_CatagoryTitle.SetText(title);
		GridSpacerWidget container = GridSpacerWidget.Cast(m_root.GetParent());
		m_SpacerGrid = GridSpacerWidget.Cast(GetGame().GetWorkspace().CreateWidgets(layoutPath, container));
		m_SpacerGrid.Show(!hideDefault);
		if (!hideDefault)
			m_IconCollapse.SetImage(1); //expanded -> chevron_up
	}

	void SetRootCatagory(bool state, VPPESPTracker root)
	{
		m_RootCatagory = state;
		m_TrackerBase = root;
		m_BtnClose.Show(state);
	}
	
	void ChangeState()
	{
		m_Collapsed = !m_Collapsed;
		if (m_Collapsed){
			m_IconCollapse.SetImage(1); //expanded -> chevron_up
		}else{
			m_IconCollapse.SetImage(0); //collapsed -> chevron_down
		}
		m_SpacerGrid.Show(m_Collapsed);
	}

	bool IsExpanded()
	{
		return m_Collapsed; //m_Collapsed == true means the section is shown/expanded
	}

	//set a specific expanded state directly (used by distance auto-collapse)
	void SetExpanded(bool state)
	{
		m_Collapsed = state;
		if (m_IconCollapse)
		{
			if (state)
				m_IconCollapse.SetImage(1); //chevron_up
			else
				m_IconCollapse.SetImage(0); //chevron_down
		}
		if (m_SpacerGrid)
			m_SpacerGrid.Show(state);
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnCollapse){
			ChangeState();
			return true;
		}
		if (w == m_BtnClose && m_RootCatagory && m_TrackerBase){
			
			EspToolsMenu espMenu = EspToolsMenu.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(EspToolsMenu));
			if (espMenu){
				espMenu.RemoveTracker(m_TrackerBase);
				delete m_TrackerBase;
			}
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
};