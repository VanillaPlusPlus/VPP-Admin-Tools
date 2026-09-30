// XML Editor backup manager (server only; INTERFACES v2.1 section 5, spec section 8).
// Disk: $profile:VPPAdminTools/XMLEditor/Backups/index.json (slug -> file key), <slug>/manifest.json, <slug>/<Id><ext>.
// The client-facing BackupId "slug/Id" is only a lookup key: every path is built from the index slug plus the
// manifest record's own Id and the extension of its file key, never from a client string.
// Every mutation runs inside an INTERACTIVE VPPXEJob (VPPXEBackupJobs.c); the Handle* methods only resolve and enqueue.

class VPPXEBackupRecord
{
	string Id;
	string FileKey;
	string Stamp;
	string AdminName;
	string AdminId;
	int Reason;
	string Summary;
	string Note;
	int ChangeCount;
	int Size;
	int Hash;
	int ResultHash;
	bool Pinned;
	bool Unverified;
	ref array<string> Touched;

	void VPPXEBackupRecord()
	{
		Id = "";
		FileKey = "";
		Stamp = "";
		AdminName = "";
		AdminId = "";
		Summary = "";
		Note = "";
		Touched = new array<string>();
	}
};

class VPPXEBackupManifest
{
	string FileKey;
	ref array<ref VPPXEBackupRecord> Records;

	void VPPXEBackupManifest()
	{
		FileKey = "";
		Records = new array<ref VPPXEBackupRecord>();
	}
};

class VPPXEBackupIndex
{
	ref array<string> Slugs;
	ref array<string> FileKeys;

	void VPPXEBackupIndex()
	{
		Slugs = new array<string>();
		FileKeys = new array<string>();
	}
};

class VPPXEBackupManager : Managed
{
	const static string BASE_DIR = "$profile:VPPAdminTools/XMLEditor/Backups/";
	const static string SAFE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-";
	const static string DIGIT_CHARS = "0123456789";
	const static int ID_LENGTH = 19;
	const static int MAX_SUMMARY = 200;
	const static int MAX_TOUCHED = 50;
	const static int MAX_ADMIN_TEXT = 64;
	const static int MAX_BACKUP_ID_LENGTH = 512;
	const static int MAX_ID_COUNTER = 1000;
	const static int KB_SATURATION = 2000000000;
	const static int MAX_TOTAL_MB = 10000;

	protected XMLEditor m_Owner;
	protected ref VPPXEBackupIndex m_Index;
	protected ref map<string, int> m_SlugIdx;
	protected ref map<string, ref VPPXEBackupManifest> m_Manifests;
	protected ref map<string, int> m_Leases;
	protected bool m_DirsReady;

	void VPPXEBackupManager(XMLEditor owner)
	{
		m_Owner = owner;
		m_SlugIdx = new map<string, int>();
		m_Manifests = new map<string, ref VPPXEBackupManifest>();
		m_Leases = new map<string, int>();
	}

	// =====================================================================
	// Names, ids, time
	// =====================================================================

	// fileKey with every char outside [A-Za-z0-9_.-] replaced by '_', then '_' + abs(hash) (keys are short).
	static string MakeSlug(string fileKey)
	{
		string allowed = SAFE_CHARS;
		string built = "";
		int keyLength = fileKey.Length();
		for (int i = 0; i < keyLength; i++)
		{
			string ch = fileKey.Get(i);
			if (allowed.IndexOf(ch) >= 0)
			{
				built += ch;
			}
			else
			{
				built += "_";
			}
		}

		int keyHash = Math.AbsInt(fileKey.Hash());
		built += "_" + keyHash.ToString();
		return built;
	}

	// Every registered file is XML (the mission JSON files are not handled by the editor).
	static string ExtOf(string fileKey)
	{
		return ".xml";
	}

	// 1..MAX_BACKUP_ID_LENGTH chars of [A-Za-z0-9_.-].
	static bool IsSafeToken(string s)
	{
		int tokenLength = s.Length();
		if (tokenLength == 0 || tokenLength > MAX_BACKUP_ID_LENGTH)
		{
			return false;
		}

		string allowed = SAFE_CHARS;
		for (int i = 0; i < tokenLength; i++)
		{
			string ch = s.Get(i);
			if (allowed.IndexOf(ch) < 0)
			{
				return false;
			}
		}

		return true;
	}

	// A slug is one path component: safe chars only and never "." or "..".
	static bool IsValidSlug(string slug)
	{
		if (!IsSafeToken(slug))
		{
			return false;
		}

		if (slug == "." || slug == "..")
		{
			return false;
		}

		return true;
	}

	// YYYYMMDD-HHMMSS-NNN (digits and the two dashes only).
	static bool IsValidId(string recId)
	{
		if (recId.Length() != ID_LENGTH)
		{
			return false;
		}

		string digits = DIGIT_CHARS;
		for (int i = 0; i < ID_LENGTH; i++)
		{
			string ch = recId.Get(i);
			if (i == 8 || i == 15)
			{
				if (ch != "-")
				{
					return false;
				}
			}
			else
			{
				if (digits.IndexOf(ch) < 0)
				{
					return false;
				}
			}
		}

		return true;
	}

	static string MakeBackupId(string slug, string recId)
	{
		return slug + "/" + recId;
	}

	// Integer value of a fixed-width part of an Id (0 when out of range).
	static int IdPart(string recId, int start, int width)
	{
		if (recId.Length() < start + width)
		{
			return 0;
		}

		string part = recId.Substring(start, width);
		return part.ToInt();
	}

	// Integer value of the fixed-width field that starts fromEnd chars before the end of a sort key.
	static int KeyTail(string key, int fromEnd, int width)
	{
		int keyLength = key.Length();
		if (keyLength < fromEnd)
		{
			return -1;
		}

		string part = key.Substring(keyLength - fromEnd, width);
		return part.ToInt();
	}

