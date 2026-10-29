// Managed: held by its set's array and by client entries / RPC Params (see BuildingSet).
// Runtime / RPC class only: set files are VPPBuildingSetFile JSON (VPPBuildingSetStore), so members may change; the
// server and its clients always run the same version.
class SpawnedBuilding : Managed
{
	private string m_Name;
	private string m_ObjectName;
	private vector m_Position;
	private vector m_Orientation;
	private bool   m_IsActive;
	private float  m_Scale;
	[NonSerialized()]
	private Object m_Building;
	private string m_NetWorkId;

	void SpawnedBuilding(string name, vector pos, vector orientation, bool isAlive = true, Object obj = null)
	{
		m_IsActive = isAlive;
		m_ObjectName = name;
		m_Position = pos;
		m_Orientation = orientation;
		m_Scale = 1.0;

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
				//only an object of this type that carries the id (an old id may belong to something else now)
				Object netWrkObj = FindByNetworkId();
				if (netWrkObj != null)
					GetGame().ObjectDelete(netWrkObj);
				//if (netWrkObj != null)
					//GetRPCManager().VSendRPC("RPC_BuildingSetManager","RemoteQuickDeleteObject", new Param1<Object>(netWrkObj),true,null);
			}
		}

		if(GetGame().IsClient())
		{
			if(m_Building != null)
				GetGame().ObjectDeleteOnClient(m_Building);
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
	
	float GetScale()
	{
		if (m_Scale < VPPBuildingSetStore.SCALE_MIN || m_Scale > VPPBuildingSetStore.SCALE_MAX)
			return 1.0;

		return m_Scale;
	}

	// Clamped; the placed object (client preview or spawned server object) follows at once.
	void SetScale(float scale)
	{
		m_Scale = Math.Clamp(scale, VPPBuildingSetStore.SCALE_MIN, VPPBuildingSetStore.SCALE_MAX);
		if (m_Building)
		{
			m_Building.SetScale(m_Scale);
			m_Building.Update();
		}
	}

	// Loading a saved set: the stored state without spawning anything (SpawnActiveBuildings spawns afterwards).
	void LoadState(bool active, float scale)
	{
		m_IsActive = active;
		m_Scale = Math.Clamp(scale, VPPBuildingSetStore.SCALE_MIN, VPPBuildingSetStore.SCALE_MAX);
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

		m_NetWorkId = "";
	}

	void ClearNetworkId()
	{
		m_NetWorkId = "";
	}

	// The object with the saved network id when it is of this building's type, else null.
	Object FindByNetworkId()
	{
		TStringArray strs = new TStringArray;
		m_NetWorkId.Split(",", strs);
		if (strs.Count() < 2)
			return null;

		Object found = GetGame().GetObjectByNetworkId(strs[1].ToInt(), strs[0].ToInt()); //low,high
		if (found == null)
			return null;

		string foundType = found.GetType();
		string ownType = m_ObjectName;
		foundType.ToLower();
		ownType.ToLower();
		if (foundType != ownType)
			return null;

		return found;
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
		m_Building = GetGame().CreateObjectEx(m_ObjectName, m_Position, ECE_SETUP|ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH);
		if (!m_Building)
			return;
		
		int low, high;
		m_Building.GetNetworkID(low, high);
		m_NetWorkId = high.ToString() + "," + low.ToString();
		m_Building.SetOrientation(m_Orientation);
		ApplySpawnScale();
		if (m_Building.CanAffectPathgraph())
		{
			m_Building.SetAffectPathgraph(true, false);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().UpdatePathgraphRegionByObject, 100, false, m_Building);
		}
		m_Building.Update();
	}

	// The engine only sends an object's scale inside its create message, and a client already in range gets that
	// message from CreateObjectEx itself, before SetScale runs (a set saved while players are online came back at scale
	// 1; one spawned at boot was fine). So the clients that already have it get one RPC; later creates (late joiners,
	// bubble re-entry) carry the scale themselves.
	protected void ApplySpawnScale()
	{
		float scale = GetScale();
		if (scale == 1.0)
			return;

		m_Building.SetScale(scale);
		if (GetGame().IsMultiplayer())
		{
			Param1<float> scaleParam = new Param1<float>(scale);
			GetRPCManager().VSendRPC("RPC_MissionGameplay", "ApplyObjectScale", scaleParam, true, null, m_Building);
		}
	}

	void SetRef(Object obj)
	{
		m_Building = obj;
	}
}