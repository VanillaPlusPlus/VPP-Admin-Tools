// XML Editor backup jobs (server only; INTERACTIVE lane, serialized with saves). Each Step does bounded work while
// VPPXEJobQueue.BudgetOk(); every reply echoes the ReqId; every failure replies an XMLE error string key through VPPXENet.
// VPPXEBackupListJob: newest-first list of every manifest (or one file), native-sorted, byte-closed chunks.
// VPPXEDiffJob: semantic types diff by name and field, or a trimmed line diff; clipped cells, byte-closed chunks.
// VPPXERestoreJob: pre-restore backup chosen by the current file state, then VPPXESafeWrite.
// VPPXEBackupOpJob: delete, pin/unpin and manual snapshot.

class VPPXEBackupListJob : VPPXEJob
{
	const static int PH_SLUGS = 0;
	const static int PH_LOAD = 1;
	const static int PH_KEYS = 2;
	const static int PH_SORT = 3;
	const static int PH_ENTRIES = 4;
	const static int PH_SEND = 5;
	const static int PH_DONE = 6;
	const static int CHUNK_HEADER_BYTES = 24;
	const static int INT_MAX_VALUE = 2147483647;

	protected VPPXEBackupManager m_Mgr;
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_FileKey;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_SubCursor;
	protected int m_TotalBytes;
	protected bool m_Replied;
	protected bool m_ProgressSent;
	protected ref array<string> m_Slugs;
	protected ref array<ref VPPXEBackupManifest> m_Lists;
	protected ref array<int> m_Revisions;
	protected ref array<bool> m_Readable;
	protected ref array<string> m_Keys;
	protected ref array<ref VPPXEBackupListChunk> m_Chunks;
	protected ref VPPXEBackupListChunk m_Open;
	protected int m_OpenBytes;

	void VPPXEBackupListJob(VPPXEBackupManager mgr, PlayerIdentity sender, int reqId, string fileKey)
	{
		m_Mgr = mgr;
		m_Sender = sender;
		m_ReqId = reqId;
		m_FileKey = fileKey;
		m_Phase = PH_SLUGS;
		m_Slugs = new array<string>();
		m_Lists = new array<ref VPPXEBackupManifest>();
		m_Revisions = new array<int>();
		m_Readable = new array<bool>();
		m_Keys = new array<string>();
		m_Chunks = new array<ref VPPXEBackupListChunk>();
	}

	override string GetLabel()
	{
		return "BackupList";
	}

	override bool Step()
	{
		// First contact for the admin: the list job used to send nothing before its chunks. DIFF, not TYPES/REGISTRY,
		// because the client treats those two stages as index activity; the Backups tab ignores the stage of list progress.
		if (!m_ProgressSent)
		{
			m_ProgressSent = true;
			VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.DIFF, 0);
		}

