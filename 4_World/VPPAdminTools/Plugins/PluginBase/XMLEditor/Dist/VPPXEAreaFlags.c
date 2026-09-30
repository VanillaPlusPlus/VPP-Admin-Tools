// XML Editor: areaflags.map sampler (DIST-ALGO v2.1 R7).
// Layout: a 24 B header of 6 ints (W, H, worldX, worldZ, 32, 0), then a u32 usage plane of W*H cells,
// then a u8 value plane of W*H cells. The file is streamed with ReadFile(fh, int[4096], 16384) inside
// budgeted steps (byte length, advancing position: CF_ModStorageModule.c:111) and only the sorted unique
// cells that are needed are kept. Validation: header sanity, total bytes == 24 + 5*W*H, and one more read
// returns 0. A cache in $profile is used when its key matches; a once-per-boot background job revalidates it.

class VPPXEAreaFlagsCache
{
	string CacheWorld;
	ref array<int> HeaderInts;
	int FirstChunkHash;
	string GameVersion;
	int PosRev;
	int SpawnRev;
	int ByteTotal;
	int LastChunkHash;
	ref array<int> Cells;
	ref array<int> Usage;
	ref array<int> Value;

	void VPPXEAreaFlagsCache()
	{
		HeaderInts = new array<int>();
		Cells = new array<int>();
		Usage = new array<int>();
		Value = new array<int>();
	}
};

// Sampled cells of one areaflags.map read; never modified after it is built (revalidation builds a new one).
class VPPXEAreaSamples : Managed
{
	bool Ok;
	string Reason;
	int W;
	int H;
	float WorldX;
	float WorldZ;
	ref array<int> HeaderInts;
	int FirstChunkHash;
	int ByteTotal;
	int LastChunkHash;
	ref array<int> Cells;
	ref array<int> Usage;
	ref array<int> Value;
	ref map<int, int> Index;

	void VPPXEAreaSamples()
	{
		Ok = false;
		Reason = "";
		W = 0;
		H = 0;
		WorldX = 0;
		WorldZ = 0;
		FirstChunkHash = 0;
		ByteTotal = 0;
		LastChunkHash = 0;
		HeaderInts = new array<int>();
		Cells = new array<int>();
		Usage = new array<int>();
		Value = new array<int>();
		Index = new map<int, int>();
	}

	void SetHeader(int w, int h, int worldX, int worldZ)
	{
		W = w;
		H = h;
		WorldX = worldX;
		WorldZ = worldZ;
		HeaderInts.Clear();
		HeaderInts.Insert(w);
		HeaderInts.Insert(h);
		HeaderInts.Insert(worldX);
		HeaderInts.Insert(worldZ);
		HeaderInts.Insert(32);
		HeaderInts.Insert(0);
	}

	// R7: col = clamp(floor(x / (worldX / W)), 0, W-1); row = clamp(floor(z / (worldZ / H)), 0, H-1)
	int CellOf(float x, float z)
	{
		if (W < 1 || H < 1)
		{
			return 0;
		}

		float cellW = WorldX / W;
		float cellH = WorldZ / H;
		int col = Math.Floor(x / cellW);
		int row = Math.Floor(z / cellH);
		col = VPPXEDistUtil.ClampInt(col, 0, W - 1);
		row = VPPXEDistUtil.ClampInt(row, 0, H - 1);
		return row * W + col;
	}

	// usage = u32 at the cell, value = u8 at the cell; unavailable: usage 0, value 255 (no-tier mode)
	bool SampleAt(float x, float z, out int usage, out int value)
	{
		usage = 0;
		value = 255;
		if (!Ok)
		{
			return false;
		}

		int cell = CellOf(x, z);
		int idx;
		if (!Index.Find(cell, idx))
		{
			return false;
		}

		usage = Usage[idx];
		value = Value[idx];
		return true;
	}
};

// Resumable sampler, driven by the MAP job (BeginSample) or by the revalidation job (BeginRevalidate).
class VPPXEAreaFlags : Managed
{
	const static string AREAFLAGS_PATH = "$mission:areaflags.map";
	const static string CACHE_PATH = "$profile:VPPAdminTools/XMLEditor/Cache/areaflags_samples.json";
	const static int CHUNK_BYTES_READ = 16384;
	const static int CHUNK_INTS = 4096;
	const static int MAX_DIM = 16384;

