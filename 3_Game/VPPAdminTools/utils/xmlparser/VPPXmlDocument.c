// Lossless XML engine: attributes, sink interface, resumable tokenizer, lazy DOM and document (INTERFACES v2.1 section 3).
//
// Position contract shared by VPPXmlReader, VPPXmlSink and VPPXmlNode:
// - line = 0-based index into the document's line array; col = byte offset inside that line (for line 0 the BOM
//   prefix is not part of the line); every end position is exclusive (just after the '>').
// - error lines (OnError, GetErrorLine) are 1-based, ready for display.
// - OnEndElement is called only for elements closed by an explicit end tag: a self-closing element is reported once,
//   by OnStartElement with selfClosing = true.
// - OnText receives raw (not decoded) non-blank text inside an element, one call per line segment.
// - the VPPXmlAttrs passed to OnStartElement is reused for the next element: copy what you keep.

enum VPPXmlNodeKind
{
	ELEMENT = 0,
	COMMENT = 1
};

// Tokenizer states of VPPXmlReader.
enum VPPXmlReaderState
{
	TEXT = 0,
	COMMENT = 1,
	PI = 2,
	DECL = 3,
	CDATA = 4,
	TAG = 5
};

class VPPXmlAttrs : Managed
{
	ref array<string> Names;
	ref array<string> Values;

	void VPPXmlAttrs()
	{
		Names = new array<string>;
		Values = new array<string>;
	}

	void Clear()
	{
		Names.Clear();
		Values.Clear();
	}

	// value is already decoded.
	void Add(string name, string value)
	{
		Names.Insert(name);
		Values.Insert(value);
	}

	int Count()
	{
		return Names.Count();
	}

	string NameAt(int i)
	{
		if (i < 0 || i >= Names.Count())
		{
			return "";
		}

		return Names[i];
	}

	string ValueAt(int i)
	{
		if (i < 0 || i >= Values.Count())
		{
			return "";
		}

		return Values[i];
	}

	// Case-insensitive, -1 if missing.
	int IndexOf(string name)
	{
		return VPPXmlText.FindNoCase(Names, name);
	}

	string Get(string name, string def = "")
	{
		int idx = IndexOf(name);
		if (idx < 0)
		{
			return def;
		}

		return Values[idx];
	}

	bool Has(string name)
	{
		return IndexOf(name) >= 0;
	}
};

// Receives the tokens of a VPPXmlReader. See the position contract at the top of this file.
class VPPXmlSink : Managed
{
	// false = the reader skips attribute parsing for this element (attrs stay empty; quotes still bound the tag).
	bool WantsAttributes(string name)
	{
		return true;
	}

	void OnStartElement(string name, VPPXmlAttrs attrs, int line, int col, int endLine, int endCol, bool selfClosing)
	{
	}

	void OnEndElement(string name, int line, int col, int endLine, int endCol)
	{
	}

	void OnText(string rawText, int line, int col)
	{
	}

	void OnComment(int line, int col, int endLine, int endCol)
	{
	}

	void OnError(string message, int line)
	{
	}
};

// Resumable tokenizer over a line array. Scans each line with IndexOfFrom; per-character loops only skip whitespace and
// check names. States: text, comment (<!-- -->, may span lines), PI (<? ?>), declaration (<!DOCTYPE ...>, with an
// optional [...] subset), CDATA (<![CDATA[ ]]>) and tag (a start or end tag, may span lines, quotes honoured).
// Errors stop parsing; they go to the sink's OnError and to GetError/GetErrorLine.
class VPPXmlReader : Managed
{
	const static string NAME_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_:.-";

	ref VPPXmlSink m_Sink;
	ref VPPXmlAttrs m_Attrs;
	ref array<string> m_Lines;
	int m_LineCount;
	int m_LineIdx;
	bool m_HaveLine;
	string m_Cur;
	int m_CurLen;
	int m_Col;
	int m_State;
	bool m_Done;
	bool m_Error;
	string m_ErrorMsg;
	int m_ErrorLine;
	ref array<string> m_OpenNames;
	ref array<string> m_OpenLower;
	ref array<int> m_OpenLines;
	bool m_RootSeen;
	int m_MarkLine;
	int m_MarkCol;
	string m_TagText;
	int m_TagPieceStart;
	string m_Quote;
	bool m_DeclBracket;
	// Cached next double / single quote position on the current line (-2 = unknown, -1 = none left), so a long single-line file
	// is scanned for each quote character at most once per line instead of once per tag.
	int m_NextDq;
	int m_NextSq;

	void VPPXmlReader(VPPXmlSink sink)
	{
		m_Sink = sink;
		m_Attrs = new VPPXmlAttrs();
		m_OpenNames = new array<string>;
		m_OpenLower = new array<string>;
		m_OpenLines = new array<int>;
		m_Cur = "";
		m_TagText = "";
		m_Quote = "";
		m_ErrorMsg = "";
		m_Done = true;
		m_NextDq = -2;
		m_NextSq = -2;
	}

	void Begin(array<string> lines)
	{
		m_Lines = lines;
		m_LineCount = 0;
		if (lines)
		{
			m_LineCount = lines.Count();
		}

		m_LineIdx = 0;
		m_HaveLine = false;
		m_Cur = "";
		m_CurLen = 0;
		m_Col = 0;
		m_State = VPPXmlReaderState.TEXT;
		m_Done = false;
		m_Error = false;
		m_ErrorMsg = "";
		m_ErrorLine = 0;
		m_OpenNames.Clear();
		m_OpenLower.Clear();
		m_OpenLines.Clear();
		m_RootSeen = false;
		m_MarkLine = 0;
		m_MarkCol = 0;
		m_TagText = "";
		m_TagPieceStart = 0;
		m_Quote = "";
		m_DeclBracket = false;
		m_NextDq = -2;
		m_NextSq = -2;
	}

	// Processes up to maxLines work units (one token or one line advance each); true when done (ok or error).
	bool Step(int maxLines)
	{
		if (m_Done)
		{
			return true;
		}

		int units = 0;
		while (!m_Done && units < maxLines)
		{
			units++;
			if (!m_HaveLine)
			{
				if (m_LineIdx >= m_LineCount)
				{
					FinishInput();
					break;
				}

				m_Cur = m_Lines[m_LineIdx];
				m_CurLen = m_Cur.Length();
				m_Col = 0;
				m_HaveLine = true;
				m_NextDq = -2;
				m_NextSq = -2;
				if (m_State == VPPXmlReaderState.TAG)
				{
					m_TagPieceStart = 0;
				}
			}

			if (m_Col >= m_CurLen)
			{
				AdvanceLine();
				continue;
			}

			switch (m_State)
			{
				case VPPXmlReaderState.TEXT:
					ScanText();
					break;
				case VPPXmlReaderState.COMMENT:
					ScanComment();
					break;
				case VPPXmlReaderState.PI:
					ScanPI();
					break;
				case VPPXmlReaderState.DECL:
					ScanDecl();
					break;
				case VPPXmlReaderState.CDATA:
					ScanCData();
					break;
				case VPPXmlReaderState.TAG:
					ScanTag();
					break;
			}
		}

		return m_Done;
	}

	bool HasError()
	{
		return m_Error;
	}

	string GetError()
	{
		return m_ErrorMsg;
	}

	// 1-based line of the error, 0 when there is none.
	int GetErrorLine()
	{
		return m_ErrorLine;
	}

	// Number of lines fully consumed so far.
	int GetLine()
	{
		return m_LineIdx;
	}

