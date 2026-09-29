/*
	XML Editor client model (WP5).

	Holds the session, the per-file types rows with their FileRevision, the merged effective rows and
	the rail order (lowercase name + PadInt(index, 7) keys, ONE native Sort per completed index reply),
	the conflict-safe staged edits (deltas rebuilt into wire UPDATEs against the CURRENT rows), the
	per-file BaseRevision captured by the first staged edit, the save locks and the index request
	tracking (per file key; "" = every TYPES file).
*/

//one staged edit of (FileKey, Name): scalars and category are absolute, flags and lists are deltas
class VPPXEStagedEdit
{
	string Name;
	string FileKey;
	string BaseSignature;
	bool Conflict;
	int SetMask;
	int ClearMask;
	int Nominal;
	int Min;
	int Lifetime;
	int Restock;
	int Cost;
	int QuantMin;
	int QuantMax;
	string Category;
	int FlagsOn;
	int FlagsOff;
	ref array<string> UsageAdd;
	ref array<string> UsageRemove;
	ref array<string> UsageUserAdd;
	ref array<string> UsageUserRemove;
	ref array<string> ValueAdd;
	ref array<string> ValueRemove;
	ref array<string> ValueUserAdd;
	ref array<string> ValueUserRemove;
	ref array<string> TagAdd;
	ref array<string> TagRemove;

	void VPPXEStagedEdit()
	{
		Name = "";
		FileKey = "";
		BaseSignature = "";
		Conflict = false;
		SetMask = 0;
		ClearMask = 0;
		Nominal = VPPXEConst.UNSET;
		Min = VPPXEConst.UNSET;
		Lifetime = VPPXEConst.UNSET;
		Restock = VPPXEConst.UNSET;
		Cost = VPPXEConst.UNSET;
		QuantMin = VPPXEConst.UNSET;
		QuantMax = VPPXEConst.UNSET;
		Category = "";
		FlagsOn = 0;
		FlagsOff = 0;
		UsageAdd = new array<string>();
		UsageRemove = new array<string>();
		UsageUserAdd = new array<string>();
		UsageUserRemove = new array<string>();
		ValueAdd = new array<string>();
		ValueRemove = new array<string>();
		ValueUserAdd = new array<string>();
		ValueUserRemove = new array<string>();
		TagAdd = new array<string>();
		TagRemove = new array<string>();
	}

	int GetScalar(int fieldBit)
	{
		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				return Nominal;
			case VPPXEField.MIN:
				return Min;
			case VPPXEField.LIFETIME:
				return Lifetime;
			case VPPXEField.RESTOCK:
				return Restock;
			case VPPXEField.COST:
				return Cost;
			case VPPXEField.QUANTMIN:
				return QuantMin;
			case VPPXEField.QUANTMAX:
				return QuantMax;
		}

		return VPPXEConst.UNSET;
	}

	void SetScalarValue(int fieldBit, int num)
	{
		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				Nominal = num;
				break;
			case VPPXEField.MIN:
				Min = num;
				break;
			case VPPXEField.LIFETIME:
				Lifetime = num;
				break;
			case VPPXEField.RESTOCK:
				Restock = num;
				break;
			case VPPXEField.COST:
				Cost = num;
				break;
			case VPPXEField.QUANTMIN:
				QuantMin = num;
				break;
			case VPPXEField.QUANTMAX:
				QuantMax = num;
				break;
		}
	}

	//listKind: VPPXEClientModel.LIST_USAGE (0), LIST_USAGE_USER (1), LIST_VALUE (2), LIST_VALUE_USER (3), LIST_TAG (4)
	array<string> GetAddList(int listKind)
	{
		if (listKind == 0)
		{
			return UsageAdd;
		}

		if (listKind == 1)
		{
			return UsageUserAdd;
		}

		if (listKind == 2)
		{
			return ValueAdd;
		}

		if (listKind == 3)
		{
			return ValueUserAdd;
		}

		return TagAdd;
	}

	array<string> GetRemoveList(int listKind)
	{
		if (listKind == 0)
		{
			return UsageRemove;
		}

		if (listKind == 1)
		{
			return UsageUserRemove;
		}

		if (listKind == 2)
		{
			return ValueRemove;
		}

		if (listKind == 3)
		{
			return ValueUserRemove;
		}

		return TagRemove;
	}

	bool HasListDelta(int listKind)
	{
		array<string> adds = GetAddList(listKind);
		array<string> removes = GetRemoveList(listKind);
		return adds.Count() > 0 || removes.Count() > 0;
	}

	//USAGE covers the usage and usage-user lists, VALUE likewise, TAG the tag list
	bool TouchesElement(int elementBit)
	{
		if (elementBit == VPPXEField.USAGE)
		{
			return HasListDelta(0) || HasListDelta(1);
		}

		if (elementBit == VPPXEField.VALUE)
		{
			return HasListDelta(2) || HasListDelta(3);
		}

		if (elementBit == VPPXEField.TAG)
		{
			return HasListDelta(4);
		}

		return false;
	}

	bool IsEmpty()
	{
		if (SetMask != 0 || ClearMask != 0 || FlagsOn != 0 || FlagsOff != 0)
		{
			return false;
		}

		for (int kind = 0; kind <= 4; kind++)
		{
			if (HasListDelta(kind))
			{
				return false;
			}
		}

		return true;
	}

	//number of individual changes (fields, flags, list names); shown in the delete dialog
	int CountChanges()
	{
		int total = 0;
		for (int i = 0; i < VPPXEConst.FIELD_COUNT; i++)
		{
			int bit = 1 << i;
			if ((SetMask & bit) != 0 || (ClearMask & bit) != 0)
			{
				total++;
			}
		}

		for (int j = 0; j < 6; j++)
		{
			int flagBit = 1 << j;
			if ((FlagsOn & flagBit) != 0 || (FlagsOff & flagBit) != 0)
			{
				total++;
			}
		}

		for (int kind = 0; kind <= 4; kind++)
		{
			array<string> adds = GetAddList(kind);
			array<string> removes = GetRemoveList(kind);
			total = total + adds.Count() + removes.Count();
		}

		return total;
	}
};

//chunk staging of one XE_GetTypeIndex request
class VPPXEIndexAssembly
{
	int ReqId;
	string Key;
	int ChunkCount;
	int LastActivity;
	ref map<int, ref VPPXETypeIndexChunk> Chunks;

	void VPPXEIndexAssembly(int reqId, string fileKey, int now)
	{
		ReqId = reqId;
		Key = fileKey;
		ChunkCount = -1;
		LastActivity = now;
		Chunks = new map<int, ref VPPXETypeIndexChunk>();
	}
};

//save lock of one file while a batch touching it is in flight
class VPPXEFileLock
{
	int ReqId;
	int RevAtSend;
	int Stamp;
	bool AwaitIndex;

	void VPPXEFileLock(int reqId, int revAtSend, int now)
	{
		ReqId = reqId;
		RevAtSend = revAtSend;
		Stamp = now;
		AwaitIndex = false;
	}
};

//one logical upload (one ReqId, one batch file) split into EDITS_PER_PART parts
class VPPXEUploadBatch
{
	int ReqId;
	string FileKey;
	string TargetFileKey;
	int EditCount;
	ref array<ref VPPXEEditBatch> Parts;

	void VPPXEUploadBatch()
	{
		ReqId = 0;
		FileKey = "";
		TargetFileKey = "";
		EditCount = 0;
		Parts = new array<ref VPPXEEditBatch>();
	}
};

//parsed rail search: plain words (name substrings), f: texts and limits conditions (all must match)
class VPPXESearchSpec
{
	ref array<string> Words;
	ref array<string> FileTexts;
	ref array<int> CondKinds;
	ref array<int> CondMasks;

	void VPPXESearchSpec()
	{
		Words = new array<string>();
		FileTexts = new array<string>();
		CondKinds = new array<int>();
		CondMasks = new array<int>();
	}

	void AddCond(int kind, int mask)
	{
		CondKinds.Insert(kind);
		CondMasks.Insert(mask);
	}
};

class VPPXEClientModel
{
	const static int LIST_USAGE = 0;
	const static int LIST_USAGE_USER = 1;
	const static int LIST_VALUE = 2;
	const static int LIST_VALUE_USER = 3;
	const static int LIST_TAG = 4;

	const static int FILTER_ALL = 0;
	const static int FILTER_ISSUES = 1;
	const static int FILTER_OVERRIDDEN = 2;
	const static int FILTER_UNSAVED = 3;
	const static int FILTER_CONFLICTS = 4;
	const static int FILTER_SPAWNING = 5;
	const static int FILTER_NOSPAWN = 6;
	const static int FILTER_NOCAT = 7;
	const static int FILTER_CATEGORY = 8;
	const static int FILTER_USAGE = 9;
	const static int FILTER_VALUE = 10;
	const static int FILTER_TAG = 11;
	const static int FILTER_UNREGISTERED = 12;

	const static int COND_CAT = 1;
	const static int COND_USAGE = 2;
	const static int COND_USAGE_GROUP = 3;
	const static int COND_VALUE = 4;
	const static int COND_VALUE_GROUP = 5;
	const static int COND_TAG = 6;

	const static int STAGED_PENDING = 1;
	const static int STAGED_CONFLICT = 2;

	const static int INDEX_TIMEOUT_MS = 30000;

