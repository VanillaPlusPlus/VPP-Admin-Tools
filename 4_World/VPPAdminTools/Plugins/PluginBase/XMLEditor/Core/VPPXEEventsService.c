// XML Editor EVENTS tab, server side: reads every EVENTS file of the registry (db/events.xml and the
// <ce type="events"> files) plus cfgeventspawns.xml for XE_GetEvents, and applies XE_SaveEvents edits (UPDATE, ADD,
// DELETE of whole <event> elements) as line patches: untouched events, comments and formatting stay byte for byte; an
// edited event is written again in the vanilla element order with the file's indentation, keeping every element,
// attribute and comment the editor does not know. Both jobs parse with the job budget (events files of modded maps
// can be large). Rules: VPPXEEventRules (3_Game VPPXEEvents.c).

class VPPXEEventsIO
{
	// The <events> root of a parsed document, or null. No out parameters, locals at the top (see VPPXEMessagesIO).
	static VPPXmlNode RootOf(VPPXmlDocument doc)
	{
		VPPXmlNode root = null;
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, "events");
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, "events"))
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

		return "#VSTR_XMLE_EVT_ERR_ROOT";
	}

	// The <event> element children of the root, in document order.
	static void EventNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (kid && kid.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(kid.Name, "event"))
			{
				outNodes.Insert(kid);
			}
		}
	}

	// Elements of an <event> this editor reads and writes.
	static bool IsKnownElement(string name)
	{
		string lower = name;
		lower.ToLower();
		if (lower == "nominal" || lower == "min" || lower == "max" || lower == "lifetime" || lower == "restock")
		{
			return true;
		}

		if (lower == "saferadius" || lower == "distanceradius" || lower == "cleanupradius" || lower == "secondary")
		{
			return true;
		}

		return lower == "flags" || lower == "position" || lower == "limit" || lower == "active" || lower == "children";
	}

	// A whole number the way the server reads it (strtol: leading digits count, anything unreadable = left out).
	// IsIntText decides whether it is readable, IntOf reads it (no out parameters: see VPPXEMessagesIO.ParseFile).
	static bool IsIntText(string text)
	{
		string trimmed = VPPXmlText.TrimWs(text);
		string first = "";
		if (trimmed == "")
		{
			return false;
		}

		first = trimmed.Get(0);
		if (VPPXEEventRules.DIGITS.IndexOf(first) >= 0)
		{
			return true;
		}

		if (first != "-" && first != "+")
		{
			return false;
		}

		return trimmed.Length() >= 2 && VPPXEEventRules.DIGITS.IndexOf(trimmed.Get(1)) >= 0;
	}

	static int IntOf(string text)
	{
		string trimmed = VPPXmlText.TrimWs(text);
		return trimmed.ToInt();
	}

	// The row of an <event> node the way the server reads it.
	static void RowOf(VPPXmlNode node, VPPXEEventRow row)
	{
		row.Line = node.StartLine + 1;
		row.Name = node.GetAttr("name", "");
		row.EventExtra = "";
		row.Present = 0;
		int attrCount = node.AttrCount();
		for (int a = 0; a < attrCount; a++)
		{
			string attrName = node.AttrNames[a];
			if (!VPPXmlText.EqualsNoCase(attrName, "name"))
			{
				row.EventExtra = row.EventExtra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(node.GetAttr(attrName, "")) + "\"";
			}
		}

		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			VPPXmlNode numberNode = node.FirstChild(VPPXEEventRules.FieldTag(field));
			if (numberNode && IsIntText(numberNode.InnerText()))
			{
				row.SetNumber(field, IntOf(numberNode.InnerText()));
				row.SetPresent(field, true);
			}
		}

		VPPXmlNode secondaryNode = node.FirstChild("secondary");
		if (secondaryNode && secondaryNode.InnerText() != "")
		{
			row.Secondary = secondaryNode.InnerText();
			row.SetPresent(VPPXEEventField.SECONDARY, true);
		}

		VPPXmlNode flagsNode = node.FirstChild("flags");
		if (flagsNode)
		{
			row.SetPresent(VPPXEEventField.FLAGS, true);
			ReadFlags(flagsNode, row);
		}

		VPPXmlNode positionNode = node.FirstChild("position");
		if (positionNode)
		{
			string position = positionNode.InnerText();
			position.ToLower();
			row.Position = position;
			row.SetPresent(VPPXEEventField.POSITION, true);
		}

		VPPXmlNode limitNode = node.FirstChild("limit");
		if (limitNode)
		{
			string limit = limitNode.InnerText();
			limit.ToLower();
			row.Limit = limit;
			row.SetPresent(VPPXEEventField.LIMIT, true);
		}

		VPPXmlNode activeNode = node.FirstChild("active");
		if (activeNode && IsIntText(activeNode.InnerText()))
		{
			row.Active = IntOf(activeNode.InnerText());
			row.SetPresent(VPPXEEventField.ACTIVE, true);
		}

		VPPXmlNode childrenNode = node.FirstChild("children");
		if (childrenNode)
		{
			row.SetPresent(VPPXEEventField.CHILDREN, true);
			ReadChildren(childrenNode, row);
		}

		row.ExtraCount = 0;
		int kids = node.ChildCount();
		for (int k = 0; k < kids; k++)
		{
			VPPXmlNode kid = node.ChildAt(k);
			if (kid && (kid.Kind != VPPXmlNodeKind.ELEMENT || !IsKnownElement(kid.Name)))
			{
				row.ExtraCount = row.ExtraCount + 1;
			}
		}
	}

	protected static void ReadFlags(VPPXmlNode flagsNode, VPPXEEventRow row)
	{
		row.Flags = 0;
		row.FlagsExtra = "";
		int attrCount = flagsNode.AttrCount();
		for (int i = 0; i < attrCount; i++)
		{
			string attrName = flagsNode.AttrNames[i];
			string attrValue = flagsNode.GetAttr(attrName, "");
			string lowerName = attrName;
			lowerName.ToLower();
			int bit = 0;
			if (lowerName == "deletable")
			{
				bit = VPPXEEventFlag.DELETABLE;
			}
			else if (lowerName == "init_random")
			{
				bit = VPPXEEventFlag.INIT_RANDOM;
			}
			else if (lowerName == "remove_damaged")
			{
				bit = VPPXEEventFlag.REMOVE_DAMAGED;
			}

			if (bit == 0)
			{
				row.FlagsExtra = row.FlagsExtra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
				continue;
			}

			if (IsIntText(attrValue) && IntOf(attrValue) == 1)
			{
				row.Flags = row.Flags | bit;
			}
		}
	}

	protected static void ReadChildren(VPPXmlNode childrenNode, VPPXEEventRow row)
	{
		int kids = childrenNode.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = childrenNode.ChildAt(i);
			if (!kid || kid.Kind != VPPXmlNodeKind.ELEMENT || !VPPXmlText.EqualsNoCase(kid.Name, "child"))
			{
				continue;
			}

			VPPXEEventChild child = new VPPXEEventChild();
			ReadChild(kid, child);
			row.Children.Insert(child);
		}
	}

	protected static void ReadChild(VPPXmlNode node, VPPXEEventChild child)
	{
		child.Line = node.StartLine + 1;
		int attrCount = node.AttrCount();
		for (int i = 0; i < attrCount; i++)
		{
			string attrName = node.AttrNames[i];
			string attrValue = node.GetAttr(attrName, "");
			string lowerName = attrName;
			lowerName.ToLower();
			bool known = true;
			if (lowerName == "type")
			{
				child.Type = VPPXmlText.TrimWs(attrValue);
			}
			else if (lowerName == "min" || lowerName == "max" || lowerName == "lootmin" || lowerName == "lootmax" || lowerName == "deloot")
			{
				// unreadable = the server default (0, deloot -1); the attribute is written again as a number
				if (IsIntText(attrValue))
				{
					SetChildNumber(child, lowerName, IntOf(attrValue));
				}
			}
			else if (lowerName == "spawnsecondary" || lowerName == "trace")
			{
				int boolValue = ReadBool(attrValue);
				known = boolValue >= 0;
				if (known && lowerName == "spawnsecondary")
				{
					child.SpawnSecondary = boolValue;
				}
				else if (known)
				{
					child.Trace = boolValue;
				}
			}
			else
			{
				known = false;
			}

			if (!known)
			{
				child.ExtraAttrs = child.ExtraAttrs + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
			}
		}
	}

	protected static void SetChildNumber(VPPXEEventChild child, string lowerName, int value)
	{
		if (lowerName == "min")
		{
			child.Min = value;
		}
		else if (lowerName == "max")
		{
			child.Max = value;
		}
		else if (lowerName == "lootmin")
		{
			child.LootMin = value;
		}
		else if (lowerName == "lootmax")
		{
			child.LootMax = value;
		}
		else
		{
			child.Deloot = value;
		}
	}

	// 1 / 0 for the server's true and false words, -1 for anything else.
	protected static int ReadBool(string text)
	{
		string lower = VPPXmlText.TrimWs(text);
		lower.ToLower();
		if (lower == "1" || lower == "true" || lower == "yes" || lower == "enabled" || lower == "on")
		{
			return 1;
		}

		if (lower == "0" || lower == "false" || lower == "no" || lower == "disabled" || lower == "off")
		{
			return 0;
		}

		return -1;
	}

	// The lines of a node exactly as in the file (the first starts at the node, the last ends with it).
	static void NodeLines(VPPXmlDocument doc, VPPXmlNode node, array<string> outLines)
	{
		outLines.Clear();
		if (!node || node.StartLine < 0 || node.EndLine < node.StartLine || node.EndLine >= doc.LineCount())
		{
			return;
		}

		string firstText = doc.GetLine(node.StartLine);
		if (node.StartLine == node.EndLine)
		{
			outLines.Insert(VPPXETypesWriter.Mid(firstText, node.StartCol, node.EndCol - node.StartCol));
			return;
		}

		outLines.Insert(VPPXETypesWriter.Mid(firstText, node.StartCol, firstText.Length() - node.StartCol));
		for (int i = node.StartLine + 1; i < node.EndLine; i++)
		{
			outLines.Insert(doc.GetLine(i));
		}

		string lastText = doc.GetLine(node.EndLine);
		outLines.Insert(VPPXETypesWriter.Mid(lastText, 0, node.EndCol));
	}

	// Indentation of an element's closing tag as it is in the file (fallback when orig is null or the tag shares its line).
	static string CloseIndentOf(VPPXmlDocument doc, VPPXmlNode orig, string fallback)
	{
		if (!orig || !doc || orig.CloseStartLine < 0 || orig.CloseStartLine >= doc.LineCount())
		{
			return fallback;
		}

		string line = doc.GetLine(orig.CloseStartLine);
		string before = VPPXETypesWriter.Mid(line, 0, orig.CloseStartCol);
		if (!VPPXmlText.IsBlank(before))
		{
			return fallback;
		}

		return before;
	}

	// One step of indentation below indent, taken from childIndent when it extends indent.
	static string StepOf(string indent, string childIndent)
	{
		int indentLen = indent.Length();
		int childLen = childIndent.Length();
		if (childLen > indentLen && childIndent.IndexOf(indent) == 0)
		{
			return VPPXETypesWriter.Mid(childIndent, indentLen, childLen - indentLen);
		}

		return VPPXEMessagesIO.IndentUnit(indent);
	}

	// Indentation of <child> lines: the first child of the node's <children> that starts a line, else one step more.
	static string GrandIndentOf(array<string> lines, VPPXmlNode node, string indent, string childIndent)
	{
		VPPXmlNode childrenNode = null;
		if (node)
		{
			childrenNode = node.FirstChild("children");
		}

		if (childrenNode)
		{
			int kids = childrenNode.ChildCount();
			for (int i = 0; i < kids; i++)
			{
				VPPXmlNode kid = childrenNode.ChildAt(i);
				if (kid && kid.Kind == VPPXmlNodeKind.ELEMENT && VPPXEMessagesIO.StartsLine(lines, kid))
				{
					return VPPXmlText.LeadingWs(lines[kid.StartLine]);
				}
			}
		}

		return childIndent + StepOf(indent, childIndent);
	}

	// An <event> block. orig (may be null) is the element it replaces: its unknown elements, comments and the
	// comments inside its <children> are copied as they are. The first line carries no indentation when firstBare.
	static void BuildBlock(VPPXEEventRow row, VPPXmlDocument doc, VPPXmlNode orig, string indent, string childIndent, string grandIndent, bool firstBare, array<string> outLines)
	{
		string head = "<event name=\"" + VPPXmlText.EncodeAttr(row.Name) + "\"" + row.EventExtra + ">";
		array<string> kept = new array<string>();
		outLines.Clear();
		if (firstBare)
		{
			outLines.Insert(head);
		}
		else
		{
			outLines.Insert(indent + head);
		}

		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			if (row.Has(field))
			{
				string tag = VPPXEEventRules.FieldTag(field);
				int number = row.GetNumber(field);
				outLines.Insert(childIndent + "<" + tag + ">" + number.ToString() + "</" + tag + ">");
			}
		}

		if (row.Has(VPPXEEventField.SECONDARY))
		{
			outLines.Insert(childIndent + "<secondary>" + VPPXEMessagesIO.EncodeValue(row.Secondary) + "</secondary>");
		}

		if (row.Has(VPPXEEventField.FLAGS))
		{
			outLines.Insert(childIndent + FlagsTag(row));
		}

		if (row.Has(VPPXEEventField.POSITION))
		{
			outLines.Insert(childIndent + "<position>" + VPPXEMessagesIO.EncodeValue(row.Position) + "</position>");
		}

		if (row.Has(VPPXEEventField.LIMIT))
		{
			outLines.Insert(childIndent + "<limit>" + VPPXEMessagesIO.EncodeValue(row.Limit) + "</limit>");
		}

		if (row.Has(VPPXEEventField.ACTIVE))
		{
			outLines.Insert(childIndent + "<active>" + row.Active.ToString() + "</active>");
		}

		if (orig && doc)
		{
			int kids = orig.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = orig.ChildAt(k);
				if (!kid || (kid.Kind == VPPXmlNodeKind.ELEMENT && IsKnownElement(kid.Name)))
				{
					continue;
				}

				AppendKept(doc, kid, childIndent, outLines);
			}
		}

		if (row.Has(VPPXEEventField.CHILDREN) || row.Children.Count() > 0)
		{
			AppendChildren(row, doc, orig, childIndent, grandIndent, outLines);
		}

		outLines.Insert(VPPXEEventsIO.CloseIndentOf(doc, orig, indent) + "</event>");
	}

	protected static string FlagsTag(VPPXEEventRow row)
	{
		string tag = "<flags deletable=\"" + FlagText(row, VPPXEEventFlag.DELETABLE) + "\"";
		tag = tag + " init_random=\"" + FlagText(row, VPPXEEventFlag.INIT_RANDOM) + "\"";
		tag = tag + " remove_damaged=\"" + FlagText(row, VPPXEEventFlag.REMOVE_DAMAGED) + "\"";
		return tag + row.FlagsExtra + "/>";
	}

	protected static string FlagText(VPPXEEventRow row, int bit)
	{
		if ((row.Flags & bit) != 0)
		{
			return "1";
		}

		return "0";
	}

	static void AppendKept(VPPXmlDocument doc, VPPXmlNode kid, string indent, array<string> outLines)
	{
		array<string> kidLines = new array<string>();
		NodeLines(doc, kid, kidLines);
		int count = kidLines.Count();
		for (int i = 0; i < count; i++)
		{
			if (i == 0)
			{
				outLines.Insert(indent + kidLines[i]);
			}
			else
			{
				outLines.Insert(kidLines[i]);
			}
		}
	}

	protected static void AppendChildren(VPPXEEventRow row, VPPXmlDocument doc, VPPXmlNode orig, string childIndent, string grandIndent, array<string> outLines)
	{
		VPPXmlNode origChildren = null;
		int comments = 0;
		if (orig)
		{
			origChildren = orig.FirstChild("children");
		}

		if (origChildren && doc)
		{
			int origKids = origChildren.ChildCount();
			for (int c = 0; c < origKids; c++)
			{
				VPPXmlNode origKid = origChildren.ChildAt(c);
				if (origKid && !IsChildElement(origKid))
				{
					comments++;
				}
			}
		}

		if (row.Children.Count() == 0 && comments == 0)
		{
			outLines.Insert(childIndent + "<children/>");
			return;
		}

		outLines.Insert(childIndent + "<children>");
		foreach (VPPXEEventChild child : row.Children)
		{
			outLines.Insert(grandIndent + ChildTag(child));
		}

		if (comments > 0)
		{
			int kids = origChildren.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = origChildren.ChildAt(k);
				if (kid && !IsChildElement(kid))
				{
					AppendKept(doc, kid, grandIndent, outLines);
				}
			}
		}

		outLines.Insert(childIndent + "</children>");
	}

	protected static bool IsChildElement(VPPXmlNode node)
	{
		return node.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(node.Name, "child");
	}

	// <child .../> with the attributes in alphabetical order (vanilla: lootmax lootmin max min type).
	static string ChildTag(VPPXEEventChild child)
	{
		string tag = "<child";
		if (child.Deloot != -1)
		{
			tag = tag + " deloot=\"" + child.Deloot.ToString() + "\"";
		}

		tag = tag + " lootmax=\"" + child.LootMax.ToString() + "\" lootmin=\"" + child.LootMin.ToString() + "\"";
		tag = tag + " max=\"" + child.Max.ToString() + "\" min=\"" + child.Min.ToString() + "\"";
		if (child.SpawnSecondary == 1)
		{
			tag = tag + " spawnsecondary=\"true\"";
		}
		else if (child.SpawnSecondary == 0)
		{
			tag = tag + " spawnsecondary=\"false\"";
		}

		if (child.Trace == 1)
		{
			tag = tag + " trace=\"true\"";
		}
		else if (child.Trace == 0)
		{
			tag = tag + " trace=\"false\"";
		}

		tag = tag + " type=\"" + VPPXmlText.EncodeAttr(child.Type) + "\"" + child.ExtraAttrs + "/>";
		return tag;
	}
};

