/*
	Unified context-menu target (crosshair object, player-list rows, API).
	T1: Kind, Source, NetLow/NetHigh, TypeName, stored Position, PlayerIds and SessionIds are the STORED
	    identity. They are set by the factories (client) or copied verbatim by FromRequest (server) and are
	    only changed afterwards by SetPlayers. GetKey() reads stored fields only, so client and server
	    compute the same key for the same request.
	T2: when SetPlayers changes the primary id, the object and net ids are dropped and the primary is
	    re-resolved, so GetObject()/GetPlayer() never return a player other than GetPlayerId().
*/
class VPPContextTarget : Managed
{
	protected int    m_Kind;
	protected int    m_Source;
	protected int    m_NetLow;
	protected int    m_NetHigh;
	protected Object m_Object; //weak
	protected string m_TypeName;
	protected vector m_Position;
	protected string m_PlayerId;
	protected int    m_SessionId;
	protected string m_DisplayName;
	protected ref array<string>     m_PlayerIds;
	protected ref array<int>        m_SessionIds;
	protected ref map<string,string> m_Extra;
	protected ref map<string,string> m_State; //client-only, never serialized

	void VPPContextTarget()
	{
		m_Kind       = EVPPContextTargetKind.NONE;
		m_SessionId  = -1;
		m_PlayerIds  = new array<string>;
		m_SessionIds = new array<int>;
		m_Extra      = new map<string,string>;
		m_State      = new map<string,string>;
	}

	static VPPContextTarget ForObject(Object obj, int source = 1 /*EVPPContextSource.CROSSHAIR*/)
	{
		VPPContextTarget t = new VPPContextTarget();
		t.m_Kind   = EVPPContextTargetKind.OBJECT;
		t.m_Source = source;
		if (!obj)
			return t;

		int low;
		int high;
		obj.GetNetworkID(low, high);
		t.m_NetLow      = low;
		t.m_NetHigh     = high;
		t.m_TypeName    = obj.GetType();
		t.m_Position    = obj.GetPosition();
		t.m_DisplayName = VPPContextUtils.GetObjectDisplayName(obj);
		t.m_Object      = obj;

		//dead bodies and players with an unknown steam64 stay OBJECT
		if (!VPPContextUtils.IsAliveMan(obj))
			return t;

		PlayerBase pb = PlayerBase.Cast(obj);
		if (!pb || !pb.GetIdentity())
			return t;

		string sid = VPPContextUtils.GetSteam64ForPlayer(pb);
		if (sid == "")
			return t;

		t.m_Kind        = EVPPContextTargetKind.PLAYER;
		t.m_PlayerId    = sid;
		t.m_SessionId   = VPPContextUtils.GetSessionIdForPlayer(pb);
		t.m_DisplayName = pb.GetIdentity().GetName();
		t.m_PlayerIds.Insert(sid);
		t.m_SessionIds.Insert(t.m_SessionId);
		return t;
	}

	static VPPContextTarget ForPlayer(string steam64, int sessionId, string displayName, int source = 2 /*EVPPContextSource.PLAYER_LIST*/)
	{
		VPPContextTarget t = new VPPContextTarget();
		t.m_Kind        = EVPPContextTargetKind.PLAYER;
		t.m_Source      = source;
		t.m_PlayerId    = steam64;
		t.m_SessionId   = sessionId;
		t.m_DisplayName = displayName;
		t.m_PlayerIds.Insert(steam64);
		t.m_SessionIds.Insert(sessionId);

		if (!GetGame().IsDedicatedServer())
		{
			PlayerBase pb = VPPContextUtils.FindPlayerBySteam64Client(steam64);
			if (pb)
				t.AdoptPlayerObject(pb);
		}
		return t;
	}

