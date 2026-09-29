// XML Editor: DIST-ALGO v2.1 matcher (INTERFACES section 9, R8-R19), aggregation, chunk building and the
// one [XMLEditor][Dist] log line per query. Runs as an INTERACTIVE job over immutable LIGHT/MAP/CLUSTERS
// snapshots; every loop over positions, groups, events or records is one unit per BudgetOk check.

// Per-layer accumulator: records (for markers), 100 m cells (R19), table rows keyed case-insensitively.
class VPPXEDistLayerAcc : Managed
{
	int Layer;
	int GridCols;
	int Records;
	int PointsSum;
	int TierAll;
	bool KeepRecords;
	bool MarkersOn;
	ref array<float> RecX;
	ref array<float> RecZ;
	ref array<int> RecTier;
	ref array<int> RecRow;
	ref map<int, int> CellSlot;
	ref array<int> CellIdx;
	ref array<int> CellCount;
	ref array<int> CellTier;
	ref array<int> CellPts;
	ref map<string, int> RowSlot;
	ref array<string> RowLabel;
	ref array<string> RowDetail;
	ref array<int> RowCount;
	ref array<int> RowPoints;
	ref array<int> RowTier;
	ref array<float> RowX;
	ref array<float> RowZ;
	ref array<int> RowPos;
	ref array<int> RowOrder;
	ref array<int> MarkerInts;
	ref array<float> Circles;

	void VPPXEDistLayerAcc(int layer, int gridCols, bool keepRecords)
	{
		Layer = layer;
		GridCols = gridCols;
		Records = 0;
		PointsSum = 0;
		TierAll = 0;
		KeepRecords = keepRecords;
		MarkersOn = false;
		RecX = new array<float>();
		RecZ = new array<float>();
		RecTier = new array<int>();
		RecRow = new array<int>();
		CellSlot = new map<int, int>();
		CellIdx = new array<int>();
		CellCount = new array<int>();
		CellTier = new array<int>();
		CellPts = new array<int>();
		RowSlot = new map<string, int>();
		RowLabel = new array<string>();
		RowDetail = new array<string>();
		RowCount = new array<int>();
		RowPoints = new array<int>();
		RowTier = new array<int>();
		RowX = new array<float>();
		RowZ = new array<float>();
		RowPos = new array<int>();
		RowOrder = new array<int>();
		MarkerInts = new array<int>();
		Circles = new array<float>();
	}

	// a row is created by its first record, so X/Z are the first record position
	int EnsureRow(string key, string label, string detail, float x, float z)
	{
		int slot;
		if (RowSlot.Find(key, slot))
		{
			return slot;
		}

		slot = RowLabel.Insert(label);
		RowDetail.Insert(detail);
		RowCount.Insert(0);
		RowPoints.Insert(0);
		RowTier.Insert(0);
		RowX.Insert(x);
		RowZ.Insert(z);
		RowSlot.Insert(key, slot);
		return slot;
	}

	void AddRowCount(int row, int count, int pts, int tier)
	{
		int oldCount = RowCount[row];
		int oldPoints = RowPoints[row];
		int oldTier = RowTier[row];
		RowCount.Set(row, oldCount + count);
		RowPoints.Set(row, oldPoints + pts);
		RowTier.Set(row, oldTier | tier);
	}

	void AddCellCount(int cellIdx, int count, int tier, int pts)
	{
		int slot;
		if (CellSlot.Find(cellIdx, slot))
		{
			int oldCount = CellCount[slot];
			int oldTier = CellTier[slot];
			int oldPts = CellPts[slot];
			CellCount.Set(slot, oldCount + count);
			CellTier.Set(slot, oldTier | tier);
			CellPts.Set(slot, oldPts + pts);
			return;
		}

		slot = CellIdx.Insert(cellIdx);
		CellCount.Insert(count);
		CellTier.Insert(tier);
		CellPts.Insert(pts);
		CellSlot.Insert(cellIdx, slot);
	}

	void AddRecord(float x, float z, int tier, int pts, int row)
	{
		Records++;
		PointsSum = PointsSum + pts;
		TierAll = TierAll | tier;
		if (KeepRecords)
		{
			RecX.Insert(x);
			RecZ.Insert(z);
			RecTier.Insert(tier);
			RecRow.Insert(row);
		}

		AddCellCount(VPPXEDistUtil.CellIndex(x, z, GridCols), 1, tier, pts);
		if (row >= 0)
		{
			AddRowCount(row, 1, pts, tier);
		}
	}
};

class VPPXEDistQueryJob : VPPXEJob
{
	const static int PH_INIT = 0;
	const static int PH_GROUPS = 1;
	const static int PH_BUILDINGS = 2;
	const static int PH_EVOBJ = 3;
	const static int PH_EVCHILD_EVENTS = 4;
	const static int PH_EVCHILD_GROUPED = 5;
	const static int PH_CONTAINERS = 6;
	const static int PH_INFECTED_EVENTS = 7;
	const static int PH_INFECTED_ZONES = 8;
	const static int PH_CLUSTER_DES = 9;
	const static int PH_CLUSTER_CELLS = 10;
	const static int PH_CLUSTER_ROWS = 11;
	const static int PH_DISPATCH_POS = 12;
	const static int PH_DISPATCH_EVENTS = 13;
	const static int PH_SORT = 14;
	const static int PH_MARKERS = 15;
	const static int PH_CHUNKS = 16;
	const static int PH_SEND = 17;
	const static int PH_DONE = 18;

	const static int SORT_BASE = 2000000000;
	const static int CHUNK_BASE_BYTES = 32;
	const static int MAX_PLAYER_EVENTS = 50;

	protected VPPXEDistService m_Service;
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_TypeName;
	protected string m_Key;
	protected int m_Mask;
	protected ref VPPXESpawnLight m_Light;
	protected ref VPPXESpawnMap m_Map;
	protected ref VPPXEAreaSamples m_Samples;
	protected ref VPPXEClusterData m_Clusters;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_Cursor2;
	protected int m_EvCur;
	protected int m_PosCur;
	protected int m_Inst;
	protected int m_CurRow;
	protected int m_StartMs;

	// T (R8)
	protected bool m_HasT;
	protected int m_Cat;
	protected int m_TagMask;
	protected int m_UsageMask;
	protected int m_ValueMask;
	protected bool m_Deloot;
	protected bool m_Nom0;
	protected bool m_Ignored;