	const static int PH_OPEN = 0;
	const static int PH_CELLS = 1;
	const static int PH_SORT = 2;
	const static int PH_DEDUPE = 3;
	const static int PH_CACHE = 4;
	const static int PH_CACHE_CHECK = 5;
	const static int PH_STREAM = 6;
	const static int PH_COMPARE = 7;
	const static int PH_INDEX = 8;
	const static int PH_SAVE = 9;
	const static int PH_DONE = 10;

	protected string m_WorldName;
	protected int m_PosRev;
	protected int m_SpawnRev;
	protected bool m_Enabled;
	protected bool m_Revalidate;
	protected ref VPPXEAreaSamples m_Current;
	protected ref array<float> m_PX;
	protected ref array<float> m_PZ;
	protected ref VPPXEAreaSamples m_Out;
	protected ref array<int> m_Needed;
	protected ref array<int> m_Cells;
	protected ref array<int> m_OutU;
	protected ref array<int> m_OutV;
	protected ref VPPXEAreaFlagsCache m_Cache;
	protected FileHandle m_Fh;
	protected bool m_FileOpen;
	protected int m_BufA[4096];
	protected int m_BufB[4096];
	protected bool m_CurB;
	protected bool m_NextB;
	protected int m_LastFull;
	protected int m_ChunkStart;
	protected int m_ChunkEnd;
	protected int m_Total;
	protected int m_Expected;
	protected int m_ValueBase;
	protected bool m_ShortSeen;
	protected int m_UCur;
	protected int m_VCur;
	protected int m_Phase;
	protected int m_Cursor;
	protected int m_Cursor2;
	protected bool m_FromCache;
	protected bool m_Changed;
	protected string m_Result;
	protected int m_StartMs;
	protected int m_Reads;

	void VPPXEAreaFlags(string worldName, int posRev, int spawnRev, bool enabled)
	{
		m_WorldName = worldName;
		m_PosRev = posRev;
		m_SpawnRev = spawnRev;
		m_Enabled = enabled;
		m_Revalidate = false;
		m_Needed = new array<int>();
		m_Cells = new array<int>();
		m_OutU = new array<int>();
		m_OutV = new array<int>();
		m_FileOpen = false;
		m_CurB = false;
		m_NextB = false;
		m_LastFull = 0;
		m_ChunkStart = 0;
		m_ChunkEnd = 0;
		m_Total = 0;
		m_Expected = 0;
		m_ValueBase = 0;
		m_ShortSeen = false;
		m_UCur = 0;
		m_VCur = 0;
		m_Phase = PH_OPEN;
		m_Cursor = 0;
		m_Cursor2 = 0;
		m_FromCache = false;
		m_Changed = false;
		m_Result = "";
		m_StartMs = 0;
		m_Reads = 0;
	}

	void ~VPPXEAreaFlags()
	{
		CloseStream();
	}

	// normal mode: sample the cells of these points (cache fast path allowed)
	void BeginSample(array<float> xs, array<float> zs)
	{
		m_PX = xs;
		m_PZ = zs;
		m_Revalidate = false;
		m_Phase = PH_OPEN;
	}

	// revalidation mode: stream the whole file and re-sample the cells of the current samples
	void BeginRevalidate(VPPXEAreaSamples current)
	{
		m_Current = current;
		m_Revalidate = true;
		m_Phase = PH_OPEN;
	}

	VPPXEAreaSamples GetSamples()
	{
		return m_Out;
	}

	bool IsFromCache()
	{
		return m_FromCache;
	}

	// revalidation: true when the file no longer matches the samples that were in use
	bool HasChanged()
	{
		return m_Changed;
	}

	string GetResultText()
	{
		return m_Result;
	}

