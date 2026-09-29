// XML Editor types parser: a sink over VPPXmlReader that builds VPPXETypeRow objects directly (no DOM).
// Used by the types index, the save job (old rows, re-parse verification) and the backups diff.

class VPPXETypesRowSink : VPPXmlSink
{
	protected int m_FileIdx;
	protected ref map<string, int> m_CatIdx;
	protected ref map<string, int> m_TagIdx;
	protected ref map<string, int> m_UsageIdx;
	protected ref map<string, int> m_ValueIdx;
	protected ref map<string, int> m_UsageGroupIdx;
	protected ref map<string, int> m_ValueGroupIdx;
	protected ref array<string> m_Stack;
	protected bool m_RootSeen;
	protected bool m_Failed;
	protected string m_Error;
	protected int m_ErrorLine;

	// Current <type>.
	protected ref VPPXETypeRow m_Row;
	protected ref array<string> m_RowRefs;
	protected bool m_InType;
	protected bool m_SkipType;
	protected bool m_HasCategory;
	protected int m_RowStart;
	protected int m_BadNum;
	protected int m_ScalarBit;
	protected string m_Text;

	// Outputs, one entry per row in document order.
	ref array<ref VPPXETypeRow> Rows;
	ref array<int> StartLines;
	ref array<int> EndLines;
	ref array<ref array<string>> UnknownRefs;
	ref array<string> UnknownSigs;
	ref array<int> BadNums;
	ref array<ref VPPXEIssue> FileIssues;

	void VPPXETypesRowSink(int fileIdx, VPPXELimits limits)
	{
		m_FileIdx = fileIdx;
		m_Stack = new array<string>();
		Rows = new array<ref VPPXETypeRow>();
		StartLines = new array<int>();
		EndLines = new array<int>();
		UnknownRefs = new array<ref array<string>>();
		UnknownSigs = new array<string>();
		BadNums = new array<int>();
		FileIssues = new array<ref VPPXEIssue>();
		m_CatIdx = new map<string, int>();
		m_TagIdx = new map<string, int>();
		m_UsageIdx = new map<string, int>();
		m_ValueIdx = new map<string, int>();
		m_UsageGroupIdx = new map<string, int>();
		m_ValueGroupIdx = new map<string, int>();
		if (limits)
		{
			FillLookup(limits.Categories, m_CatIdx);
			FillLookup(limits.Tags, m_TagIdx);
			FillLookup(limits.Usages, m_UsageIdx);
			FillLookup(limits.Values, m_ValueIdx);
			FillLookup(limits.UsageGroups, m_UsageGroupIdx);
			FillLookup(limits.ValueGroups, m_ValueGroupIdx);
		}
	}

	protected void FillLookup(array<string> names, map<string, int> lookup)
	{
		if (!names)
		{
			return;
		}

		int count = names.Count();
		for (int i = 0; i < count; i++)
		{
			string lower = names.Get(i);
			lower.ToLower();
			if (!lookup.Contains(lower))
			{
				lookup.Set(lower, i);
			}
		}
	}

	protected int Lookup(map<string, int> lookup, string name)
	{
		string lower = name;
		lower.ToLower();
		if (!lookup.Contains(lower))
		{
			return -1;
		}

		return lookup.Get(lower);
	}

	bool HasFailed()
	{
		return m_Failed;
	}

	string GetError()
	{
		return m_Error;
	}

	int GetErrorLine()
	{
		return m_ErrorLine;
	}

	protected void Fail(string message, int line)
	{
		if (m_Failed)
		{
			return;
		}

		m_Failed = true;
		m_Error = message;
		m_ErrorLine = line;
	}

	// Called once after the reader consumed every line.
	void Finish()
	{
		if (m_Failed)
		{
			return;
		}

		if (!m_RootSeen)
		{
			Fail("no <types> root element", 0);
		}
	}

	override bool WantsAttributes(string name)
	{
		return true;
	}

