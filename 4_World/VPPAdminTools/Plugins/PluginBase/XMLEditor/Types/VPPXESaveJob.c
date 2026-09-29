// XML Editor save pipeline (one INTERACTIVE job per batch): validation, stale check, DOM planning and the
// forward-pass apply per file, line-level and semantic verification, the override rule re-checked against the
// disk, backup, verified write (target first for COPY/MOVE) and the result.

// Pipeline of one file: old rows, DOM, plan, apply, join, re-parse, line-level verification.
class VPPXESaveFileWork : Managed
{
	const static int W_OLDROWS = 0;
	const static int W_DOM = 1;
	const static int W_PLAN = 2;
	const static int W_APPLY = 3;
	const static int W_JOIN = 4;
	const static int W_REPARSE = 5;
	const static int W_VLINES = 6;
	const static int W_DONE = 7;

	ref VPPXEFileEntry Entry;
	ref VPPXELimits Limits;
	string Content;
	int Revision;
	int FileIdx;
	ref array<ref VPPXETypeEdit> Edits;
	ref array<ref VPPXETypeEdit> CopyEdits;
	ref array<ref array<string>> CopyBlocks;
	ref VPPXETypesParseJob OldParse;
	ref VPPXmlDocument Doc;
	ref VPPXETypesWriter Writer;
	ref VPPXmlJoiner Joiner;
	ref VPPXETypesParseJob NewParse;
	string NewContent;
	bool Changed;
	int Phase;
	bool Failed;
	string ErrKey;
	string ErrArg;
	int RunIdx;
	int RunLine;

	void VPPXESaveFileWork(VPPXEFileEntry entry, string content, int revision, int fileIdx, VPPXELimits limits)
	{
		Entry = entry;
		Content = content;
		Revision = revision;
		FileIdx = fileIdx;
		Limits = limits;
		NewContent = "";
		Phase = W_OLDROWS;
		OldParse = new VPPXETypesParseJob(content, fileIdx, limits);
	}

	protected void SetFail(string errKey, string errArg)
	{
		Failed = true;
		ErrKey = errKey;
		ErrArg = errArg;
	}

	int GetPct()
	{
		return (Phase * 100) / W_DONE;
	}

	// One bounded unit per call (inner Steps loop while BudgetOk). True when done or failed.
	bool Advance()
	{
		if (Failed || Phase == W_DONE)
		{
			return true;
		}

		if (Phase == W_OLDROWS)
		{
			OldParse.Step();
			if (!OldParse.IsDone())
			{
				return false;
			}

			if (OldParse.Failed())
			{
				SetFail("#VSTR_XMLE_ERR_PARSE", VPPXmlText.Clip(OldParse.GetError(), VPPXEConst.MAX_CELL_CHARS));
				return true;
			}

			Doc = new VPPXmlDocument();
			Doc.BeginString(Content);
			Phase = W_DOM;
			return false;
		}

		if (Phase == W_DOM)
		{
			if (!Doc.Step())
			{
				return false;
			}

			if (Doc.HasError())
			{
				SetFail("#VSTR_XMLE_ERR_PARSE", VPPXmlText.Clip(Doc.GetError(), VPPXEConst.MAX_CELL_CHARS));
				return true;
			}

			Writer = new VPPXETypesWriter(Doc, Limits);
			Writer.SetEdits(Edits);
			Writer.SetCopies(CopyEdits, CopyBlocks);
			Phase = W_PLAN;
			return false;
		}

		if (Phase == W_PLAN)
		{
			if (!Writer.PlanStep())
			{
				return false;
			}

			if (Writer.Failed())
			{
				SetFail(Writer.GetErrKey(), Writer.GetErrArg());
				return true;
			}

			Changed = Writer.PatchCount() > 0;
			if (!Changed)
			{
				NewContent = Content;
				Phase = W_DONE;
				return true;
			}

			Phase = W_APPLY;
			return false;
		}

		if (Phase == W_APPLY)
		{
			if (!Writer.ApplyStep(256))
			{
				return false;
			}

			Joiner = new VPPXmlJoiner();
			Joiner.Begin(Writer.GetNewLines(), Writer.GetNewCR(), Writer.GetPrefix(), Writer.EndsWithNewline());
			Phase = W_JOIN;
			return false;
		}

		if (Phase == W_JOIN)
		{
			Joiner.Step(256);
			if (!Joiner.IsDone())
			{
				return false;
			}

			NewContent = Joiner.GetResult();
			Joiner = null;
			NewParse = new VPPXETypesParseJob(NewContent, FileIdx, Limits);
			Phase = W_REPARSE;
			return false;
		}

		if (Phase == W_REPARSE)
		{
			NewParse.Step();
			if (!NewParse.IsDone())
			{
				return false;
			}

			if (NewParse.Failed())
			{
				SetFail("#VSTR_XMLE_ERR_INTERNAL", VPPXmlText.Clip("reparse: " + NewParse.GetError(), VPPXEConst.MAX_CELL_CHARS));
				return true;
			}

			RunIdx = 0;
			RunLine = 0;
			Phase = W_VLINES;
			return false;
		}

		if (Phase == W_VLINES)
		{
			VerifyLinesStep();
			if (Failed || Phase == W_DONE)
			{
				return true;
			}

			return false;
		}

		return true;
	}

	// Every untouched original run must appear unchanged (text and CR flag) at its output position in the
	// re-split result. At most 256 lines per call.
	protected void VerifyLinesStep()
	{
		array<int> runs = Writer.GetRuns();
		if (RunIdx + 2 >= runs.Count())
		{
			Phase = W_DONE;
			return;
		}

		array<string> oldLines = Doc.GetLines();
		array<bool> oldCR = Doc.GetCRFlags();
		array<string> reLines = NewParse.GetLines();
		array<bool> reCR = NewParse.GetCRFlags();
		int origStart = runs.Get(RunIdx);
		int origEnd = runs.Get(RunIdx + 1);
		int outStart = runs.Get(RunIdx + 2);
		int runLen = origEnd - origStart;
		int stop = RunLine + 256;
		if (stop > runLen)
		{
			stop = runLen;
		}

		for (int k = RunLine; k < stop; k++)
		{
			int oi = origStart + k;
			int ni = outStart + k;
			bool same = oi < oldLines.Count() && ni < reLines.Count() && oi < oldCR.Count() && ni < reCR.Count();
			if (same && reLines.Get(ni) != oldLines.Get(oi))
			{
				same = false;
			}

			if (same && reCR.Get(ni) != oldCR.Get(oi))
			{
				same = false;
			}

			if (!same)
			{
				int lineNo = oi + 1;
				SetFail("#VSTR_XMLE_ERR_INTERNAL", "line " + lineNo.ToString());
				return;
			}
		}

		RunLine = stop;
		if (RunLine >= runLen)
		{
			RunIdx = RunIdx + 3;
			RunLine = 0;
		}
	}
};

