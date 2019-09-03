class UserGroupTemplate : VPPPlayerTemplate
{
	private ref array<ref UserBoxTemplate> m_UserBoxes;
	private ref UserGroup 	 m_group;
	private TextWidget 		 m_GroupName;
	private TextWidget 		 m_TotalMembers;
	private TextWidget 		 m_PermsLevel;
	private ButtonWidget     m_btnEditPermLevel;
	private CheckBoxWidget   m_SelectGroup;
	private GridSpacerWidget m_GridSpacer;
	private ScrollWidget     m_ScrollWidget;
	
	void UserGroupTemplate(GridSpacerWidget grid, UserGroup group, Widget RootWidget)
	{
		m_LayoutPath 	   = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/VPPUserGroupBox.layout";
		m_EntryBox 		   = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_GroupName 	   = TextWidget.Cast(m_EntryBox.FindAnyWidget("GroupName"));
		m_GridSpacer 	   = GridSpacerWidget.Cast(m_EntryBox.FindAnyWidget("SpacerMembersList"));
		m_TotalMembers     = TextWidget.Cast(m_EntryBox.FindAnyWidget("MemberCount"));
		m_PermsLevel  	   = TextWidget.Cast(m_EntryBox.FindAnyWidget("PermLevel"));
		m_SelectGroup 	   = CheckBoxWidget.Cast(m_EntryBox.FindAnyWidget("chkBoxSelectGrp"));
		m_ScrollWidget 	   = ScrollWidget.Cast(m_EntryBox.FindAnyWidget("ScrollerUserGroups"));
		m_btnEditPermLevel = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnEditPermLevel"));
		GetVPPUIManager().HookConfirmationDialog(m_btnEditPermLevel, RootWidget,this,"UpdatePermissionLevel", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Edit Permission Level", "Please Insert New Permission Level Value for the user group: "+group.GetGroupName());
		
		m_group = group;
		m_PermsLevel.SetText(group.GetPermissionLevel().ToString());
		m_TotalMembers.SetText(group.GetMembers().Count().ToString());
		m_GroupName.SetText(group.GetGroupName());
		m_UserBoxes = new array<ref UserBoxTemplate>;
		
		autoptr array<ref VPPUser> grpmembers = group.GetMembers();
		foreach(ref VPPUser user : grpmembers)
		{
			m_UserBoxes.Insert(new UserBoxTemplate(m_GridSpacer, user.GetUserName(), user.GetUserId(),RootWidget));
		}
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
		autoptr UserBoxTemplate toBeRemoved = GetUserBoxByID(id);
		
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
				GetRPCManager().SendRPC( "RPC_PermissionManager", "UpdateUserGroupPermLvl", new Param2<int,string>(userInput.ToInt(),m_group.GetGroupName()), true);
				GetVPPUIManager().DisplayNotification("Permission Level Updated To: "+userInput);
			}
		}
		//unhide scroll widgets
		autoptr VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).HideScrollWidgets(false);
	}
}