	protected MenuXMLEditor m_Owner;
	protected ref VPPXESessionInfo m_Session;
	protected ref VPPXELimits m_EmptyLimits;
	protected ref map<string, int> m_FileIdx;
	protected ref array<string> m_TypesFiles;
	protected ref map<string, ref array<ref VPPXETypeRow>> m_Rows;
	protected ref map<string, int> m_RowsRev;
	protected ref map<int, ref VPPXEIndexAssembly> m_Assemblies;
	protected ref map<string, int> m_PendingReq;
	protected ref map<string, int> m_PendingRev;
	protected ref array<string> m_NameList;
	protected ref map<string, ref array<VPPXETypeRow>> m_NameDefs;
	protected ref map<string, ref array<string>> m_NameDefFiles;
	protected ref map<string, ref VPPXETypeRow> m_Merged;
	protected ref map<string, VPPXETypeRow> m_FileDefRow;
	protected ref map<string, int> m_FileNameCount;
	protected ref array<string> m_Sorted;
	protected ref map<string, ref VPPXEStagedEdit> m_Staged;
	protected ref map<string, int> m_StagedCount;
	protected ref map<string, int> m_BaseRev;
	protected ref map<string, bool> m_RebasePending;
	protected ref map<string, bool> m_BulkFiles;
	protected ref map<string, ref VPPXEFileLock> m_Locks;
	protected ref map<int, string> m_SaveReqs;
	protected ref map<string, int> m_NotifiedRev;
	protected ref map<string, int> m_LastOrphansByFile;
	protected string m_LastTooManyFile;
	protected int m_LastTooManyCount;
	protected int m_IndexVersion;
	protected int m_UnregTotal;

	void VPPXEClientModel(MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_EmptyLimits = new VPPXELimits();
		m_FileIdx = new map<string, int>();
		m_TypesFiles = new array<string>();
		m_Rows = new map<string, ref array<ref VPPXETypeRow>>();
		m_RowsRev = new map<string, int>();
		m_Assemblies = new map<int, ref VPPXEIndexAssembly>();
		m_PendingReq = new map<string, int>();
		m_PendingRev = new map<string, int>();
		m_NameList = new array<string>();
		m_NameDefs = new map<string, ref array<VPPXETypeRow>>();
		m_NameDefFiles = new map<string, ref array<string>>();
		m_Merged = new map<string, ref VPPXETypeRow>();
		m_FileDefRow = new map<string, VPPXETypeRow>();
		m_FileNameCount = new map<string, int>();
		m_Sorted = new array<string>();
		m_Staged = new map<string, ref VPPXEStagedEdit>();
		m_StagedCount = new map<string, int>();
		m_BaseRev = new map<string, int>();
		m_RebasePending = new map<string, bool>();
		m_BulkFiles = new map<string, bool>();
		m_Locks = new map<string, ref VPPXEFileLock>();
		m_SaveReqs = new map<int, string>();
		m_NotifiedRev = new map<string, int>();
		m_LastOrphansByFile = new map<string, int>();
		m_LastTooManyFile = "";
		m_LastTooManyCount = 0;
		m_IndexVersion = 0;
		m_UnregTotal = 0;
	}

	//---------------------------------------------------------------- helpers

	static string LowerOf(string text)
	{
		string lower = text;
		lower.ToLower();
		return lower;
	}

	static int FindName(array<string> names, string itemName)
	{
		if (!names)
		{
			return -1;
		}

		string wanted = LowerOf(itemName);
		for (int i = 0; i < names.Count(); i++)
		{
			string candidate = LowerOf(names[i]);
			if (candidate == wanted)
			{
				return i;
			}
		}

		return -1;
	}

	static void RemoveName(array<string> names, string itemName)
	{
		int idx = FindName(names, itemName);
		while (idx >= 0)
		{
			names.RemoveOrdered(idx);
			idx = FindName(names, itemName);
		}
	}

	static int ElementOfList(int listKind)
	{
		if (listKind == LIST_USAGE || listKind == LIST_USAGE_USER)
		{
			return VPPXEField.USAGE;
		}

		if (listKind == LIST_VALUE || listKind == LIST_VALUE_USER)
		{
			return VPPXEField.VALUE;
		}

		return VPPXEField.TAG;
	}

	static void SetEditScalar(VPPXETypeEdit edit, int fieldBit, int num)
	{
		switch (fieldBit)
		{
			case VPPXEField.NOMINAL:
				edit.Nominal = num;
				break;
			case VPPXEField.MIN:
				edit.Min = num;
				break;
			case VPPXEField.LIFETIME:
				edit.Lifetime = num;
				break;
			case VPPXEField.RESTOCK:
				edit.Restock = num;
				break;
			case VPPXEField.COST:
				edit.Cost = num;
				break;
			case VPPXEField.QUANTMIN:
				edit.QuantMin = num;
				break;
			case VPPXEField.QUANTMAX:
				edit.QuantMax = num;
				break;
		}
	}

	static int ScalarBits()
	{
		return VPPXEField.NOMINAL | VPPXEField.MIN | VPPXEField.LIFETIME | VPPXEField.RESTOCK | VPPXEField.COST | VPPXEField.QUANTMIN | VPPXEField.QUANTMAX;
	}

	protected int Now()
	{
		return GetGame().GetTime();
	}

	//true when the def defines the list element for the engine (resolved mask != 0; see VPPXETypeMerge.MergeInto)
	bool DefinesElement(VPPXETypeRow row, int elementBit)
	{
		if (!row)
		{
			return false;
		}

		VPPXELimits lim = GetLimits();
		if (elementBit == VPPXEField.USAGE)
		{
			return lim.EffectiveUsage(row.Usage, row.UsageUser) != 0;
		}

		if (elementBit == VPPXEField.VALUE)
		{
			return lim.EffectiveValue(row.Value, row.ValueUser) != 0;
		}

		if (elementBit == VPPXEField.TAG)
		{
			return row.Tag != 0;
		}

		return (row.Present & elementBit) != 0;
	}

	//---------------------------------------------------------------- session and files

	void SetSession(VPPXESessionInfo info)
	{
		if (!info)
		{
			return;
		}

		string oldOrder = JoinKeys(m_TypesFiles);
		m_Session = info;
		m_FileIdx.Clear();
		m_TypesFiles.Clear();
		if (info.Files)
		{
			for (int i = 0; i < info.Files.Count(); i++)
			{
				VPPXEFileInfo fileInfo = info.Files[i];
				if (!fileInfo)
				{
					continue;
				}

				m_FileIdx.Set(fileInfo.Key, i);
				if (fileInfo.Kind == VPPXEFileKind.TYPES)
				{
					m_TypesFiles.Insert(fileInfo.Key);
				}
			}
		}

		array<string> dropKeys = new array<string>();
		for (int j = 0; j < m_Rows.Count(); j++)
		{
			string rowsKey = m_Rows.GetKey(j);
			if (m_TypesFiles.Find(rowsKey) < 0)
			{
				dropKeys.Insert(rowsKey);
			}
		}

		for (int k = 0; k < dropKeys.Count(); k++)
		{
			m_Rows.Remove(dropKeys[k]);
			m_RowsRev.Remove(dropKeys[k]);
		}

		string newOrder = JoinKeys(m_TypesFiles);
		if (dropKeys.Count() > 0 || newOrder != oldOrder)
		{
			RebuildIndex();
		}
	}

	protected string JoinKeys(array<string> keys)
	{
		string joined = "";
		for (int i = 0; i < keys.Count(); i++)
		{
			joined = joined + keys[i] + "|";
		}

		return joined;
	}

	VPPXESessionInfo GetSession()
	{
		return m_Session;
	}

	VPPXELimits GetLimits()
	{
		if (m_Session && m_Session.Limits)
		{
			return m_Session.Limits;
		}

		return m_EmptyLimits;
	}

	int GetFileCount()
	{
		if (!m_Session || !m_Session.Files)
		{
			return 0;
		}

		return m_Session.Files.Count();
	}

	VPPXEFileInfo GetFileByIdx(int idx)
	{
		if (!m_Session || !m_Session.Files || idx < 0 || idx >= m_Session.Files.Count())
		{
			return null;
		}

		return m_Session.Files[idx];
	}

	VPPXEFileInfo GetFile(string key)
	{
		int idx = FileIdxOf(key);
		return GetFileByIdx(idx);
	}

	int FileIdxOf(string key)
	{
		int idx;
		if (m_FileIdx.Find(key, idx))
		{
			return idx;
		}

		return -1;
	}

	string FileLabel(string key)
	{
		VPPXEFileInfo fileInfo = GetFile(key);
		if (fileInfo && fileInfo.Label != "")
		{
			return fileInfo.Label;
		}

		return key;
	}

	float GetWorldSize()
	{
		if (m_Session && m_Session.WorldSize > 0)
		{
			return m_Session.WorldSize;
		}

		if (GetGame() && GetGame().GetWorld())
		{
			return GetGame().GetWorld().GetWorldSize();
		}

		return 15360;
	}

	array<string> GetTypesFiles()
	{
		return m_TypesFiles;
	}

	bool IsTypesFile(string key)
	{
		return m_TypesFiles.Find(key) >= 0;
	}

	bool HasFileFlag(string key, int flag)
	{
		VPPXEFileInfo fileInfo = GetFile(key);
		if (!fileInfo)
		{
			return false;
		}

		return (fileInfo.Flags & flag) != 0;
	}

	bool IsEditable(string key)
	{
		return HasFileFlag(key, VPPXEFileFlag.EDITABLE);
	}

	//the first EDITABLE TYPES file in load order ("" when none)
	string FirstEditableTypesFile()
	{
		for (int i = 0; i < m_TypesFiles.Count(); i++)
		{
			if (IsEditable(m_TypesFiles[i]))
			{
				return m_TypesFiles[i];
			}
		}

		return "";
	}

	int GetSessionRevision(string key)
	{
		VPPXEFileInfo fileInfo = GetFile(key);
		if (!fileInfo)
		{
			return 0;
		}

		return fileInfo.Revision;
	}

	bool HasRows(string key)
	{
		return m_RowsRev.Contains(key);
	}

	int GetRowsRevision(string key)
	{
		int rev;
		if (m_RowsRev.Find(key, rev))
		{
			return rev;
		}

		return 0;
	}