// cfgeventspawns.xml: <eventposdef> of <event name> entries with a <zone> and <pos> elements. An entry is written
// again when it changed; inside it, the zone and every position whose values did not change are copied from their
// original line (numbers, spacing, attribute order), so only what was edited gets new text.
class VPPXESpawnsIO
{
	// The <eventposdef> root of a parsed document, or null.
	static VPPXmlNode RootOf(VPPXmlDocument doc)
	{
		VPPXmlNode root = null;
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, "eventposdef");
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, "eventposdef"))
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

		return "#VSTR_XMLE_SPW_ERR_ROOT";
	}

	static bool IsPosElement(VPPXmlNode node)
	{
		return node && node.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(node.Name, "pos");
	}

	static bool IsZoneElement(VPPXmlNode node)
	{
		return node && node.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(node.Name, "zone");
	}

	// The <zone> the server ends up with: the last one.
	static VPPXmlNode ZoneNode(VPPXmlNode eventNode)
	{
		VPPXmlNode found = null;
		int kids = eventNode.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = eventNode.ChildAt(i);
			if (IsZoneElement(kid))
			{
				found = kid;
			}
		}

		return found;
	}

	static void PosNodes(VPPXmlNode eventNode, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = eventNode.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = eventNode.ChildAt(i);
			if (IsPosElement(kid))
			{
				outNodes.Insert(kid);
			}
		}
	}

	static void RowOf(VPPXmlNode node, VPPXESpawnRow row)
	{
		array<VPPXmlNode> posNodes = new array<VPPXmlNode>();
		VPPXmlNode zoneNode = ZoneNode(node);
		row.Line = node.StartLine + 1;
		row.Name = node.GetAttr("name", "");
		row.NameExtra = "";
		int attrCount = node.AttrCount();
		for (int a = 0; a < attrCount; a++)
		{
			string attrName = node.AttrNames[a];
			if (!VPPXmlText.EqualsNoCase(attrName, "name"))
			{
				row.NameExtra = row.NameExtra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(node.GetAttr(attrName, "")) + "\"";
			}
		}

		row.HasZone = zoneNode != null;
		if (zoneNode)
		{
			ReadZone(zoneNode, row);
		}

		PosNodes(node, posNodes);
		for (int p = 0; p < posNodes.Count(); p++)
		{
			VPPXESpawnPos pos = new VPPXESpawnPos();
			ReadPos(posNodes[p], pos);
			pos.OrigIdx = p;
			row.Positions.Insert(pos);
		}

		row.ExtraCount = 0;
		int kids = node.ChildCount();
		for (int k = 0; k < kids; k++)
		{
			VPPXmlNode kid = node.ChildAt(k);
			if (kid && !IsPosElement(kid) && !IsZoneElement(kid))
			{
				row.ExtraCount = row.ExtraCount + 1;
			}
		}
	}

	static void ReadPos(VPPXmlNode node, VPPXESpawnPos pos)
	{
		int attrCount = node.AttrCount();
		for (int i = 0; i < attrCount; i++)
		{
			string attrName = node.AttrNames[i];
			string attrValue = node.GetAttr(attrName, "");
			string lowerName = attrName;
			lowerName.ToLower();
			if (lowerName == "x")
			{
				pos.X = VPPXmlText.TrimWs(attrValue);
			}
			else if (lowerName == "z")
			{
				pos.Z = VPPXmlText.TrimWs(attrValue);
			}
			else if (lowerName == "a")
			{
				pos.A = VPPXmlText.TrimWs(attrValue);
			}
			else if (lowerName == "y")
			{
				pos.Y = VPPXmlText.TrimWs(attrValue);
			}
			else if (lowerName == "group")
			{
				pos.Group = VPPXmlText.TrimWs(attrValue);
			}
			else
			{
				pos.Extra = pos.Extra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
			}
		}
	}

	static void ReadZone(VPPXmlNode node, VPPXESpawnRow row)
	{
		row.ZoneExtra = "";
		int attrCount = node.AttrCount();
		for (int i = 0; i < attrCount; i++)
		{
			string attrName = node.AttrNames[i];
			string attrValue = VPPXmlText.TrimWs(node.GetAttr(attrName, ""));
			string lowerName = attrName;
			lowerName.ToLower();
			if (lowerName == "smin")
			{
				row.SMin = attrValue;
			}
			else if (lowerName == "smax")
			{
				row.SMax = attrValue;
			}
			else if (lowerName == "dmin")
			{
				row.DMin = attrValue;
			}
			else if (lowerName == "dmax")
			{
				row.DMax = attrValue;
			}
			else if (lowerName == "r")
			{
				row.R = attrValue;
			}
			else
			{
				row.ZoneExtra = row.ZoneExtra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
			}
		}
	}

	static string PosTag(VPPXESpawnPos pos)
	{
		string tag = "<pos x=\"" + VPPXmlText.EncodeAttr(pos.X) + "\" z=\"" + VPPXmlText.EncodeAttr(pos.Z) + "\"";
		if (pos.A != "")
		{
			tag = tag + " a=\"" + VPPXmlText.EncodeAttr(pos.A) + "\"";
		}

		if (pos.Y != "")
		{
			tag = tag + " y=\"" + VPPXmlText.EncodeAttr(pos.Y) + "\"";
		}

		if (pos.Group != "")
		{
			tag = tag + " group=\"" + VPPXmlText.EncodeAttr(pos.Group) + "\"";
		}

		return tag + pos.Extra + " />";
	}

	static string ZoneTag(VPPXESpawnRow row)
	{
		string tag = "<zone smin=\"" + VPPXmlText.EncodeAttr(row.SMin) + "\" smax=\"" + VPPXmlText.EncodeAttr(row.SMax) + "\"";
		tag = tag + " dmin=\"" + VPPXmlText.EncodeAttr(row.DMin) + "\" dmax=\"" + VPPXmlText.EncodeAttr(row.DMax) + "\"";
		tag = tag + " r=\"" + VPPXmlText.EncodeAttr(row.R) + "\"" + row.ZoneExtra + " />";
		return tag;
	}

	// A one-line child exactly as in the file, with its own indentation when it starts its line.
	static void AppendOriginal(VPPXmlDocument doc, array<string> lines, VPPXmlNode node, string indent, array<string> outLines)
	{
		if (node.StartLine == node.EndLine && VPPXEMessagesIO.StartsLine(lines, node))
		{
			string lineText = lines[node.StartLine];
			string lead = VPPXmlText.LeadingWs(lineText);
			outLines.Insert(lead + VPPXETypesWriter.Mid(lineText, node.StartCol, node.EndCol - node.StartCol));
			return;
		}

		VPPXEEventsIO.AppendKept(doc, node, indent, outLines);
	}

	// An <event> entry; orig (may be null) is the element it replaces.
	static void BuildBlock(VPPXESpawnRow row, VPPXmlDocument doc, VPPXmlNode orig, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		string head = "<event name=\"" + VPPXmlText.EncodeAttr(row.Name) + "\"" + row.NameExtra;
		array<VPPXmlNode> origPos = new array<VPPXmlNode>();
		array<string> lines = null;
		VPPXmlNode origZone = null;
		int kept = 0;
		outLines.Clear();
		if (orig && doc)
		{
			lines = doc.GetLines();
			PosNodes(orig, origPos);
			origZone = ZoneNode(orig);
			int kids = orig.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = orig.ChildAt(k);
				if (kid && !IsPosElement(kid) && !IsZoneElement(kid))
				{
					kept++;
				}
			}
		}

		string first = indent;
		if (firstBare)
		{
			first = "";
		}

		if (!row.HasZone && row.Positions.Count() == 0 && kept == 0)
		{
			outLines.Insert(first + head + " />");
			return;
		}

		outLines.Insert(first + head + ">");
		if (row.HasZone)
		{
			VPPXESpawnRow origZoneRow = new VPPXESpawnRow();
			if (origZone)
			{
				origZoneRow.HasZone = true;
				ReadZone(origZone, origZoneRow);
			}

			if (origZone && row.ZoneSameAs(origZoneRow))
			{
				AppendOriginal(doc, lines, origZone, childIndent, outLines);
			}
			else
			{
				outLines.Insert(childIndent + ZoneTag(row));
			}
		}

		foreach (VPPXESpawnPos pos : row.Positions)
		{
			bool copied = false;
			if (pos.OrigIdx >= 0 && pos.OrigIdx < origPos.Count())
			{
				VPPXESpawnPos origValues = new VPPXESpawnPos();
				VPPXmlNode origNode = origPos[pos.OrigIdx];
				ReadPos(origNode, origValues);
				if (origValues.SameAs(pos))
				{
					AppendOriginal(doc, lines, origNode, childIndent, outLines);
					copied = true;
				}
			}

			if (!copied)
			{
				outLines.Insert(childIndent + PosTag(pos));
			}
		}

		if (kept > 0)
		{
			int keptKids = orig.ChildCount();
			for (int j = 0; j < keptKids; j++)
			{
				VPPXmlNode keptKid = orig.ChildAt(j);
				if (keptKid && !IsPosElement(keptKid) && !IsZoneElement(keptKid))
				{
					VPPXEEventsIO.AppendKept(doc, keptKid, childIndent, outLines);
				}
			}
		}

		outLines.Insert(VPPXEEventsIO.CloseIndentOf(doc, orig, indent) + "</event>");
	}
};

