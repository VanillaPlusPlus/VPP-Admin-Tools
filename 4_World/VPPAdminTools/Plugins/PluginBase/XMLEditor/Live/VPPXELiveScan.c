// XML Editor: live layer of the distribution map (current instances of one type).
// VPPXELiveScanJob: one native box query per TILE_SIZE (1024 m) map tile, one tile per Step (Physics variant for
// House, BuildingSuper and CrashBase types), appended into a member array; then a budgeted walk that null-checks
// each entity, compares the GetType() length first and only then a lowercased local copy, and drops the duplicate
// hits of objects spanning a tile edge. Capped at LiveScanMaxResults, sent in VPPXELiveChunk chunks (always one or more).
// VPPXELiveDelete: guarded delete of at most DELETE_PER_REQUEST scanned objects.

class VPPXELiveScanJob : VPPXEJob
{
	const static int PH_SCAN = 0;
	const static int PH_WALK = 1;
	const static int PH_CHUNKS = 2;
	const static int PH_SEND = 3;
	const static int PH_DONE = 4;
	const static int CHUNK_BASE_BYTES = 28;
	const static int RECORD_BYTES = 16;
	const static int TILE_SIZE = 1024;

	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_TypeName;
	protected string m_Lower;
	protected int m_NameLen;
	protected int m_WorldSize;
	protected int m_MaxResults;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_Scanned;
	protected int m_Hits;
	protected int m_StartMs;
	protected int m_TileIdx;
	protected int m_TilesPerSide;
	protected bool m_Physics;
	protected ref array<EntityAI> m_Found;
	protected ref map<EntityAI, bool> m_HitSeen;
	protected ref array<float> m_Coords;
	protected ref array<int> m_Net;
	protected ref array<ref VPPXELiveChunk> m_Chunks;
	protected ref VPPXELiveChunk m_CurChunk;
	protected int m_CurBytes;
	protected int m_CurRows;

	void VPPXELiveScanJob(PlayerIdentity sender, int reqId, string typeName, int worldSize, int maxResults)
	{
		m_Sender = sender;
		m_ReqId = reqId;
		m_TypeName = typeName;
		m_Lower = VPPXEDistUtil.Lower(typeName);
		m_NameLen = typeName.Length();
		m_WorldSize = worldSize;
		m_MaxResults = maxResults;
		if (m_MaxResults < 1)
		{
			m_MaxResults = 1;
		}

		m_Phase = PH_SCAN;
		m_Cursor = 0;
		m_Scanned = 0;
		m_Hits = 0;
		m_StartMs = GetGame().GetTime();
		m_TileIdx = 0;
		m_TilesPerSide = (m_WorldSize + TILE_SIZE - 1) / TILE_SIZE;
		if (m_TilesPerSide < 1)
		{
			m_TilesPerSide = 1;
		}

		m_Physics = UsesPhysicsScan(m_TypeName);
		m_Found = new array<EntityAI>();
		m_HitSeen = new map<EntityAI, bool>();
		m_Coords = new array<float>();
		m_Net = new array<int>();
		m_Chunks = new array<ref VPPXELiveChunk>();
		m_CurBytes = 0;
		m_CurRows = 0;
	}

	override string GetLabel()
	{
		return "live scan " + m_TypeName;
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_DONE)
			{
				return true;
			}

			if (m_Phase == PH_SCAN)
			{
				ScanUnit();
				return false;
			}