class VPPXESaveJob : VPPXEJob
{
	const static int S_VALIDATE = 0;
	const static int S_READ = 1;
	const static int S_SRC = 2;
	const static int S_TGT_PREP = 3;
	const static int S_TGT_BLOCKS = 4;
	const static int S_TGT = 5;
	const static int S_LAST = 6;
	const static int S_SEM_SRC = 7;
	const static int S_SEM_SRC_ADD = 8;
	const static int S_SEM_SRC_ACT = 9;
	const static int S_SEM_SRC_CMP = 10;
	const static int S_SEM_TGT = 11;
	const static int S_SEM_TGT_COPY = 12;
	const static int S_SEM_TGT_ACT = 13;
	const static int S_SEM_TGT_CMP = 14;
	const static int S_OVR_LOCAL = 15;
	const static int S_OVR_FILES = 16;
	const static int S_OVR_DECIDE = 17;
	const static int S_RECHECK = 18;
	const static int S_WRITE_TGT = 19;
	const static int S_WRITE_SRC = 20;
	const static int S_FINISH = 21;
	const static int S_DONE = 22;

	protected XMLEditor m_Owner;
	protected string m_PlainId;
	protected string m_AdminName;
	protected string m_AdminId;
	protected ref VPPXEEditBatch m_Batch;
	protected ref VPPXELimits m_Limits;
	protected ref VPPXEFileEntry m_File;
	protected ref VPPXEFileEntry m_TargetEntry;
	protected string m_TargetKey;
	protected int m_Reason;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_Cursor2;
	protected bool m_Done;
	protected ref VPPXESaveFileWork m_Src;
	protected ref VPPXESaveFileWork m_Tgt;
	protected ref map<string, bool> m_Touched;
	protected ref map<string, int> m_EditByName;
	protected ref map<string, int> m_LastIdx;
	protected ref array<ref VPPXETypeEdit> m_CopyEdits;
	protected ref array<ref array<string>> m_CopyBlocks;

	// Semantic verification.
	protected ref array<string> m_Expected;
	protected ref array<string> m_Actual;

	// Override rule: resulting names with their receiving and excluded files.
	protected ref array<string> m_RNames;
	protected ref array<string> m_RDisplay;
	protected ref array<string> m_RRecv;
	protected ref array<string> m_RExcl;
	protected ref array<int> m_REdit;
	protected ref map<string, bool> m_ResultSet;
	protected ref map<string, ref array<string>> m_Definers;
	protected ref array<string> m_OtherKeys;
	protected int m_OvrIdx;
	protected int m_OvrMode;
	protected int m_OvrCursor;
	protected string m_OvrKey;
	protected ref VPPXETypesParseJob m_OvrParse;

	// Write state.
	protected string m_SrcBackupId;
	protected string m_TgtBackupId;
	protected bool m_TgtWritten;
	protected bool m_SrcWritten;
	protected string m_NoticeKey;
	protected string m_Summary;
	protected ref array<string> m_TouchedNames;

	void VPPXESaveJob(XMLEditor owner, PlayerIdentity sender, VPPXEEditBatch batch)
	{
		m_Owner = owner;
		m_Batch = batch;
		m_PlainId = "";
		m_AdminName = "System";
		m_AdminId = "";
		if (sender)
		{
			m_PlainId = sender.GetPlainId();
			m_AdminName = sender.GetName();
			m_AdminId = sender.GetPlainId();
		}

		m_TargetKey = "";
		m_NoticeKey = "";
		m_SrcBackupId = "";
		m_TgtBackupId = "";
		m_Summary = "";
		m_Touched = new map<string, bool>();
		m_EditByName = new map<string, int>();
		m_LastIdx = new map<string, int>();
		m_CopyEdits = new array<ref VPPXETypeEdit>();
		m_CopyBlocks = new array<ref array<string>>();
		m_Expected = new array<string>();
		m_Actual = new array<string>();
		m_RNames = new array<string>();
		m_RDisplay = new array<string>();
		m_RRecv = new array<string>();
		m_RExcl = new array<string>();
		m_REdit = new array<int>();
		m_ResultSet = new map<string, bool>();
		m_Definers = new map<string, ref array<string>>();
		m_OtherKeys = new array<string>();
		m_TouchedNames = new array<string>();
		m_Phase = S_VALIDATE;
	}

	override string GetLabel()
	{
		return "SaveTypes";
	}

	protected PlayerIdentity Requester()
	{
		if (m_PlainId == "")
		{
			return null;
		}

		return VPPXENet.ResolveIdentity(m_PlainId);
	}

	protected VPPXEFileRegistry Reg()
	{
		return m_Owner.GetRegistry();
	}

	protected VPPXETypesService Types()
	{
		return m_Owner.GetTypes();
	}

	protected int ReqId()
	{
		if (!m_Batch)
		{
			return 0;
		}

		return m_Batch.ReqId;
	}

	override bool Step()
	{
		if (m_Done || !m_Owner || !m_Batch)
		{
			return true;
		}

		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Done)
			{
				return true;
			}