	// -1 when a is older than b, 1 when newer, 0 when equal (Ids sort by time).
	static int CompareIds(string a, string b)
	{
		int dateA = IdPart(a, 0, 8);
		int dateB = IdPart(b, 0, 8);
		if (dateA != dateB)
		{
			if (dateA < dateB)
			{
				return -1;
			}

			return 1;
		}

		int timeA = IdPart(a, 9, 6);
		int timeB = IdPart(b, 9, 6);
		if (timeA != timeB)
		{
			if (timeA < timeB)
			{
				return -1;
			}

			return 1;
		}

		int seqA = IdPart(a, 16, 3);
		int seqB = IdPart(b, 16, 3);
		if (seqA != seqB)
		{
			if (seqA < seqB)
			{
				return -1;
			}

			return 1;
		}

		return 0;
	}

	// "YYYYMMDD-HHMMSS-NNN" -> "YYYY-MM-DD HH:MM:SS".
	static string StampFromId(string recId)
	{
		if (recId.Length() != ID_LENGTH)
		{
			return "";
		}

		string yearText = recId.Substring(0, 4);
		string monthText = recId.Substring(4, 2);
		string dayText = recId.Substring(6, 2);
		string hourText = recId.Substring(9, 2);
		string minuteText = recId.Substring(11, 2);
		string secondText = recId.Substring(13, 2);
		return yearText + "-" + monthText + "-" + dayText + " " + hourText + ":" + minuteText + ":" + secondText;
	}

	// Current UTC time as the Id base "YYYYMMDD-HHMMSS" and the stamp "YYYY-MM-DD HH:MM:SS".
	// Date and time are separate engine reads: read date, time, date again; if the date rolled over (midnight)
	// use the new date and re-read the time, so an Id never pairs the old date with the new day's time.
	static void NowParts(out string idBase, out string stamp)
	{
		int yy;
		int mo;
		int dd;
		GetYearMonthDayUTC(yy, mo, dd);
		int hh;
		int mi;
		int ss;
		GetHourMinuteSecondUTC(hh, mi, ss);
		int yy2;
		int mo2;
		int dd2;
		GetYearMonthDayUTC(yy2, mo2, dd2);
		if (yy2 != yy || mo2 != mo || dd2 != dd)
		{
			yy = yy2;
			mo = mo2;
			dd = dd2;
			GetHourMinuteSecondUTC(hh, mi, ss);
		}

		string yearText = VPPXmlText.PadInt(yy, 4);
		string monthText = VPPXmlText.Pad2(mo);
		string dayText = VPPXmlText.Pad2(dd);
		string hourText = VPPXmlText.Pad2(hh);
		string minuteText = VPPXmlText.Pad2(mi);
		string secondText = VPPXmlText.Pad2(ss);
		idBase = yearText + monthText + dayText + "-" + hourText + minuteText + secondText;
		stamp = yearText + "-" + monthText + "-" + dayText + " " + hourText + ":" + minuteText + ":" + secondText;
	}

	// Days since 1970-01-01 of a proleptic Gregorian date (integer only; H. Hinnant's days_from_civil).
	static int DaysFromCivil(int y, int m, int d)
	{
		int yAdj = y;
		if (m <= 2)
		{
			yAdj = yAdj - 1;
		}

		int era = yAdj / 400;
		if (yAdj < 0)
		{
			era = (yAdj - 399) / 400;
		}

		int yoe = yAdj - era * 400;
		int mp = m - 3;
		if (m <= 2)
		{
			mp = m + 9;
		}

		int doy = (153 * mp + 2) / 5 + d - 1;
		int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		return era * 146097 + doe - 719468;
	}

	static int DaysOfId(string recId)
	{
		return DaysFromCivil(IdPart(recId, 0, 4), IdPart(recId, 4, 2), IdPart(recId, 6, 2));
	}

	static int TodayDays()
	{
		int yy;
		int mo;
		int dd;
		GetYearMonthDayUTC(yy, mo, dd);
		return DaysFromCivil(yy, mo, dd);
	}

	// At most maxChars UTF-8 characters (never cuts a multi-byte character).
	static string ClipUtf8(string s, int maxChars)
	{
		if (s.LengthUtf8() <= maxChars)
		{
			return s;
		}

		return s.SubstringUtf8(0, maxChars);
	}

	// ceil(size / 1024) without overflow.
	static int KBOf(int size)
	{
		if (size <= 0)
		{
			return 0;
		}

		if (size > 2000000000)
		{
			return 1953125;
		}

		return (size + 1023) / 1024;
	}

	static int SatAdd(int a, int b)
	{
		if (a > KB_SATURATION - b)
		{
			return KB_SATURATION;
		}

		return a + b;
	}

	// =====================================================================
	// Paths
	// =====================================================================

	void EnsureBaseDirs()
	{
		if (m_DirsReady)
		{
			return;
		}

		MakeDirectory("$profile:VPPAdminTools");
		MakeDirectory("$profile:VPPAdminTools/XMLEditor");
		MakeDirectory("$profile:VPPAdminTools/XMLEditor/Backups");
		m_DirsReady = true;
	}

	static string ManifestPath(string slug)
	{
		return BASE_DIR + slug + "/manifest.json";
	}

	// The only way a backup file path is formed: index slug + record Id + extension of the record's file key.
	static string RecordPath(string slug, VPPXEBackupRecord rec)
	{
		return BASE_DIR + slug + "/" + rec.Id + ExtOf(rec.FileKey);
	}

	// =====================================================================
	// Index (slug -> file key)
	// =====================================================================

	VPPXEBackupIndex GetIndex()
	{
		if (m_Index)
		{
			return m_Index;
		}

		EnsureBaseDirs();
		string path = BASE_DIR + "index.json";
		if (!FileExist(path))
		{
			m_Index = new VPPXEBackupIndex();
			RebuildSlugMap();
			return m_Index;
		}

		VPPXEBackupIndex loaded;
		string err;
		bool ok = JsonFileLoader<VPPXEBackupIndex>.LoadFile(path, loaded, err);
		if (!ok || !loaded || !loaded.Slugs || !loaded.FileKeys)
		{
			m_Index = RecoverIndex(path, err);
		}
		else
		{
			m_Index = SanitizeIndex(loaded);
		}

		RebuildSlugMap();
		return m_Index;
	}

