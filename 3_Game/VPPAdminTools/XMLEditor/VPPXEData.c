// XML Editor RPC DTOs (INTERFACES v2.1 section 2). Field names, types and order are frozen: the engine
// serializer sends every member in declaration order, and tools/verify_xml_editor.py checks the lists.
// Every class has a no-arg constructor that allocates each ref member.

class VPPXEFileInfo
{
	string Key;
	string Label;
	int Kind;
	int LoadOrder;
	int Flags;
	int Revision;
	int EntryCount;
	int IssueCount;

	void VPPXEFileInfo()
	{
		Key = "";
		Label = "";
	}
};

class VPPXELimits
{
	ref array<string> Categories;
	ref array<string> Tags;
	ref array<string> Usages;
	ref array<string> Values;
	ref array<string> UsageGroups;
	ref array<int> UsageGroupMasks;
	ref array<string> ValueGroups;
	ref array<int> ValueGroupMasks;

	void VPPXELimits()
	{
		Categories = new array<string>;
		Tags = new array<string>;
		Usages = new array<string>;
		Values = new array<string>;
		UsageGroups = new array<string>;
		UsageGroupMasks = new array<int>;
		ValueGroups = new array<string>;
		ValueGroupMasks = new array<int>;
	}

	// Case-insensitive lookup, -1 if missing. Lowercases local copies only (never the caller's strings).
	static int FindIn(array<string> list, string name)
	{
		return VPPXmlText.FindNoCase(list, name);
	}

	int FindCategory(string name)
	{
		return FindIn(Categories, name);
	}

	int FindTag(string name)
	{
		return FindIn(Tags, name);
	}

	int FindUsage(string name)
	{
		return FindIn(Usages, name);
	}

	int FindValue(string name)
	{
		return FindIn(Values, name);
	}

	int FindUsageGroup(string name)
	{
		return FindIn(UsageGroups, name);
	}

	int FindValueGroup(string name)
	{
		return FindIn(ValueGroups, name);
	}

	// OR of 1 << idx for every known name whose idx < 32 (unknown names and idx >= 32 are ignored).
	int MaskOf(array<string> list, array<string> names)
	{
		if (!list || !names)
		{
			return 0;
		}

		int mask = 0;
		int total = names.Count();
		for (int i = 0; i < total; i++)
		{
			int idx = FindIn(list, names[i]);
			if (idx < 0 || idx >= 32)
			{
				continue;
			}

			mask = mask | (1 << idx);
		}

		return mask;
	}

	// Plain usage bits OR the masks of every set user-group bit.
	int EffectiveUsage(int usageBits, int usageGroupBits)
	{
		return usageBits | ResolveGroups(UsageGroupMasks, usageGroupBits);
	}

	// Plain value bits OR the masks of every set user-group bit.
	int EffectiveValue(int valueBits, int valueGroupBits)
	{
		return valueBits | ResolveGroups(ValueGroupMasks, valueGroupBits);
	}

	// Names of the set bits, in list order (bits 0..31 only).
	void NamesOf(array<string> list, int mask, array<string> outNames)
	{
		if (!list || !outNames || mask == 0)
		{
			return;
		}

		int total = list.Count();
		if (total > 32)
		{
			total = 32;
		}

		for (int i = 0; i < total; i++)
		{
			if ((mask & (1 << i)) != 0)
			{
				outNames.Insert(list[i]);
			}
		}
	}

	int ResolveGroups(array<int> groupMasks, int groupBits)
	{
		if (!groupMasks || groupBits == 0)
		{
			return 0;
		}

		int total = groupMasks.Count();
		if (total > 32)
		{
			total = 32;
		}

		int result = 0;
		for (int i = 0; i < total; i++)
		{
			if ((groupBits & (1 << i)) != 0)
			{
				result = result | groupMasks[i];
			}
		}

		return result;
	}
};

class VPPXESessionInfo
{
	int Protocol;
	int PermMask;
	int RegistryRev;
	int WorldSize;
	string WorldName;
	int BackupMaxPerFile;
	int BackupMaxAgeDays;
	int BackupMaxTotalMB;
	bool IndexReady;
	int EolMode;
	ref array<ref VPPXEFileInfo> Files;
	ref VPPXELimits Limits;

	void VPPXESessionInfo()
	{
		Protocol = VPPXEConst.PROTOCOL;
		WorldName = "";
		EolMode = VPPXEEolMode.UNKNOWN;
		Files = new array<ref VPPXEFileInfo>;
		Limits = new VPPXELimits();
	}
};

class VPPXEProgress
{
	int ReqId;
	int Stage;
	int Percent;

	void VPPXEProgress()
	{
		ReqId = 0;
		Stage = 0;
		Percent = 0;
	}
};

