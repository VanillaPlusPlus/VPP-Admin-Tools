class VPPTeleportLocation
{
    private string m_Name;
    private vector m_Position;
    
    void VPPTeleportLocation(string name, vector pos)
    {
        m_Name = name;
        m_Position = pos;
    }

    string GetName()
    {
        return m_Name;
    }

    vector GetLocation()
    {
        return m_Position;
    }
	
	void Setlocation(vector pos)
	{
		m_Position = pos;
	}
	
	void SetName(string name)
	{
		m_Name = name;
	}
}