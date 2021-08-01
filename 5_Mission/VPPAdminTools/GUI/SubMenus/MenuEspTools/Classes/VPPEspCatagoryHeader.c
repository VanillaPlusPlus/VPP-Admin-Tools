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
			m_IconCollapse.LoadImageFile(0, "set:dayz_gui image:icon_collapse");
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
			m_IconCollapse.LoadImageFile(0, "set:dayz_gui image:icon_collapse");
		}else{
			m_IconCollapse.LoadImageFile(0, "set:dayz_gui image:icon_expand");
		}
		m_SpacerGrid.Show(m_Collapsed);
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