		while (m_Phase != PH_DONE && VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_SLUGS)
			{
				RunSlugs();
			}
			else if (m_Phase == PH_LOAD)
			{
				RunLoad();
			}
			else if (m_Phase == PH_KEYS)
			{
				RunKeys();
			}
			else if (m_Phase == PH_SORT)
			{
				m_Keys.Sort(true);
				m_Cursor = 0;
				m_Phase = PH_ENTRIES;
			}
			else if (m_Phase == PH_ENTRIES)
			{
				RunEntries();
			}
			else
			{
				RunSend();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
			m_Replied = true;
		}
	}

	protected void RunSlugs()
	{
		if (m_FileKey == "")
		{
			VPPXEBackupIndex idx = m_Mgr.GetIndex();
			int slugCount = idx.Slugs.Count();
			for (int i = 0; i < slugCount; i++)
			{
				m_Slugs.Insert(idx.Slugs[i]);
			}
		}
		else
		{
			string slug = m_Mgr.SlugForKey(m_FileKey);
			if (slug != "")
			{
				m_Slugs.Insert(slug);
			}
		}

		m_Cursor = 0;
		m_Phase = PH_LOAD;
	}

	// One manifest per call (one native read the first time), plus the registry revision of its file.
	protected void RunLoad()
	{
		if (m_Cursor >= m_Slugs.Count())
		{
			m_Cursor = 0;
			m_SubCursor = 0;
			m_Phase = PH_KEYS;
			return;
		}

		VPPXEBackupManifest mf = m_Mgr.GetManifest(m_Slugs[m_Cursor]);
		m_Lists.Insert(mf);
		int revision = 0;
		bool readable = false;
		XMLEditor xe = GetXMLEditor();
		if (mf && xe && xe.GetRegistry())
		{
			VPPXEFileEntry fileEntry = xe.GetRegistry().Find(mf.FileKey);
			if (fileEntry)
			{
				if ((fileEntry.Flags & VPPXEFileFlag.EXISTS) != 0 && (fileEntry.Flags & VPPXEFileFlag.READ_ERROR) == 0)
				{
					readable = true;
					revision = fileEntry.Revision;
				}
			}
		}

		m_Revisions.Insert(revision);
		m_Readable.Insert(readable);
		m_Cursor++;
	}

	// One record per call: key = Id + PadInt(manifestIdx, 4) + PadInt(recordIdx, 5).
	protected void RunKeys()
	{
		if (m_Cursor >= m_Lists.Count())
		{
			m_Phase = PH_SORT;
			return;
		}

		VPPXEBackupManifest mf = m_Lists[m_Cursor];
		if (!mf || m_SubCursor >= mf.Records.Count())
		{
			m_Cursor++;
			m_SubCursor = 0;
			return;
		}

		VPPXEBackupRecord rec = mf.Records[m_SubCursor];
		if (rec)
		{
			m_Keys.Insert(rec.Id + VPPXmlText.PadInt(m_Cursor, 4) + VPPXmlText.PadInt(m_SubCursor, 5));
			int size = rec.Size;
			if (size > 0)
			{
				if (m_TotalBytes > INT_MAX_VALUE - size)
				{
					m_TotalBytes = INT_MAX_VALUE;
				}
				else
				{
					m_TotalBytes += size;
				}
			}
		}

		m_SubCursor++;
	}

	// One entry per call, newest first (keys are sorted descending); chunks close at BACKUPS_PER_CHUNK or CHUNK_BYTES.
	protected void RunEntries()
	{
		if (m_Cursor >= m_Keys.Count())
		{
			if (m_Open)
			{
				m_Chunks.Insert(m_Open);
				m_Open = null;
			}

			if (m_Chunks.Count() == 0)
			{
				m_Chunks.Insert(NewChunk());
			}

			m_Cursor = 0;
			m_Phase = PH_SEND;
			return;
		}

		string key = m_Keys[m_Cursor];
		m_Cursor++;
		int manIdx = VPPXEBackupManager.KeyTail(key, 9, 4);
		int recIdx = VPPXEBackupManager.KeyTail(key, 5, 5);
		if (manIdx < 0 || manIdx >= m_Lists.Count())
		{
			return;
		}

		VPPXEBackupManifest mf = m_Lists[manIdx];
		if (!mf || recIdx < 0 || recIdx >= mf.Records.Count())
		{
			return;
		}

		VPPXEBackupRecord rec = mf.Records[recIdx];
		if (!rec)
		{
			return;
		}

		VPPXEBackupEntry entry = new VPPXEBackupEntry();
		entry.Id = VPPXEBackupManager.MakeBackupId(m_Slugs[manIdx], rec.Id);
		entry.FileKey = rec.FileKey;
		entry.Stamp = rec.Stamp;
		entry.AdminName = VPPXEBackupManager.ClipUtf8(rec.AdminName, VPPXEBackupManager.MAX_ADMIN_TEXT);
		entry.AdminId = VPPXEBackupManager.ClipUtf8(rec.AdminId, VPPXEBackupManager.MAX_ADMIN_TEXT);
		entry.Reason = rec.Reason;
		entry.Summary = VPPXEBackupManager.ClipUtf8(rec.Summary, VPPXEBackupManager.MAX_SUMMARY);
		entry.Note = VPPXEBackupManager.ClipUtf8(rec.Note, VPPXEConst.MAX_NOTE_LENGTH);
		entry.ChangeCount = rec.ChangeCount;
		entry.Size = rec.Size;
		entry.Hash = rec.Hash;
		entry.Pinned = rec.Pinned;
		entry.Unverified = rec.Unverified;
		entry.MatchesCurrent = false;
		if (!rec.Unverified && m_Readable[manIdx] && rec.Hash == m_Revisions[manIdx])
		{
			entry.MatchesCurrent = true;
		}

		AddEntry(entry);
	}

	protected void AddEntry(VPPXEBackupEntry entry)
	{
		int bytes = EstimateEntry(entry);
		if (m_Open && m_Open.Entries.Count() > 0)
		{
			if (m_Open.Entries.Count() >= VPPXEConst.BACKUPS_PER_CHUNK || m_OpenBytes + bytes > VPPXEConst.CHUNK_BYTES)
			{
				m_Chunks.Insert(m_Open);
				m_Open = null;
			}
		}

		if (!m_Open)
		{
			m_Open = NewChunk();
			m_OpenBytes = CHUNK_HEADER_BYTES;
		}

		m_Open.Entries.Insert(entry);
		m_OpenBytes += bytes;
	}

	protected VPPXEBackupListChunk NewChunk()
	{
		VPPXEBackupListChunk chunk = new VPPXEBackupListChunk();
		chunk.ReqId = m_ReqId;
		if (!chunk.Entries)
		{
			chunk.Entries = new array<ref VPPXEBackupEntry>();
		}

		return chunk;
	}

	// Payload estimate: each string = length + 4, each int/bool = 4, plus 4 for the element.
	protected int EstimateEntry(VPPXEBackupEntry entry)
	{
		int bytes = 4;
		bytes += entry.Id.Length() + 4;
		bytes += entry.FileKey.Length() + 4;
		bytes += entry.Stamp.Length() + 4;
		bytes += entry.AdminName.Length() + 4;
		bytes += entry.AdminId.Length() + 4;
		bytes += entry.Summary.Length() + 4;
		bytes += entry.Note.Length() + 4;
		bytes += 7 * 4;
		return bytes;
	}

	// One chunk per call; ChunkCount is known because every chunk was built first.
	protected void RunSend()
	{
		int total = m_Chunks.Count();
		if (m_Cursor >= total)
		{
			m_Phase = PH_DONE;
			return;
		}

		VPPXEBackupListChunk chunk = m_Chunks[m_Cursor];
		chunk.ReqId = m_ReqId;
		chunk.ChunkIdx = m_Cursor;
		chunk.ChunkCount = total;
		chunk.TotalBytes = m_TotalBytes;
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnBackupList");
		Param1<ref VPPXEBackupListChunk> netPayload = new Param1<ref VPPXEBackupListChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnBackupList", netRpc);
		m_Replied = true;
		m_Cursor++;
	}
};

class VPPXEDiffJob : VPPXEJob
{
	const static int PH_INIT = 0;
	const static int PH_PARSE_A = 1;
	const static int PH_PARSE_B = 2;
	const static int PH_MERGE_A = 3;
	const static int PH_MERGE_B = 4;
	const static int PH_UNION = 5;
	const static int PH_COMPARE = 6;
	const static int PH_SPLIT_A = 7;
	const static int PH_SPLIT_B = 8;
	const static int PH_PREFIX = 9;
	const static int PH_SUFFIX = 10;
	const static int PH_EMIT = 11;
	const static int PH_CHUNK = 12;
	const static int PH_SEND = 13;
	const static int PH_DONE = 14;
	const static int SPLIT_LINES_PER_CALL = 128;
	const static int COMPARE_LINES_PER_CALL = 256;
	const static int EMIT_ROWS_PER_CALL = 32;

	protected VPPXEBackupManager m_Mgr;
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_BackupId;
	protected string m_NextId;
	protected int m_Mode;
	protected int m_Phase;
	protected int m_Cursor;
	protected bool m_Replied;
	protected bool m_Released;
	protected string m_TextA;
	protected string m_TextB;
	protected ref VPPXELimits m_Limits;
	protected ref VPPXETypesParseJob m_ParseA;
	protected ref VPPXETypesParseJob m_ParseB;
	protected ref map<string, ref VPPXETypeRow> m_MergedA;
	protected ref map<string, ref VPPXETypeRow> m_MergedB;
	protected ref array<string> m_OrderA;
	protected ref array<string> m_OrderB;
	protected ref array<string> m_Union;
	protected ref VPPXmlSplitter m_SplitA;
	protected ref VPPXmlSplitter m_SplitB;
	protected int m_Prefix;
	protected int m_Suffix;
	protected int m_MidA;
	protected int m_MidB;
	protected ref array<ref VPPXEDiffRow> m_Rows;
	protected int m_Added;
	protected int m_Removed;
	protected int m_Changed;
	protected bool m_Truncated;
	protected bool m_Semantic;
	protected ref array<ref VPPXEDiffChunk> m_Chunks;
	protected ref VPPXEDiffChunk m_Open;
	protected int m_OpenBytes;

