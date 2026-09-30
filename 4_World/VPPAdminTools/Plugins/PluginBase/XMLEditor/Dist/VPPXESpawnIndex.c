// XML Editor: distribution spawn index (LIGHT and MAP stages, DIST-ALGO v2.1 inputs).
// LIGHT: merged events (R11), cfgeventspawns and cfgeventgroups (R12), merged spawnabletypes and
// presets (R16), territory zones (R15), mapclusterproto (R17 cluster -> DE map) and the cross-file lint.
// MAP: mapgroupproto through a sink (R5, live containers), mapgrouppos streamed (R6) and the
// areaflags samples (R7). All name keys are lowercase. No DOM survives a stage.

class VPPXEDistUtil
{
	static string Lower(string s)
	{
		string r = s;
		r.ToLower();
		return r;
	}

	static bool StartsWith(string s, string prefix)
	{
		int n = prefix.Length();
		if (n == 0)
		{
			return true;
		}

		if (s.Length() < n)
		{
			return false;
		}

		string head = s.Substring(0, n);
		return head == prefix;
	}

	static bool Exists(VPPXEFileEntry entry)
	{
		if (!entry)
		{
			return false;
		}

		return (entry.Flags & VPPXEFileFlag.EXISTS) != 0;
	}

	// lowercase element name of a DOM node, "" for comments and null
	static string ElemName(VPPXmlNode node)
	{
		if (!node)
		{
			return "";
		}

		if (node.Kind != VPPXmlNodeKind.ELEMENT)
		{
			return "";
		}

		string n = node.Name;
		n.ToLower();
		return n;
	}

	// the document element: the root when its name matches, else its first child with that name, else the root
	static VPPXmlNode DocElement(VPPXmlNode root, string expected)
	{
		if (!root)
		{
			return null;
		}

		if (ElemName(root) == expected)
		{
			return root;
		}

		VPPXmlNode inner = root.FirstChild(expected);
		if (inner)
		{
			return inner;
		}

		return root;
	}

	static VPPXEIssue NewIssue(int code, string fileKey, int line, string entry, string arg)
	{
		VPPXEIssue issue = new VPPXEIssue();
		issue.Code = code;
		issue.Severity = VPPXEText.IssueSeverity(code);
		issue.FileKey = fileKey;
		issue.Line = line;
		issue.Entry = entry;
		issue.Arg = VPPXmlText.Clip(arg, VPPXEConst.MAX_CELL_CHARS);
		return issue;
	}

	static VPPXEIssue CopyIssue(VPPXEIssue src)
	{
		VPPXEIssue copy = new VPPXEIssue();
		copy.Code = src.Code;
		copy.Severity = src.Severity;
		copy.FileKey = src.FileKey;
		copy.Line = src.Line;
		copy.Entry = src.Entry;
		copy.Arg = src.Arg;
		return copy;
	}

	static int WorldSize()
	{
		int size = 0;
		XMLEditor editor = GetXMLEditor();
		if (editor && editor.GetRegistry())
		{
			size = editor.GetRegistry().GetWorldSize();
		}

		if (size <= 0)
		{
			size = 15360;
		}

		return size;
	}

	static int GridCols(int worldSize)
	{
		float cols = Math.Ceil(worldSize / 100.0);
		int result = cols;
		if (result < 1)
		{
			result = 1;
		}

		return result;
	}

	// R19 100 m cell: cx = floor(x / CELL_SIZE), cz = floor(z / CELL_SIZE), index = cz * gridCols + cx
	static int CellIndex(float x, float z, int gridCols)
	{
		int cx = Math.Floor(x / VPPXEConst.CELL_SIZE);
		int cz = Math.Floor(z / VPPXEConst.CELL_SIZE);
		cx = ClampInt(cx, 0, gridCols - 1);
		cz = ClampInt(cz, 0, gridCols - 1);
		return cz * gridCols + cx;
	}

	// attribute present with TrimWs value "0" (R5 lootmax rule)
	static bool IsZeroAttr(VPPXmlAttrs attrs, string name)
	{
		if (!attrs)
		{
			return false;
		}

		if (!attrs.Has(name))
		{
			return false;
		}

		string v = VPPXmlText.TrimWs(attrs.Get(name, ""));
		return v == "0";
	}

	static int ClampInt(int v, int lo, int hi)
	{
		if (v < lo)
		{
			return lo;
		}

		if (v > hi)
		{
			return hi;
		}

		return v;
	}
};

// ---------------------------------------------------------------------------------------------
// LIGHT data
// ---------------------------------------------------------------------------------------------

class VPPXEDistEvent : Managed
{
	string Name;
	string Key;
	string Position;
	string FileKey;
	int Line;
	bool Removed;
	ref array<string> ChildKeys;
	ref array<string> ChildNames;
	ref array<int> ChildLootmax;
	ref array<int> ChildDeloot;
	ref map<string, int> ChildIndex;

	void VPPXEDistEvent()
	{
		Line = 0;
		Removed = false;
		ChildKeys = new array<string>();
		ChildNames = new array<string>();
		ChildLootmax = new array<int>();
		ChildDeloot = new array<int>();
		ChildIndex = new map<string, int>();
	}

	int FindChild(string key)
	{
		int idx;
		if (ChildIndex.Find(key, idx))
		{
			return idx;
		}

		return -1;
	}

	// R11: children are upserted by type; the later entry replaces lootmax and deloot
	void UpsertChild(string key, string name, int lootmax, int deloot)
	{
		int idx = FindChild(key);
		if (idx >= 0)
		{
			ChildLootmax.Set(idx, lootmax);
			ChildDeloot.Set(idx, deloot);
			return;
		}

		idx = ChildKeys.Insert(key);
		ChildNames.Insert(name);
		ChildLootmax.Insert(lootmax);
		ChildDeloot.Insert(deloot);
		ChildIndex.Insert(key, idx);
	}
};

class VPPXEDistPreset : Managed
{
	string Kind;
	string Name;
	string Key;
	string ChanceText;
	string FileKey;
	int Line;
	bool Used;
	ref array<string> ItemKeys;
	ref array<string> ItemNames;
	ref array<string> ItemChances;

	void VPPXEDistPreset()
	{
		Line = 0;
		Used = false;
		ItemKeys = new array<string>();
		ItemNames = new array<string>();
		ItemChances = new array<string>();
	}
};

class VPPXEDistSpawnBlock : Managed
{
	string Kind;
	bool IsPreset;
	string PresetName;
	string ChanceText;
	string FileKey;
	int Line;
	ref array<string> ItemKeys;
	ref array<string> ItemNames;
	ref array<string> ItemChances;

	void VPPXEDistSpawnBlock()
	{
		IsPreset = false;
		Line = 0;
		ItemKeys = new array<string>();
		ItemNames = new array<string>();
		ItemChances = new array<string>();
	}
};

class VPPXEDistSpawnParent : Managed
{
	string Name;
	string Key;
	string FileKey;
	int Line;
	bool Hoarder;
	bool HasDamage;
	float DamageMin;
	float DamageMax;
	string Tag;
	ref array<ref VPPXEDistSpawnBlock> Blocks;

	void VPPXEDistSpawnParent()
	{
		Line = 0;
		Hoarder = false;
		HasDamage = false;
		DamageMin = 0;
		DamageMax = 0;
		Blocks = new array<ref VPPXEDistSpawnBlock>();
	}
};

// Immutable once the LIGHT job hands it to the service; query jobs keep their own ref.
class VPPXESpawnLight : Managed
{
	int SpawnRev;
	string ListSig;
	ref array<string> RecKeys;
	ref array<int> RecRevs;

	// events (R11), merged order incl. removed entries
	ref array<ref VPPXEDistEvent> Events;
	ref map<string, int> EventIndex;
	ref map<string, bool> DefinedEvents;
	ref array<ref VPPXEDistEvent> Active;
	ref array<int> ActiveSpawn;
	ref map<string, int> ActiveIndex;

	// cfgeventgroups (R12, first wins); GroupDist = first-occurrence child indexes per group
	string GroupsFileKey;
	ref array<string> GroupKeys;
	ref array<string> GroupNames;
	ref map<string, int> GroupIndex;
	ref array<int> GroupChildStart;
	ref array<int> GroupChildCount;
	ref array<int> GroupDistStart;
	ref array<int> GroupDistCount;
	ref array<int> GroupDist;
	ref array<string> GcKeys;
	ref array<string> GcNames;
	ref array<float> GcX;
	ref array<float> GcZ;
	ref array<int> GcLootmax;
	ref array<int> GcDeloot;

	// cfgeventspawns (R12); PosGroup: group index, -1 none, -2 unknown group name
	string SpawnsFileKey;
	ref array<string> SpKeys;
	ref array<string> SpNames;
	ref array<int> SpLine;
	ref map<string, int> SpIndex;
	ref array<ref array<int>> SpPos;
	ref array<float> PosX;
	ref array<float> PosZ;
	ref array<int> PosGroup;

