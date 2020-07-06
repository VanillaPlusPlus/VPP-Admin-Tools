class WebHookEntry: ScriptedWidgetEventHandler
{
	private ref Widget 	 	  m_Root;
	private ButtonWidget 	  m_btnDeleteItem;
	private ButtonWidget 	  m_btnEditItem;
	private TextWidget   	  m_ItemName;
	private string 		 	  m_webHookName;

	void WebHookEntry(ref GridSpacerWidget parent, string webHookName)
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets("VPPAdminTools/GUI/Layouts/WebHooksUI/WebHookEntry.layout", parent);
		m_btnDeleteItem = ButtonWidget.Cast( m_Root.FindAnyWidget( "btnDeleteItem") );
		m_btnEditItem   = ButtonWidget.Cast( m_Root.FindAnyWidget( "btnEditItem") );
		autoptr Widget rootPr = parent.GetParent().GetParent().GetParent().GetParent();
		GetVPPUIManager().HookConfirmationDialog(m_btnDeleteItem, rootPr, this, "DeleteWebHook", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_WBHOOK_DEL", "#VSTR_TOOLTIP_WBHOOK_DEL " + webHookName);
		GetVPPUIManager().HookConfirmationDialog(m_btnEditItem, rootPr, this, "EditWebHook", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_WBHOOK_SELC", "#VSTR_TOOLTIP_WBHOOK_SELC");

		m_webHookName   = webHookName;
		m_ItemName 		= TextWidget.Cast( m_Root.FindAnyWidget( "ItemName") );
		m_ItemName.SetText( webHookName );
		m_Root.SetHandler(this);
		parent.Update();
	}

	void ~WebHookEntry()
	{
		if (m_Root)
			m_Root.Unlink();
	}

	void DeleteWebHook(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			autoptr MenuWebHooks hooksMangr = MenuWebHooks.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuWebHooks));
			if (hooksMangr)
			{
				hooksMangr.DeleteWebHook( this );
			}
		}
	}

	//Just for confirmation display
	void EditWebHook(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			SetSelected();
		}
	}

	string GetName()
	{
		return m_webHookName;
	}

	void SetSelected()
	{
		autoptr MenuWebHooks hooksMangr = MenuWebHooks.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuWebHooks));
		if (hooksMangr)
		{
			hooksMangr.SetSelected( this );
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_btnEditItem)
		{
			SetSelected();
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
};