// cfgeventgroups.xml: <eventgroupdef> of <group name> elements with <child> elements. Untouched groups and children
// stay byte for byte (the vanilla file mixes indentations); the <!--pos ... group="X"/--> note above a group is read
// as a hint of where the group was built and removed together with its group.
class VPPXEGroupsIO
{
	static VPPXmlNode RootOf(VPPXmlDocument doc)
	{
		VPPXmlNode root = null;
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, "eventgroupdef");
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, "eventgroupdef"))
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

		return "#VSTR_XMLE_GRP_ERR_ROOT";
	}

	static bool IsGroupElement(VPPXmlNode node)
	{
		return node && node.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(node.Name, "group");
	}

	static bool IsChildElement(VPPXmlNode node)
	{
		return node && node.Kind == VPPXmlNodeKind.ELEMENT && VPPXmlText.EqualsNoCase(node.Name, "child");
	}

	static void GroupNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (IsGroupElement(kid))
			{
				outNodes.Insert(kid);
			}
		}
	}

	static void ChildNodes(VPPXmlNode groupNode, array<VPPXmlNode> outNodes)
	{
		outNodes.Clear();
		int kids = groupNode.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = groupNode.ChildAt(i);
			if (IsChildElement(kid))
			{
				outNodes.Insert(kid);
			}
		}
	}

	// The text of a comment node (its lines joined with a space).
	static string CommentText(VPPXmlDocument doc, VPPXmlNode node)
	{
		string text = "";
		if (!doc || !node || node.StartLine < 0 || node.EndLine >= doc.LineCount())
		{
			return "";
		}

		for (int line = node.StartLine; line <= node.EndLine; line++)
		{
			string lineText = doc.GetLine(line);
			int startCol = 0;
			int endCol = lineText.Length();
			if (line == node.StartLine)
			{
				startCol = node.StartCol;
			}

			if (line == node.EndLine)
			{
				endCol = node.EndCol;
			}

			if (endCol > startCol)
			{
				if (text != "")
				{
					text = text + " ";
				}

				text = text + VPPXETypesWriter.Mid(lineText, startCol, endCol - startCol);
			}
		}

		return text;
	}

	// value of name="..." inside a comment's text, "" when missing
	static string AttrInText(string text, string attrName)
	{
		string needle = " " + attrName + "=\"";
		int at = text.IndexOf(needle);
		if (at < 0)
		{
			return "";
		}

		int valueStart = at + needle.Length();
		string rest = text.Substring(valueStart, text.Length() - valueStart);
		int quote = rest.IndexOf("\"");
		if (quote < 0)
		{
			return "";
		}

		return VPPXmlText.TrimWs(rest.Substring(0, quote));
	}

	// The comment right before the group (no element between) when it is a <!--pos ...--> note of that group (or of
	// no group), else null.
	static VPPXmlNode HintNodeOf(VPPXmlDocument doc, VPPXmlNode root, VPPXmlNode groupNode)
	{
		return HintAfter(doc, PreviousOf(root, groupNode), groupNode);
	}

	// The node right before kid among root's children (null for the first).
	static VPPXmlNode PreviousOf(VPPXmlNode root, VPPXmlNode kid)
	{
		VPPXmlNode previous = null;
		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode each = root.ChildAt(i);
			if (each == kid)
			{
				break;
			}

			previous = each;
		}

		return previous;
	}

	// previous (the node right before the group) when it is the group's <!--pos ...--> note, else null.
	static VPPXmlNode HintAfter(VPPXmlDocument doc, VPPXmlNode previous, VPPXmlNode groupNode)
	{
		if (!previous || previous.Kind != VPPXmlNodeKind.COMMENT)
		{
			return null;
		}

		string text = CommentText(doc, previous);
		if (text.IndexOf("<!--pos ") != 0)
		{
			return null;
		}

		string groupName = AttrInText(text, "group");
		if (groupName != "" && groupName != groupNode.GetAttr("name", ""))
		{
			return null;
		}

		return previous;
	}

	static void RowOf(VPPXmlDocument doc, VPPXmlNode root, VPPXmlNode node, VPPXEGroupRow row)
	{
		RowAfter(doc, PreviousOf(root, node), node, row);
	}

	// RowOf with the node right before the group already known (the list reads every group in one pass).
	static void RowAfter(VPPXmlDocument doc, VPPXmlNode previous, VPPXmlNode node, VPPXEGroupRow row)
	{
		array<VPPXmlNode> childNodes = new array<VPPXmlNode>();
		row.Line = node.StartLine + 1;
		row.Name = node.GetAttr("name", "");
		row.NameExtra = "";
		int attrCount = node.AttrCount();
		for (int a = 0; a < attrCount; a++)
		{
			string attrName = node.AttrNames[a];
			if (!VPPXmlText.EqualsNoCase(attrName, "name"))
			{
				row.NameExtra = row.NameExtra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(node.GetAttr(attrName, "")) + "\"";
			}
		}

		VPPXmlNode hint = HintAfter(doc, previous, node);
		if (hint)
		{
			string hintText = CommentText(doc, hint);
			row.HintX = AttrInText(hintText, "x");
			row.HintZ = AttrInText(hintText, "z");
			row.HintA = AttrInText(hintText, "a");
			row.HintY = AttrInText(hintText, "y");
		}

		ChildNodes(node, childNodes);
		for (int c = 0; c < childNodes.Count(); c++)
		{
			VPPXEGroupChild child = new VPPXEGroupChild();
			ReadChild(childNodes[c], child);
			child.OrigIdx = c;
			row.Children.Insert(child);
		}

		row.ExtraCount = 0;
		int kids = node.ChildCount();
		for (int k = 0; k < kids; k++)
		{
			VPPXmlNode kid = node.ChildAt(k);
			if (kid && !IsChildElement(kid))
			{
				row.ExtraCount = row.ExtraCount + 1;
			}
		}
	}

	static void ReadChild(VPPXmlNode node, VPPXEGroupChild child)
	{
		int attrCount = node.AttrCount();
		for (int i = 0; i < attrCount; i++)
		{
			string attrName = node.AttrNames[i];
			string attrValue = node.GetAttr(attrName, "");
			string value = VPPXmlText.TrimWs(attrValue);
			string lowerName = attrName;
			lowerName.ToLower();
			if (lowerName == "type")
			{
				child.Type = value;
			}
			else if (lowerName == "x")
			{
				child.X = value;
			}
			else if (lowerName == "z")
			{
				child.Z = value;
			}
			else if (lowerName == "a")
			{
				child.A = value;
			}
			else if (lowerName == "y")
			{
				child.Y = value;
			}
			else if (lowerName == "lootmin")
			{
				child.LootMin = value;
			}
			else if (lowerName == "lootmax")
			{
				child.LootMax = value;
			}
			else if (lowerName == "deloot")
			{
				child.Deloot = value;
			}
			else if (lowerName == "spawnsecondary")
			{
				child.SpawnSecondary = value;
			}
			else if (lowerName == "trace")
			{
				child.Trace = value;
			}
			else
			{
				child.Extra = child.Extra + " " + attrName + "=\"" + VPPXmlText.EncodeAttr(attrValue) + "\"";
			}
		}
	}

	// Indentation of new children: the first child that starts its line indented (the vanilla file has groups whose
	// first child sits at column 0), else one unit deeper than the group.
	static string ChildIndentOf(array<string> lines, VPPXmlNode node, string indent, string fallback)
	{
		if (!node || !lines)
		{
			if (fallback != "")
			{
				return fallback;
			}

			return indent + VPPXEMessagesIO.IndentUnit(indent);
		}

		int kids = node.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = node.ChildAt(i);
			if (IsChildElement(kid) && VPPXEMessagesIO.StartsLine(lines, kid))
			{
				string lead = VPPXmlText.LeadingWs(lines[kid.StartLine]);
				if (lead != "")
				{
					return lead;
				}
			}
		}

		if (fallback != "")
		{
			return fallback;
		}

		return indent + VPPXEMessagesIO.IndentUnit(indent);
	}

	// A changed or new child in the vanilla attribute order.
	static string ChildTag(VPPXEGroupChild child)
	{
		string tag = "<child type=\"" + VPPXmlText.EncodeAttr(child.Type) + "\"";
		if (child.Deloot != "")
		{
			tag = tag + " deloot=\"" + VPPXmlText.EncodeAttr(child.Deloot) + "\"";
		}

		if (child.LootMax != "")
		{
			tag = tag + " lootmax=\"" + VPPXmlText.EncodeAttr(child.LootMax) + "\"";
		}

		if (child.LootMin != "")
		{
			tag = tag + " lootmin=\"" + VPPXmlText.EncodeAttr(child.LootMin) + "\"";
		}

		tag = tag + " x=\"" + VPPXmlText.EncodeAttr(child.X) + "\" z=\"" + VPPXmlText.EncodeAttr(child.Z) + "\"";
		if (child.A != "")
		{
			tag = tag + " a=\"" + VPPXmlText.EncodeAttr(child.A) + "\"";
		}

		if (child.Y != "")
		{
			tag = tag + " y=\"" + VPPXmlText.EncodeAttr(child.Y) + "\"";
		}

		if (child.SpawnSecondary != "")
		{
			tag = tag + " spawnsecondary=\"" + VPPXmlText.EncodeAttr(child.SpawnSecondary) + "\"";
		}

		if (child.Trace != "")
		{
			tag = tag + " trace=\"" + VPPXmlText.EncodeAttr(child.Trace) + "\"";
		}

		return tag + child.Extra + "/>";
	}

	// A <group> block; orig (may be null) is the element it replaces.
	static void BuildBlock(VPPXEGroupRow row, VPPXmlDocument doc, VPPXmlNode orig, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		string head = "<group name=\"" + VPPXmlText.EncodeAttr(row.Name) + "\"" + row.NameExtra;
		array<VPPXmlNode> origChildren = new array<VPPXmlNode>();
		array<string> lines = null;
		int kept = 0;
		outLines.Clear();
		if (orig && doc)
		{
			lines = doc.GetLines();
			ChildNodes(orig, origChildren);
			int kids = orig.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = orig.ChildAt(k);
				if (kid && !IsChildElement(kid))
				{
					kept++;
				}
			}
		}

		string first = indent;
		if (firstBare)
		{
			first = "";
		}

		if (row.Children.Count() == 0 && kept == 0)
		{
			outLines.Insert(first + head + "/>");
			return;
		}

		outLines.Insert(first + head + ">");
		foreach (VPPXEGroupChild child : row.Children)
		{
			bool copied = false;
			if (child.OrigIdx >= 0 && child.OrigIdx < origChildren.Count())
			{
				VPPXEGroupChild origValues = new VPPXEGroupChild();
				VPPXmlNode origNode = origChildren[child.OrigIdx];
				ReadChild(origNode, origValues);
				if (origValues.SameAs(child))
				{
					VPPXESpawnsIO.AppendOriginal(doc, lines, origNode, childIndent, outLines);
					copied = true;
				}
			}

			if (!copied)
			{
				outLines.Insert(childIndent + ChildTag(child));
			}
		}

		if (kept > 0)
		{
			int keptKids = orig.ChildCount();
			for (int j = 0; j < keptKids; j++)
			{
				VPPXmlNode keptKid = orig.ChildAt(j);
				if (keptKid && !IsChildElement(keptKid))
				{
					VPPXEEventsIO.AppendKept(doc, keptKid, childIndent, outLines);
				}
			}
		}

		outLines.Insert(VPPXEEventsIO.CloseIndentOf(doc, orig, indent) + "</group>");
	}
};

