// Resumable line model of the lossless XML engine (INTERFACES v2.1 section 3):
// VPPXmlSplitter (content -> lines + per-line CR flags + prefix + final-newline flag; it copies 8000-byte windows and
// never calls IndexOfFrom or Substring on the whole content per line), VPPXmlJoiner (the exact inverse, as a
// binary-carry merge) and VPPXmlLineStream (FGets streaming for flat files).
// Splitter + joiner round-trip ANY input byte-exact: mixed CRLF/LF, blank lines, no final newline, BOM prefix.

// Engine string natives rescan the WHOLE receiver on every call (IndexOfFrom, Substring, Get and Length run strlen
// over it; Substring returns at most 8191 bytes), so the splitter never calls them on the whole content per line:
// it copies WINDOW_BYTES windows (one Substring of the content each) and splits inside the window. A line that runs
// past a window is carried over as head + tail, where the tail is always the newest (short) window piece.
class VPPXmlSplitter : Managed
{
	// Below the 8191-byte Substring cap. Native scanning per line ~2 * WINDOW_BYTES, per window one scan of the
	// content: ~8 KB is near the optimum for 0.5-2 MB files (~0.1 s for a 1.2 MB, 18k-line file).
	const static int WINDOW_BYTES = 8000;

	string m_Content;
	int m_Pos;
	int m_Len;
	string m_Win;
	int m_WinLen;
	int m_WinPos;
	string m_CarryHead;
	string m_CarryTail;
	ref array<string> m_Lines;
	ref array<bool> m_CRFlags;
	int m_CRCount;
	string m_Prefix;
	bool m_EndsWithNewline;
	bool m_Done;

	void VPPXmlSplitter()
	{
		m_Lines = new array<string>;
		m_CRFlags = new array<bool>;
		m_Prefix = "";
		m_Win = "";
		m_CarryHead = "";
		m_CarryTail = "";
		m_Done = true;
	}

	// prefix = the 1..3 bytes before the first '<' when they hold no newline (a BOM), else empty; the body starts after it.
	void Begin(string content)
	{
		m_Content = content;
		m_Len = content.Length();
		m_Pos = 0;
		m_Win = "";
		m_WinLen = 0;
		m_WinPos = 0;
		m_CarryHead = "";
		m_CarryTail = "";
		m_Lines = new array<string>;
		m_CRFlags = new array<bool>;
		m_CRCount = 0;
		m_Prefix = "";
		m_EndsWithNewline = false;
		int firstLt = content.IndexOf("<");
		if (firstLt >= 1 && firstLt <= 3)
		{
			string lead = content.Substring(0, firstLt);
			if (lead.IndexOf("\n") < 0)
			{
				m_Prefix = lead;
				m_Pos = firstLt;
			}
		}

		if (m_Len > m_Pos)
		{
			string lastCh = content.Get(m_Len - 1);
			if (lastCh == "\n")
			{
				m_EndsWithNewline = true;
			}
		}

		m_Done = m_Pos >= m_Len;
		if (m_Done)
		{
			m_Content = "";
		}
	}

	// Up to maxLines lines per call. Each line loses one trailing CR and records it in its CR flag. A last segment
	// without a newline is a line only when non-empty and keeps any CR as text (it had no line break to own it).
	// Returns true when the whole body is split.
	bool Step(int maxLines)
	{
		if (m_Done)
		{
			return true;
		}

		int produced = 0;
		while (!m_Done && produced < maxLines)
		{
			if (m_WinPos >= m_WinLen)
			{
				if (!NextWindow())
				{
					FinishTail();
					break;
				}
			}

			int nl = m_Win.IndexOfFrom(m_WinPos, "\n");
			if (nl < 0)
			{
				string rest = m_Win.Substring(m_WinPos, m_WinLen - m_WinPos);
				PushCarry(rest);
				m_WinPos = m_WinLen;
				continue;
			}

			if (nl > m_WinPos)
			{
				string piece = m_Win.Substring(m_WinPos, nl - m_WinPos);
				PushCarry(piece);
			}

			EmitCarriedLine();
			m_WinPos = nl + 1;
			produced++;
			if (m_WinPos >= m_WinLen && m_Pos >= m_Len)
			{
				FinishTail();
			}
		}

		if (m_Done)
		{
			m_Content = "";
			m_Win = "";
		}

		return m_Done;
	}

