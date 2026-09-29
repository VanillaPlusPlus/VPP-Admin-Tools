// Text helpers of the lossless XML engine: byte-checked whole-file IO, CR-stripped revision hashing and comparison,
// sort-key padding, clipping, entity coding and whitespace/name helpers (INTERFACES v2.1 section 3).
// Enforce strings are values: every helper works on local copies and never appends in a loop over a whole file.
// Engine limits: Replace is exact only up to 2560 bytes, Substring/Trim return at most 8191 bytes: whole-file content
// never goes through Replace/Substring/Trim (8000-byte windows, 2000-byte Replace pieces, binary-carry concatenation).

class VPPXmlText
{
	const static string ZERO_PAD = "00000000000000000000";
	const static string CLASS_NAME_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
	const static string DIGIT_CHARS = "0123456789";
	const static int COPY_WINDOW_BYTES = 8000;
	const static int REPLACE_SAFE_BYTES = 2000;

	// One native ReadFile. False when the file cannot be opened, when the byte count differs from the string length
	// (a NUL byte or truncation) or when it reaches READ_CAP. content is "" on failure.
	static bool ReadAll(string path, out string content)
	{
		content = "";
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (fh == 0)
		{
			return false;
		}

		string data = "";
		int bytes = ReadFile(fh, data, VPPXEConst.READ_CAP);
		CloseFile(fh);
		if (bytes < 0 || bytes >= VPPXEConst.READ_CAP)
		{
			return false;
		}

		if (bytes != data.Length())
		{
			return false;
		}

		content = data;
		return true;
	}

	// One native FPrint of the exact bytes.
	static bool WriteAll(string path, string content)
	{
		FileHandle fh = OpenFile(path, FileMode.WRITE);
		if (fh == 0)
		{
			return false;
		}

		FPrint(fh, content);
		CloseFile(fh);
		return true;
	}

	// Revision: Hash() of the content with every CR removed. Exact for any size (StripCR is windowed and Hash()
	// covers the whole string).
	static int NormalizedHash(string content)
	{
		string stripped = StripCR(content);
		return stripped.Hash();
	}

	// Copy with every CR removed, exact for any size. Content without a CR is returned as is (IndexOf is a strstr
	// and does not measure the string). Otherwise the content is copied in COPY_WINDOW_BYTES windows, each window is
	// cut into REPLACE_SAFE_BYTES pieces (native Replace is exact only up to 2560 bytes), and the stripped pieces are
	// concatenated by binary carry (CarryPush/CarryFold), which stays linear-logarithmic in the content size.
	static string StripCR(string content)
	{
		if (content.IndexOf("\r") < 0)
		{
			return content;
		}

		int len = content.Length();
		array<string> stack = new array<string>();
		array<int> levels = new array<int>();
		int pos = 0;
		while (pos < len)
		{
			int winCount = len - pos;
			if (winCount > COPY_WINDOW_BYTES)
			{
				winCount = COPY_WINDOW_BYTES;
			}

			string win = content.Substring(pos, winCount);
			pos = pos + winCount;
			int winPos = 0;
			while (winPos < winCount)
			{
				int take = winCount - winPos;
				if (take > REPLACE_SAFE_BYTES)
				{
					take = REPLACE_SAFE_BYTES;
				}

				string piece = win.Substring(winPos, take);
				winPos = winPos + take;
				piece.Replace("\r", "");
				CarryPush(stack, levels, piece);
			}
		}

		return CarryFold(stack);
	}

	// Pushes piece at level 0; while the two top entries have equal level they are concatenated (older + newer) and
	// pushed one level up (the same binary carry as VPPXmlJoiner).
	static void CarryPush(array<string> stack, array<int> levels, string piece)
	{
		stack.Insert(piece);
		levels.Insert(0);
		int top = stack.Count() - 1;
		while (top >= 1 && levels[top] == levels[top - 1])
		{
			string merged = stack[top - 1] + stack[top];
			int level = levels[top] + 1;
			stack.Remove(top);
			levels.Remove(top);
			stack.Set(top - 1, merged);
			levels.Set(top - 1, level);
			top = top - 1;
		}
	}

	// Folds a CarryPush stack from the newest entry back (acc = entry + acc).
	static string CarryFold(array<string> stack)
	{
		string acc = "";
		for (int i = stack.Count() - 1; i >= 0; i--)
		{
			acc = stack[i] + acc;
		}

		return acc;
	}

	// StripCR(a) == StripCR(b), exact for any size. Every in-process write and backup verification uses this string
	// comparison.
	static bool SameNormalized(string a, string b)
	{
		if (a == b)
		{
			return true;
		}

		string strippedA = StripCR(a);
		string strippedB = StripCR(b);
		return strippedA == strippedB;
	}