			if (m_Phase == PH_WALK)
			{
				WalkUnit();
			}
			else if (m_Phase == PH_CHUNKS)
			{
				ChunkUnit();
			}
			else if (m_Phase == PH_SEND)
			{
				SendUnit();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnAborted()
	{
		m_Found = null;
		m_HitSeen = null;
		VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
	}

	// House, BuildingSuper and CrashBase types need the Physics box query. Config-only classes (every Land_* type)
	// have no script class, so ToType() is null for them: test config inheritance first, typename as the fallback.
	static bool UsesPhysicsScan(string typeName)
	{
		if (typeName == "")
		{
			return false;
		}

		if (GetGame().IsKindOf(typeName, "House") || GetGame().IsKindOf(typeName, "HouseNoDestruct"))
		{
			return true;
		}

		typename scanType = typeName.ToType();
		if (!scanType)
		{
			return false;
		}

		if (scanType.IsInherited(House) || scanType.IsInherited(BuildingSuper) || scanType.IsInherited(CrashBase))
		{
			return true;
		}

		return false;
	}

	// one native box query per TILE_SIZE map tile (a single full-map query hitches); the job yields after each tile.
	// Tile results go into a local array and are appended to m_Found; progress 0..50 here, the walk maps to 50..100.
	protected void ScanUnit()
	{
		int tileCount = m_TilesPerSide * m_TilesPerSide;
		if (m_TileIdx >= tileCount)
		{
			m_Scanned = m_Found.Count();
			m_Cursor = 0;
			m_Phase = PH_WALK;
			return;
		}

		int tileX = m_TileIdx % m_TilesPerSide;
		int tileZ = m_TileIdx / m_TilesPerSide;
		int x0 = tileX * TILE_SIZE;
		int z0 = tileZ * TILE_SIZE;
		int x1 = x0 + TILE_SIZE;
		if (x1 > m_WorldSize)
		{
			x1 = m_WorldSize;
		}

		int z1 = z0 + TILE_SIZE;
		if (z1 > m_WorldSize)
		{
			z1 = m_WorldSize;
		}

		vector boxMin = Vector(x0, -1200, z0);
		vector boxMax = Vector(x1, 1200, z1);
		array<EntityAI> tileFound = new array<EntityAI>();
		if (m_Physics)
		{
			DayZPlayerUtils.PhysicsGetEntitiesInBox(boxMin, boxMax, tileFound);
		}
		else
		{
			DayZPlayerUtils.SceneGetEntitiesInBox(boxMin, boxMax, tileFound);
		}

		m_Found.InsertAll(tileFound);
		m_TileIdx++;
		VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.MATCH, m_TileIdx * 50 / tileCount);
		if (m_TileIdx >= tileCount)
		{
			m_Scanned = m_Found.Count();
			m_Cursor = 0;
			m_Phase = PH_WALK;
		}
	}

	protected void WalkUnit()
	{
		if (m_Cursor >= m_Found.Count())
		{
			m_Found = null;
			m_HitSeen = null;
			m_Cursor = 0;
			m_Phase = PH_CHUNKS;
			return;
		}

		EntityAI ent = m_Found[m_Cursor];
		m_Cursor++;
		if (m_Cursor % 4096 == 0 && m_Scanned > 0)
		{
			VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.MATCH, 50 + m_Cursor / (m_Scanned / 50 + 1));
		}

		if (!ent)
		{
			return;
		}

		string entType = ent.GetType();
		if (entType.Length() != m_NameLen)
		{
			return;
		}

		entType.ToLower();
		if (entType != m_Lower)
		{
			return;
		}

		// an object whose bounds span a tile edge is returned by several tiles (static buildings have net id 0:0)
		if (m_HitSeen.Contains(ent))
		{
			return;
		}

		m_HitSeen.Insert(ent, true);
		m_Hits++;
		if (m_Net.Count() / 2 >= m_MaxResults)
		{
			return;
		}

		vector pos = ent.GetPosition();
		m_Coords.Insert(pos[0]);
		m_Coords.Insert(pos[2]);
		int lowBits;
		int highBits;
		ent.GetNetworkID(lowBits, highBits);
		m_Net.Insert(lowBits);
		m_Net.Insert(highBits);
	}

	// LIVE_PER_CHUNK records or CHUNK_BYTES per chunk; a scan with no hit still sends one empty chunk
	protected void ChunkUnit()
	{
		int records = m_Net.Count() / 2;
		if (m_Cursor >= records)
		{
			if (m_CurChunk)
			{
				m_Chunks.Insert(m_CurChunk);
				m_CurChunk = null;
			}

			if (m_Chunks.Count() == 0)
			{
				m_Chunks.Insert(NewChunk());
			}

			int chunkCount = m_Chunks.Count();
			for (int i = 0; i < chunkCount; i++)
			{
				VPPXELiveChunk chunk = m_Chunks[i];
				chunk.ChunkIdx = i;
				chunk.ChunkCount = chunkCount;
			}

			m_Cursor = 0;
			m_Phase = PH_SEND;
			return;
		}

		if (m_CurChunk && m_CurRows > 0)
		{
			if (m_CurRows >= VPPXEConst.LIVE_PER_CHUNK || m_CurBytes + RECORD_BYTES > VPPXEConst.CHUNK_BYTES)
			{
				m_Chunks.Insert(m_CurChunk);
				m_CurChunk = null;
			}
		}

		if (!m_CurChunk)
		{
			m_CurChunk = NewChunk();
			m_CurBytes = CHUNK_BASE_BYTES;
			m_CurRows = 0;
		}

		int recBase = m_Cursor * 2;
		m_CurChunk.Coords.Insert(m_Coords[recBase]);
		m_CurChunk.Coords.Insert(m_Coords[recBase + 1]);
		m_CurChunk.Net.Insert(m_Net[recBase]);
		m_CurChunk.Net.Insert(m_Net[recBase + 1]);
		m_CurRows++;
		m_CurBytes = m_CurBytes + RECORD_BYTES;
		m_Cursor++;
	}

	protected VPPXELiveChunk NewChunk()
	{
		VPPXELiveChunk chunk = new VPPXELiveChunk();
		chunk.ReqId = m_ReqId;
		chunk.Total = m_Hits;
		chunk.Capped = m_Hits > m_MaxResults;
		if (!chunk.Coords)
		{
			chunk.Coords = new array<float>();
		}

		if (!chunk.Net)
		{
			chunk.Net = new array<int>();
		}

		return chunk;
	}

	protected void SendUnit()
	{
		if (m_Cursor >= m_Chunks.Count())
		{
			FinishScan();
			return;
		}

		VPPXELiveChunk chunk = m_Chunks[m_Cursor];
		m_Cursor++;
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnLiveChunk");
		Param1<ref VPPXELiveChunk> netPayload = new Param1<ref VPPXELiveChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnLiveChunk", netRpc);
	}

	protected void FinishScan()
	{
		m_Phase = PH_DONE;
		int ms = GetGame().GetTime() - m_StartMs;
		string text = "live scan of " + m_TypeName + ": " + m_Hits.ToString() + " found (entities scanned " + m_Scanned.ToString() + ")";
		VPPXELog.Action(m_Sender, text, true);
		VPPXELog.Info("[Dist] live scan of " + m_TypeName + " took " + ms.ToString() + " ms");
		VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.MATCH, 100);
	}
};