	protected ref array<int> m_GroupPts;
	protected ref array<int> m_GroupTChild;
	protected ref array<ref VPPXEDistLayerAcc> m_Layers;
	protected ref array<string> m_PlayerEvents;
	protected ref array<int> m_ParentList;
	protected ref map<string, bool> m_ParentSet;
	protected int m_ParentHits;
	protected ref map<string, int> m_InfectedEvents;
	protected ref array<int> m_DeList;
	protected ref map<int, bool> m_DeSet;
	protected ref array<int> m_DispGroups;
	protected ref map<int, bool> m_DispSet;
	protected bool m_Capped;

	protected ref array<ref VPPXEDistChunk> m_Chunks;
	protected ref VPPXEDistChunk m_CurChunk;
	protected int m_CurBytes;
	protected int m_CurRows;
	protected int m_ChunkLayer;
	protected int m_ChunkKind;
	protected ref VPPXEDistSummary m_Summary;
	protected int m_WorldSize;
	protected int m_GridCols;
	protected float m_PackScale;

	void VPPXEDistQueryJob(VPPXEDistService service, PlayerIdentity sender, int reqId, string typeName, int layerMask, VPPXESpawnLight light, VPPXESpawnMap mapData, VPPXEClusterData clusters)
	{
		m_Service = service;
		m_Sender = sender;
		m_ReqId = reqId;
		m_TypeName = typeName;
		m_Key = VPPXEDistUtil.Lower(typeName);
		m_Mask = layerMask;
		m_Light = light;
		m_Map = mapData;
		m_Clusters = clusters;
		m_Samples = mapData.Samples;
		if (!m_Samples)
		{
			m_Samples = new VPPXEAreaSamples();
		}

		m_Phase = PH_INIT;
		m_Cursor = 0;
		m_Cursor2 = 0;
		m_EvCur = 0;
		m_PosCur = 0;
		m_Inst = -2;
		m_CurRow = -1;
		m_StartMs = GetGame().GetTime();
		m_HasT = false;
		m_Cat = -1;
		m_TagMask = 0;
		m_UsageMask = 0;
		m_ValueMask = 0;
		m_Deloot = false;
		m_Nom0 = false;
		m_Ignored = false;
		m_GroupPts = new array<int>();
		m_GroupTChild = new array<int>();
		m_Layers = new array<ref VPPXEDistLayerAcc>();
		m_PlayerEvents = new array<string>();
		m_ParentSet = new map<string, bool>();
		m_ParentHits = 0;
		m_InfectedEvents = new map<string, int>();
		m_DeList = new array<int>();
		m_DeSet = new map<int, bool>();
		m_DispSet = new map<int, bool>();
		m_Capped = false;
		m_Chunks = new array<ref VPPXEDistChunk>();
		m_CurBytes = 0;
		m_CurRows = 0;
		m_ChunkLayer = 0;
		m_ChunkKind = 0;
		m_WorldSize = 0;
		m_GridCols = 1;
		m_PackScale = 1;
	}

	override string GetLabel()
	{
		return "dist query " + m_TypeName;
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_DONE)
			{
				return true;
			}

			if (m_Phase == PH_INIT)
			{
				InitUnit();
			}
			else if (m_Phase == PH_GROUPS)
			{
				GroupsUnit();
			}
			else if (m_Phase == PH_BUILDINGS)
			{
				BuildingsUnit();
			}
			else if (m_Phase == PH_EVOBJ)
			{
				EvObjUnit();
			}
			else if (m_Phase == PH_EVCHILD_EVENTS)
			{
				EvChildEventsUnit();
			}
			else if (m_Phase == PH_EVCHILD_GROUPED)
			{
				EvChildGroupedUnit();
			}
			else if (m_Phase == PH_CONTAINERS)
			{
				ContainersUnit();
			}
			else if (m_Phase == PH_INFECTED_EVENTS)
			{
				InfectedEventsUnit();
			}
			else if (m_Phase == PH_INFECTED_ZONES)
			{
				InfectedZonesUnit();
			}
			else if (m_Phase == PH_CLUSTER_DES)
			{
				ClusterDesUnit();
			}
			else if (m_Phase == PH_CLUSTER_CELLS)
			{
				ClusterCellsUnit();
			}
			else if (m_Phase == PH_CLUSTER_ROWS)
			{
				ClusterRowsUnit();
			}
			else if (m_Phase == PH_DISPATCH_POS)
			{
				DispatchPosUnit();
			}
			else if (m_Phase == PH_DISPATCH_EVENTS)
			{
				DispatchEventsUnit();
			}
			else if (m_Phase == PH_SORT)
			{
				SortUnit();
			}
			else if (m_Phase == PH_MARKERS)
			{
				MarkersUnit();
			}
			else if (m_Phase == PH_CHUNKS)
			{
				ChunksUnit();
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
		VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
		m_Layers = null;
		m_Chunks = null;
	}

	protected bool Wants(int layer)
	{
		return (m_Mask & (1 << layer)) != 0;
	}

	protected VPPXEDistLayerAcc Acc(int layer)
	{
		return m_Layers[layer];
	}