	protected void RebuildSlugMap()
	{
		m_SlugIdx.Clear();
		int slugCount = m_Index.Slugs.Count();
		for (int i = 0; i < slugCount; i++)
		{
			m_SlugIdx.Set(m_Index.Slugs[i], i);
		}
	}

	// Keeps only entries whose slug is exactly MakeSlug(key) (so a hand-edited index can never name another folder).
	protected VPPXEBackupIndex SanitizeIndex(VPPXEBackupIndex loaded)
	{
		VPPXEBackupIndex clean = new VPPXEBackupIndex();
		map<string, bool> seen = new map<string, bool>();
		int slugCount = loaded.Slugs.Count();
		int keyCount = loaded.FileKeys.Count();
		int pairCount = slugCount;
		if (keyCount < pairCount)
		{
			pairCount = keyCount;
		}

		int dropped = 0;
		if (slugCount != keyCount)
		{
			dropped = Math.AbsInt(slugCount - keyCount);
		}

		for (int i = 0; i < pairCount; i++)
		{
			string slug = loaded.Slugs[i];
			string key = loaded.FileKeys[i];
			if (key == "" || !IsValidSlug(slug) || seen.Contains(slug))
			{
				dropped++;
				continue;
			}

			string expected = MakeSlug(key);
			if (expected != slug)
			{
				dropped++;
				continue;
			}

			seen.Set(slug, true);
			clean.Slugs.Insert(slug);
			clean.FileKeys.Insert(key);
		}

		if (dropped > 0)
		{
			VPPXELog.Info("Backups: index.json had " + dropped.ToString() + " invalid entry(ies); they were ignored.");
		}

		return clean;
	}

	// index.json exists but does not load: copy it aside and rebuild it from the slug folders whose manifest loads.
	protected VPPXEBackupIndex RecoverIndex(string path, string loadError)
	{
		string idBase;
		string stamp;
		NowParts(idBase, stamp);
		string badPath = BASE_DIR + "index.bad-" + idBase + ".json";
		bool copied = CopyFile(path, badPath);
		VPPXEBackupIndex rebuilt = new VPPXEBackupIndex();
		string fname;
		FileAttr fattr;
		FindFileHandle finder = FindFile(BASE_DIR + "*", fname, fattr, FindFileFlags.ALL);
		if (finder)
		{
			bool more = true;
			while (more)
			{
				string key = ManifestKeyOf(fname);
				if (key != "")
				{
					rebuilt.Slugs.Insert(fname);
					rebuilt.FileKeys.Insert(key);
				}

				more = FindNextFile(finder, fname, fattr);
			}

			CloseFindFile(finder);
		}

		string saveErr;
		bool saved = JsonFileLoader<VPPXEBackupIndex>.SaveFile(path, rebuilt, saveErr);
		string msg = "WARNING Backups: index.json could not be loaded (" + VPPXmlText.Clip(loadError, VPPXEConst.MAX_CELL_CHARS) + ")";
		if (copied)
		{
			msg += "; copied aside to " + badPath;
		}
		else
		{
			msg += "; it could not be copied aside";
		}

		int rebuiltCount = rebuilt.Slugs.Count();
		msg += "; rebuilt " + rebuiltCount.ToString() + " file entry(ies) from the backup folders";
		if (!saved)
		{
			msg += "; the rebuilt index could not be saved: " + saveErr;
		}

		VPPXELog.Info(msg);
		return rebuilt;
	}

	// File key of a slug folder whose manifest loads and matches the slug, else "".
	protected string ManifestKeyOf(string dirName)
	{
		if (!IsValidSlug(dirName))
		{
			return "";
		}

		string mpath = ManifestPath(dirName);
		if (!FileExist(mpath))
		{
			return "";
		}

		VPPXEBackupManifest probe;
		string err;
		if (!JsonFileLoader<VPPXEBackupManifest>.LoadFile(mpath, probe, err))
		{
			return "";
		}

		if (!probe || probe.FileKey == "")
		{
			return "";
		}

		string expected = MakeSlug(probe.FileKey);
		if (expected != dirName)
		{
			return "";
		}

		return probe.FileKey;
	}

	bool SaveIndex()
	{
		EnsureBaseDirs();
		string err;
		if (!JsonFileLoader<VPPXEBackupIndex>.SaveFile(BASE_DIR + "index.json", GetIndex(), err))
		{
			VPPXELog.Info("Backups: index.json could not be saved: " + err);
			return false;
		}

		return true;
	}

	// Position of a slug in the index, -1 when absent.
	int IndexSlugOf(string slug)
	{
		GetIndex();
		int si = -1;
		if (m_SlugIdx.Find(slug, si))
		{
			return si;
		}

		return -1;
	}

	// The index slug of a file key ("" when the key has no backups yet). Never builds a path.
	string SlugForKey(string fileKey)
	{
		if (fileKey == "" || fileKey.Length() > MAX_BACKUP_ID_LENGTH)
		{
			return "";
		}

		string slug = MakeSlug(fileKey);
		int si = IndexSlugOf(slug);
		if (si < 0)
		{
			return "";
		}

		string indexedKey = m_Index.FileKeys[si];
		if (indexedKey != fileKey)
		{
			return "";
		}

		return m_Index.Slugs[si];
	}

	// =====================================================================
	// Manifests (cached after the first load)
	// =====================================================================

	VPPXEBackupManifest GetManifest(string slug)
	{
		VPPXEBackupManifest cached = m_Manifests.Get(slug);
		if (cached)
		{
			return cached;
		}

		int si = IndexSlugOf(slug);
		if (si < 0)
		{
			return null;
		}

		string fileKey = m_Index.FileKeys[si];
		string path = ManifestPath(slug);
		VPPXEBackupManifest result;
		if (FileExist(path))
		{
			VPPXEBackupManifest loaded;
			string err;
			bool ok = JsonFileLoader<VPPXEBackupManifest>.LoadFile(path, loaded, err);
			if (ok && loaded)
			{
				result = SanitizeManifest(fileKey, loaded);
			}
			else
			{
				result = RecoverManifest(slug, fileKey, err);
			}
		}
		else
		{
			result = new VPPXEBackupManifest();
			result.FileKey = fileKey;
		}

		m_Manifests.Set(slug, result);
		return result;
	}