class VPPXETypeRow
{
	string Name;
	int FileIdx;
	int Line;
	int Present;
	int Nominal;
	int Min;
	int Lifetime;
	int Restock;
	int Cost;
	int QuantMin;
	int QuantMax;
	int Flags;
	int Category;
	int Usage;
	int UsageUser;
	int Value;
	int ValueUser;
	int Tag;
	int Issues;
	int DefCount;

	// Scalars UNSET (absent), masks 0, Category -1 (none), FileIdx -1, Present 0, DefCount 0.
	void VPPXETypeRow()
	{
		Name = "";
		FileIdx = -1;
		Line = 0;
		Present = 0;
		Nominal = VPPXEConst.UNSET;
		Min = VPPXEConst.UNSET;
		Lifetime = VPPXEConst.UNSET;
		Restock = VPPXEConst.UNSET;
		Cost = VPPXEConst.UNSET;
		QuantMin = VPPXEConst.UNSET;
		QuantMax = VPPXEConst.UNSET;
		Flags = 0;
		Category = -1;
		Usage = 0;
		UsageUser = 0;
		Value = 0;
		ValueUser = 0;
		Tag = 0;
		Issues = 0;
		DefCount = 0;
	}
};

class VPPXETypeIndexChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	int RegistryRev;
	string FileKey;
	int FileRevision;
	ref array<ref VPPXETypeRow> Rows;

	void VPPXETypeIndexChunk()
	{
		FileKey = "";
		Rows = new array<ref VPPXETypeRow>;
	}
};

class VPPXEIssue
{
	int Code;
	int Severity;
	string FileKey;
	int Line;
	string Entry;
	string Arg;

	void VPPXEIssue()
	{
		FileKey = "";
		Entry = "";
		Arg = "";
	}
};

class VPPXESpawnableInfo
{
	bool Found;
	bool Hoarder;
	bool HasDamage;
	float DamageMin;
	float DamageMax;
	ref array<string> Lines;
	ref array<string> Parents;

	void VPPXESpawnableInfo()
	{
		Lines = new array<string>;
		Parents = new array<string>;
	}
};

class VPPXETypeDetails
{
	int ReqId;
	string Name;
	bool Found;
	int ConfigState;
	bool Ignored;
	ref VPPXETypeRow Effective;
	ref array<ref VPPXETypeRow> Defs;
	ref array<int> DefRevisions;
	ref array<int> FieldSource;
	ref array<string> UnknownRefs;
	ref array<string> RawBlock;
	ref array<ref VPPXEIssue> Issues;
	ref VPPXESpawnableInfo Spawnable;

	void VPPXETypeDetails()
	{
		Name = "";
		ConfigState = VPPXEConfigState.MISSING;
		Effective = new VPPXETypeRow();
		Defs = new array<ref VPPXETypeRow>;
		DefRevisions = new array<int>;
		FieldSource = new array<int>;
		for (int i = 0; i < VPPXEConst.FIELD_COUNT; i++)
		{
			FieldSource.Insert(-1);
		}

		UnknownRefs = new array<string>;
		RawBlock = new array<string>;
		Issues = new array<ref VPPXEIssue>;
		Spawnable = new VPPXESpawnableInfo();
	}
};

class VPPXETypeEdit
{
	int Op;
	string Name;
	string NewName;
	string TargetFileKey;
	int SetMask;
	int ClearMask;
	int Nominal;
	int Min;
	int Lifetime;
	int Restock;
	int Cost;
	int QuantMin;
	int QuantMax;
	int FlagsValue;
	string Category;
	int UsageMode;
	ref array<string> Usages;
	ref array<string> UsageUsers;
	int ValueMode;
	ref array<string> Values;
	ref array<string> ValueUsers;
	int TagMode;
	ref array<string> Tags;
	bool KeepUnknown;
	bool AllowOverride;

	// KeepUnknown true, AllowOverride false, list modes KEEP, scalars UNSET.
	void VPPXETypeEdit()
	{
		Op = VPPXEOp.UPDATE;
		Name = "";
		NewName = "";
		TargetFileKey = "";
		SetMask = 0;
		ClearMask = 0;
		Nominal = VPPXEConst.UNSET;
		Min = VPPXEConst.UNSET;
		Lifetime = VPPXEConst.UNSET;
		Restock = VPPXEConst.UNSET;
		Cost = VPPXEConst.UNSET;
		QuantMin = VPPXEConst.UNSET;
		QuantMax = VPPXEConst.UNSET;
		FlagsValue = 0;
		Category = "";
		UsageMode = VPPXEListMode.KEEP;
		Usages = new array<string>;
		UsageUsers = new array<string>;
		ValueMode = VPPXEListMode.KEEP;
		Values = new array<string>;
		ValueUsers = new array<string>;
		TagMode = VPPXEListMode.KEEP;
		Tags = new array<string>;
		KeepUnknown = true;
		AllowOverride = false;
	}
};

