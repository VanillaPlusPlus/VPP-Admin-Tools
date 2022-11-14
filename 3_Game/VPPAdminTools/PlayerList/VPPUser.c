class VPPUser
{
	private string m_UserName;
	private string m_Steam64ID;
	private int    m_SessionId;
	
	void VPPUser(string name, string id, int sessionId = -1)
	{
		m_UserName  = name;
		m_Steam64ID = id;
		m_SessionId = sessionId;
	}
		
	string GetUserId()
	{
		return m_Steam64ID;
	}
	
	string GetUserName()
	{
		return m_UserName;
	}

	int GetSessionId()
	{
		return m_SessionId;
	}
};

/*
	ONLY FOR BACKWARD COMPAT, DATA TYPE CHANGES WOULD CAUSE CRASH UPON CONVERTION
*/
class Old_VPPUser
{
	private string m_UserName;
	private string m_Steam64ID;
	
	void Old_VPPUser(string name, string id)
	{
	}
};