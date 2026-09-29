// XML Editor SPAWNABLES tab, server side: reads every RANDOMPRESETS file then every SPAWNABLETYPES file of the registry
// (the server's load order) for XE_GetSpawnables, and applies XE_SaveSpawnables edits (UPDATE, ADD, DELETE of whole
// <type> elements, or of whole <cargo> / <attachments> presets) as line patches. Untouched rows, comments and formatting
// stay byte for byte; inside an edited row every child that did not change (and every comment or element the editor
// does not know) is copied from its original text, so only what was edited gets new text. Rules: VPPXESpwRules
// (3_Game VPPXESpawnables.c).

class VPPXESpwIO
{
	static string RootNameOf(int fileKind)
	{
		if (fileKind == VPPXEFileKind.RANDOMPRESETS)
		{
			return "randompresets";
		}

		return "spawnabletypes";
	}

	static VPPXmlNode RootOf(VPPXmlDocument doc, int fileKind)
	{
		VPPXmlNode root = null;
		string rootName = RootNameOf(fileKind);
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, rootName);
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, rootName))
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

		return "#VSTR_XMLE_SPB_ERR_ROOT";
	}

	// A row of the file: <type> of a spawnabletypes file, <cargo> / <attachments> of a randompresets file.
	static bool IsRowElement(VPPXmlNode node, int fileKind)
	{
		if (!node || node.Kind != VPPXmlNodeKind.ELEMENT)
		{
			return false;
		}

		int kind = VPPXESpwKind.KindOf(node.Name);
		if (fileKind == VPPXEFileKind.RANDOMPRESETS)
		{
			return VPPXESpwKind.IsBlock(kind);
		}

		return kind == VPPXESpwKind.TYPE;
	}

	static void RowNodes(VPPXmlNode root, int fileKind, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (IsRowElement(kid, fileKind))
			{
				outNodes.Insert(kid);
			}
		}
	}

	// The <damage> right under a spawnabletypes root (the file's default damage), null when none.
	static VPPXmlNode FileDamageNode(VPPXmlNode root)
	{
		if (!root)
		{
			return null;
		}

		return root.FirstChild("damage");
	}

	static void RowOf(VPPXmlDocument doc, VPPXmlNode el, VPPXESpwNode row)
	{
		NodeOf(doc, el, -1, 0, row);
		row.Line = el.StartLine + 1;
	}

	// The node of an element and, recursively, of its children. A child the element's kind cannot hold (or one
	// deeper than MAX_DEPTH) is kept as OTHER, like a comment.
	static void NodeOf(VPPXmlDocument doc, VPPXmlNode el, int origIdx, int depth, VPPXESpwNode node)
	{
		node.Kind = VPPXESpwKind.KindOf(el.Name);
		node.OrigIdx = origIdx;
		ReadAttrs(el, node);
		node.Kids.Clear();
		int kids = el.ChildCount();
		for (int k = 0; k < kids; k++)
		{
			VPPXmlNode child = el.ChildAt(k);
			if (!child)
			{
				continue;
			}

			VPPXESpwNode kid = new VPPXESpwNode();
			int childKind = VPPXESpwKind.OTHER;
			if (child.Kind == VPPXmlNodeKind.ELEMENT)
			{
				childKind = VPPXESpwKind.KindOf(child.Name);
			}

			bool known = childKind != VPPXESpwKind.OTHER && VPPXESpwRules.AllowsKid(node.Kind, childKind) && depth + 1 <= VPPXESpwRules.MAX_DEPTH;
			if (known)
			{
				NodeOf(doc, child, k, depth + 1, kid);
			}
			else
			{
				kid.Kind = VPPXESpwKind.OTHER;
				kid.OrigIdx = k;
				kid.Label = LabelOf(doc, child);
			}

			node.Kids.Insert(kid);
		}
	}

	// What the UI shows for a kept child: the comment text, or the element's opening tag name.
	protected static string LabelOf(VPPXmlDocument doc, VPPXmlNode child)
	{
		string text = "";
		if (child.Kind == VPPXmlNodeKind.COMMENT)
		{
			text = VPPXEGroupsIO.CommentText(doc, child);
		}
		else
		{
			text = "<" + child.Name + ">";
		}

		text = text.Trim();
		if (text.Length() > 60)
		{
			text = text.Substring(0, 57) + "...";
		}

		return text;
	}

	// The attributes the kind knows go to their fields, every other one to Extra (written back as it was).
	protected static void ReadAttrs(VPPXmlNode el, VPPXESpwNode node)
	{
		int count = el.AttrCount();
		for (int i = 0; i < count; i++)
		{
			string attrName = el.AttrNames[i];
			string attrValue = el.AttrValues[i];
			string lower = attrName;
			lower.ToLower();
			if (!TakeAttr(node, lower, attrValue))
			{
				node.Extra = node.Extra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
			}
		}
	}

	protected static bool TakeAttr(VPPXESpwNode node, string lower, string value)
	{
		int kind = node.Kind;
		if (lower == "name" && (kind == VPPXESpwKind.TYPE || kind == VPPXESpwKind.BIND || kind == VPPXESpwKind.ITEM || VPPXESpwKind.IsBlock(kind)))
		{
			node.Name = value;
			return true;
		}

		if ((lower == "min" || lower == "max") && (kind == VPPXESpwKind.DAMAGE || kind == VPPXESpwKind.SUBCOUNTER))
		{
			if (lower == "min")
			{
				node.Min = value;
			}
			else
			{
				node.Max = value;
			}

			return true;
		}

		if (lower == "batch" && kind == VPPXESpwKind.SPAWN)
		{
			node.Min = value;
			return true;
		}

		if (lower == "chance" && (kind == VPPXESpwKind.ITEM || VPPXESpwKind.IsBlock(kind)))
		{
			node.Chance = value;
			return true;
		}

		if (lower == "preset" && VPPXESpwKind.IsBlock(kind))
		{
			node.Preset = value;
			return true;
		}

		if (kind != VPPXESpwKind.ITEM)
		{
			return false;
		}

		if (lower == "quantmin")
		{
			node.QuantMin = value;
			return true;
		}

		if (lower == "quantmax")
		{
			node.QuantMax = value;
			return true;
		}

		if (lower == "equip")
		{
			node.Equip = value;
			return true;
		}

		return false;
	}

	// ---------------------------------------------------------------- writer

	// A row block. orig (may be null) is the element it replaces: its unchanged children and every comment or unknown
	// element are copied as they are. The first line carries no indentation when firstBare.
	static void BuildBlock(VPPXESpwNode row, VPPXmlDocument doc, VPPXmlNode orig, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		outLines.Clear();
		bool hasKids = row.Kids.Count() > 0;
		string head = OpenTag(row, !hasKids);
		if (firstBare)
		{
			outLines.Insert(head);
		}
		else
		{
			outLines.Insert(indent + head);
		}

		if (!hasKids)
		{
			return;
		}

		string step = VPPXEEventsIO.StepOf(indent, childIndent);
		AppendKids(row, doc, orig, childIndent, step, 1, outLines);
		outLines.Insert(VPPXEEventsIO.CloseIndentOf(doc, orig, indent) + "</" + VPPXESpwKind.TagOf(row.Kind) + ">");
	}

	protected static void AppendKids(VPPXESpwNode node, VPPXmlDocument doc, VPPXmlNode origEl, string kidIndent, string step, int depth, array<string> outLines)
	{
		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (!kid)
			{
				continue;
			}

			VPPXmlNode origKid = null;
			if (origEl && doc && kid.OrigIdx >= 0)
			{
				origKid = origEl.ChildAt(kid.OrigIdx);
			}

			if (origKid && (kid.Kind == VPPXESpwKind.OTHER || Unchanged(doc, kid, origKid, depth)))
			{
				VPPXEEventsIO.AppendKept(doc, origKid, kidIndent, outLines);
				continue;
			}

			// a kept child whose original is gone (cannot happen with a valid edit): nothing to copy
			if (kid.Kind == VPPXESpwKind.OTHER)
			{
				continue;
			}

			if (kid.Kids.Count() == 0)
			{
				outLines.Insert(kidIndent + OpenTag(kid, true));
				continue;
			}

			outLines.Insert(kidIndent + OpenTag(kid, false));
			VPPXmlNode origForKids = null;
			if (origKid && origKid.Kind == VPPXmlNodeKind.ELEMENT && VPPXESpwKind.KindOf(origKid.Name) == kid.Kind)
			{
				origForKids = origKid;
			}

			AppendKids(kid, doc, origForKids, kidIndent + step, step, depth + 1, outLines);
			outLines.Insert(kidIndent + "</" + VPPXESpwKind.TagOf(kid.Kind) + ">");
		}
	}

	// The child is what the original element reads as (same index, same content, deep).
	protected static bool Unchanged(VPPXmlDocument doc, VPPXESpwNode kid, VPPXmlNode origKid, int depth)
	{
		if (origKid.Kind != VPPXmlNodeKind.ELEMENT || VPPXESpwKind.KindOf(origKid.Name) != kid.Kind)
		{
			return false;
		}

		VPPXESpwNode parsed = new VPPXESpwNode();
		NodeOf(doc, origKid, kid.OrigIdx, depth, parsed);
		return kid.SameAs(parsed);
	}

	// The opening tag of a node (self-closing when it has no children), attributes in the vanilla order.
	static string OpenTag(VPPXESpwNode node, bool selfClose)
	{
		int kind = node.Kind;
		string tag = "<" + VPPXESpwKind.TagOf(kind);
		if (kind == VPPXESpwKind.TYPE || kind == VPPXESpwKind.BIND)
		{
			tag = tag + Attr("name", node.Name);
		}
		else if (kind == VPPXESpwKind.DAMAGE || kind == VPPXESpwKind.SUBCOUNTER)
		{
			tag = tag + Attr("min", node.Min) + Attr("max", node.Max);
		}
		else if (kind == VPPXESpwKind.SPAWN)
		{
			tag = tag + Attr("batch", node.Min);
		}
		else if (VPPXESpwKind.IsBlock(kind))
		{
			tag = tag + Attr("chance", node.Chance) + Attr("name", node.Name) + Attr("preset", node.Preset);
		}
		else if (kind == VPPXESpwKind.ITEM)
		{
			tag = tag + Attr("name", node.Name) + Attr("chance", node.Chance);
			tag = tag + Attr("quantmin", node.QuantMin) + Attr("quantmax", node.QuantMax) + Attr("equip", node.Equip);
		}

		tag = tag + node.Extra;
		if (selfClose)
		{
			return tag + " />";
		}

		return tag + ">";
	}

	protected static string Attr(string attrName, string value)
	{
		if (value == "")
		{
			return "";
		}

		return " " + attrName + "=\"" + VPPXmlText.EncodeAttr(value) + "\"";
	}
};

