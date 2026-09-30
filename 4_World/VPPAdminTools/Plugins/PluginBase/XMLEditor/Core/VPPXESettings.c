// XML Editor server settings: $profile:VPPAdminTools/XMLEditor/XMLEditorSettings.json (clamped on load).

class VPPXESettings
{
	int BackupMaxPerFile = 30;
	int BackupMaxAgeDays = 30;
	int BackupMaxTotalMB = 250;
	int CacheIdleMinutes = 15;
	int LiveScanMaxResults = 2000;
	int LiveScanCooldownSec = 10;
	bool EnableAreaFlags = true;
	int JobBudgetMs = 4;
	int JobBoostMs = 8;
	int BackgroundSharePct = 25;
	int NetChunksPer100ms = 3;
	bool PrewarmTypes = true;

	const static string SETTINGS_PATH = "$profile:VPPAdminTools/XMLEditor/XMLEditorSettings.json";

	static VPPXESettings LoadOrCreate()
	{
		VPPXESettings loaded = new VPPXESettings();
		string err;
		if (FileExist(SETTINGS_PATH))
		{
			if (!JsonFileLoader<VPPXESettings>.LoadFile(SETTINGS_PATH, loaded, err))
			{
				VPPXELog.Warn("XMLEditorSettings.json could not be loaded, defaults are used (backup retention kept at its most permissive until the file loads): " + err);
				loaded = new VPPXESettings();
				loaded.UseSafeRetention();
			}
		}
		else
		{
			if (!JsonFileLoader<VPPXESettings>.SaveFile(SETTINGS_PATH, loaded, err))
			{
				VPPXELog.Warn("XMLEditorSettings.json could not be written: " + err);
			}
		}

		if (!loaded)
		{
			loaded = new VPPXESettings();
		}

		loaded.ClampValues();
		return loaded;
	}

	// An unreadable settings file must never shrink retention below what the owner may have configured:
	// use the clamp maxima (500 per file, no age limit, 10000 MB) so retention deletes nothing a valid file would keep.
	void UseSafeRetention()
	{
		BackupMaxPerFile = 500;
		BackupMaxAgeDays = 0;
		BackupMaxTotalMB = 10000;
	}

	void ClampValues()
	{
		BackupMaxPerFile = ClampInt(BackupMaxPerFile, 1, 500);
		BackupMaxAgeDays = ClampInt(BackupMaxAgeDays, 0, 3650);
		BackupMaxTotalMB = ClampInt(BackupMaxTotalMB, 10, 10000);
		CacheIdleMinutes = ClampInt(CacheIdleMinutes, 1, 240);
		LiveScanMaxResults = ClampInt(LiveScanMaxResults, 100, 20000);
		LiveScanCooldownSec = ClampInt(LiveScanCooldownSec, 0, 600);
		JobBudgetMs = ClampInt(JobBudgetMs, 1, 20);
		JobBoostMs = ClampInt(JobBoostMs, 1, 20);
		BackgroundSharePct = ClampInt(BackgroundSharePct, 10, 90);
		NetChunksPer100ms = ClampInt(NetChunksPer100ms, 1, 20);
	}

	static int ClampInt(int v, int lo, int hi)
	{
		if (v < lo)
		{
			return lo;
		}

		if (v > hi)
		{
			return hi;
		}

		return v;
	}
};
