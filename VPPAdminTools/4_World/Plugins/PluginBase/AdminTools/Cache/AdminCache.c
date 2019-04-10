class AdminCache {
	private string m_GUID = "";
	private string m_savedName = "";
	private ref array<ref VPPPermission> m_Perms;

	void AdminCache(string guid, string savedname, ref array<ref VPPPermission> Perms)	{
		m_GUID = guid;
		m_savedName = savedname;
		if (Perms) { m_Perms = Perms; }else{ m_Perms = createDefaultPerms(); }
	}

	private static ref array<ref VPPPermission> createDefaultPerms(){

		ref array<ref VPPPermission> permissions = new array<ref VPPPermission>;
		ref array<string> names = {
			"PlayerManager",
			"ItemManager",
			"ServerManager",
			"ObjectEditor",
			"WeatherManager",
		};

		foreach(string s : names){
			permissions.Insert(new VPPPermission(s, 1));	
		}
		return permissions;
	}

	void setPermissionArray(ref array<string> Permissions){
		for (int i = 0; i < m_Perms.Count(); ++i)
	    {
	    	ref VPPPermission perm = m_Perms.Get(i);
    	    for (int x = 0; x < Permissions.Count(); ++x)
    	    {
    	    	if(perm.getName() == Permissions.Get(x))
    	    	{
				  perm.setState(true);
				}
    	    }
	    }
	}

	void setPermission(string name, bool newState){
		foreach(VPPPermission perm : m_Perms){
			if(perm.getName() == name){
				perm.setState(newState);
			}
		}
	}

	bool hasPermission(string name){
		foreach(VPPPermission perm : m_Perms){
			if(perm.getName() == name){
				if(perm.getState()){
					return true;
				}
			}
		}
		return false;
	}

	ref array<string> getAllPermsByGUID(string GUID)
	{
		ref array<string> Perms = new array<string>;
		foreach(VPPPermission perm : m_Perms){
			if(getGUID() == GUID){
				if(perm.getState()){
					Perms.Insert(perm.getName());
				}
			}
		}
		return Perms;
	}

	string getGUID() { return m_GUID; }
	void setGUID(string guid) { m_GUID = guid; }
	
	string getName() { return m_savedName; }
	void setName(string name) { m_savedName = name; }
}