	// A tag that continues on the next line keeps the rest of this line (joined with LF) in m_TagText.
	void AdvanceLine()
	{
		if (m_State == VPPXmlReaderState.TAG)
		{
			string piece = "";
			if (m_CurLen > m_TagPieceStart)
			{
				piece = m_Cur.Substring(m_TagPieceStart, m_CurLen - m_TagPieceStart);
			}

			m_TagText = m_TagText + piece + "\n";
		}

		m_LineIdx++;
		m_HaveLine = false;
	}

	int FindFrom(int pos, string sample)
	{
		if (pos < 0 || pos >= m_CurLen)
		{
			return -1;
		}

		return m_Cur.IndexOfFrom(pos, sample);
	}

	// First '"' at or after pos on the current line (-1 = none). Positions only grow within a line, so the cache stays valid.
	int NextDoubleQuote(int pos)
	{
		if (m_NextDq == -1)
		{
			return -1;
		}

		if (m_NextDq < pos)
		{
			m_NextDq = FindFrom(pos, "\"");
		}

		return m_NextDq;
	}

	// First single quote at or after pos on the current line (-1 = none), cached like NextDoubleQuote.
	int NextSingleQuote(int pos)
	{
		if (m_NextSq == -1)
		{
			return -1;
		}

		if (m_NextSq < pos)
		{
			m_NextSq = FindFrom(pos, "'");
		}

		return m_NextSq;
	}

	bool StartsAt(int pos, string sample)
	{
		int sampleLen = sample.Length();
		if (pos < 0 || pos + sampleLen > m_CurLen)
		{
			return false;
		}

		string piece = m_Cur.Substring(pos, sampleLen);
		return piece == sample;
	}

	void Fail(string message, int line)
	{
		m_Error = true;
		m_ErrorMsg = message;
		m_ErrorLine = line;
		m_Done = true;
		if (m_Sink)
		{
			m_Sink.OnError(message, line);
		}
	}

	void EmitText(int from, int to)
	{
		if (to <= from || !m_Sink || m_OpenNames.Count() == 0)
		{
			return;
		}

		string raw = m_Cur.Substring(from, to - from);
		if (VPPXmlText.IsBlank(raw))
		{
			return;
		}

		m_Sink.OnText(raw, m_LineIdx, from);
	}

	void ScanText()
	{
		int lt = FindFrom(m_Col, "<");
		if (lt < 0)
		{
			EmitText(m_Col, m_CurLen);
			m_Col = m_CurLen;
			return;
		}

		EmitText(m_Col, lt);
		m_MarkLine = m_LineIdx;
		m_MarkCol = lt;
		string c2 = "";
		if (lt + 1 < m_CurLen)
		{
			c2 = m_Cur.Get(lt + 1);
		}

		if (c2 == "!")
		{
			if (StartsAt(lt, "<!--"))
			{
				m_State = VPPXmlReaderState.COMMENT;
				m_Col = lt + 4;
				return;
			}

			if (StartsAt(lt, "<![CDATA["))
			{
				m_State = VPPXmlReaderState.CDATA;
				m_Col = lt + 9;
				return;
			}

			m_State = VPPXmlReaderState.DECL;
			m_DeclBracket = false;
			m_Col = lt + 2;
			return;
		}

		if (c2 == "?")
		{
			m_State = VPPXmlReaderState.PI;
			m_Col = lt + 2;
			return;
		}

		m_State = VPPXmlReaderState.TAG;
		m_TagText = "";
		m_TagPieceStart = lt;
		m_Quote = "";
		m_Col = lt + 1;
	}

	void ScanComment()
	{
		int endAt = FindFrom(m_Col, "-->");
		if (endAt < 0)
		{
			m_Col = m_CurLen;
			return;
		}

		m_Col = endAt + 3;
		m_State = VPPXmlReaderState.TEXT;
		if (m_Sink)
		{
			m_Sink.OnComment(m_MarkLine, m_MarkCol, m_LineIdx, endAt + 3);
		}
	}

	void ScanPI()
	{
		int endAt = FindFrom(m_Col, "?>");
		if (endAt < 0)
		{
			m_Col = m_CurLen;
			return;
		}

		m_Col = endAt + 2;
		m_State = VPPXmlReaderState.TEXT;
	}

	void ScanDecl()
	{
		if (m_DeclBracket)
		{
			int closeBracket = FindFrom(m_Col, "]");
			if (closeBracket < 0)
			{
				m_Col = m_CurLen;
				return;
			}

			m_DeclBracket = false;
			m_Col = closeBracket + 1;
			return;
		}

		int gt = FindFrom(m_Col, ">");
		int openBracket = FindFrom(m_Col, "[");
		if (openBracket >= 0 && (gt < 0 || openBracket < gt))
		{
			m_DeclBracket = true;
			m_Col = openBracket + 1;
			return;
		}

		if (gt < 0)
		{
			m_Col = m_CurLen;
			return;
		}

		m_Col = gt + 1;
		m_State = VPPXmlReaderState.TEXT;
	}

	void ScanCData()
	{
		int endAt = FindFrom(m_Col, "]]>");
		if (endAt < 0)
		{
			EmitText(m_Col, m_CurLen);
			m_Col = m_CurLen;
			return;
		}

		EmitText(m_Col, endAt);
		m_Col = endAt + 3;
		m_State = VPPXmlReaderState.TEXT;
	}

	// Finds the tag end: the first '>' outside a quoted span (double or single quotes, which may span lines).
	void ScanTag()
	{
		int pos = m_Col;
		if (m_Quote != "")
		{
			int closeQuote = FindFrom(pos, m_Quote);
			if (closeQuote < 0)
			{
				m_Col = m_CurLen;
				return;
			}

			pos = closeQuote + 1;
			m_Quote = "";
		}

		int gt = FindFrom(pos, ">");
		int dq = NextDoubleQuote(pos);
		int sq = NextSingleQuote(pos);
		while (true)
		{
			int quoteAt = dq;
			string quoteCh = "\"";
			if (sq >= 0 && (quoteAt < 0 || sq < quoteAt))
			{
				quoteAt = sq;
				quoteCh = "'";
			}

			if (gt >= 0 && (quoteAt < 0 || gt < quoteAt))
			{
				CompleteTag(gt);
				return;
			}

			if (quoteAt < 0)
			{
				m_Col = m_CurLen;
				return;
			}

			int closeAt = FindFrom(quoteAt + 1, quoteCh);
			if (closeAt < 0)
			{
				m_Quote = quoteCh;
				m_Col = m_CurLen;
				return;
			}

			pos = closeAt + 1;
			if (gt >= 0 && gt < pos)
			{
				gt = FindFrom(pos, ">");
			}

			if (dq >= 0 && dq < pos)
			{
				dq = NextDoubleQuote(pos);
			}

			if (sq >= 0 && sq < pos)
			{
				sq = NextSingleQuote(pos);
			}
		}
	}

	void CompleteTag(int gt)
	{
		string piece = m_Cur.Substring(m_TagPieceStart, gt + 1 - m_TagPieceStart);
		string tagText = piece;
		if (m_TagText != "")
		{
			tagText = m_TagText + piece;
		}

		m_TagText = "";
		m_Col = gt + 1;
		m_State = VPPXmlReaderState.TEXT;
		ProcessTag(tagText, m_LineIdx, gt + 1);
	}