	override void OnStartElement(string name, VPPXmlAttrs attrs, int line, int col, int endLine, int endCol, bool selfClosing)
	{
		if (m_Failed)
		{
			return;
		}

		string lname = name;
		lname.ToLower();
		int depth = m_Stack.Count();
		if (depth == 0)
		{
			if (m_RootSeen)
			{
				Fail("more than one root element", line + 1);
				return;
			}

			m_RootSeen = true;
			if (lname != "types")
			{
				Fail("the root element is <" + name + ">, not <types>", line + 1);
				return;
			}
		}
		else if (depth == 1)
		{
			if (lname == "type")
			{
				BeginType(attrs, line, endLine, selfClosing);
			}
		}
		else if (depth == 2 && m_InType && !m_SkipType)
		{
			HandleChild(name, lname, attrs, selfClosing);
		}

		if (!selfClosing)
		{
			m_Stack.Insert(lname);
		}
	}

	override void OnEndElement(string name, int line, int col, int endLine, int endCol)
	{
		if (m_Failed)
		{
			return;
		}

		int n = m_Stack.Count();
		if (n == 0)
		{
			return;
		}

		string lname = name;
		lname.ToLower();
		if (m_Stack.Get(n - 1) != lname)
		{
			return;
		}

		m_Stack.Remove(n - 1);
		int depth = n - 1;
		if (depth == 1 && lname == "type")
		{
			if (m_InType)
			{
				FinishRow(endLine);
			}

			m_SkipType = false;
			m_InType = false;
			return;
		}

		if (depth == 2 && m_ScalarBit != 0)
		{
			SetScalarText(m_ScalarBit, m_Text);
			m_ScalarBit = 0;
			m_Text = "";
		}
	}

	override void OnText(string rawText, int line, int col)
	{
		if (m_ScalarBit != 0 && m_Stack.Count() == 3)
		{
			m_Text += rawText;
		}
	}

	override void OnComment(int line, int col, int endLine, int endCol)
	{
	}

	override void OnError(string message, int line)
	{
		Fail(message, line);
	}

	protected void BeginType(VPPXmlAttrs attrs, int line, int endLine, bool selfClosing)
	{
		string typeName = "";
		if (attrs && attrs.Has("name"))
		{
			typeName = attrs.Get("name", "");
		}

		if (typeName == "")
		{
			VPPXEIssue issue = new VPPXEIssue();
			issue.Code = VPPXEIssueCode.TYPE_NO_NAME;
			issue.Severity = VPPXEText.IssueSeverity(VPPXEIssueCode.TYPE_NO_NAME);
			issue.FileKey = "";
			issue.Line = line + 1;
			issue.Entry = "";
			issue.Arg = "";
			FileIssues.Insert(issue);
			m_InType = false;
			m_SkipType = !selfClosing;
			return;
		}

		m_Row = VPPXETypeMerge.NewRow(typeName);
		m_Row.FileIdx = m_FileIdx;
		m_Row.Line = line + 1;
		m_RowRefs = null;
		m_RowStart = line;
		m_BadNum = 0;
		m_HasCategory = false;
		m_ScalarBit = 0;
		m_Text = "";
		m_InType = true;
		m_SkipType = false;
		if (selfClosing)
		{
			FinishRow(endLine);
			m_InType = false;
		}
	}

	protected void AddRef(string kind, string refName)
	{
		if (!m_RowRefs)
		{
			m_RowRefs = new array<string>();
		}

		m_RowRefs.Insert(kind + "|" + refName);
	}

