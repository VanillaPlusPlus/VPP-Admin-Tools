class UserBoxTemplate : VPPPlayerTemplate
{
	private TextWidget m_PlayerName;
	private TextWidget m_GUID;
	private ButtonWidget m_RemoveUser;
	private string m_Name;
	private string m_ID;
	
	void UserBoxTemplate(GridSpacerWidget grid, string name, string id, Widget RootWidget)
	{
		m_LayoutPath = "VPPAdminTools/GUI/Layouts/PermissionManagerUI/VPPUserBox.layout";
		m_EntryBox = GetGame().GetWorkspace().CreateWidgets( m_LayoutPath, grid);
		m_GUID 		  = TextWidget.Cast(m_EntryBox.FindAnyWidget("TextGUID"));
		m_PlayerName  = TextWidget.Cast(m_EntryBox.FindAnyWidget("TxtPlayerName"));
		m_RemoveUser  = ButtonWidget.Cast(m_EntryBox.FindAnyWidget("btnRemoveUser"));
		m_Name = name;
		m_ID = id;
		
		m_PlayerName.SetText(m_Name);
		m_GUID.SetText(m_ID);
		
		GetVPPUIManager().HookConfirmationDialog(m_RemoveUser, RootWidget,this,"DeleteUser", DIAGTYPE.DIAG_YESNO, "Delete User", "Are you sure you wish to remove "+m_Name + " from the group?");
	}
	
	void ~UserBoxTemplate()
	{
		if (m_EntryBox != null)
		m_EntryBox.Unlink();
	}
	
	void DeleteUser(int result)
	{
		//unhide scroll widgets
		autoptr VPPAdminHud rootMenu = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).HideScrollWidgets(false);
		if (result == DIAGRESULT.YES)
		{
			GetRPCManager().SendRPC( "RPC_PermissionManager", "RemoteRemoveUserFromGroup", new Param1<string>(m_ID), true);
			MenuPermissionsEditor.Cast(rootMenu.GetSubMenuByType(MenuPermissionsEditor)).ReloadUserGroupsTab();
			if (m_EntryBox != null)
			m_EntryBox.Unlink();
		}
	}

	string GetName()
	{
		return m_Name;
	}
	
	string GetID()
	{
		return m_ID;
	}
}