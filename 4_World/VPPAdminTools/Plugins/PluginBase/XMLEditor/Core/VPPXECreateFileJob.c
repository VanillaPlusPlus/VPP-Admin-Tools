// XML Editor CREATE tab, server side: creates a custom types or messages file (or adopts an existing file of that
// kind at that place) and registers it with a <file name type="..."/> line in the <ce folder> block of cfgeconomycore.xml,
// or in a new <ce> block when the folder has none. Runs in one Step, like a restore, so no other write can
// interleave; both files are backed up first and written through VPPXESafeWrite. The Central Economy reads the
// registration at the next server restart (the file shows RESTART PENDING until then).

class VPPXECreateFileJob : VPPXEJob
{
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_FolderRaw;
	protected string m_FileRaw;
	protected int m_Kind;
	protected bool m_Replied;

	void VPPXECreateFileJob(PlayerIdentity sender, int reqId, string folder, string fileName, int kind)
	{
		m_Sender = sender;
		m_ReqId = reqId;
		m_FolderRaw = folder;
		m_FileRaw = fileName;
		m_Kind = kind;
	}

	override string GetLabel()
	{
		return "CreateTypesFile";
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
		VPPXENet.Result(m_Sender, m_ReqId, ok, key, arg);
	}

	protected void Run()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry() || !editor.GetBackups())
		{
			Reply(false, "#VSTR_XMLE_ERR_NOT_READY", "");
			return;
		}

		// 1. The same rules as the client form.
		string folder = VPPXECreateRules.NormalizeFolder(m_FolderRaw);
		string fileName = VPPXECreateRules.NormalizeFileName(m_FileRaw);
		string ruleKey = VPPXECreateRules.CheckFolder(folder);
		if (ruleKey == "")
		{
			ruleKey = VPPXECreateRules.CheckFileName(fileName);
		}

		if (ruleKey != "")
		{
			Reply(false, ruleKey, "");
			return;
		}

		string key = VPPXECreateRules.MakeKey(folder, fileName);
		VPPXEFileRegistry registry = editor.GetRegistry();
		if (VPPXEFileRegistry.CheckKey(key) != 0)
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_FILE", "");
			return;
		}

		if (IsRegistered(registry, key))
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_EXISTS", key);
			return;
		}

		// 2. cfgeconomycore.xml must be readable and parse to an <economycore> root with a closing tag.
		VPPXEFileEntry core = registry.Find("cfgeconomycore.xml");
		string coreContent;
		if (!core || !VPPXmlText.ReadAll(core.Path, coreContent))
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		VPPXmlDocument coreDoc = new VPPXmlDocument();
		coreDoc.BeginString(coreContent);
		coreDoc.ParseAll();
		if (coreDoc.HasError())
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		VPPXmlNode coreRoot = VPPXEFileRegistry.ResolveRoot(coreDoc, "economycore");
		if (!coreRoot || !VPPXmlText.EqualsNoCase(coreRoot.Name, "economycore") || coreRoot.SelfClosing || coreRoot.CloseStartLine < 0)
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		string newCore = "";
		if (!BuildCorePatch(coreDoc, coreRoot, folder, fileName, m_Kind, newCore))
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		// 3. An existing file at that place is adopted as it is, but only when its root fits the kind.
		string path = "$mission:" + key;
		bool adopt = false;
		if (FileExist(path))
		{
			if (!IsKindDocument(path, VPPXECreateRules.CeTypeOf(m_Kind)))
			{
				Reply(false, "#VSTR_XMLE_CR_ERR_NOT_TYPES", key);
				return;
			}

			adopt = true;
		}

		VPPXEBackupManager backups = editor.GetBackups();
		string errKey = "";
		string errArg = "";
		string noticeKey = "";
		string fileBackup = "";
		if (!adopt)
		{
			// 4. The folder (every level; MakeDirectory on an existing one is harmless) and the empty file.
			EnsureFolder(folder);
			string eol = "\n";
			if (coreDoc.DominantIsCR())
			{
				eol = "\r\n";
			}

			string rootName = VPPXECreateRules.CeTypeOf(m_Kind);
			string fileContent = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>" + eol + "<" + rootName + ">" + eol + "</" + rootName + ">" + eol;
			VPPXEFileEntry newEntry = new VPPXEFileEntry();
			newEntry.Key = key;
			newEntry.Path = path;
			newEntry.Kind = m_Kind;
			newEntry.CeFolder = folder;
			newEntry.InterruptedBackupId = "";
			fileBackup = backups.CreateBackup(newEntry, "", VPPXEBackupReason.STRUCT, m_Sender, "create " + key, 0, null, "");
			if (fileBackup == "")
			{
				Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
				return;
			}

			if (!VPPXESafeWrite.Write(newEntry, fileContent, fileBackup, "", true, errKey, errArg, noticeKey))
			{
				backups.ApplyRetention();
				VPPXELog.Action(m_Sender, "Creating " + key + " failed: " + errKey + " " + errArg + " (the folder may not exist and could not be created)", false);
				Reply(false, errKey, errArg);
				return;
			}

			backups.SetResultHash(fileBackup, VPPXmlText.NormalizedHash(fileContent));
		}

		// 5. Register it: backup of cfgeconomycore.xml, then the verified write of the patched content.
		string coreBackup = backups.CreateBackup(core, coreContent, VPPXEBackupReason.STRUCT, m_Sender, "register " + key, 1, null, "");
		if (coreBackup == "")
		{
			backups.ApplyRetention();
			Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
			return;
		}

		errKey = "";
		errArg = "";
		noticeKey = "";
		if (!VPPXESafeWrite.Write(core, newCore, coreBackup, coreContent, true, errKey, errArg, noticeKey))
		{
			backups.ApplyRetention();
			VPPXELog.Action(m_Sender, "Registering " + key + " in cfgeconomycore.xml failed: " + errKey + " " + errArg, false);
			Reply(false, errKey, errArg);
			return;
		}

		backups.SetResultHash(coreBackup, VPPXmlText.NormalizedHash(newCore));
		backups.ApplyRetention();
		string done = "Created " + key;
		if (adopt)
		{
			done = "Adopted the existing " + key;
		}

		VPPXELog.Action(m_Sender, done + " and registered it in cfgeconomycore.xml (backup " + coreBackup + ")", true);

		// The cfgeconomycore.xml change rebuilds the file list, reindexes the types and pushes the sessions.
		editor.OnFileWritten("cfgeconomycore.xml", m_Sender);
		if (adopt)
		{
			Reply(true, "#VSTR_XMLE_CR_ADOPTED", key);
		}
		else
		{
			Reply(true, "#VSTR_XMLE_CR_DONE", key);
		}
	}

	protected bool IsRegistered(VPPXEFileRegistry registry, string key)
	{
		int count = registry.Count();
		for (int i = 0; i < count; i++)
		{
			VPPXEFileEntry entry = registry.At(i);
			if (entry && VPPXmlText.EqualsNoCase(entry.Key, key))
			{
				return true;
			}
		}

		return false;
	}

	protected bool IsKindDocument(string path, string rootName)
	{
		VPPXmlDocument doc = new VPPXmlDocument();
		if (!doc.Load(path))
		{
			return false;
		}

		doc.ParseAll();
		if (doc.HasError())
		{
			return false;
		}

		VPPXmlNode root = VPPXEFileRegistry.ResolveRoot(doc, rootName);
		if (!root)
		{
			return false;
		}

		return VPPXmlText.EqualsNoCase(root.Name, rootName);
	}

	protected void EnsureFolder(string folder)
	{
		array<string> segments = new array<string>();
		folder.Split("/", segments);
		string prefix = "";
		foreach (string segment : segments)
		{
			if (prefix == "")
			{
				prefix = segment;
			}
			else
			{
				prefix = prefix + "/" + segment;
			}

			string dirPath = "$mission:" + prefix;
			if (!FileExist(dirPath))
			{
				MakeDirectory(dirPath);
			}
		}
	}

	// ---------------------------------------------------------------- cfgeconomycore.xml patch (line model)

	// Adds the <file> line to the last <ce> block of the folder (compared like the registry: normalized, any
	// case), else a new <ce> block before </economycore>. Indentation follows the neighbours; new lines use the
	// document's dominant EOL; everything else is kept byte for byte.
	protected bool BuildCorePatch(VPPXmlDocument doc, VPPXmlNode root, string folder, string fileName, int kind, out string result)
	{
		array<string> lines = new array<string>();
		lines.Copy(doc.GetLines());
		array<bool> crFlags = new array<bool>();
		crFlags.Copy(doc.GetCRFlags());
		bool cr = doc.DominantIsCR();
		string fileTag = VPPXECreateRules.FileTag(fileName, kind);
		array<string> block = new array<string>();
		VPPXmlNode ceNode = FindCeBlock(root, folder);
		if (ceNode)
		{
			block.Insert(ChildIndentOf(lines, ceNode) + fileTag);
			if (!InsertBeforeClose(lines, crFlags, ceNode.CloseStartLine, ceNode.CloseStartCol, block, cr))
			{
				return false;
			}
		}
		else
		{
			string ceIndent = RootChildIndent(lines, root);
			string unit = IndentUnit(ceIndent);
			block.Insert(ceIndent + VPPXECreateRules.CeOpenTag(folder));
			block.Insert(ceIndent + unit + fileTag);
			block.Insert(ceIndent + "</ce>");
			if (!InsertBeforeClose(lines, crFlags, root.CloseStartLine, root.CloseStartCol, block, cr))
			{
				return false;
			}
		}

		result = VPPXmlText.JoinLines(lines, crFlags, doc.GetPrefix(), doc.EndsWithNewline());
		return true;
	}

	protected VPPXmlNode FindCeBlock(VPPXmlNode root, string folder)
	{
		array<VPPXmlNode> ceNodes = new array<VPPXmlNode>();
		root.ChildrenNamed("ce", ceNodes);
		VPPXmlNode found = null;
		foreach (VPPXmlNode ceNode : ceNodes)
		{
			if (!ceNode || ceNode.SelfClosing || ceNode.CloseStartLine < 0)
			{
				continue;
			}

			string ceFolder = VPPXECreateRules.NormalizeFolder(ceNode.GetAttr("folder", ""));
			if (VPPXmlText.EqualsNoCase(ceFolder, folder))
			{
				found = ceNode;
			}
		}

		return found;
	}

	// Inserts the block lines before a closing tag. A closing tag alone on its line gets the block as whole lines
	// above it; a closing tag that shares its line is moved to a line of its own after the block.
	protected bool InsertBeforeClose(array<string> lines, array<bool> crFlags, int line, int col, array<string> block, bool cr)
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
		int afterLen = current.Length() - col;
		string after = VPPXETypesWriter.Mid(current, col, afterLen);
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

	// Indentation of the <ce> block's children: the last element child that starts its line, else the block's own
	// indentation plus one unit.
	protected string ChildIndentOf(array<string> lines, VPPXmlNode ceNode)
	{
		string childIndent = "";
		bool haveChild = false;
		int kids = ceNode.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = ceNode.ChildAt(i);
			if (!kid || kid.Kind != VPPXmlNodeKind.ELEMENT)
			{
				continue;
			}

			if (StartsLine(lines, kid))
			{
				childIndent = VPPXmlText.LeadingWs(lines[kid.StartLine]);
				haveChild = true;
			}
		}

		if (haveChild)
		{
			return childIndent;
		}

		string ceIndent = "";
		if (StartsLine(lines, ceNode))
		{
			ceIndent = VPPXmlText.LeadingWs(lines[ceNode.StartLine]);
		}

		return ceIndent + IndentUnit(ceIndent);
	}

	// Indentation of the root's first element child that starts its line (a tab when there is none).
	protected string RootChildIndent(array<string> lines, VPPXmlNode root)
	{
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (!kid || kid.Kind != VPPXmlNodeKind.ELEMENT)
			{
				continue;
			}

			if (StartsLine(lines, kid))
			{
				string indent = VPPXmlText.LeadingWs(lines[kid.StartLine]);
				if (indent != "")
				{
					return indent;
				}
			}
		}

		return "\t";
	}

	// One indentation step: a tab when the sample indentation uses tabs (or is empty), else its own spaces.
	protected string IndentUnit(string sampleIndent)
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

	protected bool StartsLine(array<string> lines, VPPXmlNode node)
	{
		if (!node || node.StartLine < 0 || node.StartLine >= lines.Count())
		{
			return false;
		}

		string lead = VPPXmlText.LeadingWs(lines[node.StartLine]);
		return lead.Length() == node.StartCol;
	}
};

