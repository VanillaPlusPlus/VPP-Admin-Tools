// XML Editor verified writes: boot EOL probe, pending-copy pre-flight, CR-stripped read-back
// verification, rollback and a journal that survives crashes (Cache/journal.json).
// Read-back verification relies on VPPXmlText.SameNormalized, which is now exact for any size.

class VPPXEJournalEntry
{
	string FileKey;
	string BackupId;
	int ExpectedHash;
	int RollbackHash;
	bool HasRollback;
	bool WasMissing;
	string PendingPath;
};

class VPPXEJournal
{
	ref array<ref VPPXEJournalEntry> Entries;

	void VPPXEJournal()
	{
		Entries = new array<ref VPPXEJournalEntry>();
	}
};

class VPPXESafeWrite
{
	const static string CACHE_DIR = "$profile:VPPAdminTools/XMLEditor/Cache/";
	const static string JOURNAL_PATH = "$profile:VPPAdminTools/XMLEditor/Cache/journal.json";
	const static string PROBE_PATH = "$profile:VPPAdminTools/XMLEditor/Cache/eol_probe.txt";
	const static string SLUG_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-";

	protected static int s_EolMode = VPPXEEolMode.UNKNOWN;
	protected static bool s_Probed;
	protected static ref VPPXEJournal s_Journal;

	// ---------------------------------------------------------------- EOL probe

	static int ProbeEol()
	{
		string lf = "\n";
		string cr = "\r";
		string content = "A" + lf + "B" + cr + lf + "C";
		string crlfForm = "A" + cr + lf + "B" + cr + cr + lf + "C";
		string lfForm = "A" + lf + "B" + lf + "C";
		int mode = VPPXEEolMode.UNKNOWN;
		int bytes = -1;
		int len = -1;
		if (VPPXmlText.WriteAll(PROBE_PATH, content))
		{
			FileHandle fh = OpenFile(PROBE_PATH, FileMode.READ);
			if (fh != 0)
			{
				string readBack;
				bytes = ReadFile(fh, readBack, VPPXEConst.READ_CAP);
				CloseFile(fh);
				len = readBack.Length();
				if (bytes == len)
				{
					if (readBack == content)
					{
						mode = VPPXEEolMode.BINARY;
					}
					else if (readBack == crlfForm)
					{
						mode = VPPXEEolMode.WRITE_CRLF;
					}
					else if (readBack == lfForm)
					{
						mode = VPPXEEolMode.READ_STRIP;
					}
				}
			}
		}

		s_EolMode = mode;
		s_Probed = true;
		VPPXELog.Info(string.Format("EOL probe: mode %1 (%2), ReadFile bytes %3, string length %4", mode, EolModeName(mode), bytes, len));
		if (mode == VPPXEEolMode.UNKNOWN)
		{
			VPPXELog.Warn("EOL probe could not classify this host's file IO: files are written as read and every write is verified");
		}

		return mode;
	}

	static int GetEolMode()
	{
		return s_EolMode;
	}

	static string EolModeName(int mode)
	{
		if (mode == VPPXEEolMode.BINARY)
		{
			return "BINARY";
		}

		if (mode == VPPXEEolMode.WRITE_CRLF)
		{
			return "WRITE_CRLF";
		}

		if (mode == VPPXEEolMode.READ_STRIP)
		{
			return "READ_STRIP";
		}

		return "UNKNOWN";
	}

	// Bytes to hand to FPrint for the probed mode. WRITE_CRLF hosts turn every LF into CR LF, so CR LF pairs are
	// folded to LF first by VPPXmlText.CRLFToLF: the native string.Replace keeps only about 5 KB of its result.
	protected static string PrepareBytes(string content, int mode)
	{
		if (mode == VPPXEEolMode.WRITE_CRLF)
		{
			return VPPXmlText.CRLFToLF(content);
		}

		return content;
	}