	// spawnabletypes and presets (R16)
	ref array<ref VPPXEDistSpawnParent> Parents;
	ref map<string, int> ParentIndex;
	ref array<int> ParentOrder;
	ref map<string, ref VPPXEDistPreset> Presets;
	ref array<string> PresetOrder;
	ref map<string, ref array<int>> ItemParents;

	// territory zones (R15); only zones whose lowercase name starts with "infected" can ever match
	ref array<string> ZoneKeys;
	ref array<float> ZoneX;
	ref array<float> ZoneZ;
	ref array<float> ZoneR;

	// mapclusterproto (R17)
	ref VPPXEClusterProto Clusters;

	// precomputed cross-file issues of this stage
	ref array<ref VPPXEIssue> Issues;

	void VPPXESpawnLight()
	{
		SpawnRev = 0;
		RecKeys = new array<string>();
		RecRevs = new array<int>();
		Events = new array<ref VPPXEDistEvent>();
		EventIndex = new map<string, int>();
		DefinedEvents = new map<string, bool>();
		Active = new array<ref VPPXEDistEvent>();
		ActiveSpawn = new array<int>();
		ActiveIndex = new map<string, int>();
		GroupKeys = new array<string>();
		GroupNames = new array<string>();
		GroupIndex = new map<string, int>();
		GroupChildStart = new array<int>();
		GroupChildCount = new array<int>();
		GroupDistStart = new array<int>();
		GroupDistCount = new array<int>();
		GroupDist = new array<int>();
		GcKeys = new array<string>();
		GcNames = new array<string>();
		GcX = new array<float>();
		GcZ = new array<float>();
		GcLootmax = new array<int>();
		GcDeloot = new array<int>();
		SpKeys = new array<string>();
		SpNames = new array<string>();
		SpLine = new array<int>();
		SpIndex = new map<string, int>();
		SpPos = new array<ref array<int>>();
		PosX = new array<float>();
		PosZ = new array<float>();
		PosGroup = new array<int>();
		Parents = new array<ref VPPXEDistSpawnParent>();
		ParentIndex = new map<string, int>();
		ParentOrder = new array<int>();
		Presets = new map<string, ref VPPXEDistPreset>();
		PresetOrder = new array<string>();
		ItemParents = new map<string, ref array<int>>();
		ZoneKeys = new array<string>();
		ZoneX = new array<float>();
		ZoneZ = new array<float>();
		ZoneR = new array<float>();
		Clusters = new VPPXEClusterProto();
		Issues = new array<ref VPPXEIssue>();
	}

	// index of the first child of type key among the distinct children of an event group, or -1
	int GroupFirstChild(int groupIdx, string key)
	{
		if (groupIdx < 0 || groupIdx >= GroupKeys.Count())
		{
			return -1;
		}

		int start = GroupDistStart[groupIdx];
		int count = GroupDistCount[groupIdx];
		for (int i = 0; i < count; i++)
		{
			int childIdx = GroupDist[start + i];
			if (GcKeys[childIdx] == key)
			{
				return childIdx;
			}
		}

		return -1;
	}

	// R17 trigger: the type is a child of an active event that is the de of some cluster
	bool NeedsClusters(string typeKey)
	{
		for (int i = 0; i < Active.Count(); i++)
		{
			VPPXEDistEvent ev = Active[i];
			if (ev.FindChild(typeKey) < 0)
			{
				continue;
			}

			if (Clusters.FindDe(ev.Key) >= 0)
			{
				return true;
			}
		}

		return false;
	}
};

// ---------------------------------------------------------------------------------------------
// MAP data
// ---------------------------------------------------------------------------------------------

class VPPXESpawnMap : Managed
{
	int SpawnRev;
	int PosRev;
	string ProtoKey;
	string PosKey;

	// mapgroupproto groups (R5, first wins)
	ref array<string> ProtoKeys;
	ref array<string> ProtoNames;
	ref map<string, int> ProtoIndex;
	ref array<int> ProtoUsage;
	ref array<int> ProtoValue;
	ref array<bool> ProtoHasValue;
	ref array<int> ProtoContStart;
	ref array<int> ProtoContCount;
	ref array<int> ProtoDispStart;
	ref array<int> ProtoDispCount;
	ref array<int> ProtoFirstPos;
	ref array<int> ProtoLastPos;

	// containers (flat, contiguous per group)
	ref array<int> ContCat;
	ref array<int> ContTag;
	ref array<int> ContPoints;
	ref array<bool> ContLive;

	// dispatch proxies (lowercase types, contiguous per group) and the reverse map type -> groups
	ref array<string> DispTypes;
	ref map<string, ref array<int>> DispatchGroups;

	// mapgrouppos (R6), file order; PosNext links the instances of one proto group in file order
	ref array<float> PosX;
	ref array<float> PosZ;
	ref array<int> PosProto;
	ref array<int> PosNext;
	int PosNoProto;
	string PosNoProtoFirst;
	int PosNoProtoLine;
	int PosLines;

	// areaflags samples (R7); replaced as a whole on revalidation
	ref VPPXEAreaSamples Samples;

	ref array<ref VPPXEIssue> Issues;

	void VPPXESpawnMap()
	{
		SpawnRev = 0;
		PosRev = 0;
		PosNoProto = 0;
		PosNoProtoLine = 0;
		PosLines = 0;
		ProtoKeys = new array<string>();
		ProtoNames = new array<string>();
		ProtoIndex = new map<string, int>();
		ProtoUsage = new array<int>();
		ProtoValue = new array<int>();
		ProtoHasValue = new array<bool>();
		ProtoContStart = new array<int>();
		ProtoContCount = new array<int>();
		ProtoDispStart = new array<int>();
		ProtoDispCount = new array<int>();
		ProtoFirstPos = new array<int>();
		ProtoLastPos = new array<int>();
		ContCat = new array<int>();
		ContTag = new array<int>();
		ContPoints = new array<int>();
		ContLive = new array<bool>();
		DispTypes = new array<string>();
		DispatchGroups = new map<string, ref array<int>>();
		PosX = new array<float>();
		PosZ = new array<float>();
		PosProto = new array<int>();
		PosNext = new array<int>();
		Issues = new array<ref VPPXEIssue>();
	}

	int FindProto(string key)
	{
		int idx;
		if (ProtoIndex.Find(key, idx))
		{
			return idx;
		}

		return -1;
	}

	void AddPosition(float x, float z, int protoIdx)
	{
		int p = PosX.Insert(x);
		PosZ.Insert(z);
		PosProto.Insert(protoIdx);
		PosNext.Insert(-1);
		if (protoIdx < 0)
		{
			return;
		}

		int last = ProtoLastPos[protoIdx];
		if (last < 0)
		{
			ProtoFirstPos.Set(protoIdx, p);
		}
		else
		{
			PosNext.Set(last, p);
		}

		ProtoLastPos.Set(protoIdx, p);
	}
};

// ---------------------------------------------------------------------------------------------
// mapgroupproto sink (R5): no attribute work for <point>, which is only counted per container
// ---------------------------------------------------------------------------------------------

class VPPXEProtoSink : VPPXmlSink
{
	protected VPPXESpawnMap m_Data;
	protected VPPXELimits m_Limits;
	protected int m_Depth;
	protected int m_Group;
	protected bool m_GroupDead;
	protected int m_Cont;
	protected bool m_ContDead;
	protected bool m_InDispatch;
	protected string m_ParseError;
	protected int m_ParseErrorLine;
	protected ref map<string, int> m_UsageBits;
	protected ref map<string, int> m_ValueBits;
	protected ref map<string, int> m_CatBits;
	protected ref map<string, int> m_TagBits;

	void VPPXEProtoSink(VPPXESpawnMap data, VPPXELimits limits)
	{
		m_Data = data;
		m_Limits = limits;
		m_Depth = 0;
		m_Group = -1;
		m_GroupDead = false;
		m_Cont = -1;
		m_ContDead = false;
		m_InDispatch = false;
		m_ParseError = "";
		m_ParseErrorLine = 0;
		m_UsageBits = new map<string, int>();
		m_ValueBits = new map<string, int>();
		m_CatBits = new map<string, int>();
		m_TagBits = new map<string, int>();
	}

	string GetParseError()
	{
		return m_ParseError;
	}

	int GetParseErrorLine()
	{
		return m_ParseErrorLine;
	}

	override bool WantsAttributes(string name)
	{
		string lname = name;
		lname.ToLower();
		if (lname == "point")
		{
			return false;
		}

		return true;
	}

