class PopUpCreatePreset extends ScriptedWidgetEventHandler
{
	private ref MenuItemManager 	m_RootClass;
	private Widget 		 			m_root;
	private ButtonWidget 			m_Close;
	private ButtonWidget 			m_Cancel;
	private ButtonWidget 			m_Save;
	private EditBoxWidget           m_editbox_name;
	private string                  m_PresetName;
	
	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
		
		m_Close  = ButtonWidget.Cast(m_root.FindAnyWidget("button_close"));
		m_Cancel = ButtonWidget.Cast(m_root.FindAnyWidget("button_cancel"));
		m_Save   = ButtonWidget.Cast(m_root.FindAnyWidget("button_save"));
		m_editbox_name   = EditBoxWidget.Cast(m_root.FindAnyWidget("editbox_name"));
	}
	
	void ~PopUpCreatePreset()
	{
		if (m_root != null)
			m_root.Unlink();
	}
	
	void Init(MenuItemManager rootClass)
	{
		m_RootClass = rootClass;
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_editbox_name)
		{
			m_PresetName = m_editbox_name.GetText();
			return true;
		}
		return false;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_Close:
			case m_Cancel:
			delete this;
			break;
			
			case m_Save:
			if (m_PresetName != "" && !m_RootClass.CheckDuplicatePreset(m_PresetName))
			{
				m_RootClass.SaveNewPreset(m_PresetName);
				delete this;
			}else{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_UNIQUE");
			}
			break;
		}
		return true;
	}
}