	// The manifest of a file key; with create, registers the key in the index first (saved at once).
	VPPXEBackupManifest GetManifestForKey(string fileKey, bool create)
	{
		string slug = SlugForKey(fileKey);
		if (slug != "")
		{
			return GetManifest(slug);
		}

		if (!create || fileKey == "" || fileKey.Length() > MAX_BACKUP_ID_LENGTH)
		{
			return null;
		}

		string newSlug = MakeSlug(fileKey);
		if (IndexSlugOf(newSlug) >= 0)
		{
			VPPXELog.Info("Backups: slug collision for " + fileKey + " (" + newSlug + "); no backup was made.");
			return null;
		}

		VPPXEBackupIndex idx = GetIndex();
		idx.Slugs.Insert(newSlug);
		idx.FileKeys.Insert(fileKey);
		int newIdx = idx.Slugs.Count() - 1;
		m_SlugIdx.Set(newSlug, newIdx);
		if (!SaveIndex())
		{
			idx.Slugs.Remove(newIdx);
			idx.FileKeys.Remove(newIdx);
			m_SlugIdx.Remove(newSlug);
			return null;
		}

		EnsureBaseDirs();
		MakeDirectory(BASE_DIR + newSlug);
		return GetManifest(newSlug);
	}

	protected VPPXEBackupManifest SanitizeManifest(string fileKey, VPPXEBackupManifest loaded)
	{
		VPPXEBackupManifest clean = new VPPXEBackupManifest();
		clean.FileKey = fileKey;
		if (!loaded.Records)
		{
			return clean;
		}

		map<string, bool> seen = new map<string, bool>();
		int dropped = 0;
		int recordCount = loaded.Records.Count();
		for (int i = 0; i < recordCount; i++)
		{
			VPPXEBackupRecord rec = loaded.Records[i];
			if (!rec)
			{
				dropped++;
				continue;
			}

			if (!IsValidId(rec.Id) || seen.Contains(rec.Id))
			{
				dropped++;
				continue;
			}

			seen.Set(rec.Id, true);
			rec.FileKey = fileKey;
			if (!rec.Touched)
			{
				rec.Touched = new array<string>();
			}

			if (rec.Unverified)
			{
				rec.Pinned = true;
			}

			clean.Records.Insert(rec);
		}

		if (dropped > 0)
		{
			VPPXELog.Info("Backups: the manifest of " + fileKey + " had " + dropped.ToString() + " invalid record(s); they were ignored.");
		}

		return clean;
	}

	// manifest.json exists but does not load: copy it aside, rebuild the records from the backup files, save.
	protected VPPXEBackupManifest RecoverManifest(string slug, string fileKey, string loadError)
	{
		int startTicks = TickCount(0);
		string slugDir = BASE_DIR + slug + "/";
		string idBase;
		string stamp;
		NowParts(idBase, stamp);
		string badPath = slugDir + "manifest.bad-" + idBase + ".json";
		bool copied = CopyFile(ManifestPath(slug), badPath);
		string ext = ExtOf(fileKey);
		array<ref VPPXEBackupRecord> found = new array<ref VPPXEBackupRecord>();
		array<string> keys = new array<string>();
		string fname;
		FileAttr fattr;
		FindFileHandle finder = FindFile(slugDir + "*", fname, fattr, FindFileFlags.ALL);
		if (finder)
		{
			bool more = true;
			while (more)
			{
				VPPXEBackupRecord rec = RecordFromFile(slugDir, fname, fileKey, ext);
				if (rec)
				{
					keys.Insert(rec.Id + VPPXmlText.PadInt(found.Count(), 5));
					found.Insert(rec);
				}

				more = FindNextFile(finder, fname, fattr);
			}

			CloseFindFile(finder);
		}

		keys.Sort();
		VPPXEBackupManifest rebuilt = new VPPXEBackupManifest();
		rebuilt.FileKey = fileKey;
		int keyCount = keys.Count();
		for (int k = 0; k < keyCount; k++)
		{
			int foundIdx = KeyTail(keys[k], 5, 5);
			rebuilt.Records.Insert(found[foundIdx]);
		}

		string saveErr;
		bool saved = JsonFileLoader<VPPXEBackupManifest>.SaveFile(ManifestPath(slug), rebuilt, saveErr);
		int ticks = TickCount(startTicks);
		string msg = "WARNING Backups: the manifest of " + fileKey + " could not be loaded (" + VPPXmlText.Clip(loadError, VPPXEConst.MAX_CELL_CHARS) + ")";
		if (copied)
		{
			msg += "; copied aside to " + badPath;
		}
		else
		{
			msg += "; it could not be copied aside";
		}

		msg += "; rebuilt " + keyCount.ToString() + " record(s) from the backup files in " + ticks.ToString() + " CPU ticks";
		if (keyCount > 0)
		{
			msg += "; " + keyCount.ToString() + " record(s) were recovered as pinned because their pin state was lost; unpin the ones you do not need in BACKUPS";
		}

		if (!saved)
		{
			msg += "; the rebuilt manifest could not be saved: " + saveErr;
		}

		VPPXELog.Info(msg);
		return rebuilt;
	}

