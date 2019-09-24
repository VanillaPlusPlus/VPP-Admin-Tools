class SpawnedBuilding
{
	private string m_Name;
	private string m_ObjectName;
	private vector m_Position;
	private vector m_Direction;
	private vector m_Orientation;
	private bool   m_IsActive;
	[NonSerialized()]
	private Object m_Building;
	private string m_NetWorkId;

	void SpawnedBuilding(string name, vector pos, vector orientation, bool isAlive = true, Object obj = null)
	{
		m_IsActive = isAlive;
		m_ObjectName = name;
		m_Position = pos;
		m_Orientation = orientation;

		string posStr = Math.Floor(pos[0]).ToString() + Math.Floor(pos[1]).ToString() + Math.Floor(pos[2]).ToString();
		m_Name = name + "-" + posStr;
		
		if (obj != null)
			m_Building = obj;
		else
			SetActive(isAlive);
	}

	void ~SpawnedBuilding()
	{
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			if (m_Building != null){
			DestroySpawnedEntity();
			}else{
				TStringArray strs = new TStringArray;
				m_NetWorkId.Split( ",",strs );
				autoptr Object netWrkObj = GetGame().GetObjectByNetworkId(strs[1].ToInt(), strs[0].ToInt()); //low,high
				GetGame().ObjectDelete(netWrkObj);
				//if (netWrkObj != null)
					//GetRPCManager().SendRPC("RPC_BuildingSetManager","RemoteQuickDeleteObject", new Param1<Object>(netWrkObj),true,null);
			}
		}
	}
	
	string GetTypeName()
	{
		return m_ObjectName;
	}
	
	string GetName()
	{
		return m_Name;
	}
	
	Object GetObject()
	{
		return m_Building;
	}
	
	vector GetPosition()
	{
		return m_Position;
	}
	
	void SetPosition(vector pos)
	{
		m_Position = pos;
		
		if(m_Building)
			m_Building.SetPosition(pos);
	}
	
	vector GetOrientation()
	{
		return m_Orientation;
	}
	
	void SetOrientation(vector o)
	{
		m_Orientation = o;
		
		if(m_Building)
			m_Building.SetOrientation(o);
	}
	
	vector GetDirection()
	{
		return m_Direction;
	}

	void SetDirection(vector d)
	{
		m_Direction = d;
		
		if(m_Building)
			m_Building.SetDirection(d);
	}

	bool GetActive()
	{
		return m_IsActive;
	}
		
	void SetActive(bool active)
	{
		m_IsActive = active;

		if(!m_IsActive)
		{
			DestroySpawnedEntity();
		}

		if(m_IsActive)
		{
			if(m_Building == null)
			{
				SpawnObject();
			}
		}
	}
	
	void DestroySpawnedEntity()
	{
		if (m_Building != null)
			GetGame().ObjectDelete(m_Building);
	}
	
	bool IsObject(Object obj)
	{
		if(m_Building)
			return m_Building == obj;
		
		return m_ObjectName == obj.GetDisplayName();
	}
	
	string GetNetworkId(bool clean = false)
	{
		if (clean)
		{
			string edited = m_NetWorkId;
			edited.Replace(",", "");
			return edited;
		}
		return m_NetWorkId;
	}

	void SpawnObject()
	{
		m_Building = Object.Cast(GetGame().CreateObject(m_ObjectName, m_Position));
		if (m_Building == null) return;
		
		int low, high;
		m_Building.GetNetworkID( low, high );
		m_NetWorkId = high.ToString() + "," + low.ToString();
		if (m_Building.CanAffectPathgraph())
		{
			m_Building.SetAffectPathgraph(true, false);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().UpdatePathgraphRegionByObject, 100, false, m_Building);
			m_Building.SetPosition(m_Position);
			m_Building.SetOrientation(m_Orientation);
		}
	}
}