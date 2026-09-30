// XML Editor file registry: discovery of every mission CE file, server-issued keys, revisions, flags,
// the background refresh job and the registry-level issues. Clients only ever send these keys.

class VPPXEFileEntry : Managed
{
	string Key;
	string Path;
	int Kind;
	int LoadOrder;
	int Flags;
	int Revision;
	int BootRevision;
	string CeFolder;
	string InterruptedBackupId;
};

class VPPXEFileRegistry : Managed
{
	const static int REFRESH_THROTTLE_MS = 30000;
	const static string ALLOWED_KEY_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_./ -";

	protected ref array<ref VPPXEFileEntry> m_Entries;
	protected ref map<string, int> m_IndexByKey;
	protected ref map<string, int> m_HashedAt;
	protected ref map<string, int> m_IssueCounts;
	protected ref map<string, int> m_BuildIssueCounts;
	protected ref map<string, int> m_BootRevs;
	protected ref array<ref VPPXEIssue> m_BuildIssues;
	protected ref array<ref VPPXEFileEntry> m_BuildList;
	protected ref map<string, bool> m_BuildKeys;
	protected ref map<int, int> m_KindCounts;
	protected bool m_Ready;
	protected bool m_BootDone;
	protected int m_Rev;
	protected string m_WorldName;
	protected int m_WorldSize;
	protected VPPXERegistryRefreshJob m_RefreshJob;

	void VPPXEFileRegistry()
	{
		m_Entries = new array<ref VPPXEFileEntry>();
		m_IndexByKey = new map<string, int>();
		m_HashedAt = new map<string, int>();
		m_IssueCounts = new map<string, int>();
		m_BuildIssueCounts = new map<string, int>();
		m_BootRevs = new map<string, int>();
		m_BuildIssues = new array<ref VPPXEIssue>();
	}

	bool IsReady()
	{
		return m_Ready;
	}

	int GetRev()
	{
		return m_Rev;
	}

	string GetWorldName()
	{
		return m_WorldName;
	}

	int GetWorldSize()
	{
		return m_WorldSize;
	}

	VPPXEFileEntry Find(string key)
	{
		if (!m_IndexByKey.Contains(key))
		{
			return null;
		}

		return At(m_IndexByKey.Get(key));
	}

	int IndexOf(string key)
	{
		if (!m_IndexByKey.Contains(key))
		{
			return -1;
		}

		return m_IndexByKey.Get(key);
	}

	VPPXEFileEntry At(int idx)
	{
		if (idx < 0 || idx >= m_Entries.Count())
		{
			return null;
		}

		return m_Entries.Get(idx);
	}

	int Count()
	{
		return m_Entries.Count();
	}

	void GetByKind(int kind, array<VPPXEFileEntry> outList)
	{
		if (!outList)
		{
			return;
		}

		foreach (VPPXEFileEntry entry : m_Entries)
		{
			if (entry && entry.Kind == kind)
			{
				outList.Insert(entry);
			}
		}
	}

	VPPXEFileEntry FirstOfKind(int kind)
	{
		foreach (VPPXEFileEntry entry : m_Entries)
		{
			if (entry && entry.Kind == kind)
			{
				return entry;
			}
		}

		return null;
	}

	// ---------------------------------------------------------------- build