class VPPXEEditBatch
{
	int ReqId;
	string FileKey;
	int BaseRevision;
	string Note;
	int PartIdx;
	int PartCount;
	int Reason;
	ref array<ref VPPXETypeEdit> Edits;

	void VPPXEEditBatch()
	{
		FileKey = "";
		Note = "";
		PartCount = 1;
		Reason = VPPXEBackupReason.EDIT;
		Edits = new array<ref VPPXETypeEdit>;
	}
};

// A batch is all-or-nothing: the first failing name travels in ErrorArg. TouchedFiles = every file key this result wrote.
class VPPXESaveResult
{
	int ReqId;
	bool Ok;
	string ErrorKey;
	string ErrorArg;
	string FileKey;
	int NewRevision;
	string BackupId;
	int Applied;
	ref array<string> TouchedFiles;
	string NoticeKey;

	void VPPXESaveResult()
	{
		ErrorKey = "";
		ErrorArg = "";
		FileKey = "";
		BackupId = "";
		TouchedFiles = new array<string>;
		NoticeKey = "";
	}
};

// Id = client-facing BackupId "slug/YYYYMMDD-HHMMSS-NNN" (a lookup key only). Unverified = raw copy of an unreadable file.
class VPPXEBackupEntry
{
	string Id;
	string FileKey;
	string Stamp;
	string AdminName;
	string AdminId;
	int Reason;
	string Summary;
	string Note;
	int ChangeCount;
	int Size;
	int Hash;
	bool Pinned;
	bool MatchesCurrent;
	bool Unverified;

	void VPPXEBackupEntry()
	{
		Id = "";
		FileKey = "";
		Stamp = "";
		AdminName = "";
		AdminId = "";
		Summary = "";
		Note = "";
	}
};

class VPPXEBackupListChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	int TotalBytes;
	ref array<ref VPPXEBackupEntry> Entries;

	void VPPXEBackupListChunk()
	{
		Entries = new array<ref VPPXEBackupEntry>;
	}
};

class VPPXEDiffRow
{
	int Kind;
	string Entry;
	string Field;
	string Before;
	string After;

	void VPPXEDiffRow()
	{
		Entry = "";
		Field = "";
		Before = "";
		After = "";
	}
};

class VPPXEDiffChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	string BackupId;
	int Mode;
	int Added;
	int Removed;
	int Changed;
	bool Truncated;
	bool Semantic;
	ref array<ref VPPXEDiffRow> Rows;

	void VPPXEDiffChunk()
	{
		BackupId = "";
		Rows = new array<ref VPPXEDiffRow>;
	}
};

class VPPXEIssuesChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	int Total;
	int Errors;
	int Warnings;
	int Infos;
	ref array<ref VPPXEIssue> Issues;

	void VPPXEIssuesChunk()
	{
		Issues = new array<ref VPPXEIssue>;
	}
};

class VPPXEDistTableRow
{
	int Layer;
	string Label;
	string Detail;
	int Count;
	int Points;
	int TierMask;
	float X;
	float Z;

	void VPPXEDistTableRow()
	{
		Label = "";
		Detail = "";
	}
};

// LayerCounts, LayerCells, LayerExtra: 8 entries indexed by VPPXELayer.
class VPPXEDistSummary
{
	int ReqId;
	string TypeName;
	int Flags;
	int CellSize;
	int GridCols;
	float PackScale;
	int TierMaskAll;
	int ChunkCount;
	ref array<int> LayerCounts;
	ref array<int> LayerCells;
	ref array<int> LayerExtra;
	ref array<string> PlayerEvents;

	void VPPXEDistSummary()
	{
		TypeName = "";
		CellSize = VPPXEConst.CELL_SIZE;
		PackScale = 1.0;
		LayerCounts = new array<int>;
		LayerCells = new array<int>;
		LayerExtra = new array<int>;
		for (int i = 0; i < 8; i++)
		{
			LayerCounts.Insert(0);
			LayerCells.Insert(0);
			LayerExtra.Insert(0);
		}

		PlayerEvents = new array<string>;
	}
};

class VPPXEDistChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	int Layer;
	int Kind;
	ref array<int> Ints;
	ref array<float> Floats;
	ref array<ref VPPXEDistTableRow> TableRows;

	void VPPXEDistChunk()
	{
		Ints = new array<int>;
		Floats = new array<float>;
		TableRows = new array<ref VPPXEDistTableRow>;
	}
};

// Coords = [x, z]*, Net = [low, high]*.
class VPPXELiveChunk
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	int Total;
	bool Capped;
	ref array<float> Coords;
	ref array<int> Net;

	void VPPXELiveChunk()
	{
		Coords = new array<float>;
		Net = new array<int>;
	}
};