	// True when the content holds an LF that is not preceded by CR (VPPXmlText.HasBareLF, exact for any size).
	protected static bool HasBareLF(string content)
	{
		return VPPXmlText.HasBareLF(content);
	}

	// ---------------------------------------------------------------- slugs and paths

	static string Slug(string key)
	{
		string slug = "";
		int len = key.Length();
		for (int i = 0; i < len; i++)
		{
			string ch = key.Get(i);
			if (SLUG_CHARS.IndexOf(ch) < 0)
			{
				slug += "_";
			}
			else
			{
				slug += ch;
			}
		}

		int h = key.Hash();
		slug += "_" + Math.AbsInt(h).ToString();
		return slug;
	}

	static string ExtOf(string key)
	{
		string lower = key;
		lower.ToLower();
		int dot = lower.LastIndexOf(".");
		int slash = lower.LastIndexOf("/");
		if (dot < 0 || dot < slash || lower.Length() - dot > 8)
		{
			return ".txt";
		}

		return lower.Substring(dot, lower.Length() - dot);
	}

	static string PendingPathOf(string key)
	{
		return CACHE_DIR + "pending_" + Slug(key) + ExtOf(key);
	}

	// ---------------------------------------------------------------- journal

	protected static VPPXEJournal Journal()
	{
		if (s_Journal)
		{
			return s_Journal;
		}

		VPPXEJournal loaded = new VPPXEJournal();
		if (FileExist(JOURNAL_PATH))
		{
			string err;
			if (!JsonFileLoader<VPPXEJournal>.LoadFile(JOURNAL_PATH, loaded, err))
			{
				VPPXELog.Warn("journal.json could not be loaded (" + err + "): starting an empty journal");
				loaded = new VPPXEJournal();
			}
		}

		if (!loaded)
		{
			loaded = new VPPXEJournal();
		}

		if (!loaded.Entries)
		{
			loaded.Entries = new array<ref VPPXEJournalEntry>();
		}

		s_Journal = loaded;
		return s_Journal;
	}

	// False when journal.json could not be saved (already logged). Write refuses to touch $mission without a saved entry.
	protected static bool SaveJournal()
	{
		string err;
		if (!JsonFileLoader<VPPXEJournal>.SaveFile(JOURNAL_PATH, Journal(), err))
		{
			VPPXELog.Warn("journal.json could not be saved: " + err);
			return false;
		}

		return true;
	}

	protected static void RemoveEntry(VPPXEJournalEntry entry)
	{
		VPPXEJournal journal = Journal();
		int idx = journal.Entries.Find(entry);
		if (idx >= 0)
		{
			journal.Entries.Remove(idx);
		}
	}

	protected static void RemoveEntriesOfKey(string fileKey)
	{
		VPPXEJournal journal = Journal();
		array<ref VPPXEJournalEntry> kept = new array<ref VPPXEJournalEntry>();
		foreach (VPPXEJournalEntry entry : journal.Entries)
		{
			if (entry && entry.FileKey != fileKey)
			{
				kept.Insert(entry);
			}
		}

		journal.Entries = kept;
	}

	static bool IsJournaled(string backupId)
	{
		if (backupId == "")
		{
			return false;
		}

		VPPXEJournal journal = Journal();
		foreach (VPPXEJournalEntry entry : journal.Entries)
		{
			if (entry && entry.BackupId == backupId)
			{
				return true;
			}
		}

		return false;
	}

	// ---------------------------------------------------------------- verified write