	void Build()
	{
		int started = GetGame().GetTime();
		m_BuildList = new array<ref VPPXEFileEntry>();
		m_BuildKeys = new map<string, bool>();
		m_KindCounts = new map<int, int>();
		m_BuildIssues = new array<ref VPPXEIssue>();
		m_BuildIssueCounts = new map<string, int>();
		m_WorldName = GetGame().GetWorldName();
		m_WorldSize = GetGame().GetWorld().GetWorldSize();

		// Root defaults of the 7 <ce> kinds and the core file itself.
		AddFile("db/types.xml", VPPXEFileKind.TYPES, "", false, false, "", 0);
		AddFile("db/globals.xml", VPPXEFileKind.GLOBALS, "", false, false, "", 0);
		AddFile("db/economy.xml", VPPXEFileKind.ECONOMY, "", false, false, "", 0);
		AddFile("db/events.xml", VPPXEFileKind.EVENTS, "", false, false, "", 0);
		AddFile("db/messages.xml", VPPXEFileKind.MESSAGES, "", false, false, "", 0);
		AddFile("cfgspawnabletypes.xml", VPPXEFileKind.SPAWNABLETYPES, "", false, false, "", 0);
		AddFile("cfgrandompresets.xml", VPPXEFileKind.RANDOMPRESETS, "", false, false, "", 0);
		AddFile("cfgeconomycore.xml", VPPXEFileKind.ECONOMYCORE, "", false, false, "", 0);

		// Every <ce folder><file name type> in document order.
		CollectEconomyCore();

		// Root-only files.
		AddFile("cfglimitsdefinition.xml", VPPXEFileKind.LIMITS, "", false, false, "", 0);
		AddFile("cfglimitsdefinitionuser.xml", VPPXEFileKind.LIMITSUSER, "", false, false, "", 0);
		AddFile("cfgignorelist.xml", VPPXEFileKind.IGNORELIST, "", false, false, "", 0);
		AddFile("cfgeventspawns.xml", VPPXEFileKind.EVENTSPAWNS, "", false, false, "", 0);
		AddFile("cfgeventgroups.xml", VPPXEFileKind.EVENTGROUPS, "", false, false, "", 0);
		AddFile("cfgplayerspawnpoints.xml", VPPXEFileKind.PLAYERSPAWNS, "", false, false, "", 0);
		AddFile("cfgweather.xml", VPPXEFileKind.WEATHER, "", false, false, "", 0);
		AddFile("cfgenvironment.xml", VPPXEFileKind.ENVIRONMENT, "", false, false, "", 0);
		AddFile("mapgroupproto.xml", VPPXEFileKind.MAPPROTO, "", false, false, "", 0);
		AddFile("mapgrouppos.xml", VPPXEFileKind.MAPPOS, "", false, false, "", 0);
		AddFile("mapclusterproto.xml", VPPXEFileKind.CLUSTERPROTO, "", false, false, "", 0);
		AddFile("mapgroupcluster.xml", VPPXEFileKind.CLUSTERPOS, "", false, false, "", 0);
		for (int ci = 1; ci <= 99; ci++)
		{
			string clusterKey = "mapgroupcluster" + VPPXmlText.Pad2(ci) + ".xml";
			AddFile(clusterKey, VPPXEFileKind.CLUSTERPOS, "", false, false, "", 0);
		}

		// Territory files listed by cfgenvironment <territories><file path>.
		CollectTerritories();

		SortIntoSessionOrder();

		foreach (VPPXEFileEntry entry : m_Entries)
		{
			HashEntry(entry);
			if (!m_BootDone)
			{
				m_BootRevs.Set(entry.Key, entry.Revision);
			}

			if (m_BootRevs.Contains(entry.Key))
			{
				entry.BootRevision = m_BootRevs.Get(entry.Key);
			}
			else
			{
				entry.BootRevision = 0;
			}
		}

		m_BootDone = true;
		foreach (VPPXEIssue bi : m_BuildIssues)
		{
			if (VPPXEText.IssueSeverity(bi.Code) >= VPPXESeverity.WARNING)
			{
				int prevCount = m_BuildIssueCounts.Get(bi.FileKey);
				m_BuildIssueCounts.Set(bi.FileKey, prevCount + 1);
			}
		}

		m_IssueCounts = new map<string, int>();
		foreach (VPPXEFileEntry flagEntry : m_Entries)
		{
			RecomputeFlags(flagEntry);
		}

		m_Rev++;
		m_Ready = true;
		m_BuildList = null;
		m_BuildKeys = null;
		m_KindCounts = null;
		int took = GetGame().GetTime() - started;
		VPPXELog.Info(string.Format("Registry built: %1 files, %2 registry issues, world %3 (%4 m), %5 ms", m_Entries.Count(), m_BuildIssues.Count(), m_WorldName, m_WorldSize, took));
	}

