class UserGroupTemplate : VPPPlayerTemplate
{
	private ref array<ref UserBoxTemplate> m_UserBoxes;
	private ref UserGroup 	 m_group;
	private TextWidget 		 m_GroupName;
	private TextWidget 		 m_TotalMembers;
	private TextWidget 		 m_PermsLevel;
	private ButtonWidget     m_btnEditPermLevel;
	private ButtonWidget     m_btnSettings;
	private CheckBoxWidget   m_SelectGroup;
	private GridSpacerWidget m_GridSpacer;
	private ScrollWidget     m_ScrollWidget;

	void UserGroupTemplate(GridSpacerWidget grid, UserGroup group, Widget RootWidget)
	{
		m_LayoutPath 	   = VPPATUIConstants.VPPUserGroupBox;
		m_EntryBox 		   = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_GroupName 	   = TextWidget.Cast(m_EntryBox.FindAnyWidget("GroupName"));
		m_GridSpacer 	   = GridSpacerWidget.Cast(m_EntryBox.FindAnyWidget("SpacerMembersList"));
		m_TotalMembers     = TextWidget.Cast(m_EntryBox.FindAnyWidget("MemberCount"));
		m_PermsLevel  	   = TextWidget.Cast(m_EntryBox.FindAnyWidget("PermLevel"));
		m_SelectGroup 	   = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("chkBoxSelectGrp"));
		m_ScrollWidget 	   = ScrollWidget.Cast(m_EntryBox.FindAnyWidget("ScrollerUserGroups"));
		m_btnEditPermLevel = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnEditPermLevel"));
		GetVPPUIManager().HookConfirmationDialog(m_btnEditPermLevel, RootWidget,this,"UpdatePermissionLevel", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TITLE_EDIT_PERMS", "#VSTR_TOOLTIP_PERMSLEVEL"+group.GetGroupName());
		m_btnSettings = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnSettings"));

		m_group = group;
		m_PermsLevel.SetText(group.GetPermissionLevel().ToString());
		m_TotalMembers.SetText(group.GetMembers().Count().ToString());
		m_GroupName.SetText(group.GetGroupName());
		m_UserBoxes = new array<ref UserBoxTemplate>;
		
		array<ref VPPUser> grpmembers = group.GetMembers();
		foreach(VPPUser user : grpmembers)
		{
			m_UserBoxes.Insert(new UserBoxTemplate(m_GridSpacer, user.GetUserName(), user.GetUserId(),RootWidget));
		}
		m_EntryBox.SetHandler(this);
	}
	
	void ~UserGroupTemplate()
	{
		if (m_EntryBox != null)
			m_EntryBox.Unlink();
	}
	
	UserBoxTemplate GetUserBoxByID(string id)
	{
		foreach(UserBoxTemplate template : m_UserBoxes)
		{
			if(template.GetID() == id)
			{
				return template;
			}
		}
		return null;
	}
	
	CheckBoxWidget GetCheckBox()
	{
		return m_SelectGroup;
	}
	
	ScrollWidget GetScrollWidget()
	{
		return m_ScrollWidget; 
	}
	
	array<string> GetGroupPermissions()
	{
		return m_group.GetPermissions();
	}
		
	void RemoveUserBoxByID(string id)
	{
		UserBoxTemplate toBeRemoved = GetUserBoxByID(id);
		
		if(toBeRemoved != null)
		{
			m_UserBoxes.RemoveItem(toBeRemoved);
			delete toBeRemoved;
		}
	}
	
	string GetGroupName()
	{
		return m_group.GetGroupName();
	}
	
	/*
		Called by HookConfirmationDialog
	*/
	void UpdatePermissionLevel(int result, string userInput)
	{
		if (result == DIAGRESULT.OK)
		{
			if (userInput != "")
			{
				m_PermsLevel.SetText(userInput);
				GetRPCManager().VSendRPC( "RPC_PermissionManager", "UpdateUserGroupPermLvl", new Param2<int,string>(userInput.ToInt(),m_group.GetGroupName()), true);
				GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_PERMLVL_UPDATED"+userInput);
			}
		}
		//unhide scroll widgets
		VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).HideScrollWidgets(false);
	}

	private Widget 	       settingsRoot;
	private ButtonWidget   button_cancel;
	private ButtonWidget   button_save;
	private CheckBoxWidget chkForceAdminName;

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_SelectGroup)
		{
			VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
			MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).OnClick(w, x, y, button);
			return true;
		}

		if (w == button_cancel)
		{
			if (settingsRoot != null)
			{
				settingsRoot.Unlink();
				return true;
			}
		}

		if (w == button_save)
		{
			GetRPCManager().VSendRPC( "RPC_PermissionManager", "UpdateUserGroupSettings", new Param2<bool,string>(chkForceAdminName.IsChecked(),m_group.GetGroupName()), true);
			GetRPCManager().VSendRPC( "RPC_PermissionManager", "SendToClient", new Param1<int>(1), true); //Request Updated user groups
			settingsRoot.Unlink();
			return true;
		}

		if (w == m_btnSettings)
		{
			if (settingsRoot != null)
			{
				settingsRoot.Unlink();
				return true;
			}
			settingsRoot = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsPopUp, m_EntryBox);
			button_cancel = ButtonWidget.Cast(settingsRoot.FindAnyWidget("button_cancel"));
			button_save = ButtonWidget.Cast(settingsRoot.FindAnyWidget("button_save"));
			chkForceAdminName = CheckBoxWidget.Cast(settingsRoot.FindAnyWidget("chkForceAdminName"));
			chkForceAdminName.SetChecked(m_group.IsForceSavedName());
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
}