	//TYPES files whose rows are missing or older than the session revision (a request already pending for the
	//same session revision is not repeated)
	void CollectIndexNeeds(array<string> outKeys)
	{
		for (int i = 0; i < m_TypesFiles.Count(); i++)
		{
			string key = m_TypesFiles[i];
			int sessionRev = GetSessionRevision(key);
			if (HasRows(key) && GetRowsRevision(key) == sessionRev)
			{
				continue;
			}

			if (m_PendingReq.Contains(key) && m_PendingRev.Get(key) == sessionRev)
			{
				continue;
			}

			outKeys.Insert(key);
		}
	}

	//true once per (file, revision): used for the NOTIFY_OTHER_ADMIN toast
	bool TakeOtherAdminNotice(string key, int revision)
	{
		int last;
		if (m_NotifiedRev.Find(key, last) && last == revision)
		{
			return false;
		}

		m_NotifiedRev.Set(key, revision);
		return true;
	}

	//---------------------------------------------------------------- index requests and chunks

	void RequestIndex(string fileKey)
	{
		if (!m_Owner)
		{
			return;
		}

		int reqId = m_Owner.NextReqId();
		int oldReq;
		if (m_PendingReq.Find(fileKey, oldReq))
		{
			m_Assemblies.Remove(oldReq);
		}

		m_PendingReq.Set(fileKey, reqId);
		m_PendingRev.Set(fileKey, GetSessionRevision(fileKey));
		m_Assemblies.Set(reqId, new VPPXEIndexAssembly(reqId, fileKey, Now()));
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetTypeIndex", new Param2<int, string>(reqId, fileKey), true, null);
	}

	bool OwnsIndexReq(int reqId)
	{
		return m_Assemblies.Contains(reqId);
	}

	bool HasPendingIndex()
	{
		return m_Assemblies.Count() > 0;
	}

	void TouchIndexReq(int reqId)
	{
		VPPXEIndexAssembly pend = m_Assemblies.Get(reqId);
		if (pend)
		{
			pend.LastActivity = Now();
		}
	}

	//a TYPES-stage progress means the server is still building the index these requests wait for
	void TouchAllIndexReqs()
	{
		int now = Now();
		for (int i = 0; i < m_Assemblies.Count(); i++)
		{
			VPPXEIndexAssembly pend = m_Assemblies.GetElement(i);
			if (pend)
			{
				pend.LastActivity = now;
			}
		}
	}

	void DropIndexReq(int reqId)
	{
		VPPXEIndexAssembly pend = m_Assemblies.Get(reqId);
		if (!pend)
		{
			return;
		}

		int pendingReq;
		if (m_PendingReq.Find(pend.Key, pendingReq) && pendingReq == reqId)
		{
			m_PendingReq.Remove(pend.Key);
			m_PendingRev.Remove(pend.Key);
		}

		m_Assemblies.Remove(reqId);
	}

	//index requests without any chunk or progress for INDEX_TIMEOUT_MS are dropped; their ids and file keys are
	//returned (outKeys may be null) so the window can re-request each key (bounded retries) or report it
	void CollectExpiredIndexReqs(array<int> outReqIds, array<string> outKeys)
	{
		int now = Now();
		for (int i = 0; i < m_Assemblies.Count(); i++)
		{
			VPPXEIndexAssembly pend = m_Assemblies.GetElement(i);
			if (pend && now - pend.LastActivity > INDEX_TIMEOUT_MS)
			{
				outReqIds.Insert(pend.ReqId);
				if (outKeys)
				{
					outKeys.Insert(pend.Key);
				}
			}
		}

		for (int j = 0; j < outReqIds.Count(); j++)
		{
			DropIndexReq(outReqIds[j]);
		}
	}

	//stores the chunk; when the reply is complete, commits its rows per file (FileRevision included),
	//rebuilds the merged rows and the rail order once and returns true with the completed files
	bool AddIndexChunk(VPPXETypeIndexChunk chunk, array<string> outDoneFiles, array<int> outDoneRevs)
	{
		if (!chunk)
		{
			return false;
		}

		VPPXEIndexAssembly pend = m_Assemblies.Get(chunk.ReqId);
		if (!pend)
		{
			return false;
		}

		pend.LastActivity = Now();
		if (pend.ChunkCount < 0)
		{
			pend.ChunkCount = chunk.ChunkCount;
		}

		if (!pend.Chunks.Contains(chunk.ChunkIdx))
		{
			pend.Chunks.Set(chunk.ChunkIdx, chunk);
		}

		if (pend.Chunks.Count() < pend.ChunkCount)
		{
			return false;
		}

		CommitAssembly(pend, outDoneFiles, outDoneRevs);
		DropIndexReq(pend.ReqId);
		RebuildIndex();
		return true;
	}

	protected void CommitAssembly(VPPXEIndexAssembly pend, array<string> outDoneFiles, array<int> outDoneRevs)
	{
		map<string, ref array<ref VPPXETypeRow>> fresh = new map<string, ref array<ref VPPXETypeRow>>();
		for (int ci = 0; ci < pend.ChunkCount; ci++)
		{
			VPPXETypeIndexChunk part = pend.Chunks.Get(ci);
			if (!part)
			{
				continue;
			}

			array<ref VPPXETypeRow> rows = fresh.Get(part.FileKey);
			if (!rows)
			{
				rows = new array<ref VPPXETypeRow>();
				fresh.Set(part.FileKey, rows);
				outDoneFiles.Insert(part.FileKey);
				outDoneRevs.Insert(part.FileRevision);
			}

			if (!part.Rows)
			{
				continue;
			}

			for (int ri = 0; ri < part.Rows.Count(); ri++)
			{
				VPPXETypeRow row = part.Rows[ri];
				if (row)
				{
					rows.Insert(row);
				}
			}
		}

		for (int fi = 0; fi < outDoneFiles.Count(); fi++)
		{
			string key = outDoneFiles[fi];
			m_Rows.Set(key, fresh.Get(key));
			m_RowsRev.Set(key, outDoneRevs[fi]);
		}
	}

	protected void RebuildIndex()
	{
		m_NameList.Clear();
		m_NameDefs.Clear();
		m_NameDefFiles.Clear();
		m_Merged.Clear();
		m_FileDefRow.Clear();
		m_FileNameCount.Clear();
		m_Sorted.Clear();
		VPPXELimits lim = GetLimits();
		for (int fi = 0; fi < m_TypesFiles.Count(); fi++)
		{
			string key = m_TypesFiles[fi];
			array<ref VPPXETypeRow> rows = m_Rows.Get(key);
			if (!rows)
			{
				continue;
			}

			for (int ri = 0; ri < rows.Count(); ri++)
			{
				VPPXETypeRow row = rows[ri];
				if (!row || row.Name == "")
				{
					continue;
				}

				string lower = LowerOf(row.Name);
				array<VPPXETypeRow> defs = m_NameDefs.Get(lower);
				array<string> defFiles = m_NameDefFiles.Get(lower);
				if (!defs)
				{
					defs = new array<VPPXETypeRow>();
					defFiles = new array<string>();
					m_NameDefs.Set(lower, defs);
					m_NameDefFiles.Set(lower, defFiles);
					m_NameList.Insert(lower);
				}

				defs.Insert(row);
				defFiles.Insert(key);
				string pairKey = key + "|" + lower;
				if (!m_FileDefRow.Contains(pairKey))
				{
					m_FileNameCount.Set(key, m_FileNameCount.Get(key) + 1);
				}

				m_FileDefRow.Set(pairKey, row);
			}
		}

		array<string> sortKeys = new array<string>();
		for (int ni = 0; ni < m_NameList.Count(); ni++)
		{
			string nameLower = m_NameList[ni];
			array<VPPXETypeRow> nameDefs = m_NameDefs.Get(nameLower);
			VPPXETypeRow firstDef = nameDefs[0];
			VPPXETypeRow acc = VPPXETypeMerge.NewRow(firstDef.Name);
			for (int di = 0; di < nameDefs.Count(); di++)
			{
				VPPXETypeMerge.MergeInto(acc, nameDefs[di], lim);
			}

			m_Merged.Set(nameLower, acc);
			sortKeys.Insert(nameLower + " " + VPPXmlText.PadInt(ni, 7));
		}

		sortKeys.Sort();
		for (int ki = 0; ki < sortKeys.Count(); ki++)
		{
			string sortKey = sortKeys[ki];
			string tail = sortKey.Substring(sortKey.Length() - 7, 7);
			int idx = tail.ToInt();
			m_Sorted.Insert(m_NameList[idx]);
		}

		m_IndexVersion++;
	}

	int GetIndexVersion()
	{
		return m_IndexVersion;
	}

	array<string> GetSortedNames()
	{
		return m_Sorted;
	}

	int GetNameCount()
	{
		return m_NameList.Count();
	}

	//distinct names defined in one file (0 before its rows arrived)
	int GetFileNameCount(string fileKey)
	{
		return m_FileNameCount.Get(fileKey);
	}

	//---------------------------------------------------------------- lookups

	VPPXETypeRow GetEffectiveRow(string typeName)
	{
		return m_Merged.Get(LowerOf(typeName));
	}

	VPPXETypeRow GetDefRow(string fileKey, string typeName)
	{
		return m_FileDefRow.Get(fileKey + "|" + LowerOf(typeName));
	}

	bool HasName(string typeName)
	{
		return m_Merged.Contains(LowerOf(typeName));
	}

	bool DefExists(string fileKey, string typeName)
	{
		return m_FileDefRow.Contains(fileKey + "|" + LowerOf(typeName));
	}

	string DisplayNameOf(string typeName)
	{
		VPPXETypeRow eff = m_Merged.Get(LowerOf(typeName));
		if (eff)
		{
			return eff.Name;
		}

		return typeName;
	}