// XE_GetEvents: cfgeventgroups.xml (whole groups), cfgeventspawns.xml (with the group names of cfgeventgroups.xml) first, then every EVENTS file of the
// registry in chunks of rows (one chunk at least per file; one empty chunk with FileCount 0 when there is none).
class VPPXEEventsListJob : VPPXEJob
{
	const static int PH_START = 0;
	const static int PH_GROUPS_PARSE = 1;
	const static int PH_SPAWNS_OPEN = 2;
	const static int PH_SPAWNS_PARSE = 3;
	const static int PH_FILE_OPEN = 4;
	const static int PH_FILE_PARSE = 5;
	const static int PH_DONE = 6;

	protected PlayerIdentity m_Sender;
	protected int m_ReqId;
	protected bool m_Replied;
	protected int m_Phase;
	protected ref array<string> m_FileKeys;
	protected int m_FileIdx;
	protected ref VPPXmlDocument m_Doc;
	protected int m_DocRevision;
	protected string m_SpawnsKey;
	protected ref array<string> m_GroupNames;
	protected string m_GroupsError;
	protected string m_GroupsKey;
	protected int m_GroupsRevision;

	void VPPXEEventsListJob(PlayerIdentity sender, int reqId)
	{
		m_Sender = sender;
		m_ReqId = reqId;
		m_Phase = PH_START;
		m_FileKeys = new array<string>();
		m_GroupNames = new array<string>();
		m_SpawnsKey = "";
		m_GroupsError = "";
		m_GroupsKey = "";
	}