// XML Editor FILES tab: removes a <ce> file. Its <file> line leaves cfgeconomycore.xml (the whole <ce> block when
// nothing else is left in it), then the file itself is deleted (DeleteFile works on $mission). Both are backed up
// first: restore the cfgeconomycore.xml backup (registers it again), then the file's backup. Every local is declared
// at the top (see VPPXEMessagesIO.ParseFile).
class VPPXERemoveCeFileJob : VPPXEJob
{
	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected string m_FileKey;
	protected bool m_Replied;

	void VPPXERemoveCeFileJob(PlayerIdentity sender, int reqId, string fileKey)
	{
		m_Sender = sender;
		m_ReqId = reqId;
		m_FileKey = fileKey;
	}

	override string GetLabel()
	{
		return "RemoveCeFile";
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
		VPPXENet.Result(m_Sender, m_ReqId, ok, key, arg);
	}

	protected void Run()
	{
		XMLEditor editor = GetXMLEditor();
		VPPXEFileRegistry registry = null;
		VPPXEBackupManager backups = null;
		VPPXEFileEntry entry = null;
		VPPXEFileEntry core = null;
		VPPXmlDocument coreDoc = new VPPXmlDocument();
		VPPXmlNode coreRoot = null;
		string coreContent = "";
		string fileContent = "";
		string newCore = "";
		string removedKey = "";
		string removedPath = "";
		bool fileDeleted = true;
		string fileNote = "deleted";
		string fileBackup = "";
		string coreBackup = "";
		string errKey = "";
		string errArg = "";
		string noticeKey = "";
		int removed = 0;
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

		registry = editor.GetRegistry();
		backups = editor.GetBackups();
		entry = registry.Find(m_FileKey);
		if (!entry)
		{
			Reply(false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		removedKey = entry.Key;
		removedPath = entry.Path;
		if ((entry.Flags & VPPXEFileFlag.FROM_CE) == 0)
		{
			Reply(false, "#VSTR_XMLE_FILES_ERR_NOT_CE", removedKey);
			return;
		}

		core = registry.Find("cfgeconomycore.xml");
		if (!core || !VPPXmlText.ReadAll(core.Path, coreContent))
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		coreDoc.BeginString(coreContent);
		coreDoc.ParseAll();
		if (coreDoc.HasError())
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		coreRoot = VPPXEFileRegistry.ResolveRoot(coreDoc, "economycore");
		if (!coreRoot || !VPPXmlText.EqualsNoCase(coreRoot.Name, "economycore"))
		{
			Reply(false, "#VSTR_XMLE_CR_ERR_CORE", "");
			return;
		}

		removed = BuildRemovePatch(coreDoc, coreRoot, removedKey, newCore);
		if (removed <= 0)
		{
			Reply(false, "#VSTR_XMLE_FILES_ERR_NOT_CE", removedKey);
			return;
		}

		if (!StillParses(newCore))
		{
			VPPXELog.Warn("Unregistering " + removedKey + " produced a cfgeconomycore.xml that does not parse; nothing was written");
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "verify");
			return;
		}

		// the file itself first (it stays on disk, but the backup keeps its content next to the registration)
		if (FileExist(entry.Path) && VPPXmlText.ReadAll(entry.Path, fileContent))
		{
			fileBackup = backups.CreateBackup(entry, fileContent, VPPXEBackupReason.STRUCT, m_Sender, "unregister " + removedKey, 0, null, "");
			if (fileBackup == "")
			{
				Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
				return;
			}
		}

		coreBackup = backups.CreateBackup(core, coreContent, VPPXEBackupReason.STRUCT, m_Sender, "unregister " + removedKey, removed, null, "");
		if (coreBackup == "")
		{
			backups.ApplyRetention();
			Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
			return;
		}

		if (!VPPXESafeWrite.Write(core, newCore, coreBackup, coreContent, true, errKey, errArg, noticeKey))
		{
			backups.ApplyRetention();
			VPPXELog.Action(m_Sender, "Unregistering " + removedKey + " from cfgeconomycore.xml failed: " + errKey + " " + errArg, false);
			Reply(false, errKey, errArg);
			return;
		}

		backups.SetResultHash(coreBackup, VPPXmlText.NormalizedHash(newCore));
		backups.ApplyRetention();

		// the registration is gone: now the file itself
		if (FileExist(removedPath))
		{
			DeleteFile(removedPath);
			fileDeleted = !FileExist(removedPath);
		}

		if (!fileDeleted)
		{
			fileNote = "could NOT be deleted and stays in its folder";
		}

		VPPXELog.Action(m_Sender, "Removed " + removedKey + " from cfgeconomycore.xml (backup " + coreBackup + "); the file " + fileNote, true);
		editor.OnFileWritten("cfgeconomycore.xml", m_Sender);
		if (fileDeleted)
		{
			Reply(true, "#VSTR_XMLE_FILES_REMOVED", removedKey);
		}
		else
		{
			Reply(true, "#VSTR_XMLE_FILES_REMOVED_KEPT", removedKey);
		}
	}

	// Removes every <file> of the <ce> blocks that registers fileKey (the key is built like the registry does); a
	// <ce> left without other elements goes as a whole. Returns the number of <file> entries removed.
	protected int BuildRemovePatch(VPPXmlDocument doc, VPPXmlNode root, string fileKey, out string result)
	{
		array<string> lines = new array<string>();
		array<bool> crFlags = new array<bool>();
		array<VPPXmlNode> ceNodes = new array<VPPXmlNode>();
		array<VPPXmlNode> matches = new array<VPPXmlNode>();
		array<VPPXmlNode> doomed = new array<VPPXmlNode>();
		array<string> noLines = new array<string>();
		bool cr = doc.DominantIsCR();
		int removed = 0;
		int others = 0;
		int kids = 0;
		string folder = "";
		string raw = "";
		lines.Copy(doc.GetLines());
		crFlags.Copy(doc.GetCRFlags());
		root.ChildrenNamed("ce", ceNodes);
		foreach (VPPXmlNode ceNode : ceNodes)
		{
			if (!ceNode)
			{
				continue;
			}

			folder = ceNode.GetAttr("folder", "");
			matches.Clear();
			others = 0;
			kids = ceNode.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = ceNode.ChildAt(k);
				if (!kid || kid.Kind != VPPXmlNodeKind.ELEMENT)
				{
					continue;
				}

				raw = kid.GetAttr("name", "");
				if (folder != "")
				{
					raw = folder + "/" + raw;
				}

				if (VPPXmlText.EqualsNoCase(kid.Name, "file") && VPPXmlText.EqualsNoCase(VPPXEFileRegistry.NormalizeKey(raw), fileKey))
				{
					matches.Insert(kid);
				}
				else
				{
					others++;
				}
			}

			if (matches.Count() == 0)
			{
				continue;
			}

			removed = removed + matches.Count();
			if (others == 0)
			{
				doomed.Insert(ceNode);
				continue;
			}

			foreach (VPPXmlNode matched : matches)
			{
				doomed.Insert(matched);
			}
		}

		// document order, so removing from the last one keeps every earlier span valid
		for (int d = doomed.Count() - 1; d >= 0; d--)
		{
			VPPXmlNode node = doomed[d];
			bool applied = false;
			if (VPPXEMessagesIO.StartsLine(lines, node) && VPPXEMessagesIO.EndsLine(lines, node))
			{
				applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, 0, node.EndLine + 1, 0, noLines, cr);
			}
			else
			{
				applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, node.StartCol, node.EndLine, node.EndCol, noLines, cr);
			}

			if (!applied)
			{
				return 0;
			}
		}

		result = VPPXmlText.JoinLines(lines, crFlags, doc.GetPrefix(), doc.EndsWithNewline());
		return removed;
	}

	protected bool StillParses(string content)
	{
		VPPXmlDocument check = new VPPXmlDocument();
		VPPXmlNode checkRoot = null;
		check.BeginString(content);
		check.ParseAll();
		if (check.HasError())
		{
			return false;
		}

		checkRoot = VPPXEFileRegistry.ResolveRoot(check, "economycore");
		return checkRoot != null && VPPXmlText.EqualsNoCase(checkRoot.Name, "economycore");
	}
};