	// tagText holds the whole tag from '<' to '>' (continuation lines joined with LF).
	void ProcessTag(string tagText, int endLine, int endCol)
	{
		int total = tagText.Length();
		if (total < 3)
		{
			Fail("empty tag", m_MarkLine + 1);
			return;
		}

		string inner = tagText.Substring(1, total - 2);
		string firstCh = inner.Get(0);
		if (firstCh == "/")
		{
			ProcessEndTag(inner, endLine, endCol);
			return;
		}

		ProcessStartTag(inner, endLine, endCol);
	}

	void ProcessEndTag(string inner, int endLine, int endCol)
	{
		string rawName = "";
		int innerLen = inner.Length();
		if (innerLen > 1)
		{
			rawName = inner.Substring(1, innerLen - 1);
		}

		string name = VPPXmlText.TrimWs(rawName);
		if (!IsXmlName(name))
		{
			string badMsg = "invalid end tag </" + name + ">";
			Fail(badMsg, m_MarkLine + 1);
			return;
		}

		int depth = m_OpenNames.Count();
		if (depth == 0)
		{
			string strayMsg = "unexpected end tag </" + name + ">";
			Fail(strayMsg, m_MarkLine + 1);
			return;
		}

		string lowerName = name;
		lowerName.ToLower();
		string expectedLower = m_OpenLower[depth - 1];
		if (lowerName != expectedLower)
		{
			string expected = m_OpenNames[depth - 1];
			string mismatchMsg = "end tag </" + name + "> does not match <" + expected + ">";
			Fail(mismatchMsg, m_MarkLine + 1);
			return;
		}

		m_OpenNames.Remove(depth - 1);
		m_OpenLower.Remove(depth - 1);
		m_OpenLines.Remove(depth - 1);
		if (m_Sink)
		{
			m_Sink.OnEndElement(name, m_MarkLine, m_MarkCol, endLine, endCol);
		}
	}

	void ProcessStartTag(string inner, int endLine, int endCol)
	{
		string body = inner;
		bool selfClosing = false;
		int last = body.Length() - 1;
		while (last >= 0)
		{
			string tailCh = body.Get(last);
			if (!VPPXmlText.IsWsChar(tailCh))
			{
				break;
			}

			last--;
		}

		if (last >= 0)
		{
			string endCh = body.Get(last);
			if (endCh == "/")
			{
				selfClosing = true;
				body = "";
				if (last > 0)
				{
					body = inner.Substring(0, last);
				}
			}
		}

		int nameEnd = NameEnd(body);
		if (nameEnd == 0)
		{
			Fail("invalid element name", m_MarkLine + 1);
			return;
		}

		string name = body.Substring(0, nameEnd);
		if (nameEnd < body.Length())
		{
			string afterCh = body.Get(nameEnd);
			if (!VPPXmlText.IsWsChar(afterCh))
			{
				string nameMsg = "invalid character in the name of <" + name + ">";
				Fail(nameMsg, m_MarkLine + 1);
				return;
			}
		}

		if (m_OpenNames.Count() == 0)
		{
			if (m_RootSeen)
			{
				string rootMsg = "more than one root element (<" + name + ">)";
				Fail(rootMsg, m_MarkLine + 1);
				return;
			}

			m_RootSeen = true;
		}

		m_Attrs.Clear();
		bool wants = false;
		if (m_Sink)
		{
			wants = m_Sink.WantsAttributes(name);
		}

		if (wants && !ParseAttributes(body, nameEnd))
		{
			return;
		}

		if (!selfClosing)
		{
			string lowerName = name;
			lowerName.ToLower();
			m_OpenNames.Insert(name);
			m_OpenLower.Insert(lowerName);
			m_OpenLines.Insert(m_MarkLine);
		}

		if (m_Sink)
		{
			m_Sink.OnStartElement(name, m_Attrs, m_MarkLine, m_MarkCol, endLine, endCol, selfClosing);
		}
	}

	// name = TrimWs(text before '='), then optional whitespace, a quote (either style) and the value up to the matching
	// quote (IndexOfFrom), decoded. A missing '=' or an unquoted value is an error.
	bool ParseAttributes(string body, int from)
	{
		int total = body.Length();
		int pos = from;
		while (true)
		{
			pos = SkipWs(body, pos, total);
			if (pos >= total)
			{
				return true;
			}

			int eq = body.IndexOfFrom(pos, "=");
			if (eq < 0)
			{
				Fail("attribute without =", m_MarkLine + 1);
				return false;
			}

			string rawName = "";
			if (eq > pos)
			{
				rawName = body.Substring(pos, eq - pos);
			}

			string attrName = VPPXmlText.TrimWs(rawName);
			if (!IsXmlName(attrName))
			{
				string attrMsg = "invalid attribute name '" + attrName + "'";
				Fail(attrMsg, m_MarkLine + 1);
				return false;
			}

			int valuePos = SkipWs(body, eq + 1, total);
			if (valuePos >= total)
			{
				string noValueMsg = "attribute " + attrName + " has no value";
				Fail(noValueMsg, m_MarkLine + 1);
				return false;
			}

			string quoteCh = body.Get(valuePos);
			if (quoteCh != "\"" && quoteCh != "'")
			{
				string unquotedMsg = "value of attribute " + attrName + " is not quoted";
				Fail(unquotedMsg, m_MarkLine + 1);
				return false;
			}

			int closeAt = -1;
			if (valuePos + 1 < total)
			{
				closeAt = body.IndexOfFrom(valuePos + 1, quoteCh);
			}

			if (closeAt < 0)
			{
				string openMsg = "value of attribute " + attrName + " is not terminated";
				Fail(openMsg, m_MarkLine + 1);
				return false;
			}

			string rawValue = "";
			if (closeAt > valuePos + 1)
			{
				rawValue = body.Substring(valuePos + 1, closeAt - valuePos - 1);
			}

			m_Attrs.Add(attrName, VPPXmlText.Decode(rawValue));
			pos = closeAt + 1;
		}

		return true;
	}

	int SkipWs(string text, int pos, int total)
	{
		int cursor = pos;
		while (cursor < total)
		{
			string ch = text.Get(cursor);
			if (!VPPXmlText.IsWsChar(ch))
			{
				break;
			}

			cursor++;
		}

		return cursor;
	}

	// Length of the leading [A-Za-z0-9_:.-] run (names are short).
	int NameEnd(string text)
	{
		int total = text.Length();
		int pos = 0;
		while (pos < total)
		{
			string ch = text.Get(pos);
			if (NAME_CHARS.IndexOf(ch) < 0)
			{
				break;
			}

			pos++;
		}

		return pos;
	}

	bool IsXmlName(string text)
	{
		int total = text.Length();
		if (total == 0)
		{
			return false;
		}

		return NameEnd(text) == total;
	}

	void FinishInput()
	{
		if (m_State != VPPXmlReaderState.TEXT)
		{
			string eofMsg = "unexpected end of file inside " + StateLabel();
			Fail(eofMsg, m_MarkLine + 1);
			return;
		}

		int depth = m_OpenNames.Count();
		if (depth > 0)
		{
			string openName = m_OpenNames[depth - 1];
			int openLine = m_OpenLines[depth - 1];
			string openMsg = "element <" + openName + "> is not closed";
			Fail(openMsg, openLine + 1);
			return;
		}

		m_Done = true;
	}