	// Rebuild after cfgeconomycore or cfgenvironment changed; keeps boot revisions and INTERRUPTED state.
	void Rebuild()
	{
		map<string, string> interrupted = new map<string, string>();
		map<string, int> parseErrors = new map<string, int>();
		foreach (VPPXEFileEntry entry : m_Entries)
		{
			if ((entry.Flags & VPPXEFileFlag.INTERRUPTED) != 0)
			{
				interrupted.Set(entry.Key, entry.InterruptedBackupId);
			}

			if ((entry.Flags & VPPXEFileFlag.PARSE_ERROR) != 0)
			{
				parseErrors.Set(entry.Key, 1);
			}
		}

		Build();
		foreach (VPPXEFileEntry rebuilt : m_Entries)
		{
			if (interrupted.Contains(rebuilt.Key))
			{
				rebuilt.Flags = rebuilt.Flags | VPPXEFileFlag.INTERRUPTED;
				rebuilt.InterruptedBackupId = interrupted.Get(rebuilt.Key);
			}

			if (parseErrors.Contains(rebuilt.Key))
			{
				rebuilt.Flags = rebuilt.Flags | VPPXEFileFlag.PARSE_ERROR;
			}

			RecomputeFlags(rebuilt);
		}

		XMLEditor editor = GetXMLEditor();
		if (editor)
		{
			editor.OnRegistryRebuilt();
		}
	}

	protected void AddFile(string rawKey, int kind, string ceFolder, bool fromCe, bool mustRegister, string sourceKey, int sourceLine)
	{
		string key = NormalizeKey(rawKey);
		int check = CheckKey(key);
		if (check == 1)
		{
			AddBuildIssue(VPPXEIssueCode.OUTSIDE, sourceKey, sourceLine, rawKey);
			return;
		}

		if (check == 2)
		{
			AddBuildIssue(VPPXEIssueCode.UNSUPPORTED_NAME, sourceKey, sourceLine, rawKey);
			return;
		}

		if (m_BuildKeys.Contains(key))
		{
			return;
		}

		string path = "$mission:" + key;
		bool exists = FileExist(path);
		if (!exists && !mustRegister)
		{
			return;
		}

		VPPXEFileEntry entry = new VPPXEFileEntry();
		entry.Key = key;
		entry.Path = path;
		entry.Kind = kind;
		entry.LoadOrder = m_KindCounts.Get(kind);
		m_KindCounts.Set(kind, entry.LoadOrder + 1);
		entry.Flags = 0;
		if (exists)
		{
			entry.Flags = entry.Flags | VPPXEFileFlag.EXISTS;
		}

		if (fromCe)
		{
			entry.Flags = entry.Flags | VPPXEFileFlag.FROM_CE;
		}

		entry.CeFolder = ceFolder;
		entry.InterruptedBackupId = "";
		m_BuildList.Insert(entry);
		m_BuildKeys.Set(key, true);
	}

	protected void AddBuildIssue(int code, string fileKey, int line, string arg)
	{
		VPPXEIssue issue = new VPPXEIssue();
		issue.Code = code;
		issue.Severity = VPPXEText.IssueSeverity(code);
		issue.FileKey = fileKey;
		issue.Line = line;
		issue.Entry = "";
		issue.Arg = VPPXmlText.Clip(arg, VPPXEConst.MAX_CELL_CHARS);
		m_BuildIssues.Insert(issue);
	}