	//distinct TYPES file keys that define the name, in load order
	void GetDefFiles(string typeName, array<string> outKeys)
	{
		array<string> defFiles = m_NameDefFiles.Get(LowerOf(typeName));
		if (!defFiles)
		{
			return;
		}

		for (int i = 0; i < defFiles.Count(); i++)
		{
			if (outKeys.Find(defFiles[i]) < 0)
			{
				outKeys.Insert(defFiles[i]);
			}
		}
	}

	//distinct file keys defining the name, excluding up to two keys
	void OtherDefFiles(string typeName, string excludeKey, string excludeKey2, array<string> outKeys)
	{
		array<string> defFiles = new array<string>();
		GetDefFiles(typeName, defFiles);
		for (int i = 0; i < defFiles.Count(); i++)
		{
			string key = defFiles[i];
			if (key == excludeKey || key == excludeKey2)
			{
				continue;
			}

			outKeys.Insert(key);
		}
	}

	//the file of the last def in load order ("" when the name has no def)
	string LastDefFile(string typeName)
	{
		array<string> defFiles = m_NameDefFiles.Get(LowerOf(typeName));
		if (!defFiles || defFiles.Count() == 0)
		{
			return "";
		}

		return defFiles[defFiles.Count() - 1];
	}

	//merge of the defs of the name in files BEFORE fileKey in load order (what that def inherits)
	VPPXETypeRow PriorMerge(string typeName, string fileKey)
	{
		string lower = LowerOf(typeName);
		VPPXETypeRow acc = VPPXETypeMerge.NewRow(typeName);
		array<VPPXETypeRow> defs = m_NameDefs.Get(lower);
		array<string> defFiles = m_NameDefFiles.Get(lower);
		if (!defs || !defFiles)
		{
			return acc;
		}

		int targetOrder = FileIdxOf(fileKey);
		if (targetOrder < 0)
		{
			return acc;
		}

		VPPXELimits lim = GetLimits();
		for (int i = 0; i < defs.Count(); i++)
		{
			int order = FileIdxOf(defFiles[i]);
			if (order < 0 || order >= targetOrder)
			{
				continue;
			}

			VPPXETypeMerge.MergeInto(acc, defs[i], lim);
		}

		return acc;
	}

	//the file of the last def BEFORE fileKey that defines the list element ("" when none)
	string PriorElementFile(string typeName, string fileKey, int elementBit)
	{
		string lower = LowerOf(typeName);
		array<VPPXETypeRow> defs = m_NameDefs.Get(lower);
		array<string> defFiles = m_NameDefFiles.Get(lower);
		string found = "";
		if (!defs || !defFiles)
		{
			return found;
		}

		int targetOrder = FileIdxOf(fileKey);
		for (int i = 0; i < defs.Count(); i++)
		{
			int order = FileIdxOf(defFiles[i]);
			if (order < 0 || order >= targetOrder)
			{
				continue;
			}

			if (DefinesElement(defs[i], elementBit))
			{
				found = defFiles[i];
			}
		}

		return found;
	}

	//def row of (fileKey, name) with its staged edit applied (null when the def does not exist)
	VPPXETypeRow GetDisplayRow(string typeName, string fileKey)
	{
		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		if (!defRow)
		{
			return null;
		}

		VPPXETypeRow shown = VPPXETypeMerge.Copy(defRow);
		VPPXEStagedEdit staged = GetStaged(fileKey, typeName);
		if (staged)
		{
			VPPXETypeEdit edit = BuildEdit(staged);
			if (edit)
			{
				VPPXETypeMerge.ApplyEdit(shown, edit, GetLimits());
			}
		}

		return shown;
	}

	//scalar with staged edits applied; scopeFile "" = effective across every def, else that file's def.
	//Returns false when no def sets it; winnerFile = the file that supplies the value.
	bool DisplayScalar(string typeName, string scopeFile, int fieldBit, out int num, out string winnerFile)
	{
		string lower = LowerOf(typeName);
		array<VPPXETypeRow> defs = m_NameDefs.Get(lower);
		array<string> defFiles = m_NameDefFiles.Get(lower);
		bool found = false;
		num = VPPXEConst.UNSET;
		winnerFile = "";
		if (!defs || !defFiles)
		{
			return false;
		}

		int total = defs.Count();
		for (int i = 0; i < total; i++)
		{
			string key = defFiles[i];
			if (scopeFile != "" && key != scopeFile)
			{
				continue;
			}

			VPPXETypeRow defRow = defs[i];
			if (!defRow)
			{
				continue;
			}

			bool present = (defRow.Present & fieldBit) != 0;
			int current = VPPXETypeMerge.GetScalar(defRow, fieldBit);
			bool lastOfFile = true;
			if (i + 1 < total)
			{
				if (defFiles[i + 1] == key)
				{
					lastOfFile = false;
				}
			}

			if (lastOfFile)
			{
				VPPXEStagedEdit staged = m_Staged.Get(key + "|" + lower);
				if (staged)
				{
					if ((staged.SetMask & fieldBit) != 0)
					{
						present = true;
						current = staged.GetScalar(fieldBit);
					}
					else if ((staged.ClearMask & fieldBit) != 0)
					{
						present = false;
					}
				}
			}

			if (present)
			{
				found = true;
				num = current;
				winnerFile = key;
			}
		}

		return found;
	}

	int DistinctDefFileCount(string lower)
	{
		array<string> defFiles = m_NameDefFiles.Get(lower);
		if (!defFiles)
		{
			return 0;
		}

		int distinct = 0;
		string previous = "";
		for (int i = 0; i < defFiles.Count(); i++)
		{
			if (i == 0 || defFiles[i] != previous)
			{
				distinct++;
			}

			previous = defFiles[i];
		}

		return distinct;
	}

	//---------------------------------------------------------------- staging

	VPPXEStagedEdit GetStaged(string fileKey, string typeName)
	{
		return m_Staged.Get(fileKey + "|" + LowerOf(typeName));
	}

	protected VPPXEStagedEdit EnsureStaged(string fileKey, string typeName)
	{
		string key = fileKey + "|" + LowerOf(typeName);
		VPPXEStagedEdit staged = m_Staged.Get(key);
		if (staged)
		{
			return staged;
		}

		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		if (!defRow)
		{
			return null;
		}

		if (m_StagedCount.Get(fileKey) <= 0)
		{
			m_BaseRev.Set(fileKey, GetRowsRevision(fileKey));
		}

		staged = new VPPXEStagedEdit();
		staged.Name = defRow.Name;
		staged.FileKey = fileKey;
		staged.BaseSignature = VPPXETypeMerge.Signature(defRow);
		m_Staged.Set(key, staged);
		m_StagedCount.Set(fileKey, m_StagedCount.Get(fileKey) + 1);
		return staged;
	}

	protected void RemoveStagedKey(string key)
	{
		VPPXEStagedEdit staged = m_Staged.Get(key);
		if (!staged)
		{
			return;
		}

		string fileKey = staged.FileKey;
		m_Staged.Remove(key);
		int left = m_StagedCount.Get(fileKey) - 1;
		if (left <= 0)
		{
			m_StagedCount.Remove(fileKey);
			m_BaseRev.Remove(fileKey);
			m_BulkFiles.Remove(fileKey);
		}
		else
		{
			m_StagedCount.Set(fileKey, left);
		}
	}

	//editing a type clears its conflict; an edit without any delta is dropped
	protected void AfterChange(VPPXEStagedEdit staged)
	{
		staged.Conflict = false;
		if (staged.IsEmpty())
		{
			RemoveStagedKey(staged.FileKey + "|" + LowerOf(staged.Name));
		}
	}

	void StageScalar(string fileKey, string typeName, int fieldBit, int num)
	{
		VPPXEStagedEdit staged = EnsureStaged(fileKey, typeName);
		if (!staged)
		{
			return;
		}

		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		staged.ClearMask = staged.ClearMask & ~fieldBit;
		if ((defRow.Present & fieldBit) != 0 && VPPXETypeMerge.GetScalar(defRow, fieldBit) == num)
		{
			staged.SetMask = staged.SetMask & ~fieldBit;
		}
		else
		{
			staged.SetMask = staged.SetMask | fieldBit;
			staged.SetScalarValue(fieldBit, num);
		}

		AfterChange(staged);
	}

	void StageClearScalar(string fileKey, string typeName, int fieldBit)
	{
		VPPXEStagedEdit staged = EnsureStaged(fileKey, typeName);
		if (!staged)
		{
			return;
		}

		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		staged.SetMask = staged.SetMask & ~fieldBit;
		if ((defRow.Present & fieldBit) != 0)
		{
			staged.ClearMask = staged.ClearMask | fieldBit;
		}
		else
		{
			staged.ClearMask = staged.ClearMask & ~fieldBit;
		}

		AfterChange(staged);
	}

	//the flags the edit is based on: the def's own flags, else what it inherits
	int BaseFlags(string fileKey, string typeName)
	{
		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		if (defRow && (defRow.Present & VPPXEField.FLAGS) != 0)
		{
			return defRow.Flags;
		}

		VPPXETypeRow prior = PriorMerge(typeName, fileKey);
		if ((prior.Present & VPPXEField.FLAGS) != 0)
		{
			return prior.Flags;
		}

		return 0;
	}

	void StageFlag(string fileKey, string typeName, int flagBit, bool on)
	{
		VPPXEStagedEdit staged = EnsureStaged(fileKey, typeName);
		if (!staged)
		{
			return;
		}

		int baseFlags = BaseFlags(fileKey, typeName);
		bool baseOn = (baseFlags & flagBit) != 0;
		staged.FlagsOn = staged.FlagsOn & ~flagBit;
		staged.FlagsOff = staged.FlagsOff & ~flagBit;
		if (on != baseOn)
		{
			if (on)
			{
				staged.FlagsOn = staged.FlagsOn | flagBit;
			}
			else
			{
				staged.FlagsOff = staged.FlagsOff | flagBit;
			}
		}

		AfterChange(staged);
	}

