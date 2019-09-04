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
	private const string m_UserGroupsSavePath  = "$profile:VPPAdminTools/Permissions/UserGroups/UserGroups.vpp";
	
	private ref array<string> 		  	  m_Permissions;
	private ref array<ref UserGroup> 	  m_UserGroups;
	
	void PermissionManager()
	{
		m_UserGroups = new array<ref UserGroup>;
		m_Permissions = new array<string>;
		m_SuperAdmins = new array<string>;
		
		// @Deprecated
		// Chat commands will add permissions when added to the chat command manager.
		AddPermissionType({ "Chat:StripPlayer", "Chat:KillPlayer", "Chat:GiveAmmo", "Chat:SpawnInventory", "Chat:SpawnCar", "Chat:SpawnOnGround", "Chat:SpawnInHands", "Chat:HealPlayer", "Chat:BringPlayer", "Chat:ReturnPlayer"});
		AddPermissionType({	"Chat:TeleportToTown", "Chat:TeleportToPoint", "Chat:GotoPlayer","Chat:BanPlayer","Chat:UnbanPlayer","Chat:refuelCar"});

		//Misc
		AddPermissionType({"DeleteObjectAtCrosshair","TeleportToCrosshair","FreeCamera"});
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
		AddPermissionType({ "MenuPlayerManager","PlayerManager:BanPlayer","PlayerManager:KickPlayer","PlayerManager:HealPlayers","PlayerManager:SetPlayerStats","PlayerManager:KillPlayers","PlayerManager:GodMode","PlayerManager:SpectatePlayer","PlayerManager:TeleportToPlayer","PlayerManager:TeleportPlayerTo","PlayerManager:SetPlayerInvisible","PlayerManager:SendMessage" });
		//Bans Manager
		AddPermissionType({ "MenuBansManager","BansManager:UnbanPlayer","BansManager:UpdateBanDuration","BansManager:UpdateBanReason" });
		//Log Viewer Menu
		AddPermissionType({ "MenuLogsViewer","LogsViewer:ViewLogs" });
		//Teleport Manager Menu
		AddPermissionType({ "MenuTeleportManager","TeleportManager:TPPlayers","TeleportManager:TPSelf","TeleportManager:DeletePreset","TeleportManager:AddNewPreset","TeleportManager:EditPreset" });
		//ESP tools menu 
		AddPermissionType({ "EspToolsMenu","EspToolsMenu:DeleteObjects","EspToolsMenu:PlayerESP" });
		
		//-----RPC's-----
		GetRPCManager().AddRPC("RPC_PermitManager", "EnableToggles", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PermitManager", "VerifyButtonsPermission", this, SingeplayerExecutionType.Server );
		GetRPCManager().AddRPC("RPC_PermissionManager","SendToClient", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteCreateUserGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteDeleteUserGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","UpdateUserGroupPermLvl", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteUpdateGroupPerms", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteAddUsersToGroup", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_PermissionManager","RemoteRemoveUserFromGroup", this, SingleplayerExecutionType.Server);
		//---------------
	}

	void ~PermissionManager()
	{
		
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
		if( GetGame().IsServer() && GetGame().IsMultiplayer() )
		{
			autoptr FileSerializer file = new FileSerializer();
			if (file.Open(m_UserGroupsSavePath, FileMode.WRITE))
			{
			    file.Write(m_UserGroups);
			    file.Close();
			}
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
				return;
        	}
			GetSimpleLogger().Log("[PermissionManager]:: EnableToggles(): "+sender+" Identity Attempted 'EnableToggles' Rejected: Not admin/part of a usergroup.");
        }
	}
	
	void VerifyButtonsPermission(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref array<string>> data;
		if (!ctx.Read(data)) return;
		
		if( type == CallType.Server && sender != null)
        {
			autoptr array<string> tmp = new array<string>;
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
		ref map<string,bool> permissions = new map<string,bool>;
		foreach(string perm : permsToUpdate)
		{
			permissions.Insert(perm,VerifyPermission(targetId,perm));
		}
		autoptr PlayerIdentity pidentity = GetIdentityById(targetId);
		if (pidentity != null)
			GetRPCManager().SendRPC( "RPC_VPPAdminHud", "VerifyButtonsPermission", new Param1<ref map<string,bool>>(permissions), true, pidentity);
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
			CreateUserGroup( data.param1, 0, {} );
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
			RemoveUserGroupByName(data.param1);
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
			if (data.param1 > -1 && data.param2 != "")
			{
				//Find group
				foreach(UserGroup group : m_UserGroups)
				{
					if (group.GetGroupName() == data.param2)
					{
						group.SetPermissionLevel(data.param1);
						Save();
						GetSimpleLogger().Log("[PermissionManager]:: UpdateUserGroupPermLvl(): Group: "+group.GetGroupName()+" Permission level changed to: "+data.param1);
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
		
		autoptr array<string> tempData = data.param1;
		if (type == CallType.Server)
		{
			foreach(UserGroup group : m_UserGroups)
			{
				if (group.GetGroupName() == data.param2)
				{
					group.SetPermissions(tempData);
					GetSimpleLogger().Log("[PermissionManager]:: RemoteUpdateGroupPerms(): Group: "+group.GetGroupName()+" Permissions updated");
					autoptr array<ref VPPUser> grpMembers = group.GetMembers();
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
		
		autoptr map<string,string> tmp = data.param1;
		tmp.Copy(data.param1);
		if (type == CallType.Server)
		{
			autoptr array<ref VPPUser> users = new array<ref VPPUser>;
			foreach(string id, string name : tmp)
			{
				users.Insert(new VPPUser(name,ConvertGUIDToSteamID(id)));
			}
			foreach(ref VPPUser user : users)
			{
				autoptr PlayerIdentity pid = GetIdentityById(user.GetUserId());
				AddMembersToGroup(user, data.param2, sender.GetPlainId());
	   			GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(true), true, pid);
				GetPlayerListManager().SendPlayerList(pid);
			}
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
			foreach(ref UserGroup group : m_UserGroups)
			{
				if (group.FindUser(data.param1))
				{
					group.RemoveMembers(data.param1);
					NotifyPlayer(sender.GetPlainId(),"Removing User: "+data.param1+" from group: "+group.GetGroupName(),NotifyTypes.NOTIFY);
					GetSimpleLogger().Log("[PermissionManager]:: RemoteRemoveUserFromGroup(): Removing User: "+data.param1+" from group: "+group.GetGroupName());
					Save();
					autoptr PlayerIdentity tgIdentity = GetIdentityById(data.param1);
					if (tgIdentity != null)
						GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(false), true, tgIdentity);
					
					return;
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
		GetSimpleLogger().Log("[PermissionManager]:: CreateUserGroup(): User Group: "+groupName+" Permission Level: "+permissionLevel+" created!");
	}

	void AddMembersToGroup(ref VPPUser user, string groupName, string id = "")
	{
		if (groupName == "")
		{
			GetSimpleLogger().Log("[PermissionManager]:: AddMembersToGroup(): Error, invalid group name");
			return;
		}
		
		if(IsSuperAdmin(user.GetUserId()))
		{
			GetSimpleLogger().Log("[Permission Manager]:: AddMembersToGroup(): User is Super Admin.");
			
			if(id != string.Empty)
				NotifyPlayer(id, "Error: User is a Super Admin", NotifyTypes.ERROR);
			
			return;
		}
		
		autoptr UserGroup selectedGroup;
		autoptr UserGroup currentGroup;
	
		foreach(ref UserGroup group : m_UserGroups)
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
			GetSimpleLogger().Log("[Permission Manager]:: AddMembersToGroup(): Invalid Group.");
			
			if(id != string.Empty)
				NotifyPlayer(id, "Error: Invalid UserGroup " + groupName, NotifyTypes.ERROR);
			
			return;
		}
		
		if(selectedGroup == currentGroup)
		{
			GetSimpleLogger().Log("[Permission Manager]:: AddMembersToGroup(): User is already in that usergroup.");
			
			if(id != string.Empty)
				NotifyPlayer(id, "Error: User: " + user.GetUserId() + " is already inside of group: " + groupName, NotifyTypes.ERROR);
			
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
				GetSimpleLogger().Log("[PermissionManager]:: AddPermissionType(): Adding " + str + " to permission collection.");
				m_Permissions.Insert(str);
				continue;
			}
			
			GetSimpleLogger().Log("[PermissionManager]:: AddPermissionType(): Tried adding duplicate permission, " + str + ", to permission collection.");
		}
	}
	
	private void CreateDefualtUserGroups()
	{
		CreateUserGroup( "Admins", 1, m_Permissions );
		AddMembersToGroup(new VPPUser("Fake User 1","76561198321354734"), "Admins");
		AddMembersToGroup(new VPPUser("Fake User 2","76561198321354724"), "Admins");
		AddMembersToGroup(new VPPUser("Fake User 3","76561198321354754"), "Admins");
		AddMembersToGroup(new VPPUser("Fake User 4","765611983213547664"), "Admins");
		AddMembersToGroup(new VPPUser("DaOne","76561198420222029"), "Admins");
		
	    GetSimpleLogger().Log("[PermissionManager]:: CreateDefualtUserGroups(): Created defualt UserGroups.vpp");
	}
	
	private void RemoveUserGroupByName(string groupname)
	{
		foreach(UserGroup group: m_UserGroups)
		{
			if (group != null && group.GetGroupName() == groupname)
			{
				foreach(ref VPPUser user : group.GetMembers())
				{
		   			GetRPCManager().SendRPC( "RPC_MissionGameplay", "EnableToggles", new Param1<bool>(false), true, GetIdentityById(user.GetUserId()));
				}
				m_UserGroups.RemoveItem(group);
				delete group;
				Save();
				GetSimpleLogger().Log("[PermissionManager]:: RemoveUserGroupByName(): Removed Group: "+groupname+" sucessfully!");
			}
		}
	}
	
	private void LoadUserGroups()
	{
		if (FileExist(m_UserGroupsSavePath))
		{
			FileSerializer file = new FileSerializer();
			if (file.Open(m_UserGroupsSavePath, FileMode.READ))
			{
			    file.Read(m_UserGroups);
			    file.Close();
			    GetSimpleLogger().Log("[PermissionManager]:: LoadUserGroups(): Loading UserGroups.vpp.");
			}
		}else
		{
			CreateDefualtUserGroups();
		}
	}
	
	private void LoadSuperAdmins()
	{
		FileHandle adminUIDSFile;
		autoptr array<string> m_ContentLines = new array<string>;
		
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
				GetSimpleLogger().Log("[PermissionManager]:: LoadSuperAdmins(): Adding Super Admin: "+ trimmed + ".");
			}
		}
		else 
		{
			GetSimpleLogger().Log("[PermissionManager]:: Error: File Missing... Creating Defaults. Please change SuperAdmins.txt file, and restart the server.");
			adminUIDSFile = OpenFile(m_SuperAdminsListPath ,FileMode.WRITE);
			FPrintln(adminUIDSFile, "76561198420222029");
			FPrintln(adminUIDSFile, "76561198321354754");
			m_SuperAdmins.Insert("76561198420222029");
			m_SuperAdmins.Insert("76561198321354754");
		}
		CloseFile(adminUIDSFile);
	}
		
	bool HasUserGroup(string id)
	{
		foreach(ref UserGroup group : m_UserGroups)
			if(group.HasMember(id))
				return true;

		return false || IsSuperAdmin(id);
	}
	
	// @Doc:
	// @Usage: Check permission to user group. 
	// @Param: string callerID, string permissionName, string TargetID
	// @Target ID should match be the same as target if there isn't a target.
	
	bool VerifyPermission(string id, string permissionName, string targetID = "")
	{
		if (m_Permissions.Find(permissionName) <= -1) //Reject permission no matter what if wrong perm type was received.
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", Target: Self, Unregistred/Unknown Permission type! Rejected.");
			NotifyPlayer(id,"The permission "+permissionName+" is unknown/unregistered!",NotifyTypes.ERROR);
			return false;
		}
		
		if(!HasUserGroup(id))
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", TargetID: " + targetID + ", Permission: " + permissionName + ", Value: false");
			NotifyPlayer(id,"You are not an admin or belong to a usergroup.",NotifyTypes.PERMISSION_REJECT);
			return false;
		}
		
		if(IsSuperAdmin(id) && !IsSuperAdmin(targetID) && id != targetID)
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", TargetID: " + targetID + ", Permission: " + permissionName + ", Value: true");
			return true;
		}
		
		if(IsSuperAdmin(targetID) && !IsSuperAdmin(id))
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", TargetID: " + targetID + ", Permission: " + permissionName + ", Value: false");
			NotifyPlayer(id,"You cannot target Super Admins with: " + permissionName, NotifyTypes.PERMISSION_REJECT);
			return false;
		}
		
		if(IsSuperAdmin(id) && targetID == id)
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", Target: Self, Permission: " + permissionName + ", Value: true");
			return true;
		}
		
		if(IsSuperAdmin(id) && IsSuperAdmin(targetID))
		{
			GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", Target: Self, Permission: " + permissionName + ", Value: true");
			return true;
		}

		int targetPermLevel = int.MAX;
		int senderPermLevel = int.MAX;
		
		foreach(ref UserGroup group : m_UserGroups)
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
						GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + " Permission: " + permissionName + ", Value: true");
						return true;
					}
				}
				
				if(group.GetPermissions().Find(permissionName) <= -1)
				{
					GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", TargetID: " + targetID + ", Permission: " + permissionName + ", Value: false");
					NotifyPlayer(id,"You don't have the following permission: " + permissionName, NotifyTypes.PERMISSION_REJECT);
					return false;
				}
			}
		}
		
		bool hasPermission = senderPermLevel < targetPermLevel || (targetID == id && senderPermLevel == targetPermLevel);
		if (!hasPermission && GetIdentityById(targetID) != null)
		{
			NotifyPlayer(id,"Permission level too low to exectue: " + permissionName + " On: "+GetIdentityById(targetID).GetName(), NotifyTypes.PERMISSION_REJECT);
		}
		GetSimpleLogger().Log("[PermissionManager]:: VerifyPermission() : Sender ID: " + id + ", TargetID : " + targetID + ", Permission: " + permissionName + ", Value: " + hasPermission);
		
		return hasPermission;
	}
	
	void NotifyPlayer(string id,string msg, int type)
	{
		switch(type)
		{
			case NotifyTypes.ERROR:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( GetIdentityById(id), 5.0, "Error: ", msg, "set:ccgui_enforce image:Icon40Emergency" );
			break;
			
			case NotifyTypes.NOTIFY:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( GetIdentityById(id), 8.0, "Notification: ", msg, "set:ccgui_enforce image:MapUserMarker" );
			break;
			
			case NotifyTypes.PERMISSION_REJECT:
				NotificationSystem.SendNotificationToPlayerIdentityExtended( GetIdentityById(id), 5.0, "Permission Rejected: ", msg, "set:ccgui_enforce image:Icon40Emergency" );
			break;
		}
	}
	
	/*
		Works with steam64ID and or GUID
	*/
	string GetPlayerUserGrpNameByID(string id)
	{
		autoptr PlayerIdentity cid = GetIdentityById(id);
		if (cid == null) return "None";
		
		if (IsSuperAdmin(cid.GetPlainId()))
		{
			return "Super Admin";
		}
		else
		{
			foreach(ref UserGroup group : m_UserGroups)
			{
				if (!group.HasMember(cid.GetPlainId()))
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
		
       	for (int i = 0; i < players.Count(); ++i)
       	{
			if (players.Get(i).GetIdentity().GetPlainId() == id || players.Get(i).GetIdentity().GetId() == id)
			{
				return players.Get(i).GetIdentity();
			}
		}
		return null;
	}
	
	/*
		Get PlayerBase using player steam64id
	*/
	PlayerBase GetPlayerBaseByID( string id )
	{
		autoptr PlayerIdentity identity = GetIdentityById(id);
	    if ( identity == NULL ) return NULL;
	
	    int networkIdLowBits;
	    int networkIdHighBits;
	    GetGame().GetPlayerNetworkIDByIdentityID( identity.GetPlayerId(), networkIdLowBits, networkIdHighBits );
	
	    return PlayerBase.Cast( GetGame().GetObjectByNetworkId( networkIdLowBits, networkIdHighBits ) );
	}
	
	/*
		GUID in out Steam64
	*/
	string ConvertGUIDToSteamID(string guid)
	{
		autoptr PlayerIdentity identity = GetIdentityById(guid);
		if (identity != null)
		{
			return identity.GetPlainId();
		}
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