	// hasRollback false = the current file exists but could not be read: no rollback is ever written.
	// A file that does not exist before the write is deleted again when the write fails (DeleteFile works on $mission,
	// verified in game 2026-09-24); if that delete does not take, it is rolled back to empty content.
	static bool Write(VPPXEFileEntry file, string newContent, string backupId, string rollbackContent, bool hasRollback, out string errKey, out string errArg, out string noticeKey)
	{
		errKey = "";
		errArg = "";
		noticeKey = "";
		if (!file)
		{
			errKey = "#VSTR_XMLE_ERR_FILE_UNKNOWN";
			return false;
		}

		// 1. Prepare the bytes for the probed EOL mode.
		bool wasMissing = !FileExist(file.Path);
		if (wasMissing)
		{
			rollbackContent = "";
			hasRollback = true;
		}

		int mode = GetEolMode();
		string bytes = PrepareBytes(newContent, mode);
		string rollbackBytes = "";
		if (hasRollback)
		{
			rollbackBytes = PrepareBytes(rollbackContent, mode);
			if (mode == VPPXEEolMode.WRITE_CRLF && HasBareLF(rollbackContent))
			{
				noticeKey = "#VSTR_XMLE_NOTE_EOL_CRLF";
			}
		}

		// 2. Journal entry first, saved before anything is written.
		VPPXEJournalEntry entry = new VPPXEJournalEntry();
		entry.FileKey = file.Key;
		entry.BackupId = backupId;
		entry.ExpectedHash = VPPXmlText.NormalizedHash(newContent);
		entry.RollbackHash = 0;
		if (hasRollback)
		{
			entry.RollbackHash = VPPXmlText.NormalizedHash(rollbackContent);
		}

		entry.HasRollback = hasRollback;
		entry.WasMissing = wasMissing;
		entry.PendingPath = PendingPathOf(file.Key);
		Journal().Entries.Insert(entry);
		if (!SaveJournal())
		{
			// Without a saved journal entry a crash mid-write could not be detected: refuse before anything is written.
			RemoveEntry(entry);
			errKey = "#VSTR_XMLE_ERR_WRITE";
			VPPXELog.Warn("The journal entry for " + file.Key + " could not be saved: nothing was written");
			return false;
		}

		// 3. Pre-flight through the pending copy in $profile; $mission is untouched on failure.
		bool preflightOk = VPPXmlText.WriteAll(entry.PendingPath, bytes);
		if (preflightOk)
		{
			string pendingRead;
			preflightOk = VPPXmlText.ReadAll(entry.PendingPath, pendingRead);
			if (preflightOk && !VPPXmlText.SameNormalized(pendingRead, bytes))
			{
				preflightOk = false;
			}

			if (preflightOk && mode == VPPXEEolMode.BINARY && pendingRead != bytes)
			{
				preflightOk = false;
			}
		}

		if (!preflightOk)
		{
			RemoveEntry(entry);
			SaveJournal();
			errKey = "#VSTR_XMLE_ERR_WRITE";
			VPPXELog.Warn("Pre-flight write of the pending copy failed for " + file.Key + ": nothing was written");
			return false;
		}

		// 4. The real write.
		if (!VPPXmlText.WriteAll(file.Path, bytes))
		{
			bool unchanged = false;
			if (wasMissing)
			{
				if (!FileExist(file.Path))
				{
					unchanged = true;
				}
			}
			else
			{
				string targetRead;
				bool targetOk = VPPXmlText.ReadAll(file.Path, targetRead);
				if (targetOk && hasRollback && VPPXmlText.SameNormalized(targetRead, rollbackContent))
				{
					unchanged = true;
				}
			}

			if (unchanged)
			{
				RemoveEntry(entry);
				SaveJournal();
				DeleteFile(entry.PendingPath);
				errKey = "#VSTR_XMLE_ERR_WRITE";
				VPPXELog.Warn("Write of " + file.Key + " failed; the file is unchanged");
				return false;
			}

			return RollbackPath(file, entry, rollbackContent, rollbackBytes, hasRollback, backupId, errKey, errArg);
		}

		// 5. Read back and verify the CR-stripped strings (and the raw bytes in BINARY mode).
		string readBack;
		bool readOk = VPPXmlText.ReadAll(file.Path, readBack);
		bool verified = false;
		if (readOk && VPPXmlText.SameNormalized(readBack, newContent))
		{
			verified = true;
			if (mode == VPPXEEolMode.BINARY && readBack != bytes)
			{
				verified = false;
			}
		}

		if (verified)
		{
			RemoveEntriesOfKey(file.Key);
			SaveJournal();
			DeleteFile(entry.PendingPath);
			XMLEditor editor = GetXMLEditor();
			if (editor && editor.GetRegistry())
			{
				editor.GetRegistry().ClearInterrupted(file.Key);
				editor.GetRegistry().NoteRevision(file.Key, VPPXmlText.NormalizedHash(newContent), true);
			}

			return true;
		}

		// 6. Rollback path.
		return RollbackPath(file, entry, rollbackContent, rollbackBytes, hasRollback, backupId, errKey, errArg);
	}