	//categoryName "" = no category
	void StageCategory(string fileKey, string typeName, string categoryName)
	{
		VPPXEStagedEdit staged = EnsureStaged(fileKey, typeName);
		if (!staged)
		{
			return;
		}

		VPPXETypeRow defRow = GetDefRow(fileKey, typeName);
		VPPXELimits lim = GetLimits();
		bool defHas = (defRow.Present & VPPXEField.CATEGORY) != 0;
		string current = "";
		if (defHas && defRow.Category >= 0 && defRow.Category < lim.Categories.Count())
		{
			current = lim.Categories[defRow.Category];
		}

		staged.SetMask = staged.SetMask & ~VPPXEField.CATEGORY;
		staged.ClearMask = staged.ClearMask & ~VPPXEField.CATEGORY;
		staged.Category = "";
		if (categoryName == "")
		{
			if (defHas)
			{
				staged.ClearMask = staged.ClearMask | VPPXEField.CATEGORY;
			}
		}
		else
		{
			string wanted = LowerOf(categoryName);
			string have = LowerOf(current);
			if (!defHas || wanted != have)
			{
				staged.SetMask = staged.SetMask | VPPXEField.CATEGORY;
				staged.Category = categoryName;
			}
		}

		AfterChange(staged);
	}

	//names of one list the edit is based on: the def's own list when it defines the element, else the inherited one
	void BaseListNames(string fileKey, string typeName, int listKind, array<string> outNames)
	{
		VPPXETypeRow src = GetDefRow(fileKey, typeName);
		int elementBit = ElementOfList(listKind);
		if (!DefinesElement(src, elementBit))
		{
			src = PriorMerge(typeName, fileKey);
		}

		if (!src)
		{
			return;
		}

		VPPXELimits lim = GetLimits();
		if (listKind == LIST_USAGE)
		{
			lim.NamesOf(lim.Usages, src.Usage, outNames);
		}
		else if (listKind == LIST_USAGE_USER)
		{
			lim.NamesOf(lim.UsageGroups, src.UsageUser, outNames);
		}
		else if (listKind == LIST_VALUE)
		{
			lim.NamesOf(lim.Values, src.Value, outNames);
		}
		else if (listKind == LIST_VALUE_USER)
		{
			lim.NamesOf(lim.ValueGroups, src.ValueUser, outNames);
		}
		else
		{
			lim.NamesOf(lim.Tags, src.Tag, outNames);
		}
	}

	void StageListName(string fileKey, string typeName, int listKind, string itemName, bool on)
	{
		VPPXEStagedEdit staged = EnsureStaged(fileKey, typeName);
		if (!staged)
		{
			return;
		}

		array<string> baseNames = new array<string>();
		BaseListNames(fileKey, typeName, listKind, baseNames);
		bool inBase = FindName(baseNames, itemName) >= 0;
		array<string> adds = staged.GetAddList(listKind);
		array<string> removes = staged.GetRemoveList(listKind);
		RemoveName(adds, itemName);
		RemoveName(removes, itemName);
		if (on && !inBase)
		{
			adds.Insert(itemName);
		}

		if (!on && inBase)
		{
			removes.Insert(itemName);
		}

		AfterChange(staged);
	}

	void RevertType(string fileKey, string typeName)
	{
		RemoveStagedKey(fileKey + "|" + LowerOf(typeName));
	}

	void RevertAll()
	{
		m_Staged.Clear();
		m_StagedCount.Clear();
		m_BaseRev.Clear();
		m_BulkFiles.Clear();
		m_RebasePending.Clear();
	}

	void ClearFile(string fileKey)
	{
		array<string> keys = new array<string>();
		CollectStagedKeys(fileKey, keys);
		for (int i = 0; i < keys.Count(); i++)
		{
			RemoveStagedKey(keys[i]);
		}

		m_BaseRev.Remove(fileKey);
		m_BulkFiles.Remove(fileKey);
		m_RebasePending.Remove(fileKey);
	}

	protected void CollectStagedKeys(string fileKey, array<string> outKeys)
	{
		for (int i = 0; i < m_Staged.Count(); i++)
		{
			VPPXEStagedEdit staged = m_Staged.GetElement(i);
			if (staged && staged.FileKey == fileKey)
			{
				outKeys.Insert(m_Staged.GetKey(i));
			}
		}
	}

	int CountStaged()
	{
		return m_Staged.Count();
	}

	int CountStagedInFile(string fileKey)
	{
		return m_StagedCount.Get(fileKey);
	}

	int CountFilesWithStaged()
	{
		return m_StagedCount.Count();
	}

	bool HasPendingEdits(string fileKey)
	{
		if (fileKey == "")
		{
			return m_Staged.Count() > 0;
		}

		return m_StagedCount.Get(fileKey) > 0;
	}

	//lower name -> STAGED_PENDING | STAGED_CONFLICT for the staged edits of scopeFile ("" = every file)
	void BuildStagedFlags(string scopeFile, map<string, int> outFlags)
	{
		for (int i = 0; i < m_Staged.Count(); i++)
		{
			VPPXEStagedEdit staged = m_Staged.GetElement(i);
			if (!staged)
			{
				continue;
			}

			if (scopeFile != "" && staged.FileKey != scopeFile)
			{
				continue;
			}

			string lower = LowerOf(staged.Name);
			int bits = outFlags.Get(lower) | STAGED_PENDING;
			if (staged.Conflict)
			{
				bits = bits | STAGED_CONFLICT;
			}

			outFlags.Set(lower, bits);
		}
	}

	void MarkBulk(string fileKey)
	{
		m_BulkFiles.Set(fileKey, true);
	}

	bool IsBulk(string fileKey)
	{
		return m_BulkFiles.Contains(fileKey);
	}

	//---------------------------------------------------------------- wire edits and batches

	//rebuilds the staged deltas into an UPDATE against the CURRENT rows; null when the def no longer exists (orphan)
	VPPXETypeEdit BuildEdit(VPPXEStagedEdit staged)
	{
		if (!staged)
		{
			return null;
		}

		VPPXETypeRow defRow = GetDefRow(staged.FileKey, staged.Name);
		if (!defRow)
		{
			return null;
		}

		VPPXETypeEdit edit = new VPPXETypeEdit();
		edit.Op = VPPXEOp.UPDATE;
		edit.Name = defRow.Name;
		int keepBits = ScalarBits() | VPPXEField.CATEGORY;
		edit.SetMask = staged.SetMask & keepBits;
		edit.ClearMask = staged.ClearMask & keepBits;
		for (int i = 0; i < 7; i++)
		{
			int bit = VPPXETypeMerge.FieldBitAt(i);
			if ((edit.SetMask & bit) != 0)
			{
				SetEditScalar(edit, bit, staged.GetScalar(bit));
			}
		}

		if ((edit.SetMask & VPPXEField.CATEGORY) != 0)
		{
			edit.Category = staged.Category;
		}

		if ((staged.FlagsOn | staged.FlagsOff) != 0)
		{
			int baseFlags = BaseFlags(staged.FileKey, staged.Name);
			edit.SetMask = edit.SetMask | VPPXEField.FLAGS;
			edit.FlagsValue = (baseFlags & ~staged.FlagsOff) | staged.FlagsOn;
		}

		if (staged.TouchesElement(VPPXEField.USAGE))
		{
			edit.UsageMode = VPPXEListMode.REPLACE;
			BuildListNames(staged, LIST_USAGE, edit.Usages);
			BuildListNames(staged, LIST_USAGE_USER, edit.UsageUsers);
		}

		if (staged.TouchesElement(VPPXEField.VALUE))
		{
			edit.ValueMode = VPPXEListMode.REPLACE;
			BuildListNames(staged, LIST_VALUE, edit.Values);
			BuildListNames(staged, LIST_VALUE_USER, edit.ValueUsers);
		}

		if (staged.TouchesElement(VPPXEField.TAG))
		{
			edit.TagMode = VPPXEListMode.REPLACE;
			BuildListNames(staged, LIST_TAG, edit.Tags);
		}

		return edit;
	}

	protected void BuildListNames(VPPXEStagedEdit staged, int listKind, array<string> outNames)
	{
		outNames.Clear();
		BaseListNames(staged.FileKey, staged.Name, listKind, outNames);
		array<string> removes = staged.GetRemoveList(listKind);
		for (int i = 0; i < removes.Count(); i++)
		{
			RemoveName(outNames, removes[i]);
		}

		array<string> adds = staged.GetAddList(listKind);
		for (int j = 0; j < adds.Count(); j++)
		{
			if (FindName(outNames, adds[j]) < 0)
			{
				outNames.Insert(adds[j]);
			}
		}
	}

	//drops staged edits whose def no longer exists; counts them per file in m_LastOrphansByFile
	protected int DropOrphans(string onlyFile)
	{
		array<string> orphanKeys = new array<string>();
		for (int i = 0; i < m_Staged.Count(); i++)
		{
			VPPXEStagedEdit staged = m_Staged.GetElement(i);
			if (!staged)
			{
				continue;
			}

			if (onlyFile != "" && staged.FileKey != onlyFile)
			{
				continue;
			}

			if (!DefExists(staged.FileKey, staged.Name) || !IsTypesFile(staged.FileKey))
			{
				orphanKeys.Insert(m_Staged.GetKey(i));
				m_LastOrphansByFile.Set(staged.FileKey, m_LastOrphansByFile.Get(staged.FileKey) + 1);
			}
		}

		for (int j = 0; j < orphanKeys.Count(); j++)
		{
			RemoveStagedKey(orphanKeys[j]);
		}

		return orphanKeys.Count();
	}

	//orphans dropped by the last BuildBatches/BuildFileBatch, per file
	map<string, int> GetLastOrphans()
	{
		return m_LastOrphansByFile;
	}

