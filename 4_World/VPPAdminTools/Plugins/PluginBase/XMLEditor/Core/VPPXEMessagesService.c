// XML Editor MESSAGES tab, server side: reads every MESSAGES file of the registry (db/messages.xml and the
// <ce type="messages"> files) for XE_GetMessages and applies XE_SaveMessages edits (UPDATE, ADD, DELETE of whole
// <message> elements) as line patches, so comments and the formatting of untouched messages stay byte for byte.
// Files are small: both jobs finish in one Step, like a restore.

class VPPXEMessagesIO
{
	// Parses content into doc; the <messages> root, or null (ParseError tells why). No out parameters and every
	// local declared at the top: a crash dump showed two overlapping local slots in the first version of the list
	// job, which had an uninitialized out-parameter target declared inside nested blocks.
	static VPPXmlNode ParseFile(VPPXmlDocument doc, string content)
	{
		VPPXmlNode root = null;
		doc.BeginString(content);
		doc.ParseAll();
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, "messages");
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, "messages"))
		{
			return null;
		}

		return root;
	}

	static string ParseError(VPPXmlDocument doc)
	{
		if (doc.HasError())
		{
			return "#VSTR_XMLE_ERR_PARSE";
		}

		return "#VSTR_XMLE_MSG_ERR_ROOT";
	}

	// The <message> element children of the root, in document order.
	static void MessageNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (kid && kid.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(kid.Name, "message"))
			{
				outNodes.Insert(kid);
			}
		}
	}

	// Fields the way the server reads them: first child of each name; an unreadable number is 0.
	static void RowOf(VPPXmlNode node, VPPXEMessageRow row)
	{
		row.Line = node.StartLine + 1;
		row.Delay = IntChild(node, "delay");
		row.Repeat = IntChild(node, "repeat");
		row.Deadline = IntChild(node, "deadline");
		row.OnConnect = IntChild(node, "onconnect");
		row.Shutdown = IntChild(node, "shutdown");
		row.Time = TextChild(node, "time");
		row.Text = TextChild(node, "text");
	}

	protected static int IntChild(VPPXmlNode node, string name)
	{
		string text = TextChild(node, name);
		if (text == "")
		{
			return 0;
		}

		return text.ToInt();
	}

	protected static string TextChild(VPPXmlNode node, string name)
	{
		VPPXmlNode child = node.FirstChild(name);
		if (!child)
		{
			return "";
		}

		return child.InnerText();
	}

	// A <message> block: the first line carries no indentation when firstBare (it replaces an element in place).
	static void BuildBlock(VPPXEMessageRow row, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		outLines.Clear();
		if (firstBare)
		{
			outLines.Insert("<message>");
		}
		else
		{
			outLines.Insert(indent + "<message>");
		}

		if (row.Delay > 0)
		{
			outLines.Insert(childIndent + "<delay>" + row.Delay.ToString() + "</delay>");
		}

		if (row.Repeat > 0)
		{
			outLines.Insert(childIndent + "<repeat>" + row.Repeat.ToString() + "</repeat>");
		}

		if (row.Deadline > 0)
		{
			outLines.Insert(childIndent + "<deadline>" + row.Deadline.ToString() + "</deadline>");
		}

		if (row.OnConnect == 1)
		{
			outLines.Insert(childIndent + "<onconnect>1</onconnect>");
		}

		if (row.Shutdown == 1)
		{
			outLines.Insert(childIndent + "<shutdown>1</shutdown>");
		}

		if (row.Time != "")
		{
			outLines.Insert(childIndent + "<time>" + EncodeValue(row.Time) + "</time>");
		}

		outLines.Insert(childIndent + "<text>" + EncodeValue(row.Text) + "</text>");
		outLines.Insert(indent + "</message>");
	}

	// & < > escaped character by character (VPPXmlText.EncodeText uses native Replace, which is exact only up to
	// 2560 bytes; a 1151-byte text of ampersands grows past that).
	static string EncodeValue(string value)
	{
		string encoded = "";
		int len = value.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = value.Get(i);
			if (ch == "&")
			{
				encoded = encoded + "&amp;";
			}
			else if (ch == "<")
			{
				encoded = encoded + "&lt;";
			}
			else if (ch == ">")
			{
				encoded = encoded + "&gt;";
			}
			else
			{
				encoded = encoded + ch;
			}
		}

		return encoded;
	}

	static bool StartsLine(array<string> lines, VPPXmlNode node)
	{
		if (!node || node.StartLine < 0 || node.StartLine >= lines.Count())
		{
			return false;
		}

		string lead = VPPXmlText.LeadingWs(lines[node.StartLine]);
		return lead.Length() == node.StartCol;
	}

	// Only whitespace follows the element on its last line.
	static bool EndsLine(array<string> lines, VPPXmlNode node)
	{
		if (!node || node.EndLine < 0 || node.EndLine >= lines.Count())
		{
			return false;
		}

		string line = lines[node.EndLine];
		string rest = VPPXETypesWriter.Mid(line, node.EndCol, line.Length() - node.EndCol);
		return VPPXmlText.IsBlank(rest);
	}

	static string IndentOf(array<string> lines, VPPXmlNode node)
	{
		if (!StartsLine(lines, node))
		{
			return "";
		}

		return VPPXmlText.LeadingWs(lines[node.StartLine]);
	}

	// Indentation of a message's children: its first child element that starts a line, else indent + one unit.
	static string ChildIndentOf(array<string> lines, VPPXmlNode node, string indent)
	{
		int kids = node.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = node.ChildAt(i);
			if (kid && kid.Kind == VPPXmlNodeKind.ELEMENT && StartsLine(lines, kid))
			{
				return VPPXmlText.LeadingWs(lines[kid.StartLine]);
			}
		}

		return indent + IndentUnit(indent);
	}

	static string IndentUnit(string sampleIndent)
	{
		if (sampleIndent == "" || sampleIndent.IndexOf("\t") >= 0)
		{
			return "\t";
		}

		if (sampleIndent.Length() > 8)
		{
			return "    ";
		}

		return sampleIndent;
	}

	// Replaces [startLine:startCol, endLine:endCol) with newLines (the text before the span prefixes the first new
	// line, the text after it ends the last one). endLine may be lines.Count() (the span runs to the end).
	static bool ApplySpan(array<string> lines, array<bool> crFlags, int startLine, int startCol, int endLine, int endCol, array<string> newLines, bool cr)
	{
		int count = lines.Count();
		if (count == 0 || startLine < 0 || startLine >= count || endLine < startLine || startCol < 0 || endCol < 0)
		{
			return false;
		}

		string firstText = lines[startLine];
		string prefix = VPPXETypesWriter.Mid(firstText, 0, startCol);
		string suffix = "";
		int lastIdx = endLine;
		bool lastFlag = cr;
		if (endLine >= count)
		{
			lastIdx = count - 1;
			lastFlag = crFlags[lastIdx];
		}
		else
		{
			string lastText = lines[endLine];
			suffix = VPPXETypesWriter.Mid(lastText, endCol, lastText.Length() - endCol);
			lastFlag = crFlags[endLine];
		}

		array<string> result = new array<string>();
		int newCount = newLines.Count();
		if (newCount == 0)
		{
			result.Insert(prefix + suffix);
		}
		else
		{
			for (int i = 0; i < newCount; i++)
			{
				string text = newLines[i];
				if (i == 0)
				{
					text = prefix + text;
				}

				if (i == newCount - 1)
				{
					text = text + suffix;
				}

				result.Insert(text);
			}
		}

		for (int r = lastIdx; r >= startLine; r--)
		{
			lines.RemoveOrdered(r);
			crFlags.RemoveOrdered(r);
		}

		int resultCount = result.Count();
		for (int j = 0; j < resultCount; j++)
		{
			lines.InsertAt(result[j], startLine + j);
			if (j == resultCount - 1)
			{
				crFlags.InsertAt(lastFlag, startLine + j);
			}
			else
			{
				crFlags.InsertAt(cr, startLine + j);
			}
		}

		return true;
	}

	// Inserts whole lines before a closing tag (a closing tag that shares its line moves to a line of its own).
	static bool InsertBeforeClose(array<string> lines, array<bool> crFlags, int line, int col, array<string> block, bool cr)
	{
		if (line < 0 || line >= lines.Count() || col < 0)
		{
			return false;
		}

		string current = lines[line];
		string before = VPPXETypesWriter.Mid(current, 0, col);
		if (VPPXmlText.IsBlank(before))
		{
			for (int i = block.Count() - 1; i >= 0; i--)
			{
				lines.InsertAt(block[i], line);
				crFlags.InsertAt(cr, line);
			}

			return true;
		}

		bool lineCr = crFlags[line];
		string after = VPPXETypesWriter.Mid(current, col, current.Length() - col);
		lines.Set(line, before);
		crFlags.Set(line, cr);
		int at = line + 1;
		foreach (string blockLine : block)
		{
			lines.InsertAt(blockLine, at);
			crFlags.InsertAt(cr, at);
			at++;
		}

		lines.InsertAt(VPPXmlText.LeadingWs(current) + after, at);
		crFlags.InsertAt(lineCr, at);
		return true;
	}
};