	static VPPContextTarget ForPlayers(array<string> steam64s, array<int> sessionIds, string primaryId, int primarySessionId, string primaryName, int source = 2 /*EVPPContextSource.PLAYER_LIST*/)
	{
		string firstId = primaryId;
		int firstSession = primarySessionId;
		if (firstId == "" && steam64s && steam64s.Count() > 0)
		{
			firstId = steam64s.Get(0);
			firstSession = -1;
			if (sessionIds && sessionIds.Count() > 0)
				firstSession = sessionIds.Get(0);
		}

		VPPContextTarget t = ForPlayer(firstId, firstSession, primaryName, source);
		if (!steam64s)
			return t;

		for (int i = 0; i < steam64s.Count(); i++)
		{
			string id = steam64s.Get(i);
			if (id == "" || t.m_PlayerIds.Find(id) != -1)
				continue;

			int sess = -1;
			if (sessionIds && i < sessionIds.Count())
				sess = sessionIds.Get(i);

			t.m_PlayerIds.Insert(id);
			t.m_SessionIds.Insert(sess);
		}
		return t;
	}

	//server-side resolution: copies the stored fields verbatim, resolves only the object (and player name)
	static VPPContextTarget FromRequest(VPPContextRequest req)
	{
		VPPContextTarget t = new VPPContextTarget();
		if (!req)
			return t;

		t.m_Kind        = req.Kind;
		t.m_Source      = req.Source;
		t.m_NetLow      = req.NetLow;
		t.m_NetHigh     = req.NetHigh;
		t.m_TypeName    = req.TypeName;
		t.m_Position    = req.Position;
		t.m_PlayerId    = req.PlayerId;
		t.m_SessionId   = req.SessionId;
		t.m_DisplayName = req.DisplayName;
		if (req.PlayerIds)
			t.m_PlayerIds.Copy(req.PlayerIds);
		if (req.SessionIds)
			t.m_SessionIds.Copy(req.SessionIds);
		if (req.Extra)
			t.m_Extra.Copy(req.Extra);

		if (t.m_Kind == EVPPContextTargetKind.PLAYER)
		{
			PermissionManager pm = GetPermissionManager();
			if (pm && req.PlayerId != "")
			{
				PlayerBase pb = pm.GetPlayerBaseByID(req.PlayerId);
				if (pb)
				{
					t.m_Object = pb;
					if (pb.GetIdentity())
						t.m_DisplayName = pb.GetIdentity().GetName();
				}
			}
			return t;
		}

		if (req.NetLow != 0 || req.NetHigh != 0)
		{
			Object netObj = GetGame().GetObjectByNetworkId(req.NetLow, req.NetHigh);
			if (netObj && (req.TypeName == "" || netObj.GetType() == req.TypeName))
				t.m_Object = netObj;
			return t;
		}

		if (req.TypeName != "")
			t.m_Object = FindStaticObject(req.TypeName, req.Position);

		return t;
	}

	void WriteToRequest(VPPContextRequest req)
	{
		if (!req)
			return;

		if (m_Object && m_NetLow == 0 && m_NetHigh == 0)
		{
			int low;
			int high;
			m_Object.GetNetworkID(low, high);
			m_NetLow  = low;
			m_NetHigh = high;
		}

		req.Kind        = m_Kind;
		req.Source      = m_Source;
		req.NetLow      = m_NetLow;
		req.NetHigh     = m_NetHigh;
		req.TypeName    = m_TypeName;
		req.Position    = m_Position;
		req.PlayerId    = m_PlayerId;
		req.SessionId   = m_SessionId;
		req.DisplayName = m_DisplayName;

		req.PlayerIds = new array<string>;
		req.PlayerIds.Copy(m_PlayerIds);
		req.SessionIds = new array<int>;
		req.SessionIds.Copy(m_SessionIds);
		req.Extra = new map<string,string>;
		req.Extra.Copy(m_Extra);
	}

	VPPContextTarget Retarget(Object obj)
	{
		return ForObject(obj, m_Source);
	}