	string GetLastTooManyFile()
	{
		return m_LastTooManyFile;
	}

	int GetLastTooManyCount()
	{
		return m_LastTooManyCount;
	}

	//SAVE: one upload per file with staged edits; false (nothing built) when a file exceeds MAX_EDITS_PER_BATCH
	bool BuildBatches(int reason, array<ref VPPXEUploadBatch> outBatches)
	{
		m_LastOrphansByFile.Clear();
		m_LastTooManyFile = "";
		m_LastTooManyCount = 0;
		DropOrphans("");
		array<string> files = new array<string>();
		for (int i = 0; i < m_StagedCount.Count(); i++)
		{
			files.Insert(m_StagedCount.GetKey(i));
		}

		for (int j = 0; j < files.Count(); j++)
		{
			int count = m_StagedCount.Get(files[j]);
			if (count > VPPXEConst.MAX_EDITS_PER_BATCH)
			{
				m_LastTooManyFile = files[j];
				m_LastTooManyCount = count;
				return false;
			}
		}

		for (int k = 0; k < files.Count(); k++)
		{
			string fileKey = files[k];
			int fileReason = reason;
			if (IsBulk(fileKey))
			{
				fileReason = VPPXEBackupReason.BULK;
			}

			VPPXEUploadBatch up = BuildFileUpload(fileKey, fileReason, null, "");
			if (up)
			{
				outBatches.Insert(up);
			}
		}

		return true;
	}

	//structural op: the file's staged UPDATEs of OTHER names + the structural edit, Reason STRUCT; null when too many
	VPPXEUploadBatch BuildFileBatch(string fileKey, int reason, VPPXETypeEdit extraEdit, string excludeName)
	{
		m_LastOrphansByFile.Clear();
		m_LastTooManyFile = "";
		m_LastTooManyCount = 0;
		DropOrphans(fileKey);
		return BuildFileUpload(fileKey, reason, extraEdit, LowerOf(excludeName));
	}

	protected VPPXEUploadBatch BuildFileUpload(string fileKey, int reason, VPPXETypeEdit extraEdit, string excludeLower)
	{
		array<ref VPPXETypeEdit> edits = new array<ref VPPXETypeEdit>();
		for (int i = 0; i < m_Staged.Count(); i++)
		{
			VPPXEStagedEdit staged = m_Staged.GetElement(i);
			if (!staged || staged.FileKey != fileKey)
			{
				continue;
			}

			if (excludeLower != "" && LowerOf(staged.Name) == excludeLower)
			{
				continue;
			}

			VPPXETypeEdit edit = BuildEdit(staged);
			if (edit)
			{
				edits.Insert(edit);
			}
		}

		if (extraEdit)
		{
			edits.Insert(extraEdit);
		}

		if (edits.Count() == 0)
		{
			return null;
		}

		if (edits.Count() > VPPXEConst.MAX_EDITS_PER_BATCH)
		{
			m_LastTooManyFile = fileKey;
			m_LastTooManyCount = edits.Count();
			return null;
		}

		int baseRev = GetRowsRevision(fileKey);
		int stagedBase;
		if (m_BaseRev.Find(fileKey, stagedBase))
		{
			baseRev = stagedBase;
		}

		VPPXEUploadBatch up = new VPPXEUploadBatch();
		up.ReqId = m_Owner.NextReqId();
		up.FileKey = fileKey;
		up.EditCount = edits.Count();
		if (extraEdit && extraEdit.TargetFileKey != "")
		{
			up.TargetFileKey = extraEdit.TargetFileKey;
		}

		int perPart = VPPXEConst.EDITS_PER_PART;
		int partCount = (edits.Count() + perPart - 1) / perPart;
		for (int p = 0; p < partCount; p++)
		{
			VPPXEEditBatch part = new VPPXEEditBatch();
			part.ReqId = up.ReqId;
			part.FileKey = fileKey;
			part.BaseRevision = baseRev;
			part.Note = "";
			part.PartIdx = p;
			part.PartCount = partCount;
			part.Reason = reason;
			int startIdx = p * perPart;
			int endIdx = startIdx + perPart;
			if (endIdx > edits.Count())
			{
				endIdx = edits.Count();
			}

			for (int e = startIdx; e < endIdx; e++)
			{
				part.Edits.Insert(edits[e]);
			}

			up.Parts.Insert(part);
		}

		return up;
	}

	//---------------------------------------------------------------- save locks

	void LockFile(string fileKey, int reqId)
	{
		m_Locks.Set(fileKey, new VPPXEFileLock(reqId, GetRowsRevision(fileKey), Now()));
	}

	bool IsFileLocked(string fileKey)
	{
		return m_Locks.Contains(fileKey);
	}

	bool IsAnyLocked()
	{
		return m_Locks.Count() > 0;
	}

	void UnlockFile(string fileKey)
	{
		m_Locks.Remove(fileKey);
	}

	void UnlockReq(int reqId)
	{
		array<string> keys = new array<string>();
		for (int i = 0; i < m_Locks.Count(); i++)
		{
			VPPXEFileLock fileLock = m_Locks.GetElement(i);
			if (fileLock && fileLock.ReqId == reqId)
			{
				keys.Insert(m_Locks.GetKey(i));
			}
		}

		for (int j = 0; j < keys.Count(); j++)
		{
			m_Locks.Remove(keys[j]);
		}
	}

	//refreshes the lock time of every file of a request (upload finished, progress, result)
	void TouchLocks(int reqId)
	{
		int now = Now();
		for (int i = 0; i < m_Locks.Count(); i++)
		{
			VPPXEFileLock fileLock = m_Locks.GetElement(i);
			if (fileLock && fileLock.ReqId == reqId)
			{
				fileLock.Stamp = now;
			}
		}
	}

	int GetLockRev(string fileKey)
	{
		VPPXEFileLock fileLock = m_Locks.Get(fileKey);
		if (!fileLock)
		{
			return 0;
		}

		return fileLock.RevAtSend;
	}

	//after an Ok result: the lock is released by the first index reply with a different FileRevision
	void SetLockAwaitIndex(string fileKey)
	{
		VPPXEFileLock fileLock = m_Locks.Get(fileKey);
		if (fileLock)
		{
			fileLock.AwaitIndex = true;
			fileLock.Stamp = Now();
		}
	}

	bool ReleaseLockOnIndex(string fileKey, int fileRevision)
	{
		VPPXEFileLock fileLock = m_Locks.Get(fileKey);
		if (!fileLock || !fileLock.AwaitIndex)
		{
			return false;
		}

		if (fileRevision == fileLock.RevAtSend)
		{
			return false;
		}

		m_Locks.Remove(fileKey);
		return true;
	}

	//locks older than INFLIGHT_TIMEOUT_MS are released; the caller re-requests their rows
	void CollectExpiredLocks(array<string> outKeys)
	{
		int now = Now();
		for (int i = 0; i < m_Locks.Count(); i++)
		{
			VPPXEFileLock fileLock = m_Locks.GetElement(i);
			if (fileLock && now - fileLock.Stamp > VPPXEConst.INFLIGHT_TIMEOUT_MS)
			{
				outKeys.Insert(m_Locks.GetKey(i));
			}
		}

		for (int j = 0; j < outKeys.Count(); j++)
		{
			m_Locks.Remove(outKeys[j]);
		}
	}

	void RegisterSave(int reqId, string fileKey)
	{
		m_SaveReqs.Set(reqId, fileKey);
	}

	bool IsSaveReq(int reqId)
	{
		return m_SaveReqs.Contains(reqId);
	}

	void ForgetSave(int reqId)
	{
		m_SaveReqs.Remove(reqId);
	}

	//---------------------------------------------------------------- rebase after STALE

	void MarkRebase(string fileKey)
	{
		m_RebasePending.Set(fileKey, true);
	}

	bool IsRebasePending(string fileKey)
	{
		return m_RebasePending.Contains(fileKey);
	}

	//BaseRevision := RowsRevision; staged edits whose def vanished are dropped (orphans);
	//a changed def signature marks a conflict. Returns the conflict count.
	int Rebase(string fileKey, out int orphans)
	{
		m_RebasePending.Remove(fileKey);
		orphans = 0;
		int conflicts = 0;
		array<string> keys = new array<string>();
		CollectStagedKeys(fileKey, keys);
		for (int i = 0; i < keys.Count(); i++)
		{
			VPPXEStagedEdit staged = m_Staged.Get(keys[i]);
			if (!staged)
			{
				continue;
			}

			VPPXETypeRow defRow = GetDefRow(fileKey, staged.Name);
			if (!defRow)
			{
				RemoveStagedKey(keys[i]);
				orphans++;
				continue;
			}

			string sig = VPPXETypeMerge.Signature(defRow);
			if (sig != staged.BaseSignature)
			{
				staged.Conflict = true;
				conflicts++;
			}

			staged.BaseSignature = sig;
		}

		if (m_StagedCount.Get(fileKey) > 0)
		{
			m_BaseRev.Set(fileKey, GetRowsRevision(fileKey));
		}
		else
		{
			m_BaseRev.Remove(fileKey);
		}

		return conflicts;
	}

	//---------------------------------------------------------------- effect preview

	protected string MaskNames(array<string> names, int mask, array<string> groups, int groupMask)
	{
		VPPXELimits lim = GetLimits();
		array<string> parts = new array<string>();
		lim.NamesOf(names, mask, parts);
		if (groups)
		{
			array<string> groupNames = new array<string>();
			lim.NamesOf(groups, groupMask, groupNames);
			for (int i = 0; i < groupNames.Count(); i++)
			{
				parts.Insert("@" + groupNames[i]);
			}
		}

		if (parts.Count() == 0)
		{
			return "-";
		}

		string joined = parts[0];
		for (int j = 1; j < parts.Count(); j++)
		{
			joined = joined + "," + parts[j];
		}

		return joined;
	}