	protected void NextPhase(int phase)
	{
		m_Phase = phase;
		m_Cursor = 0;
		m_Cursor2 = 0;
		m_EvCur = 0;
		m_PosCur = 0;
		m_Inst = -2;
		m_CurRow = -1;
		if (m_Sender)
		{
			VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.MATCH, phase * 100 / PH_DONE);
		}
	}

	// next (active event, spawn position) pair in merged event order, then position order
	protected bool NextEventPos(out int evIdx, out int posIdx)
	{
		while (m_EvCur < m_Light.Active.Count())
		{
			int spawnIdx = m_Light.ActiveSpawn[m_EvCur];
			if (spawnIdx >= 0)
			{
				array<int> posList = m_Light.SpPos[spawnIdx];
				if (m_PosCur < posList.Count())
				{
					evIdx = m_EvCur;
					posIdx = posList[m_PosCur];
					m_PosCur++;
					return true;
				}
			}

			m_EvCur++;
			m_PosCur = 0;
		}

		return false;
	}

	// R9: live, category (catIdx < 0 = no constraint), tag (0 = any)
	protected bool Accepts(int cont)
	{
		if (!m_Map.ContLive[cont])
		{
			return false;
		}

		if (m_Cat >= 0)
		{
			if (m_Cat >= 32)
			{
				return false;
			}

			if ((m_Map.ContCat[cont] & (1 << m_Cat)) == 0)
			{
				return false;
			}
		}

		if (m_TagMask != 0 && (m_TagMask & m_Map.ContTag[cont]) == 0)
		{
			return false;
		}

		return true;
	}

	// R10 usage/value tests with the area flags sampled at (x, z); tier = every value bit (the areaflags value plane is
	// one byte: up to 8 value flags, in cfglimitsdefinition <valueflags> order; custom maps may define more than 5)
	protected bool PassUV(int groupIdx, float x, float z, out int tier)
	{
		tier = 0;
		int mapUsage;
		int mapValue;
		m_Samples.SampleAt(x, z, mapUsage, mapValue);
		int u = m_Map.ProtoUsage[groupIdx] | mapUsage;
		if (m_UsageMask != 0 && (m_UsageMask & u) == 0)
		{
			return false;
		}

		int v = mapValue;
		if (m_Map.ProtoHasValue[groupIdx])
		{
			v = m_Map.ProtoValue[groupIdx];
		}

		if (m_ValueMask != 0 && (m_ValueMask & v) == 0)
		{
			return false;
		}

		tier = v & 255;
		return true;
	}

	// R8: the merged row of the type (read at job start, after any queued reindex)
	protected void InitUnit()
	{
		m_WorldSize = VPPXEDistUtil.WorldSize();
		m_GridCols = VPPXEDistUtil.GridCols(m_WorldSize);
		m_PackScale = m_WorldSize / 65535.0;
		for (int layer = 0; layer < 8; layer++)
		{
			bool keep = layer == VPPXELayer.BUILDINGS || layer == VPPXELayer.EVENT_OBJECTS || layer == VPPXELayer.EVENT_CHILD || layer == VPPXELayer.DISPATCH;
			m_Layers.Insert(new VPPXEDistLayerAcc(layer, m_GridCols, keep));
		}

		XMLEditor editor = GetXMLEditor();
		VPPXETypesService types;
		if (editor)
		{
			types = editor.GetTypes();
		}

		VPPXETypeRow row;
		if (types)
		{
			row = types.GetEffective(m_TypeName);
		}

		if (row)
		{
			m_HasT = true;
			m_Cat = row.Category;
			m_TagMask = row.Tag;
			VPPXELimits limits = types.GetLimits();
			if (limits)
			{
				m_UsageMask = limits.EffectiveUsage(row.Usage, row.UsageUser);
				m_ValueMask = limits.EffectiveValue(row.Value, row.ValueUser);
			}
			else
			{
				m_UsageMask = row.Usage;
				m_ValueMask = row.Value;
			}

			m_Deloot = (row.Flags & VPPXEFlagBit.DELOOT) != 0;
			m_Nom0 = row.Nominal <= 0;
			m_Ignored = types.IsIgnored(m_TypeName);
		}

		NextPhase(PH_GROUPS);
	}

	// points of the accepting containers per proto group (0 = no accepting container)
	protected void GroupsUnit()
	{
		if (!m_HasT || (!Wants(VPPXELayer.BUILDINGS) && !Wants(VPPXELayer.EVENT_OBJECTS)))
		{
			NextPhase(PH_BUILDINGS);
			return;
		}

		if (m_Cursor >= m_Map.ProtoKeys.Count())
		{
			NextPhase(PH_BUILDINGS);
			return;
		}

		int groupIdx = m_Cursor;
		m_Cursor++;
		int start = m_Map.ProtoContStart[groupIdx];
		int count = m_Map.ProtoContCount[groupIdx];
		int pts = 0;
		for (int i = 0; i < count; i++)
		{
			int cont = start + i;
			if (Accepts(cont))
			{
				pts = pts + m_Map.ContPoints[cont];
			}
		}

		m_GroupPts.Insert(pts);
	}

	// R10 BUILDINGS: proto groups in file order, then their instances in file order
	protected void BuildingsUnit()
	{
		if (!m_HasT || m_Deloot || !Wants(VPPXELayer.BUILDINGS))
		{
			NextPhase(PH_EVOBJ);
			return;
		}

		if (m_Cursor >= m_GroupPts.Count())
		{
			NextPhase(PH_EVOBJ);
			return;
		}

		int groupIdx = m_Cursor;
		int pts = m_GroupPts[groupIdx];
		if (pts <= 0)
		{
			m_Cursor++;
			m_Inst = -2;
			return;
		}

		if (m_Inst == -2)
		{
			m_Inst = m_Map.ProtoFirstPos[groupIdx];
			m_CurRow = -1;
		}

		if (m_Inst < 0)
		{
			m_Cursor++;
			m_Inst = -2;
			return;
		}

		int posIdx = m_Inst;
		m_Inst = m_Map.PosNext[posIdx];
		float x = m_Map.PosX[posIdx];
		float z = m_Map.PosZ[posIdx];
		int tier;
		if (!PassUV(groupIdx, x, z, tier))
		{
			return;
		}

		VPPXEDistLayerAcc acc = Acc(VPPXELayer.BUILDINGS);
		if (m_CurRow < 0)
		{
			m_CurRow = acc.EnsureRow(m_Map.ProtoKeys[groupIdx], m_Map.ProtoNames[groupIdx], pts.ToString(), x, z);
		}

		acc.AddRecord(x, z, tier, pts, m_CurRow);
	}

	// R13 EVENT_OBJECTS: distinct object types per position (first occurrence supplies lootmax, deloot, offsets)
	protected void EvObjUnit()
	{
		if (!m_HasT || !Wants(VPPXELayer.EVENT_OBJECTS))
		{
			NextPhase(PH_EVCHILD_EVENTS);
			return;
		}

		int evIdx;
		int posIdx;
		if (!NextEventPos(evIdx, posIdx))
		{
			NextPhase(PH_EVCHILD_EVENTS);
			return;
		}

		VPPXEDistEvent ev = m_Light.Active[evIdx];
		float px = m_Light.PosX[posIdx];
		float pz = m_Light.PosZ[posIdx];
		int groupIdx = m_Light.PosGroup[posIdx];
		if (groupIdx == -1)
		{
			for (int i = 0; i < ev.ChildKeys.Count(); i++)
			{
				EvObjCandidate(ev, ev.ChildKeys[i], ev.ChildNames[i], ev.ChildLootmax[i], ev.ChildDeloot[i], px, pz);
			}

			return;
		}

		if (groupIdx < 0)
		{
			return;
		}

		int start = m_Light.GroupDistStart[groupIdx];
		int count = m_Light.GroupDistCount[groupIdx];
		for (int j = 0; j < count; j++)
		{
			int childIdx = m_Light.GroupDist[start + j];
			EvObjCandidate(ev, m_Light.GcKeys[childIdx], m_Light.GcNames[childIdx], m_Light.GcLootmax[childIdx], m_Light.GcDeloot[childIdx], px + m_Light.GcX[childIdx], pz + m_Light.GcZ[childIdx]);
		}
	}

	protected void EvObjCandidate(VPPXEDistEvent ev, string objKey, string objName, int lootmax, int deloot, float qx, float qz)
	{
		if (lootmax <= 0)
		{
			return;
		}

		if (m_Deloot && deloot == 0)
		{
			return;
		}

		int groupIdx = m_Map.FindProto(objKey);
		if (groupIdx < 0 || groupIdx >= m_GroupPts.Count())
		{
			return;
		}

		int pts = m_GroupPts[groupIdx];
		if (pts <= 0)
		{
			return;
		}

		int tier;
		if (!PassUV(groupIdx, qx, qz, tier))
		{
			return;
		}

		VPPXEDistLayerAcc acc = Acc(VPPXELayer.EVENT_OBJECTS);
		string rowKey = ev.Key + " / " + objKey;
		string label = ev.Name + " / " + objName;
		int row = acc.EnsureRow(rowKey, label, pts.ToString(), qx, qz);
		acc.AddRecord(qx, qz, tier, pts, row);
	}

	// R14 part 1: active non-infected events with T as a child: fixed -> every spawn position, player/uniform -> PlayerEvents
	protected void EvChildEventsUnit()
	{
		if (!Wants(VPPXELayer.EVENT_CHILD))
		{
			NextPhase(PH_CONTAINERS);
			return;
		}

		if (m_Cursor >= m_Light.Active.Count())
		{
			m_GroupTChild.Clear();
			for (int g = 0; g < m_Light.GroupKeys.Count(); g++)
			{
				m_GroupTChild.Insert(-2);
			}

			NextPhase(PH_EVCHILD_GROUPED);
			return;
		}

		int evIdx = m_Cursor;
		m_Cursor++;
		VPPXEDistEvent ev = m_Light.Active[evIdx];
		if (ev.FindChild(m_Key) < 0)
		{
			return;
		}

		if (VPPXEDistUtil.StartsWith(ev.Key, "infected"))
		{
			return;
		}

		if (ev.Position == "fixed")
		{
			int spawnIdx = m_Light.ActiveSpawn[evIdx];
			if (spawnIdx < 0)
			{
				return;
			}

			VPPXEDistLayerAcc acc = Acc(VPPXELayer.EVENT_CHILD);
			array<int> posList = m_Light.SpPos[spawnIdx];
			for (int i = 0; i < posList.Count(); i++)
			{
				int posIdx = posList[i];
				float x = m_Light.PosX[posIdx];
				float z = m_Light.PosZ[posIdx];
				int row = acc.EnsureRow(ev.Key, ev.Name, "", x, z);
				acc.AddRecord(x, z, 0, 0, row);
			}

			return;
		}

		if (ev.Position == "player" || ev.Position == "uniform")
		{
			m_PlayerEvents.Insert(ev.Name);
		}
	}

	// R14 part 2: every spawn position whose group has T as a child, at q = p + offsets of the first such child
	protected void EvChildGroupedUnit()
	{
		int evIdx;
		int posIdx;
		if (!NextEventPos(evIdx, posIdx))
		{
			NextPhase(PH_CONTAINERS);
			return;
		}

		int groupIdx = m_Light.PosGroup[posIdx];
		if (groupIdx < 0)
		{
			return;
		}

		int childIdx = m_GroupTChild[groupIdx];
		if (childIdx == -2)
		{
			childIdx = m_Light.GroupFirstChild(groupIdx, m_Key);
			m_GroupTChild.Set(groupIdx, childIdx);
		}

		if (childIdx < 0)
		{
			return;
		}

		VPPXEDistEvent ev = m_Light.Active[evIdx];
		float qx = m_Light.PosX[posIdx] + m_Light.GcX[childIdx];
		float qz = m_Light.PosZ[posIdx] + m_Light.GcZ[childIdx];
		VPPXEDistLayerAcc acc = Acc(VPPXELayer.EVENT_CHILD);
		int row = acc.EnsureRow(ev.Key, ev.Name, "", qx, qz);
		acc.AddRecord(qx, qz, 0, 0, row);
	}

	protected string ChanceOrDash(string chanceText)
	{
		if (chanceText == "")
		{
			return "-";
		}

		return chanceText;
	}

	// R16 CONTAINERS: parents (sorted by name) whose attachments/cargo blocks list T
	protected void ContainersUnit()
	{
		if (!Wants(VPPXELayer.CONTAINERS) && !Wants(VPPXELayer.INFECTED))
		{
			NextPhase(PH_INFECTED_EVENTS);
			return;
		}

		if (!m_ParentList)
		{
			array<int> found;
			if (m_Light.ItemParents.Find(m_Key, found))
			{
				m_ParentList = found;
			}
			else
			{
				m_ParentList = new array<int>();
			}
		}

		if (m_Cursor >= m_ParentList.Count())
		{
			NextPhase(PH_INFECTED_EVENTS);
			return;
		}

		int parentIdx = m_ParentList[m_Cursor];
		m_Cursor++;
		VPPXEDistSpawnParent parent = m_Light.Parents[parentIdx];
		int matches = 0;
		string detail = "";
		for (int i = 0; i < parent.Blocks.Count(); i++)
		{
			VPPXEDistSpawnBlock block = parent.Blocks[i];
			for (int j = 0; j < block.ItemKeys.Count(); j++)
			{
				if (block.ItemKeys[j] != m_Key)
				{
					continue;
				}

				matches++;
				if (detail.Length() >= VPPXEConst.MAX_CELL_CHARS)
				{
					continue;
				}

				string part = block.Kind;
				if (block.IsPreset)
				{
					part = part + " " + block.PresetName;
				}

				part = part + " " + ChanceOrDash(block.ChanceText) + " x " + ChanceOrDash(block.ItemChances[j]);
				if (detail != "")
				{
					detail = detail + "; ";
				}

				detail = detail + part;
			}
		}

		if (matches == 0)
		{
			return;
		}

		m_ParentHits++;
		m_ParentSet.Set(parent.Key, true);
		if (!Wants(VPPXELayer.CONTAINERS))
		{
			return;
		}

		VPPXEDistLayerAcc acc = Acc(VPPXELayer.CONTAINERS);
		int row = acc.EnsureRow(parent.Key, parent.Name, detail, 0, 0);
		acc.AddRowCount(row, matches, 0, 0);
		acc.Records = acc.Records + 1;
	}

	// R15 InfectedEvents: active events named infected* with T or any R16 parent of T as a child
	protected void InfectedEventsUnit()
	{
		if (!Wants(VPPXELayer.INFECTED))
		{
			NextPhase(PH_CLUSTER_DES);
			return;
		}

		if (m_Cursor >= m_Light.Active.Count())
		{
			NextPhase(PH_INFECTED_ZONES);
			return;
		}

		int evIdx = m_Cursor;
		m_Cursor++;
		VPPXEDistEvent ev = m_Light.Active[evIdx];
		if (!VPPXEDistUtil.StartsWith(ev.Key, "infected"))
		{
			return;
		}

		bool hit = ev.FindChild(m_Key) >= 0;
		if (!hit && m_ParentSet.Count() > 0)
		{
			for (int i = 0; i < ev.ChildKeys.Count(); i++)
			{
				if (m_ParentSet.Contains(ev.ChildKeys[i]))
				{
					hit = true;
					break;
				}
			}
		}

		if (hit)
		{
			m_InfectedEvents.Set(ev.Key, evIdx);
		}
	}

	// R15 circles: every territory zone whose name is an InfectedEvent
	protected void InfectedZonesUnit()
	{
		if (m_InfectedEvents.Count() == 0 || m_Cursor >= m_Light.ZoneKeys.Count())
		{
			NextPhase(PH_CLUSTER_DES);
			return;
		}

		int zoneIdx = m_Cursor;
		m_Cursor++;
		int evIdx;
		if (!m_InfectedEvents.Find(m_Light.ZoneKeys[zoneIdx], evIdx))
		{
			return;
		}

		VPPXEDistEvent ev = m_Light.Active[evIdx];
		float zx = m_Light.ZoneX[zoneIdx];
		float zz = m_Light.ZoneZ[zoneIdx];
		float zr = m_Light.ZoneR[zoneIdx];
		VPPXEDistLayerAcc acc = Acc(VPPXELayer.INFECTED);
		acc.Circles.Insert(zx);
		acc.Circles.Insert(zz);
		acc.Circles.Insert(zr);
		int row = acc.EnsureRow(ev.Key, ev.Name, "", zx, zz);
		acc.AddRowCount(row, 1, 0, 0);
		acc.Records = acc.Records + 1;
	}

	// R17 DEs = active events with T as a child that are the de of some cluster
	protected void ClusterDesUnit()
	{
		if (!Wants(VPPXELayer.CLUSTERS) || !m_Clusters)
		{
			NextPhase(PH_DISPATCH_POS);
			return;
		}

		if (m_Cursor >= m_Light.Active.Count())
		{
			NextPhase(PH_CLUSTER_CELLS);
			return;
		}

		VPPXEDistEvent ev = m_Light.Active[m_Cursor];
		m_Cursor++;
		if (ev.FindChild(m_Key) < 0)
		{
			return;
		}

		int deIdx = m_Clusters.Proto.FindDe(ev.Key);
		if (deIdx < 0 || m_DeSet.Contains(deIdx))
		{
			return;
		}

		m_DeSet.Insert(deIdx, true);
		m_DeList.Insert(deIdx);
	}

	// R17/R19: merge the per-DE cell aggregates, one cell per unit
	protected void ClusterCellsUnit()
	{
		if (m_Cursor >= m_DeList.Count())
		{
			NextPhase(PH_CLUSTER_ROWS);
			return;
		}

		int deIdx = m_DeList[m_Cursor];
		map<int, int> cells = m_Clusters.DeCells[deIdx];
		if (m_Cursor2 >= cells.Count())
		{
			m_Cursor++;
			m_Cursor2 = 0;
			return;
		}

		int cellIdx = cells.GetKey(m_Cursor2);
		int count = cells.GetElement(m_Cursor2);
		m_Cursor2++;
		VPPXEDistLayerAcc acc = Acc(VPPXELayer.CLUSTERS);
		acc.AddCellCount(cellIdx, count, 0, 0);
		acc.Records = acc.Records + count;
	}

	// CLUSTERS table: one row per cluster name of the selected DEs
	protected void ClusterRowsUnit()
	{
		if (m_DeList.Count() == 0 || m_Cursor >= m_Clusters.Proto.ClusterKeys.Count())
		{
			NextPhase(PH_DISPATCH_POS);
			return;
		}

		int clusterIdx = m_Cursor;
		m_Cursor++;
		int deIdx = m_Clusters.Proto.ClusterDe[clusterIdx];
		if (deIdx < 0 || !m_DeSet.Contains(deIdx))
		{
			return;
		}

		int count = m_Clusters.ClusterCount[clusterIdx];
		if (count <= 0)
		{
			return;
		}

		VPPXEDistLayerAcc acc = Acc(VPPXELayer.CLUSTERS);
		int row = acc.EnsureRow(m_Clusters.Proto.ClusterKeys[clusterIdx], m_Clusters.Proto.ClusterNames[clusterIdx], m_Clusters.Proto.DeNames[deIdx], m_Clusters.ClusterX[clusterIdx], m_Clusters.ClusterZ[clusterIdx]);
		acc.AddRowCount(row, count, 0, 0);
	}

	// R18 DISPATCH part 1: every mapgrouppos instance of the groups whose dispatch list contains T
	protected void DispatchPosUnit()
	{
		if (!Wants(VPPXELayer.DISPATCH))
		{
			NextPhase(PH_SORT);
			return;
		}

		if (!m_DispGroups)
		{
			array<int> found;
			if (m_Map.DispatchGroups.Find(m_Key, found))
			{
				m_DispGroups = found;
			}
			else
			{
				m_DispGroups = new array<int>();
			}

			for (int i = 0; i < m_DispGroups.Count(); i++)
			{
				m_DispSet.Set(m_DispGroups[i], true);
			}
		}

		if (m_DispGroups.Count() == 0)
		{
			NextPhase(PH_SORT);
			return;
		}

		if (m_Cursor >= m_DispGroups.Count())
		{
			NextPhase(PH_DISPATCH_EVENTS);
			return;
		}

		int groupIdx = m_DispGroups[m_Cursor];
		if (m_Inst == -2)
		{
			m_Inst = m_Map.ProtoFirstPos[groupIdx];
			m_CurRow = -1;
		}

		if (m_Inst < 0)
		{
			m_Cursor++;
			m_Inst = -2;
			return;
		}

		int posIdx = m_Inst;
		m_Inst = m_Map.PosNext[posIdx];
		float x = m_Map.PosX[posIdx];
		float z = m_Map.PosZ[posIdx];
		VPPXEDistLayerAcc acc = Acc(VPPXELayer.DISPATCH);
		if (m_CurRow < 0)
		{
			m_CurRow = acc.EnsureRow(m_Map.ProtoKeys[groupIdx], m_Map.ProtoNames[groupIdx], "", x, z);
		}

		acc.AddRecord(x, z, 0, 0, m_CurRow);
	}

	// R18 DISPATCH part 2: R13 position/object pairs (distinct types, first-occurrence offsets, no filters)
	protected void DispatchEventsUnit()
	{
		int evIdx;
		int posIdx;
		if (!NextEventPos(evIdx, posIdx))
		{
			NextPhase(PH_SORT);
			return;
		}

		VPPXEDistEvent ev = m_Light.Active[evIdx];
		float px = m_Light.PosX[posIdx];
		float pz = m_Light.PosZ[posIdx];
		int groupIdx = m_Light.PosGroup[posIdx];
		if (groupIdx == -1)
		{
			for (int i = 0; i < ev.ChildKeys.Count(); i++)
			{
				DispatchCandidate(ev.ChildKeys[i], px, pz);
			}

			return;
		}

		if (groupIdx < 0)
		{
			return;
		}

		int start = m_Light.GroupDistStart[groupIdx];
		int count = m_Light.GroupDistCount[groupIdx];
		for (int j = 0; j < count; j++)
		{
			int childIdx = m_Light.GroupDist[start + j];
			DispatchCandidate(m_Light.GcKeys[childIdx], px + m_Light.GcX[childIdx], pz + m_Light.GcZ[childIdx]);
		}
	}

	protected void DispatchCandidate(string objKey, float qx, float qz)
	{
		int groupIdx = m_Map.FindProto(objKey);
		if (groupIdx < 0 || !m_DispSet.Contains(groupIdx))
		{
			return;
		}

		VPPXEDistLayerAcc acc = Acc(VPPXELayer.DISPATCH);
		int row = acc.EnsureRow(m_Map.ProtoKeys[groupIdx], m_Map.ProtoNames[groupIdx], "", qx, qz);
		acc.AddRecord(qx, qz, 0, 0, row);
	}

	// table rows per layer: Count desc (stable), capped at MAX_TABLE_ROWS_PER_LAYER, one native sort per layer
	protected void SortUnit()
	{
		if (m_Cursor >= m_Layers.Count())
		{
			NextPhase(PH_MARKERS);
			return;
		}

		VPPXEDistLayerAcc acc = m_Layers[m_Cursor];
		m_Cursor++;
		int rows = acc.RowLabel.Count();
		if (rows == 0)
		{
			return;
		}

		array<string> keys = new array<string>();
		for (int i = 0; i < rows; i++)
		{
			acc.RowPos.Insert(-1);
			int count = acc.RowCount[i];
			keys.Insert(VPPXmlText.PadInt(SORT_BASE - count, 10) + VPPXmlText.PadInt(i, 7));
		}

		keys.Sort();
		int cap = keys.Count();
		if (cap > VPPXEConst.MAX_TABLE_ROWS_PER_LAYER)
		{
			cap = VPPXEConst.MAX_TABLE_ROWS_PER_LAYER;
		}

		for (int j = 0; j < cap; j++)
		{
			string sortKey = keys[j];
			string tail = sortKey.Substring(10, 7);
			int slot = tail.ToInt();
			acc.RowPos.Set(slot, j);
			acc.RowOrder.Insert(slot);
		}
	}

	// MARKERS only for layers with at most MAX_MARKERS records; more -> cells only and CAPPED
	protected void MarkersUnit()
	{
		if (m_Cursor >= m_Layers.Count())
		{
			NextPhase(PH_CHUNKS);
			return;
		}

		VPPXEDistLayerAcc acc = m_Layers[m_Cursor];
		if (!acc.KeepRecords || acc.Records == 0 || acc.Records > VPPXEConst.MAX_MARKERS)
		{
			if (acc.KeepRecords && acc.Records > VPPXEConst.MAX_MARKERS)
			{
				m_Capped = true;
			}

			m_Cursor++;
			m_Cursor2 = 0;
			return;
		}

		acc.MarkersOn = true;
		if (m_Cursor2 >= acc.RecX.Count())
		{
			m_Cursor++;
			m_Cursor2 = 0;
			return;
		}

		int rec = m_Cursor2;
		m_Cursor2++;
		int px = Math.Round(acc.RecX[rec] / m_PackScale);
		int pz = Math.Round(acc.RecZ[rec] / m_PackScale);
		px = VPPXEDistUtil.ClampInt(px, 0, 65535);
		pz = VPPXEDistUtil.ClampInt(pz, 0, 65535);
		int rowPos = -1;
		int row = acc.RecRow[rec];
		if (row >= 0)
		{
			rowPos = acc.RowPos[row];
		}

		int meta = acc.RecTier[rec] | ((rowPos + 1) << 8);
		acc.MarkerInts.Insert(px);
		acc.MarkerInts.Insert(pz);
		acc.MarkerInts.Insert(meta);
	}

	protected bool IsCellLayer(int layer)
	{
		return layer == VPPXELayer.BUILDINGS || layer == VPPXELayer.EVENT_OBJECTS || layer == VPPXELayer.EVENT_CHILD || layer == VPPXELayer.CLUSTERS || layer == VPPXELayer.DISPATCH;
	}

	protected int ItemCount(VPPXEDistLayerAcc acc, int kind)
	{
		if (kind == VPPXEChunkKind.CELLS)
		{
			if (IsCellLayer(acc.Layer))
			{
				return acc.CellIdx.Count();
			}

			return 0;
		}

		if (kind == VPPXEChunkKind.MARKERS)
		{
			if (acc.MarkersOn)
			{
				return acc.MarkerInts.Count() / 3;
			}

			return 0;
		}

		if (kind == VPPXEChunkKind.CIRCLES)
		{
			if (acc.Layer == VPPXELayer.INFECTED)
			{
				return acc.Circles.Count() / 3;
			}

			return 0;
		}

		return acc.RowOrder.Count();
	}

	protected int KindCap(int kind)
	{
		if (kind == VPPXEChunkKind.CELLS)
		{
			return VPPXEConst.CELLS_PER_CHUNK;
		}

		if (kind == VPPXEChunkKind.MARKERS)
		{
			return VPPXEConst.MARKERS_PER_CHUNK;
		}

		if (kind == VPPXEChunkKind.CIRCLES)
		{
			return VPPXEConst.CIRCLES_PER_CHUNK;
		}

		return VPPXEConst.TABLE_ROWS_PER_CHUNK;
	}

	protected int ClippedLength(string s)
	{
		int n = s.Length();
		if (n > VPPXEConst.MAX_CELL_CHARS)
		{
			n = VPPXEConst.MAX_CELL_CHARS + 3;
		}

		return n;
	}

	// payload estimate: each string = length + 4, each int/float = 4
	protected int ItemBytes(VPPXEDistLayerAcc acc, int kind, int idx)
	{
		if (kind == VPPXEChunkKind.CELLS)
		{
			return 16;
		}

		if (kind == VPPXEChunkKind.MARKERS || kind == VPPXEChunkKind.CIRCLES)
		{
			return 12;
		}

		int slot = acc.RowOrder[idx];
		return 32 + ClippedLength(acc.RowLabel[slot]) + ClippedLength(acc.RowDetail[slot]);
	}

	protected void OpenChunk(int layer, int kind)
	{
		m_CurChunk = new VPPXEDistChunk();
		m_CurChunk.ReqId = m_ReqId;
		m_CurChunk.Layer = layer;
		m_CurChunk.Kind = kind;
		if (!m_CurChunk.Ints)
		{
			m_CurChunk.Ints = new array<int>();
		}

		if (!m_CurChunk.Floats)
		{
			m_CurChunk.Floats = new array<float>();
		}

		if (!m_CurChunk.TableRows)
		{
			m_CurChunk.TableRows = new array<ref VPPXEDistTableRow>();
		}

		m_CurBytes = CHUNK_BASE_BYTES;
		m_CurRows = 0;
	}

	protected void CloseChunk()
	{
		if (m_CurChunk)
		{
			m_Chunks.Insert(m_CurChunk);
		}

		m_CurChunk = null;
		m_CurBytes = 0;
		m_CurRows = 0;
	}

	protected void AppendItem(VPPXEDistLayerAcc acc, int kind, int idx)
	{
		if (kind == VPPXEChunkKind.CELLS)
		{
			m_CurChunk.Ints.Insert(acc.CellIdx[idx]);
			m_CurChunk.Ints.Insert(acc.CellCount[idx]);
			m_CurChunk.Ints.Insert(acc.CellTier[idx]);
			m_CurChunk.Ints.Insert(acc.CellPts[idx]);
			return;
		}

		if (kind == VPPXEChunkKind.MARKERS)
		{
			int markerBase = idx * 3;
			m_CurChunk.Ints.Insert(acc.MarkerInts[markerBase]);
			m_CurChunk.Ints.Insert(acc.MarkerInts[markerBase + 1]);
			m_CurChunk.Ints.Insert(acc.MarkerInts[markerBase + 2]);
			return;
		}

		if (kind == VPPXEChunkKind.CIRCLES)
		{
			int circleBase = idx * 3;
			m_CurChunk.Floats.Insert(acc.Circles[circleBase]);
			m_CurChunk.Floats.Insert(acc.Circles[circleBase + 1]);
			m_CurChunk.Floats.Insert(acc.Circles[circleBase + 2]);
			return;
		}

		int slot = acc.RowOrder[idx];
		VPPXEDistTableRow tableRow = new VPPXEDistTableRow();
		tableRow.Layer = acc.Layer;
		tableRow.Label = VPPXmlText.Clip(acc.RowLabel[slot], VPPXEConst.MAX_CELL_CHARS);
		tableRow.Detail = VPPXmlText.Clip(acc.RowDetail[slot], VPPXEConst.MAX_CELL_CHARS);
		tableRow.Count = acc.RowCount[slot];
		tableRow.Points = acc.RowPoints[slot];
		tableRow.TierMask = acc.RowTier[slot];
		tableRow.X = acc.RowX[slot];
		tableRow.Z = acc.RowZ[slot];
		m_CurChunk.TableRows.Insert(tableRow);
	}

	// every chunk is built before the first send, so ChunkCount is known
	protected void ChunksUnit()
	{
		if (m_ChunkLayer >= m_Layers.Count())
		{
			CloseChunk();
			BuildSummary();
			NextPhase(PH_SEND);
			return;
		}

		VPPXEDistLayerAcc acc = m_Layers[m_ChunkLayer];
		int items = ItemCount(acc, m_ChunkKind);
		if (m_Cursor >= items)
		{
			CloseChunk();
			m_Cursor = 0;
			m_ChunkKind++;
			if (m_ChunkKind > VPPXEChunkKind.TABLE)
			{
				m_ChunkKind = 0;
				m_ChunkLayer++;
			}

			return;
		}

		int itemBytes = ItemBytes(acc, m_ChunkKind, m_Cursor);
		if (m_CurChunk && m_CurRows > 0)
		{
			if (m_CurRows >= KindCap(m_ChunkKind) || m_CurBytes + itemBytes > VPPXEConst.CHUNK_BYTES)
			{
				CloseChunk();
			}
		}

		if (!m_CurChunk)
		{
			OpenChunk(acc.Layer, m_ChunkKind);
		}

		AppendItem(acc, m_ChunkKind, m_Cursor);
		m_CurRows++;
		m_CurBytes = m_CurBytes + itemBytes;
		m_Cursor++;
	}

	protected void EnsureLen(array<int> values, int len)
	{
		while (values.Count() < len)
		{
			values.Insert(0);
		}
	}

	protected int ComputeFlags()
	{
		int flags = VPPXEDistFlag.RULES_INFERRED;
		if (m_Samples.Ok)
		{
			flags = flags | VPPXEDistFlag.AREAFLAGS_OK;
		}
		else
		{
			flags = flags | VPPXEDistFlag.AREAFLAGS_OFF;
		}

		if (m_HasT)
		{
			if (m_Nom0)
			{
				flags = flags | VPPXEDistFlag.NOMINAL_ZERO;
			}

			if (m_Ignored)
			{
				flags = flags | VPPXEDistFlag.IGNORED;
			}

			if (m_Deloot)
			{
				flags = flags | VPPXEDistFlag.DELOOT;
			}
		}
		else
		{
			flags = flags | VPPXEDistFlag.NO_CE_ENTRY;
		}

		if (m_Capped)
		{
			flags = flags | VPPXEDistFlag.CAPPED;
		}

		if (m_PlayerEvents.Count() > 0)
		{
			flags = flags | VPPXEDistFlag.PLAYER_EVENTS;
		}

		return flags;
	}

	protected void BuildSummary()
	{
		int chunkCount = m_Chunks.Count();
		for (int c = 0; c < chunkCount; c++)
		{
			VPPXEDistChunk chunk = m_Chunks[c];
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
		}

		m_Summary = new VPPXEDistSummary();
		m_Summary.ReqId = m_ReqId;
		m_Summary.TypeName = m_TypeName;
		m_Summary.Flags = ComputeFlags();
		m_Summary.CellSize = VPPXEConst.CELL_SIZE;
		m_Summary.GridCols = m_GridCols;
		m_Summary.PackScale = m_PackScale;
		m_Summary.ChunkCount = chunkCount;
		if (!m_Summary.LayerCounts)
		{
			m_Summary.LayerCounts = new array<int>();
		}

		if (!m_Summary.LayerCells)
		{
			m_Summary.LayerCells = new array<int>();
		}

		if (!m_Summary.LayerExtra)
		{
			m_Summary.LayerExtra = new array<int>();
		}

		if (!m_Summary.PlayerEvents)
		{
			m_Summary.PlayerEvents = new array<string>();
		}

		EnsureLen(m_Summary.LayerCounts, 8);
		EnsureLen(m_Summary.LayerCells, 8);
		EnsureLen(m_Summary.LayerExtra, 8);
		int tierAll = 0;
		for (int layer = 0; layer < m_Layers.Count(); layer++)
		{
			VPPXEDistLayerAcc acc = m_Layers[layer];
			tierAll = tierAll | acc.TierAll;
			int count = acc.Records;
			int extra = acc.PointsSum;
			int cells = 0;
			if (IsCellLayer(layer))
			{
				cells = acc.CellIdx.Count();
			}

			if (layer == VPPXELayer.BUILDINGS)
			{
				extra = acc.RowLabel.Count();
			}

			if (layer == VPPXELayer.CONTAINERS)
			{
				count = acc.RowLabel.Count();
			}

			m_Summary.LayerCounts.Set(layer, count);
			m_Summary.LayerCells.Set(layer, cells);
			m_Summary.LayerExtra.Set(layer, extra);
		}

		m_Summary.TierMaskAll = tierAll;
		for (int p = 0; p < m_PlayerEvents.Count(); p++)
		{
			if (p >= MAX_PLAYER_EVENTS)
			{
				break;
			}

			m_Summary.PlayerEvents.Insert(VPPXmlText.Clip(m_PlayerEvents[p], VPPXEConst.MAX_CELL_CHARS));
		}
	}

	// summary first (SendNow), then the paced chunks
	protected void SendUnit()
	{
		if (!m_Sender)
		{
			FinishQuery();
			return;
		}

		if (m_Cursor == 0)
		{
			VPPXENet.SendNow(m_Sender, "XE_OnDistSummary", new Param1<ref VPPXEDistSummary>(m_Summary));
			m_Cursor = 1;
			return;
		}

		int chunkIdx = m_Cursor - 1;
		if (chunkIdx >= m_Chunks.Count())
		{
			FinishQuery();
			return;
		}

		VPPXEDistChunk chunk = m_Chunks[chunkIdx];
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnDistChunk");
		Param1<ref VPPXEDistChunk> netPayload = new Param1<ref VPPXEDistChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnDistChunk", netRpc);
		m_Cursor++;
	}

	protected string Flag01(bool value)
	{
		if (value)
		{
			return "1";
		}

		return "0";
	}

	// exactly one INTERFACES section 9 log line per query
	protected void FinishQuery()
	{
		VPPXEDistLayerAcc buildings = Acc(VPPXELayer.BUILDINGS);
		VPPXEDistLayerAcc eventObjects = Acc(VPPXELayer.EVENT_OBJECTS);
		VPPXEDistLayerAcc eventChild = Acc(VPPXELayer.EVENT_CHILD);
		VPPXEDistLayerAcc infected = Acc(VPPXELayer.INFECTED);
		VPPXEDistLayerAcc clusters = Acc(VPPXELayer.CLUSTERS);
		VPPXEDistLayerAcc dispatch = Acc(VPPXELayer.DISPATCH);
		int b = buildings.Records;
		int bt = buildings.RowLabel.Count();
		int bp = buildings.PointsSum;
		int bc = buildings.CellIdx.Count();
		int eo = eventObjects.Records;
		int ec = eventChild.Records;
		int iz = infected.Records;
		int cl = clusters.Records;
		int cc = clusters.CellIdx.Count();
		int dp = dispatch.Records;
		string line = "[XMLEditor][Dist] type=" + m_TypeName + " af=" + Flag01(m_Samples.Ok);
		line = line + " b=" + b.ToString() + " bt=" + bt.ToString() + " bp=" + bp.ToString() + " bc=" + bc.ToString();
		line = line + " eo=" + eo.ToString() + " ec=" + ec.ToString() + " iz=" + iz.ToString();
		line = line + " cl=" + cl.ToString() + " cc=" + cc.ToString() + " dp=" + dp.ToString() + " cp=" + m_ParentHits.ToString();
		line = line + " nom0=" + Flag01(m_HasT && m_Nom0) + " ign=" + Flag01(m_HasT && m_Ignored) + " dl=" + Flag01(m_HasT && m_Deloot);
		Print(line);
		m_Phase = PH_DONE;
	}
};