	override void OnStartElement(string name, VPPXmlAttrs attrs, int line, int col, int endLine, int endCol, bool selfClosing)
	{
		int depth = m_Depth;
		if (!selfClosing)
		{
			m_Depth++;
		}

		string lname = name;
		lname.ToLower();

		if (depth == 1)
		{
			if (lname == "group")
			{
				StartGroup(attrs);
				if (selfClosing)
				{
					EndGroup();
				}
			}

			return;
		}

		if (m_Group < 0)
		{
			return;
		}

		if (depth == 2)
		{
			if (lname == "usage")
			{
				AddGroupBit(m_UsageBits, 0, attrs.Get("name", ""));
			}
			else if (lname == "value")
			{
				AddGroupBit(m_ValueBits, 1, attrs.Get("name", ""));
			}
			else if (lname == "container")
			{
				StartContainer(attrs);
				if (selfClosing)
				{
					EndContainer();
				}
			}
			else if (lname == "dispatch")
			{
				if (!selfClosing)
				{
					m_InDispatch = true;
				}
			}

			return;
		}

		if (depth != 3)
		{
			return;
		}

		if (m_Cont >= 0)
		{
			if (lname == "category")
			{
				AddContainerBit(m_CatBits, 2, attrs.Get("name", ""));
			}
			else if (lname == "tag")
			{
				AddContainerBit(m_TagBits, 3, attrs.Get("name", ""));
			}
			else if (lname == "point")
			{
				int points = m_Data.ContPoints[m_Cont];
				m_Data.ContPoints.Set(m_Cont, points + 1);
			}

			return;
		}

		if (m_InDispatch && lname == "proxy")
		{
			string proxyType = attrs.Get("type", "");
			if (proxyType != "")
			{
				m_Data.DispTypes.Insert(VPPXEDistUtil.Lower(proxyType));
				int dispCount = m_Data.ProtoDispCount[m_Group];
				m_Data.ProtoDispCount.Set(m_Group, dispCount + 1);
			}
		}
	}

	override void OnEndElement(string name, int line, int col, int endLine, int endCol)
	{
		if (m_Depth <= 0)
		{
			return;
		}

		m_Depth--;
		if (m_Depth == 2)
		{
			if (m_Cont >= 0)
			{
				EndContainer();
			}

			m_InDispatch = false;
			return;
		}

		if (m_Depth == 1)
		{
			EndGroup();
		}
	}

	override void OnError(string message, int line)
	{
		m_ParseError = message;
		m_ParseErrorLine = line;
	}

	void FinishParse()
	{
		if (m_Group >= 0)
		{
			EndGroup();
		}
	}

	protected void StartGroup(VPPXmlAttrs attrs)
	{
		m_Group = -1;
		m_Cont = -1;
		m_InDispatch = false;
		string groupName = attrs.Get("name", "");
		string groupKey = VPPXEDistUtil.Lower(groupName);
		if (groupKey == "")
		{
			return;
		}

		// R5: the first definition wins
		if (m_Data.ProtoIndex.Contains(groupKey))
		{
			return;
		}

		m_Group = m_Data.ProtoKeys.Insert(groupKey);
		m_Data.ProtoNames.Insert(groupName);
		m_Data.ProtoIndex.Insert(groupKey, m_Group);
		m_Data.ProtoUsage.Insert(0);
		m_Data.ProtoValue.Insert(0);
		m_Data.ProtoHasValue.Insert(false);
		m_Data.ProtoContStart.Insert(m_Data.ContCat.Count());
		m_Data.ProtoContCount.Insert(0);
		m_Data.ProtoDispStart.Insert(m_Data.DispTypes.Count());
		m_Data.ProtoDispCount.Insert(0);
		m_Data.ProtoFirstPos.Insert(-1);
		m_Data.ProtoLastPos.Insert(-1);
		m_GroupDead = VPPXEDistUtil.IsZeroAttr(attrs, "lootmax");
	}

	protected void EndGroup()
	{
		if (m_Cont >= 0)
		{
			EndContainer();
		}

		m_Group = -1;
		m_InDispatch = false;
	}

	protected void StartContainer(VPPXmlAttrs attrs)
	{
		if (m_Cont >= 0)
		{
			EndContainer();
		}

		m_Cont = m_Data.ContCat.Insert(0);
		m_Data.ContTag.Insert(0);
		m_Data.ContPoints.Insert(0);
		m_Data.ContLive.Insert(false);
		m_ContDead = VPPXEDistUtil.IsZeroAttr(attrs, "lootmax");
		int contCount = m_Data.ProtoContCount[m_Group];
		m_Data.ProtoContCount.Set(m_Group, contCount + 1);
	}

	// R5 live = points >= 1 AND container lootmax not "0" AND group lootmax not "0"
	protected void EndContainer()
	{
		int points = m_Data.ContPoints[m_Cont];
		bool live = points >= 1 && !m_ContDead && !m_GroupDead;
		m_Data.ContLive.Set(m_Cont, live);
		m_Cont = -1;
	}

	// kind: 0 usage, 1 value, 2 category, 3 tag
	protected int BitIndex(map<string, int> cache, int kind, string name)
	{
		int idx;
		if (cache.Find(name, idx))
		{
			return idx;
		}

		idx = -1;
		if (m_Limits)
		{
			if (kind == 0)
			{
				idx = m_Limits.FindUsage(name);
			}
			else if (kind == 1)
			{
				idx = m_Limits.FindValue(name);
			}
			else if (kind == 2)
			{
				idx = m_Limits.FindCategory(name);
			}
			else
			{
				idx = m_Limits.FindTag(name);
			}
		}

		if (idx >= 32)
		{
			idx = -1;
		}

		cache.Insert(name, idx);
		return idx;
	}

	protected void AddGroupBit(map<string, int> cache, int kind, string name)
	{
		int idx = BitIndex(cache, kind, name);
		if (idx < 0)
		{
			return;
		}

		int bit = 1 << idx;
		if (kind == 0)
		{
			int usage = m_Data.ProtoUsage[m_Group];
			m_Data.ProtoUsage.Set(m_Group, usage | bit);
			return;
		}

		int value = m_Data.ProtoValue[m_Group];
		m_Data.ProtoValue.Set(m_Group, value | bit);
		m_Data.ProtoHasValue.Set(m_Group, true);
	}

	protected void AddContainerBit(map<string, int> cache, int kind, string name)
	{
		int idx = BitIndex(cache, kind, name);
		if (idx < 0)
		{
			return;
		}

		int bit = 1 << idx;
		if (kind == 2)
		{
			int cat = m_Data.ContCat[m_Cont];
			m_Data.ContCat.Set(m_Cont, cat | bit);
			return;
		}

		int tag = m_Data.ContTag[m_Cont];
		m_Data.ContTag.Set(m_Cont, tag | bit);
	}
};

// ---------------------------------------------------------------------------------------------
// LIGHT stage job (BACKGROUND)
// ---------------------------------------------------------------------------------------------

class VPPXESpawnLightJob : VPPXEJob
{
	const static int PH_TASKS = 0;
	const static int PH_FILES = 1;
	const static int PH_RESOLVE = 2;
	const static int PH_SORT_KEYS = 3;
	const static int PH_SORT = 4;
	const static int PH_ORDER = 5;
	const static int PH_REVERSE = 6;
	const static int PH_ACTIVE = 7;
	const static int PH_LINT_SPAWNS = 8;
	const static int PH_LINT_GROUPS = 9;
	const static int PH_LINT_PRESETS = 10;
	const static int PH_LINT_EVENTS = 11;
	const static int PH_DONE = 12;

	protected VPPXEDistService m_Service;
	protected ref VPPXESpawnLight m_Data;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_StartMs;
	protected ref array<int> m_TaskKind;
	protected ref array<string> m_TaskKey;
	protected ref array<string> m_TaskPath;
	protected int m_Task;
	protected ref VPPXmlDocument m_Doc;
	protected bool m_DocOpen;
	protected bool m_DocParsed;
	protected VPPXmlNode m_El;
	protected int m_Child;
	protected bool m_FileHasDamage;
	protected float m_FileDmgMin;
	protected float m_FileDmgMax;
	protected ref array<VPPXmlNode> m_WalkNodes;
	protected ref array<int> m_WalkIdx;
	protected ref map<string, bool> m_TerritorySeen;
	protected ref array<string> m_SortKeys;
	protected ref map<string, int> m_MissingGroups;
	protected ref array<string> m_MissingGroupNames;
	protected ref array<int> m_MissingGroupLines;

	void VPPXESpawnLightJob(VPPXEDistService service)
	{
		m_Service = service;
		m_Data = new VPPXESpawnLight();
		m_Phase = PH_TASKS;
		m_Cursor = 0;
		m_StartMs = GetGame().GetTime();
		m_TaskKind = new array<int>();
		m_TaskKey = new array<string>();
		m_TaskPath = new array<string>();
		m_Task = 0;
		m_DocOpen = false;
		m_DocParsed = false;
		m_Child = 0;
		m_FileHasDamage = false;
		m_FileDmgMin = 0;
		m_FileDmgMax = 0;
		m_WalkNodes = new array<VPPXmlNode>();
		m_WalkIdx = new array<int>();
		m_TerritorySeen = new map<string, bool>();
		m_SortKeys = new array<string>();
		m_MissingGroups = new map<string, int>();
		m_MissingGroupNames = new array<string>();
		m_MissingGroupLines = new array<int>();
	}