	override string GetLabel()
	{
		return "EventsList";
	}

	// The client waits for every chunk: an abort before the end answers even when some chunks went out.
	override void OnAborted()
	{
		if (m_Phase != PH_DONE)
		{
			m_Replied = true;
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
				if (!Start())
				{
					return true;
				}
			}
			else if (m_Phase == PH_GROUPS_PARSE)
			{
				if (!m_Doc.Step())
				{
					return false;
				}

				GroupsParsed();
				m_Doc = null;
				m_Phase = PH_SPAWNS_OPEN;
			}
			else if (m_Phase == PH_SPAWNS_OPEN)
			{
				OpenSpawns();
			}
			else if (m_Phase == PH_SPAWNS_PARSE)
			{
				if (!m_Doc.Step())
				{
					return false;
				}

				SendSpawns(VPPXESpawnsIO.RootOf(m_Doc), VPPXESpawnsIO.ParseError(m_Doc));
				m_Doc = null;
				m_Phase = PH_FILE_OPEN;
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

	// Collects the files and opens cfgeventgroups.xml; false when the request is answered already (not ready).
	protected bool Start()
	{
		string content = "";
		XMLEditor editor = GetXMLEditor();
		if (!editor || !editor.GetRegistry())
		{
			m_Replied = true;
			VPPXENet.Result(m_Sender, m_ReqId, false, "#VSTR_XMLE_ERR_NOT_READY", "");
			m_Phase = PH_DONE;
			return false;
		}

		array<VPPXEFileEntry> files = new array<VPPXEFileEntry>();
		editor.GetRegistry().GetByKind(VPPXEFileKind.EVENTS, files);
		foreach (VPPXEFileEntry fileEntry : files)
		{
			m_FileKeys.Insert(fileEntry.Key);
		}

		m_FileIdx = 0;
		m_Phase = PH_SPAWNS_OPEN;
		VPPXEFileEntry groups = editor.GetRegistry().FirstOfKind(VPPXEFileKind.EVENTGROUPS);
		if (!groups || !FileExist(groups.Path))
		{
			m_GroupsError = "#VSTR_XMLE_STATE_MISSING";
			SendGroups(null);
			return true;
		}

		m_GroupsKey = groups.Key;
		if (!VPPXmlText.ReadAll(groups.Path, content))
		{
			editor.GetRegistry().NoteRevision(groups.Key, 0, false);
			m_GroupsError = "#VSTR_XMLE_ERR_READ";
			SendGroups(null);
			return true;
		}

		m_GroupsRevision = VPPXmlText.NormalizedHash(content);
		editor.GetRegistry().NoteRevision(groups.Key, m_GroupsRevision, true);
		m_Doc = new VPPXmlDocument();
		m_Doc.BeginString(content);
		m_Phase = PH_GROUPS_PARSE;
		return true;
	}

	// <group name> of cfgeventgroups.xml, first definition of a name only (the server's rule), and the whole groups
	// for the GROUPS view.
	protected void GroupsParsed()
	{
		map<string, bool> seen = new map<string, bool>();
		VPPXmlNode root = VPPXEGroupsIO.RootOf(m_Doc);
		if (!root)
		{
			m_GroupsError = VPPXEGroupsIO.ParseError(m_Doc);
			SendGroups(null);
			return;
		}

		SendGroups(root);

		int kids = root.ChildCount();
		for (int i = 0; i < kids; i++)
		{
			VPPXmlNode kid = root.ChildAt(i);
			if (!kid || kid.Kind != VPPXmlNodeKind.ELEMENT || !VPPXmlText.EqualsNoCase(kid.Name, "group"))
			{
				continue;
			}

			string groupName = kid.GetAttr("name", "");
			if (groupName != "" && !seen.Contains(groupName))
			{
				seen.Set(groupName, true);
				m_GroupNames.Insert(groupName);
			}
		}
	}

	// The groups in chunks of about CHUNK_BYTES (one chunk at least; root null = m_GroupsError says why).
	protected void SendGroups(VPPXmlNode root)
	{
		XMLEditor editor = GetXMLEditor();
		array<ref VPPXEGroupsChunk> chunks = new array<ref VPPXEGroupsChunk>();
		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXEGroupsChunk current = new VPPXEGroupsChunk();
		bool editable = false;
		int bytes = 0;
		chunks.Insert(current);
		if (root)
		{
			if (editor && !editor.IsReadOnlyMode() && !root.SelfClosing)
			{
				editable = true;
			}

			// every group with the node right before it (its <!--pos--> note, if any)
			array<VPPXmlNode> previousNodes = new array<VPPXmlNode>();
			VPPXmlNode previous = null;
			int kids = root.ChildCount();
			for (int k = 0; k < kids; k++)
			{
				VPPXmlNode kid = root.ChildAt(k);
				if (VPPXEGroupsIO.IsGroupElement(kid))
				{
					nodes.Insert(kid);
					previousNodes.Insert(previous);
				}

				previous = kid;
			}

			int nodeCount = nodes.Count();
			if (nodeCount > VPPXEGroupRules.MAX_ROWS)
			{
				nodeCount = VPPXEGroupRules.MAX_ROWS;
				editable = false;
			}

			for (int i = 0; i < nodeCount; i++)
			{
				VPPXEGroupRow row = new VPPXEGroupRow();
				VPPXEGroupsIO.RowAfter(m_Doc, previousNodes[i], nodes[i], row);
				int size = VPPXEGroupRules.EstimateRow(row);
				if (current.Rows.Count() > 0 && bytes + size > VPPXEGroupRules.CHUNK_BYTES)
				{
					current = new VPPXEGroupsChunk();
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
			VPPXEGroupsChunk chunk = chunks[c];
			chunk.ReqId = m_ReqId;
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
			chunk.FileKey = m_GroupsKey;
			chunk.Revision = m_GroupsRevision;
			chunk.Editable = editable;
			chunk.ErrorKey = "";
			if (!root)
			{
				chunk.ErrorKey = m_GroupsError;
			}

			ScriptRPC netRpc = VPPXENet.Begin("XE_OnGroupsChunk");
			Param1<ref VPPXEGroupsChunk> netPayload = new Param1<ref VPPXEGroupsChunk>(chunk);
			netRpc.Write(netPayload);
			VPPXENet.Send(m_Sender, "XE_OnGroupsChunk", netRpc);
		}

		m_Replied = true;
	}

	protected void OpenSpawns()
	{
		string content = "";
		XMLEditor editor = GetXMLEditor();
		VPPXEFileEntry spawns = null;
		if (editor && editor.GetRegistry())
		{
			spawns = editor.GetRegistry().FirstOfKind(VPPXEFileKind.EVENTSPAWNS);
		}

		m_Phase = PH_FILE_OPEN;
		if (!spawns || !FileExist(spawns.Path))
		{
			SendSpawns(null, "#VSTR_XMLE_STATE_MISSING");
			return;
		}

		m_SpawnsKey = spawns.Key;
		if (!VPPXmlText.ReadAll(spawns.Path, content))
		{
			editor.GetRegistry().NoteRevision(spawns.Key, 0, false);
			SendSpawns(null, "#VSTR_XMLE_ERR_READ");
			return;
		}

		m_DocRevision = VPPXmlText.NormalizedHash(content);
		editor.GetRegistry().NoteRevision(spawns.Key, m_DocRevision, true);
		m_Doc = new VPPXmlDocument();
		m_Doc.BeginString(content);
		m_Phase = PH_SPAWNS_PARSE;
	}

	// The spawn entries in chunks of about CHUNK_BYTES; the group names ride along (GROUPS_PER_CHUNK per chunk, extra
	// chunks without rows when there are more). root null = errorKey says why the file cannot be used.
	protected void SendSpawns(VPPXmlNode root, string errorKey)
	{
		XMLEditor editor = GetXMLEditor();
		array<ref VPPXESpawnsChunk> chunks = new array<ref VPPXESpawnsChunk>();
		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXESpawnsChunk current = new VPPXESpawnsChunk();
		bool editable = false;
		int bytes = 0;
		int nodeCount = 0;
		string finalError = "";
		chunks.Insert(current);
		if (!root)
		{
			finalError = errorKey;
		}
		else
		{
			if (editor && !editor.IsReadOnlyMode() && !root.SelfClosing)
			{
				editable = true;
			}

			VPPXEEventsIO.EventNodes(root, nodes);
			nodeCount = nodes.Count();
			if (nodeCount > VPPXESpawnRules.MAX_ROWS)
			{
				nodeCount = VPPXESpawnRules.MAX_ROWS;
				editable = false;
			}

			for (int i = 0; i < nodeCount; i++)
			{
				VPPXESpawnRow row = new VPPXESpawnRow();
				VPPXESpawnsIO.RowOf(nodes[i], row);
				int size = VPPXESpawnRules.EstimateRow(row);
				if (current.Rows.Count() > 0 && bytes + size > VPPXESpawnRules.CHUNK_BYTES)
				{
					current = new VPPXESpawnsChunk();
					chunks.Insert(current);
					bytes = 0;
				}

				current.Rows.Insert(row);
				bytes += size;
			}
		}

		int groupCount = m_GroupNames.Count();
		int groupChunks = (groupCount + VPPXESpawnRules.GROUPS_PER_CHUNK - 1) / VPPXESpawnRules.GROUPS_PER_CHUNK;
		while (chunks.Count() < groupChunks)
		{
			chunks.Insert(new VPPXESpawnsChunk());
		}

		int chunkCount = chunks.Count();
		for (int c = 0; c < chunkCount; c++)
		{
			VPPXESpawnsChunk chunk = chunks[c];
			chunk.ReqId = m_ReqId;
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
			chunk.FileKey = m_SpawnsKey;
			chunk.Revision = m_DocRevision;
			chunk.Editable = editable;
			chunk.ErrorKey = finalError;
			chunk.GroupsError = m_GroupsError;
			int firstGroup = c * VPPXESpawnRules.GROUPS_PER_CHUNK;
			int endGroup = firstGroup + VPPXESpawnRules.GROUPS_PER_CHUNK;
			if (endGroup > groupCount)
			{
				endGroup = groupCount;
			}

			for (int g = firstGroup; g < endGroup; g++)
			{
				chunk.Groups.Insert(m_GroupNames[g]);
			}

			ScriptRPC netRpc = VPPXENet.Begin("XE_OnSpawnsChunk");
			Param1<ref VPPXESpawnsChunk> netPayload = new Param1<ref VPPXESpawnsChunk>(chunk);
			netRpc.Write(netPayload);
			VPPXENet.Send(m_Sender, "XE_OnSpawnsChunk", netRpc);
		}

		m_Replied = true;
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
			VPPXEEventsChunk emptyChunk = NewChunk(-1, 0, "");
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
			VPPXEEventsChunk errorChunk = NewChunk(m_FileIdx, fileCount, fileKey);
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
		int fileCount = m_FileKeys.Count();
		array<ref VPPXEEventsChunk> chunks = new array<ref VPPXEEventsChunk>();
		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		VPPXEEventsChunk current = NewChunk(m_FileIdx, fileCount, fileKey);
		string errorKey = "";
		bool editable = false;
		int bytes = 0;
		int nodeCount = 0;
		current.Revision = m_DocRevision;
		chunks.Insert(current);
		VPPXmlNode root = VPPXEEventsIO.RootOf(m_Doc);
		if (!root)
		{
			errorKey = VPPXEEventsIO.ParseError(m_Doc);
		}
		else
		{
			if (editor && !editor.IsReadOnlyMode() && !root.SelfClosing)
			{
				editable = true;
			}

			VPPXEEventsIO.EventNodes(root, nodes);
			nodeCount = nodes.Count();
			if (nodeCount > VPPXEEventRules.MAX_ROWS)
			{
				nodeCount = VPPXEEventRules.MAX_ROWS;
				editable = false;
			}

			current.Editable = editable;
			for (int i = 0; i < nodeCount; i++)
			{
				VPPXEEventRow row = new VPPXEEventRow();
				VPPXEEventsIO.RowOf(nodes[i], row);
				int size = VPPXEEventRules.EstimateRow(row);
				if (current.Rows.Count() > 0 && bytes + size > VPPXEEventRules.CHUNK_BYTES)
				{
					current = NewChunk(m_FileIdx, fileCount, fileKey);
					current.Revision = m_DocRevision;
					current.Editable = editable;
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
			VPPXEEventsChunk chunk = chunks[c];
			chunk.ChunkIdx = c;
			chunk.ChunkCount = chunkCount;
			chunk.ErrorKey = errorKey;
			SendChunk(chunk);
		}
	}

	protected VPPXEEventsChunk NewChunk(int fileIdx, int fileCount, string fileKey)
	{
		VPPXEEventsChunk chunk = new VPPXEEventsChunk();
		chunk.ReqId = m_ReqId;
		chunk.FileIdx = fileIdx;
		chunk.FileCount = fileCount;
		chunk.FileKey = fileKey;
		chunk.Editable = false;
		return chunk;
	}

	protected void SendChunk(VPPXEEventsChunk chunk)
	{
		ScriptRPC netRpc = VPPXENet.Begin("XE_OnEventsChunk");
		Param1<ref VPPXEEventsChunk> netPayload = new Param1<ref VPPXEEventsChunk>(chunk);
		netRpc.Write(netPayload);
		VPPXENet.Send(m_Sender, "XE_OnEventsChunk", netRpc);
		m_Replied = true;
	}
};

// A save of whole <event> elements of one file (events or spawns): stale check against the revision the client
// edited, budgeted parse, validation, patch (ADDs before the root's closing tag, then UPDATE / DELETE from the last
// element up), budgeted verify parse, re-check that nothing changed meanwhile, backup and verified write. Subclasses
// give the file kind, the root, and the edits.
class VPPXEPatchJob : VPPXEJob
{
	const static int PH_START = 0;
	const static int PH_PARSE = 1;
	const static int PH_VERIFY = 2;
	const static int PH_DONE = 3;

	protected PlayerIdentity m_Sender;
	protected bool m_Replied;
	protected int m_Phase;
	protected VPPXEFileEntry m_Entry;
	protected string m_Content;
	protected string m_NewContent;
	protected ref VPPXmlDocument m_Doc;
	protected ref VPPXmlDocument m_Check;
	protected int m_Expected;
	// the <event> elements of the original document (valid while m_Doc lives)
	protected ref array<VPPXmlNode> m_Nodes;
	protected int m_Adds;
	protected int m_Updates;
	protected int m_Deletes;

	// ---------------------------------------------------------------- subclass contract

	protected int ReqId()
	{
		return 0;
	}

	protected string TargetKey()
	{
		return "";
	}

	protected int BaseRevision()
	{
		return 0;
	}

	protected int TargetKind()
	{
		return -1;
	}

	protected string RootName()
	{
		return "";
	}

	protected string RootErrorKey()
	{
		return "#VSTR_XMLE_ERR_PARSE";
	}

	protected int MaxRows()
	{
		return 0;
	}

	// The elements an edit index counts (<event> in events and spawns files).
	protected void ElementNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		VPPXEEventsIO.EventNodes(root, outNodes);
	}

	// After an UPDATE of node was applied: a change above it (elements are patched from the last one up, so only lines
	// above node are still as read). False = failed.
	protected bool AfterUpdate(VPPXmlNode root, VPPXmlNode node, int editIdx, array<string> lines, array<bool> crFlags, bool cr)
	{
		return true;
	}

	// A node right before an element that a DELETE removes with it (null = none).
	protected VPPXmlNode DeleteLead(VPPXmlNode root, VPPXmlNode node)
	{
		return null;
	}

	protected int EditCount()
	{
		return 0;
	}

	protected int EditOp(int editIdx)
	{
		return -1;
	}

	protected int EditIndex(int editIdx)
	{
		return -1;
	}

	protected bool HasEditRow(int editIdx)
	{
		return false;
	}

	// "" or the #key of why the written row is refused.
	protected string CheckEdit(int editIdx)
	{
		return "";
	}

	// The block of an ADD or UPDATE; orig = the element it replaces (null for ADD), sample = an element for the
	// indentation (the element itself, or the last one for ADD).
	protected void BuildEdit(int editIdx, VPPXmlDocument doc, VPPXmlNode orig, VPPXmlNode sample, array<string> lines, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
	}

	protected string SummaryName()
	{
		return "";
	}

	protected string SavedKey()
	{
		return "";
	}

	// ---------------------------------------------------------------- job

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
		m_Phase = PH_DONE;
		VPPXENet.Result(m_Sender, ReqId(), ok, key, arg);
	}

	override bool Step()
	{
		while (VPPXEJobQueue.BudgetOk())
		{
			if (m_Phase == PH_START)
			{
				Start();
			}
			else if (m_Phase == PH_PARSE)
			{
				if (!m_Doc.Step())
				{
					return false;
				}

				Patch();
			}
			else if (m_Phase == PH_VERIFY)
			{
				if (!m_Check.Step())
				{
					return false;
				}

				Finish();
			}
			else
			{
				return true;
			}
		}

		return m_Phase == PH_DONE;
	}

	protected VPPXmlNode ResolveTargetRoot(VPPXmlDocument doc)
	{
		VPPXmlNode root = null;
		if (doc.HasError())
		{
			return null;
		}

		root = VPPXEFileRegistry.ResolveRoot(doc, RootName());
		if (!root || !VPPXmlText.EqualsNoCase(root.Name, RootName()))
		{
			return null;
		}

		return root;
	}

	protected void Start()
	{
		XMLEditor editor = GetXMLEditor();
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

		m_Entry = editor.GetRegistry().Find(TargetKey());
		if (!m_Entry || m_Entry.Kind != TargetKind())
		{
			Reply(false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		if (!FileExist(m_Entry.Path) || !VPPXmlText.ReadAll(m_Entry.Path, m_Content))
		{
			Reply(false, "#VSTR_XMLE_ERR_READ", "");
			return;
		}

		if (VPPXmlText.NormalizedHash(m_Content) != BaseRevision())
		{
			Reply(false, "#VSTR_XMLE_ERR_STALE", "");
			return;
		}

		m_Doc = new VPPXmlDocument();
		m_Doc.BeginString(m_Content);
		m_Phase = PH_PARSE;
	}

	// Validates the edits and builds the new content, then starts the verify parse.
	protected void Patch()
	{
		VPPXmlNode root = ResolveTargetRoot(m_Doc);
		if (!root)
		{
			if (m_Doc.HasError())
			{
				Reply(false, "#VSTR_XMLE_ERR_PARSE", "");
			}
			else
			{
				Reply(false, RootErrorKey(), "");
			}

			return;
		}

		if (root.SelfClosing || root.CloseStartLine < 0)
		{
			Reply(false, RootErrorKey(), "");
			return;
		}

		array<VPPXmlNode> nodes = new array<VPPXmlNode>();
		ElementNodes(root, nodes);
		m_Nodes = nodes;
		int nodeCount = nodes.Count();
		map<int, int> byIndex = new map<int, int>();
		if (!Validate(nodeCount, byIndex))
		{
			return;
		}

		array<string> lines = new array<string>();
		lines.Copy(m_Doc.GetLines());
		array<bool> crFlags = new array<bool>();
		crFlags.Copy(m_Doc.GetCRFlags());
		bool cr = m_Doc.DominantIsCR();
		string addIndent = "    ";
		string addChildIndent = "        ";
		VPPXmlNode sample = null;
		if (nodeCount > 0)
		{
			sample = nodes[nodeCount - 1];
			if (VPPXEMessagesIO.StartsLine(lines, sample))
			{
				addIndent = VPPXEMessagesIO.IndentOf(lines, sample);
				addChildIndent = VPPXEMessagesIO.ChildIndentOf(lines, sample, addIndent);
			}
		}

		array<string> addLines = new array<string>();
		array<string> block = new array<string>();
		int editCount = EditCount();
		for (int a = 0; a < editCount; a++)
		{
			if (EditOp(a) != VPPXEOp.ADD)
			{
				continue;
			}

			BuildEdit(a, m_Doc, null, sample, lines, addIndent, addChildIndent, false, block);
			foreach (string addLine : block)
			{
				addLines.Insert(addLine);
			}
		}

		if (addLines.Count() > 0 && !VPPXEMessagesIO.InsertBeforeClose(lines, crFlags, root.CloseStartLine, root.CloseStartCol, addLines, cr))
		{
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "add");
			return;
		}

		for (int n = nodeCount - 1; n >= 0; n--)
		{
			if (!byIndex.Contains(n))
			{
				continue;
			}

			VPPXmlNode node = nodes[n];
			int editIdx = byIndex.Get(n);
			bool applied = false;
			array<string> spanLines = new array<string>();
			if (EditOp(editIdx) == VPPXEOp.DELETE)
			{
				VPPXmlNode lead = DeleteLead(root, node);
				if (lead && VPPXEMessagesIO.StartsLine(lines, lead) && VPPXEMessagesIO.EndsLine(lines, node))
				{
					applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, lead.StartLine, 0, node.EndLine + 1, 0, spanLines, cr);
				}
				else if (VPPXEMessagesIO.StartsLine(lines, node) && VPPXEMessagesIO.EndsLine(lines, node))
				{
					applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, 0, node.EndLine + 1, 0, spanLines, cr);
				}
				else
				{
					applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, node.StartCol, node.EndLine, node.EndCol, spanLines, cr);
				}
			}
			else
			{
				string indent = VPPXEMessagesIO.IndentOf(lines, node);
				string childIndent = VPPXEMessagesIO.ChildIndentOf(lines, node, indent);
				BuildEdit(editIdx, m_Doc, node, node, lines, indent, childIndent, true, spanLines);
				applied = VPPXEMessagesIO.ApplySpan(lines, crFlags, node.StartLine, node.StartCol, node.EndLine, node.EndCol, spanLines, cr);
				if (applied)
				{
					applied = AfterUpdate(root, node, editIdx, lines, crFlags, cr);
				}
			}

			if (!applied)
			{
				Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "span");
				return;
			}
		}

		m_NewContent = VPPXmlText.JoinLines(lines, crFlags, m_Doc.GetPrefix(), m_Doc.EndsWithNewline());
		m_Expected = nodeCount + m_Adds - m_Deletes;
		m_Check = new VPPXmlDocument();
		m_Check.BeginString(m_NewContent);
		m_Phase = PH_VERIFY;
	}

	// Known ops, indexes in range and used once, every written row passes the rules; false when answered.
	protected bool Validate(int nodeCount, map<int, int> byIndex)
	{
		int editCount = EditCount();
		for (int e = 0; e < editCount; e++)
		{
			int op = EditOp(e);
			if (!HasEditRow(e))
			{
				Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
				return false;
			}

			if (op == VPPXEOp.ADD)
			{
				m_Adds++;
			}
			else if (op == VPPXEOp.UPDATE || op == VPPXEOp.DELETE)
			{
				int index = EditIndex(e);
				if (index < 0 || index >= nodeCount || byIndex.Contains(index))
				{
					Reply(false, "#VSTR_XMLE_ERR_STALE", "");
					return false;
				}

				byIndex.Set(index, e);
				if (op == VPPXEOp.UPDATE)
				{
					m_Updates++;
				}
				else
				{
					m_Deletes++;
				}
			}
			else
			{
				Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
				return false;
			}

			if (op != VPPXEOp.DELETE)
			{
				string ruleKey = CheckEdit(e);
				if (ruleKey != "")
				{
					int shown = e + 1;
					Reply(false, ruleKey, shown.ToString());
					return false;
				}
			}
		}

		if (nodeCount + m_Adds - m_Deletes > MaxRows())
		{
			Reply(false, "#VSTR_XMLE_ERR_VALIDATION", "");
			return false;
		}

		return true;
	}

	// The result must parse to the expected number of elements; then backup and a verified write.
	protected void Finish()
	{
		XMLEditor editor = GetXMLEditor();
		string fileKey = TargetKey();
		array<VPPXmlNode> checkNodes = new array<VPPXmlNode>();
		VPPXmlNode checkRoot = ResolveTargetRoot(m_Check);
		if (checkRoot)
		{
			ElementNodes(checkRoot, checkNodes);
		}

		if (!checkRoot || checkNodes.Count() != m_Expected)
		{
			VPPXELog.Warn("Save of " + fileKey + " produced an unexpected document; nothing was written");
			Reply(false, "#VSTR_XMLE_ERR_INTERNAL", "verify");
			return;
		}

		if (!editor || !editor.GetBackups() || !editor.GetRegistry())
		{
			Reply(false, "#VSTR_XMLE_ERR_NOT_READY", "");
			return;
		}

		// the job ran over several frames: the registry may have been rebuilt, or the file written meanwhile
		string current = "";
		m_Entry = editor.GetRegistry().Find(fileKey);
		if (!m_Entry || m_Entry.Kind != TargetKind())
		{
			Reply(false, "#VSTR_XMLE_ERR_FILE_UNKNOWN", "");
			return;
		}

		if (!VPPXmlText.ReadAll(m_Entry.Path, current) || VPPXmlText.NormalizedHash(current) != BaseRevision())
		{
			Reply(false, "#VSTR_XMLE_ERR_STALE", "");
			return;
		}

		VPPXEBackupManager backups = editor.GetBackups();
		int editCount = EditCount();
		string summary = SummaryName() + ": " + m_Updates.ToString() + " changed, " + m_Adds.ToString() + " added, " + m_Deletes.ToString() + " removed";
		string backupId = backups.CreateBackup(m_Entry, m_Content, VPPXEBackupReason.EDIT, m_Sender, summary, editCount, null, "");
		if (backupId == "")
		{
			Reply(false, "#VSTR_XMLE_ERR_BACKUP", "");
			return;
		}

		string writeErr = "";
		string writeArg = "";
		string noticeKey = "";
		if (!VPPXESafeWrite.Write(m_Entry, m_NewContent, backupId, m_Content, true, writeErr, writeArg, noticeKey))
		{
			backups.ApplyRetention();
			VPPXELog.Action(m_Sender, "Saving " + fileKey + " failed: " + writeErr + " " + writeArg, false);
			Reply(false, writeErr, writeArg);
			return;
		}

		backups.SetResultHash(backupId, VPPXmlText.NormalizedHash(m_NewContent));
		backups.ApplyRetention();
		VPPXELog.Action(m_Sender, "Saved " + fileKey + " (" + summary + ", backup " + backupId + ")", true);
		editor.OnFileWritten(fileKey, m_Sender);
		Reply(true, SavedKey(), fileKey);
	}
};

// XE_SaveEvents: the assembled edits of one events file.
class VPPXEEventsSaveJob : VPPXEPatchJob
{
	protected ref VPPXEEventsSave m_Save;