	int GetPercent()
	{
		if (m_Phase == PH_DONE)
		{
			return 100;
		}

		if (m_Phase != PH_STREAM || m_Expected <= 0)
		{
			return 0;
		}

		int pct = m_Total / (m_Expected / 100 + 1);
		return VPPXEDistUtil.ClampInt(pct, 0, 99);
	}

	static string CurrentGameVersion()
	{
		string version;
		GetGame().GetVersion(version);
		return version;
	}

	bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_DONE)
			{
				return true;
			}

			if (m_Phase == PH_OPEN)
			{
				OpenUnit();
			}
			else if (m_Phase == PH_CELLS)
			{
				CellsUnit();
			}
			else if (m_Phase == PH_SORT)
			{
				m_Needed.Sort();
				m_Cursor = 0;
				m_Phase = PH_DEDUPE;
			}
			else if (m_Phase == PH_DEDUPE)
			{
				DedupeUnit();
			}
			else if (m_Phase == PH_CACHE)
			{
				CacheUnit();
			}
			else if (m_Phase == PH_CACHE_CHECK)
			{
				CacheCheckUnit();
			}
			else if (m_Phase == PH_STREAM)
			{
				StreamUnit();
			}
			else if (m_Phase == PH_COMPARE)
			{
				CompareUnit();
			}
			else if (m_Phase == PH_INDEX)
			{
				IndexUnit();
			}
			else if (m_Phase == PH_SAVE)
			{
				SaveUnit();
			}
		}

		return m_Phase == PH_DONE;
	}

	protected void CloseStream()
	{
		if (m_FileOpen)
		{
			CloseFile(m_Fh);
		}

		m_FileOpen = false;
	}

	protected void Fail(string reason)
	{
		CloseStream();
		m_Out.Ok = false;
		m_Out.Reason = reason;
		m_Out.Cells = new array<int>();
		m_Out.Usage = new array<int>();
		m_Out.Value = new array<int>();
		m_Out.Index = new map<int, int>();
		if (m_Revalidate)
		{
			m_Changed = true;
		}

		m_Result = "off (" + reason + ")";
		m_Phase = PH_DONE;
	}

	protected int BufAt(int idx)
	{
		if (m_CurB)
		{
			return m_BufB[idx];
		}

		return m_BufA[idx];
	}

	protected int HashBuffer(bool useB, int count)
	{
		int h = 0;
		for (int i = 0; i < count; i++)
		{
			int v;
			if (useB)
			{
				v = m_BufB[i];
			}
			else
			{
				v = m_BufA[i];
			}

			h = h * 31 + v;
		}

		return h;
	}

	// reads the next chunk into the other buffer, so the last full chunk survives a short final read
	protected int ReadChunk()
	{
		bool intoB = m_NextB;
		int n;
		if (intoB)
		{
			n = ReadFile(m_Fh, m_BufB, CHUNK_BYTES_READ);
		}
		else
		{
			n = ReadFile(m_Fh, m_BufA, CHUNK_BYTES_READ);
		}

		m_Reads++;
		if (n <= 0)
		{
			return n;
		}

		m_CurB = intoB;
		m_NextB = !intoB;
		m_ChunkStart = m_Total;
		m_Total = m_Total + n;
		m_ChunkEnd = m_Total;
		if (n == CHUNK_BYTES_READ)
		{
			if (intoB)
			{
				m_LastFull = 2;
			}
			else
			{
				m_LastFull = 1;
			}
		}

		return n;
	}

	protected void OpenUnit()
	{
		m_StartMs = GetGame().GetTime();
		m_Out = new VPPXEAreaSamples();
		if (!m_Enabled)
		{
			Fail("disabled by EnableAreaFlags");
			return;
		}

		if (!FileExist(AREAFLAGS_PATH))
		{
			Fail("areaflags.map not found");
			return;
		}

		m_Fh = OpenFile(AREAFLAGS_PATH, FileMode.READ);
		if (m_Fh == 0)
		{
			Fail("areaflags.map could not be opened");
			return;
		}

		m_FileOpen = true;
		int n = ReadChunk();
		if (n < 24)
		{
			Fail("header too short");
			return;
		}

		int w = BufAt(0);
		int h = BufAt(1);
		int worldX = BufAt(2);
		int worldZ = BufAt(3);
		int bits = BufAt(4);
		int pad = BufAt(5);
		bool sane = w >= 1 && w <= MAX_DIM && h >= 1 && h <= MAX_DIM && worldX > 0 && worldZ > 0 && bits == 32 && pad == 0;
		if (!sane)
		{
			string headerText = w.ToString() + " " + h.ToString() + " " + worldX.ToString() + " " + worldZ.ToString() + " " + bits.ToString() + " " + pad.ToString();
			Fail("unexpected header " + headerText);
			return;
		}

		m_Out.SetHeader(w, h, worldX, worldZ);
		int cellsTotal = w * h;
		m_Expected = 24 + 5 * cellsTotal;
		m_ValueBase = 24 + 4 * cellsTotal;
		m_Out.FirstChunkHash = HashBuffer(m_CurB, n / 4);
		if (n < CHUNK_BYTES_READ)
		{
			m_ShortSeen = true;
		}

		if (m_Revalidate)
		{
			if (!SameHeader(m_Current.HeaderInts) || m_Current.FirstChunkHash != m_Out.FirstChunkHash)
			{
				Fail("areaflags.map header or first chunk changed");
				return;
			}

			m_Cells = m_Current.Cells;
			m_Phase = PH_STREAM;
			return;
		}

		m_Cursor = 0;
		m_Phase = PH_CELLS;
	}

	protected bool SameHeader(array<int> other)
	{
		if (!other || other.Count() != m_Out.HeaderInts.Count())
		{
			return false;
		}

		for (int i = 0; i < other.Count(); i++)
		{
			if (other[i] != m_Out.HeaderInts[i])
			{
				return false;
			}
		}

		return true;
	}

	protected void CellsUnit()
	{
		if (!m_PX || m_Cursor >= m_PX.Count())
		{
			m_Phase = PH_SORT;
			return;
		}

		m_Needed.Insert(m_Out.CellOf(m_PX[m_Cursor], m_PZ[m_Cursor]));
		m_Cursor++;
	}

	protected void DedupeUnit()
	{
		if (m_Cursor >= m_Needed.Count())
		{
			m_Needed = null;
			m_PX = null;
			m_PZ = null;
			m_Phase = PH_CACHE;
			return;
		}

		int cell = m_Needed[m_Cursor];
		m_Cursor++;
		int last = m_Cells.Count() - 1;
		if (last >= 0 && m_Cells[last] == cell)
		{
			return;
		}

		m_Cells.Insert(cell);
	}

	// fast path key: CacheWorld, HeaderInts, FirstChunkHash, GameVersion, PosRev, SpawnRev (one native load)
	protected void CacheUnit()
	{
		m_Cursor = 0;
		m_Cursor2 = 0;
		m_Phase = PH_STREAM;
		if (!FileExist(CACHE_PATH))
		{
			return;
		}

		VPPXEAreaFlagsCache cache = new VPPXEAreaFlagsCache();
		string err;
		if (!JsonFileLoader<VPPXEAreaFlagsCache>.LoadFile(CACHE_PATH, cache, err))
		{
			VPPXELog.Info("[Dist] areaflags cache unreadable, re-sampling: " + err);
			return;
		}

		if (!cache.HeaderInts || !cache.Cells || !cache.Usage || !cache.Value)
		{
			return;
		}

		if (cache.CacheWorld != m_WorldName || cache.FirstChunkHash != m_Out.FirstChunkHash || cache.PosRev != m_PosRev || cache.SpawnRev != m_SpawnRev)
		{
			return;
		}

		if (cache.GameVersion != CurrentGameVersion() || !SameHeader(cache.HeaderInts))
		{
			return;
		}

		if (cache.Usage.Count() != cache.Cells.Count() || cache.Value.Count() != cache.Cells.Count())
		{
			return;
		}

		m_Cache = cache;
		m_Phase = PH_CACHE_CHECK;
	}

	// every needed cell must be in the cached (sorted unique) Cells: one merge step per unit
	protected void CacheCheckUnit()
	{
		if (m_Cursor >= m_Cells.Count())
		{
			CloseStream();
			m_Out.Cells = m_Cache.Cells;
			m_Out.Usage = m_Cache.Usage;
			m_Out.Value = m_Cache.Value;
			m_Out.ByteTotal = m_Cache.ByteTotal;
			m_Out.LastChunkHash = m_Cache.LastChunkHash;
			m_Out.Ok = true;
			m_Out.Reason = "cache";
			m_FromCache = true;
			m_Cache = null;
			m_Cursor = 0;
			m_Phase = PH_INDEX;
			return;
		}

		if (m_Cursor2 >= m_Cache.Cells.Count())
		{
			CacheMiss();
			return;
		}

		int needed = m_Cells[m_Cursor];
		int cached = m_Cache.Cells[m_Cursor2];
		if (cached < needed)
		{
			m_Cursor2++;
			return;
		}

		if (cached > needed)
		{
			CacheMiss();
			return;
		}

		m_Cursor++;
		m_Cursor2++;
	}

	protected void CacheMiss()
	{
		m_Cache = null;
		m_Cursor = 0;
		m_Cursor2 = 0;
		m_Phase = PH_STREAM;
	}

	// sample the current chunk, then read the next one; validation at EOF
	protected void StreamUnit()
	{
		SampleCurrent();
		if (m_Total > m_Expected)
		{
			Fail("areaflags.map is larger than its header says");
			return;
		}

		int n = ReadChunk();
		if (n < 0)
		{
			Fail("read error");
			return;
		}

		if (n == 0)
		{
			FinishStream();
			return;
		}

		if (m_ShortSeen)
		{
			Fail("data after a short read");
			return;
		}

		if (n < CHUNK_BYTES_READ)
		{
			m_ShortSeen = true;
		}
	}

	protected void SampleCurrent()
	{
		int count = m_Cells.Count();
		while (m_UCur < count)
		{
			int cellU = m_Cells[m_UCur];
			int byteU = 24 + 4 * cellU;
			if (byteU + 4 > m_ChunkEnd)
			{
				break;
			}

			int relU = byteU - m_ChunkStart;
			m_OutU.Insert(BufAt(relU / 4));
			m_UCur++;
		}

		if (m_UCur < count)
		{
			return;
		}

		while (m_VCur < count)
		{
			int cellV = m_Cells[m_VCur];
			int byteV = m_ValueBase + cellV;
			if (byteV >= m_ChunkEnd)
			{
				break;
			}

			int relV = byteV - m_ChunkStart;
			int word = BufAt(relV / 4);
			int shift = 8 * (relV % 4);
			m_OutV.Insert((word >> shift) & 255);
			m_VCur++;
		}
	}

	protected void FinishStream()
	{
		int lastHash = 0;
		if (m_LastFull == 1)
		{
			lastHash = HashBuffer(false, CHUNK_INTS);
		}
		else if (m_LastFull == 2)
		{
			lastHash = HashBuffer(true, CHUNK_INTS);
		}

		CloseStream();
		if (m_Total != m_Expected)
		{
			Fail("size " + m_Total.ToString() + " B, expected " + m_Expected.ToString() + " B");
			return;
		}

		if (m_UCur != m_Cells.Count() || m_VCur != m_Cells.Count())
		{
			Fail("not every needed cell was sampled");
			return;
		}

		m_Out.Cells = m_Cells;
		m_Out.Usage = m_OutU;
		m_Out.Value = m_OutV;
		m_Out.ByteTotal = m_Total;
		m_Out.LastChunkHash = lastHash;
		m_Out.Ok = true;
		m_Out.Reason = "streamed";
		m_Cursor = 0;
		if (m_Revalidate)
		{
			if (m_Current.ByteTotal != m_Out.ByteTotal || m_Current.LastChunkHash != m_Out.LastChunkHash || !m_Current.Ok)
			{
				m_Changed = true;
				m_Phase = PH_INDEX;
				return;
			}

			m_Phase = PH_COMPARE;
			return;
		}

		m_Phase = PH_INDEX;
	}

	protected void CompareUnit()
	{
		if (m_Cursor >= m_Out.Cells.Count())
		{
			m_Changed = false;
			int ms = GetGame().GetTime() - m_StartMs;
			m_Result = "revalidated, unchanged (" + m_Reads.ToString() + " reads in " + ms.ToString() + " ms)";
			m_Phase = PH_DONE;
			return;
		}

		bool usageDiffers = m_Current.Usage.Count() <= m_Cursor || m_Current.Usage[m_Cursor] != m_Out.Usage[m_Cursor];
		bool valueDiffers = m_Current.Value.Count() <= m_Cursor || m_Current.Value[m_Cursor] != m_Out.Value[m_Cursor];
		if (usageDiffers || valueDiffers)
		{
			m_Changed = true;
			m_Cursor = 0;
			m_Phase = PH_INDEX;
			return;
		}

		m_Cursor++;
	}

	protected void IndexUnit()
	{
		if (m_Cursor >= m_Out.Cells.Count())
		{
			int ms = GetGame().GetTime() - m_StartMs;
			if (m_FromCache)
			{
				m_Result = "cache hit (" + m_Out.Cells.Count().ToString() + " cells in " + ms.ToString() + " ms)";
				m_Phase = PH_DONE;
				return;
			}

			m_Result = "streamed " + m_Total.ToString() + " B, " + m_Out.Cells.Count().ToString() + " cells sampled (" + m_Reads.ToString() + " reads in " + ms.ToString() + " ms)";
			if (m_Revalidate)
			{
				m_Result = "revalidated, samples replaced; " + m_Result;
			}

			m_Phase = PH_SAVE;
			return;
		}

		m_Out.Index.Insert(m_Out.Cells[m_Cursor], m_Cursor);
		m_Cursor++;
	}

	// one native save; a revalidation keeps the PosRev/SpawnRev key of the MAP data whose cache hit it checks
	protected void SaveUnit()
	{
		m_Phase = PH_DONE;
		VPPXEAreaFlagsCache cache = new VPPXEAreaFlagsCache();
		cache.CacheWorld = m_WorldName;
		cache.HeaderInts = m_Out.HeaderInts;
		cache.FirstChunkHash = m_Out.FirstChunkHash;
		cache.GameVersion = CurrentGameVersion();
		cache.PosRev = m_PosRev;
		cache.SpawnRev = m_SpawnRev;
		cache.ByteTotal = m_Out.ByteTotal;
		cache.LastChunkHash = m_Out.LastChunkHash;
		cache.Cells = m_Out.Cells;
		cache.Usage = m_Out.Usage;
		cache.Value = m_Out.Value;
		string err;
		if (!JsonFileLoader<VPPXEAreaFlagsCache>.SaveFile(CACHE_PATH, cache, err))
		{
			VPPXELog.Info("[Dist] areaflags cache could not be saved: " + err);
		}
	}
};