	void VPPXEDiffJob(VPPXEBackupManager mgr, PlayerIdentity sender, int reqId, string backupId, string nextId, int mode)
	{
		m_Mgr = mgr;
		m_Sender = sender;
		m_ReqId = reqId;
		m_BackupId = backupId;
		m_NextId = nextId;
		m_Mode = mode;
		m_Phase = PH_INIT;
		m_TextA = "";
		m_TextB = "";
		m_MergedA = new map<string, ref VPPXETypeRow>();
		m_MergedB = new map<string, ref VPPXETypeRow>();
		m_OrderA = new array<string>();
		m_OrderB = new array<string>();
		m_Union = new array<string>();
		m_Rows = new array<ref VPPXEDiffRow>();
		m_Chunks = new array<ref VPPXEDiffChunk>();
	}

	override string GetLabel()
	{
		return "BackupDiff";
	}

	override bool Step()
	{
		bool yieldNow = false;
		while (!yieldNow && m_Phase != PH_DONE && VPPXEJobQueue.BudgetOk())
		{
			yieldNow = RunPhase();
		}

		return m_Phase == PH_DONE;
	}

	override void OnFinished()
	{
		ReleaseLeases();
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
			m_Replied = true;
		}

		ReleaseLeases();
	}

	protected void ReleaseLeases()
	{
		if (m_Released)
		{
			return;
		}

		m_Released = true;
		if (m_Mgr)
		{
			m_Mgr.Release(m_BackupId);
			m_Mgr.Release(m_NextId);
		}
	}

	protected void Fail(string key)
	{
		VPPXENet.Result(m_Sender, m_ReqId, false, key, "");
		m_Replied = true;
		m_Phase = PH_DONE;
	}

	protected void Report(int percent)
	{
		VPPXENet.Progress(m_Sender, m_ReqId, VPPXEStage.DIFF, percent);
	}

	// Returns true when the phase must yield the rest of the frame (an inner parse job ran out of budget).
	protected bool RunPhase()
	{
		if (m_Phase == PH_INIT)
		{
			RunInit();
			return false;
		}

		if (m_Phase == PH_PARSE_A)
		{
			return RunParse(true);
		}

		if (m_Phase == PH_PARSE_B)
		{
			return RunParse(false);
		}

		if (m_Phase == PH_MERGE_A)
		{
			RunMerge(true);
			return false;
		}

		if (m_Phase == PH_MERGE_B)
		{
			RunMerge(false);
			return false;
		}

		if (m_Phase == PH_UNION)
		{
			RunUnion();
			return false;
		}

		if (m_Phase == PH_COMPARE)
		{
			RunCompare();
			return false;
		}

		if (m_Phase == PH_SPLIT_A)
		{
			RunSplit(true);
			return false;
		}

		if (m_Phase == PH_SPLIT_B)
		{
			RunSplit(false);
			return false;
		}

		if (m_Phase == PH_PREFIX)
		{
			RunPrefix();
			return false;
		}

		if (m_Phase == PH_SUFFIX)
		{
			RunSuffix();
			return false;
		}

		if (m_Phase == PH_EMIT)
		{
			RunEmit();
			return false;
		}

		if (m_Phase == PH_CHUNK)
		{
			RunChunk();
			return false;
		}

		RunSend();
		return false;
	}

	// A = the backup; B = the next newer (verified) backup for VS_NEXT, else the current file ("" when missing).
	protected void RunInit()
	{
		Report(0);
		string slugA;
		VPPXEBackupRecord recA = m_Mgr.ResolveRecord(m_BackupId, slugA);
		if (!recA)
		{
			Fail("#VSTR_XMLE_ERR_BACKUP_MISSING");
			return;
		}

		if (recA.Unverified)
		{
			Fail("#VSTR_XMLE_ERR_READ");
			return;
		}

		string textA;
		if (!VPPXmlText.ReadAll(VPPXEBackupManager.RecordPath(slugA, recA), textA))
		{
			Fail("#VSTR_XMLE_ERR_READ");
			return;
		}

		VPPXEFileEntry fileEntry = null;
		XMLEditor xe = GetXMLEditor();
		if (xe && xe.GetRegistry())
		{
			fileEntry = xe.GetRegistry().Find(recA.FileKey);
		}

		string textB = "";
		if (m_NextId != "")
		{
			string slugB;
			VPPXEBackupRecord recB = m_Mgr.ResolveRecord(m_NextId, slugB);
			if (!recB || recB.Unverified)
			{
				Fail("#VSTR_XMLE_ERR_BACKUP_MISSING");
				return;
			}

			if (!VPPXmlText.ReadAll(VPPXEBackupManager.RecordPath(slugB, recB), textB))
			{
				Fail("#VSTR_XMLE_ERR_READ");
				return;
			}
		}
		else
		{
			if (!fileEntry)
			{
				Fail("#VSTR_XMLE_ERR_FILE_UNKNOWN");
				return;
			}

			if (FileExist(fileEntry.Path))
			{
				if (!VPPXmlText.ReadAll(fileEntry.Path, textB))
				{
					Fail("#VSTR_XMLE_ERR_READ");
					return;
				}
			}
		}

		VPPXELog.Action(m_Sender, "Viewed the changes of backup " + m_BackupId, false);
		m_TextA = VPPXmlText.StripCR(textA);
		m_TextB = VPPXmlText.StripCR(textB);
		bool typesKind = false;
		if (fileEntry)
		{
			if (fileEntry.Kind == VPPXEFileKind.TYPES)
			{
				typesKind = true;
			}
		}

		if (!typesKind)
		{
			StartLineDiff();
			return;
		}

		if (xe && xe.GetTypes())
		{
			m_Limits = xe.GetTypes().GetLimits();
		}

		if (!m_Limits)
		{
			m_Limits = new VPPXELimits();
		}

		m_Semantic = true;
		m_ParseA = new VPPXETypesParseJob(m_TextA, 0, m_Limits);
		m_Phase = PH_PARSE_A;
		Report(5);
	}

	protected bool RunParse(bool sideA)
	{
		VPPXETypesParseJob parser = m_ParseB;
		if (sideA)
		{
			parser = m_ParseA;
		}

		if (!parser.Step())
		{
			return true;
		}

		if (parser.Failed())
		{
			StartLineDiff();
			return false;
		}

		if (sideA)
		{
			m_ParseB = new VPPXETypesParseJob(m_TextB, 1, m_Limits);
			m_Phase = PH_PARSE_B;
			Report(40);
			return false;
		}

		m_TextA = "";
		m_TextB = "";
		m_Cursor = 0;
		m_Phase = PH_MERGE_A;
		Report(75);
		return false;
	}

	// One row per call: occurrences merge per lowercase name in document order (VPPXETypeMerge).
	protected void RunMerge(bool sideA)
	{
		VPPXETypesParseJob parser = m_ParseB;
		map<string, ref VPPXETypeRow> merged = m_MergedB;
		array<string> order = m_OrderB;
		if (sideA)
		{
			parser = m_ParseA;
			merged = m_MergedA;
			order = m_OrderA;
		}

		array<ref VPPXETypeRow> rows = parser.GetRows();
		int rowCount = 0;
		if (rows)
		{
			rowCount = rows.Count();
		}

		if (m_Cursor >= rowCount)
		{
			m_Cursor = 0;
			if (sideA)
			{
				m_Phase = PH_MERGE_B;
			}
			else
			{
				m_ParseA = null;
				m_ParseB = null;
				m_Phase = PH_UNION;
			}

			return;
		}

		VPPXETypeRow row = rows[m_Cursor];
		m_Cursor++;
		if (!row)
		{
			return;
		}

		string low = row.Name;
		low.ToLower();
		VPPXETypeRow acc = merged.Get(low);
		if (!acc)
		{
			acc = VPPXETypeMerge.NewRow(row.Name);
			merged.Set(low, acc);
			order.Insert(low);
		}

		VPPXETypeMerge.MergeInto(acc, row, m_Limits);
	}

	// Union of both name lists (A's names, then B's names missing in A), then ONE native sort.
	protected void RunUnion()
	{
		int countA = m_OrderA.Count();
		int countB = m_OrderB.Count();
		if (m_Cursor < countA)
		{
			m_Union.Insert(m_OrderA[m_Cursor]);
			m_Cursor++;
			return;
		}

		int indexB = m_Cursor - countA;
		if (indexB < countB)
		{
			string nameB = m_OrderB[indexB];
			if (!m_MergedA.Contains(nameB))
			{
				m_Union.Insert(nameB);
			}

			m_Cursor++;
			return;
		}

		m_Union.Sort();
		m_Cursor = 0;
		m_Phase = PH_COMPARE;
	}

	// One name per call: only in B -> ADDED, only in A -> REMOVED, both -> one CHANGED row per differing field.
	protected void RunCompare()
	{
		int nameCount = m_Union.Count();
		if (m_Cursor >= nameCount)
		{
			BeginChunks();
			return;
		}

		string low = m_Union[m_Cursor];
		m_Cursor++;
		VPPXETypeRow rowA = m_MergedA.Get(low);
		VPPXETypeRow rowB = m_MergedB.Get(low);
		if (!rowA && rowB)
		{
			EmitRow(VPPXEDiffKind.ADDED, rowB.Name, "", "", "");
		}
		else if (rowA && !rowB)
		{
			EmitRow(VPPXEDiffKind.REMOVED, rowA.Name, "", "", "");
		}
		else if (rowA && rowB)
		{
			CompareRows(rowA, rowB);
		}

		int percent = 80 + (m_Cursor * 15) / nameCount;
		Report(percent);
	}

	protected void CompareRows(VPPXETypeRow rowA, VPPXETypeRow rowB)
	{
		for (int i = 0; i < VPPXEConst.FIELD_COUNT; i++)
		{
			int fieldBit = VPPXETypeMerge.FieldBitAt(i);
			string before = FieldText(rowA, fieldBit);
			string after = FieldText(rowB, fieldBit);
			if (before != after)
			{
				EmitRow(VPPXEDiffKind.CHANGED, rowB.Name, VPPXETypeMerge.FieldElementName(fieldBit), before, after);
			}
		}
	}

	// Data text of one field: numbers, "-" for unset, FormatFlags, the category name, names joined by comma (@group).
	protected string FieldText(VPPXETypeRow row, int fieldBit)
	{
		if ((row.Present & fieldBit) == 0)
		{
			return "-";
		}

		if (VPPXETypeMerge.IsScalarBit(fieldBit))
		{
			int scalar = VPPXETypeMerge.GetScalar(row, fieldBit);
			if (scalar == VPPXEConst.UNSET)
			{
				return "-";
			}

			return scalar.ToString();
		}

		if (fieldBit == VPPXEField.FLAGS)
		{
			string flagText = VPPXETypeMerge.FormatFlags(row.Flags);
			if (flagText == "")
			{
				return "0";
			}

			return flagText;
		}

		if (fieldBit == VPPXEField.CATEGORY)
		{
			return CategoryText(row.Category);
		}

		if (fieldBit == VPPXEField.USAGE)
		{
			return NamesText(m_Limits.Usages, row.Usage, m_Limits.UsageGroups, row.UsageUser);
		}

		if (fieldBit == VPPXEField.VALUE)
		{
			return NamesText(m_Limits.Values, row.Value, m_Limits.ValueGroups, row.ValueUser);
		}

		if (fieldBit == VPPXEField.TAG)
		{
			return NamesText(m_Limits.Tags, row.Tag, null, 0);
		}

		return "-";
	}

	protected string CategoryText(int category)
	{
		if (category == -2)
		{
			return "?";
		}

		if (category >= 0 && m_Limits.Categories)
		{
			if (category < m_Limits.Categories.Count())
			{
				return m_Limits.Categories[category];
			}
		}

		return "-";
	}

	protected string NamesText(array<string> names, int mask, array<string> groupNames, int groupMask)
	{
		array<string> found = new array<string>();
		if (names && mask != 0)
		{
			m_Limits.NamesOf(names, mask, found);
		}

		array<string> groups = new array<string>();
		if (groupNames && groupMask != 0)
		{
			m_Limits.NamesOf(groupNames, groupMask, groups);
		}

		string text = "";
		int foundCount = found.Count();
		for (int i = 0; i < foundCount; i++)
		{
			if (text != "")
			{
				text += ",";
			}

			text += found[i];
		}

		int groupCount = groups.Count();
		for (int j = 0; j < groupCount; j++)
		{
			if (text != "")
			{
				text += ",";
			}

			text += "@" + groups[j];
		}

		if (text == "")
		{
			return "-";
		}

		return text;
	}

	// Line diff (other kinds, or a types file that does not parse): CR-free splits, trimmed prefix and suffix.
	protected void StartLineDiff()
	{
		m_Semantic = false;
		m_ParseA = null;
		m_ParseB = null;
		m_SplitA = new VPPXmlSplitter();
		m_SplitA.Begin(m_TextA);
		m_SplitB = new VPPXmlSplitter();
		m_SplitB.Begin(m_TextB);
		m_TextA = "";
		m_TextB = "";
		m_Phase = PH_SPLIT_A;
		Report(20);
	}

	protected void RunSplit(bool sideA)
	{
		VPPXmlSplitter splitter = m_SplitB;
		if (sideA)
		{
			splitter = m_SplitA;
		}

		if (!splitter.Step(SPLIT_LINES_PER_CALL))
		{
			return;
		}

		if (sideA)
		{
			m_Phase = PH_SPLIT_B;
			Report(35);
			return;
		}

		m_Prefix = 0;
		m_Suffix = 0;
		m_Phase = PH_PREFIX;
		Report(50);
	}

	protected void RunPrefix()
	{
		array<string> linesA = m_SplitA.GetLines();
		array<string> linesB = m_SplitB.GetLines();
		int limit = linesA.Count();
		if (linesB.Count() < limit)
		{
			limit = linesB.Count();
		}

		int steps = 0;
		while (m_Prefix < limit && steps < COMPARE_LINES_PER_CALL)
		{
			if (linesA[m_Prefix] != linesB[m_Prefix])
			{
				m_Phase = PH_SUFFIX;
				return;
			}

			m_Prefix++;
			steps++;
		}

		if (m_Prefix >= limit)
		{
			m_Phase = PH_SUFFIX;
		}
	}

	protected void RunSuffix()
	{
		array<string> linesA = m_SplitA.GetLines();
		array<string> linesB = m_SplitB.GetLines();
		int countA = linesA.Count();
		int countB = linesB.Count();
		int limit = countA - m_Prefix;
		if (countB - m_Prefix < limit)
		{
			limit = countB - m_Prefix;
		}

		int steps = 0;
		while (m_Suffix < limit && steps < COMPARE_LINES_PER_CALL)
		{
			if (linesA[countA - 1 - m_Suffix] != linesB[countB - 1 - m_Suffix])
			{
				StartEmit();
				return;
			}

			m_Suffix++;
			steps++;
		}

		if (m_Suffix >= limit)
		{
			StartEmit();
		}
	}

	// INFO row (first differing line, last differing line of the longer side), then REMOVED and ADDED rows.
	protected void StartEmit()
	{
		array<string> linesA = m_SplitA.GetLines();
		array<string> linesB = m_SplitB.GetLines();
		m_MidA = linesA.Count() - m_Prefix - m_Suffix;
		m_MidB = linesB.Count() - m_Prefix - m_Suffix;
		if (m_MidA < 0)
		{
			m_MidA = 0;
		}

		if (m_MidB < 0)
		{
			m_MidB = 0;
		}

		if (m_MidA == 0 && m_MidB == 0)
		{
			BeginChunks();
			return;
		}

		m_Removed = m_MidA;
		m_Added = m_MidB;
		int longer = m_MidA;
		if (m_MidB > longer)
		{
			longer = m_MidB;
		}

		int firstLine = m_Prefix + 1;
		int lastLine = m_Prefix + longer;
		AppendRow(VPPXEDiffKind.INFO, "", "", firstLine.ToString(), lastLine.ToString());
		m_Cursor = 0;
		m_Phase = PH_EMIT;
		Report(70);
	}

	protected void RunEmit()
	{
		array<string> linesA = m_SplitA.GetLines();
		array<string> linesB = m_SplitB.GetLines();
		int steps = 0;
		while (steps < EMIT_ROWS_PER_CALL)
		{
			steps++;
			bool moreA = m_Cursor < m_MidA;
			int cursorB = m_Cursor - m_MidA;
			bool moreB = false;
			if (!moreA && cursorB < m_MidB)
			{
				moreB = true;
			}

			if (!moreA && !moreB)
			{
				BeginChunks();
				return;
			}

			if (m_Rows.Count() >= VPPXEConst.MAX_DIFF_ROWS)
			{
				m_Truncated = true;
				BeginChunks();
				return;
			}

			if (moreA)
			{
				int idxA = m_Prefix + m_Cursor;
				int lineNoA = idxA + 1;
				AppendRow(VPPXEDiffKind.REMOVED, "", lineNoA.ToString(), linesA[idxA], "");
			}
			else
			{
				int idxB = m_Prefix + cursorB;
				int lineNoB = idxB + 1;
				AppendRow(VPPXEDiffKind.ADDED, "", lineNoB.ToString(), "", linesB[idxB]);
			}

			m_Cursor++;
		}
	}

	// Semantic rows: counted always, stored up to MAX_DIFF_ROWS (then Truncated).
	protected void EmitRow(int kind, string entryText, string fieldText, string beforeText, string afterText)
	{
		if (kind == VPPXEDiffKind.ADDED)
		{
			m_Added++;
		}
		else if (kind == VPPXEDiffKind.REMOVED)
		{
			m_Removed++;
		}
		else if (kind == VPPXEDiffKind.CHANGED)
		{
			m_Changed++;
		}

		if (m_Rows.Count() >= VPPXEConst.MAX_DIFF_ROWS)
		{
			m_Truncated = true;
			return;
		}

		AppendRow(kind, entryText, fieldText, beforeText, afterText);
	}

	// Every cell copied from a file is cut with Clip(text, MAX_CELL_CHARS).
	protected void AppendRow(int kind, string entryText, string fieldText, string beforeText, string afterText)
	{
		VPPXEDiffRow row = new VPPXEDiffRow();
		row.Kind = kind;
		row.Entry = VPPXmlText.Clip(entryText, VPPXEConst.MAX_CELL_CHARS);
		row.Field = VPPXmlText.Clip(fieldText, VPPXEConst.MAX_CELL_CHARS);
		row.Before = VPPXmlText.Clip(beforeText, VPPXEConst.MAX_CELL_CHARS);
		row.After = VPPXmlText.Clip(afterText, VPPXEConst.MAX_CELL_CHARS);
		m_Rows.Insert(row);
	}

	protected void BeginChunks()
	{
		m_MergedA = null;
		m_MergedB = null;
		m_OrderA = null;
		m_OrderB = null;
		m_Union = null;
		m_SplitA = null;
		m_SplitB = null;
		m_Cursor = 0;
		m_Phase = PH_CHUNK;
	}

	// One row per call; chunks close at DIFF_ROWS_PER_CHUNK rows or CHUNK_BYTES (a chunk always holds a row).
	protected void RunChunk()
	{
		int rowCount = m_Rows.Count();
		if (m_Cursor >= rowCount)
		{
			if (m_Open)
			{
				m_Chunks.Insert(m_Open);
				m_Open = null;
			}

			if (m_Chunks.Count() == 0)
			{
				m_Chunks.Insert(NewChunk());
			}

			m_Cursor = 0;
			m_Phase = PH_SEND;
			Report(100);
			return;
		}

		VPPXEDiffRow row = m_Rows[m_Cursor];
		m_Cursor++;
		int bytes = EstimateRow(row);
		if (m_Open && m_Open.Rows.Count() > 0)
		{
			if (m_Open.Rows.Count() >= VPPXEConst.DIFF_ROWS_PER_CHUNK || m_OpenBytes + bytes > VPPXEConst.CHUNK_BYTES)
			{
				m_Chunks.Insert(m_Open);
				m_Open = null;
			}
		}

		if (!m_Open)
		{
			m_Open = NewChunk();
			m_OpenBytes = HeaderBytes();
		}

		m_Open.Rows.Insert(row);
		m_OpenBytes += bytes;
	}

	protected VPPXEDiffChunk NewChunk()
	{
		VPPXEDiffChunk chunk = new VPPXEDiffChunk();
		chunk.ReqId = m_ReqId;
		chunk.BackupId = m_BackupId;
		chunk.Mode = m_Mode;
		chunk.Added = m_Added;
		chunk.Removed = m_Removed;
		chunk.Changed = m_Changed;
		chunk.Truncated = m_Truncated;
		chunk.Semantic = m_Semantic;
		if (!chunk.Rows)
		{
			chunk.Rows = new array<ref VPPXEDiffRow>();
		}

		return chunk;
	}

	protected int HeaderBytes()
	{
		return 9 * 4 + m_BackupId.Length() + 4 + 4 + 4;
	}

	protected int EstimateRow(VPPXEDiffRow row)
	{
		int bytes = 4 + 4;
		bytes += row.Entry.Length() + 4;
		bytes += row.Field.Length() + 4;
		bytes += row.Before.Length() + 4;
		bytes += row.After.Length() + 4;
		return bytes;
	}

	protected void RunSend()
	{
		int total = m_Chunks.Count();
		if (m_Cursor >= total)
		{
			m_Phase = PH_DONE;
			return;
		}

		VPPXEDiffChunk chunk = m_Chunks[m_Cursor];
		chunk.ChunkIdx = m_Cursor;
		chunk.ChunkCount = total;
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnBackupDiff");
		Param1<ref VPPXEDiffChunk> netPayload = new Param1<ref VPPXEDiffChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnBackupDiff", netRpc);
		m_Replied = true;
		m_Cursor++;
	}
};

