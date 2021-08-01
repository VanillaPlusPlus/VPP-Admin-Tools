class VPPButton: ScriptedWidgetEventHandler
{
	ButtonWidget Button;
	string 		 ButtonType;
	string 	     ButtonName;
	string 	     ButtonIcon;
	string       ButtonDescription;
	private      Widget m_Root; //root
	private bool m_PermissionActive;
	
	void VPPButton(Widget btn, string btnType, string name, string icon, string description)
	{
		VPPAdminHud.m_OnPermissionsChanged.Insert(this.OnPermissionChange);

		m_Root    		  = btn;
		Button            = ButtonWidget.Cast(btn.FindAnyWidget("Button"));
		ButtonType		  = btnType;
		ButtonName        = name;
		ButtonIcon        = icon;
		ButtonDescription = description;
		ImageWidget.Cast(btn.FindAnyWidget("Image")).LoadImageFile(0, icon);
		TextWidget.Cast(btn.FindAnyWidget("ButtonName")).SetText(name);

		ToolTipHandler toolTip;
		Button.GetScript(toolTip);
		toolTip.SetTitle(name+":");
		toolTip.SetContentText(description);
		toolTip.SetLeftSide(true);

		btn.SetHandler(this);
		btn.Show(false);
	}

	void OnWidgetScriptInit(Widget w)
	{
		w.SetHandler(this);
	}

	void OnPermissionChange(map<string,bool> permissions)
	{
		if (!permissions || permissions.Count() <= 0)
			return;

		permissions.Find(ButtonType, m_PermissionActive);
		m_Root.Show(m_PermissionActive);
		DestroySubMenu(); //close up menu related to this button
	}

	void DestroySubMenu()
	{
		VPPAdminHud root = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
			if (!root)
				return;

		AdminHudSubMenu menu = root.GetSubMenuByType(ButtonType.ToType());
		if (menu && !m_PermissionActive)
		{
			menu.HideSubMenu();
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == Button)
		{
			VPPAdminHud root = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
			if (!root)
				return false;

			typename btnType = ButtonType.ToType();
			AdminHudSubMenu menu = root.GetSubMenuByType(btnType);
			if (menu)
			{
				if (menu.IsSubMenuVisible())
				{
					menu.HideSubMenu();
				}else if (m_PermissionActive){
					menu.ShowSubMenu();
				}
				return true;
			}

			if (m_PermissionActive){
				root.CreateSubMenu(btnType);
			}
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
	
	void ~VPPButton()
	{
		if (m_Root != null)
			m_Root.Unlink();

		VPPAdminHud.m_OnPermissionsChanged.Remove(this.OnPermissionChange);
	}
};