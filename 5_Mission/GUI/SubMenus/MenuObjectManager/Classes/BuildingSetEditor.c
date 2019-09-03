class BuildingSetEditor: ScriptedWidgetEventHandler
{
	private Widget 		   m_root;
	private ButtonWidget   m_Apply;
	private ButtonWidget   m_button_close;
	private CheckBoxWidget m_Active;
	private EditBoxWidget  m_SetName;
	private bool 		   m_EditMode;
	
	void BuildingSetEditor(Widget parent, string setName, bool active, bool editMode = false)
	{
		m_root   		= GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/ObjectManagerUI/EditDialog.layout", parent);
		m_Apply   		= ButtonWidget.Cast(m_root.FindAnyWidget("button_ok"));
		m_button_close  = ButtonWidget.Cast(m_root.FindAnyWidget("panel_close"));
		m_SetName 		= EditBoxWidget.Cast(m_root.FindAnyWidget("InputSetName"));
		m_Active        = CheckBoxWidget.Cast(m_root.FindAnyWidget("checkbox_active"));
		m_root.SetHandler(this);
		m_EditMode = editMode;
		
		m_SetName.SetText(setName);
		m_Active.SetChecked(active);
	}
	
	void ~BuildingSetEditor()
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
			autoptr MenuObjectManager objMenu = MenuObjectManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuObjectManager));
			if (objMenu)
			{
				if (!m_EditMode && objMenu.CheckDuplicateSet(m_SetName.GetText()) || m_SetName.GetText() == "" || m_SetName.GetText().Length() < 4){
					GetVPPUIManager().DisplayNotification("Error, set name does not meet requirements!\nduplicate name or name too short!");
					return false;
				}
				if (m_EditMode)
				{
					objMenu.UpdateBuildingSet(m_SetName.GetText(),m_Active.IsChecked());
					delete this;
				}else{
					objMenu.CreateNewSet(m_SetName.GetText(),m_Active.IsChecked());
					delete this;
				}
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