	protected void HandleChild(string name, string lname, VPPXmlAttrs attrs, bool selfClosing)
	{
		int sbit = ScalarBitOf(lname);
		if (sbit != 0)
		{
			if (selfClosing)
			{
				SetScalarText(sbit, "");
			}
			else
			{
				m_ScalarBit = sbit;
				m_Text = "";
			}

			return;
		}

		if (lname == "flags")
		{
			int flagsValue = 0;
			for (int fi = 0; fi < 6; fi++)
			{
				int flagBit = VPPXETypeMerge.FlagBitAt(fi);
				string attrName = VPPXETypeMerge.FlagAttrName(flagBit);
				if (attrs && attrs.Has(attrName))
				{
					string attrValue = VPPXmlText.TrimWs(attrs.Get(attrName, "0"));
					if (attrValue.ToInt() != 0)
					{
						flagsValue = flagsValue | flagBit;
					}
				}
			}

			m_Row.Flags = flagsValue;
			m_Row.Present = m_Row.Present | VPPXEField.FLAGS;
			return;
		}

		if (lname == "category")
		{
			if (m_HasCategory)
			{
				m_Row.Issues = m_Row.Issues | VPPXERowIssue.MULTI_CATEGORY;
				return;
			}

			m_HasCategory = true;
			string catName = "";
			if (attrs)
			{
				catName = attrs.Get("name", "");
			}

			int catIdx = Lookup(m_CatIdx, catName);
			if (catIdx >= 0)
			{
				m_Row.Category = catIdx;
			}
			else
			{
				m_Row.Category = -2;
				AddRef("category", catName);
			}

			m_Row.Present = m_Row.Present | VPPXEField.CATEGORY;
			return;
		}

		if (lname == "usage")
		{
			HandleListElement(attrs, m_UsageIdx, m_UsageGroupIdx, "usage", "usageuser", true);
			return;
		}

		if (lname == "value")
		{
			HandleListElement(attrs, m_ValueIdx, m_ValueGroupIdx, "value", "valueuser", false);
			return;
		}

		if (lname == "tag")
		{
			string tagName = "";
			if (attrs)
			{
				tagName = attrs.Get("name", "");
			}

			int tagIdx = Lookup(m_TagIdx, tagName);
			if (tagIdx >= 0)
			{
				if (tagIdx < 32)
				{
					m_Row.Tag = m_Row.Tag | (1 << tagIdx);
				}
			}
			else
			{
				AddRef("tag", tagName);
			}

			return;
		}

		AddRef("child", name);
	}

	protected void HandleListElement(VPPXmlAttrs attrs, map<string, int> plainIdx, map<string, int> groupIdx, string plainKind, string groupKind, bool isUsage)
	{
		if (!attrs)
		{
			return;
		}

		if (attrs.Has("name"))
		{
			string plainName = attrs.Get("name", "");
			int idx = Lookup(plainIdx, plainName);
			if (idx >= 0)
			{
				if (idx < 32)
				{
					if (isUsage)
					{
						m_Row.Usage = m_Row.Usage | (1 << idx);
					}
					else
					{
						m_Row.Value = m_Row.Value | (1 << idx);
					}
				}
			}
			else
			{
				AddRef(plainKind, plainName);
			}
		}

		if (attrs.Has("user"))
		{
			string groupName = attrs.Get("user", "");
			int gidx = Lookup(groupIdx, groupName);
			if (gidx >= 0)
			{
				if (gidx < 32)
				{
					if (isUsage)
					{
						m_Row.UsageUser = m_Row.UsageUser | (1 << gidx);
					}
					else
					{
						m_Row.ValueUser = m_Row.ValueUser | (1 << gidx);
					}
				}
			}
			else
			{
				AddRef(groupKind, groupName);
			}
		}
	}

	protected void SetScalarText(int fieldBit, string rawText)
	{
		string decoded = VPPXmlText.Decode(rawText);
		string trimmed = VPPXmlText.TrimWs(decoded);
		int v = 0;
		if (IsIntText(trimmed))
		{
			v = trimmed.ToInt();
		}
		else
		{
			m_BadNum = m_BadNum | fieldBit;
		}

		VPPXETypeMerge.SetScalar(m_Row, fieldBit, v);
		m_Row.Present = m_Row.Present | fieldBit;
	}