// XE_GetMessages: every MESSAGES file of the registry, in chunks of rows (one chunk at least per file; one empty
// chunk with FileCount 0 when there is no messages file at all).
class VPPXEMessagesListJob : VPPXEJob
{
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected bool m_Replied;

	void VPPXEMessagesListJob(PlayerIdentity sender, int reqId)
	{
		m_Sender = sender;
		m_ReqId = reqId;
	}

	override string GetLabel()
	{
		return "MessagesList";
	}

	override bool Step()
	{
		Run();
		return true;
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			m_Replied = true;
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
		}
	}

	// m_Replied is set once a reply went out, so a script error before that still answers (OnAborted).
	protected void Run()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			m_Replied = true;
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
			return;
		}

		array<VPPXEFileEntry> files = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.MESSAGES, files);
		int fileCount = files.Count();
		if (fileCount == 0)
		{
			VPPXEMessagesChunk emptyChunk = NewChunk(-1, 0, "");
			emptyChunk.ChunkCount = 1;
			SendChunk(emptyChunk);
			m_Replied = true;
			return;
		}

		for (int fi = 0; fi < fileCount; fi++)
		{
			SendFile(editor, files[fi], fi, fileCount);
		}

		m_Replied = true;
	}

	protected void SendFile(XMLEditor editor, VPPXEFileEntry entry, int fileIdx, int fileCount)
	{
		array<ref VPPXEMessagesChunk> chunks = new array<ref VPPXEMessagesChunk>();
		VPPXEMessagesChunk firstChunk = NewChunk(fileIdx, fileCount, entry.Key);
		chunks.Insert(firstChunk);
		string errorKey = FillChunks(editor, entry, fileIdx, fileCount, chunks);
		int chunkCount = chunks.Count();
		for (int c = 0; c < chunkCount; c++)
		{
			VPPXEMessagesChunk chunk = chunks[c];
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
			chunk.ErrorKey = errorKey;
			SendChunk(chunk);
		}
	}

	// Reads and parses the file and fills chunks (chunks[0] exists) with its rows; the error key, "" when readable.
	protected string FillChunks(XMLEditor editor, VPPXEFileEntry entry, int fileIdx, int fileCount, array<ref VPPXEMessagesChunk> chunks)
	{
		string content = "";
		int revision = 0;
		VPPXmlDocument doc = new VPPXmlDocument();
		VPPXmlNode root = null;
		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXEMessagesChunk current = chunks[0];
		bool editable = false;
		int bytes = 0;
		int nodeCount = 0;
		if (!FileExist(entry.Path))
		{
			return "#VSTR_XMLE_STATE_MISSING";
		}

		if (!VPPXmlText.ReadAll(entry.Path, content))
		{
			editor.GetRegistry().NoteRevision(entry.Key, 0, false);
			return "#VSTR_XMLE_ERR_READ";
		}

		revision = VPPXmlText.NormalizedHash(content);
		editor.GetRegistry().NoteRevision(entry.Key, revision, true);
		current.Revision = revision;
		root = VPPXEMessagesIO.ParseFile(doc, content);
		if (!root)
		{
			return VPPXEMessagesIO.ParseError(doc);
		}

		if (!editor.IsReadOnlyMode() && !root.SelfClosing)
		{
			editable = true;
		}

		VPPXEMessagesIO.MessageNodes(root, nodes);
		nodeCount = nodes.Count();
		if (nodeCount > VPPXEMessageRules.MAX_ROWS)
		{
			nodeCount = VPPXEMessageRules.MAX_ROWS;
			editable = false;
		}

		current.Editable = editable;
		for (int i = 0; i < nodeCount; i++)
		{
			VPPXEMessageRow row = new VPPXEMessageRow();
			VPPXEMessagesIO.RowOf(nodes[i], row);
			int size = VPPXEMessageRules.EstimateRow(row);
			if (current.Rows.Count() > 0 && bytes + size > VPPXEMessageRules.CHUNK_BYTES)
			{
				current = NewChunk(fileIdx, fileCount, entry.Key);
				current.Revision = revision;
				current.Editable = editable;
				chunks.Insert(current);
				bytes = 0;
			}

			current.Rows.Insert(row);
			bytes += size;
		}

		return "";
	}

	protected VPPXEMessagesChunk NewChunk(int fileIdx, int fileCount, string fileKey)
	{
		VPPXEMessagesChunk chunk = new VPPXEMessagesChunk();
		chunk.ReqId = m_ReqId;
		chunk.FileIdx = fileIdx;
		chunk.FileCount = fileCount;
		chunk.FileKey = fileKey;
		chunk.Editable = false;
		return chunk;
	}

	protected void SendChunk(VPPXEMessagesChunk chunk)
	{
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnMessagesChunk");
		Param1<ref VPPXEMessagesChunk> netPayload = new Param1<ref VPPXEMessagesChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnMessagesChunk", netRpc);
	}
};