	// Copies the next window of the content (one Substring, which scans the content once).
	protected bool NextWindow()
	{
		if (m_Pos >= m_Len)
		{
			return false;
		}

		int count = m_Len - m_Pos;
		if (count > WINDOW_BYTES)
		{
			count = WINDOW_BYTES;
		}

		m_Win = m_Content.Substring(m_Pos, count);
		m_WinLen = count;
		m_WinPos = 0;
		m_Pos = m_Pos + count;
		return true;
	}

	// The pending line is m_CarryHead + m_CarryTail; part (never empty) becomes the new tail.
	protected void PushCarry(string part)
	{
		if (m_CarryTail != "")
		{
			m_CarryHead = m_CarryHead + m_CarryTail;
		}

		m_CarryTail = part;
	}

	// Ends the pending line at a newline: one trailing CR (always in the short tail) goes into the CR flag.
	protected void EmitCarriedLine()
	{
		bool hadCR = false;
		string tail = m_CarryTail;
		int tailLen = tail.Length();
		if (tailLen > 0)
		{
			string lastCh = tail.Get(tailLen - 1);
			if (lastCh == "\r")
			{
				hadCR = true;
				tail = "";
				if (tailLen > 1)
				{
					tail = m_CarryTail.Substring(0, tailLen - 1);
				}
			}
		}

		string line = tail;
		if (m_CarryHead != "")
		{
			line = m_CarryHead + tail;
		}

		m_CarryHead = "";
		m_CarryTail = "";
		m_Lines.Insert(line);
		m_CRFlags.Insert(hadCR);
		if (hadCR)
		{
			m_CRCount++;
		}
	}

	// End of the body: a pending segment (no newline after it) is the last line and keeps any CR as text.
	protected void FinishTail()
	{
		string line = m_CarryTail;
		if (m_CarryHead != "")
		{
			line = m_CarryHead + m_CarryTail;
		}

		if (line != "")
		{
			m_Lines.Insert(line);
			m_CRFlags.Insert(false);
		}

		m_CarryHead = "";
		m_CarryTail = "";
		m_Done = true;
	}

	bool IsDone()
	{
		return m_Done;
	}

	array<string> GetLines()
	{
		return m_Lines;
	}

	array<bool> GetCRFlags()
	{
		return m_CRFlags;
	}

	string GetPrefix()
	{
		return m_Prefix;
	}

	bool EndsWithNewline()
	{
		return m_EndsWithNewline;
	}

	// Number of lines whose CR flag is set (counted while splitting).
	int CRCount()
	{
		return m_CRCount;
	}
};

// Binary-carry join: leaves of JOIN_BLOCK_LINES lines are built by append and pushed on a stack of (string, level);
// while the two top entries have equal level they are concatenated (older + newer) and pushed one level up. At the
// end the stack is folded from the newest entry back (acc = entry + acc) and the prefix is put in front.
// Copying is about 16-18x the file size, spread over budgeted Steps. Do not replace this by a fixed block/group
// scheme: a flat final concatenation of k groups is quadratic in k.
class VPPXmlJoiner : Managed
{
	ref array<string> m_Lines;
	ref array<bool> m_CRFlags;
	string m_Prefix;
	bool m_EndsWithNewline;
	int m_Next;
	int m_Count;
	int m_FlagCount;
	string m_Leaf;
	int m_LeafLines;
	ref array<string> m_Stack;
	ref array<int> m_Levels;
	string m_Result;
	bool m_Done;

	void VPPXmlJoiner()
	{
		m_Stack = new array<string>;
		m_Levels = new array<int>;
		m_Prefix = "";
		m_Leaf = "";
		m_Result = "";
		m_Done = true;
	}

	// Every line is followed by CRLF or LF per its CR flag, except the last one, which gets a line break only when
	// endsWithNewline. The arrays are read, never modified.
	void Begin(array<string> lines, array<bool> crFlags, string prefix, bool endsWithNewline)
	{
		m_Lines = lines;
		m_CRFlags = crFlags;
		m_Prefix = prefix;
		m_EndsWithNewline = endsWithNewline;
		m_Next = 0;
		m_Count = 0;
		if (lines)
		{
			m_Count = lines.Count();
		}

		m_FlagCount = 0;
		if (crFlags)
		{
			m_FlagCount = crFlags.Count();
		}

		m_Leaf = "";
		m_LeafLines = 0;
		m_Stack = new array<string>;
		m_Levels = new array<int>;
		m_Result = "";
		m_Done = false;
	}