	string StateLabel()
	{
		switch (m_State)
		{
			case VPPXmlReaderState.COMMENT:
				return "a comment";
			case VPPXmlReaderState.PI:
				return "a processing instruction";
			case VPPXmlReaderState.DECL:
				return "a declaration";
			case VPPXmlReaderState.CDATA:
				return "a CDATA section";
			case VPPXmlReaderState.TAG:
				return "a tag";
		}

		return "text";
	}
};

// DOM node. Attribute, text and child arrays stay null until first needed; every accessor is null-safe.
// Positions follow the contract at the top of this file: StartLine/StartCol at '<', OpenEnd after the start tag '>',
// CloseStart at '</' (-1 when self-closing or a comment), End after the final '>'.
class VPPXmlNode : Managed
{
	int Kind;
	string Name;
	ref array<string> AttrNames;
	ref array<string> AttrValues;
	string Text;
	int StartLine;
	int StartCol;
	int OpenEndLine;
	int OpenEndCol;
	int CloseStartLine;
	int CloseStartCol;
	int EndLine;
	int EndCol;
	bool SelfClosing;
	ref array<ref VPPXmlNode> Children;
	VPPXmlNode Parent;

	void VPPXmlNode()
	{
		Kind = VPPXmlNodeKind.ELEMENT;
		Name = "";
		Text = "";
		CloseStartLine = -1;
		CloseStartCol = -1;
	}

	void AddAttr(string name, string value)
	{
		if (!AttrNames)
		{
			AttrNames = new array<string>;
			AttrValues = new array<string>;
		}

		AttrNames.Insert(name);
		AttrValues.Insert(value);
	}

	void AddChild(VPPXmlNode child)
	{
		if (!child)
		{
			return;
		}

		if (!Children)
		{
			Children = new array<ref VPPXmlNode>;
		}

		child.Parent = this;
		Children.Insert(child);
	}

	// Decoded value of an attribute (case-insensitive name), def when absent.
	string GetAttr(string name, string def = "")
	{
		int idx = FindAttr(name);
		if (idx < 0)
		{
			return def;
		}

		return AttrValues[idx];
	}

	int FindAttr(string name)
	{
		if (!AttrNames)
		{
			return -1;
		}

		return VPPXmlText.FindNoCase(AttrNames, name);
	}

	int AttrCount()
	{
		if (!AttrNames)
		{
			return 0;
		}

		return AttrNames.Count();
	}

	// TrimWs(Decode(Text)).
	string InnerText()
	{
		string decoded = VPPXmlText.Decode(Text);
		return VPPXmlText.TrimWs(decoded);
	}

	int ChildCount()
	{
		if (!Children)
		{
			return 0;
		}

		return Children.Count();
	}

	VPPXmlNode ChildAt(int i)
	{
		if (!Children || i < 0 || i >= Children.Count())
		{
			return null;
		}

		return Children[i];
	}

	// First ELEMENT child with this name (case-insensitive), or null.
	VPPXmlNode FirstChild(string name)
	{
		if (!Children)
		{
			return null;
		}

		int total = Children.Count();
		for (int i = 0; i < total; i++)
		{
			VPPXmlNode child = Children[i];
			if (child && child.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(child.Name, name))
			{
				return child;
			}
		}

		return null;
	}

	// Appends every ELEMENT child with this name (case-insensitive) in document order.
	void ChildrenNamed(string name, array<VPPXmlNode> outList)
	{
		if (!Children || !outList)
		{
			return;
		}

		int total = Children.Count();
		for (int i = 0; i < total; i++)
		{
			VPPXmlNode child = Children[i];
			if (child && child.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(child.Name, name))
			{
				outList.Insert(child);
			}
		}
	}
};

// Builds ELEMENT nodes (attributes copied) and COMMENT nodes as children of the current element; top-level comments
// and PIs are skipped. Text segments of one element are concatenated into Text (segments from different lines are
// joined with LF).
class VPPXmlDomBuilder : VPPXmlSink
{
	ref VPPXmlNode m_Root;
	ref array<VPPXmlNode> m_Open;
	VPPXmlNode m_LastTextNode;
	int m_LastTextLine;

	void VPPXmlDomBuilder()
	{
		m_Open = new array<VPPXmlNode>;
		m_LastTextLine = -1;
	}

	VPPXmlNode GetRoot()
	{
		return m_Root;
	}

	VPPXmlNode CurrentNode()
	{
		int depth = m_Open.Count();
		if (depth == 0)
		{
			return null;
		}

		return m_Open[depth - 1];
	}

	override void OnStartElement(string name, VPPXmlAttrs attrs, int line, int col, int endLine, int endCol, bool selfClosing)
	{
		VPPXmlNode node = new VPPXmlNode();
		node.Kind = VPPXmlNodeKind.ELEMENT;
		node.Name = name;
		if (attrs)
		{
			int total = attrs.Count();
			for (int i = 0; i < total; i++)
			{
				node.AddAttr(attrs.NameAt(i), attrs.ValueAt(i));
			}
		}

		node.StartLine = line;
		node.StartCol = col;
		node.OpenEndLine = endLine;
		node.OpenEndCol = endCol;
		node.SelfClosing = selfClosing;
		if (selfClosing)
		{
			node.EndLine = endLine;
			node.EndCol = endCol;
		}

		VPPXmlNode parent = CurrentNode();
		if (parent)
		{
			parent.AddChild(node);
		}
		else
		{
			if (m_Root)
			{
				return;
			}

			m_Root = node;
		}

		if (!selfClosing)
		{
			m_Open.Insert(node);
		}
	}

	override void OnEndElement(string name, int line, int col, int endLine, int endCol)
	{
		int depth = m_Open.Count();
		if (depth == 0)
		{
			return;
		}

		VPPXmlNode node = m_Open[depth - 1];
		m_Open.Remove(depth - 1);
		if (!node)
		{
			return;
		}

		node.CloseStartLine = line;
		node.CloseStartCol = col;
		node.EndLine = endLine;
		node.EndCol = endCol;
	}

	override void OnText(string rawText, int line, int col)
	{
		VPPXmlNode node = CurrentNode();
		if (!node)
		{
			return;
		}

		if (node.Text == "")
		{
			node.Text = rawText;
		}
		else if (m_LastTextNode == node && m_LastTextLine == line)
		{
			node.Text = node.Text + rawText;
		}
		else
		{
			node.Text = node.Text + "\n" + rawText;
		}

		m_LastTextNode = node;
		m_LastTextLine = line;
	}

	override void OnComment(int line, int col, int endLine, int endCol)
	{
		VPPXmlNode parent = CurrentNode();
		if (!parent)
		{
			return;
		}

		VPPXmlNode node = new VPPXmlNode();
		node.Kind = VPPXmlNodeKind.COMMENT;
		node.StartLine = line;
		node.StartCol = col;
		node.OpenEndLine = endLine;
		node.OpenEndCol = endCol;
		node.EndLine = endLine;
		node.EndCol = endCol;
		parent.AddChild(node);
	}
};

// Sink used by SelfTest case (j): declines every attribute and counts the elements.
class VPPXmlSelfTestSink : VPPXmlSink
{
	int m_Starts;
	int m_Ends;
	int m_AttrTotal;

	override bool WantsAttributes(string name)
	{
		return false;
	}

	override void OnStartElement(string name, VPPXmlAttrs attrs, int line, int col, int endLine, int endCol, bool selfClosing)
	{
		m_Starts++;
		if (attrs)
		{
			m_AttrTotal = m_AttrTotal + attrs.Count();
		}
	}

	override void OnEndElement(string name, int line, int col, int endLine, int endCol)
	{
		m_Ends++;
	}
};