	// A record rebuilt from a backup file named <Id><ext>; null for any other file (manifest*.json, foreign files).
	protected VPPXEBackupRecord RecordFromFile(string slugDir, string fname, string fileKey, string ext)
	{
		if (fname.IndexOf("manifest") == 0)
		{
			return null;
		}

		int extLength = ext.Length();
		if (fname.Length() != ID_LENGTH + extLength)
		{
			return null;
		}

		string recId = fname.Substring(0, ID_LENGTH);
		string tail = fname.Substring(ID_LENGTH, extLength);
		tail.ToLower();
		if (tail != ext || !IsValidId(recId))
		{
			return null;
		}

		VPPXEBackupRecord rec = new VPPXEBackupRecord();
		rec.Id = recId;
		rec.FileKey = fileKey;
		rec.Stamp = StampFromId(recId);
		rec.AdminName = "unknown";
		rec.AdminId = "";
		rec.Reason = VPPXEBackupReason.MANUAL;
		rec.Summary = "";
		rec.Note = "";
		// The pin state was lost with the manifest: pin every recovered record so retention cannot delete a backup the owner had pinned.
		rec.Pinned = true;
		string content;
		if (VPPXmlText.ReadAll(slugDir + fname, content))
		{
			rec.Size = content.Length();
			rec.Hash = VPPXmlText.NormalizedHash(content);
		}
		else
		{
			rec.Size = 0;
			rec.Hash = 0;
			rec.Unverified = true;
		}

		return rec;
	}

	bool SaveManifest(string slug)
	{
		VPPXEBackupManifest mf = m_Manifests.Get(slug);
		if (!mf)
		{
			return false;
		}

		EnsureBaseDirs();
		MakeDirectory(BASE_DIR + slug);
		string err;
		if (!JsonFileLoader<VPPXEBackupManifest>.SaveFile(ManifestPath(slug), mf, err))
		{
			VPPXELog.Info("Backups: the manifest of " + mf.FileKey + " could not be saved: " + err);
			return false;
		}

		return true;
	}

	static int FindRecordIdx(VPPXEBackupManifest mf, string recId)
	{
		if (!mf || !mf.Records)
		{
			return -1;
		}

		int recordCount = mf.Records.Count();
		for (int i = 0; i < recordCount; i++)
		{
			VPPXEBackupRecord rec = mf.Records[i];
			if (rec && rec.Id == recId)
			{
				return i;
			}
		}

		return -1;
	}

	// RESOLVE: split at the single slash, both parts [A-Za-z0-9_.-]+, slug in the index, Id of a record in its manifest.
	// slug is set to the INDEX's own slug string; null when anything fails (callers reply ERR_BACKUP_MISSING).
	VPPXEBackupRecord ResolveRecord(string backupId, out string slug)
	{
		slug = "";
		int idLength = backupId.Length();
		if (idLength < 3 || idLength > MAX_BACKUP_ID_LENGTH)
		{
			return null;
		}

		int slash = backupId.IndexOf("/");
		if (slash <= 0 || slash != backupId.LastIndexOf("/"))
		{
			return null;
		}

		string slugPart = backupId.Substring(0, slash);
		string idPart = backupId.Substring(slash + 1, idLength - slash - 1);
		if (!IsSafeToken(slugPart) || !IsSafeToken(idPart))
		{
			return null;
		}

		int si = IndexSlugOf(slugPart);
		if (si < 0)
		{
			return null;
		}

		string indexSlug = m_Index.Slugs[si];
		VPPXEBackupManifest mf = GetManifest(indexSlug);
		int ri = FindRecordIdx(mf, idPart);
		if (ri < 0)
		{
			return null;
		}

		slug = indexSlug;
		return mf.Records[ri];
	}

	// Canonical BackupId of the next newer record of the same file that is not Unverified, "" when none.
	string NextNewerId(string slug, VPPXEBackupRecord rec)
	{
		VPPXEBackupManifest mf = GetManifest(slug);
		if (!mf || !rec)
		{
			return "";
		}

		VPPXEBackupRecord best = null;
		int recordCount = mf.Records.Count();
		for (int i = 0; i < recordCount; i++)
		{
			VPPXEBackupRecord cand = mf.Records[i];
			if (!cand || cand == rec || cand.Unverified)
			{
				continue;
			}

			if (CompareIds(cand.Id, rec.Id) <= 0)
			{
				continue;
			}

			if (!best || CompareIds(cand.Id, best.Id) < 0)
			{
				best = cand;
			}
		}

		if (!best)
		{
			return "";
		}

		return MakeBackupId(slug, best.Id);
	}

	// =====================================================================
	// Creating backups (synchronous; called only inside INTERACTIVE jobs)
	// =====================================================================

	protected VPPXEBackupRecord NewRecord(string recId, string fileKey, string stamp, int reason, PlayerIdentity who, string summary, string note, int changeCount)
	{
		VPPXEBackupRecord rec = new VPPXEBackupRecord();
		rec.Id = recId;
		rec.FileKey = fileKey;
		rec.Stamp = stamp;
		rec.AdminName = "System";
		rec.AdminId = "";
		if (who)
		{
			rec.AdminName = ClipUtf8(who.GetName(), MAX_ADMIN_TEXT);
			rec.AdminId = ClipUtf8(who.GetPlainId(), MAX_ADMIN_TEXT);
		}

		rec.Reason = reason;
		rec.Summary = ClipUtf8(summary, MAX_SUMMARY);
		rec.Note = ClipUtf8(note, VPPXEConst.MAX_NOTE_LENGTH);
		rec.ChangeCount = changeCount;
		return rec;
	}

	// Registers the file key if needed, creates the slug folder and picks a free Id (NNN counts up while taken).
	protected string AllocateId(VPPXEFileEntry fileEntry, out string slug, out string stamp)
	{
		slug = "";
		stamp = "";
		if (!fileEntry || fileEntry.Key == "")
		{
			return "";
		}

		VPPXEBackupManifest mf = GetManifestForKey(fileEntry.Key, true);
		if (!mf)
		{
			return "";
		}

		string indexSlug = SlugForKey(fileEntry.Key);
		if (indexSlug == "")
		{
			return "";
		}

		EnsureBaseDirs();
		MakeDirectory(BASE_DIR + indexSlug);
		string idBase;
		string nowStamp;
		NowParts(idBase, nowStamp);
		string ext = ExtOf(fileEntry.Key);
		for (int n = 0; n < MAX_ID_COUNTER; n++)
		{
			string candidate = idBase + "-" + VPPXmlText.Pad3(n);
			string candidatePath = BASE_DIR + indexSlug + "/" + candidate + ext;
			if (!FileExist(candidatePath) && FindRecordIdx(mf, candidate) < 0)
			{
				slug = indexSlug;
				stamp = nowStamp;
				return candidate;
			}
		}

		VPPXELog.Info("Backups: no free backup id for " + fileEntry.Key + " at " + idBase + "; no backup was made.");
		return "";
	}