// XE_GetSpawnables: every RANDOMPRESETS then every SPAWNABLETYPES file of the registry in chunks of rows (one chunk at
// least per file; one empty chunk with FileCount 0 when there is none). The first chunk carries the ignore list.
class VPPXESpawnablesListJob : VPPXEJob
{
	const static int PH_START = 0;
	const static int PH_FILE_OPEN = 1;
	const static int PH_FILE_PARSE = 2;
	const static int PH_DONE = 3;

	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected int m_Phase;
	protected ref array<string> m_FileKeys;
	protected ref array<int> m_FileKinds;
	protected int m_FileIdx;
	protected ref VPPXmlDocument m_Doc;
	protected int m_DocRevision;
	protected ref array<string> m_Ignored;
	protected bool m_IgnoredSent;

	void VPPXESpawnablesListJob(PlayerIdentity sender, int reqId)
	{
		m_Sender = sender;
		m_ReqId = reqId;
		m_Phase = PH_START;
		m_FileKeys = new array<string>();
		m_FileKinds = new array<int>();
		m_Ignored = new array<string>();
	}

	override string GetLabel()
	{
		return "SpawnablesList";
	}

	// The client waits for every chunk: an abort before the end answers even when some chunks went out.
	override void OnAborted()
	{
		if (m_Phase != PH_DONE)
		{
			m_Phase = PH_DONE;
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_INTERNAL", GetLabel());
		}
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_START)
			{
				Start();
			}
			else if (m_Phase == PH_FILE_OPEN)
			{
				OpenFile();
			}
			else if (m_Phase == PH_FILE_PARSE)
			{
				if (!m_Doc.Step())
				{
					return false;
				}

				SendParsedFile();
				m_Doc = null;
				m_FileIdx++;
				m_Phase = PH_FILE_OPEN;
			}
			else
			{
				return true;
			}
		}

		return m_Phase == PH_DONE;
	}

	protected void Start()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			m_Phase = PH_DONE;
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
			return;
		}

		array<VPPXEFileEntry> presets = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.RANDOMPRESETS, presets);
		foreach (VPPXEFileEntry presetEntry : presets)
		{
			m_FileKeys.Insert(presetEntry.Key);
			m_FileKinds.Insert(VPPXEFileKind.RANDOMPRESETS);
		}

		array<VPPXEFileEntry> spawnables = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.SPAWNABLETYPES, spawnables);
		foreach (VPPXEFileEntry spawnableEntry : spawnables)
		{
			m_FileKeys.Insert(spawnableEntry.Key);
			m_FileKinds.Insert(VPPXEFileKind.SPAWNABLETYPES);
		}

		if (editor.GetTypes())
		{
			editor.GetTypes().IgnoredNames(m_Ignored);
		}

		m_FileIdx = 0;
		m_Phase = PH_FILE_OPEN;
	}

	// Reads the next file and starts its parse, or sends the error chunk of an unreadable one, or finishes.
	protected void OpenFile()
	{
		string content = "";
		XMLEditor editor = GetXMLEditor();
		int fileCount = m_FileKeys.Count();
		VPPXEFileEntry entry = null;
		if (fileCount == 0)
		{
			VPPXESpawnablesChunk emptyChunk = NewChunk(-1, 0, "", -1);
			emptyChunk.ChunkCount = 1;
			SendChunk(emptyChunk);
			m_Phase = PH_DONE;
			return;
		}

		if (m_FileIdx >= fileCount)
		{
			m_Phase = PH_DONE;
			return;
		}

		string fileKey = m_FileKeys[m_FileIdx];
		int fileKind = m_FileKinds[m_FileIdx];
		string errorKey = "";
		if (editor && editor.GetRegistry())
		{
			entry = editor.GetRegistry().Find(fileKey);
		}

		if (!entry)
		{
			errorKey = "#VSTR_XMLE_ERR_FILE_UNKNOWN";
		}
		else if (!FileExist(entry.Path))
		{
			errorKey = "#VSTR_XMLE_STATE_MISSING";
		}
		else if (!VPPXmlText.ReadAll(entry.Path, content))
		{
			errorKey = "#VSTR_XMLE_ERR_READ";
			if (editor && editor.GetRegistry())
			{
				editor.GetRegistry().NoteRevision(entry.Key, 0, false);
			}
		}

		if (errorKey != "")
		{
			VPPXESpawnablesChunk errorChunk = NewChunk(m_FileIdx, fileCount, fileKey, fileKind);
			errorChunk.ChunkCount = 1;
			errorChunk.ErrorKey = errorKey;
			SendChunk(errorChunk);
			m_FileIdx++;
			return;
		}

		m_DocRevision = VPPXmlText.NormalizedHash(content);
		if (editor && editor.GetRegistry())
		{
			editor.GetRegistry().NoteRevision(entry.Key, m_DocRevision, true);
		}

		m_Doc = new VPPXmlDocument();
		m_Doc.BeginString(content);
		m_Phase = PH_FILE_PARSE;
	}

	// Rows of the parsed file in chunks of about CHUNK_BYTES.
	protected void SendParsedFile()
	{
		XMLEditor editor = GetXMLEditor();
		string fileKey = m_FileKeys[m_FileIdx];
		int fileKind = m_FileKinds[m_FileIdx];
		int fileCount = m_FileKeys.Count();
		array<ref VPPXESpawnablesChunk> chunks = new array<ref VPPXESpawnablesChunk>();
		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXESpawnablesChunk current = NewChunk(m_FileIdx, fileCount, fileKey, fileKind);
		string errorKey = "";
		bool editable = false;
		int bytes = 0;
		int nodeCount = 0;
		current.Revision = m_DocRevision;
		chunks.Insert(current);
		VPPXmlNode root = VPPXESpwIO.RootOf(m_Doc, fileKind);
		if (!root)
		{
			errorKey = VPPXESpwIO.ParseError(m_Doc);
		}
		else
		{
			if (editor && !editor.IsReadOnlyMode() && !root.SelfClosing)
			{
				editable = true;
			}

			VPPXmlNode fileDamage = VPPXESpwIO.FileDamageNode(root);
			if (fileKind == VPPXEFileKind.SPAWNABLETYPES && fileDamage)
			{
				current.FileDmgMin = fileDamage.GetAttr("min", "");
				current.FileDmgMax = fileDamage.GetAttr("max", "");
			}

			VPPXESpwIO.RowNodes(root, fileKind, nodes);
			nodeCount = nodes.Count();
			if (nodeCount > VPPXESpwRules.MAX_ROWS)
			{
				nodeCount = VPPXESpwRules.MAX_ROWS;
				editable = false;
			}

			current.Editable = editable;
			for (int i = 0; i < nodeCount; i++)
			{
				VPPXESpwNode row = new VPPXESpwNode();
				VPPXESpwIO.RowOf(m_Doc, nodes[i], row);
				int size = VPPXESpwRules.EstimateNode(row);
				if (current.Rows.Count() > 0 && bytes + size > VPPXESpwRules.CHUNK_BYTES)
				{
					current = NewChunk(m_FileIdx, fileCount, fileKey, fileKind);
					current.Revision = m_DocRevision;
					current.Editable = editable;
					current.FileDmgMin = chunks[0].FileDmgMin;
					current.FileDmgMax = chunks[0].FileDmgMax;
					chunks.Insert(current);
					bytes = 0;
				}

				current.Rows.Insert(row);
				bytes += size;
			}
		}

		int chunkCount = chunks.Count();
		for (int c = 0; c < chunkCount; c++)
		{
			VPPXESpawnablesChunk chunk = chunks[c];
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
			chunk.ErrorKey = errorKey;
			SendChunk(chunk);
		}
	}

	protected VPPXESpawnablesChunk NewChunk(int fileIdx, int fileCount, string fileKey, int fileKind)
	{
		VPPXESpawnablesChunk chunk = new VPPXESpawnablesChunk();
		chunk.ReqId = m_ReqId;
		chunk.FileIdx = fileIdx;
		chunk.FileCount = fileCount;
		chunk.FileKey = fileKey;
		chunk.Kind = fileKind;
		chunk.Editable = false;
		return chunk;
	}

	protected void SendChunk(VPPXESpawnablesChunk chunk)
	{
		if (!m_IgnoredSent)
		{
			chunk.Ignored.Copy(m_Ignored);
			m_IgnoredSent = true;
		}

		ScriptRPC netRpc = VPPXENet.Begin("XE_OnSpawnablesChunk");
		Param1<ref VPPXESpawnablesChunk> netPayload = new Param1<ref VPPXESpawnablesChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnSpawnablesChunk", netRpc);
	}
};