class VPPXERestoreJob : VPPXEJob
{
	const static int PH_CHECK = 0;
	const static int PH_PREBACKUP = 1;
	const static int PH_WRITE = 2;
	const static int PH_FINISH = 3;
	const static int PH_DONE = 4;

	protected VPPXEBackupManager m_Mgr;
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_BackupId;
	protected string m_FileKey;
	protected string m_RecordId;
	protected string m_Content;
	protected int m_ContentHash;
	protected string m_Current;
	protected bool m_HasRollback;
	protected string m_PreId;
	protected bool m_WriteOk;
	protected string m_ErrKey;
	protected string m_ErrArg;
	protected string m_NoticeKey;
	protected int m_Phase;
	protected bool m_Replied;
	protected bool m_Released;

	void VPPXERestoreJob(VPPXEBackupManager mgr, PlayerIdentity sender, int reqId, string backupId)
	{
		m_Mgr = mgr;
		m_Sender = sender;
		m_ReqId = reqId;
		m_BackupId = backupId;
		m_FileKey = "";
		m_RecordId = "";
		m_Content = "";
		m_Current = "";
		m_PreId = "";
		m_ErrKey = "";
		m_ErrArg = "";
		m_NoticeKey = "";
		m_Phase = PH_CHECK;
	}

	override string GetLabel()
	{
		return "Restore";
	}

