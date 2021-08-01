class PopUpNewPositionEditor extends ScriptedWidgetEventHandler
{
	private ref MenuTeleportManager m_RootClass;
	private Widget 		 			m_root;
	private ButtonWidget 			m_Close;
	private ButtonWidget 			m_Cancel;
	private ButtonWidget 			m_Save;
	private EditBoxWidget           m_editbox_name;
	private EditBoxWidget           m_edit_pos;
	private vector					m_Position;
	private string                  m_PositionName;
	private vector					m_OldPosition;
	private string                  m_OldPositionName;
	private bool 					m_editMode;
	
	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
		
		m_Close  = ButtonWidget.Cast(m_root.FindAnyWidget("button_close"));
		m_Cancel = ButtonWidget.Cast(m_root.FindAnyWidget("button_cancel"));
		m_Save   = ButtonWidget.Cast(m_root.FindAnyWidget("button_save"));
		m_editbox_name   = EditBoxWidget.Cast(m_root.FindAnyWidget("editbox_name"));
		m_edit_pos       = EditBoxWidget.Cast(m_root.FindAnyWidget("edit_pos"));
		
		m_root.SetSort(1024,true);
	}
	
	void ~PopUpNewPositionEditor()
	{
		if (m_root != null)
			m_root.Unlink();
	}
	
	void Init(MenuTeleportManager rootClass, vector pos, bool editMode = false, string oldName = "")
	{
		m_RootClass = rootClass;
		m_Position  = pos;
		m_editMode  = editMode;
		m_edit_pos.SetText(g_Game.VectorToString(pos));
		
		if (editMode){
			m_editbox_name.SetText(oldName);
			m_OldPositionName = oldName;
			m_OldPosition     = pos;
		}
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_edit_pos)
		{
			string input = m_edit_pos.GetText();
			input.Replace(" ",","); //incase input has no ',' instead has space
			m_Position = g_Game.StringToVector(input);
			return true;
		}
		
		if (w == m_editbox_name)
		{
			m_PositionName = m_editbox_name.GetText();
			return true;
		}
			
		return false;
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		switch(w)
		{
			case m_Close:
			delete this;
			break;
			
			case m_Cancel:
			delete this;
			break;
			
			case m_Save:
			if (m_PositionName != "" && !m_RootClass.CheckDuplicate(m_PositionName))
			{
				if (m_Position != Vector(0,0,0))
					m_RootClass.SaveNewMarker(m_PositionName,m_Position,m_editMode,m_OldPositionName,m_OldPosition);
					else
					GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_INVALIDPOS");
			}else{
				GetVPPUIManager().DisplayError("#VSTR_NOTIFY_ERR_TP_NAME_UNIQUE");
			}
			break;
		}
		return true;
	}
}