	protected void FinishRow(int endLine)
	{
		if (!m_Row)
		{
			return;
		}

		int bits = m_Row.Issues;
		if (m_BadNum != 0)
		{
			bits = bits | VPPXERowIssue.NOT_NUMBER;
		}

		if (IsNegative(VPPXEField.NOMINAL) || IsNegative(VPPXEField.MIN) || IsNegative(VPPXEField.LIFETIME) || IsNegative(VPPXEField.RESTOCK) || IsNegative(VPPXEField.COST))
		{
			bits = bits | VPPXERowIssue.NEGATIVE;
		}

		if (IsQuantOut(VPPXEField.QUANTMIN) || IsQuantOut(VPPXEField.QUANTMAX))
		{
			bits = bits | VPPXERowIssue.QUANT;
		}

		if ((m_Row.Present & VPPXEField.NOMINAL) != 0 && (m_Row.Present & VPPXEField.MIN) != 0 && m_Row.Nominal > 0 && m_Row.Min > m_Row.Nominal)
		{
			bits = bits | VPPXERowIssue.MIN_GT_NOMINAL;
		}

		if (m_RowRefs)
		{
			foreach (string rowRef : m_RowRefs)
			{
				if (rowRef.IndexOf("child|") != 0)
				{
					bits = bits | VPPXERowIssue.UNKNOWN_NAME;
					break;
				}
			}
		}

		// List Present bits follow the masks, like VPPXETypeMerge.ApplyEdit (Present = mask != 0).
		if ((m_Row.Usage | m_Row.UsageUser) != 0)
		{
			m_Row.Present = m_Row.Present | VPPXEField.USAGE;
		}
		else
		{
			m_Row.Present = m_Row.Present & ~VPPXEField.USAGE;
		}

		if ((m_Row.Value | m_Row.ValueUser) != 0)
		{
			m_Row.Present = m_Row.Present | VPPXEField.VALUE;
		}
		else
		{
			m_Row.Present = m_Row.Present & ~VPPXEField.VALUE;
		}

		if (m_Row.Tag != 0)
		{
			m_Row.Present = m_Row.Present | VPPXEField.TAG;
		}
		else
		{
			m_Row.Present = m_Row.Present & ~VPPXEField.TAG;
		}

		m_Row.Issues = bits;
		Rows.Insert(m_Row);
		StartLines.Insert(m_RowStart);
		EndLines.Insert(endLine);
		UnknownRefs.Insert(m_RowRefs);
		UnknownSigs.Insert(JoinRefs(m_RowRefs));
		BadNums.Insert(m_BadNum);
		m_Row = null;
		m_RowRefs = null;
	}

	protected bool IsNegative(int fieldBit)
	{
		if ((m_Row.Present & fieldBit) == 0)
		{
			return false;
		}

		int v = VPPXETypeMerge.GetScalar(m_Row, fieldBit);
		if (v != VPPXEConst.UNSET && v < 0)
		{
			return true;
		}

		return false;
	}

	protected bool IsQuantOut(int fieldBit)
	{
		if ((m_Row.Present & fieldBit) == 0)
		{
			return false;
		}

		int v = VPPXETypeMerge.GetScalar(m_Row, fieldBit);
		if (v == -1 || v == VPPXEConst.UNSET)
		{
			return false;
		}

		if (v < 0 || v > 100)
		{
			return true;
		}

		return false;
	}

	// ---------------------------------------------------------------- shared static helpers

	// "kind|name" references joined in document order (the unknown-refs signature of a row).
	static string JoinRefs(array<string> refs)
	{
		if (!refs || refs.Count() == 0)
		{
			return "";
		}

		string joined = refs.Get(0);
		int count = refs.Count();
		for (int i = 1; i < count; i++)
		{
			joined += "," + refs.Get(i);
		}

		return joined;
	}

	// VPPXEField bit of a scalar element name (lowercase), 0 when not a scalar.
	static int ScalarBitOf(string lname)
	{
		if (lname == "nominal")
		{
			return VPPXEField.NOMINAL;
		}

		if (lname == "min")
		{
			return VPPXEField.MIN;
		}

		if (lname == "lifetime")
		{
			return VPPXEField.LIFETIME;
		}

		if (lname == "restock")
		{
			return VPPXEField.RESTOCK;
		}

		if (lname == "cost")
		{
			return VPPXEField.COST;
		}

		if (lname == "quantmin")
		{
			return VPPXEField.QUANTMIN;
		}

		if (lname == "quantmax")
		{
			return VPPXEField.QUANTMAX;
		}

		return 0;
	}

