enum NotifyTypes
{
	ERROR,
	NOTIFY,
	PERMISSION_REJECT
};

class PermissionManager extends ConfigurablePlugin
{
	private ref   array<string> m_SuperAdmins;
	private const string m_SuperAdminsListPath = "$profile:VPPAdminTools/Permissions/SuperAdmins/SuperAdmins.txt";
	private const string m_UserGroupsSavePath  = "$profile:VPPAdminTools/Permissions/UserGroups/UserGroups";
		
	private ref PermissionLoader 		  m_PermissionLoader;
	private ref array<string> 		  	  m_Permissions;
	private ref array<ref UserGroup> 	  m_UserGroups;
	
	void PermissionManager()
	{
		m_PermissionLoader = new PermissionLoader();

		m_UserGroups = new array<ref UserGroup>;
		m_Permissions = new array<string>;
		m_SuperAdmins = new array<string>;

		#ifdef VPPADMINTOOLS_DEBUG
		m_SuperAdmins.Insert("76561198420222029");
		m_SuperAdmins.Insert("76561198321354754");
		#endif
		
		//Misc
		AddPermissionType({"DeleteObjectAtCrosshair","TeleportToCrosshair","FreeCamera", "RepairVehiclesAtCrosshair"});
		//Item Manager
		AddPermissionType({ "MenuItemManager","MenuItemManager:SpawnItem","MenuItemManager:EditPreset","MenuItemManager:SpawnPreset","MenuItemManager:DeletePreset","MenuItemManager:AddPreset" });
		//Server Manager
		AddPermissionType({ "MenuServerManager","ServerManager:RestartServer","ServerManager:LockServer","ServerManager:KickAllPlayers","ServerManager:LoadScripts" });
		//Weather Manager
		AddPermissionType({ "MenuWeatherManager","WeatherManager:ApplyWeather","WeatherManager:ApplyTime","WeatherManager:SavePreset","WeatherManager:DeletePreset","WeatherManager:ApplyPreset","WeatherManager:ApplyTimePreset","WeatherManager:SaveTimePreset","WeatherManager:DeleteTimePreset" });
		//Object Manager
		AddPermissionType({ "MenuObjectManager","MenuObjectManager:CreateNewSet","MenuObjectManager:UpdateSet","MenuObjectManager:DeleteSet","MenuObjectManager:EditSet" });
		//Permissions Editor
		AddPermissionType({ "MenuPermissionsEditor","PermissionsEditor:RemoveUser","PermissionsEditor:AddUser","PermissionsEditor:CreateUserGroup","PermissionsEditor:DeleteUserGroup","PermissionsEditor:ChangePermLevel"});
		//Player Manager
		AddPermissionType({ "MenuPlayerManager","PlayerManager:GiveGodmode","PlayerManager:BanPlayer","PlayerManager:KickPlayer","PlayerManager:HealPlayers","PlayerManager:SetPlayerStats","PlayerManager:KillPlayers","PlayerManager:GodMode","PlayerManager:SpectatePlayer","PlayerManager:TeleportToPlayer","PlayerManager:TeleportPlayerTo","PlayerManager:SetPlayerInvisible","PlayerManager:SendMessage", "PlayerManager:GiveUnlimitedAmmo", "PlayerManager:MakePlayerVomit", "PlayerManager:FreezePlayers", "PlayerManager:ChangeScale"});
		//Bans Manager
		AddPermissionType({ "MenuBansManager","BansManager:UnbanPlayer","BansManager:UpdateBanDuration","BansManager:UpdateBanReason" });
		//WebHooks Menu
		AddPermissionType({ "MenuWebHooks","MenuWebHooks:Create", "MenuWebHooks:Edit", "MenuWebHooks:Delete" });
		//Teleport Manager Menu
		AddPermissionType({ "MenuTeleportManager","TeleportManager:ViewPlayerPositions","TeleportManager:TPPlayers","TeleportManager:TPSelf","TeleportManager:DeletePreset","TeleportManager:AddNewPreset","TeleportManager:EditPreset" });
		//ESP tools menu 
		AddPermissionType({ "EspToolsMenu","EspToolsMenu:DeleteObjects","EspToolsMenu:PlayerESP", "EspToolsMenu:RestPasscodeFence", "EspToolsMenu:RetriveCodeFromObj", "EspToolsMenu:PlayerMeshEsp" });
		//XML Editor menu
		AddPermissionType({ "MenuXMLEditor" });
		//Commands console menu
		AddPermissionType({ "MenuCommandsConsole" });
		
		//-----RPC's-----
		GetRPCManager().AddRPC("RPC_PermitManager", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PermitManager", "VerifyButtonsPermission", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PermissionManager","SendToClient", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteCreateUserGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteDeleteUserGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","UpdateUserGroupPermLvl", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","UpdateUserGroupSettings", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteUpdateGroupPerms", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteAddUsersToGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteRemoveUserFromGroup", this, SingleplayerExecutionType.Server);
		//---------------
	}