			RunPhase();
		}

		if (m_Done)
		{
			return true;
		}

		VPPXENet.Progress(Requester(), ReqId(), VPPXEStage.SAVE, ProgressPct());
		return false;
	}

	protected int ProgressPct()
	{
		if (m_Phase <= S_READ)
		{
			return 2;
		}

		if (m_Phase == S_SRC && m_Src)
		{
			return 5 + (m_Src.GetPct() * 45) / 100;
		}

		if (m_Phase <= S_TGT)
		{
			return 55;
		}

		if (m_Phase <= S_SEM_TGT_CMP)
		{
			return 70;
		}

		if (m_Phase <= S_OVR_DECIDE)
		{
			return 80;
		}

		return 90;
	}

	protected void RunPhase()
	{
		if (m_Phase == S_VALIDATE)
		{
			ValidateStep();
			return;
		}

		if (m_Phase == S_READ)
		{
			ReadSource();
			return;
		}

		if (m_Phase == S_SRC)
		{
			if (m_Src.Advance())
			{
				if (m_Src.Failed)
				{
					FailSave(m_Src.ErrKey, m_Src.ErrArg);
					return;
				}

				if (m_TargetKey != "")
				{
					m_Phase = S_TGT_PREP;
				}
				else
				{
					m_Phase = S_LAST;
					m_Cursor = 0;
				}
			}

			return;
		}

		if (m_Phase == S_TGT_PREP)
		{
			PrepareTarget();
			return;
		}

		if (m_Phase == S_TGT_BLOCKS)
		{
			TargetBlockStep();
			return;
		}

		if (m_Phase == S_TGT)
		{
			if (m_Tgt.Advance())
			{
				if (m_Tgt.Failed)
				{
					FailSave(m_Tgt.ErrKey, m_Tgt.ErrArg);
					return;
				}

				m_Phase = S_LAST;
				m_Cursor = 0;
			}

			return;
		}

		if (m_Phase == S_LAST)
		{
			LastIndexStep();
			return;
		}

		if (m_Phase == S_SEM_SRC)
		{
			ExpectSourceStep();
			return;
		}

		if (m_Phase == S_SEM_SRC_ADD)
		{
			ExpectAddStep();
			return;
		}

		if (m_Phase == S_SEM_SRC_ACT)
		{
			ActualStep(m_Src, S_SEM_SRC_CMP);
			return;
		}

		if (m_Phase == S_SEM_SRC_CMP)
		{
			if (CompareStep())
			{
				BeginTargetSemantics();
			}

			return;
		}

		if (m_Phase == S_SEM_TGT)
		{
			ExpectTargetStep();
			return;
		}

		if (m_Phase == S_SEM_TGT_COPY)
		{
			ExpectCopyStep();
			return;
		}

		if (m_Phase == S_SEM_TGT_ACT)
		{
			ActualStep(m_Tgt, S_SEM_TGT_CMP);
			return;
		}

		if (m_Phase == S_SEM_TGT_CMP)
		{
			if (CompareStep())
			{
				BeginOverride();
			}

			return;
		}

		if (m_Phase == S_OVR_LOCAL)
		{
			OverrideLocalStep();
			return;
		}

		if (m_Phase == S_OVR_FILES)
		{
			OverrideFilesStep();
			return;
		}

		if (m_Phase == S_OVR_DECIDE)
		{
			OverrideDecideStep();
			return;
		}

		if (m_Phase == S_RECHECK)
		{
			RecheckRevisions();
			return;
		}

		if (m_Phase == S_WRITE_TGT)
		{
			WriteTarget();
			return;
		}

		if (m_Phase == S_WRITE_SRC)
		{
			WriteSource();
			return;
		}

		if (m_Phase == S_FINISH)
		{
			Finish();
			return;
		}

		m_Done = true;
	}

	// ---------------------------------------------------------------- 1. validation (before any IO)

	protected void ValidateStep()
	{
		if (m_Cursor == 0 && !m_File)
		{
			if (!ValidateHeader())
			{
				return;
			}
		}

		array<ref VPPXETypeEdit> edits = m_Batch.Edits;
		if (m_Cursor >= edits.Count())
		{
			m_Phase = S_READ;
			m_Cursor = 0;
			return;
		}

		int idx = m_Cursor;
		m_Cursor++;
		ValidateEdit(edits.Get(idx), idx);
	}

	protected bool ValidateHeader()
	{
		if (!m_Batch.Edits || m_Batch.Edits.Count() == 0 || m_Batch.Edits.Count() > VPPXEConst.MAX_EDITS_PER_BATCH)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", m_Batch.FileKey);
			return false;
		}

		VPPXEFileEntry entry = Reg().Find(m_Batch.FileKey);
		if (!entry || entry.Kind != VPPXEFileKind.TYPES)
		{
			FailSave("#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return false;
		}

		if ((entry.Flags & VPPXEFileFlag.EDITABLE) == 0)
		{
			FailSave("#VSTR_XMLE_ERR_READONLY_FILE", "");
			return false;
		}

		m_File = entry;
		m_Limits = Types().GetLimits();
		m_Reason = m_Batch.Reason;
		if (m_Reason != VPPXEBackupReason.EDIT && m_Reason != VPPXEBackupReason.BULK && m_Reason != VPPXEBackupReason.STRUCT)
		{
			m_Reason = VPPXEBackupReason.EDIT;
		}

		return true;
	}

	protected void ValidateEdit(VPPXETypeEdit edit, int idx)
	{
		if (!edit)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		int op = edit.Op;
		if (op < VPPXEOp.UPDATE || op > VPPXEOp.MOVE)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return;
		}

		if (edit.Name == "" || edit.Name.Length() > VPPXEConst.MAX_NAME_LENGTH)
		{
			FailSave("#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(edit.Name, VPPXEConst.MAX_NAME_LENGTH));
			return;
		}

		if (op == VPPXEOp.ADD && !VPPXmlText.IsValidClassName(edit.Name))
		{
			FailSave("#VSTR_XMLE_ERR_NAME_INVALID", edit.Name);
			return;
		}

		// One op per name per batch: Name for every op plus NewName for RENAME/DUPLICATE.
		string lower = edit.Name;
		lower.ToLower();
		if (m_Touched.Contains(lower))
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return;
		}

		m_Touched.Set(lower, true);
		m_EditByName.Set(lower, idx);
		if (op == VPPXEOp.RENAME || op == VPPXEOp.DUPLICATE)
		{
			if (!VPPXmlText.IsValidClassName(edit.NewName))
			{
				FailSave("#VSTR_XMLE_ERR_NAME_INVALID", VPPXmlText.Clip(edit.NewName, VPPXEConst.MAX_NAME_LENGTH));
				return;
			}

			string newLower = edit.NewName;
			newLower.ToLower();
			if (m_Touched.Contains(newLower))
			{
				FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.NewName);
				return;
			}

			m_Touched.Set(newLower, true);
			AddResultName(newLower, edit.NewName, m_Batch.FileKey, "", idx);
		}

		if (op == VPPXEOp.ADD)
		{
			AddResultName(lower, edit.Name, m_Batch.FileKey, "", idx);
		}

		if (op == VPPXEOp.COPY || op == VPPXEOp.MOVE)
		{
			if (!ValidateTarget(edit))
			{
				return;
			}

			string excluded = "";
			if (op == VPPXEOp.MOVE)
			{
				excluded = m_Batch.FileKey;
			}

			AddResultName(lower, edit.Name, edit.TargetFileKey, excluded, idx);
		}

		if (op == VPPXEOp.UPDATE)
		{
			ValidateUpdateValues(edit);
		}
	}

	protected bool ValidateTarget(VPPXETypeEdit edit)
	{
		string targetKey = edit.TargetFileKey;
		if (targetKey == m_Batch.FileKey)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return false;
		}

		if (m_TargetKey != "" && m_TargetKey != targetKey)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return false;
		}

		VPPXEFileEntry target = Reg().Find(targetKey);
		if (!target || target.Kind != VPPXEFileKind.TYPES)
		{
			FailSave("#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return false;
		}

		if ((target.Flags & VPPXEFileFlag.EDITABLE) == 0)
		{
			FailSave("#VSTR_XMLE_ERR_READONLY_FILE", "");
			return false;
		}

		m_TargetKey = targetKey;
		m_TargetEntry = target;
		m_CopyEdits.Insert(edit);
		return true;
	}

	protected void AddResultName(string lowerName, string displayName, string recvKey, string exclKey, int editIdx)
	{
		m_RNames.Insert(lowerName);
		m_RDisplay.Insert(displayName);
		m_RRecv.Insert(recvKey);
		m_RExcl.Insert(exclKey);
		m_REdit.Insert(editIdx);
		m_ResultSet.Set(lowerName, true);
	}

	// Numbers (no negatives, quant -1 or 0..100), flag bits, known category/usage/value/tag names, modes.
	protected void ValidateUpdateValues(VPPXETypeEdit edit)
	{
		// The writer and VPPXETypeMerge.ApplyEdit disagree on these (set-vs-clear precedence, FLAGS clear), so
		// the semantic verification could never match: reject them up front.
		if ((edit.SetMask & edit.ClearMask) != 0)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return;
		}

		if ((edit.ClearMask & VPPXEField.FLAGS) != 0)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return;
		}

		int setMask = edit.SetMask;
		if (!CheckScalar(edit, setMask, VPPXEField.NOMINAL, edit.Nominal, false))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.MIN, edit.Min, false))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.LIFETIME, edit.Lifetime, false))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.RESTOCK, edit.Restock, false))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.COST, edit.Cost, false))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.QUANTMIN, edit.QuantMin, true))
		{
			return;
		}

		if (!CheckScalar(edit, setMask, VPPXEField.QUANTMAX, edit.QuantMax, true))
		{
			return;
		}

		if ((setMask & VPPXEField.FLAGS) != 0 && (edit.FlagsValue & ~63) != 0)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name + ": flags");
			return;
		}

		if ((setMask & VPPXEField.CATEGORY) != 0 && m_Limits.FindCategory(edit.Category) < 0)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name + ": category " + edit.Category);
			return;
		}

		if (!CheckMode(edit, edit.UsageMode) || !CheckMode(edit, edit.ValueMode) || !CheckMode(edit, edit.TagMode))
		{
			return;
		}

		if (edit.UsageMode == VPPXEListMode.REPLACE)
		{
			if (!CheckNames(edit, edit.Usages, m_Limits.Usages, "usage") || !CheckNames(edit, edit.UsageUsers, m_Limits.UsageGroups, "usage @"))
			{
				return;
			}
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE)
		{
			if (!CheckNames(edit, edit.Values, m_Limits.Values, "value") || !CheckNames(edit, edit.ValueUsers, m_Limits.ValueGroups, "value @"))
			{
				return;
			}
		}

		if (edit.TagMode == VPPXEListMode.REPLACE)
		{
			CheckNames(edit, edit.Tags, m_Limits.Tags, "tag");
		}
	}

	protected bool CheckScalar(VPPXETypeEdit edit, int setMask, int fieldBit, int value, bool isQuant)
	{
		if ((setMask & fieldBit) == 0)
		{
			return true;
		}

		bool ok = value >= 0;
		if (isQuant)
		{
			ok = value == -1 || (value >= 0 && value <= 100);
		}

		if (!ok)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name + ": " + VPPXETypesWriter.ElementNameOf(fieldBit));
			return false;
		}

		return true;
	}

	protected bool CheckMode(VPPXETypeEdit edit, int mode)
	{
		if (mode != VPPXEListMode.KEEP && mode != VPPXEListMode.REPLACE)
		{
			FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
			return false;
		}

		return true;
	}

	protected bool CheckNames(VPPXETypeEdit edit, array<string> names, array<string> known, string label)
	{
		if (!names)
		{
			return true;
		}

		foreach (string listName : names)
		{
			if (VPPXELimits.FindIn(known, listName) < 0)
			{
				FailSave("#VSTR_XMLE_ERR_VALIDATION", edit.Name + ": " + label + " " + listName);
				return false;
			}
		}

		return true;
	}

	// ---------------------------------------------------------------- 2. read and stale check

	protected void ReadSource()
	{
		string content;
		if (!VPPXmlText.ReadAll(m_File.Path, content))
		{
			Reg().NoteRevision(m_File.Key, 0, false);
			FailSave("#VSTR_XMLE_ERR_READ", "");
			return;
		}

		int rev = VPPXmlText.NormalizedHash(content);
		Reg().NoteRevision(m_File.Key, rev, true);
		if (rev != m_Batch.BaseRevision)
		{
			Types().ReindexFile(m_File.Key);
			FailSave("#VSTR_XMLE_ERR_STALE", m_File.Key);
			return;
		}

		m_Src = new VPPXESaveFileWork(m_File, content, rev, Reg().IndexOf(m_File.Key), m_Limits);
		m_Src.Edits = m_Batch.Edits;
		m_Phase = S_SRC;
	}

	// ---------------------------------------------------------------- COPY/MOVE target

	protected void PrepareTarget()
	{
		if (!m_TargetEntry)
		{
			FailSave("#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		string content;
		if (!VPPXmlText.ReadAll(m_TargetEntry.Path, content))
		{
			Reg().NoteRevision(m_TargetEntry.Key, 0, false);
			FailSave("#VSTR_XMLE_ERR_READ", "");
			return;
		}

		int rev = VPPXmlText.NormalizedHash(content);
		Reg().NoteRevision(m_TargetEntry.Key, rev, true);
		m_Tgt = new VPPXESaveFileWork(m_TargetEntry, content, rev, Reg().IndexOf(m_TargetEntry.Key), m_Limits);
		m_Tgt.CopyEdits = m_CopyEdits;
		m_Tgt.CopyBlocks = m_CopyBlocks;
		m_Cursor = 0;
		m_Phase = S_TGT_BLOCKS;
	}

	// One COPY/MOVE edit per call: the raw block of the last source occurrence (LastIndexStep refuses names defined
	// more than once in the source, so this block is the whole definition).
	protected void TargetBlockStep()
	{
		if (m_Cursor >= m_CopyEdits.Count())
		{
			m_Phase = S_TGT;
			return;
		}

		VPPXETypeEdit edit = m_CopyEdits.Get(m_Cursor);
		m_Cursor++;
		string lower = edit.Name;
		lower.ToLower();
		array<string> block = m_Src.Writer.ExtractBlock(lower);
		if (!block)
		{
			FailSave("#VSTR_XMLE_ERR_NOT_FOUND", edit.Name);
			return;
		}

		m_CopyBlocks.Insert(block);
	}

	// ---------------------------------------------------------------- 5. semantic verification

	// One old source row per call: the last occurrence of each name. A repeated name whose edit would lose merged
	// fields (see LosesMergedFields) fails the batch here, before any write.
	protected void LastIndexStep()
	{
		array<ref VPPXETypeRow> rows = m_Src.OldParse.GetRows();
		if (m_Cursor >= rows.Count())
		{
			m_Cursor = 0;
			m_Expected = new array<string>();
			m_Actual = new array<string>();
			if (m_Src.Changed)
			{
				m_Phase = S_SEM_SRC;
			}
			else
			{
				BeginTargetSemantics();
			}

			return;
		}

		string lower = rows.Get(m_Cursor).Name;
		lower.ToLower();
		if (m_LastIdx.Contains(lower) && m_EditByName.Contains(lower))
		{
			VPPXETypeRow earlier = rows.Get(m_LastIdx.Get(lower));
			VPPXETypeEdit dupEdit = m_Batch.Edits.Get(m_EditByName.Get(lower));
			if (LosesMergedFields(earlier, dupEdit))
			{
				FailSave("#VSTR_XMLE_ERR_DUP_IN_FILE", dupEdit.Name);
				return;
			}
		}

		m_LastIdx.Set(lower, m_Cursor);
		m_Cursor++;
	}

	// CE merges repeated definitions within one file (DIST-ALGO R3), but COPY/MOVE/DUPLICATE copy only the last
	// occurrence and UPDATE edits only the last one. True when the edit would drop, or fail to reach, a field that
	// the earlier occurrence contributes to the merged row; the admin must resolve the duplicates first.
	protected bool LosesMergedFields(VPPXETypeRow earlier, VPPXETypeEdit edit)
	{
		int op = edit.Op;
		if (op == VPPXEOp.COPY || op == VPPXEOp.MOVE || op == VPPXEOp.DUPLICATE)
		{
			return true;
		}

		if (op != VPPXEOp.UPDATE)
		{
			return false;
		}

		if ((edit.ClearMask & earlier.Present) != 0 || !m_Limits)
		{
			return true;
		}

		// A list REPLACE that resolves to no known bit does not replace the earlier list in the merge.
		VPPXETypeRow probe = VPPXETypeMerge.Copy(earlier);
		VPPXETypeMerge.ApplyEdit(probe, edit, m_Limits);
		if (edit.UsageMode == VPPXEListMode.REPLACE && m_Limits.EffectiveUsage(probe.Usage, probe.UsageUser) == 0 && m_Limits.EffectiveUsage(earlier.Usage, earlier.UsageUser) != 0)
		{
			return true;
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE && m_Limits.EffectiveValue(probe.Value, probe.ValueUser) == 0 && m_Limits.EffectiveValue(earlier.Value, earlier.ValueUser) != 0)
		{
			return true;
		}

		if (edit.TagMode == VPPXEListMode.REPLACE && probe.Tag == 0 && earlier.Tag != 0)
		{
			return true;
		}

		return false;
	}

	protected static string SemKey(string typeName, VPPXETypeRow row, string unknownSig)
	{
		return typeName + "|" + CanonSig(row) + "|" + unknownSig;
	}

	// Values of present fields only (robust against values left behind in absent fields).
	static string CanonSig(VPPXETypeRow r)
	{
		string s = SigScalar(r, VPPXEField.NOMINAL) + "|" + SigScalar(r, VPPXEField.MIN) + "|" + SigScalar(r, VPPXEField.LIFETIME) + "|" + SigScalar(r, VPPXEField.RESTOCK);
		s += "|" + SigScalar(r, VPPXEField.COST) + "|" + SigScalar(r, VPPXEField.QUANTMIN) + "|" + SigScalar(r, VPPXEField.QUANTMAX);
		string flagsText = "-";
		if ((r.Present & VPPXEField.FLAGS) != 0)
		{
			flagsText = r.Flags.ToString();
		}

		string catText = "-";
		if ((r.Present & VPPXEField.CATEGORY) != 0)
		{
			catText = r.Category.ToString();
		}

		s += "|" + flagsText + "|" + catText + "|" + r.Usage.ToString() + "/" + r.UsageUser.ToString();
		s += "|" + r.Value.ToString() + "/" + r.ValueUser.ToString() + "|" + r.Tag.ToString();
		return s;
	}

	protected static string SigScalar(VPPXETypeRow r, int fieldBit)
	{
		if ((r.Present & fieldBit) == 0)
		{
			return "-";
		}

		int v = VPPXETypeMerge.GetScalar(r, fieldBit);
		return v.ToString();
	}

	// Expected unknown-refs signature after an UPDATE: category refs vanish when the category is set or
	// cleared; list refs vanish on REPLACE without KeepUnknown.
	protected string FilterUsig(array<string> refs, VPPXETypeEdit edit)
	{
		if (!refs || refs.Count() == 0)
		{
			return "";
		}

		bool catTouched = ((edit.SetMask | edit.ClearMask) & VPPXEField.CATEGORY) != 0;
		bool dropUsage = !edit.KeepUnknown && edit.UsageMode == VPPXEListMode.REPLACE;
		bool dropValue = !edit.KeepUnknown && edit.ValueMode == VPPXEListMode.REPLACE;
		bool dropTag = !edit.KeepUnknown && edit.TagMode == VPPXEListMode.REPLACE;
		array<string> kept = new array<string>();
		foreach (string rowRef : refs)
		{
			bool drop = false;
			if (catTouched && rowRef.IndexOf("category|") == 0)
			{
				drop = true;
			}

			if (dropUsage && (rowRef.IndexOf("usage|") == 0 || rowRef.IndexOf("usageuser|") == 0))
			{
				drop = true;
			}

			if (dropValue && (rowRef.IndexOf("value|") == 0 || rowRef.IndexOf("valueuser|") == 0))
			{
				drop = true;
			}

			if (dropTag && rowRef.IndexOf("tag|") == 0)
			{
				drop = true;
			}

			if (!drop)
			{
				kept.Insert(rowRef);
			}
		}

		return VPPXETypesRowSink.JoinRefs(kept);
	}

	// One old source row per call: the expected occurrence after the batch.
	protected void ExpectSourceStep()
	{
		array<ref VPPXETypeRow> rows = m_Src.OldParse.GetRows();
		if (m_Cursor >= rows.Count())
		{
			m_Cursor = 0;
			m_Phase = S_SEM_SRC_ADD;
			return;
		}

		int ri = m_Cursor;
		m_Cursor++;
		VPPXETypeRow oldRow = rows.Get(ri);
		string usig = m_Src.OldParse.GetUnknownSigs().Get(ri);
		string lower = oldRow.Name;
		lower.ToLower();
		if (!m_EditByName.Contains(lower))
		{
			m_Expected.Insert(SemKey(oldRow.Name, oldRow, usig));
			return;
		}

		VPPXETypeEdit edit = m_Batch.Edits.Get(m_EditByName.Get(lower));
		bool isLast = m_LastIdx.Get(lower) == ri;
		int op = edit.Op;
		if (op == VPPXEOp.DELETE || op == VPPXEOp.MOVE)
		{
			return;
		}

		if (op == VPPXEOp.RENAME)
		{
			m_Expected.Insert(SemKey(edit.NewName, oldRow, usig));
			return;
		}

		if (op == VPPXEOp.UPDATE && isLast)
		{
			VPPXETypeRow applied = VPPXETypeMerge.Copy(oldRow);
			VPPXETypeMerge.ApplyEdit(applied, edit, m_Limits);
			array<string> refs = m_Src.OldParse.GetUnknownRefs().Get(ri);
			m_Expected.Insert(SemKey(oldRow.Name, applied, FilterUsig(refs, edit)));
			return;
		}

		m_Expected.Insert(SemKey(oldRow.Name, oldRow, usig));
		if (op == VPPXEOp.DUPLICATE && isLast)
		{
			m_Expected.Insert(SemKey(edit.NewName, oldRow, usig));
		}
	}

	// One edit per call: ADD adds the template occurrence.
	protected void ExpectAddStep()
	{
		if (m_Cursor >= m_Batch.Edits.Count())
		{
			m_Cursor = 0;
			m_Phase = S_SEM_SRC_ACT;
			return;
		}

		VPPXETypeEdit edit = m_Batch.Edits.Get(m_Cursor);
		m_Cursor++;
		if (edit.Op == VPPXEOp.ADD)
		{
			m_Expected.Insert(SemKey(edit.Name, VPPXETypeMerge.TemplateRow(edit.Name), ""));
		}
	}

	// One new row per call.
	protected void ActualStep(VPPXESaveFileWork work, int nextPhase)
	{
		array<ref VPPXETypeRow> rows = work.NewParse.GetRows();
		if (m_Cursor >= rows.Count())
		{
			m_Cursor = 0;
			m_Expected.Sort();
			m_Actual.Sort();
			m_Phase = nextPhase;
			return;
		}

		VPPXETypeRow row = rows.Get(m_Cursor);
		string usig = work.NewParse.GetUnknownSigs().Get(m_Cursor);
		m_Actual.Insert(SemKey(row.Name, row, usig));
		m_Cursor++;
	}

	// Compares the sorted lists one entry per call; true when the lists are identical.
	protected bool CompareStep()
	{
		if (m_Expected.Count() != m_Actual.Count())
		{
			FailSave("#VSTR_XMLE_ERR_INTERNAL", FirstMismatchName());
			return false;
		}

		if (m_Cursor >= m_Expected.Count())
		{
			m_Cursor = 0;
			return true;
		}

		if (m_Expected.Get(m_Cursor) != m_Actual.Get(m_Cursor))
		{
			FailSave("#VSTR_XMLE_ERR_INTERNAL", FirstMismatchName());
			return false;
		}

		m_Cursor++;
		return false;
	}

	// Bounded by the payload: stops at the first differing sorted entry.
	protected string FirstMismatchName()
	{
		int count = m_Expected.Count();
		if (m_Actual.Count() < count)
		{
			count = m_Actual.Count();
		}

		string mismatch = "";
		for (int i = m_Cursor; i < count; i++)
		{
			if (m_Expected.Get(i) != m_Actual.Get(i))
			{
				mismatch = m_Expected.Get(i);
				break;
			}

			if (i - m_Cursor > 2000)
			{
				break;
			}
		}

		if (mismatch == "" && m_Expected.Count() > count)
		{
			mismatch = m_Expected.Get(count);
		}

		if (mismatch == "" && m_Actual.Count() > count)
		{
			mismatch = m_Actual.Get(count);
		}

		int bar = mismatch.IndexOf("|");
		if (bar > 0)
		{
			return mismatch.Substring(0, bar);
		}

		return "verify";
	}

	protected void BeginTargetSemantics()
	{
		m_Cursor = 0;
		m_Expected = new array<string>();
		m_Actual = new array<string>();
		if (m_Tgt && m_Tgt.Changed)
		{
			m_Phase = S_SEM_TGT;
			return;
		}

		BeginOverride();
	}

	// Old target rows stay unchanged.
	protected void ExpectTargetStep()
	{
		array<ref VPPXETypeRow> rows = m_Tgt.OldParse.GetRows();
		if (m_Cursor >= rows.Count())
		{
			m_Cursor = 0;
			m_Phase = S_SEM_TGT_COPY;
			return;
		}

		VPPXETypeRow row = rows.Get(m_Cursor);
		m_Expected.Insert(SemKey(row.Name, row, m_Tgt.OldParse.GetUnknownSigs().Get(m_Cursor)));
		m_Cursor++;
	}

	// Each COPY/MOVE adds the last source occurrence (as written in the source).
	protected void ExpectCopyStep()
	{
		if (m_Cursor >= m_CopyEdits.Count())
		{
			m_Cursor = 0;
			m_Phase = S_SEM_TGT_ACT;
			return;
		}

		VPPXETypeEdit edit = m_CopyEdits.Get(m_Cursor);
		m_Cursor++;
		string lower = edit.Name;
		lower.ToLower();
		if (!m_LastIdx.Contains(lower))
		{
			FailSave("#VSTR_XMLE_ERR_NOT_FOUND", edit.Name);
			return;
		}

		int ri = m_LastIdx.Get(lower);
		VPPXETypeRow srcRow = m_Src.OldParse.GetRows().Get(ri);
		m_Expected.Insert(SemKey(srcRow.Name, srcRow, m_Src.OldParse.GetUnknownSigs().Get(ri)));
	}

	// ---------------------------------------------------------------- override rule (checked against the disk)

	protected void BeginOverride()
	{
		m_Expected = null;
		m_Actual = null;
		m_Cursor = 0;
		m_Cursor2 = 0;
		if (m_RNames.Count() == 0)
		{
			m_Phase = S_RECHECK;
			return;
		}

		array<VPPXEFileEntry> typesFiles = new array<VPPXEFileEntry>();
		Reg().GetByKind(VPPXEFileKind.TYPES, typesFiles);
		foreach (VPPXEFileEntry entry : typesFiles)
		{
			if (entry.Key != m_Batch.FileKey && entry.Key != m_TargetKey)
			{
				m_OtherKeys.Insert(entry.Key);
			}
		}

		m_OvrIdx = 0;
		m_OvrMode = 0;
		m_Phase = S_OVR_LOCAL;
	}

	protected void AddDefiner(string lowerName, string fileKey)
	{
		array<string> keys = m_Definers.Get(lowerName);
		if (!keys)
		{
			keys = new array<string>();
			m_Definers.Set(lowerName, keys);
		}

		if (keys.Find(fileKey) < 0)
		{
			keys.Insert(fileKey);
		}
	}

	// The batch file and the target file: their freshly parsed old rows decide.
	protected void OverrideLocalStep()
	{
		array<ref VPPXETypeRow> srcRows = m_Src.OldParse.GetRows();
		if (m_Cursor < srcRows.Count())
		{
			string srcLower = srcRows.Get(m_Cursor).Name;
			srcLower.ToLower();
			if (m_ResultSet.Contains(srcLower))
			{
				AddDefiner(srcLower, m_File.Key);
			}

			m_Cursor++;
			return;
		}

		if (m_Tgt)
		{
			array<ref VPPXETypeRow> tgtRows = m_Tgt.OldParse.GetRows();
			if (m_Cursor2 < tgtRows.Count())
			{
				string tgtLower = tgtRows.Get(m_Cursor2).Name;
				tgtLower.ToLower();
				if (m_ResultSet.Contains(tgtLower))
				{
					AddDefiner(tgtLower, m_TargetKey);
				}

				m_Cursor2++;
				return;
			}
		}

		m_Phase = S_OVR_FILES;
	}

	// Every other TYPES file: an unchanged file uses the index, a changed one is parsed here (and reindexed).
	protected void OverrideFilesStep()
	{
		if (m_OvrMode == 0)
		{
			if (m_OvrIdx >= m_OtherKeys.Count())
			{
				m_Cursor = 0;
				m_Phase = S_OVR_DECIDE;
				return;
			}

			m_OvrKey = m_OtherKeys.Get(m_OvrIdx);
			m_OvrIdx++;
			VPPXEFileEntry entry = Reg().Find(m_OvrKey);
			if (!entry || (entry.Flags & VPPXEFileFlag.EXISTS) == 0 || (entry.Flags & VPPXEFileFlag.READ_ERROR) != 0)
			{
				return;
			}

			string content;
			if (!VPPXmlText.ReadAll(entry.Path, content))
			{
				return;
			}

			int rev = VPPXmlText.NormalizedHash(content);
			Reg().NoteRevision(m_OvrKey, rev, true);
			if (Types().IsReady() && Types().GetFileRevision(m_OvrKey) == rev)
			{
				m_OvrCursor = 0;
				m_OvrMode = 1;
				return;
			}

			m_OvrParse = new VPPXETypesParseJob(content, Reg().IndexOf(m_OvrKey), m_Limits);
			Types().ReindexFile(m_OvrKey);
			m_OvrMode = 2;
			return;
		}

		if (m_OvrMode == 1)
		{
			if (m_OvrCursor >= m_RNames.Count())
			{
				m_OvrMode = 0;
				return;
			}

			string lowerName = m_RNames.Get(m_OvrCursor);
			m_OvrCursor++;
			if (Types().FileDefinesName(m_OvrKey, lowerName))
			{
				AddDefiner(lowerName, m_OvrKey);
			}

			return;
		}

		if (m_OvrMode == 2)
		{
			m_OvrParse.Step();
			if (!m_OvrParse.IsDone())
			{
				return;
			}

			if (m_OvrParse.Failed())
			{
				m_OvrParse = null;
				m_OvrMode = 0;
				return;
			}

			m_OvrCursor = 0;
			m_OvrMode = 3;
			return;
		}

		array<ref VPPXETypeRow> rows = m_OvrParse.GetRows();
		if (m_OvrCursor >= rows.Count())
		{
			m_OvrParse = null;
			m_OvrMode = 0;
			return;
		}

		string rowLower = rows.Get(m_OvrCursor).Name;
		rowLower.ToLower();
		m_OvrCursor++;
		if (m_ResultSet.Contains(rowLower))
		{
			AddDefiner(rowLower, m_OvrKey);
		}
	}

	// others = files defining the resulting name, minus the receiving file (and the source for MOVE).
	protected void OverrideDecideStep()
	{
		if (m_Cursor >= m_RNames.Count())
		{
			m_Phase = S_RECHECK;
			return;
		}

		int i = m_Cursor;
		m_Cursor++;
		array<string> defs = m_Definers.Get(m_RNames.Get(i));
		if (!defs)
		{
			return;
		}

		bool others = false;
		foreach (string defKey : defs)
		{
			if (defKey != m_RRecv.Get(i) && defKey != m_RExcl.Get(i))
			{
				others = true;
			}
		}

		if (!others)
		{
			return;
		}

		VPPXETypeEdit edit = m_Batch.Edits.Get(m_REdit.Get(i));
		if (!edit.AllowOverride)
		{
			FailSave("#VSTR_XMLE_ERR_NAME_OTHER_FILE", m_RDisplay.Get(i));
		}
	}

	// ---------------------------------------------------------------- 6/7. re-check, backup, write

	protected void RecheckRevisions()
	{
		string current;
		if (!VPPXmlText.ReadAll(m_File.Path, current))
		{
			Reg().NoteRevision(m_File.Key, 0, false);
			FailSave("#VSTR_XMLE_ERR_READ", "");
			return;
		}

		int rev = VPPXmlText.NormalizedHash(current);
		if (rev != m_Src.Revision)
		{
			Reg().NoteRevision(m_File.Key, rev, true);
			Types().ReindexFile(m_File.Key);
			FailSave("#VSTR_XMLE_ERR_STALE", m_File.Key);
			return;
		}

		if (m_Tgt)
		{
			string targetCurrent;
			if (!VPPXmlText.ReadAll(m_TargetEntry.Path, targetCurrent))
			{
				Reg().NoteRevision(m_TargetKey, 0, false);
				FailSave("#VSTR_XMLE_ERR_READ", "");
				return;
			}

			int targetRev = VPPXmlText.NormalizedHash(targetCurrent);
			if (targetRev != m_Tgt.Revision)
			{
				Reg().NoteRevision(m_TargetKey, targetRev, true);
				Types().ReindexFile(m_TargetKey);
				FailSave("#VSTR_XMLE_ERR_STALE", m_TargetKey);
				return;
			}
		}

		m_Summary = BuildSummary();
		BuildTouched();
		m_Phase = S_WRITE_TGT;
	}

	// CreateBackup returns "" when the file changed on disk after it was read: report that as STALE (the
	// client refreshes the file named by ErrorArg) instead of a backup failure. False when the file still matches.
	protected bool FailIfChangedOnDisk(VPPXEFileEntry entry, int expectedRev)
	{
		string current;
		if (!VPPXmlText.ReadAll(entry.Path, current))
		{
			Reg().NoteRevision(entry.Key, 0, false);
			Types().ReindexFile(entry.Key);
			FailSave("#VSTR_XMLE_ERR_STALE", entry.Key);
			return true;
		}

		int rev = VPPXmlText.NormalizedHash(current);
		if (rev == expectedRev)
		{
			return false;
		}

		Reg().NoteRevision(entry.Key, rev, true);
		Types().ReindexFile(entry.Key);
		FailSave("#VSTR_XMLE_ERR_STALE", entry.Key);
		return true;
	}

	protected void WriteTarget()
	{
		if (!m_Tgt || !m_Tgt.Changed)
		{
			m_Phase = S_WRITE_SRC;
			return;
		}

		PlayerIdentity who = Requester();
		m_TgtBackupId = m_Owner.GetBackups().CreateBackup(m_TargetEntry, m_Tgt.Content, m_Reason, who, m_Summary, m_Batch.Edits.Count(), m_TouchedNames, m_Batch.Note);
		if (m_TgtBackupId == "")
		{
			if (!FailIfChangedOnDisk(m_TargetEntry, m_Tgt.Revision))
			{
				FailSave("#VSTR_XMLE_ERR_BACKUP", "");
			}

			return;
		}

		string errKey;
		string errArg;
		string noticeKey;
		if (!VPPXESafeWrite.Write(m_TargetEntry, m_Tgt.NewContent, m_TgtBackupId, m_Tgt.Content, true, errKey, errArg, noticeKey))
		{
			FailSave(errKey, errArg);
			return;
		}

		m_TgtWritten = true;
		if (noticeKey != "")
		{
			m_NoticeKey = noticeKey;
		}

		m_Owner.GetBackups().SetResultHash(m_TgtBackupId, VPPXmlText.NormalizedHash(m_Tgt.NewContent));
		m_Phase = S_WRITE_SRC;
	}

	protected void WriteSource()
	{
		if (!m_Src.Changed)
		{
			m_Phase = S_FINISH;
			return;
		}

		PlayerIdentity who = Requester();
		m_SrcBackupId = m_Owner.GetBackups().CreateBackup(m_File, m_Src.Content, m_Reason, who, m_Summary, m_Batch.Edits.Count(), m_TouchedNames, m_Batch.Note);
		if (m_SrcBackupId == "")
		{
			RevertTarget();
			if (!FailIfChangedOnDisk(m_File, m_Src.Revision))
			{
				FailSave("#VSTR_XMLE_ERR_BACKUP", "");
			}

			return;
		}

		string errKey;
		string errArg;
		string noticeKey;
		if (!VPPXESafeWrite.Write(m_File, m_Src.NewContent, m_SrcBackupId, m_Src.Content, true, errKey, errArg, noticeKey))
		{
			RevertTarget();
			FailSave(errKey, errArg);
			return;
		}

		m_SrcWritten = true;
		if (noticeKey != "")
		{
			m_NoticeKey = noticeKey;
		}

		m_Owner.GetBackups().SetResultHash(m_SrcBackupId, VPPXmlText.NormalizedHash(m_Src.NewContent));
		m_Phase = S_FINISH;
	}

	// The source write failed after the target was written: write the target back to its old content.
	// The target is written back only while it still holds what this job wrote: an external edit made since
	// WriteTarget (an earlier pump frame) is in no backup, so it is left as is instead of being overwritten.
	protected void RevertTarget()
	{
		if (!m_TgtWritten)
		{
			return;
		}

		m_TgtWritten = false;
		string current;
		bool readOk = VPPXmlText.ReadAll(m_TargetEntry.Path, current);
		if (!readOk || !VPPXmlText.SameNormalized(current, m_Tgt.NewContent))
		{
			if (readOk)
			{
				Reg().NoteRevision(m_TargetKey, VPPXmlText.NormalizedHash(current), true);
			}
			else
			{
				Reg().NoteRevision(m_TargetKey, 0, false);
			}

			Types().ReindexFile(m_TargetKey);
			VPPXELog.Warn("Did not revert " + m_TargetKey + " after a failed save of " + m_File.Key + ": the file changed on disk after the save wrote it and was left as is");
			m_Owner.OnFileWritten(m_TargetKey, Requester());
			return;
		}

		string errKey;
		string errArg;
		string noticeKey;
		bool reverted = VPPXESafeWrite.Write(m_TargetEntry, m_Tgt.Content, m_TgtBackupId, m_Tgt.NewContent, true, errKey, errArg, noticeKey);
		if (reverted)
		{
			// The target holds its old content again: point the backup's result hash at it so the registry does not flag an external change.
			m_Owner.GetBackups().SetResultHash(m_TgtBackupId, VPPXmlText.NormalizedHash(m_Tgt.Content));
			VPPXELog.ActionBy(m_AdminName, m_AdminId, "reverted " + m_TargetKey + " after the write of " + m_File.Key + " failed", false);
		}
		else
		{
			VPPXELog.Warn("Could not revert " + m_TargetKey + " after a failed save of " + m_File.Key + ": " + errKey + " " + errArg);
		}

		m_Owner.OnFileWritten(m_TargetKey, Requester());
	}

	// ---------------------------------------------------------------- 8. finish

	protected void Finish()
	{
		m_Owner.GetBackups().ApplyRetention();
		VPPXESaveResult r = new VPPXESaveResult();
		r.ReqId = m_Batch.ReqId;
		r.Ok = true;
		r.ErrorKey = "";
		r.ErrorArg = "";
		r.FileKey = m_Batch.FileKey;
		r.NewRevision = m_Src.Revision;
		if (m_SrcWritten)
		{
			r.NewRevision = VPPXmlText.NormalizedHash(m_Src.NewContent);
		}

		r.BackupId = m_SrcBackupId;
		r.Applied = m_Batch.Edits.Count();
		if (m_TgtWritten)
		{
			r.TouchedFiles.Insert(m_TargetKey);
		}

		if (m_SrcWritten)
		{
			r.TouchedFiles.Insert(m_File.Key);
		}

		r.NoticeKey = m_NoticeKey;
		PlayerIdentity who = Requester();
		VPPXENet.SendNow(who, "XE_OnSaveResult", new Param1<ref VPPXESaveResult>(r));
		LogSaved();
		if (m_TgtWritten)
		{
			m_Owner.OnFileWritten(m_TargetKey, who);
		}

		if (m_SrcWritten)
		{
			m_Owner.OnFileWritten(m_File.Key, who);
		}

		m_Phase = S_DONE;
		m_Done = true;
	}

	protected void LogSaved()
	{
		int count = m_Batch.Edits.Count();
		string names = "";
		int shown = 0;
		foreach (string touchedName : m_TouchedNames)
		{
			if (shown >= 3)
			{
				break;
			}

			if (names != "")
			{
				names += ", ";
			}

			names += touchedName;
			shown++;
		}

		int more = count - shown;
		if (more > 0)
		{
			names += " +" + more.ToString();
		}

		string text = string.Format("Saved %1 change(s) to %2: %3 (backup %4)", count, m_Batch.FileKey, names, m_SrcBackupId);
		if (m_TgtWritten)
		{
			text += " and " + m_TargetKey + " (backup " + m_TgtBackupId + ")";
		}

		VPPXELog.ActionBy(m_AdminName, m_AdminId, text, true);
	}

	// e.g. "AKM nominal 6>8; +2 more", at most 200 characters.
	protected string BuildSummary()
	{
		int count = m_Batch.Edits.Count();
		string text = "";
		int shown = 0;
		for (int i = 0; i < count && i < 20; i++)
		{
			string part = DescribeEdit(m_Batch.Edits.Get(i));
			if (text.Length() + part.Length() + 2 > 170)
			{
				break;
			}

			if (text != "")
			{
				text += "; ";
			}

			text += part;
			shown++;
		}

		int more = count - shown;
		if (more > 0)
		{
			text += "; +" + more.ToString() + " more";
		}

		if (text.Length() > 200)
		{
			text = text.Substring(0, 200);
		}

		return text;
	}

	protected string DescribeEdit(VPPXETypeEdit edit)
	{
		int op = edit.Op;
		if (op == VPPXEOp.ADD)
		{
			return "+" + edit.Name;
		}

		if (op == VPPXEOp.DELETE)
		{
			return "-" + edit.Name;
		}

		if (op == VPPXEOp.RENAME)
		{
			return edit.Name + ">" + edit.NewName;
		}

		if (op == VPPXEOp.DUPLICATE)
		{
			return edit.Name + "=>" + edit.NewName;
		}

		if (op == VPPXEOp.COPY)
		{
			return edit.Name + " copy>" + edit.TargetFileKey;
		}

		if (op == VPPXEOp.MOVE)
		{
			return edit.Name + " move>" + edit.TargetFileKey;
		}

		VPPXETypeRow oldRow = null;
		string lower = edit.Name;
		lower.ToLower();
		if (m_LastIdx.Contains(lower))
		{
			oldRow = m_Src.OldParse.GetRows().Get(m_LastIdx.Get(lower));
		}

		string text = edit.Name;
		for (int si = 0; si < 7; si++)
		{
			int fieldBit = VPPXETypesWriter.CanonBitAt(si);
			string before = "-";
			if (oldRow && (oldRow.Present & fieldBit) != 0)
			{
				before = VPPXETypeMerge.GetScalar(oldRow, fieldBit).ToString();
			}

			if ((edit.SetMask & fieldBit) != 0)
			{
				text += " " + VPPXETypesWriter.ElementNameOf(fieldBit) + " " + before + ">" + VPPXETypesWriter.EditScalar(edit, fieldBit).ToString();
			}
			else if ((edit.ClearMask & fieldBit) != 0)
			{
				text += " " + VPPXETypesWriter.ElementNameOf(fieldBit) + " " + before + ">-";
			}

			if (text.Length() > 120)
			{
				return text;
			}
		}

		if (((edit.SetMask | edit.ClearMask) & VPPXEField.FLAGS) != 0)
		{
			text += " flags";
		}

		if ((edit.SetMask & VPPXEField.CATEGORY) != 0)
		{
			text += " category>" + edit.Category;
		}
		else if ((edit.ClearMask & VPPXEField.CATEGORY) != 0)
		{
			text += " category>-";
		}

		if (edit.UsageMode == VPPXEListMode.REPLACE)
		{
			text += " usage";
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE)
		{
			text += " value";
		}

		if (edit.TagMode == VPPXEListMode.REPLACE)
		{
			text += " tag";
		}

		return text;
	}

	// At most 50 names: the resulting name of structural ops, else the edited name.
	protected void BuildTouched()
	{
		m_TouchedNames = new array<string>();
		int count = m_Batch.Edits.Count();
		for (int i = 0; i < count && m_TouchedNames.Count() < 50; i++)
		{
			VPPXETypeEdit edit = m_Batch.Edits.Get(i);
			if (edit.Op == VPPXEOp.RENAME || edit.Op == VPPXEOp.DUPLICATE)
			{
				m_TouchedNames.Insert(edit.NewName);
			}
			else
			{
				m_TouchedNames.Insert(edit.Name);
			}
		}
	}

	// Replies XE_OnSaveResult (Ok false) and ends the job.
	protected void FailSave(string errKey, string errArg)
	{
		if (m_Done)
		{
			return;
		}

		m_Done = true;
		m_Phase = S_DONE;
		VPPXESaveResult r = new VPPXESaveResult();
		r.ReqId = ReqId();
		r.Ok = false;
		r.ErrorKey = errKey;
		r.ErrorArg = errArg;
		r.FileKey = m_Batch.FileKey;
		r.NewRevision = 0;
		r.BackupId = "";
		r.Applied = 0;
		r.NoticeKey = "";
		VPPXENet.SendNow(Requester(), "XE_OnSaveResult", new Param1<ref VPPXESaveResult>(r));
		string text = string.Format("save of %1 failed: %2 %3", m_Batch.FileKey, errKey, errArg);
		VPPXELog.ActionBy(m_AdminName, m_AdminId, text, false);
	}

	override void OnAborted()
	{
		VPPXENet.Result(Requester(), ReqId(), false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
		if (m_Batch)
		{
			VPPXESafeWrite.CheckJournalFor(m_Batch.FileKey);
		}

		if (m_TargetKey != "")
		{
			VPPXESafeWrite.CheckJournalFor(m_TargetKey);
		}
	}
};