	//same GetKey(); used for drill-in pages that carry Extras (m_State is not copied)
	VPPContextTarget CloneTarget()
	{
		VPPContextTarget t = new VPPContextTarget();
		t.m_Kind        = m_Kind;
		t.m_Source      = m_Source;
		t.m_NetLow      = m_NetLow;
		t.m_NetHigh     = m_NetHigh;
		t.m_Object      = m_Object;
		t.m_TypeName    = m_TypeName;
		t.m_Position    = m_Position;
		t.m_PlayerId    = m_PlayerId;
		t.m_SessionId   = m_SessionId;
		t.m_DisplayName = m_DisplayName;
		t.m_PlayerIds.Copy(m_PlayerIds);
		t.m_SessionIds.Copy(m_SessionIds);
		t.m_Extra.Copy(m_Extra);
		return t;
	}

	int GetKind()
	{
		return m_Kind;
	}

	int GetSource()
	{
		return m_Source;
	}

	Object GetObject()
	{
		if (m_Object)
			return m_Object;

		if (m_NetLow != 0 || m_NetHigh != 0)
		{
			Object o = GetGame().GetObjectByNetworkId(m_NetLow, m_NetHigh);
			if (o && o.GetType() == m_TypeName && IsObjectOfPrimary(o))
				m_Object = o;
		}
		return m_Object;
	}

	EntityAI GetEntity()
	{
		return EntityAI.Cast(GetObject());
	}

	ItemBase GetItem()
	{
		return ItemBase.Cast(GetObject());
	}

	Weapon_Base GetWeapon()
	{
		return Weapon_Base.Cast(GetObject());
	}

	PlayerBase GetPlayer()
	{
		if (m_Kind != EVPPContextTargetKind.PLAYER)
			return null;

		PlayerBase pb = PlayerBase.Cast(GetObject());
		if (pb)
			return pb;

		if (m_PlayerId == "")
			return null;

		if (GetGame().IsServer() && GetPermissionManager())
			return GetPermissionManager().GetPlayerBaseByID(m_PlayerId);

		return VPPContextUtils.FindPlayerBySteam64Client(m_PlayerId);
	}

	bool HasObject()
	{
		return GetObject() != null;
	}

	bool IsPlayer()
	{
		return m_Kind == EVPPContextTargetKind.PLAYER;
	}

	bool IsMulti()
	{
		return GetPlayerCount() > 1;
	}

	int GetPlayerCount()
	{
		return m_PlayerIds.Count();
	}

	string GetPlayerId()
	{
		return m_PlayerId;
	}

	int GetSessionId()
	{
		return m_SessionId;
	}

	array<string> GetPlayerIds()
	{
		return m_PlayerIds;
	}

	array<int> GetSessionIds()
	{
		return m_SessionIds;
	}

	//SECURITY (T2): the dispatcher level filter relies on the primary being re-resolved here
	void SetPlayers(array<string> steam64s, array<int> sessionIds)
	{
		string oldPrimary = m_PlayerId;

		//copy first: the arguments may alias m_PlayerIds / m_SessionIds
		array<string> newIds = new array<string>;
		array<int> newSessions = new array<int>;
		if (steam64s)
			newIds.Copy(steam64s);

		for (int i = 0; i < newIds.Count(); i++)
		{
			int sess = -1;
			if (sessionIds && i < sessionIds.Count())
				sess = sessionIds.Get(i);
			newSessions.Insert(sess);
		}

		m_PlayerIds  = newIds;
		m_SessionIds = newSessions;

		m_PlayerId  = "";
		m_SessionId = -1;
		if (m_PlayerIds.Count() > 0)
		{
			m_PlayerId  = m_PlayerIds.Get(0);
			m_SessionId = m_SessionIds.Get(0);
		}

		if (m_PlayerId == oldPrimary)
			return;

		m_Object  = null;
		m_NetLow  = 0;
		m_NetHigh = 0;

		PlayerBase pb = null;
		if (GetGame().IsServer() && GetPermissionManager() && m_PlayerId != "")
			pb = GetPermissionManager().GetPlayerBaseByID(m_PlayerId);

		if (!pb && !GetGame().IsDedicatedServer() && m_PlayerId != "")
			pb = VPPContextUtils.FindPlayerBySteam64Client(m_PlayerId);

		if (pb)
		{
			m_Object = pb;
			if (pb.GetIdentity())
				m_DisplayName = pb.GetIdentity().GetName();
		}
	}