	VPPXESpawnLight GetLightData()
	{
		return m_Data;
	}

	override string GetLabel()
	{
		return "dist LIGHT";
	}

	protected void ReportStageProgress(int stage, int pct)
	{
		if (m_Service)
		{
			m_Service.ReportProgress(stage, pct);
		}
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_DONE)
			{
				return true;
			}

			if (m_Phase == PH_TASKS)
			{
				BeginTasks();
			}
			else if (m_Phase == PH_FILES)
			{
				FilesUnit();
			}
			else if (m_Phase == PH_RESOLVE)
			{
				ResolveUnit();
			}
			else if (m_Phase == PH_SORT_KEYS)
			{
				SortKeyUnit();
			}
			else if (m_Phase == PH_SORT)
			{
				m_SortKeys.Sort();
				m_Cursor = 0;
				m_Phase = PH_ORDER;
			}
			else if (m_Phase == PH_ORDER)
			{
				OrderUnit();
			}
			else if (m_Phase == PH_REVERSE)
			{
				ReverseUnit();
			}
			else if (m_Phase == PH_ACTIVE)
			{
				ActiveUnit();
			}
			else if (m_Phase == PH_LINT_SPAWNS)
			{
				LintSpawnsUnit();
			}
			else if (m_Phase == PH_LINT_GROUPS)
			{
				LintGroupsUnit();
			}
			else if (m_Phase == PH_LINT_PRESETS)
			{
				LintPresetsUnit();
			}
			else if (m_Phase == PH_LINT_EVENTS)
			{
				LintEventsUnit();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnFinished()
	{
		m_Doc = null;
		if (m_Service)
		{
			m_Service.OnLightJobDone(this, true);
		}
	}

	override void OnAborted()
	{
		m_Doc = null;
		VPPXELog.Info("[Dist] LIGHT stage aborted with a script error");
		if (m_Service)
		{
			m_Service.OnLightJobDone(this, false);
		}
	}

	// R1 order per kind; revisions recorded for the lazy invalidation check
	protected void BeginTasks()
	{
		m_Phase = PH_FILES;
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			return;
		}

		VPPXEFileRegistry reg = editor.GetRegistry();
		AddKindTasks(reg, VPPXEFileKind.RANDOMPRESETS);
		AddKindTasks(reg, VPPXEFileKind.SPAWNABLETYPES);
		AddKindTasks(reg, VPPXEFileKind.EVENTS);
		AddKindTasks(reg, VPPXEFileKind.EVENTGROUPS);
		AddKindTasks(reg, VPPXEFileKind.EVENTSPAWNS);
		AddKindTasks(reg, VPPXEFileKind.CLUSTERPROTO);
		AddKindTasks(reg, VPPXEFileKind.TERRITORY);
		m_Data.ListSig = VPPXEDistService.BuildListSig(reg);
		ReportStageProgress(VPPXEStage.SPAWN_LIGHT, 0);
	}

	protected void AddKindTasks(VPPXEFileRegistry reg, int kind)
	{
		array<VPPXEFileEntry> list = new array<VPPXEFileEntry>();
		reg.GetByKind(kind, list);
		for (int i = 0; i < list.Count(); i++)
		{
			VPPXEFileEntry entry = list[i];
			if (!entry)
			{
				continue;
			}

			m_Data.RecKeys.Insert(entry.Key);
			m_Data.RecRevs.Insert(entry.Revision);
			if (kind == VPPXEFileKind.EVENTSPAWNS || kind == VPPXEFileKind.EVENTGROUPS)
			{
				m_Data.SpawnRev = m_Data.SpawnRev * 31 + entry.Revision;
			}

			if (!VPPXEDistUtil.Exists(entry))
			{
				continue;
			}

			if (kind == VPPXEFileKind.TERRITORY)
			{
				string seenKey = VPPXEDistUtil.Lower(entry.Key);
				if (m_TerritorySeen.Contains(seenKey))
				{
					continue;
				}

				m_TerritorySeen.Insert(seenKey, true);
			}

			m_TaskKind.Insert(kind);
			m_TaskKey.Insert(entry.Key);
			m_TaskPath.Insert(entry.Path);
		}
	}

	protected string RootNameOf(int kind)
	{
		if (kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return "randompresets";
		}

		if (kind == VPPXEFileKind.SPAWNABLETYPES)
		{
			return "spawnabletypes";
		}

		if (kind == VPPXEFileKind.EVENTS)
		{
			return "events";
		}

		if (kind == VPPXEFileKind.EVENTGROUPS)
		{
			return "eventgroupdef";
		}

		if (kind == VPPXEFileKind.EVENTSPAWNS)
		{
			return "eventposdef";
		}

		if (kind == VPPXEFileKind.CLUSTERPROTO)
		{
			return "prototype";
		}

		return "territory-type";
	}

	protected void FilesUnit()
	{
		if (m_Task >= m_TaskKind.Count())
		{
			m_Phase = PH_RESOLVE;
			m_Cursor = 0;
			return;
		}

		if (!m_DocOpen)
		{
			OpenTask();
			return;
		}

		if (!m_DocParsed)
		{
			if (!m_Doc.Step())
			{
				return;
			}

			m_DocParsed = true;
			if (m_Doc.HasError())
			{
				string errText = "[Dist] LIGHT skipped " + m_TaskKey[m_Task] + " (line " + m_Doc.GetErrorLine().ToString() + ": " + m_Doc.GetError() + ")";
				VPPXELog.Info(errText);
				EndTask();
				return;
			}

			BeginWalk();
			return;
		}

		WalkUnit();
	}

	protected void OpenTask()
	{
		m_Doc = new VPPXmlDocument();
		m_DocOpen = true;
		m_DocParsed = false;
		if (!m_Doc.Load(m_TaskPath[m_Task]))
		{
			VPPXELog.Info("[Dist] LIGHT could not read " + m_TaskKey[m_Task]);
			EndTask();
		}
	}

	protected void EndTask()
	{
		m_Doc = null;
		m_El = null;
		m_DocOpen = false;
		m_DocParsed = false;
		m_Child = 0;
		m_WalkNodes.Clear();
		m_WalkIdx.Clear();
		m_Task++;
		int total = m_TaskKind.Count();
		if (total < 1)
		{
			total = 1;
		}

		ReportStageProgress(VPPXEStage.SPAWN_LIGHT, m_Task * 90 / total);
	}

	protected void BeginWalk()
	{
		int kind = m_TaskKind[m_Task];
		VPPXmlNode root = m_Doc.GetRoot();
		m_Child = 0;
		m_WalkNodes.Clear();
		m_WalkIdx.Clear();
		if (kind == VPPXEFileKind.TERRITORY)
		{
			if (root)
			{
				m_WalkNodes.Insert(root);
				m_WalkIdx.Insert(0);
			}

			return;
		}

		m_El = VPPXEDistUtil.DocElement(root, RootNameOf(kind));
		m_FileHasDamage = false;
		if (kind == VPPXEFileKind.SPAWNABLETYPES && m_El)
		{
			VPPXmlNode fileDamage = m_El.FirstChild("damage");
			if (fileDamage)
			{
				m_FileHasDamage = true;
				m_FileDmgMin = fileDamage.GetAttr("min", "0").ToFloat();
				m_FileDmgMax = fileDamage.GetAttr("max", "0").ToFloat();
			}
		}
	}

	// one top-level element per unit (one DFS step for territory files)
	protected void WalkUnit()
	{
		int kind = m_TaskKind[m_Task];
		if (kind == VPPXEFileKind.TERRITORY)
		{
			ZoneUnit();
			return;
		}

		if (!m_El || m_Child >= m_El.ChildCount())
		{
			EndTask();
			return;
		}

		VPPXmlNode node = m_El.ChildAt(m_Child);
		m_Child++;
		string nodeName = VPPXEDistUtil.ElemName(node);
		if (nodeName == "")
		{
			return;
		}

		string fileKey = m_TaskKey[m_Task];
		if (kind == VPPXEFileKind.RANDOMPRESETS)
		{
			if (nodeName == "cargo" || nodeName == "attachments")
			{
				ParsePreset(node, nodeName, fileKey);
			}

			return;
		}

		if (kind == VPPXEFileKind.SPAWNABLETYPES)
		{
			if (nodeName == "type")
			{
				ParseSpawnable(node, fileKey);
			}

			return;
		}

		if (kind == VPPXEFileKind.EVENTS)
		{
			if (nodeName == "event")
			{
				ParseEvent(node, fileKey);
			}

			return;
		}

		if (kind == VPPXEFileKind.EVENTGROUPS)
		{
			if (nodeName == "group")
			{
				ParseEventGroup(node, fileKey);
			}

			return;
		}

		if (kind == VPPXEFileKind.EVENTSPAWNS)
		{
			if (nodeName == "event")
			{
				ParseEventSpawn(node, fileKey);
			}

			return;
		}

		if (kind == VPPXEFileKind.CLUSTERPROTO)
		{
			if (nodeName == "cluster")
			{
				m_Data.Clusters.AddCluster(node);
			}
		}
	}