	override bool Step()
	{
		while (m_Phase != PH_DONE && VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_CHECK)
			{
				RunCheck();
			}
			else if (m_Phase == PH_PREBACKUP)
			{
				RunPreBackup();
			}
			else if (m_Phase == PH_WRITE)
			{
				RunWrite();
			}
			else
			{
				RunFinish();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnFinished()
	{
		ReleaseLease();
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
			m_Replied = true;
		}

		ReleaseLease();
		if (m_FileKey != "")
		{
			VPPXESafeWrite.CheckJournalFor(m_FileKey);
		}
	}

	protected void ReleaseLease()
	{
		if (m_Released)
		{
			return;
		}

		m_Released = true;
		if (m_Mgr)
		{
			m_Mgr.Release(m_BackupId);
		}
	}

	protected void Fail(string key)
	{
		VPPXENet.Result(m_Sender, m_ReqId, false, key, "");
		m_Replied = true;
		VPPXELog.Action(m_Sender, "Restore of backup " + m_BackupId + " refused: " + key, false);
		m_Phase = PH_DONE;
	}

	protected VPPXEFileEntry LookupFile()
	{
		XMLEditor xe = GetXMLEditor();
		if (!xe || !xe.GetRegistry() || m_FileKey == "")
		{
			return null;
		}

		return xe.GetRegistry().Find(m_FileKey);
	}

	// Resolve, registered file, not Unverified, ReadAll the backup and check its normalized hash against the record.
	protected void RunCheck()
	{
		string slug;
		VPPXEBackupRecord rec = m_Mgr.ResolveRecord(m_BackupId, slug);
		if (!rec)
		{
			Fail("#VSTR_XMLE_ERR_BACKUP_MISSING");
			return;
		}

		m_FileKey = rec.FileKey;
		m_RecordId = rec.Id;
		if (!LookupFile())
		{
			Fail("#VSTR_XMLE_ERR_FILE_UNKNOWN");
			return;
		}

		if (rec.Unverified)
		{
			Fail("#VSTR_XMLE_ERR_READ");
			return;
		}

		string text;
		if (!VPPXmlText.ReadAll(VPPXEBackupManager.RecordPath(slug, rec), text))
		{
			Fail("#VSTR_XMLE_ERR_BACKUP_MISSING");
			return;
		}

		int textHash = VPPXmlText.NormalizedHash(text);
		if (textHash != rec.Hash)
		{
			Fail("#VSTR_XMLE_ERR_READ");
			return;
		}

		m_Content = text;
		m_ContentHash = textHash;
		m_Phase = PH_PREBACKUP;
	}

	// The current state decides the pre-restore backup: missing -> CreateBackup(""); readable -> CreateBackup(content);
	// exists but unreadable -> CreateRawBackup (pinned, Unverified, never overwritten) and no rollback.
	// On success the write runs in the same call (RunWrite), so CreateBackup's SourceStillMatches check and
	// VPPXESafeWrite.Write run back to back and no external edit can slip in between unbacked. PH_WRITE stays
	// in Step only so a job resumed in that phase still completes.
	protected void RunPreBackup()
	{
		VPPXEFileEntry fileEntry = LookupFile();
		if (!fileEntry)
		{
			Fail("#VSTR_XMLE_ERR_FILE_UNKNOWN");
			return;
		}

		string summary = "restore " + m_RecordId;
		if (!FileExist(fileEntry.Path))
		{
			m_Current = "";
			m_HasRollback = true;
			m_PreId = m_Mgr.CreateBackup(fileEntry, "", VPPXEBackupReason.RESTORE_PRE, m_Sender, summary, 0, null, "");
		}
		else
		{
			string current;
			if (VPPXmlText.ReadAll(fileEntry.Path, current))
			{
				GetXMLEditor().GetRegistry().NoteRevision(m_FileKey, VPPXmlText.NormalizedHash(current), true);
				m_Current = current;
				m_HasRollback = true;
				m_PreId = m_Mgr.CreateBackup(fileEntry, current, VPPXEBackupReason.RESTORE_PRE, m_Sender, summary, 0, null, "");
			}
			else
			{
				GetXMLEditor().GetRegistry().NoteRevision(m_FileKey, 0, false);
				m_Current = "";
				m_HasRollback = false;
				m_PreId = m_Mgr.CreateRawBackup(fileEntry, VPPXEBackupReason.RESTORE_PRE, m_Sender, summary, "");
			}
		}

		if (m_PreId == "")
		{
			Fail("#VSTR_XMLE_ERR_BACKUP");
			return;
		}

		m_Phase = PH_WRITE;
		RunWrite();
	}

	protected void RunWrite()
	{
		VPPXEFileEntry fileEntry = LookupFile();
		if (!fileEntry)
		{
			Fail("#VSTR_XMLE_ERR_FILE_UNKNOWN");
			return;
		}

		string errKey = "";
		string errArg = "";
		string noticeKey = "";
		m_WriteOk = VPPXESafeWrite.Write(fileEntry, m_Content, m_PreId, m_Current, m_HasRollback, errKey, errArg, noticeKey);
		m_ErrKey = errKey;
		m_ErrArg = errArg;
		m_NoticeKey = noticeKey;
		m_Current = "";
		m_Phase = PH_FINISH;
	}

	protected void RunFinish()
	{
		if (m_WriteOk)
		{
			m_Mgr.SetResultHash(m_PreId, m_ContentHash);
		}

		m_Mgr.ApplyRetention();
		VPPXESaveResult result = new VPPXESaveResult();
		result.ReqId = m_ReqId;
		result.Ok = m_WriteOk;
		result.ErrorKey = m_ErrKey;
		result.ErrorArg = m_ErrArg;
		result.FileKey = m_FileKey;
		result.NewRevision = 0;
		result.Applied = 0;
		if (m_WriteOk)
		{
			result.NewRevision = m_ContentHash;
			result.Applied = 1;
		}

		result.BackupId = m_PreId;
		if (!result.TouchedFiles)
		{
			result.TouchedFiles = new array<string>();
		}

		result.TouchedFiles.Insert(m_FileKey);
		result.NoticeKey = m_NoticeKey;
		VPPXENet.SendNow(m_Sender, "XE_OnSaveResult", new Param1<ref VPPXESaveResult>(result));
		m_Replied = true;
		if (m_WriteOk)
		{
			VPPXELog.Action(m_Sender, "Restored " + m_FileKey + " from " + m_BackupId + " (pre-restore backup " + m_PreId + ")", true);
		}
		else
		{
			VPPXELog.Action(m_Sender, "Restore of " + m_FileKey + " from " + m_BackupId + " failed: " + m_ErrKey + " " + m_ErrArg + " (pre-restore backup " + m_PreId + ")", false);
		}

		m_Content = "";
		GetXMLEditor().OnFileWritten(m_FileKey, m_Sender);
		ReleaseLease();
		m_Phase = PH_DONE;
	}
};

class VPPXEBackupOpJob : VPPXEJob
{
	const static int OP_DELETE = 0;
	const static int OP_PIN = 1;
	const static int OP_SNAPSHOT = 2;
	const static int PH_WORK = 0;
	const static int PH_PURGE = 1;
	const static int PH_RETAIN = 2;
	const static int PH_REPLY = 3;
	const static int PH_DONE = 4;