// Once-per-boot BACKGROUND revalidation of a cache hit: streams the whole file and compares
// ByteTotal, LastChunkHash and the samples; on any mismatch the service swaps the samples.
class VPPXEAreaFlagsRevalidateJob : VPPXEJob
{
	protected VPPXEDistService m_Service;
	protected ref VPPXEAreaSamples m_Checked;
	protected ref VPPXEAreaFlags m_Area;

	void VPPXEAreaFlagsRevalidateJob(VPPXEDistService service, VPPXEAreaSamples samplesToCheck, string worldName, int posRev, int spawnRev)
	{
		m_Service = service;
		m_Checked = samplesToCheck;
		m_Area = new VPPXEAreaFlags(worldName, posRev, spawnRev, true);
		m_Area.BeginRevalidate(samplesToCheck);
	}

	override string GetLabel()
	{
		return "dist areaflags revalidation";
	}

	override bool Step()
	{
		return m_Area.Step();
	}

	override void OnFinished()
	{
		if (m_Service)
		{
			m_Service.OnAreaFlagsRevalidated(m_Checked, m_Area);
		}

		m_Area = null;
	}

	override void OnAborted()
	{
		m_Area = null;
		VPPXELog.Info("[Dist] areaflags revalidation aborted with a script error; the cached samples stay in use");
	}
};