	// every <zone> element at any depth, in document order
	protected void ZoneUnit()
	{
		int top = m_WalkNodes.Count() - 1;
		if (top < 0)
		{
			EndTask();
			return;
		}

		VPPXmlNode node = m_WalkNodes[top];
		int childIdx = m_WalkIdx[top];
		if (!node || childIdx >= node.ChildCount())
		{
			m_WalkNodes.Remove(top);
			m_WalkIdx.Remove(top);
			return;
		}

		m_WalkIdx.Set(top, childIdx + 1);
		VPPXmlNode child = node.ChildAt(childIdx);
		string childName = VPPXEDistUtil.ElemName(child);
		if (childName == "")
		{
			return;
		}

		if (childName == "zone")
		{
			AddZone(child);
			return;
		}

		m_WalkNodes.Insert(child);
		m_WalkIdx.Insert(0);
	}

	protected void AddZone(VPPXmlNode node)
	{
		string zoneKey = VPPXEDistUtil.Lower(node.GetAttr("name", ""));
		if (!VPPXEDistUtil.StartsWith(zoneKey, "infected"))
		{
			return;
		}

		m_Data.ZoneKeys.Insert(zoneKey);
		m_Data.ZoneX.Insert(node.GetAttr("x", "0").ToFloat());
		m_Data.ZoneZ.Insert(node.GetAttr("z", "0").ToFloat());
		m_Data.ZoneR.Insert(node.GetAttr("r", "0").ToFloat());
	}

	// R16 presets: a later (kind, name) definition replaces the earlier one
	protected void ParsePreset(VPPXmlNode node, string kindName, string fileKey)
	{
		string presetName = node.GetAttr("name", "");
		if (presetName == "")
		{
			return;
		}

		VPPXEDistPreset preset = new VPPXEDistPreset();
		preset.Kind = kindName;
		preset.Name = presetName;
		preset.Key = kindName + "|" + VPPXEDistUtil.Lower(presetName);
		preset.ChanceText = node.GetAttr("chance", "");
		preset.FileKey = fileKey;
		preset.Line = node.StartLine + 1;
		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode item = node.ChildAt(i);
			if (VPPXEDistUtil.ElemName(item) != "item")
			{
				continue;
			}

			string itemName = item.GetAttr("name", "");
			if (itemName == "")
			{
				continue;
			}

			preset.ItemKeys.Insert(VPPXEDistUtil.Lower(itemName));
			preset.ItemNames.Insert(itemName);
			preset.ItemChances.Insert(item.GetAttr("chance", ""));
		}