	protected string FieldText(VPPXETypeRow row, int fieldBit)
	{
		VPPXELimits lim = GetLimits();
		if (VPPXETypeMerge.IsScalarBit(fieldBit))
		{
			if ((row.Present & fieldBit) == 0)
			{
				return "-";
			}

			int num = VPPXETypeMerge.GetScalar(row, fieldBit);
			return num.ToString();
		}

		if (fieldBit == VPPXEField.FLAGS)
		{
			if ((row.Present & VPPXEField.FLAGS) == 0)
			{
				return "-";
			}

			string flagText = VPPXETypeMerge.FormatFlags(row.Flags);
			if (flagText == "")
			{
				return "0";
			}

			return flagText;
		}

		if (fieldBit == VPPXEField.CATEGORY)
		{
			if ((row.Present & VPPXEField.CATEGORY) == 0 || row.Category < 0 || row.Category >= lim.Categories.Count())
			{
				return "-";
			}

			return lim.Categories[row.Category];
		}

		if (fieldBit == VPPXEField.USAGE)
		{
			return MaskNames(lim.Usages, row.Usage, lim.UsageGroups, row.UsageUser);
		}

		if (fieldBit == VPPXEField.VALUE)
		{
			return MaskNames(lim.Values, row.Value, lim.ValueGroups, row.ValueUser);
		}

		return MaskNames(lim.Tags, row.Tag, null, 0);
	}

	//data text of the effective change of resultName for a structural op (VPPXEOp):
	//ADD (template in targetFile), DUPLICATE/RENAME (copy of srcName's def in srcFile), COPY (copy into targetFile),
	//MOVE (copy into targetFile, srcFile def removed), DELETE (srcFile def removed)
	string DescribeEffectChange(string resultName, int op, string srcName, string srcFileKey, string targetFileKey)
	{
		VPPXELimits lim = GetLimits();
		string lower = LowerOf(resultName);
		array<VPPXETypeRow> afterRows = new array<VPPXETypeRow>();
		array<string> afterFiles = new array<string>();
		VPPXETypeRow before = VPPXETypeMerge.NewRow(resultName);
		string winnerBefore = "-";
		array<VPPXETypeRow> defs = m_NameDefs.Get(lower);
		array<string> defFiles = m_NameDefFiles.Get(lower);
		if (defs && defFiles)
		{
			for (int i = 0; i < defs.Count(); i++)
			{
				VPPXETypeMerge.MergeInto(before, defs[i], lim);
				winnerBefore = defFiles[i];
				bool removed = false;
				if ((op == VPPXEOp.MOVE || op == VPPXEOp.DELETE) && defFiles[i] == srcFileKey)
				{
					removed = true;
				}

				if (!removed)
				{
					afterRows.Insert(defs[i]);
					afterFiles.Insert(defFiles[i]);
				}
			}
		}

		VPPXETypeRow inserted = null;
		string insertFile = "";
		if (op == VPPXEOp.ADD)
		{
			inserted = VPPXETypeMerge.TemplateRow(resultName);
			insertFile = targetFileKey;
		}
		else if (op == VPPXEOp.DUPLICATE || op == VPPXEOp.RENAME || op == VPPXEOp.COPY || op == VPPXEOp.MOVE)
		{
			VPPXETypeRow srcDef = GetDefRow(srcFileKey, srcName);
			if (srcDef)
			{
				inserted = VPPXETypeMerge.Copy(srcDef);
				inserted.Name = resultName;
			}

			insertFile = srcFileKey;
			if (op == VPPXEOp.COPY || op == VPPXEOp.MOVE)
			{
				insertFile = targetFileKey;
			}
		}

		if (inserted)
		{
			int insertOrder = FileIdxOf(insertFile);
			int at = afterRows.Count();
			for (int j = 0; j < afterFiles.Count(); j++)
			{
				if (FileIdxOf(afterFiles[j]) > insertOrder)
				{
					at = j;
					break;
				}
			}

			afterRows.InsertAt(inserted, at);
			afterFiles.InsertAt(insertFile, at);
		}

		VPPXETypeRow after = VPPXETypeMerge.NewRow(resultName);
		string winnerAfter = "-";
		for (int k = 0; k < afterRows.Count(); k++)
		{
			VPPXETypeMerge.MergeInto(after, afterRows[k], lim);
			winnerAfter = afterFiles[k];
		}

		array<string> parts = new array<string>();
		for (int f = 0; f < VPPXEConst.FIELD_COUNT; f++)
		{
			int bit = VPPXETypeMerge.FieldBitAt(f);
			string textBefore = FieldText(before, bit);
			string textAfter = FieldText(after, bit);
			if (textBefore != textAfter)
			{
				parts.Insert(VPPXETypeMerge.FieldElementName(bit) + " " + textBefore + ">" + textAfter);
			}
		}

		if (winnerBefore != winnerAfter)
		{
			string labelBefore = winnerBefore;
			string labelAfter = winnerAfter;
			if (winnerBefore != "-")
			{
				labelBefore = FileLabel(winnerBefore);
			}

			if (winnerAfter != "-")
			{
				labelAfter = FileLabel(winnerAfter);
			}

			parts.Insert("file " + labelBefore + ">" + labelAfter);
		}

		if (parts.Count() == 0)
		{
			if (m_Owner)
			{
				return m_Owner.Tr("#VSTR_XMLE_EFFECT_NONE");
			}

			return "";
		}

		string joined = parts[0];
		for (int n = 1; n < parts.Count(); n++)
		{
			joined = joined + "; " + parts[n];
		}

		return joined;
	}

	//---------------------------------------------------------------- search and filters

	protected int PrefixMask(array<string> names, string argLower)
	{
		int mask = 0;
		if (!names || argLower == "")
		{
			return 0;
		}

		for (int i = 0; i < names.Count() && i < 32; i++)
		{
			string candidate = LowerOf(names[i]);
			if (candidate.IndexOf(argLower) == 0)
			{
				mask = mask | (1 << i);
			}
		}

		return mask;
	}

	//tokens split on spaces: c:NAME u:NAME v:NAME t:NAME (limits names, case-insensitive prefix; u:@G / v:@G = user groups),
	//f:TEXT (substring of a def's file key); other tokens are name substrings; all must match
	VPPXESearchSpec ParseSearch(string searchText)
	{
		VPPXESearchSpec spec = new VPPXESearchSpec();
		VPPXELimits lim = GetLimits();
		string lowered = LowerOf(searchText);
		array<string> tokens = new array<string>();
		lowered.Split(" ", tokens);
		for (int i = 0; i < tokens.Count(); i++)
		{
			string token = tokens[i];
			int len = token.Length();
			if (len >= 2 && token.Get(1) == ":")
			{
				string prefix = token.Get(0);
				string arg = "";
				if (len > 2)
				{
					arg = token.Substring(2, len - 2);
				}

				bool isGroup = false;
				if (arg.Length() > 0 && arg.Get(0) == "@")
				{
					isGroup = true;
					arg = arg.Substring(1, arg.Length() - 1);
				}

				if (prefix == "c" || prefix == "u" || prefix == "v" || prefix == "t" || prefix == "f")
				{
					if (arg == "")
					{
						continue;
					}

					if (prefix == "c")
					{
						spec.AddCond(COND_CAT, PrefixMask(lim.Categories, arg));
					}
					else if (prefix == "u" && isGroup)
					{
						spec.AddCond(COND_USAGE_GROUP, PrefixMask(lim.UsageGroups, arg));
					}
					else if (prefix == "u")
					{
						spec.AddCond(COND_USAGE, PrefixMask(lim.Usages, arg));
					}
					else if (prefix == "v" && isGroup)
					{
						spec.AddCond(COND_VALUE_GROUP, PrefixMask(lim.ValueGroups, arg));
					}
					else if (prefix == "v")
					{
						spec.AddCond(COND_VALUE, PrefixMask(lim.Values, arg));
					}
					else if (prefix == "t")
					{
						spec.AddCond(COND_TAG, PrefixMask(lim.Tags, arg));
					}
					else
					{
						spec.FileTexts.Insert(arg);
					}

					continue;
				}
			}

			spec.Words.Insert(token);
		}

		return spec;
	}

	protected bool MatchesSearch(VPPXESearchSpec spec, string lower, VPPXETypeRow row)
	{
		for (int i = 0; i < spec.Words.Count(); i++)
		{
			if (lower.IndexOf(spec.Words[i]) < 0)
			{
				return false;
			}
		}

		VPPXELimits lim = GetLimits();
		for (int j = 0; j < spec.CondKinds.Count(); j++)
		{
			int mask = spec.CondMasks[j];
			int kind = spec.CondKinds[j];
			bool ok = false;
			if (kind == COND_CAT)
			{
				if ((row.Present & VPPXEField.CATEGORY) != 0 && row.Category >= 0 && row.Category < 32)
				{
					ok = (mask & (1 << row.Category)) != 0;
				}
			}
			else if (kind == COND_USAGE)
			{
				ok = (lim.EffectiveUsage(row.Usage, row.UsageUser) & mask) != 0;
			}
			else if (kind == COND_USAGE_GROUP)
			{
				ok = (row.UsageUser & mask) != 0;
			}
			else if (kind == COND_VALUE)
			{
				ok = (lim.EffectiveValue(row.Value, row.ValueUser) & mask) != 0;
			}
			else if (kind == COND_VALUE_GROUP)
			{
				ok = (row.ValueUser & mask) != 0;
			}
			else if (kind == COND_TAG)
			{
				ok = (row.Tag & mask) != 0;
			}

			if (!ok)
			{
				return false;
			}
		}

		if (spec.FileTexts.Count() > 0)
		{
			array<string> defFiles = m_NameDefFiles.Get(lower);
			for (int k = 0; k < spec.FileTexts.Count(); k++)
			{
				bool fileOk = false;
				if (defFiles)
				{
					for (int m = 0; m < defFiles.Count(); m++)
					{
						string keyLower = LowerOf(defFiles[m]);
						if (keyLower.IndexOf(spec.FileTexts[k]) >= 0)
						{
							fileOk = true;
							break;
						}
					}
				}

				if (!fileOk)
				{
					return false;
				}
			}
		}

		return true;
	}