	// Appends and saves; on a failed save the record is taken back out.
	protected bool AppendRecord(string slug, VPPXEBackupRecord rec)
	{
		VPPXEBackupManifest mf = GetManifest(slug);
		if (!mf)
		{
			return false;
		}

		mf.Records.Insert(rec);
		if (!SaveManifest(slug))
		{
			mf.Records.Remove(mf.Records.Count() - 1);
			return false;
		}

		return true;
	}

	// True when the file on disk still holds content ("" = the file does not exist). Read now, in the same
	// synchronous call as the backup and the caller's write, so no frame gap separates the check from them.
	protected bool SourceStillMatches(VPPXEFileEntry file, string content)
	{
		if (!FileExist(file.Path))
		{
			return content == "";
		}

		string nowText;
		if (!VPPXmlText.ReadAll(file.Path, nowText))
		{
			return false;
		}

		return VPPXmlText.SameNormalized(nowText, content);
	}

	// content = exactly what the caller read from the file ("" ONLY for a file that does not exist).
	// CopyFile, then ReadAll the copy and require SameNormalized; else WriteAll(content) and check again.
	// The WriteAll fallback runs only while the file itself still matches content (a bad copy): when the file
	// changed after the caller read it, no backup is made and "" is returned, so the caller never overwrites an
	// external edit that no backup holds.
	string CreateBackup(VPPXEFileEntry file, string content, int reason, PlayerIdentity who, string summary, int changeCount, array<string> touched, string note)
	{
		string slug;
		string stamp;
		string recId = AllocateId(file, slug, stamp);
		if (recId == "")
		{
			return "";
		}

		string backupPath = BASE_DIR + slug + "/" + recId + ExtOf(file.Key);
		bool verified = false;
		string copyText;
		if (CopyFile(file.Path, backupPath))
		{
			if (VPPXmlText.ReadAll(backupPath, copyText))
			{
				verified = VPPXmlText.SameNormalized(copyText, content);
			}
		}

		copyText = "";
		if (!verified && !SourceStillMatches(file, content))
		{
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			VPPXELog.Info("Backups: " + file.Key + " changed on disk after it was read; no backup was made and the write is refused.");
			return "";
		}

		if (!verified)
		{
			string rewrittenText;
			// Same bytes VPPXESafeWrite hands FPrint: on a WRITE_CRLF host a CRLF written as-is lands as CR CR LF.
			// VPPXmlText.CRLFToLF, not string.Replace, which keeps only about 5 KB of its result.
			string fallbackBytes = content;
			if (VPPXESafeWrite.GetEolMode() == VPPXEEolMode.WRITE_CRLF)
			{
				fallbackBytes = VPPXmlText.CRLFToLF(content);
			}

			if (VPPXmlText.WriteAll(backupPath, fallbackBytes))
			{
				if (VPPXmlText.ReadAll(backupPath, rewrittenText))
				{
					verified = VPPXmlText.SameNormalized(rewrittenText, content);
				}
			}
		}

		if (!verified)
		{
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			VPPXELog.Info("Backups: the backup of " + file.Key + " could not be written or did not verify; no backup was made.");
			return "";
		}

		VPPXEBackupRecord rec = NewRecord(recId, file.Key, stamp, reason, who, summary, note, changeCount);
		rec.Size = content.Length();
		rec.Hash = VPPXmlText.NormalizedHash(content);
		rec.Pinned = false;
		rec.Unverified = false;
		if (touched)
		{
			int touchedCount = touched.Count();
			if (touchedCount > MAX_TOUCHED)
			{
				touchedCount = MAX_TOUCHED;
			}

			for (int i = 0; i < touchedCount; i++)
			{
				rec.Touched.Insert(touched[i]);
			}
		}

		if (!AppendRecord(slug, rec))
		{
			DeleteFile(backupPath);
			return "";
		}

		return MakeBackupId(slug, recId);
	}

	// For a file that EXISTS but cannot be read: a byte-exact CopyFile kept as an Unverified, pinned record
	// (Hash 0, Size 0). The copy is never checked against content (there is none) and never overwritten.
	string CreateRawBackup(VPPXEFileEntry file, int reason, PlayerIdentity who, string summary, string note)
	{
		string slug;
		string stamp;
		string recId = AllocateId(file, slug, stamp);
		if (recId == "")
		{
			return "";
		}

		string backupPath = BASE_DIR + slug + "/" + recId + ExtOf(file.Key);
		if (!CopyFile(file.Path, backupPath))
		{
			if (FileExist(backupPath))
			{
				DeleteFile(backupPath);
			}

			VPPXELog.Info("WARNING Backups: " + file.Key + " could not be read and the raw copy failed; no backup was made.");
			return "";
		}

		VPPXEBackupRecord rec = NewRecord(recId, file.Key, stamp, reason, who, summary, note, 0);
		rec.Size = 0;
		rec.Hash = 0;
		rec.Pinned = true;
		rec.Unverified = true;
		if (!AppendRecord(slug, rec))
		{
			VPPXELog.Info("WARNING Backups: " + file.Key + " could not be read; its raw copy " + backupPath + " was kept on disk but could not be recorded.");
			return "";
		}

		string backupId = MakeBackupId(slug, recId);
		VPPXELog.Info("WARNING Backups: " + file.Key + " could not be read; a raw copy was kept as pinned, unverified backup " + backupId + " (recover it by hand from " + backupPath + ").");
		return backupId;
	}

	// Resolve, refuse Unverified records, ReadAll the record's file. Used by VPPXESafeWrite's journal checks.
	bool ReadBackup(string backupId, out string content)
	{
		content = "";
		string slug;
		VPPXEBackupRecord rec = ResolveRecord(backupId, slug);
		if (!rec || rec.Unverified)
		{
			return false;
		}

		string text;
		if (!VPPXmlText.ReadAll(RecordPath(slug, rec), text))
		{
			return false;
		}

		content = text;
		return true;
	}