// XE_SaveSpawnables: the assembled edits of one spawnabletypes or randompresets file (the file's registry kind decides
// which rows the indexes address).
class VPPXESpawnablesSaveJob : VPPXEPatchJob
{
	protected ref VPPXESpawnablesSave m_Save;
	protected int m_FileKind;

	void VPPXESpawnablesSaveJob(PlayerIdentity sender, VPPXESpawnablesSave save)
	{
		m_Sender = sender;
		m_Save = save;
		m_FileKind = -1;
		XMLEditor editor = GetXMLEditor();
		VPPXEFileEntry entry = null;
		if (editor && editor.GetRegistry())
		{
			entry = editor.GetRegistry().Find(save.FileKey);
		}

		if (entry && (entry.Kind == VPPXEFileKind.SPAWNABLETYPES || entry.Kind == VPPXEFileKind.RANDOMPRESETS))
		{
			m_FileKind = entry.Kind;
		}
	}

	override string GetLabel()
	{
		return "SpawnablesSave";
	}

	protected override int ReqId()
	{
		return m_Save.ReqId;
	}

	protected override string TargetKey()
	{
		return m_Save.FileKey;
	}

	protected override int BaseRevision()
	{
		return m_Save.BaseRevision;
	}

	protected override int TargetKind()
	{
		return m_FileKind;
	}

