/*
 Confirmation window handler
*/
class ConfirmationEventHandler extends ScriptedWidgetEventHandler
{
	protected Widget     	   m_root;
	protected Class 		   m_CallBackClass;
	protected string   	 	   m_CallbackFunc;
	protected ref VPPDialogBox m_diagBox;
	
	protected int     		   m_DiagType;
	protected string           m_Title;
	protected string           m_Message;
	protected ref Widget       m_Parent;
	protected bool             m_allowChars;
	
	void OnWidgetScriptInit(Widget w)
	{
		m_root = w;
		m_root.SetHandler(this);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_root)
		{
			if (m_CallBackClass.ClassName() == "MenuServerManager")
			{
				//Disable mapwidget (stupid bug with mapwidget)
				autoptr MenuServerManager rootClass;
				if (Class.CastTo(rootClass,m_CallBackClass))
				{
					rootClass.ToggleMapWidget(false);
				}
			}
			
			if (m_CallBackClass.ClassName() == "UserBoxTemplate" || m_CallBackClass.ClassName() == "UserGroupTemplate" || m_CallBackClass.ClassName() == "MenuPermissionsEditor")
			{
				//Disable scroll widgets (stupid bug with scroll widget)
				autoptr VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
				MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).HideScrollWidgets(true);
			}
			

			m_diagBox = GetVPPUIManager().CreateDialogBox(m_Parent);
			m_diagBox.InitDiagBox(m_DiagType,m_Title,m_Message,this);
			if (m_allowChars) m_diagBox.AllowCharInput();
			return true;
		}
		return false;
	}
	
	void OnDiagResult(int outcome, string input)
	{
		GetGame().GameScript.CallFunctionParams( m_CallBackClass, m_CallbackFunc, null, new Param2<int,string>(outcome, input) );
	}

	void InitEvent(Class callbackClass, string functionName, int diagtype, string title, string message, Widget parent, bool allowChars = false)
	{
		m_CallBackClass  = callbackClass;
		m_CallbackFunc   = functionName;
		m_DiagType  	 = diagtype;
		m_Parent	 	 = parent;
		m_Title		 	 = title;
		m_Message		 = message;
		m_allowChars	 = allowChars;
	}
};