		bool existed = m_Data.Presets.Contains(preset.Key);
		m_Data.Presets.Set(preset.Key, preset);
		if (!existed)
		{
			m_Data.PresetOrder.Insert(preset.Key);
		}
	}

	// R16 spawnabletypes merged across files: a later block list replaces the earlier one only if non-empty
	protected void ParseSpawnable(VPPXmlNode node, string fileKey)
	{
		string typeName = node.GetAttr("name", "");
		if (typeName == "")
		{
			return;
		}

		string typeKey = VPPXEDistUtil.Lower(typeName);
		VPPXEDistSpawnParent parent;
		int parentIdx;
		bool existed = m_Data.ParentIndex.Find(typeKey, parentIdx);
		if (existed)
		{
			parent = m_Data.Parents[parentIdx];
		}
		else
		{
			parent = new VPPXEDistSpawnParent();
			parent.Name = typeName;
			parent.Key = typeKey;
			parentIdx = m_Data.Parents.Insert(parent);
			m_Data.ParentIndex.Insert(typeKey, parentIdx);
		}

		parent.FileKey = fileKey;
		parent.Line = node.StartLine + 1;
		array<ref VPPXEDistSpawnBlock> blocks = new array<ref VPPXEDistSpawnBlock>();
		bool ownDamage = false;
		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode child = node.ChildAt(i);
			string childName = VPPXEDistUtil.ElemName(child);
			if (childName == "hoarder")
			{
				parent.Hoarder = true;
			}
			else if (childName == "damage")
			{
				ownDamage = true;
				parent.HasDamage = true;
				parent.DamageMin = child.GetAttr("min", "0").ToFloat();
				parent.DamageMax = child.GetAttr("max", "0").ToFloat();
			}
			else if (childName == "tag")
			{
				parent.Tag = child.GetAttr("name", "");
			}
			else if (childName == "attachments" || childName == "cargo")
			{
				blocks.Insert(ParseBlock(child, childName, fileKey));
			}
		}

		if (!ownDamage && m_FileHasDamage)
		{
			parent.HasDamage = true;
			parent.DamageMin = m_FileDmgMin;
			parent.DamageMax = m_FileDmgMax;
		}

		if (blocks.Count() > 0 || !existed)
		{
			parent.Blocks = blocks;
		}
	}

	protected VPPXEDistSpawnBlock ParseBlock(VPPXmlNode node, string kindName, string fileKey)
	{
		VPPXEDistSpawnBlock block = new VPPXEDistSpawnBlock();
		block.Kind = kindName;
		block.FileKey = fileKey;
		block.Line = node.StartLine + 1;
		block.ChanceText = node.GetAttr("chance", "");
		string presetName = node.GetAttr("preset", "");
		if (presetName != "")
		{
			block.IsPreset = true;
			block.PresetName = presetName;
			return block;
		}

		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode item = node.ChildAt(i);
			if (VPPXEDistUtil.ElemName(item) != "item")
			{
				continue;
			}

			string itemName = item.GetAttr("name", "");
			if (itemName == "")
			{
				continue;
			}

			block.ItemKeys.Insert(VPPXEDistUtil.Lower(itemName));
			block.ItemNames.Insert(itemName);
			block.ItemChances.Insert(item.GetAttr("chance", ""));
		}

		return block;
	}

	// R11: <active> "0" removes the event with everything merged so far; no <active> counts as active
	protected void ParseEvent(VPPXmlNode node, string fileKey)
	{
		string eventName = node.GetAttr("name", "");
		if (eventName == "")
		{
			return;
		}

		string eventKey = VPPXEDistUtil.Lower(eventName);
		m_Data.DefinedEvents.Set(eventKey, true);
		VPPXmlNode activeNode = node.FirstChild("active");
		if (activeNode)
		{
			string activeText = VPPXmlText.TrimWs(activeNode.InnerText());
			if (activeText == "0")
			{
				RemoveEvent(eventKey);
				return;
			}
		}

		VPPXEDistEvent ev = FindOrAddEvent(eventKey, eventName);
		ev.FileKey = fileKey;
		ev.Line = node.StartLine + 1;
		VPPXmlNode positionNode = node.FirstChild("position");
		if (positionNode)
		{
			string positionText = VPPXmlText.TrimWs(positionNode.InnerText());
			ev.Position = VPPXEDistUtil.Lower(positionText);
		}

		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode child = node.ChildAt(i);
			string childName = VPPXEDistUtil.ElemName(child);
			if (childName == "child")
			{
				AddEventChild(ev, child);
			}
			else if (childName == "children")
			{
				int m = child.ChildCount();
				for (int j = 0; j < m; j++)
				{
					VPPXmlNode grandChild = child.ChildAt(j);
					if (VPPXEDistUtil.ElemName(grandChild) == "child")
					{
						AddEventChild(ev, grandChild);
					}
				}
			}
		}
	}

	protected void AddEventChild(VPPXEDistEvent ev, VPPXmlNode node)
	{
		string childType = node.GetAttr("type", "");
		if (childType == "")
		{
			return;
		}

		int lootmax = node.GetAttr("lootmax", "0").ToInt();
		int deloot = -1;
		if (node.FindAttr("deloot") >= 0)
		{
			deloot = node.GetAttr("deloot", "0").ToInt();
		}

		ev.UpsertChild(VPPXEDistUtil.Lower(childType), childType, lootmax, deloot);
	}

	protected void RemoveEvent(string eventKey)
	{
		int idx;
		if (!m_Data.EventIndex.Find(eventKey, idx))
		{
			return;
		}

		VPPXEDistEvent old = m_Data.Events[idx];
		old.Removed = true;
		m_Data.EventIndex.Remove(eventKey);
	}

	protected VPPXEDistEvent FindOrAddEvent(string eventKey, string eventName)
	{
		int idx;
		if (m_Data.EventIndex.Find(eventKey, idx))
		{
			return m_Data.Events[idx];
		}

		VPPXEDistEvent ev = new VPPXEDistEvent();
		ev.Name = eventName;
		ev.Key = eventKey;
		ev.Position = "";
		idx = m_Data.Events.Insert(ev);
		m_Data.EventIndex.Insert(eventKey, idx);
		return ev;
	}

	// R12 cfgeventgroups: the first definition wins; child x/z are offsets, lootmax 0 when absent
	protected void ParseEventGroup(VPPXmlNode node, string fileKey)
	{
		string groupName = node.GetAttr("name", "");
		if (groupName == "")
		{
			return;
		}

		string groupKey = VPPXEDistUtil.Lower(groupName);
		if (m_Data.GroupIndex.Contains(groupKey))
		{
			return;
		}

		m_Data.GroupsFileKey = fileKey;
		int groupIdx = m_Data.GroupKeys.Insert(groupKey);
		m_Data.GroupNames.Insert(groupName);
		m_Data.GroupIndex.Insert(groupKey, groupIdx);
		int childStart = m_Data.GcKeys.Count();
		int distStart = m_Data.GroupDist.Count();
		map<string, bool> seenTypes = new map<string, bool>();
		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode child = node.ChildAt(i);
			if (VPPXEDistUtil.ElemName(child) != "child")
			{
				continue;
			}

			string childType = child.GetAttr("type", "");
			if (childType == "")
			{
				continue;
			}

			string childKey = VPPXEDistUtil.Lower(childType);
			int deloot = -1;
			if (child.FindAttr("deloot") >= 0)
			{
				deloot = child.GetAttr("deloot", "0").ToInt();
			}

			int childIdx = m_Data.GcKeys.Insert(childKey);
			m_Data.GcNames.Insert(childType);
			m_Data.GcX.Insert(child.GetAttr("x", "0").ToFloat());
			m_Data.GcZ.Insert(child.GetAttr("z", "0").ToFloat());
			m_Data.GcLootmax.Insert(child.GetAttr("lootmax", "0").ToInt());
			m_Data.GcDeloot.Insert(deloot);
			if (!seenTypes.Contains(childKey))
			{
				seenTypes.Insert(childKey, true);
				m_Data.GroupDist.Insert(childIdx);
			}
		}

		m_Data.GroupChildStart.Insert(childStart);
		m_Data.GroupChildCount.Insert(m_Data.GcKeys.Count() - childStart);
		m_Data.GroupDistStart.Insert(distStart);
		m_Data.GroupDistCount.Insert(m_Data.GroupDist.Count() - distStart);
	}

	// R12 cfgeventspawns: event name -> positions (x, z, group); repeated event names extend the list
	protected void ParseEventSpawn(VPPXmlNode node, string fileKey)
	{
		string eventName = node.GetAttr("name", "");
		if (eventName == "")
		{
			return;
		}

		m_Data.SpawnsFileKey = fileKey;
		string eventKey = VPPXEDistUtil.Lower(eventName);
		int spawnIdx;
		if (!m_Data.SpIndex.Find(eventKey, spawnIdx))
		{
			spawnIdx = m_Data.SpKeys.Insert(eventKey);
			m_Data.SpNames.Insert(eventName);
			m_Data.SpLine.Insert(node.StartLine + 1);
			m_Data.SpPos.Insert(new array<int>());
			m_Data.SpIndex.Insert(eventKey, spawnIdx);
		}

		array<int> posList = m_Data.SpPos[spawnIdx];
		int n = node.ChildCount();
		for (int i = 0; i < n; i++)
		{
			VPPXmlNode child = node.ChildAt(i);
			if (VPPXEDistUtil.ElemName(child) != "pos")
			{
				continue;
			}

			int groupIdx = -1;
			string groupName = child.GetAttr("group", "");
			if (groupName != "")
			{
				string groupKey = VPPXEDistUtil.Lower(groupName);
				if (!m_Data.GroupIndex.Find(groupKey, groupIdx))
				{
					groupIdx = -2;
					NoteMissingGroup(groupKey, groupName, child.StartLine + 1);
				}
			}

			int posIdx = m_Data.PosX.Insert(child.GetAttr("x", "0").ToFloat());
			m_Data.PosZ.Insert(child.GetAttr("z", "0").ToFloat());
			m_Data.PosGroup.Insert(groupIdx);
			posList.Insert(posIdx);
		}
	}

	protected void NoteMissingGroup(string groupKey, string groupName, int line)
	{
		if (m_MissingGroups.Contains(groupKey))
		{
			return;
		}

		m_MissingGroups.Insert(groupKey, line);
		m_MissingGroupNames.Insert(groupName);
		m_MissingGroupLines.Insert(line);
	}

	// resolve preset references of the final block lists (PRESET_MISSING)
	protected void ResolveUnit()
	{
		if (m_Cursor >= m_Data.Parents.Count())
		{
			m_Phase = PH_SORT_KEYS;
			m_Cursor = 0;
			return;
		}

		VPPXEDistSpawnParent parent = m_Data.Parents[m_Cursor];
		m_Cursor++;
		for (int i = 0; i < parent.Blocks.Count(); i++)
		{
			VPPXEDistSpawnBlock block = parent.Blocks[i];
			if (!block.IsPreset)
			{
				continue;
			}

			string presetKey = block.Kind + "|" + VPPXEDistUtil.Lower(block.PresetName);
			VPPXEDistPreset preset;
			if (!m_Data.Presets.Find(presetKey, preset))
			{
				m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.PRESET_MISSING, block.FileKey, block.Line, parent.Name, block.PresetName));
				continue;
			}

			preset.Used = true;
			block.ItemKeys = preset.ItemKeys;
			block.ItemNames = preset.ItemNames;
			block.ItemChances = preset.ItemChances;
			if (block.ChanceText == "")
			{
				block.ChanceText = preset.ChanceText;
			}
		}
	}

	// parents sorted by name through one native sort over "<lowercase name> <index>" keys
	protected void SortKeyUnit()
	{
		if (m_Cursor >= m_Data.Parents.Count())
		{
			m_Phase = PH_SORT;
			return;
		}

		VPPXEDistSpawnParent parent = m_Data.Parents[m_Cursor];
		m_SortKeys.Insert(parent.Key + " " + VPPXmlText.PadInt(m_Cursor, 7));
		m_Cursor++;
	}

	protected void OrderUnit()
	{
		if (m_Cursor >= m_SortKeys.Count())
		{
			m_SortKeys.Clear();
			m_Phase = PH_REVERSE;
			m_Cursor = 0;
			return;
		}

		string sortKey = m_SortKeys[m_Cursor];
		m_Cursor++;
		string tail = sortKey.Substring(sortKey.Length() - 7, 7);
		m_Data.ParentOrder.Insert(tail.ToInt());
	}

	// reverse index item -> parents (in parent name order)
	protected void ReverseUnit()
	{
		if (m_Cursor >= m_Data.ParentOrder.Count())
		{
			m_Phase = PH_ACTIVE;
			m_Cursor = 0;
			return;
		}

		int parentIdx = m_Data.ParentOrder[m_Cursor];
		m_Cursor++;
		VPPXEDistSpawnParent parent = m_Data.Parents[parentIdx];
		for (int i = 0; i < parent.Blocks.Count(); i++)
		{
			VPPXEDistSpawnBlock block = parent.Blocks[i];
			for (int j = 0; j < block.ItemKeys.Count(); j++)
			{
				string itemKey = block.ItemKeys[j];
				array<int> owners;
				if (!m_Data.ItemParents.Find(itemKey, owners))
				{
					owners = new array<int>();
					m_Data.ItemParents.Insert(itemKey, owners);
				}

				int last = owners.Count() - 1;
				if (last >= 0 && owners[last] == parentIdx)
				{
					continue;
				}

				owners.Insert(parentIdx);
			}
		}
	}

	// every event left in the merged map is active (R11), in merged order
	protected void ActiveUnit()
	{
		if (m_Cursor >= m_Data.Events.Count())
		{
			m_Phase = PH_LINT_SPAWNS;
			m_Cursor = 0;
			return;
		}

		VPPXEDistEvent ev = m_Data.Events[m_Cursor];
		m_Cursor++;
		if (ev.Removed)
		{
			return;
		}

		int spawnIdx;
		if (!m_Data.SpIndex.Find(ev.Key, spawnIdx))
		{
			spawnIdx = -1;
		}

		int activeIdx = m_Data.Active.Insert(ev);
		m_Data.ActiveSpawn.Insert(spawnIdx);
		m_Data.ActiveIndex.Insert(ev.Key, activeIdx);
	}

	// EVSPAWN_NO_EVENT: defined in no events file (inactive events count as defined)
	protected void LintSpawnsUnit()
	{
		if (m_Cursor >= m_Data.SpKeys.Count())
		{
			m_Phase = PH_LINT_GROUPS;
			m_Cursor = 0;
			return;
		}

		string spawnKey = m_Data.SpKeys[m_Cursor];
		if (!m_Data.DefinedEvents.Contains(spawnKey))
		{
			m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.EVSPAWN_NO_EVENT, m_Data.SpawnsFileKey, m_Data.SpLine[m_Cursor], "", m_Data.SpNames[m_Cursor]));
		}

		m_Cursor++;
	}

	// EVSPAWN_NO_GROUP: one issue per unknown group name
	protected void LintGroupsUnit()
	{
		if (m_Cursor >= m_MissingGroupNames.Count())
		{
			m_Phase = PH_LINT_PRESETS;
			m_Cursor = 0;
			return;
		}

		m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.EVSPAWN_NO_GROUP, m_Data.SpawnsFileKey, m_MissingGroupLines[m_Cursor], "", m_MissingGroupNames[m_Cursor]));
		m_Cursor++;
	}

	// PRESET_EMPTY and PRESET_UNUSED on the final (kind, name) definitions
	protected void LintPresetsUnit()
	{
		if (m_Cursor >= m_Data.PresetOrder.Count())
		{
			m_Phase = PH_LINT_EVENTS;
			m_Cursor = 0;
			return;
		}

		string presetKey = m_Data.PresetOrder[m_Cursor];
		m_Cursor++;
		VPPXEDistPreset preset;
		if (!m_Data.Presets.Find(presetKey, preset))
		{
			return;
		}

		if (preset.ItemKeys.Count() == 0)
		{
			m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.PRESET_EMPTY, preset.FileKey, preset.Line, "", preset.Name));
		}

		if (!preset.Used)
		{
			m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.PRESET_UNUSED, preset.FileKey, preset.Line, "", preset.Name));
		}
	}

	// EVENT_NO_POS: active, position fixed, name prefix static/vehicle/item, no positions
	protected void LintEventsUnit()
	{
		if (m_Cursor >= m_Data.Active.Count())
		{
			FinishStage();
			return;
		}

		VPPXEDistEvent ev = m_Data.Active[m_Cursor];
		int spawnIdx = m_Data.ActiveSpawn[m_Cursor];
		m_Cursor++;
		if (ev.Position != "fixed")
		{
			return;
		}

		bool prefixed = VPPXEDistUtil.StartsWith(ev.Key, "static") || VPPXEDistUtil.StartsWith(ev.Key, "vehicle") || VPPXEDistUtil.StartsWith(ev.Key, "item");
		if (!prefixed)
		{
			return;
		}

		int posCount = 0;
		if (spawnIdx >= 0)
		{
			array<int> posList = m_Data.SpPos[spawnIdx];
			posCount = posList.Count();
		}

		if (posCount > 0)
		{
			return;
		}

		m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.EVENT_NO_POS, ev.FileKey, ev.Line, "", ev.Name));
	}

	protected void FinishStage()
	{
		m_Phase = PH_DONE;
		int ms = GetGame().GetTime() - m_StartMs;
		string text = "[Dist] LIGHT built in " + ms.ToString() + " ms: " + m_Data.Events.Count().ToString() + " events (" + m_Data.Active.Count().ToString() + " active)";
		text = text + ", " + m_Data.PosX.Count().ToString() + " spawn positions, " + m_Data.GroupKeys.Count().ToString() + " event groups";
		text = text + ", " + m_Data.Parents.Count().ToString() + " spawnable types, " + m_Data.PresetOrder.Count().ToString() + " presets";
		text = text + ", " + m_Data.ZoneKeys.Count().ToString() + " infected zones, " + m_Data.Clusters.ClusterKeys.Count().ToString() + " clusters, " + m_Data.Issues.Count().ToString() + " issues";
		VPPXELog.Info(text);
		ReportStageProgress(VPPXEStage.SPAWN_LIGHT, 100);
	}
};