	// True when the content holds an LF that is not preceded by CR. Exact for any size: it reads the CR flags of the
	// windowed splitter instead of running Replace over the content.
	static bool HasBareLF(string content)
	{
		if (content.IndexOf("\n") < 0)
		{
			return false;
		}

		array<string> lines = new array<string>();
		array<bool> crFlags = new array<bool>();
		string prefix = "";
		bool endsWithNewline = false;
		SplitLines(content, lines, crFlags, prefix, endsWithNewline);
		int count = lines.Count();
		for (int i = 0; i < count; i++)
		{
			bool hasBreak = i < count - 1 || endsWithNewline;
			bool hadCR = crFlags[i];
			if (hasBreak && !hadCR)
			{
				return true;
			}
		}

		return false;
	}

	// Exactly content.Replace("\r\n", "\n") without its size limit: every CRLF becomes LF and lone CRs stay. Splits
	// with the windowed splitter and joins with every CR flag cleared.
	static string CRLFToLF(string content)
	{
		if (content.IndexOf("\r\n") < 0)
		{
			return content;
		}

		array<string> lines = new array<string>();
		array<bool> crFlags = new array<bool>();
		string prefix = "";
		bool endsWithNewline = false;
		SplitLines(content, lines, crFlags, prefix, endsWithNewline);
		array<bool> noCR = new array<bool>();
		int count = lines.Count();
		for (int i = 0; i < count; i++)
		{
			noCR.Insert(false);
		}

		return JoinLines(lines, noCR, prefix, endsWithNewline);
	}

	// Zero-padded decimal of a non-negative int (negative values are clamped to 0); longer numbers are returned as is.
	static string PadInt(int v, int width)
	{
		int num = v;
		if (num < 0)
		{
			num = 0;
		}

		string digits = num.ToString();
		int missing = width - digits.Length();
		if (missing <= 0)
		{
			return digits;
		}

		string zeros = "";
		while (missing > 0)
		{
			int take = missing;
			if (take > ZERO_PAD.Length())
			{
				take = ZERO_PAD.Length();
			}

			zeros = zeros + ZERO_PAD.Substring(0, take);
			missing = missing - take;
		}

		return zeros + digits;
	}

	// s when s.LengthUtf8() <= maxChars, else the first maxChars UTF-8 characters + "..." (never splits a multi-byte sequence).
	static string Clip(string s, int maxChars)
	{
		int limit = maxChars;
		if (limit < 0)
		{
			limit = 0;
		}

		if (s.LengthUtf8() <= limit)
		{
			return s;
		}

		string head = "";
		if (limit > 0)
		{
			head = s.SubstringUtf8(0, limit);
		}

		return head + "...";
	}

	// Convenience wrapper that runs a VPPXmlSplitter to completion without a budget: unbudgeted; linear with the
	// windowed splitter (about 0.1 s per MB). Jobs drive a VPPXmlSplitter themselves. The out arrays are cleared first.
	static void SplitLines(string content, array<string> outLines, array<bool> outCR, out string prefix, out bool endsWithNewline)
	{
		VPPXmlSplitter splitter = new VPPXmlSplitter();
		splitter.Begin(content);
		bool finished = false;
		while (!finished)
		{
			finished = splitter.Step(VPPXEConst.SMALL_DOC_LINES);
		}

		prefix = splitter.GetPrefix();
		endsWithNewline = splitter.EndsWithNewline();
		if (outLines)
		{
			outLines.Clear();
			outLines.Copy(splitter.GetLines());
		}

		if (outCR)
		{
			outCR.Clear();
			outCR.Copy(splitter.GetCRFlags());
		}
	}

	// Convenience wrapper that runs a VPPXmlJoiner to completion without a budget: unbudgeted; linear with the
	// windowed splitter (about 0.1 s per MB).
	static string JoinLines(array<string> lines, array<bool> crFlags, string prefix, bool endsWithNewline)
	{
		VPPXmlJoiner joiner = new VPPXmlJoiner();
		joiner.Begin(lines, crFlags, prefix, endsWithNewline);
		bool finished = false;
		while (!finished)
		{
			finished = joiner.Step(VPPXEConst.SMALL_DOC_LINES);
		}

		return joiner.GetResult();
	}

	// Decode, EncodeText and EncodeAttr use native Replace, which is exact only up to 2560 bytes: they are for values
	// (attributes, element text), which stay far below that, never for whole-file content.
	// &lt; &gt; &quot; &apos; first, &amp; last (so "&amp;lt;" decodes to "&lt;").
	static string Decode(string s)
	{
		if (s.IndexOf("&") < 0)
		{
			return s;
		}

		string decoded = s;
		decoded.Replace("&lt;", "<");
		decoded.Replace("&gt;", ">");
		decoded.Replace("&quot;", "\"");
		decoded.Replace("&apos;", "'");
		decoded.Replace("&amp;", "&");
		return decoded;
	}

