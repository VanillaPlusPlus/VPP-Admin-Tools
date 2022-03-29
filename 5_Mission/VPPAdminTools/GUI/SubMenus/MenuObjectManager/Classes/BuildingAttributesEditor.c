class BuildingAttributesEditor: ScriptedWidgetEventHandler
{
	private Widget 		   m_root;
	private ButtonWidget   m_Apply;
	private ButtonWidget   m_button_close;
	private CheckBoxWidget m_Active;
	ref 	SpawnedBuilding m_ToEdt;
	
	void BuildingAttributesEditor(SpawnedBuilding sp)
	{
		m_ToEdt         = sp;
		m_root   		= GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.EditDialogBuilding, null);
		m_Apply   		= ButtonWidget.Cast(m_root.FindAnyWidget("button_ok"));
		m_button_close  = ButtonWidget.Cast(m_root.FindAnyWidget("panel_close"));
		m_Active        = CheckBoxWidget.Cast(m_root.FindAnyWidget("checkbox_active"));
		m_root.SetHandler(this);
		m_root.SetSort(1023,true);
		
		m_Active.SetChecked(sp.GetActive());
	}
	
	void ~BuildingAttributesEditor()
	{
		if (m_root)
			m_root.Unlink();
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		if (w == m_Apply)
		{
			//Close and callback
			MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
			{
				m_ToEdt.SetActive(m_Active.IsChecked());
				objMenu.UpdateBuildingAttributes(m_ToEdt);
				delete this;
			}
			return true;
		}
		
		if (w == m_button_close)
		{
			delete this;
			return true;
		}
		return false;
	}
};