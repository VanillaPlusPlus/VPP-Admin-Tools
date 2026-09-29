// XML Editor types service: limits and ignore list, the budgeted index job (one job at a time, coalesced),
// merged effective rows, per-file revisions, def-file lookup, details data and the types lint.

class VPPXETypesFileData : Managed
{
	string Key;
	int FileIdx;
	int FileRevision;
	int LimitsSig;
	bool Indexed;
	bool ReadError;
	bool ParseError;
	int IssueCount;
	ref array<ref VPPXETypeRow> Rows;
	ref array<int> StartLines;
	ref array<int> EndLines;
	ref array<ref array<string>> UnknownRefs;
	ref array<string> UnknownSigs;
	ref array<int> BadNums;
	ref array<int> DupLines;
	ref array<string> OverArgs;
	ref array<bool> LastDef;
	ref array<int> WireBits;
	ref array<ref VPPXEIssue> FileIssues;
	ref array<string> Lines;

	void VPPXETypesFileData(string key)
	{
		Key = key;
		FileIdx = -1;
		ClearRows();
	}

	void ClearRows()
	{
		Rows = new array<ref VPPXETypeRow>();
		StartLines = new array<int>();
		EndLines = new array<int>();
		UnknownRefs = new array<ref array<string>>();
		UnknownSigs = new array<string>();
		BadNums = new array<int>();
		DupLines = new array<int>();
		OverArgs = new array<string>();
		LastDef = new array<bool>();
		WireBits = new array<int>();
		FileIssues = new array<ref VPPXEIssue>();
		Lines = new array<string>();
	}
};

// Thin queue wrapper: the service holds the index state (only one index job exists at a time).
class VPPXETypesIndexJob : VPPXEJob
{
	protected VPPXETypesService m_Owner;

	void VPPXETypesIndexJob(VPPXETypesService owner)
	{
		m_Owner = owner;
	}

	override bool Step()
	{
		if (!m_Owner)
		{
			return true;
		}

		return m_Owner.RunIndexStep(this);
	}

	override void OnFinished()
	{
		if (m_Owner)
		{
			m_Owner.OnIndexJobEnded(this, true);
		}
	}

	override void OnAborted()
	{
		if (m_Owner)
		{
			m_Owner.OnIndexJobEnded(this, false);
		}
	}

	override string GetLabel()
	{
		return "TypesIndex";
	}
};

class VPPXETypesService : Managed
{
	const static int IX_BEGIN = 0;
	const static int IX_FILES = 1;
	const static int IX_RESET = 2;
	const static int IX_NAMES = 3;
	const static int IX_SWAP = 4;
	const static int IX_COUNT = 5;
	const static int IX_END = 6;
	const static int IX_DONE = 7;
	const static int MAX_DETAIL_ISSUES = 30;
	const static int MAX_DETAIL_REFS = 50;
	const static int MAX_RAW_LINES = 60;

	protected ref VPPXELimits m_Limits;
	protected int m_LimitsSig;
	protected bool m_LimitsLoaded;
	protected ref array<ref VPPXEIssue> m_LimitIssues;
	protected ref array<ref VPPXEIssue> m_UnusedIssues;
	protected ref map<string, int> m_LimitCounts;
	protected string m_LimitsKey;
	protected ref map<string, bool> m_Ignore;
	protected int m_IgnoreRev;
	protected bool m_IgnoreLoaded;
	// Registry revisions of the limits, limits-user and ignore files when the cached limits/ignore list were loaded.
	protected int m_SrcRev;

	protected ref array<ref VPPXETypesFileData> m_Files;
	protected ref map<string, ref VPPXETypesFileData> m_FileByKey;
	protected ref map<string, ref array<int>> m_NameMap;
	protected ref map<string, ref VPPXETypeRow> m_Effective;
	protected ref map<string, int> m_EffBits;
	protected ref map<string, bool> m_Dirty;
	protected bool m_Ready;
	protected int m_Revision;
	protected bool m_LastJobFailed;

	// Index job state.
	protected VPPXETypesIndexJob m_Job;
	protected int m_JobLane;
	protected int m_IxPhase;
	protected string m_IxKey;
	protected int m_IxRevision;
	protected ref VPPXETypesParseJob m_IxParse;
	protected bool m_IxChanged;
	protected int m_IxTotal;
	protected int m_IxDone;
	protected int m_IxFile;
	protected int m_IxRow;
	protected int m_IxName;
	protected int m_IxCount;
	protected int m_IxStarted;
	protected int m_IxCatUsed;
	protected int m_IxTagUsed;
	protected int m_IxUsageUsed;
	protected int m_IxValueUsed;
	protected ref map<string, ref array<int>> m_IxNameMap;
	protected ref map<string, ref VPPXETypeRow> m_IxEff;
	protected ref map<string, int> m_IxEffBits;

	void VPPXETypesService()
	{
		m_LimitIssues = new array<ref VPPXEIssue>();
		m_UnusedIssues = new array<ref VPPXEIssue>();
		m_LimitCounts = new map<string, int>();
		m_Ignore = new map<string, bool>();
		m_Files = new array<ref VPPXETypesFileData>();
		m_FileByKey = new map<string, ref VPPXETypesFileData>();
		m_NameMap = new map<string, ref array<int>>();
		m_Effective = new map<string, ref VPPXETypeRow>();
		m_EffBits = new map<string, int>();
		m_Dirty = new map<string, bool>();
	}

