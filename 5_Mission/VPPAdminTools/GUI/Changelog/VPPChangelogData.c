/*
	Changelog data model, inline markup and per-client state.
	VPPChangelogContent.Build fills a VPPChangelogBuilder; MenuChangelog turns its entries into rows.
*/
enum EVPPChangelogKind
{
	SECTION = 0,
	ADDED = 1,
	CHANGED = 2,
	FIXED = 3,
	REMOVED = 4,
	KNOWN = 5,
	PARAGRAPH = 6,
	BULLET = 7,
	LINK = 8,
	GAP = 9
};

class VPPChangelogEntry : Managed
{
	int Kind;
	string Text;	//version (SECTION), label (LINK) or markup
	string Extra;	//date (SECTION) or url (LINK)

	void VPPChangelogEntry(int kind, string text, string extra)
	{
		Kind = kind;
		Text = text;
		Extra = extra;
	}
};

class VPPChangelogBuilder : Managed
{
	protected ref array<ref VPPChangelogEntry> m_Entries;

	void VPPChangelogBuilder()
	{
		m_Entries = new array<ref VPPChangelogEntry>;
	}

	void Section(string version, string dateText)
	{
		Push(EVPPChangelogKind.SECTION, version, dateText);
	}

	void Added(string text)
	{
		Push(EVPPChangelogKind.ADDED, text, "");
	}

	void Changed(string text)
	{
		Push(EVPPChangelogKind.CHANGED, text, "");
	}

	void Fixed(string text)
	{
		Push(EVPPChangelogKind.FIXED, text, "");
	}

	void Removed(string text)
	{
		Push(EVPPChangelogKind.REMOVED, text, "");
	}

	void Known(string text)
	{
		Push(EVPPChangelogKind.KNOWN, text, "");
	}

	void Paragraph(string text)
	{
		Push(EVPPChangelogKind.PARAGRAPH, text, "");
	}

	void Bullet(string text)
	{
		Push(EVPPChangelogKind.BULLET, text, "");
	}

	void LinkRow(string label, string url)
	{
		Push(EVPPChangelogKind.LINK, label, url);
	}

	void Gap()
	{
		Push(EVPPChangelogKind.GAP, "", "");
	}

	int Count()
	{
		return m_Entries.Count();
	}

	VPPChangelogEntry Get(int index)
	{
		if (index < 0 || index >= m_Entries.Count())
			return null;

		return m_Entries[index];
	}

	protected void Push(int kind, string text, string extra)
	{
		m_Entries.Insert(new VPPChangelogEntry(kind, text, extra));
	}
};

/*
	Inline colour tokens {orange} {blue} {green} {yellow} {red} {muted} {white} ... {/}
	become RichText colour tags (the vanilla NewsCarousel format). No nesting; unclosed spans
	are closed automatically; unknown tokens stay literal.
*/
class VPPChangelogMarkup
{
	const static string RGBA_ORANGE = "232, 163, 61, 255";
	const static string RGBA_BLUE = "96, 158, 235, 255";
	const static string RGBA_GREEN = "102, 196, 106, 255";
	const static string RGBA_YELLOW = "226, 192, 84, 255";
	const static string RGBA_RED = "224, 96, 96, 255";
	const static string RGBA_MUTED = "154, 160, 166, 255";
	const static string RGBA_WHITE = "255, 255, 255, 255";
	const static string RGBA_LINK = "96, 158, 235, 255";
	const static string RGBA_LINK_HOVER = "150, 196, 255, 255";

	//a text that is a stringtable key is translated, anything else is returned unchanged
	static string Localize(string text)
	{
		if (text.IndexOf("#") == 0)
			return Widget.TranslateString(text);

		return text;
	}

	protected static string OpenTag(string rgba)
	{
		return "<color rgba=\"" + rgba + "\">";
	}

	static string Colorize(string text, string rgba)
	{
		return OpenTag(rgba) + text + "</color>";
	}

	//string.Replace edits the string in place and returns the number of replacements
	static string ToRichText(string text)
	{
		string result = Localize(text);
		int opens = 0;
		opens += result.Replace("{orange}", OpenTag(RGBA_ORANGE));
		opens += result.Replace("{blue}", OpenTag(RGBA_BLUE));
		opens += result.Replace("{green}", OpenTag(RGBA_GREEN));
		opens += result.Replace("{yellow}", OpenTag(RGBA_YELLOW));
		opens += result.Replace("{red}", OpenTag(RGBA_RED));
		opens += result.Replace("{muted}", OpenTag(RGBA_MUTED));
		opens += result.Replace("{white}", OpenTag(RGBA_WHITE));
		int closes = result.Replace("{/}", "</color>");
		while (closes < opens)
		{
			result += "</color>";
			closes++;
		}

		return result;
	}