	// Optional minus followed by 1-10 digits.
	static bool IsIntText(string s)
	{
		int len = s.Length();
		if (len == 0 || len > 11)
		{
			return false;
		}

		int start = 0;
		if (s.Get(0) == "-")
		{
			start = 1;
		}

		if (start >= len)
		{
			return false;
		}

		string digits = "0123456789";
		for (int i = start; i < len; i++)
		{
			if (digits.IndexOf(s.Get(i)) < 0)
			{
				return false;
			}
		}

		return true;
	}
};

class VPPXETypesParseJob : VPPXEJob
{
	protected ref VPPXmlSplitter m_Splitter;
	protected ref VPPXmlReader m_Reader;
	protected ref VPPXETypesRowSink m_Sink;
	protected int m_Phase;
	protected bool m_Failed;
	protected string m_Error;
	protected int m_ErrorLine;

	void VPPXETypesParseJob(string content, int fileIdx, VPPXELimits limits)
	{
		m_Sink = new VPPXETypesRowSink(fileIdx, limits);
		m_Splitter = new VPPXmlSplitter();
		m_Splitter.Begin(content);
		m_Phase = 0;
	}

	// Driven by its owner (never enqueued by itself when used inside another job).
	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == 0)
			{
				m_Splitter.Step(128);
				if (m_Splitter.IsDone())
				{
					m_Reader = new VPPXmlReader(m_Sink);
					m_Reader.Begin(m_Splitter.GetLines());
					m_Phase = 1;
				}

				continue;
			}

			if (m_Phase == 1)
			{
				bool readDone = m_Reader.Step(64);
				if (m_Reader.HasError())
				{
					SetFailed(m_Reader.GetError(), m_Reader.GetErrorLine());
					m_Phase = 2;
					return true;
				}

				if (m_Sink.HasFailed())
				{
					SetFailed(m_Sink.GetError(), m_Sink.GetErrorLine());
					m_Phase = 2;
					return true;
				}

				if (readDone)
				{
					m_Sink.Finish();
					if (m_Sink.HasFailed())
					{
						SetFailed(m_Sink.GetError(), m_Sink.GetErrorLine());
					}

					m_Phase = 2;
					return true;
				}

				continue;
			}

			return true;
		}

		return false;
	}

	protected void SetFailed(string message, int line)
	{
		m_Failed = true;
		m_Error = message;
		m_ErrorLine = line;
	}

	bool IsDone()
	{
		return m_Phase == 2;
	}

	bool Failed()
	{
		return m_Failed;
	}

	string GetError()
	{
		return m_Error;
	}

	// 1-based line reported by the reader or the sink (0 = unknown).
	int GetErrorLine()
	{
		return m_ErrorLine;
	}

	array<ref VPPXETypeRow> GetRows()
	{
		return m_Sink.Rows;
	}

	void GetFileIssues(array<ref VPPXEIssue> outIssues)
	{
		if (!outIssues)
		{
			return;
		}

		foreach (VPPXEIssue issue : m_Sink.FileIssues)
		{
			outIssues.Insert(issue);
		}
	}

	array<int> GetStartLines()
	{
		return m_Sink.StartLines;
	}

	array<int> GetEndLines()
	{
		return m_Sink.EndLines;
	}

	array<ref array<string>> GetUnknownRefs()
	{
		return m_Sink.UnknownRefs;
	}

	array<string> GetUnknownSigs()
	{
		return m_Sink.UnknownSigs;
	}

	array<int> GetBadNums()
	{
		return m_Sink.BadNums;
	}

	array<string> GetLines()
	{
		return m_Splitter.GetLines();
	}

	array<bool> GetCRFlags()
	{
		return m_Splitter.GetCRFlags();
	}

	override void OnAborted()
	{
		SetFailed("aborted", 0);
		m_Phase = 2;
	}

	override string GetLabel()
	{
		return "TypesParse";
	}
};
