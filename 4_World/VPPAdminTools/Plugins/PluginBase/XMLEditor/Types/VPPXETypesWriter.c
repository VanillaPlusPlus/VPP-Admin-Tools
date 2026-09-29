// XML Editor types writer: budgeted patch planning from DOM node positions (never from text search), one
// native key sort, an overlap check, then a resumable forward pass that emits NEW line and CR arrays and
// records every untouched run (origStart, origEnd, outStart) for the save job's line-level verification.

class VPPXEWritePatch : Managed
{
	int StartLine;
	int StartCol;
	int EndLine;
	int EndCol;
	int PatchIdx;
	string Owner;
	ref array<string> NewLines;

	void VPPXEWritePatch(int startLine, int startCol, int endLine, int endCol, array<string> newLines, int patchIdx, string owner)
	{
		StartLine = startLine;
		StartCol = startCol;
		EndLine = endLine;
		EndCol = endCol;
		NewLines = newLines;
		PatchIdx = patchIdx;
		Owner = owner;
	}
};

class VPPXETypesWriter : Managed
{
	const static int PH_MAP = 0;
	const static int PH_PLAN = 1;
	const static int PH_KEYS = 2;
	const static int PH_SORT = 3;
	const static int PH_CHECK = 4;
	const static int PH_PLANNED = 5;
	const static int LIST_USAGE = 0;
	const static int LIST_VALUE = 1;
	const static int LIST_TAG = 2;

	protected ref VPPXmlDocument m_Doc;
	protected ref VPPXELimits m_Limits;
	protected ref array<string> m_Lines;
	protected ref array<bool> m_CR;
	protected VPPXmlNode m_Root;
	protected ref array<VPPXmlNode> m_TypeNodes;
	protected ref map<string, ref array<VPPXmlNode>> m_ByName;
	protected ref array<ref VPPXETypeEdit> m_Edits;
	protected ref array<ref VPPXETypeEdit> m_CopyEdits;
	protected ref array<ref array<string>> m_CopyBlocks;
	protected ref array<ref VPPXEWritePatch> m_Patches;
	protected ref array<string> m_SortKeys;
	protected ref array<int> m_Order;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_PrevEndLine;
	protected int m_PrevEndCol;
	protected bool m_Failed;
	protected string m_ErrKey;
	protected string m_ErrArg;
	protected string m_IndentUnit;
	protected bool m_UnitKnown;
	protected bool m_DominantCR;
	protected string m_CurOwner;

	// Forward-pass applier state.
	protected ref array<string> m_OutLines;
	protected ref array<bool> m_OutCR;
	protected ref array<int> m_Runs;
	protected int m_PCursor;
	protected int m_SrcLine;
	protected int m_SrcCol;
	protected string m_Pending;
	protected bool m_HasPending;
	protected bool m_RunOpen;
	protected int m_RunOrigStart;
	protected int m_RunOrigEnd;
	protected int m_RunOutStart;
	protected bool m_ApplyDone;

	void VPPXETypesWriter(VPPXmlDocument doc, VPPXELimits limits)
	{
		m_Doc = doc;
		m_Limits = limits;
		m_TypeNodes = new array<VPPXmlNode>();
		m_ByName = new map<string, ref array<VPPXmlNode>>();
		m_Edits = new array<ref VPPXETypeEdit>();
		m_CopyEdits = new array<ref VPPXETypeEdit>();
		m_CopyBlocks = new array<ref array<string>>();
		m_Patches = new array<ref VPPXEWritePatch>();
		m_SortKeys = new array<string>();
		m_Order = new array<int>();
		m_OutLines = new array<string>();
		m_OutCR = new array<bool>();
		m_Runs = new array<int>();
		m_IndentUnit = "    ";
		m_Pending = "";
		m_Phase = PH_MAP;
		m_Cursor = 0;
		if (!m_Doc)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", "document");
			return;
		}

		m_Lines = m_Doc.GetLines();
		m_CR = m_Doc.GetCRFlags();
		m_DominantCR = m_Doc.DominantIsCR();
		VPPXmlNode root = m_Doc.GetRoot();
		if (root && LowerName(root) != "types")
		{
			VPPXmlNode inner = root.FirstChild("types");
			if (inner)
			{
				root = inner;
			}
		}