	protected VPPXEBackupManager m_Mgr;
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected int m_Op;
	protected ref array<string> m_Ids;
	protected string m_BackupId;
	protected bool m_Pinned;
	protected string m_FileKey;
	protected string m_Note;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_Deleted;
	protected int m_Skipped;
	protected bool m_Replied;
	protected string m_NewId;
	protected ref map<string, bool> m_Doomed;
	protected ref map<string, bool> m_SlugSeen;
	protected ref array<string> m_DoomedSlugs;

	void VPPXEBackupOpJob(VPPXEBackupManager mgr, PlayerIdentity sender, int reqId, int op, array<string> ids, string backupId, bool pinned, string fileKey, string note)
	{
		m_Mgr = mgr;
		m_Sender = sender;
		m_ReqId = reqId;
		m_Op = op;
		m_Ids = ids;
		if (!m_Ids)
		{
			m_Ids = new array<string>();
		}

		m_BackupId = backupId;
		m_Pinned = pinned;
		m_FileKey = fileKey;
		m_Note = note;
		m_NewId = "";
		m_Phase = PH_WORK;
		m_Doomed = new map<string, bool>();
		m_SlugSeen = new map<string, bool>();
		m_DoomedSlugs = new array<string>();
	}

	override string GetLabel()
	{
		if (m_Op == OP_DELETE)
		{
			return "BackupDelete";
		}

		if (m_Op == OP_PIN)
		{
			return "BackupPin";
		}

		return "BackupSnapshot";
	}