	void VPPXEEventsSaveJob(PlayerIdentity sender, VPPXEEventsSave save)
	{
		m_Sender = sender;
		m_Save = save;
		m_Phase = PH_START;
		m_Content = "";
		m_NewContent = "";
	}

	override string GetLabel()
	{
		return "EventsSave";
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
		return VPPXEFileKind.EVENTS;
	}

	protected override string RootName()
	{
		return "events";
	}

	protected override string RootErrorKey()
	{
		return "#VSTR_XMLE_EVT_ERR_ROOT";
	}

	protected override int MaxRows()
	{
		return VPPXEEventRules.MAX_ROWS;
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
		VPPXEEventEdit edit = m_Save.Edits[editIdx];
		return edit && edit.Row;
	}

	protected override string CheckEdit(int editIdx)
	{
		return VPPXEEventRules.CheckRow(m_Save.Edits[editIdx].Row);
	}

	protected override void BuildEdit(int editIdx, VPPXmlDocument doc, VPPXmlNode orig, VPPXmlNode sample, array<string> lines, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		string grandIndent = VPPXEEventsIO.GrandIndentOf(lines, sample, indent, childIndent);
		VPPXEEventsIO.BuildBlock(m_Save.Edits[editIdx].Row, doc, orig, indent, childIndent, grandIndent, firstBare, outLines);
	}