	protected override string RootName()
	{
		return VPPXESpwIO.RootNameOf(m_FileKind);
	}

	protected override string RootErrorKey()
	{
		return "#VSTR_XMLE_SPB_ERR_ROOT";
	}

	protected override int MaxRows()
	{
		return VPPXESpwRules.MAX_ROWS;
	}

	protected override void ElementNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		VPPXESpwIO.RowNodes(root, m_FileKind, outNodes);
	}

	protected override int EditCount()
	{
		return m_Save.Edits.Count();
	}

	protected override int EditOp(int editIdx)
	{
		return m_Save.Edits[editIdx].Op;
	}

	protected override int EditIndex(int editIdx)
	{
		return m_Save.Edits[editIdx].Index;
	}

	protected override bool HasEditRow(int editIdx)
	{
		VPPXESpwEdit edit = m_Save.Edits[editIdx];
		return edit != null && edit.Row != null;
	}

	protected override string CheckEdit(int editIdx)
	{
		bool presetsFile = m_FileKind == VPPXEFileKind.RANDOMPRESETS;
		return VPPXESpwRules.CheckRow(m_Save.Edits[editIdx].Row, presetsFile);
	}

	protected override void BuildEdit(int editIdx, VPPXmlDocument doc, VPPXmlNode orig, VPPXmlNode sample, array<string> lines, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		VPPXESpwIO.BuildBlock(m_Save.Edits[editIdx].Row, doc, orig, indent, childIndent, firstBare, outLines);
	}

	protected override string SummaryName()
	{
		return VPPXESpwIO.RootNameOf(m_FileKind);
	}

	protected override string SavedKey()
	{
		return "#VSTR_XMLE_SPB_SAVED";
	}
};