	// entry.WasMissing = the file did not exist before the write: it is deleted again (else emptied, rollbackContent "").
	protected static bool RollbackPath(VPPXEFileEntry file, VPPXEJournalEntry entry, string rollbackContent, string rollbackBytes, bool hasRollback, string backupId, out string errKey, out string errArg)
	{
		if (entry.WasMissing)
		{
			DeleteFile(file.Path);
			if (!FileExist(file.Path))
			{
				RemoveEntry(entry);
				SaveJournal();
				DeleteFile(entry.PendingPath);
				errKey = "#VSTR_XMLE_ERR_VERIFY";
				errArg = "";
				VPPXELog.Warn("Write of " + file.Key + " did not verify; the file did not exist before and was deleted again");
				XMLEditor delEditor = GetXMLEditor();
				if (delEditor && delEditor.GetRegistry())
				{
					delEditor.GetRegistry().NoteRevision(file.Key, 0, false);
				}

				return false;
			}
		}

		if (hasRollback)
		{
			bool rollbackWritten = VPPXmlText.WriteAll(file.Path, rollbackBytes);
			string rollbackRead;
			bool rollbackReadOk = VPPXmlText.ReadAll(file.Path, rollbackRead);
			if (rollbackWritten && rollbackReadOk && VPPXmlText.SameNormalized(rollbackRead, rollbackContent))
			{
				RemoveEntry(entry);
				SaveJournal();
				DeleteFile(entry.PendingPath);
				errKey = "#VSTR_XMLE_ERR_VERIFY";
				errArg = "";
				string rbMsg = "Write of " + file.Key + " did not verify and was rolled back";
				if (entry.WasMissing)
				{
					rbMsg = rbMsg + " (the file did not exist before, could not be deleted and was left empty)";
				}

				VPPXELog.Warn(rbMsg);
				XMLEditor rbEditor = GetXMLEditor();
				if (rbEditor && rbEditor.GetRegistry())
				{
					rbEditor.GetRegistry().NoteRevision(file.Key, VPPXmlText.NormalizedHash(rollbackContent), true);
				}

				return false;
			}
		}

		// The file is in an unknown state: keep the journal entry, mark INTERRUPTED and pin the backup.
		SaveJournal();
		XMLEditor editor = GetXMLEditor();
		if (editor)
		{
			if (editor.GetRegistry())
			{
				editor.GetRegistry().MarkInterrupted(file.Key, backupId);
			}

			if (editor.GetBackups())
			{
				editor.GetBackups().AutoPin(backupId);
			}
		}

		errKey = "#VSTR_XMLE_ERR_ROLLBACK";
		errArg = backupId;
		VPPXELog.Warn("Write of " + file.Key + " did not verify and could not be rolled back: restore backup " + backupId + " (journal entry kept)");
		return false;
	}

	// ---------------------------------------------------------------- journal checks