// ---------------------------------------------------------------------------------------------
// MAP stage job (BACKGROUND)
// ---------------------------------------------------------------------------------------------

class VPPXESpawnMapJob : VPPXEJob
{
	const static int PH_INIT = 0;
	const static int PH_PROTO_READ = 1;
	const static int PH_PROTO_SPLIT = 2;
	const static int PH_PROTO_PARSE = 3;
	const static int PH_PROTO_DISPATCH = 4;
	const static int PH_POS_OPEN = 5;
	const static int PH_POS_STREAM = 6;
	const static int PH_POINTS_BUILDINGS = 7;
	const static int PH_POINTS_EVENTS = 8;
	const static int PH_AREA = 9;
	const static int PH_DONE = 10;

	protected VPPXEDistService m_Service;
	protected ref VPPXESpawnLight m_Light;
	protected ref VPPXESpawnMap m_Data;
	protected ref VPPXELimits m_Limits;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_StartMs;
	protected string m_ProtoPath;
	protected string m_PosPath;
	protected string m_Content;
	protected int m_ProtoLineCount;
	protected ref VPPXmlSplitter m_Splitter;
	protected ref VPPXmlReader m_Reader;
	protected ref VPPXEProtoSink m_Sink;
	protected ref VPPXmlLineStream m_Stream;
	protected bool m_InComment;
	protected ref array<float> m_SX;
	protected ref array<float> m_SZ;
	protected ref VPPXEAreaFlags m_Area;
	protected bool m_FromCache;
	protected string m_AreaText;

	void VPPXESpawnMapJob(VPPXEDistService service, VPPXESpawnLight light)
	{
		m_Service = service;
		m_Light = light;
		m_Data = new VPPXESpawnMap();
		m_Phase = PH_INIT;
		m_Cursor = 0;
		m_StartMs = GetGame().GetTime();
		m_ProtoPath = "";
		m_PosPath = "";
		m_Content = "";
		m_ProtoLineCount = 0;
		m_InComment = false;
		m_SX = new array<float>();
		m_SZ = new array<float>();
		m_FromCache = false;
		m_AreaText = "";
	}

	void ~VPPXESpawnMapJob()
	{
		if (m_Stream)
		{
			m_Stream.Close();
		}
	}

	VPPXESpawnMap GetMapData()
	{
		return m_Data;
	}

	bool IsFromCache()
	{
		return m_FromCache;
	}

	override string GetLabel()
	{
		return "dist MAP";
	}

