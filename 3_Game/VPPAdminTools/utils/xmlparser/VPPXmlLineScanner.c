// Fast per-line attribute extraction for flat map files (mapgrouppos, mapgroupcluster*), read line by line with
// VPPXmlLineStream. No tokenizer, no DOM: one IndexOfFrom per candidate plus a few character checks.

class VPPXmlLineScanner
{
	// Finds attrName followed by optional spaces, '=', optional spaces and a quote (either style), where the char before
	// attrName is a space, tab or '<' (or attrName starts the line); value = the raw text up to the matching quote.
	static bool Attr(string line, string attrName, out string value)
	{
		value = "";
		int total = line.Length();
		int nameLen = attrName.Length();
		if (nameLen == 0)
		{
			return false;
		}

		int from = 0;
		while (from < total)
		{
			int at = line.IndexOfFrom(from, attrName);
			if (at < 0)
			{
				return false;
			}

			from = at + 1;
			if (at > 0)
			{
				string before = line.Get(at - 1);
				if (before != " " && before != "\t" && before != "<")
				{
					continue;
				}
			}

			int pos = SkipBlanks(line, at + nameLen, total);
			if (pos >= total)
			{
				return false;
			}

			string eqCh = line.Get(pos);
			if (eqCh != "=")
			{
				continue;
			}

			pos = SkipBlanks(line, pos + 1, total);
			if (pos >= total)
			{
				return false;
			}

			string quoteCh = line.Get(pos);
			if (quoteCh != "\"" && quoteCh != "'")
			{
				continue;
			}

			int closeAt = -1;
			if (pos + 1 < total)
			{
				closeAt = line.IndexOfFrom(pos + 1, quoteCh);
			}

			if (closeAt < 0)
			{
				return false;
			}

			if (closeAt > pos + 1)
			{
				value = line.Substring(pos + 1, closeAt - pos - 1);
			}

			return true;
		}

		return false;
	}

	// "x y z" -> x and the third number; "x z" -> x and the second. Whitespace-separated, empty tokens skipped.
	static bool XZ(string posValue, out float x, out float z)
	{
		x = 0;
		z = 0;
		string norm = posValue;
		norm.Replace("\t", " ");
		array<string> parts = new array<string>;
		norm.Split(" ", parts);
		int count = parts.Count();
		if (count < 2)
		{
			return false;
		}

		string xText = parts[0];
		string zText = parts[1];
		if (count >= 3)
		{
			zText = parts[2];
		}

		x = xText.ToFloat();
		z = zText.ToFloat();
		return true;
	}

	// First index at or after pos that is not a space or tab.
	static int SkipBlanks(string line, int pos, int total)
	{
		int cursor = pos;
		while (cursor < total)
		{
			string ch = line.Get(cursor);
			if (ch != " " && ch != "\t")
			{
				break;
			}

			cursor++;
		}

		return cursor;
	}
};
