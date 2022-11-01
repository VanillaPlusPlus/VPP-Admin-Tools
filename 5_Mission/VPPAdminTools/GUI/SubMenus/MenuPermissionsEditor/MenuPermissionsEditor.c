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
		GetRPCManager().AddRPC("RPC_PermissionMenu","HandleUserGroups", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_PermissionMenu","HandlePermissions", this, SingleplayerExecutionType.Client);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		GetRPCManager().VSendRPC( "RPC_PermissionManager", "SendToClient", new Param1<int>(0), true); //Get User Groups % Permissions levels 0 = both 1 = user groups 2 = perms

		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuPermissionsEditor);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_ScrollerUserGroups = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ScrollerUserGroups"));
		m_SpacerUserGroups   = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerUserGroups"));
		m_SpacerPermissions  = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SpacerPermissions"));
		m_ScrollerPermissions  = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ScrollerPermissions"));
		m_chkAllPerms        = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "chkAllPerms"));
		m_btnAddMembers		 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnAddMembers"));
		m_btnDeleteGroup	 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnDeleteGroup"));
		GetVPPUIManager().HookConfirmationDialog(m_btnDeleteGroup, M_SUB_WIDGET,this,"DeleteUserGroup", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIPE_TITLE_DELGROUP", "#VSTR_TOOLTIPE_DELGROUP");
		
		m_btnCreateGroup     = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnCreateGroup"));
		GetVPPUIManager().HookConfirmationDialog(m_btnCreateGroup, M_SUB_WIDGET,this,"CreateUserGroup", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "#VSTR_TOOLTIP_TITLE_CREATEGROUP", "#VSTR_TOOLTIP_CREATEGROUP", true);
		
		m_SavePermissions	 = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnSaveChanges"));
		GetVPPUIManager().HookConfirmationDialog(m_SavePermissions, M_SUB_WIDGET,this,"ApplyNewPermissions", DIAGTYPE.DIAG_YESNO, "#VSTR_TOOLTIP_TITLE_UPDATE_PERMSGRP", "#VSTR_TOOLTIP_UPDATE_PERMSGRP");
		M_SUB_WIDGET.SetHandler(this);
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
		
		UserGroupTemplate selectedGrp = GetSelectedUserGroup();
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
		foreach(UserGroupTemplate group : m_UserGroups)
		{
			CheckBoxWidget checkBox = CheckBoxWidget.Cast(w);
			if (checkBox != null && checkBox == group.GetCheckBox() && group.GetCheckBox().IsChecked())
			{
				UnCheckAllGroups(checkBox);
				ResetPermissionBoxs();
				
				array<string> groupPerms = group.GetGroupPermissions();
				foreach(string perm : groupPerms)
				{
					if (m_AvailablePerms.Find(perm) > -1)
					{
						PermissionBox permBox = GetPermBoxByName(perm);
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
			
			MenuPlayerManager pManager = MenuPlayerManager.Cast(VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud)).GetSubMenuByType(MenuPlayerManager));
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
		return super.OnClick(w, x, y, button);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		
	}
	
	/*
		Called from MenuPlayerManager once user decides to add selected players to selected group
	*/
	void AddMembersToSelectedGroup(array<ref VPPPlayerEntry> selectedPlayers)
	{
		if (selectedPlayers != null && selectedPlayers.Count() > 0)
		{
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_PERMS_REQUSTADD "+GetSelectedUserGroup().GetGroupName());
			map<string,string> playersMap = new map<string,string>;
			
			foreach(VPPPlayerEntry entry : selectedPlayers)
			{
				playersMap.Insert(entry.GetID(),entry.GetPlayerName());
			}
			//Send RPC to add selected players
			GetRPCManager().VSendRPC( "RPC_PermissionManager", "RemoteAddUsersToGroup", new Param2<ref map<string,string>,string>(playersMap,GetSelectedUserGroup().GetGroupName()), true);
			ReloadUserGroupsTab();
			return;
		}
		
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_ERR_ADD_PLAYERSPERMS");
	}
	
	void DeleteUserGroup(int result)
	{
		HideScrollWidgets(false);
		if (result == DIAGRESULT.YES)
		{
			UserGroupTemplate selectedGroup = GetSelectedUserGroup();
			if (selectedGroup != null)
			{
				GetRPCManager().VSendRPC( "RPC_PermissionManager", "RemoteDeleteUserGroup", new Param1<string>(selectedGroup.GetGroupName()), true);
			}
			ReloadUserGroupsTab();
		}
	}
	
	void ApplyNewPermissions(int result)
	{
		HideScrollWidgets(false);
		if (result == DIAGRESULT.YES)
		{
			UserGroupTemplate grp = GetSelectedUserGroup();
			if (grp != null)
			{
				array<string> selectedPerms = new array<string>;
				foreach(PermissionBox pbox : m_PermissionBoxes)
				{
					if (pbox != null && pbox.IsChecked())
					{
						selectedPerms.Insert(pbox.GetName());
					}
				}
				//Send Update to server
				GetRPCManager().VSendRPC( "RPC_PermissionManager", "RemoteUpdateGroupPerms", new Param2<ref array<string>,string>(selectedPerms,grp.GetGroupName()), true);
				GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_PERMS_UPD_GRP "+grp.GetGroupName()+" #VSTR_NOTIFY_PERMS_UPD_GRP_2");
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
					GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_GRPNAME "+userInput+" #VSTR_NOTIFY_ALREADY_EXISTS");
					return;
				}
			}
			//Make UserGroup
			GetRPCManager().VSendRPC( "RPC_PermissionManager", "RemoteCreateUserGroup", new Param1<string>(userInput), true);
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_USER_GRP_CREATED "+userInput+"!");
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
			CheckBoxWidget checkBox = group.GetCheckBox();
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
			UserGroupTemplate grp = m_UserGroups.Get(i);
			m_UserGroups.Remove(i);
			delete grp;
		}
		m_UserGroups = new array<ref UserGroupTemplate>;
		GetRPCManager().VSendRPC( "RPC_PermissionManager", "SendToClient", new Param1<int>(1), true); //Request Updated user groups
	}

	//-----RPCs-----
	void HandleUserGroups(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<ref array<ref UserGroup>> data;
			if(!ctx.Read(data)) return;
			
			array<ref UserGroup> input = data.param1;
			
			//Delete old--
			for(int i = 0; i < m_UserGroups.Count(); i++)
			{
				UserGroupTemplate grp = m_UserGroups.Get(i);
				m_UserGroups.Remove(i);
				delete grp;
			}
			m_UserGroups = new array<ref UserGroupTemplate>;
			//----
			foreach(UserGroup group : input)
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
			PermissionBox entry = new PermissionBox(m_LastGrid.GetGrid(), perm);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_PermissionBoxes.Insert(entry);
		}
		
		m_SpacerPermissions.Update();
		m_ScrollerPermissions.Update();
		m_LastGrid.GetGrid().Update();
	}
	//---------------
}