	protected void ReportStageProgress(int stage, int pct)
	{
		if (m_Service)
		{
			m_Service.ReportProgress(stage, pct);
		}
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
				BeginMap();
			}
			else if (m_Phase == PH_PROTO_READ)
			{
				ProtoReadUnit();
			}
			else if (m_Phase == PH_PROTO_SPLIT)
			{
				ProtoSplitUnit();
			}
			else if (m_Phase == PH_PROTO_PARSE)
			{
				ProtoParseUnit();
			}
			else if (m_Phase == PH_PROTO_DISPATCH)
			{
				DispatchUnit();
			}
			else if (m_Phase == PH_POS_OPEN)
			{
				PosOpenUnit();
			}
			else if (m_Phase == PH_POS_STREAM)
			{
				PosStreamUnit();
			}
			else if (m_Phase == PH_POINTS_BUILDINGS)
			{
				PointsBuildingsUnit();
			}
			else if (m_Phase == PH_POINTS_EVENTS)
			{
				PointsEventsUnit();
			}
			else if (m_Phase == PH_AREA)
			{
				AreaUnit();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnFinished()
	{
		CloseAll();
		if (m_Service)
		{
			m_Service.OnMapJobDone(this, true);
		}
	}

	override void OnAborted()
	{
		CloseAll();
		VPPXELog.Info("[Dist] MAP stage aborted with a script error");
		if (m_Service)
		{
			m_Service.OnMapJobDone(this, false);
		}
	}

	protected void CloseAll()
	{
		if (m_Stream)
		{
			m_Stream.Close();
		}

		m_Stream = null;
		m_Splitter = null;
		m_Reader = null;
		m_Sink = null;
		m_Area = null;
	}

	protected void BeginMap()
	{
		m_Phase = PH_PROTO_READ;
		m_Data.SpawnRev = m_Light.SpawnRev;
		XMLEditor editor = GetXMLEditor();
		if (!editor)
		{
			return;
		}

		if (editor.GetTypes())
		{
			m_Limits = editor.GetTypes().GetLimits();
		}

		VPPXEFileRegistry reg = editor.GetRegistry();
		if (!reg)
		{
			return;
		}

		VPPXEFileEntry protoEntry = reg.FirstOfKind(VPPXEFileKind.MAPPROTO);
		if (VPPXEDistUtil.Exists(protoEntry))
		{
			m_ProtoPath = protoEntry.Path;
			m_Data.ProtoKey = protoEntry.Key;
		}

		VPPXEFileEntry posEntry = reg.FirstOfKind(VPPXEFileKind.MAPPOS);
		if (VPPXEDistUtil.Exists(posEntry))
		{
			m_PosPath = posEntry.Path;
			m_Data.PosKey = posEntry.Key;
		}

		ReportStageProgress(VPPXEStage.PROTO, 0);
	}

	// one native read of mapgroupproto (about 1.2 MB), then a resumable split
	protected void ProtoReadUnit()
	{
		m_Phase = PH_PROTO_DISPATCH;
		m_Cursor = 0;
		if (m_ProtoPath == "")
		{
			VPPXELog.Info("[Dist] MAP: no mapgroupproto.xml; buildings hold no loot");
			return;
		}

		if (!VPPXmlText.ReadAll(m_ProtoPath, m_Content))
		{
			VPPXELog.Info("[Dist] MAP: could not read " + m_Data.ProtoKey);
			m_Content = "";
			return;
		}

		m_Splitter = new VPPXmlSplitter();
		m_Splitter.Begin(m_Content);
		m_Content = "";
		m_Phase = PH_PROTO_SPLIT;
	}

	protected void ProtoSplitUnit()
	{
		if (!m_Splitter.Step(128))
		{
			return;
		}

		array<string> lines = m_Splitter.GetLines();
		m_ProtoLineCount = lines.Count();
		m_Sink = new VPPXEProtoSink(m_Data, m_Limits);
		m_Reader = new VPPXmlReader(m_Sink);
		m_Reader.Begin(lines);
		m_Phase = PH_PROTO_PARSE;
	}

	protected void ProtoParseUnit()
	{
		if (!m_Reader.Step(64))
		{
			int total = m_ProtoLineCount;
			if (total < 1)
			{
				total = 1;
			}

			ReportStageProgress(VPPXEStage.PROTO, m_Reader.GetLine() * 100 / total);
			return;
		}

		if (m_Reader.HasError())
		{
			string errText = "[Dist] MAP: " + m_Data.ProtoKey + " stopped at line " + m_Reader.GetErrorLine().ToString() + ": " + m_Reader.GetError() + " (groups read so far are kept)";
			VPPXELog.Info(errText);
		}

		m_Sink.FinishParse();
		m_Reader = null;
		m_Sink = null;
		m_Splitter = null;
		m_Phase = PH_PROTO_DISPATCH;
		m_Cursor = 0;
		ReportStageProgress(VPPXEStage.PROTO, 100);
	}

	// reverse map dispatch proxy type -> proto groups (R18)
	protected void DispatchUnit()
	{
		if (m_Cursor >= m_Data.ProtoKeys.Count())
		{
			m_Phase = PH_POS_OPEN;
			m_Cursor = 0;
			return;
		}

		int groupIdx = m_Cursor;
		m_Cursor++;
		int start = m_Data.ProtoDispStart[groupIdx];
		int count = m_Data.ProtoDispCount[groupIdx];
		for (int i = 0; i < count; i++)
		{
			string dispKey = m_Data.DispTypes[start + i];
			array<int> groups;
			if (!m_Data.DispatchGroups.Find(dispKey, groups))
			{
				groups = new array<int>();
				m_Data.DispatchGroups.Insert(dispKey, groups);
			}

			int last = groups.Count() - 1;
			if (last >= 0 && groups[last] == groupIdx)
			{
				continue;
			}

			groups.Insert(groupIdx);
		}
	}

	protected void PosOpenUnit()
	{
		m_Phase = PH_POINTS_BUILDINGS;
		m_Cursor = 0;
		ReportStageProgress(VPPXEStage.POSITIONS, 0);
		if (m_PosPath == "")
		{
			VPPXELog.Info("[Dist] MAP: no mapgrouppos.xml; no building positions");
			return;
		}

		m_Stream = new VPPXmlLineStream();
		if (!m_Stream.Open(m_PosPath))
		{
			VPPXELog.Info("[Dist] MAP: could not open " + m_Data.PosKey);
			m_Stream = null;
			return;
		}

		m_InComment = false;
		m_Phase = PH_POS_STREAM;
	}

	// mapgrouppos is streamed line by line (never read whole)
	protected void PosStreamUnit()
	{
		string line;
		if (!m_Stream.Next(line))
		{
			m_Data.PosRev = m_Stream.RollingHash();
			m_Data.PosLines = m_Stream.LineNo();
			m_Stream.Close();
			m_Stream = null;
			m_Phase = PH_POINTS_BUILDINGS;
			m_Cursor = 0;
			ReportStageProgress(VPPXEStage.POSITIONS, 100);
			return;
		}

		ProcessPosLine(line);
		int lineNo = m_Stream.LineNo();
		if (lineNo % 512 == 0)
		{
			ReportStageProgress(VPPXEStage.POSITIONS, lineNo * 100 / (lineNo + 5000));
		}
	}

	// R6: a line with <group, name= and pos=; a line inside a comment is skipped
	protected void ProcessPosLine(string line)
	{
		if (m_InComment)
		{
			if (line.IndexOf("-->") >= 0)
			{
				m_InComment = false;
			}

			return;
		}

		int commentStart = line.IndexOf("<!--");
		if (commentStart >= 0)
		{
			if (line.IndexOfFrom(commentStart + 4, "-->") < 0)
			{
				m_InComment = true;
			}

			return;
		}

		if (line.IndexOf("<group") < 0)
		{
			return;
		}

		string groupName;
		if (!VPPXmlLineScanner.Attr(line, "name", groupName))
		{
			return;
		}

		string posValue;
		if (!VPPXmlLineScanner.Attr(line, "pos", posValue))
		{
			return;
		}

		float x;
		float z;
		if (!VPPXmlLineScanner.XZ(posValue, x, z))
		{
			return;
		}

		int protoIdx = m_Data.FindProto(VPPXEDistUtil.Lower(groupName));
		if (protoIdx < 0)
		{
			if (m_Data.PosNoProto == 0)
			{
				m_Data.PosNoProtoFirst = groupName;
				m_Data.PosNoProtoLine = m_Stream.LineNo();
			}

			m_Data.PosNoProto = m_Data.PosNoProto + 1;
		}

		m_Data.AddPosition(x, z, protoIdx);
	}

	// R7 sample points: every building position ...
	protected void PointsBuildingsUnit()
	{
		if (m_Cursor >= m_Data.PosX.Count())
		{
			m_Phase = PH_POINTS_EVENTS;
			m_Cursor = 0;
			return;
		}

		m_SX.Insert(m_Data.PosX[m_Cursor]);
		m_SZ.Insert(m_Data.PosZ[m_Cursor]);
		m_Cursor++;
	}

	// ... every eventspawns position, and every grouped position plus its children's offsets (R12 q = p + offset)
	protected void PointsEventsUnit()
	{
		if (m_Cursor >= m_Light.PosX.Count())
		{
			m_Phase = PH_AREA;
			m_Cursor = 0;
			ReportStageProgress(VPPXEStage.AREAFLAGS, 0);
			return;
		}

		float px = m_Light.PosX[m_Cursor];
		float pz = m_Light.PosZ[m_Cursor];
		int groupIdx = m_Light.PosGroup[m_Cursor];
		m_Cursor++;
		m_SX.Insert(px);
		m_SZ.Insert(pz);
		if (groupIdx < 0)
		{
			return;
		}

		int start = m_Light.GroupDistStart[groupIdx];
		int count = m_Light.GroupDistCount[groupIdx];
		for (int i = 0; i < count; i++)
		{
			int childIdx = m_Light.GroupDist[start + i];
			m_SX.Insert(px + m_Light.GcX[childIdx]);
			m_SZ.Insert(pz + m_Light.GcZ[childIdx]);
		}
	}

	protected void AreaUnit()
	{
		if (!m_Area)
		{
			bool enabled = true;
			string worldName = "";
			XMLEditor editor = GetXMLEditor();
			if (editor)
			{
				if (editor.GetSettings())
				{
					enabled = editor.GetSettings().EnableAreaFlags;
				}

				if (editor.GetRegistry())
				{
					worldName = editor.GetRegistry().GetWorldName();
				}
			}

			m_Area = new VPPXEAreaFlags(worldName, m_Data.PosRev, m_Data.SpawnRev, enabled);
			m_Area.BeginSample(m_SX, m_SZ);
			return;
		}

		if (!m_Area.Step())
		{
			ReportStageProgress(VPPXEStage.AREAFLAGS, m_Area.GetPercent());
			return;
		}

		m_Data.Samples = m_Area.GetSamples();
		m_FromCache = m_Area.IsFromCache();
		m_AreaText = m_Area.GetResultText();
		m_Area = null;
		m_SX = null;
		m_SZ = null;
		FinishStage();
	}

	protected void FinishStage()
	{
		if (m_Data.PosNoProto > 0)
		{
			m_Data.Issues.Insert(VPPXEDistUtil.NewIssue(VPPXEIssueCode.POS_NO_PROTO, m_Data.PosKey, m_Data.PosNoProtoLine, m_Data.PosNoProtoFirst, m_Data.PosNoProto.ToString()));
		}

		m_Phase = PH_DONE;
		int ms = GetGame().GetTime() - m_StartMs;
		string text = "[Dist] MAP built in " + ms.ToString() + " ms: " + m_Data.ProtoKeys.Count().ToString() + " proto groups, " + m_Data.ContCat.Count().ToString() + " containers";
		text = text + ", " + m_Data.DispatchGroups.Count().ToString() + " dispatch types, " + m_Data.PosX.Count().ToString() + " building positions (" + m_Data.PosNoProto.ToString() + " without proto)";
		text = text + "; areaflags: " + m_AreaText;
		VPPXELog.Info(text);
		ReportStageProgress(VPPXEStage.AREAFLAGS, 100);
	}
};