	protected override string SummaryName()
	{
		return "events";
	}

	protected override string SavedKey()
	{
		return "#VSTR_XMLE_EVT_SAVED";
	}
};

// XE_SaveEventSpawns: the assembled edits of cfgeventspawns.xml.
class VPPXESpawnsSaveJob : VPPXEPatchJob
{
	protected ref VPPXESpawnsSave m_Save;

	void VPPXESpawnsSaveJob(PlayerIdentity sender, VPPXESpawnsSave save)
	{
		m_Sender = sender;
		m_Save = save;
		m_Phase = PH_START;
		m_Content = "";
		m_NewContent = "";
	}

	override string GetLabel()
	{
		return "SpawnsSave";
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
		return VPPXEFileKind.EVENTSPAWNS;
	}

	protected override string RootName()
	{
		return "eventposdef";
	}

	protected override string RootErrorKey()
	{
		return "#VSTR_XMLE_SPW_ERR_ROOT";
	}

	protected override int MaxRows()
	{
		return VPPXESpawnRules.MAX_ROWS;
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
		VPPXESpawnEdit edit = m_Save.Edits[editIdx];
		return edit && edit.Row;
	}

	// An UPDATE is checked against the entry as it is in the file: unchanged positions and zone are copied, not checked.
	protected override string CheckEdit(int editIdx)
	{
		VPPXESpawnEdit edit = m_Save.Edits[editIdx];
		VPPXESpawnRow orig = null;
		if (edit.Op == VPPXEOp.UPDATE && m_Nodes && edit.Index >= 0 && edit.Index < m_Nodes.Count())
		{
			orig = new VPPXESpawnRow();
			VPPXESpawnsIO.RowOf(m_Nodes[edit.Index], orig);
		}

		return VPPXESpawnRules.CheckRow(edit.Row, orig);
	}