	string GetTypeName()
	{
		return m_TypeName;
	}

	vector GetPosition()
	{
		Object obj = GetObject();
		if (obj)
			return obj.GetPosition();
		return m_Position;
	}

	string GetDisplayName()
	{
		return m_DisplayName;
	}

	void SetDisplayName(string name)
	{
		m_DisplayName = name;
	}

	string GetKey()
	{
		string key;
		if (m_Kind == EVPPContextTargetKind.PLAYER)
		{
			int count = m_PlayerIds.Count();
			key = "P:" + m_PlayerId;
			key = key + ":" + count.ToString();
			return key;
		}

		if (m_NetLow != 0 || m_NetHigh != 0)
		{
			key = "N:" + m_NetHigh.ToString();
			key = key + ":" + m_NetLow.ToString();
			return key;
		}

		key = "S:" + m_TypeName;
		key = key + "@" + VPPContextUtils.FormatPosition(m_Position);
		return key;
	}

	bool IsSameTarget(VPPContextTarget other)
	{
		if (!other)
			return false;
		return GetKey() == other.GetKey();
	}

	string Describe()
	{
		string text;
		if (m_Kind == EVPPContextTargetKind.PLAYER)
		{
			text = m_DisplayName + " (steamid=" + m_PlayerId + ")";
			int more = m_PlayerIds.Count() - 1;
			if (more > 0)
				text = text + " +" + more.ToString() + " more";
			return text;
		}

		text = m_TypeName + " (pos=" + VPPContextUtils.FormatPosition(GetPosition()) + ")";
		return text;
	}

	string GetExtra(string key)
	{
		string val;
		if (m_Extra.Find(key, val))
			return val;
		return "";
	}

	void SetExtra(string key, string value)
	{
		m_Extra.Set(key, value);
	}

	bool HasExtra(string key)
	{
		return m_Extra.Contains(key);
	}

	map<string,string> GetExtras()
	{
		return m_Extra;
	}

	bool HasState(string key)
	{
		return m_State.Contains(key);
	}

	string GetState(string key)
	{
		string val;
		if (m_State.Find(key, val))
			return val;
		return "";
	}

	bool GetStateBool(string key, bool fallback)
	{
		string val;
		if (!m_State.Find(key, val))
			return fallback;
		return VPPContextUtils.StateToBool(val, fallback);
	}

	void SetState(string key, string value)
	{
		m_State.Set(key, value);
	}

	void MergeState(map<string,string> state)
	{
		if (!state)
			return;

		foreach (string stateKey, string stateValue : state)
		{
			m_State.Set(stateKey, stateValue);
		}
	}

	void ClearState()
	{
		m_State.Clear();
	}

	//-----------------------------------------------------------------
	// internals
	//-----------------------------------------------------------------
	protected void AdoptPlayerObject(PlayerBase pb)
	{
		if (!pb)
			return;

		int low;
		int high;
		pb.GetNetworkID(low, high);
		m_Object   = pb;
		m_NetLow   = low;
		m_NetHigh  = high;
		m_TypeName = pb.GetType();
		m_Position = pb.GetPosition();
	}

	//a lazily re-resolved object of a PLAYER target must belong to the primary steam64 (T2);
	//client-supplied net ids can never swap in another player
	protected bool IsObjectOfPrimary(Object obj)
	{
		if (m_Kind != EVPPContextTargetKind.PLAYER)
			return true;
		if (m_PlayerId == "")
			return false;

		Man man = Man.Cast(obj);
		if (!man)
			return false;
		return VPPContextUtils.GetSteam64ForPlayer(man) == m_PlayerId;
	}

	protected static Object FindStaticObject(string typeName, vector pos)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition3D(pos, 1.0, objs, null);

		Object best = null;
		float bestDist = -1.0;
		foreach (Object candidate : objs)
		{
			if (!candidate || candidate.GetType() != typeName)
				continue;

			float dist = vector.Distance(candidate.GetPosition(), pos);
			if (!best || dist < bestDist)
			{
				best = candidate;
				bestDist = dist;
			}
		}
		return best;
	}
};