// A loaded document: exact line model (lines, CR flags, prefix, final newline) plus a DOM, built by budgeted Steps.
class VPPXmlDocument : Managed
{
	const static int PHASE_IDLE = 0;
	const static int PHASE_SPLIT = 1;
	const static int PHASE_PARSE = 2;
	const static int PHASE_DONE = 3;
	const static int SPLIT_LINES_PER_STEP = 128;
	const static int READER_UNITS_PER_STEP = 64;
	const static int SERIALIZE_LINES_PER_STEP = 4096;

	ref VPPXmlSplitter m_Splitter;
	ref VPPXmlReader m_Reader;
	ref VPPXmlDomBuilder m_Builder;
	ref VPPXmlNode m_Root;
	ref array<string> m_Lines;
	ref array<bool> m_CRFlags;
	string m_Prefix;
	bool m_EndsWithNewline;
	int m_CRCount;
	int m_Revision;
	int m_Phase;
	bool m_Unbudgeted;
	bool m_Error;
	string m_ErrorMsg;
	int m_ErrorLine;

	void VPPXmlDocument()
	{
		Reset();
	}

	void Reset()
	{
		m_Splitter = null;
		m_Reader = null;
		m_Builder = null;
		m_Root = null;
		m_Lines = null;
		m_CRFlags = null;
		m_Prefix = "";
		m_EndsWithNewline = false;
		m_CRCount = 0;
		m_Revision = 0;
		m_Phase = PHASE_IDLE;
		m_Unbudgeted = false;
		m_Error = false;
		m_ErrorMsg = "";
		m_ErrorLine = 0;
	}

	// ReadAll + BeginString; false (and HasError) when the file cannot be read completely.
	bool Load(string path)
	{
		string content = "";
		if (!VPPXmlText.ReadAll(path, content))
		{
			Reset();
			m_Phase = PHASE_DONE;
			m_Error = true;
			m_ErrorMsg = "the file could not be read completely";
			return false;
		}

		BeginString(content);
		return true;
	}

	// Resets the document; splitting and parsing happen in Step.
	void BeginString(string content)
	{
		Reset();
		m_Revision = VPPXmlText.NormalizedHash(content);
		m_Splitter = new VPPXmlSplitter();
		m_Splitter.Begin(content);
		m_Phase = PHASE_SPLIT;
	}

	// Splits, then tokenizes, while VPPXEJobQueue.BudgetOk(); true when done (ok or error).
	bool Step()
	{
		if (m_Phase == PHASE_DONE)
		{
			return true;
		}

		if (m_Phase == PHASE_IDLE)
		{
			m_Phase = PHASE_DONE;
			m_Error = true;
			m_ErrorMsg = "no content loaded";
			return true;
		}

		while (m_Phase != PHASE_DONE && CanContinue())
		{
			if (m_Phase == PHASE_SPLIT)
			{
				if (m_Splitter.Step(SPLIT_LINES_PER_STEP))
				{
					StartParse();
				}

				continue;
			}

			if (m_Reader.Step(READER_UNITS_PER_STEP))
			{
				FinishParse();
			}
		}

		return m_Phase == PHASE_DONE;
	}

	// Loops Step ignoring the budget. ONLY for documents under SMALL_DOC_LINES (economycore, environment, limits,
	// ignorelist, SelfTest). Returns !HasError(); a parse that did not reach PHASE_DONE is reported as an error.
	// Keep this.Step(): a bare Step() resolves to the vanilla attribute class Step (2_GameLib TestingFramework.c).
	bool ParseAll()
	{
		m_Unbudgeted = true;
		this.Step();
		m_Unbudgeted = false;
		if (m_Phase != PHASE_DONE)
		{
			m_Splitter = null;
			m_Reader = null;
			m_Builder = null;
			m_Root = null;
			m_Phase = PHASE_DONE;
			m_Error = true;
			m_ErrorMsg = "parsing did not finish";
			m_ErrorLine = 0;
		}

		return !m_Error;
	}

	bool CanContinue()
	{
		if (m_Unbudgeted)
		{
			return true;
		}

		return VPPXEJobQueue.BudgetOk();
	}

	void StartParse()
	{
		m_Lines = m_Splitter.GetLines();
		m_CRFlags = m_Splitter.GetCRFlags();
		m_Prefix = m_Splitter.GetPrefix();
		m_EndsWithNewline = m_Splitter.EndsWithNewline();
		m_CRCount = m_Splitter.CRCount();
		m_Splitter = null;
		m_Builder = new VPPXmlDomBuilder();
		m_Reader = new VPPXmlReader(m_Builder);
		m_Reader.Begin(m_Lines);
		m_Phase = PHASE_PARSE;
	}

	void FinishParse()
	{
		if (m_Reader.HasError())
		{
			m_Error = true;
			m_ErrorMsg = m_Reader.GetError();
			m_ErrorLine = m_Reader.GetErrorLine();
		}
		else
		{
			m_Root = m_Builder.GetRoot();
			if (!m_Root)
			{
				m_Error = true;
				m_ErrorMsg = "no root element";
				m_ErrorLine = 1;
			}
		}

		m_Reader = null;
		m_Builder = null;
		m_Phase = PHASE_DONE;
	}

	// True once parsing finished (ok or error); check HasError().
	bool IsParsed()
	{
		return m_Phase == PHASE_DONE;
	}

	bool HasError()
	{
		return m_Error;
	}

	string GetError()
	{
		return m_ErrorMsg;
	}

	// 1-based line of the parse error, 0 when unknown.
	int GetErrorLine()
	{
		return m_ErrorLine;
	}

	// Root element, null until parsed or on error.
	VPPXmlNode GetRoot()
	{
		return m_Root;
	}

	// The live line array (null until the split finished); the writer reads it and builds new arrays.
	array<string> GetLines()
	{
		return m_Lines;
	}

	array<bool> GetCRFlags()
	{
		return m_CRFlags;
	}

	int LineCount()
	{
		if (!m_Lines)
		{
			return 0;
		}

		return m_Lines.Count();
	}

	string GetLine(int i)
	{
		if (!m_Lines || i < 0 || i >= m_Lines.Count())
		{
			return "";
		}

		return m_Lines[i];
	}

	string GetPrefix()
	{
		return m_Prefix;
	}

	bool EndsWithNewline()
	{
		return m_EndsWithNewline;
	}

	// "\r\n" when most lines end with CRLF, else "\n".
	string GetDominantEol()
	{
		if (DominantIsCR())
		{
			return "\r\n";
		}

		return "\n";
	}

	bool DominantIsCR()
	{
		return m_CRCount * 2 > LineCount();
	}

	// NormalizedHash of the original content.
	int GetRevision()
	{
		return m_Revision;
	}

	// Exact text between two positions, lines joined with LF. (LineCount(), 0) is the position after the last line.
	string Slice(int startLine, int startCol, int endLine, int endCol)
	{
		int total = LineCount();
		if (total == 0 || startLine < 0 || startLine >= total || endLine < startLine)
		{
			return "";
		}

		int lastLine = endLine;
		if (lastLine > total)
		{
			lastLine = total;
		}

		if (startLine == lastLine)
		{
			string only = GetLine(startLine);
			return CutLine(only, startCol, endCol);
		}

		array<string> parts = new array<string>;
		string firstText = GetLine(startLine);
		parts.Insert(CutLine(firstText, startCol, firstText.Length()));
		for (int i = startLine + 1; i < lastLine; i++)
		{
			parts.Insert(GetLine(i));
		}

		string lastText = GetLine(lastLine);
		parts.Insert(CutLine(lastText, 0, endCol));
		return VPPXmlText.JoinLines(parts, null, "", false);
	}