	protected void CollectEconomyCore()
	{
		string corePath = "$mission:cfgeconomycore.xml";
		if (!FileExist(corePath))
		{
			return;
		}

		VPPXmlDocument doc = new VPPXmlDocument();
		if (!doc.Load(corePath))
		{
			VPPXELog.Warn("cfgeconomycore.xml could not be read: no <ce> files are registered");
			return;
		}

		doc.ParseAll();
		if (doc.HasError())
		{
			VPPXELog.Warn("cfgeconomycore.xml does not parse (" + doc.GetError() + "): no <ce> files are registered");
			return;
		}

		VPPXmlNode root = ResolveRoot(doc, "economycore");
		if (!root)
		{
			return;
		}

		array<VPPXmlNode> ceNodes = new array<VPPXmlNode>();
		root.ChildrenNamed("ce", ceNodes);
		foreach (VPPXmlNode ceNode : ceNodes)
		{
			if (!ceNode)
			{
				continue;
			}

			string folder = ceNode.GetAttr("folder", "");
			array<VPPXmlNode> fileNodes = new array<VPPXmlNode>();
			ceNode.ChildrenNamed("file", fileNodes);
			foreach (VPPXmlNode fileNode : fileNodes)
			{
				if (!fileNode)
				{
					continue;
				}

				string fileName = fileNode.GetAttr("name", "");
				string fileType = fileNode.GetAttr("type", "");
				int line = fileNode.StartLine + 1;
				int kind = CeKindOf(fileType);
				if (kind < 0)
				{
					AddBuildIssue(VPPXEIssueCode.CE_TYPE, "cfgeconomycore.xml", line, fileType);
					continue;
				}

				string raw = fileName;
				if (folder != "")
				{
					raw = folder + "/" + fileName;
				}

				AddFile(raw, kind, folder, true, true, "cfgeconomycore.xml", line);
			}
		}
	}

	protected void CollectTerritories()
	{
		string envPath = "$mission:cfgenvironment.xml";
		if (!FileExist(envPath))
		{
			return;
		}

		VPPXmlDocument doc = new VPPXmlDocument();
		if (!doc.Load(envPath))
		{
			return;
		}

		doc.ParseAll();
		if (doc.HasError())
		{
			VPPXELog.Warn("cfgenvironment.xml does not parse (" + doc.GetError() + "): no territory files are registered");
			return;
		}

		VPPXmlNode root = ResolveRoot(doc, "env");
		if (!root)
		{
			return;
		}

		array<VPPXmlNode> terrNodes = new array<VPPXmlNode>();
		root.ChildrenNamed("territories", terrNodes);
		foreach (VPPXmlNode terrNode : terrNodes)
		{
			if (!terrNode)
			{
				continue;
			}

			array<VPPXmlNode> fileNodes = new array<VPPXmlNode>();
			terrNode.ChildrenNamed("file", fileNodes);
			foreach (VPPXmlNode fileNode : fileNodes)
			{
				if (!fileNode)
				{
					continue;
				}

				string pathAttr = fileNode.GetAttr("path", "");
				if (pathAttr == "")
				{
					continue;
				}

				AddFile(pathAttr, VPPXEFileKind.TERRITORY, "", false, false, "cfgenvironment.xml", fileNode.StartLine + 1);
			}
		}
	}

	// The named root element of a parsed document (also when GetRoot() is a container of it).
	static VPPXmlNode ResolveRoot(VPPXmlDocument doc, string rootName)
	{
		if (!doc)
		{
			return null;
		}

		VPPXmlNode root = doc.GetRoot();
		if (!root)
		{
			return null;
		}

		string lower = root.Name;
		lower.ToLower();
		if (lower == rootName)
		{
			return root;
		}

		VPPXmlNode inner = root.FirstChild(rootName);
		if (inner)
		{
			return inner;
		}

		return root;
	}

	static int CeKindOf(string ceType)
	{
		string lower = ceType;
		lower.ToLower();
		if (lower == "types")
		{
			return VPPXEFileKind.TYPES;
		}

		if (lower == "globals")
		{
			return VPPXEFileKind.GLOBALS;
		}

		if (lower == "economy")
		{
			return VPPXEFileKind.ECONOMY;
		}

		if (lower == "messages")
		{
			return VPPXEFileKind.MESSAGES;
		}

		if (lower == "events")
		{
			return VPPXEFileKind.EVENTS;
		}

		if (lower == "spawnabletypes")
		{
			return VPPXEFileKind.SPAWNABLETYPES;
		}

		if (lower == "randompresets")
		{
			return VPPXEFileKind.RANDOMPRESETS;
		}

		return -1;
	}