	// Appends up to maxLines lines; returns true when the result is complete.
	bool Step(int maxLines)
	{
		if (m_Done)
		{
			return true;
		}

		int consumed = 0;
		while (m_Next < m_Count && consumed < maxLines)
		{
			bool isLast = m_Next == m_Count - 1;
			string eol = "";
			if (!isLast || m_EndsWithNewline)
			{
				bool hasCR = false;
				if (m_Next < m_FlagCount)
				{
					hasCR = m_CRFlags[m_Next];
				}

				if (hasCR)
				{
					eol = "\r\n";
				}
				else
				{
					eol = "\n";
				}
			}

			string piece = m_Lines[m_Next] + eol;
			m_Leaf = m_Leaf + piece;
			m_LeafLines++;
			m_Next++;
			consumed++;
			if (m_LeafLines >= VPPXEConst.JOIN_BLOCK_LINES)
			{
				PushLeaf();
			}
		}

		if (m_Next >= m_Count)
		{
			Finish();
		}

		return m_Done;
	}

	void PushLeaf()
	{
		m_Stack.Insert(m_Leaf);
		m_Levels.Insert(0);
		m_Leaf = "";
		m_LeafLines = 0;
		int top = m_Stack.Count() - 1;
		while (top >= 1 && m_Levels[top] == m_Levels[top - 1])
		{
			string merged = m_Stack[top - 1] + m_Stack[top];
			int level = m_Levels[top] + 1;
			m_Stack.Remove(top);
			m_Levels.Remove(top);
			m_Stack.Set(top - 1, merged);
			m_Levels.Set(top - 1, level);
			top = top - 1;
		}
	}

	void Finish()
	{
		if (m_LeafLines > 0)
		{
			m_Stack.Insert(m_Leaf);
			m_Levels.Insert(0);
			m_Leaf = "";
			m_LeafLines = 0;
		}

		string acc = "";
		for (int i = m_Stack.Count() - 1; i >= 0; i--)
		{
			acc = m_Stack[i] + acc;
		}

		m_Result = m_Prefix + acc;
		m_Stack.Clear();
		m_Levels.Clear();
		m_Lines = null;
		m_CRFlags = null;
		m_Done = true;
	}

	bool IsDone()
	{
		return m_Done;
	}

	string GetResult()
	{
		return m_Result;
	}
};

// FGets-based line reader for the flat map files (mapgrouppos, mapgroupcluster*), which are never held whole.
// Next strips one trailing CR; RollingHash = h * 31 + line.Hash() over all lines read (int wrap).
// Callers call Next inside their budget loop and Close on finish or cancel (the destructor closes as well).
class VPPXmlLineStream : Managed
{
	FileHandle m_Fh;
	bool m_Open;
	int m_LineNo;
	int m_RollingHash;

	void ~VPPXmlLineStream()
	{
		Close();
	}

	bool Open(string path)
	{
		Close();
		m_LineNo = 0;
		m_RollingHash = 0;
		m_Fh = OpenFile(path, FileMode.READ);
		if (m_Fh == 0)
		{
			return false;
		}

		m_Open = true;
		return true;
	}

	// False at EOF (FGets < 0) or when the stream is not open.
	bool Next(out string line)
	{
		line = "";
		if (!m_Open)
		{
			return false;
		}

		string raw = "";
		int got = FGets(m_Fh, raw);
		if (got < 0)
		{
			return false;
		}

		int rawLen = raw.Length();
		if (rawLen > 0)
		{
			string lastCh = raw.Get(rawLen - 1);
			if (lastCh == "\r")
			{
				if (rawLen > 1)
				{
					raw = raw.Substring(0, rawLen - 1);
				}
				else
				{
					raw = "";
				}
			}
		}

		m_LineNo++;
		m_RollingHash = m_RollingHash * 31 + raw.Hash();
		line = raw;
		return true;
	}

	void Close()
	{
		if (m_Open)
		{
			CloseFile(m_Fh);
		}

		m_Open = false;
	}

	int LineNo()
	{
		return m_LineNo;
	}

	int RollingHash()
	{
		return m_RollingHash;
	}

	bool IsOpen()
	{
		return m_Open;
	}
};