	string CutLine(string text, int from, int to)
	{
		int textLen = text.Length();
		int a = from;
		int b = to;
		if (a < 0)
		{
			a = 0;
		}

		if (b > textLen)
		{
			b = textLen;
		}

		if (b <= a)
		{
			return "";
		}

		return text.Substring(a, b - a);
	}

	// Resumable join of the current line arrays (the caller keeps a ref and drives Step).
	VPPXmlJoiner BeginSerialize()
	{
		VPPXmlJoiner joiner = new VPPXmlJoiner();
		joiner.Begin(m_Lines, m_CRFlags, m_Prefix, m_EndsWithNewline);
		return joiner;
	}

	// Small documents only.
	string SerializeAll()
	{
		VPPXmlJoiner joiner = BeginSerialize();
		bool finished = false;
		while (!finished)
		{
			finished = joiner.Step(SERIALIZE_LINES_PER_STEP);
		}

		return joiner.GetResult();
	}

	// Embedded round-trip, DOM and Hash-sensitivity cases, run once at boot. Returns false with a message on the first
	// failure; the server editor then runs read-only (no file gets EDITABLE).
	static bool SelfTest(out string failure)
	{
		failure = "";
		if (!SelfTestLines(failure))
		{
			return false;
		}

		if (!SelfTestParse(failure))
		{
			return false;
		}

		if (!SelfTestJoiner(failure))
		{
			return false;
		}

		if (!SelfTestHash(failure))
		{
			return false;
		}

		return true;
	}

	// Split with the splitter and join with the joiner, both in steps of stepLines; true when byte-exact.
	static bool RoundTrips(string content, int stepLines)
	{
		VPPXmlSplitter splitter = new VPPXmlSplitter();
		splitter.Begin(content);
		bool splitDone = false;
		while (!splitDone)
		{
			splitDone = splitter.Step(stepLines);
		}

		VPPXmlJoiner joiner = new VPPXmlJoiner();
		joiner.Begin(splitter.GetLines(), splitter.GetCRFlags(), splitter.GetPrefix(), splitter.EndsWithNewline());
		bool joinDone = false;
		while (!joinDone)
		{
			joinDone = joiner.Step(stepLines);
		}

		string joined = joiner.GetResult();
		return joined == content;
	}

	static bool RoundTripsAllSteps(string content)
	{
		if (!RoundTrips(content, 1))
		{
			return false;
		}

		if (!RoundTrips(content, 3))
		{
			return false;
		}

		return RoundTrips(content, 1000);
	}

	static VPPXmlDocument ParseSmall(string content)
	{
		VPPXmlDocument doc = new VPPXmlDocument();
		doc.BeginString(content);
		doc.ParseAll();
		return doc;
	}

	// (a) mixed CRLF/LF with blank lines and (b) no final newline with a BOM prefix, plus edge inputs.
	static bool SelfTestLines(out string failure)
	{
		string caseA = "<a>\r\n";
		caseA = caseA + "\n";
		caseA = caseA + "  <b>1</b>\r\n";
		caseA = caseA + "\r\n";
		caseA = caseA + "\t<c/>\n";
		caseA = caseA + "</a>\n";
		if (!RoundTripsAllSteps(caseA))
		{
			failure = "(a) mixed CRLF/LF content does not round-trip";
			return false;
		}

		array<string> linesA = new array<string>;
		array<bool> crA = new array<bool>;
		string prefixA = "";
		bool endsA = false;
		VPPXmlText.SplitLines(caseA, linesA, crA, prefixA, endsA);
		if (linesA.Count() != 6 || crA.Count() != 6 || !endsA || prefixA != "")
		{
			failure = "(a) wrong line model for mixed CRLF/LF content";
			return false;
		}

		if (!crA[0] || crA[1] || !crA[2] || !crA[3] || crA[4] || crA[5] || linesA[1] != "" || linesA[2] != "  <b>1</b>")
		{
			failure = "(a) wrong CR flags or line text for mixed CRLF/LF content";
			return false;
		}

		int byte1 = 239;
		int byte2 = 187;
		int byte3 = 191;
		string bom = byte1.AsciiToString() + byte2.AsciiToString() + byte3.AsciiToString();
		string caseB = bom + "<a>\r\n";
		caseB = caseB + "<b x=\"1\"/>\n";
		caseB = caseB + "</a>";
		if (!RoundTripsAllSteps(caseB))
		{
			failure = "(b) BOM content without a final newline does not round-trip";
			return false;
		}

		array<string> linesB = new array<string>;
		array<bool> crB = new array<bool>;
		string prefixB = "";
		bool endsB = true;
		VPPXmlText.SplitLines(caseB, linesB, crB, prefixB, endsB);
		int bomLen = bom.Length();
		if (endsB || linesB.Count() != 3 || linesB[2] != "</a>")
		{
			failure = "(b) wrong line model for content without a final newline";
			return false;
		}

		if (bomLen >= 1 && bomLen <= 3 && prefixB != bom)
		{
			failure = "(b) the BOM prefix was not kept apart";
			return false;
		}

		string caseB2 = "xy<a/>";
		array<string> linesB2 = new array<string>;
		array<bool> crB2 = new array<bool>;
		string prefixB2 = "";
		bool endsB2 = true;
		VPPXmlText.SplitLines(caseB2, linesB2, crB2, prefixB2, endsB2);
		if (prefixB2 != "xy" || linesB2.Count() != 1 || linesB2[0] != "<a/>" || endsB2 || !RoundTripsAllSteps(caseB2))
		{
			failure = "(b) a short prefix before the first tag is not kept byte-exact";
			return false;
		}

		// Splitter window edges (8000 bytes): line 1 is longer than a window (CR at 8195, LF at 8196) and the CRLF of
		// line 2 spans the 16000-byte window edge (CR at 15999, LF at 16000).
		string xs = "x";
		for (int dx = 0; dx < 13; dx++)
		{
			xs = xs + xs;
		}

		string fill = xs.Substring(0, 7799);
		string caseL = "<a>" + xs + "\r\n";
		caseL = caseL + "<b>" + fill + "\r\n";
		string shortLines = "<c/>\n<d/>\r\n\n";
		for (int dl = 0; dl < 8; dl++)
		{
			shortLines = shortLines + shortLines;
		}

		caseL = caseL + shortLines + "</a>";
		if (!RoundTrips(caseL, 7) || !RoundTrips(caseL, 1000))
		{
			failure = "(b) content with lines across splitter windows does not round-trip";
			return false;
		}

		array<string> linesL = new array<string>;
		array<bool> crL = new array<bool>;
		string prefixL = "";
		bool endsL = true;
		VPPXmlText.SplitLines(caseL, linesL, crL, prefixL, endsL);
		int countL = linesL.Count();
		int crCountL = crL.Count();
		if (countL < 3 || crCountL != countL)
		{
			failure = "(b) wrong line model across splitter windows";
			return false;
		}

		string firstL = linesL[0];
		string secondL = linesL[1];
		string lastLine = linesL[countL - 1];
		bool cr0 = crL[0];
		bool cr1 = crL[1];
		if (countL != 771 || firstL.Length() != 8195 || !cr0 || secondL.Length() != 7802 || !cr1 || lastLine != "</a>" || endsL)
		{
			failure = "(b) wrong line model across splitter windows";
			return false;
		}

		array<string> edgeCases = new array<string>;
		edgeCases.Insert("");
		edgeCases.Insert("\n");
		edgeCases.Insert("\r\n\r\n");
		edgeCases.Insert("<a/>\r");
		edgeCases.Insert("<a/>\n\n\n");
		edgeCases.Insert("\n<a/>");
		edgeCases.Insert("<a>\r\r\n</a>");
		int edgeTotal = edgeCases.Count();
		for (int i = 0; i < edgeTotal; i++)
		{
			if (!RoundTripsAllSteps(edgeCases[i]))
			{
				failure = "(b) edge input " + i.ToString() + " does not round-trip";
				return false;
			}
		}

		return true;
	}