	protected VPPXEFileRegistry Registry()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor)
		{
			return null;
		}

		return editor.GetRegistry();
	}

	bool IsReady()
	{
		return m_Ready;
	}

	int GetRevision()
	{
		return m_Revision;
	}

	bool IsIndexBusy()
	{
		if (m_Job)
		{
			return true;
		}

		return false;
	}

	// True once after an index job aborted (read by XMLEditor.OnTypesReindexed).
	bool ConsumeJobFailed()
	{
		bool failed = m_LastJobFailed;
		m_LastJobFailed = false;
		return failed;
	}

	// ---------------------------------------------------------------- scheduling

	// INTERACTIVE: an admin request waits for the index.
	void EnsureIndex()
	{
		if (m_Ready && !m_Job && !SourcesChanged())
		{
			return;
		}

		if (!m_Ready)
		{
			MarkAllDirty();
		}

		StartJob(VPPXELane.INTERACTIVE);
	}

	// BACKGROUND prewarm at boot.
	void Prewarm()
	{
		if (m_Ready || m_Job)
		{
			return;
		}

		MarkAllDirty();
		StartJob(VPPXELane.BACKGROUND);
	}

	// INTERACTIVE and coalesced: the key joins the running or queued index job.
	void ReindexFile(string key)
	{
		if (key == "")
		{
			return;
		}

		m_Dirty.Set(key, true);
		StartJob(VPPXELane.INTERACTIVE);
	}

	protected void MarkAllDirty()
	{
		VPPXEFileRegistry reg = Registry();
		if (!reg)
		{
			return;
		}

		array<VPPXEFileEntry> entries = new array<VPPXEFileEntry>();
		reg.GetByKind(VPPXEFileKind.TYPES, entries);
		foreach (VPPXEFileEntry entry : entries)
		{
			m_Dirty.Set(entry.Key, true);
		}
	}

	protected void StartJob(int lane)
	{
		if (m_Job)
		{
			if (lane == VPPXELane.INTERACTIVE && m_JobLane != VPPXELane.INTERACTIVE)
			{
				PromoteJob();
			}

			return;
		}

		VPPXETypesIndexJob job = new VPPXETypesIndexJob(this);
		m_Job = job;
		m_JobLane = lane;
		ResetIxState();
		VPPXEJobQueue.Get().Enqueue(job, lane);
	}

	protected void PromoteJob()
	{
		VPPXETypesIndexJob job = m_Job;
		if (!job)
		{
			return;
		}

		VPPXEJobQueue queue = VPPXEJobQueue.Get();
		queue.Cancel(job);
		queue.Enqueue(job, VPPXELane.INTERACTIVE);
		m_JobLane = VPPXELane.INTERACTIVE;
	}

	protected void ResetIxState()
	{
		m_IxPhase = IX_BEGIN;
		m_IxKey = "";
		m_IxRevision = 0;
		m_IxParse = null;
		m_IxChanged = false;
		m_IxTotal = 0;
		m_IxDone = 0;
		m_IxFile = 0;
		m_IxRow = 0;
		m_IxName = 0;
		m_IxCount = 0;
		m_IxNameMap = null;
		m_IxEff = null;
		m_IxEffBits = null;
	}

	void OnIndexJobEnded(VPPXETypesIndexJob job, bool ok)
	{
		if (m_Job && m_Job != job)
		{
			return;
		}

		m_Job = null;
		m_IxParse = null;
		if (!ok)
		{
			m_LastJobFailed = true;
			m_Ready = false;
			ResetIxState();
			VPPXELog.Warn("The types index job aborted; the next request rebuilds the index");
		}
		else
		{
			int rowCount = 0;
			foreach (VPPXETypesFileData fd : m_Files)
			{
				rowCount += fd.Rows.Count();
			}

			VPPXELog.Info(string.Format("Types index ready: %1 files, %2 rows, %3 names, %4 ms", m_Files.Count(), rowCount, m_Effective.Count(), GetGame().GetTime() - m_IxStarted));
		}

		XMLEditor editor = GetXMLEditor();
		if (editor)
		{
			editor.OnTypesReindexed();
		}
	}

	int GetProgressPct()
	{
		if (!m_Job)
		{
			return 100;
		}

		if (m_IxPhase <= IX_FILES)
		{
			int total = m_IxTotal;
			if (total < 1)
			{
				total = 1;
			}

			int done = m_IxDone;
			if (done > total)
			{
				done = total;
			}

			return 5 + (80 * done) / total;
		}

		int files = m_Files.Count();
		if (files < 1)
		{
			files = 1;
		}

		int fileCursor = m_IxFile;
		if (fileCursor > files)
		{
			fileCursor = files;
		}

		return 85 + (14 * fileCursor) / files;
	}

	// ---------------------------------------------------------------- index job body

	bool RunIndexStep(VPPXETypesIndexJob job)
	{
		if (job != m_Job)
		{
			return true;
		}

		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_IxPhase == IX_BEGIN)
			{
				IndexBegin();
				continue;
			}

			if (m_IxPhase == IX_FILES)
			{
				IndexFilesStep();
				continue;
			}

			if (m_IxPhase == IX_RESET)
			{
				IndexResetStep();
				continue;
			}

			if (m_IxPhase == IX_NAMES)
			{
				IndexNamesStep();
				continue;
			}

			if (m_IxPhase == IX_SWAP)
			{
				IndexSwap();
				continue;
			}

			if (m_IxPhase == IX_COUNT)
			{
				IndexCountStep();
				continue;
			}

			if (m_IxPhase == IX_END)
			{
				IndexEnd();
				continue;
			}

			return true;
		}

		NotifyProgress();
		return false;
	}

	protected void NotifyProgress()
	{
		XMLEditor editor = GetXMLEditor();
		if (editor)
		{
			editor.NotifyTypesProgress(GetProgressPct());
		}
	}

	protected void IndexBegin()
	{
		m_IxStarted = GetGame().GetTime();
		m_SrcRev = SourcesRev();
		LoadLimits();
		bool forceFinal = !m_Ready;
		int ignoreRev = LoadIgnore();
		if (ignoreRev != m_IgnoreRev)
		{
			m_IgnoreRev = ignoreRev;
			forceFinal = true;
		}

		if (SyncFileList())
		{
			forceFinal = true;
		}

		foreach (VPPXETypesFileData fd : m_Files)
		{
			if (fd.Indexed && fd.LimitsSig != m_LimitsSig)
			{
				m_Dirty.Set(fd.Key, true);
			}
		}

		m_IxChanged = forceFinal;
		m_IxTotal = m_Dirty.Count();
		m_IxDone = 0;
		m_IxPhase = IX_FILES;
	}

	// Rebuilds m_Files in registry load order. Returns true when the list or its order changed.
	protected bool SyncFileList()
	{
		VPPXEFileRegistry reg = Registry();
		if (!reg)
		{
			return false;
		}

		array<VPPXEFileEntry> entries = new array<VPPXEFileEntry>();
		reg.GetByKind(VPPXEFileKind.TYPES, entries);
		array<ref VPPXETypesFileData> list = new array<ref VPPXETypesFileData>();
		map<string, ref VPPXETypesFileData> byKey = new map<string, ref VPPXETypesFileData>();
		bool changed = entries.Count() != m_Files.Count();
		int count = entries.Count();
		for (int i = 0; i < count; i++)
		{
			VPPXEFileEntry entry = entries.Get(i);
			VPPXETypesFileData fd = m_FileByKey.Get(entry.Key);
			if (!fd)
			{
				fd = new VPPXETypesFileData(entry.Key);
				m_Dirty.Set(entry.Key, true);
			}

			int regIdx = reg.IndexOf(entry.Key);
			if (fd.FileIdx != regIdx)
			{
				fd.FileIdx = regIdx;
				m_Dirty.Set(entry.Key, true);
			}

			if (!changed && m_Files.Get(i).Key != entry.Key)
			{
				changed = true;
			}

			list.Insert(fd);
			byKey.Set(entry.Key, fd);
		}

		m_Files = list;
		m_FileByKey = byKey;
		return changed;
	}

	protected string PopDirty()
	{
		while (m_Dirty.Count() > 0)
		{
			string key = m_Dirty.GetKey(0);
			m_Dirty.Remove(key);
			if (m_FileByKey.Contains(key))
			{
				return key;
			}
		}

		return "";
	}

	protected void IndexFilesStep()
	{
		if (m_IxParse)
		{
			m_IxParse.Step();
			if (m_IxParse.IsDone())
			{
				StoreParse();
				m_IxParse = null;
				m_IxDone++;
			}

			return;
		}

		string key = PopDirty();
		if (key == "")
		{
			if (m_IxChanged || !m_Ready)
			{
				m_IxPhase = IX_RESET;
				m_IxFile = 0;
				m_IxRow = 0;
				m_IxNameMap = new map<string, ref array<int>>();
				m_IxEff = new map<string, ref VPPXETypeRow>();
				m_IxEffBits = new map<string, int>();
				m_IxCatUsed = 0;
				m_IxTagUsed = 0;
				m_IxUsageUsed = 0;
				m_IxValueUsed = 0;
			}
			else
			{
				m_IxPhase = IX_DONE;
			}

			return;
		}

		ProcessFile(key);
	}

	protected void ProcessFile(string key)
	{
		VPPXETypesFileData fd = m_FileByKey.Get(key);
		VPPXEFileRegistry reg = Registry();
		if (!fd || !reg)
		{
			m_IxDone++;
			return;
		}

		VPPXEFileEntry entry = reg.Find(key);
		if (!entry || !FileExist(entry.Path))
		{
			reg.NoteRevision(key, 0, false);
			SetEmpty(fd, false);
			reg.SetParseError(key, false);
			m_IxDone++;
			return;
		}

		string content;
		if (!VPPXmlText.ReadAll(entry.Path, content))
		{
			reg.NoteRevision(key, 0, false);
			SetEmpty(fd, true);
			reg.SetParseError(key, false);
			m_IxDone++;
			return;
		}

		int rev = VPPXmlText.NormalizedHash(content);
		reg.NoteRevision(key, rev, true);
		int regIdx = reg.IndexOf(key);
		if (fd.Indexed && !fd.ReadError && fd.FileRevision == rev && fd.LimitsSig == m_LimitsSig && fd.FileIdx == regIdx && fd.Lines && fd.Rows)
		{
			m_IxDone++;
			return;
		}

		fd.FileIdx = regIdx;
		m_IxKey = key;
		m_IxRevision = rev;
		m_IxParse = new VPPXETypesParseJob(content, regIdx, GetLimits());
	}

	protected void SetEmpty(VPPXETypesFileData fd, bool readError)
	{
		fd.ClearRows();
		fd.FileRevision = 0;
		fd.Indexed = true;
		fd.ReadError = readError;
		fd.ParseError = false;
		fd.LimitsSig = m_LimitsSig;
		m_IxChanged = true;
	}

	protected void StoreParse()
	{
		VPPXETypesFileData fd = m_FileByKey.Get(m_IxKey);
		VPPXEFileRegistry reg = Registry();
		if (!fd)
		{
			return;
		}

		fd.ClearRows();
		if (m_IxParse.Failed())
		{
			VPPXEIssue issue = new VPPXEIssue();
			issue.Code = VPPXEIssueCode.PARSE;
			issue.Severity = VPPXEText.IssueSeverity(VPPXEIssueCode.PARSE);
			issue.FileKey = m_IxKey;
			// VPPXETypesParseJob error lines are 1-based (VPPXmlReader reports 1-based lines), like VPPXmlDocument's; 0 = unknown.
			issue.Line = m_IxParse.GetErrorLine();
			issue.Entry = "";
			issue.Arg = VPPXmlText.Clip(m_IxParse.GetError(), VPPXEConst.MAX_CELL_CHARS);
			fd.FileIssues.Insert(issue);
			fd.ParseError = true;
			if (reg)
			{
				reg.SetParseError(m_IxKey, true);
			}

			VPPXELog.Warn("Types file " + m_IxKey + " does not parse (line " + issue.Line.ToString() + "): " + issue.Arg);
		}
		else
		{
			fd.Rows = m_IxParse.GetRows();
			fd.StartLines = m_IxParse.GetStartLines();
			fd.EndLines = m_IxParse.GetEndLines();
			fd.UnknownRefs = m_IxParse.GetUnknownRefs();
			fd.UnknownSigs = m_IxParse.GetUnknownSigs();
			fd.BadNums = m_IxParse.GetBadNums();
			fd.Lines = m_IxParse.GetLines();
			m_IxParse.GetFileIssues(fd.FileIssues);
			foreach (VPPXEIssue fileIssue : fd.FileIssues)
			{
				fileIssue.FileKey = m_IxKey;
			}

			fd.ParseError = false;
			if (reg)
			{
				reg.SetParseError(m_IxKey, false);
			}
		}

		fd.FileRevision = m_IxRevision;
		fd.Indexed = true;
		fd.ReadError = false;
		fd.LimitsSig = m_LimitsSig;
		m_IxChanged = true;
	}

	// One row per call: clears derived bits, builds the lowercase name map and the DUPLICATE bits.
	protected void IndexResetStep()
	{
		if (m_IxFile >= m_Files.Count())
		{
			m_IxPhase = IX_NAMES;
			m_IxName = 0;
			return;
		}

		VPPXETypesFileData fd = m_Files.Get(m_IxFile);
		if (m_IxRow == 0)
		{
			fd.DupLines = new array<int>();
			fd.OverArgs = new array<string>();
			fd.LastDef = new array<bool>();
			fd.WireBits = new array<int>();
		}

		if (m_IxRow >= fd.Rows.Count())
		{
			m_IxFile++;
			m_IxRow = 0;
			return;
		}

		int ri = m_IxRow;
		m_IxRow++;
		VPPXETypeRow row = fd.Rows.Get(ri);
		row.Issues = row.Issues & ~VPPXERowIssue.DUPLICATE;
		row.Issues = row.Issues & ~VPPXERowIssue.OVERRIDDEN;
		row.DefCount = 0;
		fd.DupLines.Insert(0);
		fd.OverArgs.Insert("");
		fd.LastDef.Insert(false);
		fd.WireBits.Insert(0);
		string lower = row.Name;
		lower.ToLower();
		array<int> pairs = m_IxNameMap.Get(lower);
		if (!pairs)
		{
			pairs = new array<int>();
			m_IxNameMap.Set(lower, pairs);
		}
		else
		{
			int pc = pairs.Count();
			if (pc >= 2 && pairs.Get(pc - 2) == m_IxFile)
			{
				int prevRi = pairs.Get(pc - 1);
				VPPXETypeRow prev = fd.Rows.Get(prevRi);
				prev.Issues = prev.Issues | VPPXERowIssue.DUPLICATE;
				row.Issues = row.Issues | VPPXERowIssue.DUPLICATE;
				fd.DupLines.Set(ri, prev.Line);
				if (fd.DupLines.Get(prevRi) == 0)
				{
					fd.DupLines.Set(prevRi, row.Line);
				}
			}
		}

		pairs.Insert(m_IxFile);
		pairs.Insert(ri);
	}

	// One name per call: OVERRIDDEN, DefCount, the merged effective row and the effective-only bits.
	protected void IndexNamesStep()
	{
		if (m_IxName >= m_IxNameMap.Count())
		{
			m_IxPhase = IX_SWAP;
			return;
		}

		string lower = m_IxNameMap.GetKey(m_IxName);
		array<int> pairs = m_IxNameMap.GetElement(m_IxName);
		m_IxName++;
		if (!pairs || pairs.Count() < 2)
		{
			return;
		}

		int pairCount = pairs.Count();
		int defCount = pairCount / 2;
		int firstFo = pairs.Get(0);
		bool multiFile = false;
		for (int p1 = 2; p1 + 1 < pairCount; p1 += 2)
		{
			if (pairs.Get(p1) != firstFo)
			{
				multiFile = true;
				break;
			}
		}

		if (multiFile)
		{
			for (int p2 = 0; p2 + 1 < pairCount; p2 += 2)
			{
				int ofo = pairs.Get(p2);
				int ori = pairs.Get(p2 + 1);
				VPPXETypesFileData ofd = m_Files.Get(ofo);
				VPPXETypeRow orow = ofd.Rows.Get(ori);
				orow.Issues = orow.Issues | VPPXERowIssue.OVERRIDDEN;
				ofd.OverArgs.Set(ori, NearestOtherKey(pairs, ofo));
			}
		}

		VPPXETypeRow firstRow = m_Files.Get(firstFo).Rows.Get(pairs.Get(1));
		VPPXETypeRow eff = VPPXETypeMerge.NewRow(firstRow.Name);
		VPPXELimits limits = GetLimits();
		for (int p3 = 0; p3 + 1 < pairCount; p3 += 2)
		{
			VPPXETypesFileData mfd = m_Files.Get(pairs.Get(p3));
			VPPXETypeRow mrow = mfd.Rows.Get(pairs.Get(p3 + 1));
			mrow.DefCount = defCount;
			VPPXETypeMerge.MergeInto(eff, mrow, limits);
		}

		int lastFo = pairs.Get(pairCount - 2);
		int lastRi = pairs.Get(pairCount - 1);
		m_Files.Get(lastFo).LastDef.Set(lastRi, true);

		int effBits = 0;
		if (m_Ignore.Contains(lower))
		{
			effBits = effBits | VPPXERowIssue.IGNORED;
		}

		int effUsage = limits.EffectiveUsage(eff.Usage, eff.UsageUser);
		int effValue = limits.EffectiveValue(eff.Value, eff.ValueUser);
		if (eff.Nominal != VPPXEConst.UNSET && eff.Nominal > 0 && effUsage == 0 && effValue == 0)
		{
			effBits = effBits | VPPXERowIssue.NO_PLACEMENT;
		}

		int cfgState = ConfigStateOf(eff.Name);
		if (cfgState == VPPXEConfigState.MISSING)
		{
			effBits = effBits | VPPXERowIssue.NOT_IN_CONFIG;
		}
		else if (cfgState == VPPXEConfigState.NOT_PUBLIC)
		{
			effBits = effBits | VPPXERowIssue.NOT_PUBLIC;
		}

		eff.Issues = eff.Issues | effBits;
		m_IxEff.Set(lower, eff);
		m_IxEffBits.Set(lower, effBits);
		for (int p4 = 0; p4 + 1 < pairCount; p4 += 2)
		{
			m_Files.Get(pairs.Get(p4)).WireBits.Set(pairs.Get(p4 + 1), effBits);
		}

		if (eff.Category >= 0 && eff.Category < 32)
		{
			m_IxCatUsed = m_IxCatUsed | (1 << eff.Category);
		}

		m_IxTagUsed = m_IxTagUsed | eff.Tag;
		m_IxUsageUsed = m_IxUsageUsed | effUsage;
		m_IxValueUsed = m_IxValueUsed | effValue;
	}

	// The other file (load order) closest to fileOrder among the defs of one name; ties prefer the earlier.
	protected string NearestOtherKey(array<int> pairs, int fileOrder)
	{
		int best = -1;
		int bestDist = 0;
		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			int other = pairs.Get(p);
			if (other == fileOrder)
			{
				continue;
			}

			int dist = Math.AbsInt(other - fileOrder);
			if (best < 0 || dist < bestDist || (dist == bestDist && other < best))
			{
				best = other;
				bestDist = dist;
			}
		}

		if (best < 0)
		{
			return "";
		}

		return m_Files.Get(best).Key;
	}

	protected void IndexSwap()
	{
		m_NameMap = m_IxNameMap;
		m_Effective = m_IxEff;
		m_EffBits = m_IxEffBits;
		m_IxNameMap = null;
		m_IxEff = null;
		m_IxEffBits = null;
		BuildUnusedIssues();
		m_IxPhase = IX_COUNT;
		m_IxFile = 0;
		m_IxRow = 0;
		m_IxCount = 0;
	}

	// One row per call: per-file ERROR+WARNING counts (cached for FileInfo.IssueCount).
	protected void IndexCountStep()
	{
		if (m_IxFile >= m_Files.Count())
		{
			m_IxPhase = IX_END;
			return;
		}

		VPPXETypesFileData fd = m_Files.Get(m_IxFile);
		if (m_IxRow >= fd.Rows.Count())
		{
			int fileCount = m_IxCount;
			foreach (VPPXEIssue fileIssue : fd.FileIssues)
			{
				if (fileIssue.Severity >= VPPXESeverity.WARNING)
				{
					fileCount++;
				}
			}

			fd.IssueCount = fileCount;
			m_IxFile++;
			m_IxRow = 0;
			m_IxCount = 0;
			return;
		}

		m_IxCount += EmitRowIssues(m_IxFile, m_IxRow, null);
		m_IxRow++;
	}

	protected void IndexEnd()
	{
		m_LimitCounts = new map<string, int>();
		CountLimitIssues(m_LimitIssues);
		CountLimitIssues(m_UnusedIssues);
		m_Ready = true;
		m_Revision++;
		if (SourcesChanged())
		{
			// The limits or ignore list changed while this job ran: reload them and re-dirty (IndexBegin).
			m_IxPhase = IX_BEGIN;
			return;
		}

		if (m_Dirty.Count() > 0)
		{
			m_IxChanged = false;
			m_IxTotal = m_Dirty.Count();
			m_IxDone = 0;
			m_IxPhase = IX_FILES;
			return;
		}

		m_IxPhase = IX_DONE;
	}

	protected void CountLimitIssues(array<ref VPPXEIssue> issues)
	{
		foreach (VPPXEIssue issue : issues)
		{
			if (issue.Severity >= VPPXESeverity.WARNING)
			{
				int prev = m_LimitCounts.Get(issue.FileKey);
				m_LimitCounts.Set(issue.FileKey, prev + 1);
			}
		}
	}

	// LIMIT_UNUSED: categories, tags, usages and values no effective row uses (user groups excluded).
	protected void BuildUnusedIssues()
	{
		m_UnusedIssues = new array<ref VPPXEIssue>();
		VPPXELimits limits = GetLimits();
		AddUnused(limits.Categories, m_IxCatUsed);
		AddUnused(limits.Tags, m_IxTagUsed);
		AddUnused(limits.Usages, m_IxUsageUsed);
		AddUnused(limits.Values, m_IxValueUsed);
	}

	protected void AddUnused(array<string> names, int usedMask)
	{
		if (!names)
		{
			return;
		}

		int count = names.Count();
		if (count > 32)
		{
			count = 32;
		}

		for (int i = 0; i < count; i++)
		{
			if ((usedMask & (1 << i)) == 0)
			{
				m_UnusedIssues.Insert(NewIssue(VPPXEIssueCode.LIMIT_UNUSED, m_LimitsKey, 0, "", names.Get(i)));
			}
		}
	}

	// ---------------------------------------------------------------- limits and ignore list

	// A registry-seen change of the limits or ignore files is picked up here: with a ready index a reindex job
	// (IndexBegin reloads both and re-dirties every file whose LimitsSig differs; the push follows the job),
	// without one a direct reload. Never while an index job runs (IndexEnd re-checks the sources instead).
	VPPXELimits GetLimits()
	{
		if (!m_LimitsLoaded)
		{
			m_SrcRev = SourcesRev();
			LoadLimits();
			return m_Limits;
		}

		if (!m_Job && SourcesChanged())
		{
			if (m_Ready)
			{
				StartJob(VPPXELane.INTERACTIVE);
			}
			else
			{
				m_SrcRev = SourcesRev();
				LoadLimits();
				m_IgnoreRev = LoadIgnore();
			}
		}

		return m_Limits;
	}

	// Combined registry revisions of the first LIMITS, LIMITSUSER and IGNORELIST entries (no file I/O).
	protected int SourcesRev()
	{
		VPPXEFileRegistry reg = Registry();
		if (!reg)
		{
			return 0;
		}

		int rev = 17;
		rev = rev * 31 + SourceRevOf(reg, VPPXEFileKind.LIMITS);
		rev = rev * 31 + SourceRevOf(reg, VPPXEFileKind.LIMITSUSER);
		rev = rev * 31 + SourceRevOf(reg, VPPXEFileKind.IGNORELIST);
		return rev;
	}

	protected int SourceRevOf(VPPXEFileRegistry reg, int kind)
	{
		VPPXEFileEntry entry = reg.FirstOfKind(kind);
		if (!entry)
		{
			return -1;
		}

		return entry.Revision;
	}

	// True when the limits/ignore files changed (registry view) since the cached copies were loaded.
	protected bool SourcesChanged()
	{
		if (!m_LimitsLoaded)
		{
			return false;
		}

		return SourcesRev() != m_SrcRev;
	}

	// Parses cfglimitsdefinition + user (small documents). The limits object is replaced only when the
	// content changed, so holders of the previous object keep a consistent view.
	protected void LoadLimits()
	{
		VPPXELimits lim = new VPPXELimits();
		array<ref VPPXEIssue> issues = new array<ref VPPXEIssue>();
		string sigText = "limits";
		string defKey = "cfglimitsdefinition.xml";
		VPPXEFileRegistry reg = Registry();
		VPPXEFileEntry defEntry = null;
		VPPXEFileEntry userEntry = null;
		if (reg)
		{
			defEntry = reg.FirstOfKind(VPPXEFileKind.LIMITS);
			userEntry = reg.FirstOfKind(VPPXEFileKind.LIMITSUSER);
		}

		if (defEntry)
		{
			defKey = defEntry.Key;
			VPPXmlDocument defDoc = new VPPXmlDocument();
			if (defDoc.Load(defEntry.Path))
			{
				defDoc.ParseAll();
				sigText = sigText + "|" + defDoc.GetRevision().ToString();
				if (defDoc.HasError())
				{
					issues.Insert(NewIssue(VPPXEIssueCode.PARSE, defEntry.Key, defDoc.GetErrorLine(), "", defDoc.GetError()));
				}
				else
				{
					VPPXmlNode defRoot = VPPXEFileRegistry.ResolveRoot(defDoc, "lists");
					ReadLimitList(defRoot, "categories", "category", lim.Categories, defEntry.Key, issues);
					ReadLimitList(defRoot, "tags", "tag", lim.Tags, defEntry.Key, issues);
					ReadLimitList(defRoot, "usageflags", "usage", lim.Usages, defEntry.Key, issues);
					ReadLimitList(defRoot, "valueflags", "value", lim.Values, defEntry.Key, issues);
				}
			}
		}

		if (userEntry)
		{
			VPPXmlDocument userDoc = new VPPXmlDocument();
			if (userDoc.Load(userEntry.Path))
			{
				userDoc.ParseAll();
				sigText = sigText + "|" + userDoc.GetRevision().ToString();
				if (userDoc.HasError())
				{
					issues.Insert(NewIssue(VPPXEIssueCode.PARSE, userEntry.Key, userDoc.GetErrorLine(), "", userDoc.GetError()));
				}
				else
				{
					VPPXmlNode userRoot = VPPXEFileRegistry.ResolveRoot(userDoc, "user_lists");
					ReadUserGroups(userRoot, "usageflags", "usage", lim.Usages, lim.UsageGroups, lim.UsageGroupMasks, userEntry.Key, issues);
					ReadUserGroups(userRoot, "valueflags", "value", lim.Values, lim.ValueGroups, lim.ValueGroupMasks, userEntry.Key, issues);
				}
			}
		}

		int sig = sigText.Hash();
		m_LimitsKey = defKey;
		m_LimitIssues = issues;
		if (!m_Limits || sig != m_LimitsSig)
		{
			m_Limits = lim;
			m_LimitsSig = sig;
		}

		m_LimitsLoaded = true;
	}

	protected void ReadLimitList(VPPXmlNode root, string containerName, string elemName, array<string> outList, string fileKey, array<ref VPPXEIssue> issues)
	{
		if (!root || !outList)
		{
			return;
		}

		map<string, bool> seen = new map<string, bool>();
		array<VPPXmlNode> containers = new array<VPPXmlNode>();
		root.ChildrenNamed(containerName, containers);
		foreach (VPPXmlNode listNode : containers)
		{
			if (!listNode)
			{
				continue;
			}

			array<VPPXmlNode> items = new array<VPPXmlNode>();
			listNode.ChildrenNamed(elemName, items);
			foreach (VPPXmlNode itemNode : items)
			{
				if (!itemNode)
				{
					continue;
				}

				string itemName = itemNode.GetAttr("name", "");
				if (itemName == "")
				{
					continue;
				}

				string lower = itemName;
				lower.ToLower();
				if (seen.Contains(lower))
				{
					issues.Insert(NewIssue(VPPXEIssueCode.LIMIT_DUP, fileKey, itemNode.StartLine + 1, "", itemName));
					continue;
				}

				seen.Set(lower, true);
				outList.Insert(itemName);
			}
		}

		if (outList.Count() > 32)
		{
			issues.Insert(NewIssue(VPPXEIssueCode.LIMIT_BITS, fileKey, 0, "", containerName));
		}
	}

	// <usageflags|valueflags><user name> = OR of child name= bits and of already-resolved user= groups.
	protected void ReadUserGroups(VPPXmlNode root, string containerName, string elemName, array<string> plainList, array<string> outGroups, array<int> outMasks, string fileKey, array<ref VPPXEIssue> issues)
	{
		if (!root || !outGroups || !outMasks)
		{
			return;
		}

		map<string, bool> seen = new map<string, bool>();
		array<VPPXmlNode> containers = new array<VPPXmlNode>();
		root.ChildrenNamed(containerName, containers);
		foreach (VPPXmlNode listNode : containers)
		{
			if (!listNode)
			{
				continue;
			}

			array<VPPXmlNode> users = new array<VPPXmlNode>();
			listNode.ChildrenNamed("user", users);
			foreach (VPPXmlNode userNode : users)
			{
				if (!userNode)
				{
					continue;
				}

				string groupName = userNode.GetAttr("name", "");
				if (groupName == "")
				{
					continue;
				}

				string lower = groupName;
				lower.ToLower();
				if (seen.Contains(lower))
				{
					issues.Insert(NewIssue(VPPXEIssueCode.LIMIT_DUP, fileKey, userNode.StartLine + 1, "", groupName));
					continue;
				}

				seen.Set(lower, true);
				int mask = 0;
				array<VPPXmlNode> members = new array<VPPXmlNode>();
				userNode.ChildrenNamed(elemName, members);
				foreach (VPPXmlNode member : members)
				{
					if (!member)
					{
						continue;
					}

					string plainName = member.GetAttr("name", "");
					if (plainName != "")
					{
						int idx = VPPXELimits.FindIn(plainList, plainName);
						if (idx >= 0 && idx < 32)
						{
							mask = mask | (1 << idx);
						}
					}

					string refGroup = member.GetAttr("user", "");
					if (refGroup != "")
					{
						int gidx = VPPXELimits.FindIn(outGroups, refGroup);
						if (gidx >= 0 && gidx < outMasks.Count())
						{
							mask = mask | outMasks.Get(gidx);
						}
					}
				}

				outGroups.Insert(groupName);
				outMasks.Insert(mask);
			}
		}

		if (outGroups.Count() > 32)
		{
			issues.Insert(NewIssue(VPPXEIssueCode.LIMIT_BITS, fileKey, 0, "", containerName + " user"));
		}
	}

	// Returns the revision of the ignore list read (0 when missing or unreadable).
	protected int LoadIgnore()
	{
		map<string, bool> ignore = new map<string, bool>();
		int rev = 0;
		VPPXEFileRegistry reg = Registry();
		VPPXEFileEntry entry = null;
		if (reg)
		{
			entry = reg.FirstOfKind(VPPXEFileKind.IGNORELIST);
		}

		if (entry)
		{
			VPPXmlDocument doc = new VPPXmlDocument();
			if (doc.Load(entry.Path))
			{
				doc.ParseAll();
				rev = doc.GetRevision();
				VPPXmlNode root = VPPXEFileRegistry.ResolveRoot(doc, "ignore");
				if (!doc.HasError() && root)
				{
					array<VPPXmlNode> typeNodes = new array<VPPXmlNode>();
					root.ChildrenNamed("type", typeNodes);
					foreach (VPPXmlNode typeNode : typeNodes)
					{
						if (!typeNode)
						{
							continue;
						}

						string lower = typeNode.GetAttr("name", "");
						lower.ToLower();
						if (lower != "")
						{
							ignore.Set(lower, true);
						}
					}
				}
			}
		}

		m_Ignore = ignore;
		m_IgnoreLoaded = true;
		return rev;
	}

	// The lowercase names of cfgignorelist.xml (read once, like IsIgnored).
	void IgnoredNames(array<string> outNames)
	{
		outNames.Clear();
		if (!m_IgnoreLoaded)
		{
			m_IgnoreRev = LoadIgnore();
		}

		for (int i = 0; i < m_Ignore.Count(); i++)
		{
			outNames.Insert(m_Ignore.GetKey(i));
		}
	}

	bool IsIgnored(string typeName)
	{
		if (!m_IgnoreLoaded)
		{
			m_IgnoreRev = LoadIgnore();
		}

		string lower = typeName;
		lower.ToLower();
		return m_Ignore.Contains(lower);
	}

	// Config state of a class: MISSING, NOT_PUBLIC (scope < 2) or PUBLIC.
	int ConfigStateOf(string typeName)
	{
		if (!VPPXmlText.IsValidClassName(typeName))
		{
			return VPPXEConfigState.MISSING;
		}

		string path = "CfgVehicles " + typeName;
		if (!GetGame().ConfigIsExisting(path))
		{
			path = "CfgWeapons " + typeName;
			if (!GetGame().ConfigIsExisting(path))
			{
				path = "CfgMagazines " + typeName;
				if (!GetGame().ConfigIsExisting(path))
				{
					path = "CfgAmmo " + typeName;
					if (!GetGame().ConfigIsExisting(path))
					{
						return VPPXEConfigState.MISSING;
					}
				}
			}
		}

		int cfgScope = GetGame().ConfigGetInt(path + " scope");
		if (cfgScope < 2)
		{
			return VPPXEConfigState.NOT_PUBLIC;
		}

		return VPPXEConfigState.PUBLIC;
	}

	// ---------------------------------------------------------------- queries

	protected VPPXETypesFileData FileAt(int fileOrder)
	{
		if (fileOrder < 0 || fileOrder >= m_Files.Count())
		{
			return null;
		}

		return m_Files.Get(fileOrder);
	}

	int FileOrderOf(string key)
	{
		int count = m_Files.Count();
		for (int i = 0; i < count; i++)
		{
			if (m_Files.Get(i).Key == key)
			{
				return i;
			}
		}

		return -1;
	}

	VPPXETypesFileData GetFileData(string key)
	{
		return m_FileByKey.Get(key);
	}

	// indexed FileRevision == registry Revision and no index job queued or running.
	bool IsFileCurrent(string key)
	{
		if (!m_Ready || m_Job)
		{
			return false;
		}

		return !IsFileStale(key);
	}

	bool AllFilesCurrent()
	{
		if (!m_Ready || m_Job)
		{
			return false;
		}

		VPPXEFileRegistry reg = Registry();
		if (!reg)
		{
			return false;
		}

		array<VPPXEFileEntry> entries = new array<VPPXEFileEntry>();
		reg.GetByKind(VPPXEFileKind.TYPES, entries);
		foreach (VPPXEFileEntry entry : entries)
		{
			if (IsFileStale(entry.Key))
			{
				return false;
			}
		}

		return true;
	}

	// The index does not hold the registry revision of this file (a reindex is needed).
	bool IsFileStale(string key)
	{
		VPPXEFileRegistry reg = Registry();
		if (!reg)
		{
			return true;
		}

		VPPXEFileEntry entry = reg.Find(key);
		VPPXETypesFileData fd = m_FileByKey.Get(key);
		if (!entry || !fd || !fd.Indexed)
		{
			return true;
		}

		if (fd.FileIdx != reg.IndexOf(key))
		{
			return true;
		}

		// Limits or ignore list changed: every file's rows, lint and effective bits depend on them.
		if (SourcesChanged())
		{
			return true;
		}

		return fd.FileRevision != entry.Revision;
	}

	int GetFileRevision(string key)
	{
		VPPXETypesFileData fd = m_FileByKey.Get(key);
		if (!fd || !fd.Indexed)
		{
			return 0;
		}

		return fd.FileRevision;
	}

	int GetEntryCount(string key)
	{
		VPPXETypesFileData fd = m_FileByKey.Get(key);
		if (!fd || !fd.Rows)
		{
			return 0;
		}

		return fd.Rows.Count();
	}

	VPPXETypeRow GetEffective(string typeName)
	{
		string lower = typeName;
		lower.ToLower();
		return m_Effective.Get(lower);
	}

	void GetEffectiveNames(array<string> outNames)
	{
		if (!outNames)
		{
			return;
		}

		int count = m_Effective.Count();
		for (int i = 0; i < count; i++)
		{
			VPPXETypeRow eff = m_Effective.GetElement(i);
			if (eff)
			{
				outNames.Insert(eff.Name);
			}
		}
	}

	void GetDefFileKeys(string typeName, array<string> outKeys)
	{
		if (!outKeys)
		{
			return;
		}

		string lower = typeName;
		lower.ToLower();
		array<int> pairs = m_NameMap.Get(lower);
		if (!pairs)
		{
			return;
		}

		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			VPPXETypesFileData fd = FileAt(pairs.Get(p));
			if (fd && outKeys.Find(fd.Key) < 0)
			{
				outKeys.Insert(fd.Key);
			}
		}
	}

	// Index view: does the TYPES file define the (lowercase) name?
	bool FileDefinesName(string key, string lowerName)
	{
		array<int> pairs = m_NameMap.Get(lowerName);
		if (!pairs)
		{
			return false;
		}

		int fo = FileOrderOf(key);
		if (fo < 0)
		{
			return false;
		}

		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			if (pairs.Get(p) == fo)
			{
				return true;
			}
		}

		return false;
	}

	// Copy of a def row for the wire, carrying the effective-only bits of its name.
	VPPXETypeRow WireRow(VPPXETypesFileData fd, int ri)
	{
		if (!fd || ri < 0 || ri >= fd.Rows.Count())
		{
			return null;
		}

		VPPXETypeRow copy = VPPXETypeMerge.Copy(fd.Rows.Get(ri));
		if (ri < fd.WireBits.Count())
		{
			copy.Issues = copy.Issues | fd.WireBits.Get(ri);
		}

		return copy;
	}

	// ---------------------------------------------------------------- issues

	int GetIssueCount(string key)
	{
		int count = m_LimitCounts.Get(key);
		VPPXETypesFileData fd = m_FileByKey.Get(key);
		if (fd)
		{
			count += fd.IssueCount;
		}

		return count;
	}

	// Resumable: cursor 0 = file-level issues, then 1 + a global row position. Returns the next cursor or -1.
	int CollectIssuesStep(array<ref VPPXEIssue> outIssues, string fileKeyFilter, int cursor, int maxRows)
	{
		if (!m_Ready || !outIssues)
		{
			return -1;
		}

		if (cursor <= 0)
		{
			foreach (VPPXETypesFileData fd : m_Files)
			{
				if (fileKeyFilter != "" && fd.Key != fileKeyFilter)
				{
					continue;
				}

				foreach (VPPXEIssue fileIssue : fd.FileIssues)
				{
					outIssues.Insert(fileIssue);
				}
			}

			foreach (VPPXEIssue limitIssue : m_LimitIssues)
			{
				if (fileKeyFilter == "" || limitIssue.FileKey == fileKeyFilter)
				{
					outIssues.Insert(limitIssue);
				}
			}

			foreach (VPPXEIssue unusedIssue : m_UnusedIssues)
			{
				if (fileKeyFilter == "" || unusedIssue.FileKey == fileKeyFilter)
				{
					outIssues.Insert(unusedIssue);
				}
			}

			return 1;
		}

		int g = cursor - 1;
		int fo = 0;
		int start = 0;
		int fileCount = m_Files.Count();
		while (fo < fileCount && g >= start + m_Files.Get(fo).Rows.Count())
		{
			start += m_Files.Get(fo).Rows.Count();
			fo++;
		}

		int processed = 0;
		while (fo < fileCount && processed < maxRows)
		{
			VPPXETypesFileData cur = m_Files.Get(fo);
			int rows = cur.Rows.Count();
			if (fileKeyFilter != "" && cur.Key != fileKeyFilter)
			{
				start += rows;
				g = start;
				fo++;
				continue;
			}

			int ri = g - start;
			if (ri >= rows)
			{
				start += rows;
				g = start;
				fo++;
				continue;
			}

			EmitRowIssues(fo, ri, outIssues);
			processed++;
			g++;
		}

		if (fo >= fileCount)
		{
			return -1;
		}

		return g + 1;
	}

	// Issues of one name through the name map (O(defs of that name)).
	void CollectIssuesFor(string typeName, array<ref VPPXEIssue> outIssues)
	{
		if (!outIssues || !m_Ready)
		{
			return;
		}

		string lower = typeName;
		lower.ToLower();
		array<int> pairs = m_NameMap.Get(lower);
		if (!pairs)
		{
			return;
		}

		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			EmitRowIssues(pairs.Get(p), pairs.Get(p + 1), outIssues);
		}
	}

	// Expands the bits of one def row into issues; returns its ERROR+WARNING count. outIssues null = count only.
	protected int EmitRowIssues(int fo, int ri, array<ref VPPXEIssue> outIssues)
	{
		VPPXETypesFileData fd = FileAt(fo);
		if (!fd || ri < 0 || ri >= fd.Rows.Count())
		{
			return 0;
		}

		VPPXETypeRow row = fd.Rows.Get(ri);
		int bits = row.Issues;
		int ew = 0;
		string key = fd.Key;
		int line = row.Line;
		string entryName = row.Name;

		if (ri < fd.UnknownRefs.Count())
		{
			array<string> refs = fd.UnknownRefs.Get(ri);
			if (refs)
			{
				foreach (string rowRef : refs)
				{
					int bar = rowRef.IndexOf("|");
					if (bar < 0)
					{
						continue;
					}

					string refKind = VPPXETypesWriter.Mid(rowRef, 0, bar);
					string refName = VPPXETypesWriter.Mid(rowRef, bar + 1, rowRef.Length() - bar - 1);
					if (refKind == "usageuser" || refKind == "valueuser")
					{
						refName = "@" + refName;
					}

					ew += AddIssue(outIssues, RefIssueCode(refKind), key, line, entryName, refName);
				}
			}
		}

		if ((bits & VPPXERowIssue.MULTI_CATEGORY) != 0)
		{
			ew += AddIssue(outIssues, VPPXEIssueCode.MULTI_CATEGORY, key, line, entryName, CategoryLabel(fd, ri));
		}

		if ((bits & VPPXERowIssue.MIN_GT_NOMINAL) != 0)
		{
			ew += AddIssue(outIssues, VPPXEIssueCode.MIN_GT_NOMINAL, key, line, entryName, "");
		}

		if ((bits & VPPXERowIssue.QUANT) != 0)
		{
			ew += QuantIssue(outIssues, row, VPPXEField.QUANTMIN, key, line, entryName);
			ew += QuantIssue(outIssues, row, VPPXEField.QUANTMAX, key, line, entryName);
		}

		if ((row.Present & VPPXEField.QUANTMIN) != 0 && (row.Present & VPPXEField.QUANTMAX) != 0 && row.QuantMin >= 0 && row.QuantMax >= 0 && row.QuantMin > row.QuantMax)
		{
			ew += AddIssue(outIssues, VPPXEIssueCode.QUANT_ORDER, key, line, entryName, "");
		}

		if ((bits & VPPXERowIssue.NEGATIVE) != 0)
		{
			ew += NegativeIssue(outIssues, row, VPPXEField.NOMINAL, key, line, entryName);
			ew += NegativeIssue(outIssues, row, VPPXEField.MIN, key, line, entryName);
			ew += NegativeIssue(outIssues, row, VPPXEField.LIFETIME, key, line, entryName);
			ew += NegativeIssue(outIssues, row, VPPXEField.RESTOCK, key, line, entryName);
			ew += NegativeIssue(outIssues, row, VPPXEField.COST, key, line, entryName);
		}

		if ((bits & VPPXERowIssue.NOT_NUMBER) != 0 && ri < fd.BadNums.Count())
		{
			int badMask = fd.BadNums.Get(ri);
			for (int fi = 0; fi < VPPXEConst.FIELD_COUNT; fi++)
			{
				int fieldBit = VPPXETypeMerge.FieldBitAt(fi);
				if ((badMask & fieldBit) != 0)
				{
					ew += AddIssue(outIssues, VPPXEIssueCode.NOT_NUMBER, key, line, entryName, VPPXETypeMerge.FieldElementName(fieldBit));
				}
			}
		}

		if ((bits & VPPXERowIssue.DUPLICATE) != 0 && ri < fd.DupLines.Count())
		{
			ew += AddIssue(outIssues, VPPXEIssueCode.DUP_IN_FILE, key, line, entryName, fd.DupLines.Get(ri).ToString());
		}

		if ((bits & VPPXERowIssue.OVERRIDDEN) != 0 && ri < fd.OverArgs.Count())
		{
			ew += AddIssue(outIssues, VPPXEIssueCode.OVERRIDDEN, key, line, entryName, fd.OverArgs.Get(ri));
		}

		if (ri < fd.LastDef.Count() && fd.LastDef.Get(ri))
		{
			string lower = entryName;
			lower.ToLower();
			int effBits = m_EffBits.Get(lower);
			if ((effBits & VPPXERowIssue.NOT_IN_CONFIG) != 0)
			{
				ew += AddIssue(outIssues, VPPXEIssueCode.NOT_IN_CONFIG, key, line, entryName, "");
			}

			if ((effBits & VPPXERowIssue.NOT_PUBLIC) != 0)
			{
				ew += AddIssue(outIssues, VPPXEIssueCode.NOT_PUBLIC, key, line, entryName, "");
			}

			if ((effBits & VPPXERowIssue.IGNORED) != 0)
			{
				string nominalText = "0";
				VPPXETypeRow eff = m_Effective.Get(lower);
				if (eff && eff.Nominal != VPPXEConst.UNSET)
				{
					nominalText = eff.Nominal.ToString();
				}

				ew += AddIssue(outIssues, VPPXEIssueCode.IGNORED, key, line, entryName, nominalText);
			}

			if ((effBits & VPPXERowIssue.NO_PLACEMENT) != 0)
			{
				ew += AddIssue(outIssues, VPPXEIssueCode.NO_PLACEMENT, key, line, entryName, "");
			}
		}

		return ew;
	}

	protected int QuantIssue(array<ref VPPXEIssue> outIssues, VPPXETypeRow row, int fieldBit, string key, int line, string entryName)
	{
		if ((row.Present & fieldBit) == 0)
		{
			return 0;
		}

		int v = VPPXETypeMerge.GetScalar(row, fieldBit);
		if (v == -1 || v == VPPXEConst.UNSET || (v >= 0 && v <= 100))
		{
			return 0;
		}

		return AddIssue(outIssues, VPPXEIssueCode.QUANT_RANGE, key, line, entryName, v.ToString());
	}

	protected int NegativeIssue(array<ref VPPXEIssue> outIssues, VPPXETypeRow row, int fieldBit, string key, int line, string entryName)
	{
		if ((row.Present & fieldBit) == 0)
		{
			return 0;
		}

		int v = VPPXETypeMerge.GetScalar(row, fieldBit);
		if (v == VPPXEConst.UNSET || v >= 0)
		{
			return 0;
		}

		return AddIssue(outIssues, VPPXEIssueCode.NEGATIVE, key, line, entryName, VPPXETypeMerge.FieldElementName(fieldBit));
	}

	protected string CategoryLabel(VPPXETypesFileData fd, int ri)
	{
		VPPXETypeRow row = fd.Rows.Get(ri);
		VPPXELimits limits = GetLimits();
		if (row.Category >= 0 && limits && row.Category < limits.Categories.Count())
		{
			return limits.Categories.Get(row.Category);
		}

		if (ri < fd.UnknownRefs.Count())
		{
			array<string> refs = fd.UnknownRefs.Get(ri);
			if (refs)
			{
				foreach (string rowRef : refs)
				{
					if (rowRef.IndexOf("category|") == 0)
					{
						return VPPXETypesWriter.Mid(rowRef, 9, rowRef.Length() - 9);
					}
				}
			}
		}

		return "";
	}

	static int RefIssueCode(string refKind)
	{
		if (refKind == "category")
		{
			return VPPXEIssueCode.UNKNOWN_CATEGORY;
		}

		if (refKind == "usage" || refKind == "usageuser")
		{
			return VPPXEIssueCode.UNKNOWN_USAGE;
		}

		if (refKind == "value" || refKind == "valueuser")
		{
			return VPPXEIssueCode.UNKNOWN_VALUE;
		}

		if (refKind == "tag")
		{
			return VPPXEIssueCode.UNKNOWN_TAG;
		}

		return VPPXEIssueCode.UNKNOWN_CHILD;
	}

	protected int AddIssue(array<ref VPPXEIssue> outIssues, int code, string fileKey, int line, string entryName, string arg)
	{
		int severity = VPPXEText.IssueSeverity(code);
		if (outIssues)
		{
			outIssues.Insert(NewIssue(code, fileKey, line, entryName, arg));
		}

		if (severity >= VPPXESeverity.WARNING)
		{
			return 1;
		}

		return 0;
	}

	protected VPPXEIssue NewIssue(int code, string fileKey, int line, string entryName, string arg)
	{
		VPPXEIssue issue = new VPPXEIssue();
		issue.Code = code;
		issue.Severity = VPPXEText.IssueSeverity(code);
		issue.FileKey = fileKey;
		issue.Line = line;
		issue.Entry = entryName;
		issue.Arg = VPPXmlText.Clip(arg, VPPXEConst.MAX_CELL_CHARS);
		return issue;
	}

	// ---------------------------------------------------------------- details

	static int EstimateRow(VPPXETypeRow r)
	{
		if (!r)
		{
			return 4;
		}

		return 84 + r.Name.Length();
	}

	static int EstimateIssue(VPPXEIssue issue)
	{
		return 32 + issue.FileKey.Length() + issue.Entry.Length() + issue.Arg.Length();
	}

	// Defs (load order), DefRevisions, Effective and FieldSource. Returns the updated payload estimate.
	int FillDetailsCore(VPPXETypeDetails d, string typeName, int est)
	{
		d.Found = false;
		if (!m_Ready)
		{
			return est;
		}

		string lower = typeName;
		lower.ToLower();
		array<int> pairs = m_NameMap.Get(lower);
		if (!pairs || pairs.Count() < 2)
		{
			return est;
		}

		d.Found = true;
		int total = est;
		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			VPPXETypesFileData fd = FileAt(pairs.Get(p));
			VPPXETypeRow copy = WireRow(fd, pairs.Get(p + 1));
			if (!copy)
			{
				continue;
			}

			d.Defs.Insert(copy);
			d.DefRevisions.Insert(fd.FileRevision);
			total += EstimateRow(copy) + 4;
			for (int fi = 0; fi < VPPXEConst.FIELD_COUNT; fi++)
			{
				int fieldBit = VPPXETypeMerge.FieldBitAt(fi);
				if (DefHasField(copy, fieldBit))
				{
					SetFieldSource(d, fi, copy.FileIdx);
				}
			}
		}

		VPPXETypeRow eff = m_Effective.Get(lower);
		if (eff)
		{
			d.Effective = VPPXETypeMerge.Copy(eff);
			total += EstimateRow(eff);
		}

		total += 4 + 4 * VPPXEConst.FIELD_COUNT;
		return total;
	}

	// Mirrors VPPXETypeMerge.MergeInto: which elements of a def replace the accumulated value.
	protected bool DefHasField(VPPXETypeRow row, int fieldBit)
	{
		VPPXELimits limits = GetLimits();
		if (fieldBit == VPPXEField.USAGE)
		{
			if (limits)
			{
				return limits.EffectiveUsage(row.Usage, row.UsageUser) != 0;
			}

			return (row.Usage | row.UsageUser) != 0;
		}

		if (fieldBit == VPPXEField.VALUE)
		{
			if (limits)
			{
				return limits.EffectiveValue(row.Value, row.ValueUser) != 0;
			}

			return (row.Value | row.ValueUser) != 0;
		}

		if (fieldBit == VPPXEField.TAG)
		{
			return row.Tag != 0;
		}

		return (row.Present & fieldBit) != 0;
	}

	protected void SetFieldSource(VPPXETypeDetails d, int fieldIndex, int fileIdx)
	{
		while (d.FieldSource.Count() <= fieldIndex)
		{
			d.FieldSource.Insert(-1);
		}

		d.FieldSource.Set(fieldIndex, fileIdx);
	}

	int FillDetailsIssues(VPPXETypeDetails d, string typeName, int est, int budget)
	{
		array<ref VPPXEIssue> all = new array<ref VPPXEIssue>();
		CollectIssuesFor(typeName, all);
		int total = est;
		foreach (VPPXEIssue issue : all)
		{
			if (d.Issues.Count() >= MAX_DETAIL_ISSUES)
			{
				break;
			}

			int size = EstimateIssue(issue);
			if (total + size > budget)
			{
				break;
			}

			d.Issues.Insert(issue);
			total += size;
		}

		return total;
	}

	// "<defIndex>|<kind>|<name>", at most 50.
	int FillDetailsUnknownRefs(VPPXETypeDetails d, string typeName, int est, int budget)
	{
		int total = est;
		if (!m_Ready)
		{
			return total;
		}

		string lower = typeName;
		lower.ToLower();
		array<int> pairs = m_NameMap.Get(lower);
		if (!pairs)
		{
			return total;
		}

		int defIndex = 0;
		int pairCount = pairs.Count();
		for (int p = 0; p + 1 < pairCount; p += 2)
		{
			VPPXETypesFileData fd = FileAt(pairs.Get(p));
			int ri = pairs.Get(p + 1);
			if (!fd || ri < 0 || ri >= fd.UnknownRefs.Count())
			{
				defIndex++;
				continue;
			}

			array<string> refs = fd.UnknownRefs.Get(ri);
			if (refs)
			{
				foreach (string rowRef : refs)
				{
					if (d.UnknownRefs.Count() >= MAX_DETAIL_REFS)
					{
						return total;
					}

					string item = defIndex.ToString() + "|" + VPPXmlText.Clip(rowRef, VPPXEConst.MAX_CELL_CHARS);
					int size = item.Length() + 4;
					if (total + size > budget)
					{
						return total;
					}

					d.UnknownRefs.Insert(item);
					total += size;
				}
			}

			defIndex++;
		}

		return total;
	}

	// Raw lines of the winning (last) def, at most 60, each clipped; a cut block ends with "...".
	int FillDetailsRawBlock(VPPXETypeDetails d, string typeName, int est, int budget)
	{
		int total = est;
		if (!m_Ready)
		{
			return total;
		}

		string lower = typeName;
		lower.ToLower();
		array<int> pairs = m_NameMap.Get(lower);
		if (!pairs || pairs.Count() < 2)
		{
			return total;
		}

		VPPXETypesFileData fd = FileAt(pairs.Get(pairs.Count() - 2));
		int ri = pairs.Get(pairs.Count() - 1);
		if (!fd || !fd.Lines || ri < 0 || ri >= fd.StartLines.Count() || ri >= fd.EndLines.Count())
		{
			return total;
		}

		int first = fd.StartLines.Get(ri);
		int last = fd.EndLines.Get(ri);
		if (last >= fd.Lines.Count())
		{
			last = fd.Lines.Count() - 1;
		}

		for (int li = first; li <= last; li++)
		{
			if (li < 0)
			{
				continue;
			}

			string lineText = VPPXmlText.Clip(fd.Lines.Get(li), VPPXEConst.MAX_CELL_CHARS);
			int size = lineText.Length() + 4;
			if (d.RawBlock.Count() >= MAX_RAW_LINES - 1 && li < last)
			{
				d.RawBlock.Insert("...");
				return total + 7;
			}

			if (total + size + 7 > budget)
			{
				d.RawBlock.Insert("...");
				return total + 7;
			}

			d.RawBlock.Insert(lineText);
			total += size;
		}

		return total;
	}

	// ---------------------------------------------------------------- idle freeing

	// All or nothing: rows, lines, FileRevisions, counts and the name map; the next request re-indexes.
	void FreeCaches()
	{
		if (m_Job)
		{
			VPPXETypesIndexJob job = m_Job;
			m_Job = null;
			VPPXEJobQueue.Get().Cancel(job);
		}

		ResetIxState();
		m_Files = new array<ref VPPXETypesFileData>();
		m_FileByKey = new map<string, ref VPPXETypesFileData>();
		m_NameMap = new map<string, ref array<int>>();
		m_Effective = new map<string, ref VPPXETypeRow>();
		m_EffBits = new map<string, int>();
		m_Dirty = new map<string, bool>();
		m_LimitCounts = new map<string, int>();
		m_UnusedIssues = new array<ref VPPXEIssue>();
		m_Ready = false;
		VPPXELog.Info("Types caches freed");
	}
};