	override void OnInit()
	{
		Load();
	}
	
	override void Load()
	{
		if(GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			MakeDirectory("$profile:VPPAdminTools/Permissions");
			MakeDirectory("$profile:VPPAdminTools/Permissions/SuperAdmins");
			MakeDirectory("$profile:VPPAdminTools/Permissions/UserGroups");
			
			LoadSuperAdmins();
			LoadUserGroups();
		}
	}
	
	override void Save()
	{
		if(GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			JsonFileLoader<ref array<ref UserGroup>>.JsonSaveFile(m_UserGroupsSavePath + ".json", m_UserGroups);
		}
	}
	
	/*
		RPC Functions
	*/
	//----------------------------------
	void EnableToggles( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
	   	if( type == CallType.Server )
        {
			if (sender == NULL) return;
			
        	if (HasUserGroup(sender.GetPlainId()))
        	{
        		Param1<bool>> param = new Param1<bool>(true);
	   			GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", param, true, sender);
        	}
        }
	}
	
	void VerifyButtonsPermission(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server && sender != null)
        {
			array<string> tmp = new array<string>;
			tmp.Copy(data.param1);
			SendUpdatedPermissions(tmp,sender.GetPlainId());
		}
	}
	
	/*
		Call this function each time super admin updates users' permissions
		@Param1: permsToUpdate array of all permissions that the client now needs to update ( we shall mostly use this for updating catagory buttons permissions only! )
		@Param2: targetId ACCEPTS ONLY Steam64 ID!
	*/
	void SendUpdatedPermissions(array<string> permsToUpdate, string targetId)
	{
		map<string,bool> permissions = new map<string,bool>;
		foreach(string perm : permsToUpdate)
		{
			permissions.Insert(perm,VerifyPermission(targetId,perm, "", false));
		}

		PlayerIdentity identity = GetIdentityById(targetId);
		if (identity != null)
			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "VerifyButtonsPermission", new Param1<ref map<string,bool>>(permissions), true, identity);
	}
	
	/*
		Send over perms and user groups to superadmins when editing perms via UI
	*/
	void SendToClient(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<int> data;
		if (!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (sender == null) {
				GetSimpleLogger().Log("[PermissionManager] SendToClient: Error! Unable to send perms and user groups to target! NULL IDENTITY!!");
				return;
			}
			if (data.param1 == 0){
				GetRPCManager().SendRPC( "RPC_PermissionMenu", "HandlePermissions", new Param1<ref array<string>>(m_Permissions), true, sender);
				GetRPCManager().SendRPC( "RPC_PermissionMenu", "HandleUserGroups", new Param1<ref array<ref UserGroup>>(m_UserGroups), true, sender);
			}
			if (data.param1 == 1)
				GetRPCManager().SendRPC( "RPC_PermissionMenu", "HandleUserGroups", new Param1<ref array<ref UserGroup>>(m_UserGroups), true, sender);
			
			if (data.param1 == 2)
				GetRPCManager().SendRPC( "RPC_PermissionMenu", "HandlePermissions", new Param1<ref array<string>>(m_Permissions), true, sender);
		}
	}
	
	/*
		Create user group via RPC
	*/
	void RemoteCreateUserGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "PermissionsEditor:CreateUserGroup")) return;

			CreateUserGroup( data.param1, 0, {} );
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PermissionManager] Create User Group: " + data.param1));
		}
	}
	
	/*
		Delete user group via RPC
	*/
	void RemoteDeleteUserGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "PermissionsEditor:DeleteUserGroup")) return;

			RemoveUserGroupByName(data.param1);
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PermissionManager] Delete User Group: " + data.param1));
		}
	}
	
	/*
		Update a user groups permission level via RPC
	*/
	void UpdateUserGroupPermLvl(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<int,string> data; //lvl, group name
		if (!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "PermissionsEditor:ChangePermLevel")) return;

			if (data.param1 > -1 && data.param2 != "")
			{
				//Find group
				foreach(UserGroup group : m_UserGroups)
				{
					if (group.GetGroupName() == data.param2)
					{
						group.SetPermissionLevel(data.param1);
						Save();
						string logMsg = string.Format("[PermissionManager] Group: (%1) Permission level was changed to: (%2) by: \"%3\" (steamid=%4)", group.GetGroupName(), data.param1, sender.GetName(), sender.GetPlainId());
						GetSimpleLogger().Log(logMsg);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), logMsg));
						return;
					}
				}
			}
		}
	}

	/*
		Update a user groups settings via RPC
	*/
	void UpdateUserGroupSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<bool,string> data; //forcesavedname, group name
		if (!ctx.Read(data)) return;

		if(type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "MenuPermissionsEditor", "", false)) return;

			if (data.param2 != "")
			{
				foreach(UserGroup group : m_UserGroups)
				{
					if (group.GetGroupName() == data.param2)
					{
						group.SetForceSavedName(data.param1);
						Save();
						NotifyPlayer(sender.GetPlainId(),"#VSTR_UPDATED_PERM_SETTING"+ " : "+group.GetGroupName(),NotifyTypes.NOTIFY);
						string logMsg = string.Format("[PermissionManager] Group: (%1) Settings changed by: \"%2\" (steamid=%3)", group.GetGroupName(), sender.GetName(), sender.GetPlainId());
						GetSimpleLogger().Log(logMsg);
						GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), logMsg));
						return;
					}
				}
			}
		}
	}
	
	/*
		Update user groups permissions via RPC
	*/
	void RemoteUpdateGroupPerms(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref array<string>,string> data; //permission types, group name
		if (!ctx.Read(data)) return;
		
		array<string> tempData = data.param1;
		if (type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "MenuPermissionsEditor", "", false)) return;

			foreach(UserGroup group : m_UserGroups)
			{
				if (group.GetGroupName() == data.param2)
				{
					group.SetPermissions(tempData);

					string logMsg = string.Format("[PermissionManager] Group: (%1) Permissions updated by: \"%2\" (steamid=%3)", group.GetGroupName(), sender.GetName(), sender.GetPlainId());
					GetSimpleLogger().Log(logMsg);
					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), logMsg));
					
					array<ref VPPUser> grpMembers = group.GetMembers();
					foreach(ref VPPUser member : grpMembers)
					{
						SendUpdatedPermissions(tempData, member.GetUserId());
					}
					Save();
					return;
				}
			}
		}
	}
	
	/*
		Add new Members to an existing user group via RPC
	*/
	void RemoteAddUsersToGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<ref map<string,string>,string> data; //steam64 Id | username, group name
		if (!ctx.Read(data)) return;
		
		map<string,string> tmp = data.param1;
		tmp.Copy(data.param1);
		if (type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "PermissionsEditor:AddUser")) return;

			foreach(string id, string name : tmp)
			{
				PlayerBase player;
				string steam64 = ConvertGUIDToSteamID(id, player);

				AddMembersToGroup(new VPPUser(name, steam64), data.param2, sender.GetPlainId());
				GetSimpleLogger().Log(string.Format("[PermissionManager] Added \"%1\" (steamid=%2) to user group (%3) by: \"%4\" (steamid=%5)", name, steam64, data.param2, sender.GetName(), sender.GetPlainId()));

				if (player != NULL)
				{
					GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(true), true, player.GetIdentity());
					GetPlayerListManager().SendPlayerList(player.GetIdentity());
				}
			}
			GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PermissionManager] Adding Users to User Group: " +data.param2 + " Total: " + tmp.Count()));
		}
	}
	
	/*
		Remove a user from usergroup via RPC
	*/
	
	void RemoteRemoveUserFromGroup(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data; //steam64 Id
		if (!ctx.Read(data)) return;
		
		if (type == CallType.Server)
		{
			if (!VerifyPermission(sender.GetPlainId(), "MenuPermissionsEditor", "", false)) return;

			bool removed = false;
			foreach(UserGroup group : m_UserGroups)
			{
				if (group.FindUser(data.param1))
				{
					group.RemoveMembers(data.param1);
					NotifyPlayer(sender.GetPlainId(),"#VSTR_PERMS_REMOVE_USER"+data.param1+"#VSTR_FROM_GROUP"+group.GetGroupName(),NotifyTypes.NOTIFY);

					GetWebHooksManager().PostData(AdminActivityMessage, new AdminActivityMessage(sender.GetPlainId(), sender.GetName(), "[PermissionManager] Removing User: " + data.param1 + " from Group: " + group.GetGroupName()));
					Save();

					string logMsg = string.Format("[PermissionManager] Removed (steamid=%1) from Group: (%2) by: \"%3\" (steamid=%4)",data.param1, group.GetGroupName(), sender.GetName(), sender.GetPlainId());
					GetSimpleLogger().Log(logMsg);
					removed = true;
					break;
				}
			}

			if (removed)
			{
				PlayerIdentity identity = GetIdentityById(data.param1);	
				if (identity != null){
					GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(false), true, identity);
				}
			}
		}
	}
	//----------------------------------
	
	private void CreateUserGroup( string groupName, int permissionLevel, array<string> permissioncatagorys )
	{
		if (groupName == "")
		{
		 	GetSimpleLogger().Log("CreateUserGroup() ERROR:: Invalid GroupName.");
			return;
		}
		
		if (permissioncatagorys == null)
		{
			GetSimpleLogger().Log("CreateUserGroup() ERROR:: Invalid Permissions.");
			return;
		}
		
		foreach(UserGroup group : m_UserGroups)
		{
			if(group.GetGroupName() == groupName)
			{
				GetSimpleLogger().Log("[PermissionManager]:: CreateUserGroup(): ERROR: UserGroup already exsists by this name.");
				return;
			}
		}
		
		m_UserGroups.Insert(new UserGroup(groupName,permissionLevel,permissioncatagorys));
		Save();
		GetSimpleLogger().Log(string.Format("[PermissionManager] Group: (%1) Permission Level: (%2) was created!", groupName, permissionLevel));
	}

	void AddMembersToGroup(VPPUser user, string groupName, string id = "")
	{
		if (groupName == "")
		{
			GetSimpleLogger().Log("[PermissionManager]:: AddMembersToGroup(): Error, invalid group name");
			return;
		}
		
		if(IsSuperAdmin(user.GetUserId()))
		{
			if(id != string.Empty)
				NotifyPlayer(id, "#VSTR_ERROR_USER_SUPERADMIN", NotifyTypes.ERROR);
			
			return;
		}
		
		UserGroup selectedGroup;
		UserGroup currentGroup;
	
		foreach(UserGroup group : m_UserGroups)
		{
			if(group.HasMember(user.GetUserId()))
			{
				currentGroup = group;
			}
		
			if(group.GetGroupName() == groupName)
			{
				selectedGroup = group;
			}
		}
		
		if(selectedGroup == null)
		{
			if(id != string.Empty)
				NotifyPlayer(id, "#VSTR_ERROR_INVALID_GROUP" + groupName, NotifyTypes.ERROR);
			
			return;
		}
		
		if(selectedGroup == currentGroup)
		{
			if(id != string.Empty)
				NotifyPlayer(id, "Error: User: " + user.GetUserId() + " " + "#VSTR_PERMS_USER_INGTROUP" + " " + groupName, NotifyTypes.ERROR);
			
			return;
		}
		
		if(currentGroup == null && selectedGroup != null)
		{
			selectedGroup.AddMembers({user});
			Save();
			return;
		}
		
		if(selectedGroup != currentGroup)
		{
			selectedGroup.AddMembers({user});
			currentGroup.RemoveMembers(user.GetUserId());
			Save();
			
			return;
		}
	}
	
	/*
		@Doc: This function should be only used by other plugins to add new permissions to the pool
		@Param: array<string> newPerms
		@Usage: GetPermissionManager().AddPermissionType("");
		@Discription: Adds permission to the pool of total permissions, which can be usaged by GetPermissionManager().VerifyPermission().
	*/
	void AddPermissionType(array<string> newPerms)
	{
		foreach(string str : newPerms)
		{
			if (m_Permissions.Find(str) == -1)
			{
				GetSimpleLogger().Log(string.Format("[PermissionManager] Perm (%1) has been registered!", str));
				m_Permissions.Insert(str);
				continue;
			}
			
			GetSimpleLogger().Log("[PermissionManager] Tried registering a duplicate permission (" + str + ")");
		}
	}
	
	private void CreateDefualtUserGroups()
	{
		CreateUserGroup( "Admins", 1, m_Permissions );
		AddMembersToGroup(new VPPUser("Example User","26561198420222028"), "Admins");
		
	    GetSimpleLogger().Log("[PermissionManager] Created default UserGroups.vpp");
	}
	
	private void RemoveUserGroupByName(string groupname)
	{
		foreach(UserGroup group: m_UserGroups)
		{
			if (group != null && group.GetGroupName() == groupname)
			{
				foreach(VPPUser user : group.GetMembers())
				{
					PlayerIdentity identity = GetIdentityById(user.GetUserId());
					if (identity != null)
		   				GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(false), true, identity);
				}
				m_UserGroups.RemoveItem(group);
				delete group;
				Save();

				string logMsg = string.Format("[PermissionManager] Group (%1) was removed!", groupname);
				GetSimpleLogger().Log(logMsg);
			}
		}
	}
	
	private void LoadUserGroups()
	{
		//backward compatibility check & conversion
		if (FileExist(m_UserGroupsSavePath + ".vpp"))
		{
			array<Old_UserGroup> userGroups = new array<Old_UserGroup>;
			if (!m_PermissionLoader.ConvertFormat((m_UserGroupsSavePath + ".vpp"), userGroups))
			{
				GetSimpleLogger().Log("[PermissionManager] Failed to convert UserGroups to JSON format!");
				Print("[PermissionManager] Failed to convert UserGroups to JSON format!");
				CreateDefualtUserGroups();
				return;
			}
		}

		if (FileExist(m_UserGroupsSavePath + ".json"))
		{
			JsonFileLoader<ref array<ref UserGroup>>.JsonLoadFile(m_UserGroupsSavePath + ".json", m_UserGroups);
			if (m_UserGroups == NULL)
			{
				GetSimpleLogger().Log("[PermissionManager] Failed to load UserGroups.json, creating defaults");
				CreateDefualtUserGroups();
				return;
			}
			GetSimpleLogger().Log("[PermissionManager] Loaded UserGroups.json");
		}
		else
		{
			CreateDefualtUserGroups();
		}
	}
	
	private void LoadSuperAdmins()
	{
		FileHandle adminUIDSFile;
		array<string> m_ContentLines = new array<string>;
		
		if (FileExist(m_SuperAdminsListPath))
		{
			adminUIDSFile = OpenFile(m_SuperAdminsListPath, FileMode.READ);
			string line_content;
			
			while ( FGets(adminUIDSFile,line_content) > 0 )
			{
				m_ContentLines.Insert(line_content);
				line_content = "";
			}
			
			for (int i = 0; i < m_ContentLines.Count(); ++i)
			{
				string trimmed = m_ContentLines.Get(i);
				trimmed.Trim();
				trimmed.Replace(" ","");
				m_SuperAdmins.Insert(trimmed);
				GetSimpleLogger().Log("[PermissionManager] Adding Super Admin ("+ trimmed + ")");
			}
		}
		else 
		{
			GetSimpleLogger().Log("[PermissionManager] SuperAdmins.txt File Missing...Creating Defaults. Please change SuperAdmins.txt file, and restart the server.");
			adminUIDSFile = OpenFile(m_SuperAdminsListPath ,FileMode.WRITE);
			#ifdef VPPADMINTOOLS_DEBUG
			FPrintln(adminUIDSFile, "76561198420222029");
			FPrintln(adminUIDSFile, "76561198321354754");
			#else
			FPrintln(adminUIDSFile, "Remove this text and add your steam64 ID (each ID entry on a *new line*, no spaces between or commas)");
			#endif
		}
		CloseFile(adminUIDSFile);
	}
		
	bool HasUserGroup(string id)
	{
		foreach(UserGroup group : m_UserGroups)
			if(group.HasMember(id))
				return true;

		return false || IsSuperAdmin(id);
	}

	//Get A user group by user ID
	UserGroup GetUserGroup(string id)
	{
		foreach(UserGroup group : m_UserGroups)
			if(group.HasMember(id))
				return group;

		return null;
	}
	
	// @Doc:
	// @Usage: Check permission to user group. 
	// @Param: string callerID, string permissionName, string TargetID
	// @Target ID should match be the same as target if there isn't a target.
	
	bool VerifyPermission(string id, string permissionName, string targetID = "", bool sendNotify = true)
	{
		if (m_Permissions.Find(permissionName) <= -1) //Reject permission no matter what if wrong perm type was received.
		{
			GetSimpleLogger().Log(string.Format("[PermissionManager] (steamid=%1) Targeting: (himself), Unregistred/Unknown Permission type (%2) Rejected.", id, permissionName));

			if ( sendNotify )
				NotifyPlayer(id, permissionName + "#VSTR_ERROR_UNKOWN_PERM",NotifyTypes.ERROR);

			return false;
		}
		
		//Reject permission if name not matching usergroup
		UserGroup userGroup = GetUserGroup(id);
		if (userGroup != null && userGroup.IsForceSavedName())
		{
			VPPUser user = userGroup.FindUser(id);
			PlayerBase identity = GetPlayerBaseByID(id);
			if (identity && user)
			{
				if (user.GetUserName() != identity.VPlayerGetName())
				{
					NotifyPlayer(id, "#VSTR_ERROR_NAME_MISMATCH" + user.GetUserName(),NotifyTypes.PERMISSION_REJECT, 15.0);
					return false;
				}
			}
		}

		if(!HasUserGroup(id))
		{
			string logMsg = string.Format("[PermissionManager] Permission denied for (steamid=%1) to exectue (%2) Reason: Does not belong to a user group!", id, permissionName);
			GetSimpleLogger().Log(logMsg);

			if ( sendNotify )
				NotifyPlayer(id,"#VSTR_ERROR_NOT_ADMIN",NotifyTypes.PERMISSION_REJECT);
			
			return false;
		}
		
		if(IsSuperAdmin(id) && !IsSuperAdmin(targetID) && id != targetID)
		{
			return true;
		}
		
		if(IsSuperAdmin(targetID) && !IsSuperAdmin(id))
		{
			if ( sendNotify )
				NotifyPlayer(id,"#VSTR_ERROR_CANT_TARGET" + permissionName, NotifyTypes.PERMISSION_REJECT);
			
			return false;
		}
		
		if(IsSuperAdmin(id) && targetID == id)
		{
			return true;
		}
		
		if(IsSuperAdmin(id) && IsSuperAdmin(targetID))
		{
			return true;
		}

		int targetPermLevel = int.MAX;
		int senderPermLevel = int.MAX;
		
		foreach(UserGroup group : m_UserGroups)
		{
			if(group.FindUser(targetID) != null)
			{
				targetPermLevel = group.GetPermissionLevel();
			}
			
			if(group.FindUser(id) != null)
			{
				if(group.GetPermissions().Find(permissionName) > -1)
				{
					senderPermLevel = group.GetPermissionLevel();	
					
					if (targetID == string.Empty)
					{
						return true;
					}
				}
				
				if(group.GetPermissions().Find(permissionName) <= -1)
				{
					if ( sendNotify )
						NotifyPlayer(id,"#VSTR_ERROR_NO_PERM" + permissionName, NotifyTypes.PERMISSION_REJECT);
					
					return false;
				}
			}
		}
		
		bool hasPermission = senderPermLevel < targetPermLevel || (targetID == id && senderPermLevel == targetPermLevel);
		PlayerBase targetIdentity = GetPlayerBaseByID(targetID);
		if (!hasPermission &&  targetIdentity != null)
		{
			if ( sendNotify )
				NotifyPlayer(id,"#VSTR_PERM_LVL_LOW" + permissionName + " On: "+targetIdentity.VPlayerGetName(), NotifyTypes.PERMISSION_REJECT);
		}
		return hasPermission;
	}
	
	void NotifyPlayer(string id,string msg, int type, float duration = 8.0)
	{
		PlayerIdentity identity = GetIdentityById(id);
		if (identity == null) return;

		switch(type)
		{
			case NotifyTypes.ERROR:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( identity, duration, "Error: ", msg, "set:ccgui_enforce image:Icon40Emergency" );
			break;
			
			case NotifyTypes.NOTIFY:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( identity, duration, "Notification: ", msg, "set:ccgui_enforce image:MapUserMarker" );
			break;
			
			case NotifyTypes.PERMISSION_REJECT:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( identity, duration, "Permission Rejected: ", msg, "set:ccgui_enforce image:Icon40Emergency" );
			break;
		}
	}
	
	/*
		Works with steam64ID and or GUID
	*/
	string GetPlayerUserGrpNameByID(string id)
	{
		if (GetIdentityById(id) == null) return "None";
		
		if (IsSuperAdmin(id))
		{
			return "Super Admin";
		}
		else
		{
			foreach(UserGroup group : m_UserGroups)
			{
				if (!group.HasMember(id))
					continue;
				else
					return group.GetGroupName();
			}
		}
		return "None";
	}
	
	/*
		Get Identity using either GUID or Steam64ID
	*/
	PlayerIdentity GetIdentityById(string id)
	{
		autoptr array<Man> players = new array<Man>;
       	GetGame().GetWorld().GetPlayerList( players );
		
		for(int i = 0; i < players.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(players[i]);
			if(player)
			{
				if(player.VPlayerGetSteamId() == id || player.VPlayerGetHashedId() == id)
				{
					return player.GetIdentity();
				}
			}
		}

		return null;
	}
	
	/*
		Get PlayerBase using player steam64id
	*/
	PlayerBase GetPlayerBaseByID( string id )
	{
		autoptr array<Man> players = new array<Man>;
       	GetGame().GetWorld().GetPlayerList( players );
		
		for(int i = 0; i < players.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(players[i]);
			if(player)
			{
				if(player.VPlayerGetSteamId() == id || player.VPlayerGetHashedId() == id)
				{
					return player;
				}
			}
		}
		return NULL;
	}

	/*
		Get PlayerBase using player session ID
	*/
	PlayerBase GetPlayerBaseBySessionID( int id )
	{
		array<Man> players = new array<Man>;
       	GetGame().GetWorld().GetPlayerList(players);
		
		for(int i = 0; i < players.Count(); i++)
		{
			PlayerBase player = PlayerBase.Cast(players[i]);
			if(player)
			{
				if(player.VPlayerGetSessionId() == id)
				{
					return player;
				}
			}
		}
		return NULL;
	}
	
	/*
		GUID in out Steam64
	*/
	string ConvertGUIDToSteamID(string guid, out PlayerBase pb)
	{
		pb = GetPlayerBaseByID(guid);
		if (pb != null)
			return pb.VPlayerGetSteamId();

		return "";
	}
	
	//@Doc Check to see if caller is Super Admin.
	//@Usage For only super admin actions.
	bool IsSuperAdmin(string steam64Id)
	{
		return m_SuperAdmins.Find(steam64Id) >= 0;
	}
};

/*Global Getter for PermissionManager*/
PermissionManager GetPermissionManager()
{
	return PermissionManager.Cast(GetPluginManager().GetPluginByType(PermissionManager));
}