		m_Root = root;
		if (!m_Lines || !m_CR || !m_Root || LowerName(m_Root) != "types")
		{
			Fail("#VSTR_XMLE_ERR_PARSE", "no <types> root element");
		}
	}

	// ---------------------------------------------------------------- inputs and results

	void AddEdit(VPPXETypeEdit edit)
	{
		m_Edits.Insert(edit);
	}

	// O(1) handoff of the batch edits (the array is shared, not copied).
	void SetEdits(array<ref VPPXETypeEdit> edits)
	{
		if (edits)
		{
			m_Edits = edits;
		}
	}

	// O(1) handoff of the target-side COPY/MOVE edits and their source blocks (parallel arrays).
	void SetCopies(array<ref VPPXETypeEdit> edits, array<ref array<string>> blocks)
	{
		if (edits && blocks)
		{
			m_CopyEdits = edits;
			m_CopyBlocks = blocks;
		}
	}

	// Target side of COPY/MOVE: insert the raw block of the source occurrence (see ExtractBlock).
	void AddCopyIn(VPPXETypeEdit edit, array<string> block)
	{
		m_CopyEdits.Insert(edit);
		m_CopyBlocks.Insert(block);
	}

	bool Failed()
	{
		return m_Failed;
	}

	string GetErrKey()
	{
		return m_ErrKey;
	}

	string GetErrArg()
	{
		return m_ErrArg;
	}

	int PatchCount()
	{
		return m_Patches.Count();
	}

	bool IsPlanned()
	{
		return m_Phase == PH_PLANNED;
	}

	array<string> GetNewLines()
	{
		return m_OutLines;
	}

	array<bool> GetNewCR()
	{
		return m_OutCR;
	}

	// Untouched runs as [origStart, origEnd (exclusive), outStart]*.
	array<int> GetRuns()
	{
		return m_Runs;
	}

	string GetPrefix()
	{
		return m_Doc.GetPrefix();
	}

	bool EndsWithNewline()
	{
		return m_Doc.EndsWithNewline();
	}

	// Name map lookup (valid once planning passed the map phase).
	bool HasName(string lowerName)
	{
		return m_ByName.Contains(lowerName);
	}

	// Raw lines of the LAST occurrence of a name; the first line starts at its '<'. Null when absent.
	array<string> ExtractBlock(string lowerName)
	{
		array<VPPXmlNode> nodes = m_ByName.Get(lowerName);
		if (!nodes || nodes.Count() == 0)
		{
			return null;
		}

		return ExtractBlockOf(nodes.Get(nodes.Count() - 1));
	}

	protected void Fail(string errKey, string errArg)
	{
		if (m_Failed)
		{
			return;
		}

		m_Failed = true;
		m_ErrKey = errKey;
		m_ErrArg = errArg;
	}

	// ---------------------------------------------------------------- planning

	// Loops while BudgetOk: map building one child per iteration, planning one edit per iteration, then the
	// sort keys, ONE native sort and the overlap check. Returns true when planning is finished (ok or failed).
	bool PlanStep()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Failed || m_Phase == PH_PLANNED)
			{
				return true;
			}

			if (m_Phase == PH_MAP)
			{
				MapStep();
				continue;
			}

			if (m_Phase == PH_PLAN)
			{
				PlanOneEdit();
				continue;
			}

			if (m_Phase == PH_KEYS)
			{
				KeyStep();
				continue;
			}

			if (m_Phase == PH_SORT)
			{
				m_SortKeys.Sort();
				m_Phase = PH_CHECK;
				m_Cursor = 0;
				m_PrevEndLine = -1;
				m_PrevEndCol = -1;
				continue;
			}

			if (m_Phase == PH_CHECK)
			{
				CheckStep();
				continue;
			}
		}

		if (m_Failed || m_Phase == PH_PLANNED)
		{
			return true;
		}

		return false;
	}

	protected void MapStep()
	{
		if (m_Cursor >= m_Root.ChildCount())
		{
			m_Phase = PH_PLAN;
			m_Cursor = 0;
			return;
		}

		VPPXmlNode child = m_Root.ChildAt(m_Cursor);
		m_Cursor++;
		if (!child || child.Kind != VPPXmlNodeKind.ELEMENT || LowerName(child) != "type")
		{
			return;
		}

		m_TypeNodes.Insert(child);
		if (!m_UnitKnown)
		{
			TryLearnIndentUnit(child);
		}

		string typeName = child.GetAttr("name", "");
		if (typeName == "")
		{
			return;
		}

		string lower = typeName;
		lower.ToLower();
		array<VPPXmlNode> nodes = m_ByName.Get(lower);
		if (!nodes)
		{
			nodes = new array<VPPXmlNode>();
			m_ByName.Set(lower, nodes);
		}

		nodes.Insert(child);
	}

	protected void PlanOneEdit()
	{
		int editCount = m_Edits.Count();
		if (m_Cursor < editCount)
		{
			VPPXETypeEdit edit = m_Edits.Get(m_Cursor);
			m_Cursor++;
			PlanEdit(edit);
			return;
		}

		int copyIdx = m_Cursor - editCount;
		if (copyIdx < m_CopyEdits.Count())
		{
			m_Cursor++;
			PlanCopyIn(m_CopyEdits.Get(copyIdx), m_CopyBlocks.Get(copyIdx));
			return;
		}

		m_Phase = PH_KEYS;
		m_Cursor = 0;
	}

	protected void KeyStep()
	{
		if (m_Cursor >= m_Patches.Count())
		{
			m_Phase = PH_SORT;
			return;
		}

		VPPXEWritePatch p = m_Patches.Get(m_Cursor);
		string kindDigit = "1";
		if (p.StartLine == p.EndLine && p.StartCol == p.EndCol)
		{
			kindDigit = "0";
		}

		string sortKey = VPPXmlText.PadInt(p.StartLine, 8) + VPPXmlText.PadInt(p.StartCol, 6) + kindDigit + VPPXmlText.PadInt(m_Cursor, 6);
		m_SortKeys.Insert(sortKey);
		m_Cursor++;
	}

	protected void CheckStep()
	{
		if (m_Cursor >= m_SortKeys.Count())
		{
			m_Phase = PH_PLANNED;
			InitApply();
			return;
		}

		string sortKey = m_SortKeys.Get(m_Cursor);
		m_Cursor++;
		int idx = sortKey.Substring(sortKey.Length() - 6, 6).ToInt();
		VPPXEWritePatch p = m_Patches.Get(idx);
		if (p.StartLine < m_PrevEndLine || (p.StartLine == m_PrevEndLine && p.StartCol < m_PrevEndCol))
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", p.Owner);
			return;
		}

		m_Order.Insert(idx);
		m_PrevEndLine = p.EndLine;
		m_PrevEndCol = p.EndCol;
	}

	protected void PlanEdit(VPPXETypeEdit edit)
	{
		if (!edit)
		{
			Fail("#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		m_CurOwner = edit.Name;
		string lower = edit.Name;
		lower.ToLower();
		array<VPPXmlNode> nodes = m_ByName.Get(lower);
		int op = edit.Op;
		if (op == VPPXEOp.ADD)
		{
			if (nodes && nodes.Count() > 0)
			{
				Fail("#VSTR_XMLE_ERR_NAME_EXISTS", edit.Name);
				return;
			}

			if (!VPPXmlText.IsValidClassName(edit.Name))
			{
				Fail("#VSTR_XMLE_ERR_NAME_INVALID", edit.Name);
				return;
			}

			PlanInsertBlock(BuildTemplateLines(edit.Name));
			return;
		}

		if (!nodes || nodes.Count() == 0)
		{
			Fail("#VSTR_XMLE_ERR_NOT_FOUND", edit.Name);
			return;
		}

		VPPXmlNode lastNode = nodes.Get(nodes.Count() - 1);
		if (op == VPPXEOp.UPDATE)
		{
			PlanUpdate(lastNode, edit);
			return;
		}

		if (op == VPPXEOp.DELETE || op == VPPXEOp.MOVE)
		{
			foreach (VPPXmlNode delNode : nodes)
			{
				PlanRemoveElement(delNode);
			}

			return;
		}

		if (op == VPPXEOp.COPY)
		{
			return;
		}

		if (op == VPPXEOp.RENAME || op == VPPXEOp.DUPLICATE)
		{
			if (!VPPXmlText.IsValidClassName(edit.NewName))
			{
				Fail("#VSTR_XMLE_ERR_NAME_INVALID", edit.NewName);
				return;
			}

			string newLower = edit.NewName;
			newLower.ToLower();
			if (m_ByName.Contains(newLower))
			{
				Fail("#VSTR_XMLE_ERR_NAME_EXISTS", edit.NewName);
				return;
			}

			if (op == VPPXEOp.RENAME)
			{
				foreach (VPPXmlNode renNode : nodes)
				{
					PlanReplaceName(renNode, edit.NewName);
				}

				return;
			}

			array<string> block = ExtractBlockOf(lastNode);
			if (!block || !ReplaceNameInBlock(block, lastNode, edit.NewName))
			{
				Fail("#VSTR_XMLE_ERR_INTERNAL", edit.Name);
				return;
			}

			IndentBlock(block);
			PlanInsertBlock(block);
			return;
		}

		Fail("#VSTR_XMLE_ERR_VALIDATION", edit.Name);
	}

	protected void PlanCopyIn(VPPXETypeEdit edit, array<string> block)
	{
		if (!edit || !block || block.Count() == 0)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", "copy");
			return;
		}

		m_CurOwner = edit.Name;
		string lower = edit.Name;
		lower.ToLower();
		if (m_ByName.Contains(lower))
		{
			Fail("#VSTR_XMLE_ERR_NAME_EXISTS", edit.Name);
			return;
		}

		array<string> copied = new array<string>();
		foreach (string blockLine : block)
		{
			copied.Insert(blockLine);
		}

		IndentBlock(copied);
		PlanInsertBlock(copied);
	}

	// UPDATE on the last occurrence: scalars, flags, category and REPLACE lists.
	protected void PlanUpdate(VPPXmlNode typeNode, VPPXETypeEdit edit)
	{
		if (typeNode.SelfClosing)
		{
			PlanUpdateSelfClosing(typeNode, edit);
			return;
		}

		array<VPPXmlNode> kids = new array<VPPXmlNode>();
		ElementChildren(typeNode, kids);
		string ci = ChildIndentFor(typeNode);
		for (int si = 0; si < 7; si++)
		{
			int scalarBit = CanonBitAt(si);
			if ((edit.SetMask & scalarBit) != 0)
			{
				PlanSetScalar(typeNode, kids, scalarBit, EditScalar(edit, scalarBit), ci);
			}
			else if ((edit.ClearMask & scalarBit) != 0)
			{
				PlanRemoveAllNamed(kids, ElementNameOf(scalarBit));
			}
		}

		if ((edit.SetMask & VPPXEField.FLAGS) != 0)
		{
			PlanSetFlags(typeNode, kids, edit.FlagsValue, ci);
		}
		else if ((edit.ClearMask & VPPXEField.FLAGS) != 0)
		{
			PlanRemoveAllNamed(kids, "flags");
		}

		if ((edit.SetMask & VPPXEField.CATEGORY) != 0)
		{
			string catText = CategoryText(edit.Category);
			if (catText == "")
			{
				return;
			}

			PlanSetCategory(typeNode, kids, catText, ci);
		}
		else if ((edit.ClearMask & VPPXEField.CATEGORY) != 0)
		{
			PlanRemoveAllNamed(kids, "category");
		}

		if (edit.UsageMode == VPPXEListMode.REPLACE)
		{
			array<string> usageTexts = ListTexts("usage", edit.Usages, edit.UsageUsers, LIST_USAGE);
			if (m_Failed)
			{
				return;
			}

			PlanReplaceList(typeNode, kids, "usage", usageTexts, LIST_USAGE, edit.KeepUnknown, ci);
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE)
		{
			array<string> valueTexts = ListTexts("value", edit.Values, edit.ValueUsers, LIST_VALUE);
			if (m_Failed)
			{
				return;
			}

			PlanReplaceList(typeNode, kids, "value", valueTexts, LIST_VALUE, edit.KeepUnknown, ci);
		}

		if (edit.TagMode == VPPXEListMode.REPLACE)
		{
			array<string> tagTexts = ListTexts("tag", edit.Tags, null, LIST_TAG);
			if (m_Failed)
			{
				return;
			}

			PlanReplaceList(typeNode, kids, "tag", tagTexts, LIST_TAG, edit.KeepUnknown, ci);
		}
	}

	// A self-closing <type name="X"/> that needs children is rewritten into a block.
	protected void PlanUpdateSelfClosing(VPPXmlNode typeNode, VPPXETypeEdit edit)
	{
		array<string> texts = new array<string>();
		for (int si = 0; si < 7; si++)
		{
			int scalarBit = CanonBitAt(si);
			if ((edit.SetMask & scalarBit) != 0)
			{
				texts.Insert(ScalarText(scalarBit, EditScalar(edit, scalarBit)));
			}
		}

		if ((edit.SetMask & VPPXEField.FLAGS) != 0)
		{
			texts.Insert(FlagsText(edit.FlagsValue));
		}

		if ((edit.SetMask & VPPXEField.CATEGORY) != 0)
		{
			string catText = CategoryText(edit.Category);
			if (catText == "")
			{
				return;
			}

			texts.Insert(catText);
		}

		if (edit.UsageMode == VPPXEListMode.REPLACE)
		{
			AppendAll(texts, ListTexts("usage", edit.Usages, edit.UsageUsers, LIST_USAGE));
		}

		if (edit.ValueMode == VPPXEListMode.REPLACE)
		{
			AppendAll(texts, ListTexts("value", edit.Values, edit.ValueUsers, LIST_VALUE));
		}

		if (edit.TagMode == VPPXEListMode.REPLACE)
		{
			AppendAll(texts, ListTexts("tag", edit.Tags, null, LIST_TAG));
		}

		if (m_Failed || texts.Count() == 0)
		{
			return;
		}

		string ti = TypeIndentOf(typeNode);
		string ci = ti + IndentUnit();
		array<string> inner = new array<string>();
		foreach (string text : texts)
		{
			inner.Insert(ci + text);
		}

		RewriteSelfClosing(typeNode, inner, ti + "</" + typeNode.Name + ">");
	}

	protected void AppendAll(array<string> target, array<string> source)
	{
		if (!source)
		{
			return;
		}

		foreach (string item : source)
		{
			target.Insert(item);
		}
	}

	protected void PlanSetScalar(VPPXmlNode typeNode, array<VPPXmlNode> kids, int fieldBit, int value, string ci)
	{
		string elemName = ElementNameOf(fieldBit);
		string valueText = value.ToString();
		VPPXmlNode last = LastChildNamed(kids, elemName);
		if (last)
		{
			if (last.SelfClosing)
			{
				AddPatch(last.StartLine, last.StartCol, last.EndLine, last.EndCol, One("<" + last.Name + ">" + valueText + "</" + last.Name + ">"));
			}
			else
			{
				AddPatch(last.OpenEndLine, last.OpenEndCol, last.CloseStartLine, last.CloseStartCol, One(valueText));
			}

			return;
		}

		PlanInsertChild(typeNode, kids, CanonIndexOfName(elemName), One(ScalarText(fieldBit, value)), ci);
	}

	protected void PlanSetFlags(VPPXmlNode typeNode, array<VPPXmlNode> kids, int flagsValue, string ci)
	{
		VPPXmlNode flagsNode = LastChildNamed(kids, "flags");
		if (!flagsNode)
		{
			PlanInsertChild(typeNode, kids, CanonIndexOfName("flags"), One(FlagsText(flagsValue)), ci);
			return;
		}

		string tagText = BuildFlagsTag(flagsNode, flagsValue);
		if (flagsNode.SelfClosing)
		{
			AddPatch(flagsNode.StartLine, flagsNode.StartCol, flagsNode.EndLine, flagsNode.EndCol, One(tagText));
		}
		else
		{
			AddPatch(flagsNode.StartLine, flagsNode.StartCol, flagsNode.OpenEndLine, flagsNode.OpenEndCol, One(tagText));
		}
	}

	// <flags + attributes in original order (known updated, unknown kept) + missing known in canonical order + tail.
	protected string BuildFlagsTag(VPPXmlNode flagsNode, int flagsValue)
	{
		string text = "<" + flagsNode.Name;
		int seen = 0;
		int attrCount = flagsNode.AttrCount();
		for (int ai = 0; ai < attrCount; ai++)
		{
			string attrName = flagsNode.AttrNames.Get(ai);
			string attrLower = attrName;
			attrLower.ToLower();
			int flagBit = FlagBitOfAttr(attrLower);
			string attrValue;
			if (flagBit != 0)
			{
				attrValue = "0";
				if ((flagsValue & flagBit) != 0)
				{
					attrValue = "1";
				}

				seen = seen | flagBit;
			}
			else
			{
				attrValue = VPPXmlText.EncodeAttr(flagsNode.AttrValues.Get(ai));
			}

			text += " " + attrName + "=\"" + attrValue + "\"";
		}

		for (int fi = 0; fi < 6; fi++)
		{
			int missingBit = 1 << fi;
			if ((seen & missingBit) != 0)
			{
				continue;
			}

			string missingValue = "0";
			if ((flagsValue & missingBit) != 0)
			{
				missingValue = "1";
			}

			text += " " + FlagAttrAt(fi) + "=\"" + missingValue + "\"";
		}

		if (!flagsNode.SelfClosing)
		{
			return text + ">";
		}

		string original = m_Doc.Slice(flagsNode.StartLine, flagsNode.StartCol, flagsNode.EndLine, flagsNode.EndCol);
		if (original.IndexOf(" />") >= 0)
		{
			return text + " />";
		}

		return text + "/>";
	}

	protected void PlanSetCategory(VPPXmlNode typeNode, array<VPPXmlNode> kids, string catText, string ci)
	{
		array<VPPXmlNode> cats = new array<VPPXmlNode>();
		ChildrenNamedLower(kids, "category", cats);
		if (cats.Count() == 0)
		{
			PlanInsertChild(typeNode, kids, CanonIndexOfName("category"), One(catText), ci);
			return;
		}

		VPPXmlNode first = cats.Get(0);
		AddPatch(first.StartLine, first.StartCol, first.EndLine, first.EndCol, One(catText));
		int catCount = cats.Count();
		for (int i = 1; i < catCount; i++)
		{
			PlanRemoveElement(cats.Get(i));
		}
	}

	// REPLACE: removes the known elements of that kind (and the unknown ones when !keepUnknown) and inserts
	// the new elements where the first removed one was, else after the last kept one, else in canonical order.
	protected void PlanReplaceList(VPPXmlNode typeNode, array<VPPXmlNode> kids, string elemName, array<string> texts, int listKind, bool keepUnknown, string ci)
	{
		array<VPPXmlNode> removeList = new array<VPPXmlNode>();
		VPPXmlNode lastKept = null;
		foreach (VPPXmlNode kid : kids)
		{
			if (LowerName(kid) != elemName)
			{
				continue;
			}

			if (!keepUnknown || IsKnownListElement(kid, listKind))
			{
				removeList.Insert(kid);
			}
			else
			{
				lastKept = kid;
			}
		}

		if (removeList.Count() > 0)
		{
			VPPXmlNode firstRemoved = removeList.Get(0);
			if (texts.Count() > 0)
			{
				ReplaceNodeWithTexts(firstRemoved, texts);
			}
			else
			{
				PlanRemoveElement(firstRemoved);
			}

			int removeCount = removeList.Count();
			for (int i = 1; i < removeCount; i++)
			{
				PlanRemoveElement(removeList.Get(i));
			}

			return;
		}

		if (texts.Count() == 0)
		{
			return;
		}

		if (lastKept)
		{
			InsertAfterNode(lastKept, texts, ci);
			return;
		}

		PlanInsertChild(typeNode, kids, CanonIndexOfName(elemName), texts, ci);
	}

	protected bool IsKnownListElement(VPPXmlNode node, int listKind)
	{
		if (!m_Limits)
		{
			return false;
		}

		array<string> plain = m_Limits.Usages;
		array<string> groups = m_Limits.UsageGroups;
		if (listKind == LIST_VALUE)
		{
			plain = m_Limits.Values;
			groups = m_Limits.ValueGroups;
		}
		else if (listKind == LIST_TAG)
		{
			plain = m_Limits.Tags;
			groups = null;
		}

		string plainName = node.GetAttr("name", "");
		if (plainName != "" && VPPXELimits.FindIn(plain, plainName) >= 0)
		{
			return true;
		}

		if (groups)
		{
			string groupName = node.GetAttr("user", "");
			if (groupName != "" && VPPXELimits.FindIn(groups, groupName) >= 0)
			{
				return true;
			}
		}

		return false;
	}

	// Element texts with the canonical limits spelling; unknown names fail the batch (ERR_VALIDATION).
	protected array<string> ListTexts(string elemName, array<string> plainNames, array<string> groupNames, int listKind)
	{
		array<string> texts = new array<string>();
		if (!m_Limits)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", "limits");
			return texts;
		}

		array<string> plain = m_Limits.Usages;
		array<string> groups = m_Limits.UsageGroups;
		if (listKind == LIST_VALUE)
		{
			plain = m_Limits.Values;
			groups = m_Limits.ValueGroups;
		}
		else if (listKind == LIST_TAG)
		{
			plain = m_Limits.Tags;
			groups = null;
		}

		map<string, bool> seen = new map<string, bool>();
		if (plainNames)
		{
			foreach (string plainName : plainNames)
			{
				int idx = VPPXELimits.FindIn(plain, plainName);
				if (idx < 0)
				{
					Fail("#VSTR_XMLE_ERR_VALIDATION", m_CurOwner + ": " + elemName + " " + plainName);
					return texts;
				}

				string canonName = plain.Get(idx);
				if (seen.Contains("n" + canonName))
				{
					continue;
				}

				seen.Set("n" + canonName, true);
				texts.Insert("<" + elemName + " name=\"" + VPPXmlText.EncodeAttr(canonName) + "\"/>");
			}
		}

		if (groupNames && groups)
		{
			foreach (string groupName : groupNames)
			{
				int gidx = VPPXELimits.FindIn(groups, groupName);
				if (gidx < 0)
				{
					Fail("#VSTR_XMLE_ERR_VALIDATION", m_CurOwner + ": " + elemName + " @" + groupName);
					return texts;
				}

				string canonGroup = groups.Get(gidx);
				if (seen.Contains("u" + canonGroup))
				{
					continue;
				}

				seen.Set("u" + canonGroup, true);
				texts.Insert("<" + elemName + " user=\"" + VPPXmlText.EncodeAttr(canonGroup) + "\"/>");
			}
		}
		else if (groupNames && groupNames.Count() > 0)
		{
			Fail("#VSTR_XMLE_ERR_VALIDATION", m_CurOwner + ": " + elemName);
		}

		return texts;
	}

	protected string CategoryText(string categoryName)
	{
		if (!m_Limits)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", "limits");
			return "";
		}

		int idx = VPPXELimits.FindIn(m_Limits.Categories, categoryName);
		if (idx < 0)
		{
			Fail("#VSTR_XMLE_ERR_VALIDATION", m_CurOwner + ": category " + categoryName);
			return "";
		}

		return "<category name=\"" + VPPXmlText.EncodeAttr(m_Limits.Categories.Get(idx)) + "\"/>";
	}

	// ---------------------------------------------------------------- patch primitives

	protected void AddPatch(int startLine, int startCol, int endLine, int endCol, array<string> newLines)
	{
		int lineCount = m_Lines.Count();
		if (startLine < 0 || endLine < startLine || endLine > lineCount || startCol < 0 || endCol < 0)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
			return;
		}

		if (endLine == startLine && endCol < startCol)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
			return;
		}

		m_Patches.Insert(new VPPXEWritePatch(startLine, startCol, endLine, endCol, newLines, m_Patches.Count(), m_CurOwner));
	}

	protected void PlanRemoveElement(VPPXmlNode node)
	{
		if (IsAlone(node) && node.EndLine + 1 <= m_Lines.Count())
		{
			AddPatch(node.StartLine, 0, node.EndLine + 1, 0, new array<string>());
			return;
		}

		AddPatch(node.StartLine, node.StartCol, node.EndLine, node.EndCol, new array<string>());
	}

	protected void PlanRemoveAllNamed(array<VPPXmlNode> kids, string lname)
	{
		foreach (VPPXmlNode kid : kids)
		{
			if (LowerName(kid) == lname)
			{
				PlanRemoveElement(kid);
			}
		}
	}

	protected void ReplaceNodeWithTexts(VPPXmlNode node, array<string> texts)
	{
		array<string> newLines = new array<string>();
		if (IsAlone(node))
		{
			string ci = LeadingWsOfLine(node.StartLine);
			int count = texts.Count();
			for (int i = 0; i < count; i++)
			{
				if (i == 0)
				{
					newLines.Insert(texts.Get(i));
				}
				else
				{
					newLines.Insert(ci + texts.Get(i));
				}
			}
		}
		else
		{
			newLines.Insert(JoinTexts(texts));
		}

		AddPatch(node.StartLine, node.StartCol, node.EndLine, node.EndCol, newLines);
	}

	protected void PlanInsertChild(VPPXmlNode typeNode, array<VPPXmlNode> kids, int canonIdx, array<string> texts, string ci)
	{
		VPPXmlNode anchor = null;
		foreach (VPPXmlNode kid : kids)
		{
			int kidCanon = CanonIndexOfName(LowerName(kid));
			if (kidCanon >= 0 && kidCanon < canonIdx)
			{
				anchor = kid;
			}
		}

		if (anchor)
		{
			InsertAfterNode(anchor, texts, ci);
			return;
		}

		InsertAsFirstChild(typeNode, texts, ci);
	}

	protected void InsertAfterNode(VPPXmlNode anchor, array<string> texts, string ci)
	{
		array<string> newLines = new array<string>();
		if (IsAlone(anchor) && anchor.EndLine + 1 <= m_Lines.Count())
		{
			foreach (string text : texts)
			{
				newLines.Insert(ci + text);
			}

			newLines.Insert("");
			AddPatch(anchor.EndLine + 1, 0, anchor.EndLine + 1, 0, newLines);
			return;
		}

		newLines.Insert(JoinTexts(texts));
		AddPatch(anchor.EndLine, anchor.EndCol, anchor.EndLine, anchor.EndCol, newLines);
	}

	protected void InsertAsFirstChild(VPPXmlNode typeNode, array<string> texts, string ci)
	{
		array<string> newLines = new array<string>();
		string lineText = m_Lines.Get(typeNode.OpenEndLine);
		string after = Mid(lineText, typeNode.OpenEndCol, lineText.Length() - typeNode.OpenEndCol);
		if (VPPXmlText.IsBlank(after) && typeNode.OpenEndLine + 1 <= m_Lines.Count())
		{
			foreach (string text : texts)
			{
				newLines.Insert(ci + text);
			}

			newLines.Insert("");
			AddPatch(typeNode.OpenEndLine + 1, 0, typeNode.OpenEndLine + 1, 0, newLines);
			return;
		}

		newLines.Insert(JoinTexts(texts));
		AddPatch(typeNode.OpenEndLine, typeNode.OpenEndCol, typeNode.OpenEndLine, typeNode.OpenEndCol, newLines);
	}

	// Inserts a block of full lines before </types> (ADD, DUPLICATE, COPY/MOVE target).
	protected void PlanInsertBlock(array<string> blockLines)
	{
		if (!blockLines || m_Failed)
		{
			return;
		}

		if (m_Root.SelfClosing)
		{
			RewriteSelfClosing(m_Root, blockLines, "</" + m_Root.Name + ">");
			return;
		}

		int closeLine = m_Root.CloseStartLine;
		int closeCol = m_Root.CloseStartCol;
		if (closeLine < 0 || closeLine >= m_Lines.Count())
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
			return;
		}

		string before = Mid(m_Lines.Get(closeLine), 0, closeCol);
		array<string> newLines = new array<string>();
		if (VPPXmlText.IsBlank(before))
		{
			foreach (string blockLine : blockLines)
			{
				newLines.Insert(blockLine);
			}

			newLines.Insert("");
			AddPatch(closeLine, 0, closeLine, 0, newLines);
			return;
		}

		newLines.Insert("");
		foreach (string inlineLine : blockLines)
		{
			newLines.Insert(inlineLine);
		}

		newLines.Insert("");
		AddPatch(closeLine, closeCol, closeLine, closeCol, newLines);
	}

	// <x .../> -> <x ...> + inner lines + closeLine, replacing the element span.
	protected void RewriteSelfClosing(VPPXmlNode node, array<string> innerLines, string closeLine)
	{
		array<string> tagLines = ExtractBlockOf(node);
		if (!tagLines || tagLines.Count() == 0)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
			return;
		}

		int lastIdx = tagLines.Count() - 1;
		string lastLine = tagLines.Get(lastIdx);
		int slash = lastLine.LastIndexOf("/>");
		if (slash < 0)
		{
			Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
			return;
		}

		string head = Mid(lastLine, 0, slash);
		int guard = 0;
		while (head.Length() > 0 && guard < 64)
		{
			string tail = head.Get(head.Length() - 1);
			if (tail != " " && tail != "\t")
			{
				break;
			}

			head = Mid(head, 0, head.Length() - 1);
			guard++;
		}

		tagLines.Set(lastIdx, head + ">");
		array<string> newLines = new array<string>();
		foreach (string tagLine : tagLines)
		{
			newLines.Insert(tagLine);
		}

		foreach (string innerLine : innerLines)
		{
			newLines.Insert(innerLine);
		}

		newLines.Insert(closeLine);
		AddPatch(node.StartLine, node.StartCol, node.EndLine, node.EndCol, newLines);
	}

	protected void PlanReplaceName(VPPXmlNode node, string newName)
	{
		for (int li = node.StartLine; li <= node.OpenEndLine; li++)
		{
			if (li < 0 || li >= m_Lines.Count())
			{
				break;
			}

			string lineText = m_Lines.Get(li);
			int colFrom = 0;
			int colTo = lineText.Length();
			if (li == node.StartLine)
			{
				colFrom = node.StartCol;
			}

			if (li == node.OpenEndLine)
			{
				colTo = node.OpenEndCol;
			}

			int valueStart;
			int valueEnd;
			if (FindAttrValue(lineText, "name", colFrom, colTo, valueStart, valueEnd))
			{
				AddPatch(li, valueStart, li, valueEnd, One(VPPXmlText.EncodeAttr(newName)));
				return;
			}
		}

		Fail("#VSTR_XMLE_ERR_INTERNAL", m_CurOwner);
	}

	protected bool ReplaceNameInBlock(array<string> block, VPPXmlNode node, string newName)
	{
		int openLines = node.OpenEndLine - node.StartLine;
		int blockCount = block.Count();
		for (int bi = 0; bi <= openLines && bi < blockCount; bi++)
		{
			string lineText = block.Get(bi);
			int colTo = lineText.Length();
			if (bi == openLines)
			{
				colTo = node.OpenEndCol;
				if (bi == 0)
				{
					colTo = node.OpenEndCol - node.StartCol;
				}
			}

			int valueStart;
			int valueEnd;
			if (FindAttrValue(lineText, "name", 0, colTo, valueStart, valueEnd))
			{
				string head = Mid(lineText, 0, valueStart);
				string tail = Mid(lineText, valueEnd, lineText.Length() - valueEnd);
				block.Set(bi, head + VPPXmlText.EncodeAttr(newName) + tail);
				return true;
			}
		}

		return false;
	}

	// Finds attrName = "value" (either quote) between colFrom and colTo; the name must follow a space or tab.
	protected bool FindAttrValue(string lineText, string attrName, int colFrom, int colTo, out int valueStart, out int valueEnd)
	{
		valueStart = -1;
		valueEnd = -1;
		string lower = lineText;
		lower.ToLower();
		int limit = colTo;
		if (limit > lower.Length())
		{
			limit = lower.Length();
		}

		int nameLen = attrName.Length();
		int pos = colFrom;
		while (pos >= 0 && pos < limit)
		{
			int hit = lower.IndexOfFrom(pos, attrName);
			if (hit < 0 || hit >= limit)
			{
				return false;
			}

			pos = hit + 1;
			if (hit > 0)
			{
				string before = lower.Get(hit - 1);
				if (before != " " && before != "\t")
				{
					continue;
				}
			}

			int k = hit + nameLen;
			while (k < limit && IsSpaceChar(lower.Get(k)))
			{
				k++;
			}

			if (k >= limit || lower.Get(k) != "=")
			{
				continue;
			}

			k++;
			while (k < limit && IsSpaceChar(lower.Get(k)))
			{
				k++;
			}

			if (k >= limit)
			{
				continue;
			}

			string quote = lower.Get(k);
			if (quote != "\"" && quote != "'")
			{
				continue;
			}

			int closeQuote = lower.IndexOfFrom(k + 1, quote);
			if (closeQuote < 0 || closeQuote > limit)
			{
				continue;
			}

			valueStart = k + 1;
			valueEnd = closeQuote;
			return true;
		}

		return false;
	}

	// ---------------------------------------------------------------- text helpers

	protected array<string> BuildTemplateLines(string typeName)
	{
		string ti = TypeIndent();
		string ci = ti + IndentUnit();
		VPPXETypeRow tmpl = VPPXETypeMerge.TemplateRow(typeName);
		array<string> lines = new array<string>();
		lines.Insert(ti + "<type name=\"" + VPPXmlText.EncodeAttr(typeName) + "\">");
		for (int si = 0; si < 7; si++)
		{
			int scalarBit = CanonBitAt(si);
			lines.Insert(ci + ScalarText(scalarBit, VPPXETypeMerge.GetScalar(tmpl, scalarBit)));
		}

		lines.Insert(ci + FlagsText(tmpl.Flags));
		lines.Insert(ti + "</type>");
		return lines;
	}

	protected void IndentBlock(array<string> block)
	{
		if (!block || block.Count() == 0)
		{
			return;
		}

		block.Set(0, TypeIndent() + block.Get(0));
	}

	protected array<string> ExtractBlockOf(VPPXmlNode node)
	{
		if (!node || node.StartLine < 0 || node.EndLine < node.StartLine || node.EndLine >= m_Lines.Count())
		{
			return null;
		}

		array<string> block = new array<string>();
		for (int li = node.StartLine; li <= node.EndLine; li++)
		{
			string lineText = m_Lines.Get(li);
			int colFrom = 0;
			int colTo = lineText.Length();
			if (li == node.StartLine)
			{
				colFrom = node.StartCol;
			}

			if (li == node.EndLine)
			{
				colTo = node.EndCol;
			}

			if (colTo < colFrom)
			{
				colTo = colFrom;
			}

			block.Insert(Mid(lineText, colFrom, colTo - colFrom));
		}

		return block;
	}

	protected string ScalarText(int fieldBit, int value)
	{
		string elemName = ElementNameOf(fieldBit);
		return "<" + elemName + ">" + value.ToString() + "</" + elemName + ">";
	}

	protected string FlagsText(int flagsValue)
	{
		string text = "<flags";
		for (int fi = 0; fi < 6; fi++)
		{
			string flagValue = "0";
			if ((flagsValue & (1 << fi)) != 0)
			{
				flagValue = "1";
			}

			text += " " + FlagAttrAt(fi) + "=\"" + flagValue + "\"";
		}

		return text + "/>";
	}

	protected string JoinTexts(array<string> texts)
	{
		string joined = "";
		foreach (string text : texts)
		{
			joined += text;
		}

		return joined;
	}

	protected array<string> One(string text)
	{
		array<string> single = new array<string>();
		single.Insert(text);
		return single;
	}

	protected string TypeIndent()
	{
		int typeCount = m_TypeNodes.Count();
		if (typeCount > 0)
		{
			VPPXmlNode lastType = m_TypeNodes.Get(typeCount - 1);
			if (IsLineStart(lastType))
			{
				return LeadingWsOfLine(lastType.StartLine);
			}
		}

		if (m_Root && IsLineStart(m_Root))
		{
			return LeadingWsOfLine(m_Root.StartLine) + IndentUnit();
		}

		return IndentUnit();
	}

	protected string TypeIndentOf(VPPXmlNode typeNode)
	{
		if (IsLineStart(typeNode))
		{
			return LeadingWsOfLine(typeNode.StartLine);
		}

		return TypeIndent();
	}

	protected string ChildIndentFor(VPPXmlNode typeNode)
	{
		int count = typeNode.ChildCount();
		for (int i = 0; i < count; i++)
		{
			VPPXmlNode child = typeNode.ChildAt(i);
			if (child && child.Kind == VPPXmlNodeKind.ELEMENT && IsLineStart(child))
			{
				return LeadingWsOfLine(child.StartLine);
			}
		}

		return TypeIndentOf(typeNode) + IndentUnit();
	}

	protected string IndentUnit()
	{
		if (m_UnitKnown)
		{
			return m_IndentUnit;
		}

		return "    ";
	}

	// Indent unit = first type block's child indent minus its own indent (fallback 4 spaces).
	protected void TryLearnIndentUnit(VPPXmlNode typeNode)
	{
		if (!IsLineStart(typeNode))
		{
			return;
		}

		string typeIndent = LeadingWsOfLine(typeNode.StartLine);
		int count = typeNode.ChildCount();
		for (int i = 0; i < count; i++)
		{
			VPPXmlNode child = typeNode.ChildAt(i);
			if (!child || child.Kind != VPPXmlNodeKind.ELEMENT || !IsLineStart(child))
			{
				continue;
			}

			string childIndent = LeadingWsOfLine(child.StartLine);
			if (childIndent.Length() > typeIndent.Length() && childIndent.IndexOf(typeIndent) == 0)
			{
				m_IndentUnit = childIndent.Substring(typeIndent.Length(), childIndent.Length() - typeIndent.Length());
				m_UnitKnown = true;
			}

			return;
		}
	}

	protected string LeadingWsOfLine(int lineIdx)
	{
		if (lineIdx < 0 || lineIdx >= m_Lines.Count())
		{
			return "";
		}

		return VPPXmlText.LeadingWs(m_Lines.Get(lineIdx));
	}

	protected bool IsLineStart(VPPXmlNode node)
	{
		if (!node || node.StartLine < 0 || node.StartLine >= m_Lines.Count())
		{
			return false;
		}

		string before = Mid(m_Lines.Get(node.StartLine), 0, node.StartCol);
		return VPPXmlText.IsBlank(before);
	}

	// Only whitespace before the element on its first line and after it on its last line.
	protected bool IsAlone(VPPXmlNode node)
	{
		if (!IsLineStart(node) || node.EndLine < 0 || node.EndLine >= m_Lines.Count())
		{
			return false;
		}

		string lineText = m_Lines.Get(node.EndLine);
		string after = Mid(lineText, node.EndCol, lineText.Length() - node.EndCol);
		return VPPXmlText.IsBlank(after);
	}

	protected void ElementChildren(VPPXmlNode node, array<VPPXmlNode> outKids)
	{
		int count = node.ChildCount();
		for (int i = 0; i < count; i++)
		{
			VPPXmlNode child = node.ChildAt(i);
			if (child && child.Kind == VPPXmlNodeKind.ELEMENT)
			{
				outKids.Insert(child);
			}
		}
	}

	protected void ChildrenNamedLower(array<VPPXmlNode> kids, string lname, array<VPPXmlNode> outList)
	{
		foreach (VPPXmlNode kid : kids)
		{
			if (LowerName(kid) == lname)
			{
				outList.Insert(kid);
			}
		}
	}

	protected VPPXmlNode LastChildNamed(array<VPPXmlNode> kids, string lname)
	{
		VPPXmlNode found = null;
		foreach (VPPXmlNode kid : kids)
		{
			if (LowerName(kid) == lname)
			{
				found = kid;
			}
		}

		return found;
	}

	// Bounds-safe Substring: "" when the range is empty or starts at/after the end.
	static string Mid(string s, int start, int len)
	{
		int total = s.Length();
		int startIdx = start;
		if (startIdx < 0)
		{
			startIdx = 0;
		}

		if (startIdx >= total || len <= 0)
		{
			return "";
		}

		int count = len;
		if (startIdx + count > total)
		{
			count = total - startIdx;
		}

		return s.Substring(startIdx, count);
	}

	static string LowerName(VPPXmlNode node)
	{
		if (!node)
		{
			return "";
		}

		string lower = node.Name;
		lower.ToLower();
		return lower;
	}

	static bool IsSpaceChar(string ch)
	{
		if (ch == " " || ch == "\t" || ch == "\r" || ch == "\n")
		{
			return true;
		}

		return false;
	}

	// Canonical XML element order: nominal, lifetime, restock, min, quantmin, quantmax, cost, flags,
	// category, usage, value, tag.
	static int CanonBitAt(int index)
	{
		if (index == 0)
		{
			return VPPXEField.NOMINAL;
		}

		if (index == 1)
		{
			return VPPXEField.LIFETIME;
		}

		if (index == 2)
		{
			return VPPXEField.RESTOCK;
		}

		if (index == 3)
		{
			return VPPXEField.MIN;
		}

		if (index == 4)
		{
			return VPPXEField.QUANTMIN;
		}

		if (index == 5)
		{
			return VPPXEField.QUANTMAX;
		}

		if (index == 6)
		{
			return VPPXEField.COST;
		}

		if (index == 7)
		{
			return VPPXEField.FLAGS;
		}

		if (index == 8)
		{
			return VPPXEField.CATEGORY;
		}

		if (index == 9)
		{
			return VPPXEField.USAGE;
		}

		if (index == 10)
		{
			return VPPXEField.VALUE;
		}

		return VPPXEField.TAG;
	}

	static int CanonIndexOfName(string lname)
	{
		for (int i = 0; i < 12; i++)
		{
			if (ElementNameOf(CanonBitAt(i)) == lname)
			{
				return i;
			}
		}

		return -1;
	}

	static string ElementNameOf(int fieldBit)
	{
		if (fieldBit == VPPXEField.NOMINAL)
		{
			return "nominal";
		}

		if (fieldBit == VPPXEField.MIN)
		{
			return "min";
		}

		if (fieldBit == VPPXEField.LIFETIME)
		{
			return "lifetime";
		}

		if (fieldBit == VPPXEField.RESTOCK)
		{
			return "restock";
		}

		if (fieldBit == VPPXEField.COST)
		{
			return "cost";
		}

		if (fieldBit == VPPXEField.QUANTMIN)
		{
			return "quantmin";
		}

		if (fieldBit == VPPXEField.QUANTMAX)
		{
			return "quantmax";
		}

		if (fieldBit == VPPXEField.FLAGS)
		{
			return "flags";
		}

		if (fieldBit == VPPXEField.CATEGORY)
		{
			return "category";
		}

		if (fieldBit == VPPXEField.USAGE)
		{
			return "usage";
		}

		if (fieldBit == VPPXEField.VALUE)
		{
			return "value";
		}

		if (fieldBit == VPPXEField.TAG)
		{
			return "tag";
		}

		return "";
	}

	static string FlagAttrAt(int index)
	{
		if (index == 0)
		{
			return "count_in_cargo";
		}

		if (index == 1)
		{
			return "count_in_hoarder";
		}

		if (index == 2)
		{
			return "count_in_map";
		}

		if (index == 3)
		{
			return "count_in_player";
		}

		if (index == 4)
		{
			return "crafted";
		}

		return "deloot";
	}

	// VPPXEFlagBit of a lowercase flags attribute name, 0 for unknown attributes.
	static int FlagBitOfAttr(string attrLower)
	{
		for (int i = 0; i < 6; i++)
		{
			if (FlagAttrAt(i) == attrLower)
			{
				return 1 << i;
			}
		}

		return 0;
	}

	static int EditScalar(VPPXETypeEdit edit, int fieldBit)
	{
		if (fieldBit == VPPXEField.NOMINAL)
		{
			return edit.Nominal;
		}

		if (fieldBit == VPPXEField.MIN)
		{
			return edit.Min;
		}

		if (fieldBit == VPPXEField.LIFETIME)
		{
			return edit.Lifetime;
		}

		if (fieldBit == VPPXEField.RESTOCK)
		{
			return edit.Restock;
		}

		if (fieldBit == VPPXEField.COST)
		{
			return edit.Cost;
		}

		if (fieldBit == VPPXEField.QUANTMIN)
		{
			return edit.QuantMin;
		}

		return edit.QuantMax;
	}

	// ---------------------------------------------------------------- forward-pass applier

	protected void InitApply()
	{
		m_OutLines = new array<string>();
		m_OutCR = new array<bool>();
		m_Runs = new array<int>();
		m_PCursor = 0;
		m_SrcLine = 0;
		m_SrcCol = 0;
		m_Pending = "";
		m_HasPending = false;
		m_RunOpen = false;
		m_ApplyDone = false;
	}

	bool IsApplied()
	{
		return m_ApplyDone;
	}

	// Copies at most maxLines untouched lines per iteration; loops while BudgetOk. True when done.
	bool ApplyStep(int maxLines)
	{
		if (m_Failed || m_Phase != PH_PLANNED)
		{
			return true;
		}

		int lineCount = m_Lines.Count();
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_ApplyDone)
			{
				return true;
			}

			bool atTail = m_PCursor >= m_Order.Count();
			VPPXEWritePatch p = null;
			int targetLine = lineCount;
			if (!atTail)
			{
				p = m_Patches.Get(m_Order.Get(m_PCursor));
				targetLine = p.StartLine;
			}

			// 1) Finish a partially consumed source line before moving past it.
			if (m_SrcLine < targetLine && (m_HasPending || m_SrcCol > 0))
			{
				string restText = m_Lines.Get(m_SrcLine);
				string rest = Mid(restText, m_SrcCol, restText.Length() - m_SrcCol);
				EmitLine(m_Pending + rest, m_CR.Get(m_SrcLine));
				m_Pending = "";
				m_HasPending = false;
				m_SrcLine++;
				m_SrcCol = 0;
				continue;
			}

			// 2) Copy whole untouched lines up to the next patch (resumable, maxLines per iteration).
			if (m_SrcLine < targetLine)
			{
				int stop = m_SrcLine + maxLines;
				if (stop > targetLine)
				{
					stop = targetLine;
				}

				CopyRun(m_SrcLine, stop);
				m_SrcLine = stop;
				continue;
			}

			if (atTail)
			{
				if (m_HasPending)
				{
					EmitLine(m_Pending, m_DominantCR);
					m_Pending = "";
					m_HasPending = false;
				}

				CloseRun();
				m_ApplyDone = true;
				return true;
			}

			// 3) Apply the patch: prefix of its first line + replacement lines; the suffix of its last line
			//    is appended later, so a merged line takes the CR flag of the original line whose EOL ends it.
			CloseRun();
			string startText = "";
			if (p.StartLine < lineCount)
			{
				startText = m_Lines.Get(p.StartLine);
			}

			int prefixLen = p.StartCol - m_SrcCol;
			if (prefixLen > 0 && m_SrcCol + prefixLen <= startText.Length())
			{
				m_Pending = m_Pending + Mid(startText, m_SrcCol, prefixLen);
			}

			m_HasPending = true;
			int newCount = p.NewLines.Count();
			for (int k = 0; k < newCount; k++)
			{
				if (k == 0)
				{
					m_Pending = m_Pending + p.NewLines.Get(0);
				}
				else
				{
					EmitLine(m_Pending, m_DominantCR);
					m_Pending = p.NewLines.Get(k);
				}
			}

			m_SrcLine = p.EndLine;
			m_SrcCol = p.EndCol;
			if (m_SrcCol == 0 && m_Pending == "")
			{
				m_HasPending = false;
			}

			m_PCursor++;
		}

		return m_ApplyDone;
	}

	protected void EmitLine(string text, bool crFlag)
	{
		CloseRun();
		m_OutLines.Insert(text);
		m_OutCR.Insert(crFlag);
	}

	protected void CopyRun(int fromLine, int toLine)
	{
		if (!m_RunOpen || m_RunOrigEnd != fromLine)
		{
			CloseRun();
			m_RunOpen = true;
			m_RunOrigStart = fromLine;
			m_RunOutStart = m_OutLines.Count();
		}

		for (int li = fromLine; li < toLine; li++)
		{
			m_OutLines.Insert(m_Lines.Get(li));
			m_OutCR.Insert(m_CR.Get(li));
		}

		m_RunOrigEnd = toLine;
	}

	protected void CloseRun()
	{
		if (!m_RunOpen)
		{
			return;
		}

		if (m_RunOrigEnd > m_RunOrigStart)
		{
			m_Runs.Insert(m_RunOrigStart);
			m_Runs.Insert(m_RunOrigEnd);
			m_Runs.Insert(m_RunOutStart);
		}

		m_RunOpen = false;
	}
};