	// Boot: clears entries whose file equals the pending copy or the backup; the rest mark INTERRUPTED.
	static void CheckJournal(VPPXEFileRegistry registry)
	{
		VPPXEJournal journal = Journal();
		if (journal.Entries.Count() == 0)
		{
			return;
		}

		array<ref VPPXEJournalEntry> kept = new array<ref VPPXEJournalEntry>();
		int cleared = 0;
		foreach (VPPXEJournalEntry entry : journal.Entries)
		{
			if (!entry)
			{
				continue;
			}

			if (CheckEntry(registry, entry))
			{
				cleared++;
				DeleteFile(entry.PendingPath);
			}
			else
			{
				kept.Insert(entry);
			}
		}

		journal.Entries = kept;
		SaveJournal();
		VPPXELog.Info(string.Format("Journal check: %1 entries cleared, %2 kept (INTERRUPTED)", cleared, kept.Count()));
	}

	// Same check for one key, right away (OnAborted of save and restore jobs).
	static void CheckJournalFor(string fileKey)
	{
		if (fileKey == "")
		{
			return;
		}

		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			return;
		}

		VPPXEJournal journal = Journal();
		array<ref VPPXEJournalEntry> kept = new array<ref VPPXEJournalEntry>();
		bool changed = false;
		foreach (VPPXEJournalEntry entry : journal.Entries)
		{
			if (!entry)
			{
				changed = true;
				continue;
			}

			if (entry.FileKey != fileKey)
			{
				kept.Insert(entry);
				continue;
			}

			if (CheckEntry(editor.GetRegistry(), entry))
			{
				changed = true;
				DeleteFile(entry.PendingPath);
			}
			else
			{
				kept.Insert(entry);
			}
		}

		if (changed)
		{
			journal.Entries = kept;
		}

		SaveJournal();
	}

	// True = the entry can be cleared. False = INTERRUPTED was set and the backup pinned.
	protected static bool CheckEntry(VPPXEFileRegistry registry, VPPXEJournalEntry entry)
	{
		VPPXEFileEntry file = null;
		if (registry)
		{
			file = registry.Find(entry.FileKey);
		}

		if (!file)
		{
			// The file may be half-written and the entry is about to be dropped: pin the backup so retention keeps it.
			XMLEditor dropEditor = GetXMLEditor();
			if (dropEditor && dropEditor.GetBackups())
			{
				dropEditor.GetBackups().AutoPin(entry.BackupId);
			}

			VPPXELog.Warn("Journal entry for unregistered file " + entry.FileKey + " dropped; backup " + entry.BackupId + " was pinned for manual review");
			return true;
		}

		string current;
		bool cleared = false;
		if (entry.WasMissing && !FileExist(file.Path))
		{
			cleared = true;
		}
		else if (VPPXmlText.ReadAll(file.Path, current))
		{
			string pending;
			if (VPPXmlText.ReadAll(entry.PendingPath, pending))
			{
				if (VPPXmlText.SameNormalized(current, pending))
				{
					cleared = true;
				}
			}
			else if (VPPXmlText.NormalizedHash(current) == entry.ExpectedHash)
			{
				cleared = true;
			}

			if (!cleared && entry.WasMissing && VPPXmlText.SameNormalized(current, ""))
			{
				cleared = true;
			}

			if (!cleared && entry.HasRollback)
			{
				string backupContent;
				XMLEditor editor = GetXMLEditor();
				bool backupRead = false;
				if (editor && editor.GetBackups())
				{
					backupRead = editor.GetBackups().ReadBackup(entry.BackupId, backupContent);
				}

				if (backupRead)
				{
					if (VPPXmlText.SameNormalized(current, backupContent))
					{
						cleared = true;
					}
				}
				else if (VPPXmlText.NormalizedHash(current) == entry.RollbackHash)
				{
					cleared = true;
				}
			}
		}

		if (cleared)
		{
			return true;
		}

		registry.MarkInterrupted(entry.FileKey, entry.BackupId);
		XMLEditor pinEditor = GetXMLEditor();
		if (pinEditor && pinEditor.GetBackups())
		{
			pinEditor.GetBackups().AutoPin(entry.BackupId);
		}

		return false;
	}
};
