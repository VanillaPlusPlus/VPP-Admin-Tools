class MenuPermissionsEditor extends AdminHudSubMenu
{
	private GridSpacerWidget  m_SpacerUserGroups;
	private GridSpacerWidget  m_SpacerPermissions;
	private ScrollWidget      m_ScrollerPermissions;
	private CheckBoxWidget    m_chkAllPerms;
	private ScrollWidget 	  m_ScrollerUserGroups;
	private ButtonWidget      m_btnCreateGroup;
	private ButtonWidget      m_SavePermissions;
	private ButtonWidget      m_btnDeleteGroup;
	private ButtonWidget      m_btnAddMembers;
	private ref CustomGridSpacer m_LastGrid;
	private ref array<ref CustomGridSpacer> m_DataGrids;
	
	private ref array<string> m_AvailablePerms; /* all perms from PermissionManager*/
	private ref array<ref UserGroupTemplate> m_UserGroups;
	private ref array<ref PermissionBox> 	 m_PermissionBoxes;
	
	void MenuPermissionsEditor()
	{
		m_DataGrids  	  = new array<ref CustomGridSpacer>;
		m_UserGroups      = new array<ref UserGroupTemplate>;
		m_PermissionBoxes = new array<ref PermissionBox>;
		GetRPCManager().AddRPC("RPC_PermissionMenu","HandleUserGroups", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionMenu","HandlePermissions", this, SingleplayerExecutionType.Server);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		GetRPCManager().SendRPC( "RPC_PermissionManager", "SendToClient", new Param1<int>(0), true); //Get User Groups % Permissions levels 0 = both 1 = user groups 2 = perms

		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/PermissionManagerUI/MenuPermissionsEditor.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_ScrollerUserGroups = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ScrollerUserGroups"));
		m_SpacerUserGroups   = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerUserGroups"));
		m_SpacerPermissions  = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerPermissions"));
		m_ScrollerPermissions  = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ScrollerPermissions"));
		m_chkAllPerms        = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkAllPerms"));
		m_btnAddMembers		 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnAddMembers"));
		m_btnDeleteGroup	 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnDeleteGroup"));
		GetVPPUIManager().HookConfirmationDialog(m_btnDeleteGroup, M_SUB_WIDGET,this,"DeleteUserGroup", DIAGTYPE.DIAG_YESNO, "Delete User Group", "Are you sure you wish to delete selected user group?");
		
		m_btnCreateGroup     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnCreateGroup"));
		GetVPPUIManager().HookConfirmationDialog(m_btnCreateGroup, M_SUB_WIDGET,this,"CreateUserGroup", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Create User Group", "Please Insert a new for the group. Duplicate names will be rejected.", true);
		
		m_SavePermissions	 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnSaveChanges"));
		GetVPPUIManager().HookConfirmationDialog(m_SavePermissions, M_SUB_WIDGET,this,"ApplyNewPermissions", DIAGTYPE.DIAG_YESNO, "Update Permissions", "Are you sure you wish to update selected group's permissions?");
	}
	
	override void HideBrokenWidgets(bool state)
	{
		m_ScrollerPermissions.Show(!state);
		m_ScrollerUserGroups.Show(!state);
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible()) return;
		
		autoptr UserGroupTemplate selectedGrp = GetSelectedUserGroup();
		if (selectedGrp == null)
		{
			m_btnDeleteGroup.Enable(false);
			m_SavePermissions.Enable(false);
			m_btnAddMembers.Enable(false);
			return;
		}
		m_btnDeleteGroup.Enable(true);
		m_SavePermissions.Enable(true);
		m_btnAddMembers.Enable(true);
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		foreach(UserGroupTemplate group : m_UserGroups)
		{
			autoptr CheckBoxWidget checkBox = CheckBoxWidget.Cast(w);
			if (checkBox != null && checkBox == group.GetCheckBox() && group.GetCheckBox().IsChecked())
			{
				UnCheckAllGroups(checkBox);
				ResetPermissionBoxs();
				
				autoptr array<string> groupPerms = group.GetGroupPermissions();
				foreach(string perm : groupPerms)
				{
					if (m_AvailablePerms.Find(perm) > -1)
					{
						autoptr PermissionBox permBox = GetPermBoxByName(perm);
						if (permBox != null)
						{
							permBox.SetChecked(true);
						}
					}
				}
			}
		}
		
		if (w == m_chkAllPerms)
		{
			foreach(PermissionBox box : m_PermissionBoxes)
			{
				if (box != null)
					box.SetChecked(m_chkAllPerms.IsChecked());
			}
		}
		
		if (w == m_btnAddMembers)
		{
			//Hide perms editor
			if (IsSubMenuVisible())
				HideSubMenu();
			
			autoptr MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			if (pManager)
			{
				pManager.ShowSubMenu();
				pManager.TogglePlayerSelectMode(true);
				return false;
			}
			VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).CreateSubMenu(MenuPlayerManager);
			pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
			pManager.TogglePlayerSelectMode(true);
		}
		return false;
	}
	
	/*
		Called from MenuPlayerManager once user decides to add selected players to selected group
	*/
	void AddMembersToSelectedGroup(array<ref VPPPlayerEntry> selectedPlayers)
	{
		if (selectedPlayers != null && selectedPlayers.Count() > 0)
		{
			GetVPPUIManager().DisplayNotification("Requesting Server to Add Members to Group: "+GetSelectedUserGroup().GetGroupName());
			autoptr map<string,string> playersMap = new map<string,string>;
			
			foreach(ref VPPPlayerEntry entry : selectedPlayers)
			{
				playersMap.Insert(entry.GetID(),entry.GetPlayerName());
			}
			//Send RPC to add selected players
			GetRPCManager().SendRPC( "RPC_PermissionManager", "RemoteAddUsersToGroup", new Param2<ref map<string,string>,string>(playersMap,GetSelectedUserGroup().GetGroupName()), true);
			ReloadUserGroupsTab();
			return;
		}
		
		GetVPPUIManager().DisplayNotification("Error Adding Selected Players! Something Went Wrong.");
	}
	
	void DeleteUserGroup(int result)
	{
		HideScrollWidgets(false);
		if (result == DIAGRESULT.YES)
		{
			autoptr UserGroupTemplate selectedGroup = GetSelectedUserGroup();
			if (selectedGroup != null)
			{
				GetRPCManager().SendRPC( "RPC_PermissionManager", "RemoteDeleteUserGroup", new Param1<string>(selectedGroup.GetGroupName()), true);
			}
			ReloadUserGroupsTab();
		}
	}
	
	void ApplyNewPermissions(int result)
	{
		HideScrollWidgets(false);
		if (result == DIAGRESULT.YES)
		{
			autoptr UserGroupTemplate grp = GetSelectedUserGroup();
			if (grp != null)
			{
				autoptr array<string> selectedPerms = new array<string>;
				foreach(PermissionBox pbox : m_PermissionBoxes)
				{
					if (pbox != null && pbox.IsChecked())
					{
						selectedPerms.Insert(pbox.GetName());
					}
				}
				//Send Update to server
				GetRPCManager().SendRPC( "RPC_PermissionManager", "RemoteUpdateGroupPerms", new Param2<ref array<string>,string>(selectedPerms,grp.GetGroupName()), true);
				GetVPPUIManager().DisplayNotification("Permissions for group:"+grp.GetGroupName()+" updated & saved!");
				ReloadUserGroupsTab();
			}
		}
	}
	
	void CreateUserGroup(int result, string userInput)
	{
		HideScrollWidgets(false);
		if (result == DIAGRESULT.OK && userInput != "")
		{
			foreach(UserGroupTemplate group : m_UserGroups)
			{
				if (group.GetGroupName() == userInput)
				{
					GetVPPUIManager().DisplayNotification("Group Name: "+userInput+" already exists!");
					return;
				}
			}
			//Make UserGroup
			GetRPCManager().SendRPC( "RPC_PermissionManager", "RemoteCreateUserGroup", new Param1<string>(userInput), true);
			GetVPPUIManager().DisplayNotification("Created New User Group: "+userInput+"!");
			ReloadUserGroupsTab();
		}
	}
	
	UserGroupTemplate GetSelectedUserGroup()
	{
		if (m_UserGroups.Count() <= 0) return null;
		
		foreach(UserGroupTemplate group : m_UserGroups)
		{
			if (group != null && group.GetCheckBox().IsChecked())
			{
				return group;
			}
		}
		return null;
	}
	
	PermissionBox GetPermBoxByName(string name)
	{
		foreach(PermissionBox permBox : m_PermissionBoxes)
		{
			if (permBox.GetName() == name)
			{
				return permBox;
			}
		}
		return null;
	}
	
	void UnCheckAllGroups(CheckBoxWidget Ignore = null)
	{
		foreach(UserGroupTemplate group : m_UserGroups)
		{
			autoptr CheckBoxWidget checkBox = group.GetCheckBox();
			if (checkBox != null && checkBox.IsChecked() && checkBox != Ignore)
				checkBox.SetChecked(false);
		}
	}
	
	void ResetPermissionBoxs()
	{
		foreach(PermissionBox permBox : m_PermissionBoxes)
		{
			if (permBox != null)
				permBox.SetChecked(false);
		}
	}
	
	//only to stop overlaying bug
	void HideScrollWidgets(bool state)
	{
		if (state)
		m_ScrollerUserGroups.Show(false);
		else
		m_ScrollerUserGroups.Show(true);
	}
	
	/*
		Clean user groups tab from all entries & send request to get from server
	*/
	void ReloadUserGroupsTab()
	{
		for(int i = 0; i < m_UserGroups.Count(); i++)
		{
			autoptr UserGroupTemplate grp = m_UserGroups.Get(i);
			m_UserGroups.Remove(i);
			delete grp;
		}
		m_UserGroups = new array<ref UserGroupTemplate>;
		GetRPCManager().SendRPC( "RPC_PermissionManager", "SendToClient", new Param1<int>(1), true); //Request Updated user groups
	}

	//-----RPCs-----
	void HandleUserGroups(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<ref array<ref UserGroup>> data;
			if(!ctx.Read(data)) return;
			
			autoptr array<ref UserGroup> input = data.param1;
			
			//Delete old--
			for(int i = 0; i < m_UserGroups.Count(); i++)
			{
				autoptr UserGroupTemplate grp = m_UserGroups.Get(i);
				m_UserGroups.Remove(i);
				delete grp;
			}
			m_UserGroups = new array<ref UserGroupTemplate>;
			//----
			foreach(ref UserGroup group : input)
			{
				m_UserGroups.Insert(new UserGroupTemplate(m_SpacerUserGroups, group, M_SUB_WIDGET));
			}
		}
	}
	
	void HandlePermissions(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<ref array<string>> data;
			if (!ctx.Read(data)) return;
			m_AvailablePerms = data.param1;
			
			foreach(CustomGridSpacer cusGrid : m_DataGrids)
			{
				if (cusGrid != null)
					delete cusGrid;
			}
			m_DataGrids = new array<ref CustomGridSpacer>;
			
			//init first "page"
			m_DataGrids.Insert(new CustomGridSpacer(m_SpacerPermissions));
			m_LastGrid = m_DataGrids[0];
			m_ScrollerPermissions.Update();
			m_SpacerPermissions.Update();
			
			foreach(string perm : m_AvailablePerms)
			{
				AddEntry(perm);
			}
		}
	}
	
	void AddEntry(string perm)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_SpacerPermissions);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100)
		{
			ref PermissionBox entry = new PermissionBox(m_LastGrid.GetGrid(), perm);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_PermissionBoxes.Insert(entry);
		}
		
		m_SpacerPermissions.Update();
		m_ScrollerPermissions.Update();
		m_LastGrid.GetGrid().Update();
	}
	//---------------
}