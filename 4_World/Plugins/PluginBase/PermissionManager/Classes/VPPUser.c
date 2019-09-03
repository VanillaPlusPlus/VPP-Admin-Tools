class VPPUser
{
	private string m_UserName;
	private string m_Steam64ID;
	
	void VPPUser(string name, string id)
	{
		m_UserName  = name;
		m_Steam64ID = id;
	}
		
	string GetUserId()
	{
		return m_Steam64ID;
	}
	
	string GetUserName()
	{
		return m_UserName;
	}
};