	// (c) to (j): tokenizer and DOM cases.
	static bool SelfTestParse(out string failure)
	{
		string caseC = "<types><type name=\"A\"><nominal>5</nominal></type></types>";
		VPPXmlDocument docC = ParseSmall(caseC);
		VPPXmlNode typeC = null;
		VPPXmlNode nomC = null;
		VPPXmlNode rootC = docC.GetRoot();
		if (rootC)
		{
			typeC = rootC.FirstChild("type");
		}

		if (typeC)
		{
			nomC = typeC.FirstChild("nominal");
		}

		if (docC.HasError() || !nomC)
		{
			string whyC = docC.GetError();
			if (whyC == "")
			{
				whyC = "no <types>/<type>/<nominal> tree was built";
			}

			failure = "(c) one-line types document does not parse: " + whyC;
			return false;
		}

		if (rootC.StartCol != 0 || rootC.OpenEndCol != 7 || rootC.CloseStartCol != 49 || rootC.EndCol != 57)
		{
			failure = "(c) wrong columns for <types>";
			return false;
		}

		if (typeC.StartCol != 7 || typeC.OpenEndCol != 22 || typeC.CloseStartCol != 42 || typeC.EndCol != 49 || typeC.GetAttr("name") != "A")
		{
			failure = "(c) wrong columns or name for <type>";
			return false;
		}

		if (nomC.StartCol != 22 || nomC.OpenEndCol != 31 || nomC.CloseStartCol != 32 || nomC.EndCol != 42 || nomC.InnerText() != "5")
		{
			failure = "(c) wrong columns or text for <nominal>";
			return false;
		}

		string sliceC = docC.Slice(0, 22, 0, 42);
		if (sliceC != "<nominal>5</nominal>")
		{
			failure = "(c) Slice returned the wrong text";
			return false;
		}

		string caseD = "<types>\n";
		caseD = caseD + "  <!-- <type name=\"X\">\n";
		caseD = caseD + "  <nominal>1</nominal></type> -->\n";
		caseD = caseD + "  <type name=\"B\"/>\n";
		caseD = caseD + "</types>\n";
		VPPXmlDocument docD = ParseSmall(caseD);
		VPPXmlNode rootD = docD.GetRoot();
		if (docD.HasError() || !rootD)
		{
			failure = "(d) document with a multi-line comment does not parse: " + docD.GetError();
			return false;
		}

		array<VPPXmlNode> typesD = new array<VPPXmlNode>;
		rootD.ChildrenNamed("type", typesD);
		VPPXmlNode commentD = rootD.ChildAt(0);
		if (typesD.Count() != 1 || rootD.ChildCount() != 2 || !commentD || commentD.Kind != VPPXmlNodeKind.COMMENT)
		{
			failure = "(d) tags inside a multi-line comment were not ignored";
			return false;
		}

		VPPXmlNode typeD = typesD[0];
		if (typeD.GetAttr("name") != "B" || commentD.StartLine != 1 || commentD.EndLine != 2)
		{
			failure = "(d) wrong element or comment span after a multi-line comment";
			return false;
		}

		string caseE = "<cargo chance= \"0.03\" a = 'b'/>";
		VPPXmlDocument docE = ParseSmall(caseE);
		VPPXmlNode rootE = docE.GetRoot();
		if (docE.HasError() || !rootE || rootE.GetAttr("chance") != "0.03" || rootE.GetAttr("a") != "b")
		{
			failure = "(e) spaced attribute assignment does not parse";
			return false;
		}

		string caseF = "<t name='Q' v='say \"hi\"'></t>";
		VPPXmlDocument docF = ParseSmall(caseF);
		VPPXmlNode rootF = docF.GetRoot();
		if (docF.HasError() || !rootF || rootF.GetAttr("name") != "Q" || rootF.GetAttr("v") != "say \"hi\"")
		{
			failure = "(f) single-quoted attributes do not parse";
			return false;
		}

		string caseG = "<r><flags a=\"1\" /><flags a=\"1\"/></r>";
		VPPXmlDocument docG = ParseSmall(caseG);
		VPPXmlNode rootG = docG.GetRoot();
		if (docG.HasError() || !rootG || rootG.ChildCount() != 2)
		{
			failure = "(g) self-closing elements do not parse";
			return false;
		}

		for (int g = 0; g < 2; g++)
		{
			VPPXmlNode flagsG = rootG.ChildAt(g);
			if (!flagsG || !flagsG.SelfClosing || flagsG.GetAttr("a") != "1" || flagsG.CloseStartLine != -1 || flagsG.EndCol != flagsG.OpenEndCol)
			{
				failure = "(g) wrong self-closing element " + g.ToString();
				return false;
			}
		}

		string caseH = "<a v=\"x &amp; y &lt;z&gt; &quot;q&quot; &amp;lt;\"/>";
		VPPXmlDocument docH = ParseSmall(caseH);
		VPPXmlNode rootH = docH.GetRoot();
		if (docH.HasError() || !rootH || rootH.GetAttr("v") != "x & y <z> \"q\" &lt;")
		{
			failure = "(h) entities in an attribute do not decode";
			return false;
		}

		string caseI = "<a>\n";
		caseI = caseI + "  <b>\n";
		caseI = caseI + "  </c>\n";
		caseI = caseI + "</a>\n";
		VPPXmlDocument docI = ParseSmall(caseI);
		if (!docI.HasError() || docI.GetErrorLine() != 3 || docI.GetRoot())
		{
			failure = "(i) a mismatched end tag is not reported on its line";
			return false;
		}

		string caseJ = "<root><point pos=\"1 > 2\" x='>'/><point/>\n";
		caseJ = caseJ + "<point\n";
		caseJ = caseJ + "  a=\"3 >\n";
		caseJ = caseJ + " 4\"/></root>\n";
		array<string> linesJ = new array<string>;
		array<bool> crJ = new array<bool>;
		string prefixJ = "";
		bool endsJ = false;
		VPPXmlText.SplitLines(caseJ, linesJ, crJ, prefixJ, endsJ);
		VPPXmlSelfTestSink sinkJ = new VPPXmlSelfTestSink();
		VPPXmlReader readerJ = new VPPXmlReader(sinkJ);
		readerJ.Begin(linesJ);
		bool readDone = false;
		while (!readDone)
		{
			readDone = readerJ.Step(64);
		}

		if (readerJ.HasError() || sinkJ.m_Starts != 4 || sinkJ.m_Ends != 1 || sinkJ.m_AttrTotal != 0)
		{
			failure = "(j) a sink without attributes lost the tag end past a quoted '>': " + readerJ.GetError();
			return false;
		}

		return true;
	}

