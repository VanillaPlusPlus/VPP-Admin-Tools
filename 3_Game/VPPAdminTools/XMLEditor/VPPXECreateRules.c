// XML Editor CREATE tab: the rules for a new custom CE file (types or messages), shared by the client form (live
// validation) and the server job (authoritative). Folder: 1 to 3 segments of [A-Za-z0-9_-] (48 chars each) separated by "/".
// File: [A-Za-z0-9_.-] (64 chars, not starting with a dot, no ".."); ".xml" is appended when missing.

class VPPXECreateRules
{
	const static int MAX_FOLDER_DEPTH = 3;
	const static int MAX_SEGMENT_CHARS = 48;
	const static int MAX_FILE_CHARS = 64;
	const static int MAX_INPUT_CHARS = 256;
	const static string FOLDER_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
	const static string FILE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-";

	// Backslashes to "/", surrounding blanks and slashes dropped, repeated slashes collapsed.
	static string NormalizeFolder(string raw)
	{
		if (raw.Length() > MAX_INPUT_CHARS)
		{
			return raw;
		}

		string folder = raw;
		folder.Replace("\\", "/");
		folder = folder.Trim();
		int guard = 0;
		while (folder.IndexOf("//") >= 0 && guard < 64)
		{
			folder.Replace("//", "/");
			guard++;
		}

		while (folder.Length() > 0 && folder.Get(0) == "/")
		{
			folder = folder.Substring(1, folder.Length() - 1);
		}

		while (folder.Length() > 0 && folder.Get(folder.Length() - 1) == "/")
		{
			folder = folder.Substring(0, folder.Length() - 1);
		}

		return folder;
	}

	// Trimmed, with ".xml" appended unless it already ends with it (any case).
	static string NormalizeFileName(string raw)
	{
		if (raw.Length() > MAX_INPUT_CHARS)
		{
			return raw;
		}

		string fileName = raw.Trim();
		if (fileName == "")
		{
			return "";
		}

		string lower = fileName;
		lower.ToLower();
		int len = lower.Length();
		bool hasExt = false;
		if (len >= 4)
		{
			if (lower.Substring(len - 4, 4) == ".xml")
			{
				hasExt = true;
			}
		}

		if (!hasExt)
		{
			fileName = fileName + ".xml";
		}

		return fileName;
	}

	// "" when valid, else the #key of the problem (for a normalized folder).
	static string CheckFolder(string folder)
	{
		if (folder == "")
		{
			return "#VSTR_XMLE_CR_ERR_FOLDER_EMPTY";
		}

		if (folder.Length() > MAX_INPUT_CHARS)
		{
			return "#VSTR_XMLE_CR_ERR_FOLDER";
		}

		array<string> segments = new array<string>();
		folder.Split("/", segments);
		if (segments.Count() == 0 || segments.Count() > MAX_FOLDER_DEPTH)
		{
			return "#VSTR_XMLE_CR_ERR_FOLDER";
		}

		foreach (string segment : segments)
		{
			if (!ValidChars(segment, FOLDER_CHARS, MAX_SEGMENT_CHARS))
			{
				return "#VSTR_XMLE_CR_ERR_FOLDER";
			}
		}

		return "";
	}

	// "" when valid, else the #key of the problem (for a normalized file name).
	static string CheckFileName(string fileName)
	{
		if (fileName == "")
		{
			return "#VSTR_XMLE_CR_ERR_FILE_EMPTY";
		}

		if (fileName.Get(0) == "." || fileName.IndexOf("..") >= 0)
		{
			return "#VSTR_XMLE_CR_ERR_FILE";
		}

		if (!ValidChars(fileName, FILE_CHARS, MAX_FILE_CHARS))
		{
			return "#VSTR_XMLE_CR_ERR_FILE";
		}

		return "";
	}

	// Mission-relative key of the new file (the registry key of a <ce folder><file name> entry).
	static string MakeKey(string folder, string fileName)
	{
		return folder + "/" + fileName;
	}

	// Kinds the tab can create.
	static bool IsCreatableKind(int kind)
	{
		if (kind == VPPXEFileKind.SPAWNABLETYPES || kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return true;
		}

		return kind == VPPXEFileKind.TYPES || kind == VPPXEFileKind.MESSAGES || kind == VPPXEFileKind.EVENTS;
	}

	// The <ce> type attribute and the root element of a creatable kind.
	static string CeTypeOf(int kind)
	{
		if (kind == VPPXEFileKind.MESSAGES)
		{
			return "messages";
		}

		if (kind == VPPXEFileKind.EVENTS)
		{
			return "events";
		}

		if (kind == VPPXEFileKind.SPAWNABLETYPES)
		{
			return "spawnabletypes";
		}

		if (kind == VPPXEFileKind.RANDOMPRESETS)
		{
			return "randompresets";
		}

		return "types";
	}

	// The <file> element added to the <ce> block of the folder.
	static string FileTag(string fileName, int kind)
	{
		return "<file name=\"" + fileName + "\" type=\"" + CeTypeOf(kind) + "\" />";
	}

	static string CeOpenTag(string folder)
	{
		return "<ce folder=\"" + folder + "\">";
	}

	// Folder part of a mission key ("" for a root file).
	static string FolderOfKey(string key)
	{
		int slash = key.LastIndexOf("/");
		if (slash <= 0)
		{
			return "";
		}

		return key.Substring(0, slash);
	}

	// File part of a mission key.
	static string FileOfKey(string key)
	{
		int slash = key.LastIndexOf("/");
		if (slash < 0)
		{
			return key;
		}

		return key.Substring(slash + 1, key.Length() - slash - 1);
	}

	protected static bool ValidChars(string text, string allowed, int maxChars)
	{
		int len = text.Length();
		if (len == 0 || len > maxChars)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			if (allowed.IndexOf(text.Get(i)) < 0)
			{
				return false;
			}
		}

		return true;
	}
};