	//"1.6" reads as "v1.6"; a version that does not start with a digit (e.g. "MyMod 1.2") is kept as written
	static string FormatVersion(string version)
	{
		if (version == "")
			return "";

		string first = version.Substring(0, 1);
		string digits = "0123456789";
		if (digits.IndexOf(first) >= 0)
			return "v" + version;

		return version;
	}

	static string FormatSubtitle(string version)
	{
		if (version == "")
			return "";

		return string.Format(Widget.TranslateString("#VSTR_CHANGELOG_SUBTITLE"), FormatVersion(version));
	}

	static string GetChipLabel(int kind)
	{
		switch (kind)
		{
			case EVPPChangelogKind.ADDED:
				return "#VSTR_CHANGELOG_ADDED";
			case EVPPChangelogKind.CHANGED:
				return "#VSTR_CHANGELOG_CHANGED";
			case EVPPChangelogKind.FIXED:
				return "#VSTR_CHANGELOG_FIXED";
			case EVPPChangelogKind.REMOVED:
				return "#VSTR_CHANGELOG_REMOVED";
			case EVPPChangelogKind.KNOWN:
				return "#VSTR_CHANGELOG_KNOWN";
		}

		return "";
	}

	static int GetChipColor(int kind)
	{
		switch (kind)
		{
			case EVPPChangelogKind.ADDED:
				return ARGB(255, 76, 175, 80);
			case EVPPChangelogKind.CHANGED:
				return ARGB(255, 61, 125, 214);
			case EVPPChangelogKind.FIXED:
				return ARGB(255, 232, 163, 61);
			case EVPPChangelogKind.REMOVED:
				return ARGB(255, 194, 69, 69);
			case EVPPChangelogKind.KNOWN:
				return ARGB(255, 217, 178, 61);
		}

		return ARGB(255, 92, 97, 102);
	}

	//the orange and yellow chips get a dark label, the others a white one
	static int GetChipTextColor(int kind)
	{
		if (kind == EVPPChangelogKind.FIXED || kind == EVPPChangelogKind.KNOWN)
			return ARGB(255, 8, 9, 10);

		return ARGB(255, 255, 255, 255);
	}
};

/*
	Per-client changelog state (raw profile strings, the same on every server).
	- changelogVersion in CfgMods AVPPAdminTools is the update number; 0 or missing = never auto-open.
	- vppat_changelog_hide_ver: update number for which Don't show again was ticked ("0" = not ticked).
	- vppat_changelog_seen_ver: last update number the window was shown for (drives the Stats HUD tint).
	Bumping changelogVersion makes both stale, so nothing needs clearing.
*/
class VPPChangelogState
{
	const static string CONFIG_PATH = "CfgMods AVPPAdminTools changelogVersion";
	const static string PROFILE_HIDE_VER = "vppat_changelog_hide_ver";
	const static string PROFILE_SEEN_VER = "vppat_changelog_seen_ver";

	protected static int s_Version = -1;
	protected static string s_SeenVer;
	protected static bool s_SeenLoaded;

	static int GetVersion()
	{
		if (s_Version >= 0)
			return s_Version;

		s_Version = 0;
		if (GetGame().ConfigIsExisting(CONFIG_PATH))
			s_Version = GetGame().ConfigGetInt(CONFIG_PATH);

		if (s_Version < 0)
			s_Version = 0;

		return s_Version;
	}

	static bool IsDontShow()
	{
		int ver = GetVersion();
		if (ver <= 0)
			return false;

		string stored;
		GetGame().GetProfileString(PROFILE_HIDE_VER, stored);
		return stored == ver.ToString();
	}

	static void SetDontShow(bool state)
	{
		string value = "0";
		int ver = GetVersion();
		if (state)
			value = ver.ToString();

		GetGame().SetProfileString(PROFILE_HIDE_VER, value);
		GetGame().SaveProfile();
	}

	static bool ShouldAutoOpen()
	{
		if (GetVersion() <= 0)
			return false;

		return !IsDontShow();
	}

	static bool HasUnseen()
	{
		int ver = GetVersion();
		if (ver <= 0)
			return false;

		LoadSeen();
		return s_SeenVer != ver.ToString();
	}

	static void MarkSeen()
	{
		int ver = GetVersion();
		if (ver <= 0)
			return;

		LoadSeen();
		string current = ver.ToString();
		if (s_SeenVer == current)
			return;

		s_SeenVer = current;
		GetGame().SetProfileString(PROFILE_SEEN_VER, current);
		GetGame().SaveProfile();
	}

	protected static void LoadSeen()
	{
		if (s_SeenLoaded)
			return;

		s_SeenLoaded = true;
		string stored;
		GetGame().GetProfileString(PROFILE_SEEN_VER, stored);
		s_SeenVer = stored;
	}
};
