class PermissionBox : VPPPlayerTemplate
{
	private CheckBoxWidget m_PermCheckBox;
	private string 		   m_PermName;
	void PermissionBox(GridSpacerWidget grid, string name)
	{
		m_LayoutPath   = VPPATUIConstants.VPPPermissionBox;
		m_EntryBox     = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_PermCheckBox = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("CheckBox"));
		m_PermName     = name;
		m_PermCheckBox.SetText(name);
	}
	
	void ~PermissionBox()
	{
		if (m_EntryBox != null)
		m_EntryBox.Unlink();
	}
	
	void SetChecked(bool state)
	{
		m_PermCheckBox.SetChecked(state);
	}
	
	bool IsChecked()
	{
		return m_PermCheckBox.IsChecked();
	}
	
	string GetName()
	{
		return m_PermName;
	}
}