class VPPXELiveDelete
{
	// each pair must be a non-zero network id resolving to an EntityAI of the scanned type that is not a Man, not a
	// baked map building, has no root player and, for a Transport, no crew; everything else is refused and counted
	static void Run(PlayerIdentity sender, int reqId, string typeName, array<int> netPairs)
	{
		if (!sender)
		{
			return;
		}

		if (!VPPXmlText.IsValidClassName(typeName))
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(typeName, VPPXEConst.MAX_CELL_CHARS));
			return;
		}

		string lowerName = VPPXEDistUtil.Lower(typeName);
		int pairCount = 0;
		if (netPairs)
		{
			pairCount = netPairs.Count() / 2;
		}

		int refused = 0;
		if (pairCount > VPPXEConst.DELETE_PER_REQUEST)
		{
			refused = pairCount - VPPXEConst.DELETE_PER_REQUEST;
			pairCount = VPPXEConst.DELETE_PER_REQUEST;
		}

		array<Object> accepted = new array<Object>();
		map<string, bool> seen = new map<string, bool>();
		for (int i = 0; i < pairCount; i++)
		{
			int lowBits = netPairs[i * 2];
			int highBits = netPairs[i * 2 + 1];
			string netKey = lowBits.ToString() + ":" + highBits.ToString();
			if (seen.Contains(netKey))
			{
				continue;
			}

			seen.Insert(netKey, true);
			Object obj = null;
			if (lowBits != 0 || highBits != 0)
			{
				obj = GetGame().GetObjectByNetworkId(lowBits, highBits);
			}

			if (IsDeletable(obj, lowerName))
			{
				accepted.Insert(obj);
			}
			else
			{
				refused++;
			}
		}

		int deleted = 0;
		for (int j = 0; j < accepted.Count(); j++)
		{
			Object target = accepted[j];
			if (!target)
			{
				continue;
			}

			GetGame().ObjectDelete(target);
			deleted++;
		}

		VPPXENet.Result(sender, reqId, true, "#VSTR_XMLE_MAP_DELETED", deleted.ToString());
		if (refused > 0)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_REFUSED", refused.ToString());
		}

		string text = "deleted " + deleted.ToString() + " live " + typeName + " (refused " + refused.ToString() + ")";
		VPPXELog.Action(sender, text, true);
	}

	static bool IsDeletable(Object obj, string lowerName)
	{
		if (!obj)
		{
			return false;
		}

		EntityAI ent = EntityAI.Cast(obj);
		if (!ent)
		{
			return false;
		}

		string entType = ent.GetType();
		entType.ToLower();
		if (entType != lowerName)
		{
			return false;
		}

		if (ent.IsMan())
		{
			return false;
		}

		// a baked map object (House, BuildingSuper, every config-only Land_* class) is not replicated when the server
		// deletes it (static objects are not networked; RegisterNetworkStaticObject ones still exist on every client).
		// Only a CrashBase wreck or a building with a CE profile (event wrecks such as Land_*_DE) counts as dynamic.
		if (ent.IsInherited(Building) || ent.IsKindOf("House"))
		{
			if (!ent.IsInherited(CrashBase) && !ent.GetEconomyProfile())
			{
				return false;
			}
		}

		if (ent.GetHierarchyRootPlayer())
		{
			return false;
		}

		Transport vehicle = Transport.Cast(ent);
		if (vehicle)
		{
			int crewSize = vehicle.CrewSize();
			for (int i = 0; i < crewSize; i++)
			{
				if (vehicle.CrewMember(i))
				{
					return false;
				}
			}
		}

		return true;
	}
};