	override bool Step()
	{
		while (m_Phase != PH_DONE && VPPXEJobQueue.BudgetOk())
		{
			if (m_Op == OP_DELETE)
			{
				RunDelete();
			}
			else if (m_Op == OP_PIN)
			{
				RunPin();
			}
			else
			{
				RunSnapshot();
			}
		}

		return m_Phase == PH_DONE;
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
			m_Replied = true;
		}
	}

	protected void Reply(bool ok, string key, string arg)
	{
		VPPXENet.Result(m_Sender, m_ReqId, ok, key, arg);
		m_Replied = true;
	}

	protected void Fail(string key, string arg)
	{
		Reply(false, key, arg);
		m_Phase = PH_DONE;
	}

	// Delete: resolve each id (one per call), skip (and count) pinned, leased or journaled records; then purge
	// per manifest (DeleteFile on the record's own path, save); reply the number removed.
	protected void RunDelete()
	{
		if (m_Phase == PH_WORK)
		{
			if (m_Cursor >= m_Ids.Count())
			{
				m_Cursor = 0;
				m_Phase = PH_PURGE;
				return;
			}

			MarkForDelete(m_Ids[m_Cursor]);
			m_Cursor++;
			return;
		}

		if (m_Phase == PH_PURGE)
		{
			if (m_Cursor >= m_DoomedSlugs.Count())
			{
				m_Phase = PH_REPLY;
				return;
			}

			m_Deleted += m_Mgr.PurgeDoomed(m_DoomedSlugs[m_Cursor], m_Doomed);
			m_Cursor++;
			return;
		}

		Reply(true, "#VSTR_XMLE_BK_DELETED", m_Deleted.ToString());
		string text = "Deleted " + m_Deleted.ToString() + " backup(s)";
		if (m_Skipped > 0)
		{
			text += "; kept " + m_Skipped.ToString() + " pinned, in use or missing";
		}

		VPPXELog.Action(m_Sender, text, true);
		m_Phase = PH_DONE;
	}