	protected override void BuildEdit(int editIdx, VPPXmlDocument doc, VPPXmlNode orig, VPPXmlNode sample, array<string> lines, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		VPPXESpawnsIO.BuildBlock(m_Save.Edits[editIdx].Row, doc, orig, indent, childIndent, firstBare, outLines);
	}

	protected override string SummaryName()
	{
		return "spawns";
	}

	protected override string SavedKey()
	{
		return "#VSTR_XMLE_SPW_SAVED";
	}
};

// XE_SaveEventGroups: the assembled edits of cfgeventgroups.xml.
class VPPXEGroupsSaveJob : VPPXEPatchJob
{
	protected ref VPPXEGroupsSave m_Save;

	void VPPXEGroupsSaveJob(PlayerIdentity sender, VPPXEGroupsSave save)
	{
		m_Sender = sender;
		m_Save = save;
		m_Phase = PH_START;
		m_Content = "";
		m_NewContent = "";
	}

	override string GetLabel()
	{
		return "GroupsSave";
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
		return VPPXEFileKind.EVENTGROUPS;
	}

	protected override string RootName()
	{
		return "eventgroupdef";
	}

	protected override string RootErrorKey()
	{
		return "#VSTR_XMLE_GRP_ERR_ROOT";
	}

	protected override int MaxRows()
	{
		return VPPXEGroupRules.MAX_ROWS;
	}

	protected override void ElementNodes(VPPXmlNode root, array<VPPXmlNode> outNodes)
	{
		VPPXEGroupsIO.GroupNodes(root, outNodes);
	}

	// a renamed group keeps its <!--pos ... group="..."/--> note: the name in it follows (one-line notes)
	protected override bool AfterUpdate(VPPXmlNode root, VPPXmlNode node, int editIdx, array<string> lines, array<bool> crFlags, bool cr)
	{
		string oldName = node.GetAttr("name", "");
		string newName = m_Save.Edits[editIdx].Row.Name;
		if (oldName == newName)
		{
			return true;
		}

		VPPXmlNode hint = VPPXEGroupsIO.HintNodeOf(m_Doc, root, node);
		if (!hint || hint.StartLine != hint.EndLine || hint.StartLine >= lines.Count())
		{
			return true;
		}

		string noteText = VPPXETypesWriter.Mid(lines[hint.StartLine], hint.StartCol, hint.EndCol - hint.StartCol);
		string oldAttr = " group=\"" + oldName + "\"";
		if (noteText.IndexOf(oldAttr) < 0)
		{
			return true;
		}

		noteText.Replace(oldAttr, " group=\"" + VPPXmlText.EncodeAttr(newName) + "\"");
		array<string> noteLines = new array<string>();
		noteLines.Insert(noteText);
		return VPPXEMessagesIO.ApplySpan(lines, crFlags, hint.StartLine, hint.StartCol, hint.EndLine, hint.EndCol, noteLines, cr);
	}

	// the <!--pos ...--> note of a deleted group goes with it
	protected override VPPXmlNode DeleteLead(VPPXmlNode root, VPPXmlNode node)
	{
		return VPPXEGroupsIO.HintNodeOf(m_Doc, root, node);
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
		VPPXEGroupEdit edit = m_Save.Edits[editIdx];
		return edit && edit.Row;
	}

	// An UPDATE is checked against the group as it is in the file: unchanged children are copied, not checked.
	protected override string CheckEdit(int editIdx)
	{
		VPPXEGroupEdit edit = m_Save.Edits[editIdx];
		VPPXEGroupRow orig = null;
		if (edit.Op == VPPXEOp.UPDATE && m_Nodes && edit.Index >= 0 && edit.Index < m_Nodes.Count())
		{
			orig = new VPPXEGroupRow();
			VPPXEGroupsIO.RowOf(m_Doc, ResolveTargetRoot(m_Doc), m_Nodes[edit.Index], orig);
		}

		return VPPXEGroupRules.CheckRow(edit.Row, orig);
	}

	protected override void BuildEdit(int editIdx, VPPXmlDocument doc, VPPXmlNode orig, VPPXmlNode sample, array<string> lines, string indent, string childIndent, bool firstBare, array<string> outLines)
	{
		string groupChildIndent = VPPXEGroupsIO.ChildIndentOf(lines, sample, indent, "");
		VPPXEGroupsIO.BuildBlock(m_Save.Edits[editIdx].Row, doc, orig, indent, groupChildIndent, firstBare, outLines);
	}

	protected override string SummaryName()
	{
		return "groups";
	}

	protected override string SavedKey()
	{
		return "#VSTR_XMLE_GRP_SAVED";
	}
};