	// & first, then < and >. Values only (native Replace is exact only up to 2560 bytes).
	static string EncodeText(string s)
	{
		if (s.IndexOf("&") < 0 && s.IndexOf("<") < 0 && s.IndexOf(">") < 0)
		{
			return s;
		}

		string encoded = s;
		encoded.Replace("&", "&amp;");
		encoded.Replace("<", "&lt;");
		encoded.Replace(">", "&gt;");
		return encoded;
	}

	// & first, then <, > and the double quote. Values only (native Replace is exact only up to 2560 bytes).
	static string EncodeAttr(string s)
	{
		if (s.IndexOf("&") < 0 && s.IndexOf("<") < 0 && s.IndexOf(">") < 0 && s.IndexOf("\"") < 0)
		{
			return s;
		}

		string encoded = s;
		encoded.Replace("&", "&amp;");
		encoded.Replace("<", "&lt;");
		encoded.Replace(">", "&gt;");
		encoded.Replace("\"", "&quot;");
		return encoded;
	}

	// Case-insensitive equality; lowercases local copies only, and only when the lengths match.
	static bool EqualsNoCase(string a, string b)
	{
		if (a.Length() != b.Length())
		{
			return false;
		}

		if (a == b)
		{
			return true;
		}

		string lowerA = a;
		string lowerB = b;
		lowerA.ToLower();
		lowerB.ToLower();
		return lowerA == lowerB;
	}

	// Case-insensitive index of name in list, -1 if missing (short lists: attributes, limits).
	static int FindNoCase(array<string> list, string name)
	{
		if (!list)
		{
			return -1;
		}

		string wanted = name;
		wanted.ToLower();
		int wantedLen = wanted.Length();
		int total = list.Count();
		for (int i = 0; i < total; i++)
		{
			string entry = list[i];
			if (entry.Length() != wantedLen)
			{
				continue;
			}

			entry.ToLower();
			if (entry == wanted)
			{
				return i;
			}
		}

		return -1;
	}

	// Space, tab, CR or LF.
	static bool IsWsChar(string ch)
	{
		if (ch == " " || ch == "\t" || ch == "\r" || ch == "\n")
		{
			return true;
		}

		return false;
	}

	// Leading spaces and tabs of a line (the indentation).
	static string LeadingWs(string line)
	{
		int total = line.Length();
		int pos = 0;
		while (pos < total)
		{
			string ch = line.Get(pos);
			if (ch != " " && ch != "\t")
			{
				break;
			}

			pos++;
		}

		if (pos == 0)
		{
			return "";
		}

		return line.Substring(0, pos);
	}

	// True when s holds only spaces, tabs, CR and LF (or is empty).
	static bool IsBlank(string s)
	{
		int total = s.Length();
		for (int i = 0; i < total; i++)
		{
			string ch = s.Get(i);
			if (!IsWsChar(ch))
			{
				return false;
			}
		}

		return true;
	}

	// s without leading and trailing spaces, tabs, CR and LF.
	static string TrimWs(string s)
	{
		int total = s.Length();
		int first = 0;
		while (first < total)
		{
			string headCh = s.Get(first);
			if (!IsWsChar(headCh))
			{
				break;
			}

			first++;
		}

		if (first >= total)
		{
			return "";
		}

		int last = total - 1;
		while (last > first)
		{
			string tailCh = s.Get(last);
			if (!IsWsChar(tailCh))
			{
				break;
			}

			last--;
		}

		if (first == 0 && last == total - 1)
		{
			return s;
		}

		return s.Substring(first, last - first + 1);
	}

	// 1..MAX_NAME_LENGTH chars of [A-Za-z0-9_], first char not a digit.
	static bool IsValidClassName(string s)
	{
		int total = s.Length();
		if (total < 1 || total > VPPXEConst.MAX_NAME_LENGTH)
		{
			return false;
		}

		string firstCh = s.Get(0);
		if (DIGIT_CHARS.IndexOf(firstCh) >= 0)
		{
			return false;
		}

		for (int i = 0; i < total; i++)
		{
			string ch = s.Get(i);
			if (CLASS_NAME_CHARS.IndexOf(ch) < 0)
			{
				return false;
			}
		}

		return true;
	}

	static string Pad2(int v)
	{
		return PadInt(v, 2);
	}

	static string Pad3(int v)
	{
		return PadInt(v, 3);
	}
};
