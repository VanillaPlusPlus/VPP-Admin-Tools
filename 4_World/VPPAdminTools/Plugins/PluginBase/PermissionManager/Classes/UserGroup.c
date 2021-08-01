class UserGroup
{
	private string 		  			   m_groupName;
	private int    		  			   m_permissionLevel;
	private ref array<ref VPPUser>     m_GroupMembers;
	private ref array<string>  		   m_Permissions;
	private bool                       m_ForceSavedName;
	
	void UserGroup(string groupName, int permissionLevel, array<string> permissioncatagorys)
	{
		m_groupName 	   	  = groupName;
		m_permissionLevel 	  = permissionLevel;
		m_GroupMembers        = new array<ref VPPUser>;
		m_Permissions 		  = permissioncatagorys;
	}
	
	void SetGroupName(string name)
	{
		m_groupName = name;
	}
	
	string GetGroupName()
	{
		return m_groupName;
	}

	bool IsForceSavedName()
	{
		return m_ForceSavedName;
	}

	void SetForceSavedName(bool state)
	{
		m_ForceSavedName = state;
	}
	
	void SetPermissionLevel(int level)
	{
		if(level >= 0){
			m_permissionLevel = level;
			return;
		}
		
		m_permissionLevel = 0;
	}
	
	int GetPermissionLevel()
	{
		return m_permissionLevel;
	}
	
	void ClearMembers()
	{
		m_GroupMembers.Clear();
		m_GroupMembers  = new array<ref VPPUser>;
	}
	
	array<ref VPPUser> GetMembers()
	{
		return m_GroupMembers;
	}
	
	bool HasMember(string id)
	{
		foreach(VPPUser user : m_GroupMembers)
		{
			if (user.GetUserId() == id)
			{
				return true;
			}
		}
		return false;
	}
	
	void AddMembers(array<ref VPPUser> users)
	{
		foreach(VPPUser user: users)
		{
			if (!HasMember(user.GetUserId()))
			{
				m_GroupMembers.Insert(user);
			}
		}
	}
	
	VPPUser FindUser(string id)
	{
		if (HasMember(id))
		{
			foreach(VPPUser user : m_GroupMembers){
				if (user.GetUserId() == id) 
					return user;
			}
		}
		return null;
	}
	
	void RemoveMembers(string ID)
	{
		VPPUser member;		
		member = FindUser(ID);
		if (member != null)
		{
			m_GroupMembers.RemoveItem(member);
		}
	}
	
	void SetPermissions(array<string> Permissions)
	{
		m_Permissions = Permissions;
	}
	
	array<string> GetPermissions()
	{
		return m_Permissions;
	}
};

/*
	ONLY FOR BACKWARD COMPAT, DATA TYPE CHANGES WOULD CAUSE CRASH UPON CONVERTION
*/
class Old_UserGroup
{
	private string 		  			   m_groupName;
	private int    		  			   m_permissionLevel;
	private ref array<ref Old_VPPUser> m_GroupMembers;
	private ref array<string>  		   m_Permissions;
	private bool                       m_ForceSavedName;

	void UserGroup(string groupName, int permissionLevel, array<string> permissioncatagorys)
	{
	}
};