// XE_SaveMessages: the assembled edits of one file, applied against the revision the client edited.
class VPPXEMessagesSaveJob : VPPXEJob
{
	protected PlayerIdentity m_Sender;
	protected ref VPPXEMessagesSave m_Save;
	protected bool m_Replied;

	void VPPXEMessagesSaveJob(PlayerIdentity sender, VPPXEMessagesSave save)
	{
		m_Sender = sender;
		m_Save = save;
	}

	override string GetLabel()
	{
		return "MessagesSave";
	}

	override bool Step()
	{
		Run();
		return true;
	}

	override void OnAborted()
	{
		if (!m_Replied)
		{
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
		}
	}

	protected void Reply(bool ok, string key, string arg)
	{
		m_Replied = true;
		VPPXENet.Result(m_Sender, m_Save.ReqId, ok, key, arg);
	}

	protected void Run()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry() || !editor.GetBackups())
		{
			Reply(false, "#VSTR_XMLE_ERR_NOT_READY", "");
			return;
		}

		if (editor.IsReadOnlyMode())
		{
			Reply(false, "#VSTR_XMLE_ERR_READONLY_FILE", "");
			return;
		}

		string fileKey = m_Save.FileKey;
		VPPXEFileEntry entry = editor.GetRegistry().Find(fileKey);
		if (!entry || entry.Kind != VPPXEFileKind.MESSAGES)
		{
			Reply(false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		string content = "";
		if (!FileExist(entry.Path) || !VPPXmlText.ReadAll(entry.Path, content))
		{
			Reply(false, "#VSTR_XMLE_ERR_READ", "");
			return;
		}

		if (VPPXmlText.NormalizedHash(content) != m_Save.BaseRevision)
		{
			Reply(false, "#VSTR_XMLE_ERR_STALE", "");
			return;
		}

		VPPXmlDocument doc = new VPPXmlDocument();
		VPPXmlNode root = VPPXEMessagesIO.ParseFile(doc, content);
		if (!root)
		{
			Reply(false, VPPXEMessagesIO.ParseError(doc), "");
			return;
		}

		if (root.SelfClosing || root.CloseStartLine < 0)
		{
			Reply(false, "#VSTR_XMLE_MSG_ERR_ROOT", "");
			return;
		}

		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXEMessagesIO.MessageNodes(root, nodes);
		int nodeCount = nodes.Count();

		// 1. Validate: known ops, indexes in range and used once, every written row passes the rules.
		map<int, int> byIndex = new map<int, int>();
		int adds = 0;
		int updates = 0;
		int deletes = 0;
		int editCount = m_Save.Edits.Count();
		for (int e = 0; e < editCount; e++)
		{
			VPPXEMessageEdit edit = m_Save.Edits[e];
			if (!edit)
			{
				Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
				return;
			}

			if (edit.Op == VPPXEOp.ADD)
			{
				adds++;
			}
			else if (edit.Op == VPPXEOp.UPDATE || edit.Op == VPPXEOp.DELETE)
			{
				if (edit.Index < 0 || edit.Index >= nodeCount || byIndex.Contains(edit.Index))
				{
					Reply(false, "#VSTR_XMLE_ERR_STALE", "");
					return;
				}

				byIndex.Set(edit.Index, e);
				if (edit.Op == VPPXEOp.UPDATE)
				{
					updates++;
				}
				else
				{
					deletes++;
				}
			}
			else
			{
				Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
				return;
			}

			if (edit.Op != VPPXEOp.DELETE)
			{
				string ruleKey = VPPXEMessageRules.CheckRow(edit.Row);
				if (ruleKey != "")
				{
					int shown = e + 1;
					Reply(false, ruleKey, shown.ToString());
					return;
				}
			}
		}

		int expected = nodeCount + adds - deletes;
		if (expected > VPPXEMessageRules.MAX_ROWS)
		{
			Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return;
		}

		// 2. Patch: ADDs before </messages> first (nothing earlier moves), then UPDATE and DELETE from the last
		// message up, so every span still points at the original text.
		array<string> lines = new array<string>();
		lines.Copy(doc.GetLines());
		array<bool> crFlags = new array<bool>();
		crFlags.Copy(doc.GetCRFlags());
		bool cr = doc.DominantIsCR();
		string addIndent = "\t";
		string addChildIndent = "\t\t";
		if (nodeCount > 0)
		{
			VPPXmlNode lastNode = nodes[nodeCount - 1];
			if (VPPXEMessagesIO.StartsLine(lines, lastNode))
			{
				addIndent = VPPXEMessagesIO.IndentOf(lines, lastNode);
				addChildIndent = VPPXEMessagesIO.ChildIndentOf(lines, lastNode, addIndent);
			}
		}

		array<string> addLines = new array<string>();
		array<string> block = new array<string>();
		for (int a = 0; a < editCount; a++)
		{
			VPPXEMessageEdit addEdit = m_Save.Edits[a];
			if (addEdit.Op != VPPXEOp.ADD)
			{
				continue;
			}

			VPPXEMessagesIO.BuildBlock(addEdit.Row, addIndent, addChildIndent, false, block);
			foreach (string addLine : block)
			{
				addLines.Insert(addLine);
			}
		}

		if (addLines.Count() > 0 && !VPPXEMessagesIO.InsertBeforeClose(lines, crFlags, root.CloseStartLine, root.CloseStartCol, addLines, cr))
		{
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "add");
			return;
		}

		for (int n = nodeCount - 1; n >= 0; n--)
		{
			if (!byIndex.Contains(n))
			{
				continue;
			}

			VPPXmlNode node = nodes[n];
			VPPXEMessageEdit nodeEdit = m_Save.Edits[byIndex.Get(n)];
			bool applied = false;
			array<string> spanLines = new array<string>();
			if (nodeEdit.Op == VPPXEOp.DELETE)
			{
				if (VPPXEMessagesIO.StartsLine(lines, node) && VPPXEMessagesIO.EndsLine(lines, node))
				{
					applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, 0, node.EndLine + 1, 0, spanLines, cr);
				}
				else
				{
					applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, node.StartCol, node.EndLine, node.EndCol, spanLines, cr);
				}
			}
			else
			{
				string indent = VPPXEMessagesIO.IndentOf(lines, node);
				string childIndent = VPPXEMessagesIO.ChildIndentOf(lines, node, indent);
				VPPXEMessagesIO.BuildBlock(nodeEdit.Row, indent, childIndent, true, spanLines);
				applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, node.StartCol, node.EndLine, node.EndCol, spanLines, cr);
			}

			if (!applied)
			{
				Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "span");
				return;
			}
		}

		string newContent = VPPXmlText.JoinLines(lines, crFlags, doc.GetPrefix(), doc.EndsWithNewline());

		// 3. The result must parse to the expected number of messages before anything is written.
		VPPXmlDocument check = new VPPXmlDocument();
		VPPXmlNode checkRoot = VPPXEMessagesIO.ParseFile(check, newContent);
		array<VPPXmlNode> checkNodes = new array<VPPXmlNode>();
		if (checkRoot)
		{
			VPPXEMessagesIO.MessageNodes(checkRoot, checkNodes);
		}

		if (!checkRoot || checkNodes.Count() != expected)
		{
			VPPXELog.Warn("Messages save of " + fileKey + " produced an unexpected document; nothing was written");
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "verify");
			return;
		}

		// 4. Backup, verified write.
		VPPXEBackupManager backups = editor.GetBackups();
		string summary = "messages: " + updates.ToString() + " changed, " + adds.ToString() + " added, " + deletes.ToString() + " removed";
		string backupId = backups.CreateBackup(entry, content, VPPXEBackupReason.EDIT, m_Sender, summary, editCount, null, "");
		if (backupId == "")
		{
			Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
			return;
		}

		string writeErr = "";
		string writeArg = "";
		string noticeKey = "";
		if (!VPPXESafeWrite.Write(entry, newContent, backupId, content, true, writeErr, writeArg, noticeKey))
		{
			backups.ApplyRetention();
			VPPXELog.Action(m_Sender, "Saving " + fileKey + " failed: " + writeErr + " " + writeArg, false);
			Reply(false, writeErr, writeArg);
			return;
		}

		backups.SetResultHash(backupId, VPPXmlText.NormalizedHash(newContent));
		backups.ApplyRetention();
		VPPXELog.Action(m_Sender, "Saved " + fileKey + " (" + summary + ", backup " + backupId + ")", true);
		editor.OnFileWritten(fileKey, m_Sender);
		Reply(true, "#VSTR_XMLE_MSG_SAVED", fileKey);
	}
};