	protected bool MatchesFilter(int filterKind, int filterArg, string lower, VPPXETypeRow row, map<string, int> stagedFlags)
	{
		VPPXELimits lim = GetLimits();
		bool hasNominal = (row.Present & VPPXEField.NOMINAL) != 0;
		bool argOk = filterArg >= 0 && filterArg < 32;
		int argBit = 0;
		if (argOk)
		{
			argBit = 1 << filterArg;
		}

		if (filterKind == FILTER_ISSUES)
		{
			return VPPXEText.RowSeverity(row.Issues) >= VPPXESeverity.WARNING;
		}

		if (filterKind == FILTER_OVERRIDDEN)
		{
			return DistinctDefFileCount(lower) >= 2;
		}

		if (filterKind == FILTER_UNSAVED)
		{
			return (stagedFlags.Get(lower) & STAGED_PENDING) != 0;
		}

		if (filterKind == FILTER_CONFLICTS)
		{
			return (stagedFlags.Get(lower) & STAGED_CONFLICT) != 0;
		}

		if (filterKind == FILTER_SPAWNING)
		{
			return hasNominal && row.Nominal > 0;
		}

		if (filterKind == FILTER_NOSPAWN)
		{
			return !hasNominal || row.Nominal <= 0;
		}

		if (filterKind == FILTER_NOCAT)
		{
			return (row.Present & VPPXEField.CATEGORY) == 0 || row.Category < 0;
		}

		if (filterKind == FILTER_CATEGORY)
		{
			return (row.Present & VPPXEField.CATEGORY) != 0 && row.Category == filterArg;
		}

		if (filterKind == FILTER_USAGE)
		{
			return argOk && (lim.EffectiveUsage(row.Usage, row.UsageUser) & argBit) != 0;
		}

		if (filterKind == FILTER_VALUE)
		{
			return argOk && (lim.EffectiveValue(row.Value, row.ValueUser) & argBit) != 0;
		}

		if (filterKind == FILTER_TAG)
		{
			return argOk && (row.Tag & argBit) != 0;
		}

		//registered rows never match "not registered" (the bulk editor gets an empty set)
		if (filterKind == FILTER_UNREGISTERED)
		{
			return false;
		}

		return true;
	}

	//every lowercase name of the scope ("" = merged effective rows, else that file's defs) matching the search and the
	//Show filter, in rail order (uncapped: the bulk editor uses the full set)
	int CollectMatches(string scopeFile, string searchText, int filterKind, int filterArg, array<string> outLower)
	{
		VPPXESearchSpec spec = ParseSearch(searchText);
		map<string, int> stagedFlags = new map<string, int>();
		BuildStagedFlags(scopeFile, stagedFlags);
		for (int i = 0; i < m_Sorted.Count(); i++)
		{
			string lower = m_Sorted[i];
			VPPXETypeRow row;
			if (scopeFile == "")
			{
				row = m_Merged.Get(lower);
			}
			else
			{
				row = m_FileDefRow.Get(scopeFile + "|" + lower);
			}

			if (!row)
			{
				continue;
			}

			if (!MatchesSearch(spec, lower, row))
			{
				continue;
			}

			if (!MatchesFilter(filterKind, filterArg, lower, row, stagedFlags))
			{
				continue;
			}

			outLower.Insert(lower);
		}

		return outLower.Count();
	}

	//Show filter "not registered": game config classes (VPPXEConfigClasses) that no types file defines, in name order,
	//matching the search words (c:/u:/v:/t:/f: terms need a types entry, so they match nothing); GetUnregisteredTotal()
	//is the count before the search
	int CollectUnregistered(string searchText, array<string> outLower)
	{
		VPPXESearchSpec spec = ParseSearch(searchText);
		string lower = "";
		bool wordsOk = true;
		int total = 0;
		int classCount = 0;
		m_UnregTotal = 0;
		VPPXEConfigClasses.EnsureScanned();
		classCount = VPPXEConfigClasses.Count();
		for (int i = 0; i < classCount; i++)
		{
			lower = VPPXEConfigClasses.LowerAt(i);
			if (m_Merged.Contains(lower))
			{
				continue;
			}

			total++;
			if (spec.CondKinds.Count() > 0 || spec.FileTexts.Count() > 0)
			{
				continue;
			}

			wordsOk = true;
			for (int w = 0; w < spec.Words.Count(); w++)
			{
				if (lower.IndexOf(spec.Words[w]) < 0)
				{
					wordsOk = false;
					break;
				}
			}

			if (wordsOk)
			{
				outLower.Insert(lower);
			}
		}

		m_UnregTotal = total;
		return outLower.Count();
	}

	int GetUnregisteredTotal()
	{
		return m_UnregTotal;
	}

	//number of names in the scope
	int CountScope(string scopeFile)
	{
		if (scopeFile == "")
		{
			return m_NameList.Count();
		}

		return GetFileNameCount(scopeFile);
	}

	//rail row severity: -1 none, else VPPXESeverity
	int RowSeverityOf(string lower, string scopeFile)
	{
		VPPXETypeRow row;
		if (scopeFile == "")
		{
			row = m_Merged.Get(lower);
		}
		else
		{
			row = m_FileDefRow.Get(scopeFile + "|" + lower);
		}

		if (!row)
		{
			return -1;
		}

		return VPPXEText.RowSeverity(row.Issues);
	}
};

// Game config classes a types file can define, for the TYPES rail's "not registered" filter: the public (scope 2)
// CfgVehicles / CfgWeapons / CfgMagazines classes the Item Manager lists, minus buildings, characters and AI
// (House, HouseNoDestruct, Man, DZ_LightAI), which the Central Economy never spawns from types. Scanned once per client
// session on first use (mods do not change while the game runs), sorted by lowercase name.
class VPPXEConfigClasses
{
	const static int ROOT_COUNT = 3;

	protected static ref array<string> s_Lowers;
	protected static ref map<string, string> s_Display;
	protected static ref map<string, int> s_Root;

	static string RootName(int rootIdx)
	{
		if (rootIdx == 0)
		{
			return "CfgVehicles";
		}

		if (rootIdx == 1)
		{
			return "CfgWeapons";
		}

		return "CfgMagazines";
	}

	//one config full path (the class and every parent) touching a base the Central Economy does not spawn from types
	protected static bool IsExcludedPath(TStringArray fullPath)
	{
		string entry = "";
		for (int i = 0; i < fullPath.Count(); i++)
		{
			entry = fullPath[i];
			entry.ToLower();
			if (entry == "house" || entry == "housenodestruct" || entry == "man" || entry == "dz_lightai")
			{
				return true;
			}
		}

		return false;
	}

	static void EnsureScanned()
	{
		if (s_Lowers)
		{
			return;
		}

		s_Lowers = new array<string>();
		s_Display = new map<string, string>();
		s_Root = new map<string, int>();
		string rootPath = "";
		string className = "";
		string lower = "";
		int classCount = 0;
		int scope = 0;
		TStringArray fullPath = new TStringArray();
		for (int r = 0; r < ROOT_COUNT; r++)
		{
			rootPath = RootName(r);
			classCount = GetGame().ConfigGetChildrenCount(rootPath);
			for (int i = 0; i < classCount; i++)
			{
				className = "";
				GetGame().ConfigGetChildName(rootPath, i, className);
				if (className == "")
				{
					continue;
				}

				scope = GetGame().ConfigGetInt(rootPath + " " + className + " scope");
				if (scope < 2)
				{
					continue;
				}

				lower = className;
				lower.ToLower();
				if (s_Display.Contains(lower))
				{
					continue;
				}

				if (r == 0)
				{
					fullPath.Clear();
					GetGame().ConfigGetFullPath(rootPath + " " + className, fullPath);
					if (IsExcludedPath(fullPath))
					{
						continue;
					}
				}

				s_Lowers.Insert(lower);
				s_Display.Set(lower, className);
				s_Root.Set(lower, r);
			}
		}

		s_Lowers.Sort();
		Print("[XMLEditor] TYPES: scanned " + s_Lowers.Count().ToString() + " public config classes for the 'not registered' filter");
	}

	static int Count()
	{
		if (!s_Lowers)
		{
			return 0;
		}

		return s_Lowers.Count();
	}

	static string LowerAt(int index)
	{
		return s_Lowers[index];
	}

	static bool Has(string className)
	{
		if (!s_Display)
		{
			return false;
		}

		string lower = className;
		lower.ToLower();
		return s_Display.Contains(lower);
	}

	//config spelling of a class name (the name as given when it is not a scanned class)
	static string DisplayOf(string className)
	{
		string lower = className;
		lower.ToLower();
		string shown = "";
		if (s_Display && s_Display.Find(lower, shown))
		{
			return shown;
		}

		return className;
	}

	//config root ("CfgVehicles", ...) of a scanned class, "" when it is not one
	static string RootOf(string className)
	{
		string lower = className;
		lower.ToLower();
		int rootIdx = -1;
		if (!s_Root || !s_Root.Find(lower, rootIdx))
		{
			return "";
		}

		return RootName(rootIdx);
	}

	//the in-game display name of a scanned class ("" when it has none)
	static string GameNameOf(string className)
	{
		string rootPath = RootOf(className);
		string gameName = "";
		if (rootPath == "")
		{
			return "";
		}

		GetGame().ConfigGetText(rootPath + " " + DisplayOf(className) + " displayName", gameName);
		return gameName;
	}
};