	protected void MarkForDelete(string backupId)
	{
		string slug;
		VPPXEBackupRecord rec = m_Mgr.ResolveRecord(backupId, slug);
		if (!rec)
		{
			m_Skipped++;
			return;
		}

		string canonical = VPPXEBackupManager.MakeBackupId(slug, rec.Id);
		if (m_Doomed.Contains(canonical))
		{
			return;
		}

		if (rec.Pinned || m_Mgr.IsLeased(canonical) || VPPXESafeWrite.IsJournaled(canonical))
		{
			m_Skipped++;
			return;
		}

		m_Doomed.Set(canonical, true);
		if (!m_SlugSeen.Contains(slug))
		{
			m_SlugSeen.Set(slug, true);
			m_DoomedSlugs.Insert(slug);
		}
	}

	// Pin/unpin; an Unverified record always stays pinned.
	protected void RunPin()
	{
		string slug;
		VPPXEBackupRecord rec = m_Mgr.ResolveRecord(m_BackupId, slug);
		if (!rec)
		{
			Fail("#VSTR_XMLE_ERR_BACKUP_MISSING", "");
			return;
		}

		if (!m_Pinned && rec.Unverified)
		{
			Fail("#VSTR_XMLE_ERR_VALIDATION", rec.Id);
			return;
		}

		bool previous = rec.Pinned;
		rec.Pinned = m_Pinned;
		if (!m_Mgr.SaveManifest(slug))
		{
			rec.Pinned = previous;
			Fail("#VSTR_XMLE_ERR_WRITE", "");
			return;
		}

		string canonical = VPPXEBackupManager.MakeBackupId(slug, rec.Id);
		if (m_Pinned)
		{
			Reply(true, "#VSTR_XMLE_BK_PINNED", canonical);
			VPPXELog.Action(m_Sender, "Pinned backup " + canonical, true);
		}
		else
		{
			Reply(true, "#VSTR_XMLE_BK_UNPINNED", canonical);
			VPPXELog.Action(m_Sender, "Unpinned backup " + canonical, true);
		}

		m_Phase = PH_DONE;
	}

	// Snapshot: registered, existing and readable file; CreateBackup MANUAL with the note; retention; reply.
	protected void RunSnapshot()
	{
		if (m_Phase == PH_WORK)
		{
			VPPXEFileEntry fileEntry = null;
			XMLEditor xe = GetXMLEditor();
			if (xe && xe.GetRegistry())
			{
				fileEntry = xe.GetRegistry().Find(m_FileKey);
			}

			if (!fileEntry)
			{
				Fail("#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
				return;
			}

			if (!FileExist(fileEntry.Path))
			{
				Fail("#VSTR_XMLE_ERR_READ", "");
				return;
			}

			string content;
			if (!VPPXmlText.ReadAll(fileEntry.Path, content))
			{
				Fail("#VSTR_XMLE_ERR_READ", "");
				return;
			}

			m_NewId = m_Mgr.CreateBackup(fileEntry, content, VPPXEBackupReason.MANUAL, m_Sender, "", 0, null, m_Note);
			if (m_NewId == "")
			{
				Fail("#VSTR_XMLE_ERR_BACKUP", "");
				return;
			}

			m_Phase = PH_RETAIN;
			return;
		}

		if (m_Phase == PH_RETAIN)
		{
			m_Mgr.ApplyRetention();
			m_Phase = PH_REPLY;
			return;
		}

		Reply(true, "#VSTR_XMLE_BK_CREATED", m_NewId);
		string text = "Created snapshot " + m_NewId + " of " + m_FileKey;
		if (m_Note != "")
		{
			string noteText = VPPXEBackupManager.ClipUtf8(m_Note, VPPXEConst.MAX_NOTE_LENGTH);
			text += " (note: " + noteText + ")";
		}

		VPPXELog.Action(m_Sender, text, true);
		m_Phase = PH_DONE;
	}
};