	// Backslashes to /, ./ segments dropped, original case kept.
	static string NormalizeKey(string raw)
	{
		string key = raw;
		key.Replace("\\", "/");
		key = key.Trim();
		int guard = 0;
		while (key.IndexOf("/./") >= 0 && guard < 64)
		{
			key.Replace("/./", "/");
			guard++;
		}

		while (key.IndexOf("./") == 0 && key.Length() > 2)
		{
			key = key.Substring(2, key.Length() - 2);
		}

		return key;
	}

	// 0 = ok, 1 = OUTSIDE (.., colon, leading /), 2 = UNSUPPORTED_NAME (char outside [A-Za-z0-9_./ -]).
	static int CheckKey(string key)
	{
		if (key == "" || key.IndexOf("..") >= 0 || key.IndexOf(":") >= 0 || key.IndexOf("/") == 0)
		{
			return 1;
		}

		int len = key.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = key.Get(i);
			if (ALLOWED_KEY_CHARS.IndexOf(ch) < 0)
			{
				return 2;
			}
		}

		return 0;
	}

	static bool IsTracked(int kind)
	{
		if (kind == VPPXEFileKind.MAPPROTO || kind == VPPXEFileKind.MAPPOS)
		{
			return false;
		}

		if (kind == VPPXEFileKind.CLUSTERPROTO || kind == VPPXEFileKind.CLUSTERPOS)
		{
			return false;
		}

		return true;
	}

	// Session order: TYPES, SPAWNABLETYPES, RANDOMPRESETS, EVENTS, GLOBALS, ECONOMY, MESSAGES (each by load
	// order), then the root-only kinds in enum order.
	static int SessionGroup(int kind)
	{
		if (kind == VPPXEFileKind.TYPES)
		{
			return 0;
		}

		if (kind == VPPXEFileKind.SPAWNABLETYPES)
		{
			return 1;
		}

		if (kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return 2;
		}

		if (kind == VPPXEFileKind.EVENTS)
		{
			return 3;
		}

		if (kind == VPPXEFileKind.GLOBALS)
		{
			return 4;
		}

		if (kind == VPPXEFileKind.ECONOMY)
		{
			return 5;
		}

		if (kind == VPPXEFileKind.MESSAGES)
		{
			return 6;
		}

		return 7 + kind;
	}

	protected void SortIntoSessionOrder()
	{
		array<string> keys = new array<string>();
		int count = m_BuildList.Count();
		for (int i = 0; i < count; i++)
		{
			VPPXEFileEntry entry = m_BuildList.Get(i);
			string sortKey = VPPXmlText.PadInt(SessionGroup(entry.Kind), 3) + VPPXmlText.PadInt(entry.LoadOrder, 4) + VPPXmlText.PadInt(i, 5);
			keys.Insert(sortKey);
		}

		keys.Sort();
		m_Entries = new array<ref VPPXEFileEntry>();
		m_IndexByKey = new map<string, int>();
		foreach (string k : keys)
		{
			int idx = k.Substring(k.Length() - 5, 5).ToInt();
			VPPXEFileEntry sorted = m_BuildList.Get(idx);
			m_IndexByKey.Set(sorted.Key, m_Entries.Count());
			m_Entries.Insert(sorted);
		}
	}

	// ---------------------------------------------------------------- revisions and flags

	// Synchronous re-hash of one entry. Returns true when the revision or any flag changed.
	protected bool HashEntry(VPPXEFileEntry entry)
	{
		int oldRev = entry.Revision;
		int oldFlags = entry.Flags;
		bool exists = FileExist(entry.Path);
		if (exists)
		{
			entry.Flags = entry.Flags | VPPXEFileFlag.EXISTS;
		}
		else
		{
			entry.Flags = entry.Flags & ~VPPXEFileFlag.EXISTS;
		}

		if (exists && IsTracked(entry.Kind))
		{
			string content;
			if (VPPXmlText.ReadAll(entry.Path, content))
			{
				entry.Revision = VPPXmlText.NormalizedHash(content);
				entry.Flags = entry.Flags & ~VPPXEFileFlag.READ_ERROR;
			}
			else
			{
				entry.Revision = 0;
				entry.Flags = entry.Flags | VPPXEFileFlag.READ_ERROR;
			}
		}
		else
		{
			entry.Revision = 0;
			entry.Flags = entry.Flags & ~VPPXEFileFlag.READ_ERROR;
		}

		m_HashedAt.Set(entry.Key, GetGame().GetTime());
		RecomputeFlags(entry);
		if (oldRev != entry.Revision || oldFlags != entry.Flags)
		{
			return true;
		}

		return false;
	}

	protected void RecomputeFlags(VPPXEFileEntry entry)
	{
		int f = entry.Flags;
		f = f & ~VPPXEFileFlag.EDITABLE;
		f = f & ~VPPXEFileFlag.PENDING_RESTART;
		f = f & ~VPPXEFileFlag.EXTERNAL_CHANGE;

		bool readOnlyMode = false;
		XMLEditor editor = GetXMLEditor();
		if (editor)
		{
			readOnlyMode = editor.IsReadOnlyMode();
		}

		bool exists = (f & VPPXEFileFlag.EXISTS) != 0;
		bool readError = (f & VPPXEFileFlag.READ_ERROR) != 0;
		bool parseError = (f & VPPXEFileFlag.PARSE_ERROR) != 0;
		if (entry.Kind == VPPXEFileKind.TYPES && exists && !readError && !parseError && !readOnlyMode)
		{
			f = f | VPPXEFileFlag.EDITABLE;
		}

		if (IsTracked(entry.Kind))
		{
			if (entry.Revision != entry.BootRevision)
			{
				f = f | VPPXEFileFlag.PENDING_RESTART;
			}

			int lastHash = 0;
			if (editor && editor.GetBackups())
			{
				lastHash = editor.GetBackups().GetLastResultHash(entry.Key);
			}

			if (lastHash != 0 && lastHash != entry.Revision)
			{
				f = f | VPPXEFileFlag.EXTERNAL_CHANGE;
			}
		}

		entry.Flags = f;
		RecomputeIssueCount(entry);
	}

	protected void RecomputeIssueCount(VPPXEFileEntry entry)
	{
		int count = m_BuildIssueCounts.Get(entry.Key);
		if ((entry.Flags & VPPXEFileFlag.FROM_CE) != 0 && (entry.Flags & VPPXEFileFlag.EXISTS) == 0)
		{
			count++;
		}

		if ((entry.Flags & VPPXEFileFlag.READ_ERROR) != 0)
		{
			count++;
		}

		if ((entry.Flags & VPPXEFileFlag.INTERRUPTED) != 0)
		{
			count++;
		}

		m_IssueCounts.Set(entry.Key, count);
	}

	// Synchronous re-hash of one file (after our own writes). A changed cfgeconomycore or cfgenvironment
	// rebuilds the file list.
	void Refresh(string key)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return;
		}

		bool changed = HashEntry(entry);
		if (changed && NeedsRebuild(entry.Kind))
		{
			Rebuild();
		}
	}

	static bool NeedsRebuild(int kind)
	{
		if (kind == VPPXEFileKind.ECONOMYCORE || kind == VPPXEFileKind.ENVIRONMENT)
		{
			return true;
		}

		return false;
	}

	// A job that just read the file reports what it saw. No push by itself.
	void NoteRevision(string key, int revision, bool readOk)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return;
		}

		if (readOk)
		{
			entry.Revision = revision;
			entry.Flags = entry.Flags | VPPXEFileFlag.EXISTS;
			entry.Flags = entry.Flags & ~VPPXEFileFlag.READ_ERROR;
		}
		else
		{
			entry.Revision = 0;
			if (FileExist(entry.Path))
			{
				entry.Flags = entry.Flags | VPPXEFileFlag.EXISTS;
				entry.Flags = entry.Flags | VPPXEFileFlag.READ_ERROR;
			}
			else
			{
				entry.Flags = entry.Flags & ~VPPXEFileFlag.EXISTS;
				entry.Flags = entry.Flags & ~VPPXEFileFlag.READ_ERROR;
			}
		}

		m_HashedAt.Set(key, GetGame().GetTime());
		RecomputeFlags(entry);
	}

	// Set by the types service after each parse of a TYPES file.
	void SetParseError(string key, bool hasError)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return;
		}

		if (hasError)
		{
			entry.Flags = entry.Flags | VPPXEFileFlag.PARSE_ERROR;
		}
		else
		{
			entry.Flags = entry.Flags & ~VPPXEFileFlag.PARSE_ERROR;
		}

		RecomputeFlags(entry);
	}

	// Recomputes EDITABLE for every entry (read-only mode switch).
	void RefreshAllFlags()
	{
		foreach (VPPXEFileEntry entry : m_Entries)
		{
			RecomputeFlags(entry);
		}
	}

	void MarkInterrupted(string key, string backupId)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return;
		}

		entry.Flags = entry.Flags | VPPXEFileFlag.INTERRUPTED;
		entry.InterruptedBackupId = backupId;
		RecomputeFlags(entry);
		VPPXELog.Warn("File " + key + " is marked INTERRUPTED (backup " + backupId + ")");
	}

	void ClearInterrupted(string key)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return;
		}

		if ((entry.Flags & VPPXEFileFlag.INTERRUPTED) == 0)
		{
			return;
		}

		entry.Flags = entry.Flags & ~VPPXEFileFlag.INTERRUPTED;
		entry.InterruptedBackupId = "";
		RecomputeFlags(entry);
		VPPXELog.Info("INTERRUPTED cleared for " + key + " after a verified write");
	}

	// ---------------------------------------------------------------- issues

	// Registry-level issues only (small: one per problem file).
	void CollectIssues(array<ref VPPXEIssue> outIssues)
	{
		if (!outIssues)
		{
			return;
		}

		foreach (VPPXEIssue bi : m_BuildIssues)
		{
			outIssues.Insert(NewIssue(bi.Code, bi.FileKey, bi.Line, bi.Arg));
		}

		foreach (VPPXEFileEntry entry : m_Entries)
		{
			int f = entry.Flags;
			if ((f & VPPXEFileFlag.FROM_CE) != 0 && (f & VPPXEFileFlag.EXISTS) == 0)
			{
				outIssues.Insert(NewIssue(VPPXEIssueCode.FILE_MISSING, entry.Key, 0, ""));
			}

			if ((f & VPPXEFileFlag.READ_ERROR) != 0)
			{
				outIssues.Insert(NewIssue(VPPXEIssueCode.READ_FAIL, entry.Key, 0, ""));
			}

			if ((f & VPPXEFileFlag.INTERRUPTED) != 0)
			{
				outIssues.Insert(NewIssue(VPPXEIssueCode.INTERRUPTED, entry.Key, 0, entry.InterruptedBackupId));
			}

			if ((f & VPPXEFileFlag.PENDING_RESTART) != 0)
			{
				outIssues.Insert(NewIssue(VPPXEIssueCode.PENDING, entry.Key, 0, ""));
			}

			if ((f & VPPXEFileFlag.EXTERNAL_CHANGE) != 0)
			{
				outIssues.Insert(NewIssue(VPPXEIssueCode.EXTERNAL, entry.Key, 0, ""));
			}
		}
	}

	protected VPPXEIssue NewIssue(int code, string fileKey, int line, string arg)
	{
		VPPXEIssue issue = new VPPXEIssue();
		issue.Code = code;
		issue.Severity = VPPXEText.IssueSeverity(code);
		issue.FileKey = fileKey;
		issue.Line = line;
		issue.Entry = "";
		issue.Arg = VPPXmlText.Clip(arg, VPPXEConst.MAX_CELL_CHARS);
		return issue;
	}

	int GetIssueCount(string key)
	{
		return m_IssueCounts.Get(key);
	}

	// ---------------------------------------------------------------- background refresh

	void EnqueueRefresh()
	{
		if (!m_Ready || m_RefreshJob)
		{
			return;
		}

		VPPXERegistryRefreshJob job = new VPPXERegistryRefreshJob(this);
		m_RefreshJob = job;
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.BACKGROUND);
	}

	// Keys whose 30 s throttle expired (snapshot taken when the refresh job starts).
	void GetRefreshKeys(array<string> outKeys)
	{
		int now = GetGame().GetTime();
		foreach (VPPXEFileEntry entry : m_Entries)
		{
			int last = m_HashedAt.Get(entry.Key);
			if (!m_HashedAt.Contains(entry.Key) || now - last >= REFRESH_THROTTLE_MS || now < last)
			{
				outKeys.Insert(entry.Key);
			}
		}
	}

	// One refresh step: 0 unchanged or gone, 1 a TYPES file changed, 2 another file changed, 3 the file
	// list must be rebuilt (cfgeconomycore or cfgenvironment changed).
	int RefreshOne(string key)
	{
		VPPXEFileEntry entry = Find(key);
		if (!entry)
		{
			return 0;
		}

		if (!HashEntry(entry))
		{
			return 0;
		}

		if (NeedsRebuild(entry.Kind))
		{
			return 3;
		}

		if (entry.Kind == VPPXEFileKind.TYPES)
		{
			return 1;
		}

		return 2;
	}
};