	void SetResultHash(string backupId, int resultHash)
	{
		string slug;
		VPPXEBackupRecord rec = ResolveRecord(backupId, slug);
		if (!rec)
		{
			return;
		}

		rec.ResultHash = resultHash;
		SaveManifest(slug);
	}

	// ResultHash of the newest record with a non-zero ResultHash, 0 = unknown.
	int GetLastResultHash(string fileKey)
	{
		string slug = SlugForKey(fileKey);
		if (slug == "")
		{
			return 0;
		}

		VPPXEBackupManifest mf = GetManifest(slug);
		if (!mf)
		{
			return 0;
		}

		VPPXEBackupRecord best = null;
		int recordCount = mf.Records.Count();
		for (int i = 0; i < recordCount; i++)
		{
			VPPXEBackupRecord rec = mf.Records[i];
			if (!rec || rec.ResultHash == 0)
			{
				continue;
			}

			if (!best || CompareIds(rec.Id, best.Id) > 0)
			{
				best = rec;
			}
		}

		if (!best)
		{
			return 0;
		}

		return best.ResultHash;
	}

	// =====================================================================
	// Leases and pins
	// =====================================================================

	void Lease(string backupId)
	{
		if (backupId == "")
		{
			return;
		}

		int held = 0;
		if (m_Leases.Find(backupId, held))
		{
			m_Leases.Set(backupId, held + 1);
		}
		else
		{
			m_Leases.Set(backupId, 1);
		}
	}

	void Release(string backupId)
	{
		if (backupId == "")
		{
			return;
		}

		int held = 0;
		if (!m_Leases.Find(backupId, held))
		{
			return;
		}

		if (held <= 1)
		{
			m_Leases.Remove(backupId);
		}
		else
		{
			m_Leases.Set(backupId, held - 1);
		}
	}

	bool IsLeased(string backupId)
	{
		return m_Leases.Contains(backupId);
	}

	// Pins + saves (journal, interrupted and unverified backups).
	void AutoPin(string backupId)
	{
		string slug;
		VPPXEBackupRecord rec = ResolveRecord(backupId, slug);
		if (!rec)
		{
			VPPXELog.Info("Backups: auto-pin of " + backupId + " failed: no such backup.");
			return;
		}

		if (!rec.Pinned)
		{
			rec.Pinned = true;
			SaveManifest(slug);
		}

		VPPXELog.Info("Backups: auto-pinned " + backupId + ".");
	}

	// =====================================================================
	// Retention (call only inside INTERACTIVE jobs)
	// =====================================================================

	protected bool IsRetentionExempt(string backupId, VPPXEBackupRecord rec, bool newest)
	{
		if (rec.Pinned || newest)
		{
			return true;
		}

		if (IsLeased(backupId))
		{
			return true;
		}

		return VPPXESafeWrite.IsJournaled(backupId);
	}

	void ApplyRetention()
	{
		if (!m_Owner)
		{
			return;
		}

		VPPXESettings st = m_Owner.GetSettings();
		if (!st)
		{
			return;
		}

		int maxPerFile = st.BackupMaxPerFile;
		if (maxPerFile < 1)
		{
			maxPerFile = 1;
		}

		int maxAgeDays = st.BackupMaxAgeDays;
		if (maxAgeDays < 0)
		{
			maxAgeDays = 0;
		}

		int maxMB = st.BackupMaxTotalMB;
		if (maxMB < 1)
		{
			maxMB = 1;
		}

		if (maxMB > MAX_TOTAL_MB)
		{
			maxMB = MAX_TOTAL_MB;
		}

		int capKB = maxMB * 1024;
		int today = TodayDays();
		VPPXEBackupIndex idx = GetIndex();
		map<string, bool> doomed = new map<string, bool>();
		array<string> globalKeys = new array<string>();
		int totalKB = 0;
		int slugCount = idx.Slugs.Count();
		for (int mi = 0; mi < slugCount; mi++)
		{
			string slug = idx.Slugs[mi];
			VPPXEBackupManifest mf = GetManifest(slug);
			if (!mf || mf.Records.Count() == 0)
			{
				continue;
			}

			int fileKB = RetainPerFile(mi, slug, mf, maxPerFile, maxAgeDays, today, doomed, globalKeys);
			totalKB = SatAdd(totalKB, fileKB);
		}

		// Global cap in KB: delete the globally oldest non-exempt records until the total fits.
		if (totalKB > capKB)
		{
			globalKeys.Sort();
			int globalCount = globalKeys.Count();
			for (int g = 0; g < globalCount; g++)
			{
				if (totalKB <= capKB)
				{
					break;
				}

				string globalKey = globalKeys[g];
				int manIdx = KeyTail(globalKey, 9, 4);
				int recIdx = KeyTail(globalKey, 5, 5);
				string globalSlug = idx.Slugs[manIdx];
				VPPXEBackupManifest globalMf = GetManifest(globalSlug);
				VPPXEBackupRecord victim = globalMf.Records[recIdx];
				doomed.Set(MakeBackupId(globalSlug, victim.Id), true);
				totalKB -= KBOf(victim.Size);
			}
		}

		if (doomed.Count() == 0)
		{
			return;
		}

		int removed = 0;
		for (int di = 0; di < slugCount; di++)
		{
			removed += PurgeDoomed(idx.Slugs[di], doomed);
		}

		VPPXELog.Info("Backups: retention removed " + removed.ToString() + " backup(s).");
	}

