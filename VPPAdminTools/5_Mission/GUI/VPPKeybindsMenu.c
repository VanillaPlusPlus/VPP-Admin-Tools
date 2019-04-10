class VPPKeybindsMenu extends UIScriptedMenu
{
	private Widget      		  m_dropDown_root;
	private EditBoxWidget         m_InputKeyBox;
	private ButtonWidget          m_BtnClear;
	private ButtonWidget          m_BtnApply;
	private ButtonWidget          m_BtnResetBinds;
	protected ref DropdownPrefab  m_KBDropdown;
	private string 				  m_CurrentAction;
	private bool 				  m_menuStatus;

	void VPPKeybindsMenu()
	{
		
	}

	void ~VPPKeybindsMenu()
	{

	}

	void setStatus(bool status)
	{
		m_menuStatus = status;
	}

	bool getStatus()
	{
		return m_menuStatus;
	}

	override Widget Init()
    {
    	layoutRoot = GetGame().GetWorkspace().CreateWidgets( "VPPAdminTools/GUI/Layouts/VPPKeybindsMenu.layout" );

    	m_InputKeyBox   = EditBoxWidget.Cast(layoutRoot.FindAnyWidget( "InputKeyBox" ));
    	m_BtnClear      = ButtonWidget.Cast(layoutRoot.FindAnyWidget( "BtnClear" ));
		m_BtnApply      = ButtonWidget.Cast(layoutRoot.FindAnyWidget( "BtnApply" ));
		m_BtnResetBinds = ButtonWidget.Cast(layoutRoot.FindAnyWidget( "BtnResetBinds" ));

    	m_dropDown_root   = Widget.Cast(layoutRoot.FindAnyWidget( "DropDownPanel" ));
		m_KBDropdown	  = new DropdownPrefab( m_dropDown_root, "Select a KeyBind:" );
		m_KBDropdown.m_OnSelectItem.Insert( OnSelectDropDownItem );
	    m_KBDropdown.AddElement( "Admin Tools" );
	    m_KBDropdown.AddElement( "TP To Crosshair" );
	    m_KBDropdown.AddElement( "Delete On Crosshair" );
	    m_KBDropdown.AddElement( "GodMode Toggle" );

    	return layoutRoot;
    }

    void OnSelectDropDownItem( int index )
	{
		string keytxt;
		switch(index)
		{
			case 0:
			m_KBDropdown.SetText( "Admin Tools" );
			keytxt = g_Game.getKeyBindsConfig().getActionKey("AdminTools");
			keytxt.Replace("KC_", "");
			m_InputKeyBox.SetText(keytxt);
			m_CurrentAction = "AdminTools";
			break;

		
			case 1:
			m_KBDropdown.SetText( "TP To Crosshair" );
			keytxt = g_Game.getKeyBindsConfig().getActionKey("CrosshairTP");
			keytxt.Replace("KC_", "");
			m_InputKeyBox.SetText(keytxt);
			m_CurrentAction = "CrosshairTP";
			break;
			
			case 2:
			m_KBDropdown.SetText( "Delete On Crosshair" );
			keytxt = g_Game.getKeyBindsConfig().getActionKey("CrosshairDelete");
			keytxt.Replace("KC_", "");
			m_InputKeyBox.SetText(keytxt);
			m_CurrentAction = "CrosshairDelete";
			break;

			case 3:
			m_KBDropdown.SetText( "GodMode Toggle" );
			keytxt = g_Game.getKeyBindsConfig().getActionKey("GodModeToggle");
			keytxt.Replace("KC_", "");
			m_InputKeyBox.SetText(keytxt);
			m_CurrentAction = "GodModeToggle";
			break;
		}
		m_KBDropdown.Close();
	}

	//-----------------------
	override bool OnClick( Widget w, int x, int y, int button )
	{
		if (w == m_BtnClear)
		{
			m_InputKeyBox.SetText("");
			m_InputKeyBox.Enable(true);
			return true;
		}

		if (w == m_BtnResetBinds)
		{
			g_Game.getKeyBindsConfig().makeDefaults();
			OnSelectDropDownItem(0);
			return true;
		}

		if (w == m_BtnApply)
		{
			Close();
			return true;
		}

		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		return true;
	}

	void KeyPressReg(int key)
	{
		ref map<int,string> keysMap = g_Game.getKeyBindsMap();
		if (keysMap.Contains(key))
		{
			string keyBindType = keysMap.Get( key );
			g_Game.getKeyBindsConfig().setActionKey(keyBindType,m_CurrentAction);
			keyBindType.Replace("KC_", "");
			m_InputKeyBox.SetText( keyBindType );
		}
	}
}