// BACKGROUND job: re-hashes one tracked file per iteration (only files whose 30 s throttle expired).
class VPPXERegistryRefreshJob : VPPXEJob
{
	protected VPPXEFileRegistry m_Owner;
	protected ref array<string> m_Keys;
	protected ref array<string> m_ChangedTypes;
	protected int m_Cursor;
	protected bool m_Started;
	protected bool m_NeedRebuild;
	protected bool m_OtherChanged;

	void VPPXERegistryRefreshJob(VPPXEFileRegistry owner)
	{
		m_Owner = owner;
		m_Keys = new array<string>();
		m_ChangedTypes = new array<string>();
	}

	override bool Step()
	{
		if (!m_Owner)
		{
			return true;
		}

		if (!m_Started)
		{
			m_Started = true;
			m_Owner.GetRefreshKeys(m_Keys);
		}

		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Cursor >= m_Keys.Count())
			{
				Finish();
				return true;
			}

			string key = m_Keys.Get(m_Cursor);
			m_Cursor++;
			int result = m_Owner.RefreshOne(key);
			if (result == 1)
			{
				m_ChangedTypes.Insert(key);
			}
			else if (result == 2)
			{
				m_OtherChanged = true;
			}
			else if (result == 3)
			{
				m_NeedRebuild = true;
			}
		}

		return false;
	}

	protected void Finish()
	{
		XMLEditor editor = GetXMLEditor();
		if (!editor)
		{
			return;
		}

		if (m_NeedRebuild)
		{
			VPPXELog.Info("cfgeconomycore.xml or cfgenvironment.xml changed: rebuilding the file list");
			m_Owner.Rebuild();
			return;
		}

		foreach (string typesKey : m_ChangedTypes)
		{
			editor.GetTypes().ReindexFile(typesKey);
		}

		if (m_OtherChanged && m_ChangedTypes.Count() == 0)
		{
			editor.PushSessions();
		}
	}

	override string GetLabel()
	{
		return "RegistryRefresh";
	}
};