	// Per file: order by Id (native key sort), drop non-exempt records that are too old or beyond BackupMaxPerFile
	// (oldest first). Returns the KB of the surviving records; surviving non-exempt ones go into globalKeys.
	protected int RetainPerFile(int mi, string slug, VPPXEBackupManifest mf, int maxPerFile, int maxAgeDays, int today, map<string, bool> doomed, array<string> globalKeys)
	{
		int recordCount = mf.Records.Count();
		array<string> keys = new array<string>();
		int unpinned = 0;
		for (int ri = 0; ri < recordCount; ri++)
		{
			VPPXEBackupRecord rec = mf.Records[ri];
			keys.Insert(rec.Id + VPPXmlText.PadInt(ri, 5));
			if (!rec.Pinned)
			{
				unpinned++;
			}
		}

		keys.Sort();
		int keptKB = 0;
		for (int k = 0; k < recordCount; k++)
		{
			int recIdx = KeyTail(keys[k], 5, 5);
			VPPXEBackupRecord cand = mf.Records[recIdx];
			string backupId = MakeBackupId(slug, cand.Id);
			bool newest = false;
			if (k == recordCount - 1)
			{
				newest = true;
			}

			bool exempt = IsRetentionExempt(backupId, cand, newest);
			if (!exempt)
			{
				bool expired = false;
				if (maxAgeDays > 0)
				{
					int age = today - DaysOfId(cand.Id);
					if (age > maxAgeDays)
					{
						expired = true;
					}
				}

				if (expired || unpinned > maxPerFile)
				{
					doomed.Set(backupId, true);
					unpinned--;
					continue;
				}

				globalKeys.Insert(cand.Id + VPPXmlText.PadInt(mi, 4) + VPPXmlText.PadInt(recIdx, 5));
			}

			keptKB = SatAdd(keptKB, KBOf(cand.Size));
		}

		return keptKB;
	}

	// Removes the doomed records of one manifest (DeleteFile on the path built from each record), saves it.
	// Returns the number of records removed. Used by retention and by the delete job.
	int PurgeDoomed(string slug, map<string, bool> doomed)
	{
		VPPXEBackupManifest mf = GetManifest(slug);
		if (!mf)
		{
			return 0;
		}

		array<ref VPPXEBackupRecord> kept = new array<ref VPPXEBackupRecord>();
		int purged = 0;
		int recordCount = mf.Records.Count();
		for (int i = 0; i < recordCount; i++)
		{
			VPPXEBackupRecord rec = mf.Records[i];
			string backupId = MakeBackupId(slug, rec.Id);
			if (!doomed.Contains(backupId))
			{
				kept.Insert(rec);
				continue;
			}

			string path = RecordPath(slug, rec);
			if (FileExist(path))
			{
				if (!DeleteFile(path))
				{
					VPPXELog.Info("Backups: could not delete " + path + ".");
				}
			}

			purged++;
		}

		if (purged > 0)
		{
			mf.Records = kept;
			SaveManifest(slug);
		}

		return purged;
	}

	// =====================================================================
	// Request entry points (each enqueues one INTERACTIVE job; O(1) or one manifest lookup)
	// =====================================================================

	void HandleList(PlayerIdentity sender, int reqId, string fileKey)
	{
		if (!sender)
		{
			return;
		}

		VPPXEBackupListJob job = new VPPXEBackupListJob(this, sender, reqId, fileKey);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void HandleDiff(PlayerIdentity sender, int reqId, string backupId, int mode)
	{
		if (!sender)
		{
			return;
		}

		string slug;
		VPPXEBackupRecord rec = ResolveRecord(backupId, slug);
		if (!rec)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_BACKUP_MISSING", "");
			return;
		}

		int useMode = VPPXEDiffMode.VS_CURRENT;
		if (mode == VPPXEDiffMode.VS_NEXT)
		{
			useMode = VPPXEDiffMode.VS_NEXT;
		}

		string canonicalId = MakeBackupId(slug, rec.Id);
		string nextId = "";
		if (useMode == VPPXEDiffMode.VS_NEXT)
		{
			nextId = NextNewerId(slug, rec);
		}

		Lease(canonicalId);
		Lease(nextId);
		VPPXEDiffJob job = new VPPXEDiffJob(this, sender, reqId, canonicalId, nextId, useMode);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void HandleRestore(PlayerIdentity sender, int reqId, string backupId)
	{
		if (!sender)
		{
			return;
		}

		string slug;
		VPPXEBackupRecord rec = ResolveRecord(backupId, slug);
		if (!rec)
		{
			VPPXENet.Result(sender, reqId, false, "#VSTR_XMLE_ERR_BACKUP_MISSING", "");
			return;
		}

		string canonicalId = MakeBackupId(slug, rec.Id);
		Lease(canonicalId);
		VPPXERestoreJob job = new VPPXERestoreJob(this, sender, reqId, canonicalId);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void HandleDelete(PlayerIdentity sender, int reqId, array<string> backupIds)
	{
		if (!sender)
		{
			return;
		}

		array<string> ids = new array<string>();
		if (backupIds)
		{
			int idCount = backupIds.Count();
			int idCap = VPPXEConst.DELETE_PER_REQUEST;
			if (idCount > idCap)
			{
				VPPXELog.Info("Backups: a delete request named " + idCount.ToString() + " backups; only the first " + idCap.ToString() + " are handled.");
				idCount = idCap;
			}

			for (int i = 0; i < idCount; i++)
			{
				ids.Insert(backupIds[i]);
			}
		}

		VPPXEBackupOpJob job = new VPPXEBackupOpJob(this, sender, reqId, VPPXEBackupOpJob.OP_DELETE, ids, "", false, "", "");
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void HandlePin(PlayerIdentity sender, int reqId, string backupId, bool pinned)
	{
		if (!sender)
		{
			return;
		}

		VPPXEBackupOpJob job = new VPPXEBackupOpJob(this, sender, reqId, VPPXEBackupOpJob.OP_PIN, null, backupId, pinned, "", "");
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}

	void HandleSnapshot(PlayerIdentity sender, int reqId, string fileKey, string note)
	{
		if (!sender)
		{
			return;
		}

		VPPXEBackupOpJob job = new VPPXEBackupOpJob(this, sender, reqId, VPPXEBackupOpJob.OP_SNAPSHOT, null, "", false, fileKey, note);
		VPPXEJobQueue.Get().Enqueue(job, VPPXELane.INTERACTIVE);
	}
};
