class BuildingAttributesEditor: ScriptedWidgetEventHandler
{
	private Widget 		   m_root;
	private ButtonWidget   m_Apply;
	private ButtonWidget   m_button_close;
	private CheckBoxWidget m_Active;
	private EditBoxWidget  m_Scale;
	ref 	SpawnedBuilding m_ToEdt;

	void BuildingAttributesEditor(SpawnedBuilding sp)
	{
		m_ToEdt         = sp;
		m_root   		= GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.EditDialogBuilding, null);
		m_Apply   		= ButtonWidget.Cast(m_root.FindAnyWidget("button_ok"));
		m_button_close  = ButtonWidget.Cast(m_root.FindAnyWidget("panel_close"));
		m_Active        = CheckBoxWidget.Cast(m_root.FindAnyWidget("checkbox_active"));
		m_Scale         = EditBoxWidget.Cast(m_root.FindAnyWidget("edit_scale"));
		m_root.SetHandler(this);
		m_root.SetSort(1023,true);

		m_Active.SetChecked(sp.GetActive());
		float scale = sp.GetScale();
		m_Scale.SetText(scale.ToString());
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
			//a scale outside the range keeps the window open
			string scaleText = m_Scale.GetText();
			scaleText = scaleText.Trim();
			float scale = scaleText.ToFloat();
			if (scale < VPPBuildingSetStore.SCALE_MIN || scale > VPPBuildingSetStore.SCALE_MAX)
			{
				GetVPPUIManager().DisplayError("#VSTR_CTX_RESULT_SCALE_RANGE");
				return true;
			}

			//Close and callback
			MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
			{
				m_ToEdt.SetActive(m_Active.IsChecked());
				m_ToEdt.SetScale(scale);
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