	// (k) the binary-carry joiner over 1, 7, 8, 9, 17 and 600 lines (stack edge cases), both final-newline modes.
	static bool SelfTestJoiner(out string failure)
	{
		array<int> sizes = new array<int>;
		sizes.Insert(1);
		sizes.Insert(7);
		sizes.Insert(8);
		sizes.Insert(9);
		sizes.Insert(17);
		sizes.Insert(600);
		int sizeTotal = sizes.Count();
		for (int s = 0; s < sizeTotal; s++)
		{
			int lineCount = sizes[s];
			string countText = lineCount.ToString();
			for (int mode = 0; mode < 2; mode++)
			{
				bool endsNl = mode == 1;
				array<string> lines = new array<string>;
				array<bool> crFlags = new array<bool>;
				string expected = BuildJoinFixture(lineCount, endsNl, lines, crFlags);
				array<int> steps = new array<int>;
				steps.Insert(1);
				steps.Insert(5);
				steps.Insert(1000);
				for (int k = 0; k < 3; k++)
				{
					VPPXmlJoiner joiner = new VPPXmlJoiner();
					joiner.Begin(lines, crFlags, "", endsNl);
					bool joinDone = false;
					while (!joinDone)
					{
						joinDone = joiner.Step(steps[k]);
					}

					string joined = joiner.GetResult();
					if (joined != expected)
					{
						failure = "(k) the joiner differs for " + countText + " lines";
						return false;
					}
				}

				if (!RoundTripsAllSteps(expected))
				{
					failure = "(k) " + countText + " lines do not round-trip";
					return false;
				}
			}
		}

		return true;
	}

	// Lines "<l n=\"i\"/>" with a CR flag on every third line; returns the expected join built in 16-line blocks.
	static string BuildJoinFixture(int count, bool endsNl, array<string> outLines, array<bool> outCR)
	{
		array<string> blocks = new array<string>;
		string block = "";
		int blockLines = 0;
		for (int i = 0; i < count; i++)
		{
			string line = "<l n=\"" + i.ToString() + "\"/>";
			bool hasCR = (i % 3) == 1;
			outLines.Insert(line);
			outCR.Insert(hasCR);
			string eol = "";
			if (i < count - 1 || endsNl)
			{
				if (hasCR)
				{
					eol = "\r\n";
				}
				else
				{
					eol = "\n";
				}
			}

			block = block + line + eol;
			blockLines++;
			if (blockLines == 16)
			{
				blocks.Insert(block);
				block = "";
				blockLines = 0;
			}
		}

		if (blockLines > 0)
		{
			blocks.Insert(block);
		}

		string expected = "";
		int blockTotal = blocks.Count();
		for (int b = 0; b < blockTotal; b++)
		{
			expected = expected + blocks[b];
		}

		return expected;
	}

	// (l) Hash() must change when only the last char or one middle char of a >1 MB string changes, and the CR-stripped
	// comparison and hash must treat a CRLF sample and its LF copy as equal, also above the native Replace limit.
	// The variants are built by concatenation only: Substring returns at most 8191 bytes.
	static bool SelfTestHash(out string failure)
	{
		string unit = "abcdefgh";
		string pow = unit;
		string acc = "";
		string half = "";
		string halfMinus = "";
		for (int d = 0; d < 17; d++)
		{
			if (d == 16)
			{
				half = pow;
				halfMinus = acc;
			}

			acc = acc + pow;
			pow = pow + pow;
		}

		string big = pow;
		int bigLen = big.Length();
		if (bigLen <= 1000000)
		{
			failure = "(l) the Hash test string is too short";
			return false;
		}

		string rebuilt = acc + unit;
		if (rebuilt.Length() != bigLen || rebuilt != big)
		{
			failure = "(l) concatenation of a 1 MB string lost bytes";
			return false;
		}

		int baseHash = big.Hash();
		string lastVariant = acc + "abcdefgZ";
		if (lastVariant.Length() != bigLen || lastVariant.Hash() == baseHash)
		{
			failure = "(l) Hash() ignores a change of the last character of a 1 MB string";
			return false;
		}

		string midVariant = half + "Zbcdefgh" + halfMinus;
		if (midVariant.Length() != bigLen || midVariant.Hash() == baseHash)
		{
			failure = "(l) Hash() ignores a change of a middle character of a 1 MB string";
			return false;
		}

		string crlfSample = "<a>\r\n";
		crlfSample = crlfSample + "  <b/>\r\n";
		crlfSample = crlfSample + "</a>\r\n";
		string lfSample = "<a>\n";
		lfSample = lfSample + "  <b/>\n";
		lfSample = lfSample + "</a>\n";
		if (!VPPXmlText.SameNormalized(crlfSample, lfSample))
		{
			failure = "(l) SameNormalized treats CRLF and LF copies as different";
			return false;
		}

		if (VPPXmlText.NormalizedHash(crlfSample) != VPPXmlText.NormalizedHash(lfSample))
		{
			failure = "(l) NormalizedHash differs between CRLF and LF copies";
			return false;
		}

		if (VPPXmlText.SameNormalized(crlfSample, "<a>\n</a>\n"))
		{
			failure = "(l) SameNormalized treats different content as equal";
			return false;
		}

		// 1024 lines (about 44 KB): far above the 2560 bytes up to which native Replace is exact.
		string lfLine = "<type name=\"X\"><nominal>5</nominal></type>";
		string lfBig = lfLine + "\n";
		string crlfBig = lfLine + "\r\n";
		for (int dl = 0; dl < 10; dl++)
		{
			lfBig = lfBig + lfBig;
			crlfBig = crlfBig + crlfBig;
		}

		string strippedCrlf = VPPXmlText.StripCR(crlfBig);
		string strippedLf = VPPXmlText.StripCR(lfBig);
		int hashLf = VPPXmlText.NormalizedHash(lfBig);
		int hashCrlf = VPPXmlText.NormalizedHash(crlfBig);
		int hashEmpty = VPPXmlText.NormalizedHash("");
		string lfBigEdited = lfBig + "<x/>\n";
		int hashEdited = VPPXmlText.NormalizedHash(lfBigEdited);
		bool sameBig = VPPXmlText.SameNormalized(crlfBig, lfBig);
		bool sameEdited = VPPXmlText.SameNormalized(crlfBig, lfBigEdited);
		string convertedBig = VPPXmlText.CRLFToLF(crlfBig);
		bool bareLf = VPPXmlText.HasBareLF(lfBig);
		bool bareCrlf = VPPXmlText.HasBareLF(crlfBig);
		if (hashLf == hashEmpty)
		{
			failure = "(l) NormalizedHash of a 44 KB LF text equals the hash of an empty string";
			return false;
		}

		if (strippedCrlf != lfBig || strippedLf != lfBig)
		{
			failure = "(l) StripCR is not exact on a 44 KB text";
			return false;
		}

		if (hashCrlf != hashLf || !sameBig)
		{
			failure = "(l) CRLF and LF copies of a 44 KB text differ after normalization";
			return false;
		}

		if (sameEdited || hashEdited == hashLf)
		{
			failure = "(l) a change at the end of a 44 KB text is not seen";
			return false;
		}

		if (convertedBig != lfBig)
		{
			failure = "(l) CRLFToLF is not exact on a 44 KB text";
			return false;
		}

		if (!bareLf || bareCrlf)
		{
			failure = "(l) HasBareLF is wrong on a 44 KB text";
			return false;
		}

		return true;
	}
};
