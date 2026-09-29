// XML Editor SPAWNABLES tab: wire data and the rules of cfgspawnabletypes.xml and cfgrandompresets.xml (the vanilla
// files and every <ce type="spawnabletypes"> / <ce type="randompresets"> file), taken from the 1.30 server's own
// loaders (memory: dayz-spawnabletypes-spec):
// - every randompresets file loads before any spawnabletypes file. <cargo name> and <attachments name> presets are two
//   separate namespaces (a cargo block only finds cargo presets); the FIRST preset of a name wins, later ones are
//   ignored; a preset without a loadable item is dropped ("is EMPTY").
// - a block (<cargo> / <attachments> of a type, a preset, or nested in an item): preset="x" copies that preset's items
//   and its chance, a chance attribute overrides it, inline items are appended; no chance (and no preset) or a
//   negative one = 1.0.
// - <item name chance quantmin quantmax equip>: chance defaults to 1.0; quantmin / quantmax are percent 0..100; an item
//   can carry its own <damage> and its own blocks (nested). Items listed in cfgignorelist are dropped.
// - <type name>: must be a Central Economy type, else skipped. <hoarder/>, <unique/>, <damage min max>, <bind name>,
//   <subcounter min max>, <spawn batch>, and blocks. A <damage> right under the root is the file's default damage.
// - the same type again (a later file): hoarder / unique reset only when it names one of them, damage is replaced when
//   it has one, cargo and attachments are each replaced when it has blocks of that kind.
// The wire classes derive from Managed: nodes are held by more than one array (chunk + working copy, edit + part).

// What a node of the tree is. Rows are TYPE nodes (spawnabletypes) or CARGO / ATTACHMENTS nodes (randompresets).
class VPPXESpwKind
{
	const static int OTHER = 0;
	const static int HOARDER = 1;
	const static int UNIQUE = 2;
	const static int DAMAGE = 3;
	const static int BIND = 4;
	const static int SUBCOUNTER = 5;
	const static int SPAWN = 6;
	const static int CARGO = 7;
	const static int ATTACHMENTS = 8;
	const static int ITEM = 9;
	const static int TYPE = 10;

	static bool IsBlock(int kind)
	{
		return kind == CARGO || kind == ATTACHMENTS;
	}

	// The element name of a kind ("" for OTHER).
	static string TagOf(int kind)
	{
		if (kind == HOARDER)
		{
			return "hoarder";
		}

		if (kind == UNIQUE)
		{
			return "unique";
		}

		if (kind == DAMAGE)
		{
			return "damage";
		}

		if (kind == BIND)
		{
			return "bind";
		}

		if (kind == SUBCOUNTER)
		{
			return "subcounter";
		}

		if (kind == SPAWN)
		{
			return "spawn";
		}

		if (kind == CARGO)
		{
			return "cargo";
		}

		if (kind == ATTACHMENTS)
		{
			return "attachments";
		}

		if (kind == ITEM)
		{
			return "item";
		}

		if (kind == TYPE)
		{
			return "type";
		}

		return "";
	}

	// The kind of an element name (case-insensitive), OTHER when unknown.
	static int KindOf(string elementName)
	{
		string lower = elementName;
		lower.ToLower();
		for (int kind = HOARDER; kind <= TYPE; kind++)
		{
			if (TagOf(kind) == lower)
			{
				return kind;
			}
		}

		return OTHER;
	}
};

// One element (or a kept comment / unknown element) of a type or preset. Number attributes are kept as the text written
// in the file ("" = absent), so an untouched value stays exactly as it was.
class VPPXESpwNode : Managed
{
	int Kind;
	// index of the element among the children (elements and comments) of its parent in the file; -1 = added here
	int OrigIdx;
	// rows: the line of the element in the file (1-based)
	int Line;
	// TYPE / ITEM class name, preset name (a randompresets row), BIND name
	string Name;
	string Preset;
	string Chance;
	// DAMAGE / SUBCOUNTER min and max, SPAWN batch (in Min)
	string Min;
	string Max;
	string QuantMin;
	string QuantMax;
	string Equip;
	// attributes the editor does not know, as ' name="value"' text (written back as they are)
	string Extra;
	// OTHER: a short text for the UI (the comment, or the element name)
	string Label;
	ref array<ref VPPXESpwNode> Kids;

	void VPPXESpwNode()
	{
		OrigIdx = -1;
		Name = "";
		Preset = "";
		Chance = "";
		Min = "";
		Max = "";
		QuantMin = "";
		QuantMax = "";
		Equip = "";
		Extra = "";
		Label = "";
		Kids = new array<ref VPPXESpwNode>;
	}

	// Deep copy (the line and the original indexes included).
	void CopyFrom(VPPXESpwNode other)
	{
		if (!other)
		{
			return;
		}

		Kind = other.Kind;
		OrigIdx = other.OrigIdx;
		Line = other.Line;
		Name = other.Name;
		Preset = other.Preset;
		Chance = other.Chance;
		Min = other.Min;
		Max = other.Max;
		QuantMin = other.QuantMin;
		QuantMax = other.QuantMax;
		Equip = other.Equip;
		Extra = other.Extra;
		Label = other.Label;
		Kids.Clear();
		foreach (VPPXESpwNode kid : other.Kids)
		{
			VPPXESpwNode copy = new VPPXESpwNode();
			copy.CopyFrom(kid);
			Kids.Insert(copy);
		}
	}

	// Same content, deep (the line number is ignored; the original indexes count: a moved child is a change).
	bool SameAs(VPPXESpwNode other)
	{
		if (!other || Kind != other.Kind || OrigIdx != other.OrigIdx || Name != other.Name || Preset != other.Preset)
		{
			return false;
		}

		if (Chance != other.Chance || Min != other.Min || Max != other.Max || QuantMin != other.QuantMin)
		{
			return false;
		}

		if (QuantMax != other.QuantMax || Equip != other.Equip || Extra != other.Extra || Label != other.Label)
		{
			return false;
		}

		int count = Kids.Count();
		if (count != other.Kids.Count())
		{
			return false;
		}

		for (int i = 0; i < count; i++)
		{
			VPPXESpwNode mine = Kids[i];
			VPPXESpwNode theirs = other.Kids[i];
			if (!mine || !mine.SameAs(theirs))
			{
				return false;
			}
		}

		return true;
	}

	// The first child of a kind (null when none).
	VPPXESpwNode FirstKid(int kind)
	{
		foreach (VPPXESpwNode kid : Kids)
		{
			if (kid && kid.Kind == kind)
			{
				return kid;
			}
		}

		return null;
	}

	bool HasKid(int kind)
	{
		return FirstKid(kind) != null;
	}

	// Children that are blocks (cargo and attachments), in file order.
	int BlockCount()
	{
		int count = 0;
		foreach (VPPXESpwNode kid : Kids)
		{
			if (kid && VPPXESpwKind.IsBlock(kid.Kind))
			{
				count++;
			}
		}

		return count;
	}

	int CountKind(int kind)
	{
		int count = 0;
		foreach (VPPXESpwNode kid : Kids)
		{
			if (kid && kid.Kind == kind)
			{
				count++;
			}
		}

		return count;
	}
};

class VPPXESpawnablesChunk : Managed
{
	int ReqId;
	int FileIdx;
	int FileCount;
	int ChunkIdx;
	int ChunkCount;
	string FileKey;
	// VPPXEFileKind.SPAWNABLETYPES or RANDOMPRESETS
	int Kind;
	int Revision;
	bool Editable;
	string ErrorKey;
	// spawnabletypes: the <damage> right under the root ("" when the file has none)
	string FileDmgMin;
	string FileDmgMax;
	// first chunk of the reply only: the lowercase names of cfgignorelist.xml
	ref array<string> Ignored;
	ref array<ref VPPXESpwNode> Rows;

	void VPPXESpawnablesChunk()
	{
		FileKey = "";
		ErrorKey = "";
		FileDmgMin = "";
		FileDmgMax = "";
		Ignored = new array<string>;
		Rows = new array<ref VPPXESpwNode>;
	}
};

class VPPXESpwEdit : Managed
{
	int Op;
	int Index;
	ref VPPXESpwNode Row;

	void VPPXESpwEdit()
	{
		Row = new VPPXESpwNode();
	}
};

class VPPXESpawnablesSave : Managed
{
	int ReqId;
	int PartIdx;
	int PartCount;
	string FileKey;
	int BaseRevision;
	ref array<ref VPPXESpwEdit> Edits;

	void VPPXESpawnablesSave()
	{
		FileKey = "";
		Edits = new array<ref VPPXESpwEdit>;
	}
};

class VPPXESpwRules
{
	const static int MAX_ROWS = 8000;
	// blocks inside items inside blocks: type > block > item > block > item > block
	// node levels below a row: type 0, block 1, item 2, nested block 3, item 4, block 5, item 6, its damage 7
	const static int MAX_DEPTH = 8;
	const static int MAX_KIDS = 300;
	const static int CHUNK_BYTES = 12000;
	const static int EDITS_PER_PART = 20;

	// The children a node of this kind may have (depth: 0 = a row).
	static bool AllowsKid(int parentKind, int kidKind)
	{
		if (kidKind == VPPXESpwKind.OTHER)
		{
			return true;
		}

		if (parentKind == VPPXESpwKind.TYPE)
		{
			return kidKind != VPPXESpwKind.ITEM && kidKind != VPPXESpwKind.TYPE;
		}

		if (VPPXESpwKind.IsBlock(parentKind))
		{
			return kidKind == VPPXESpwKind.ITEM;
		}

		if (parentKind == VPPXESpwKind.ITEM)
		{
			return kidKind == VPPXESpwKind.DAMAGE || VPPXESpwKind.IsBlock(kidKind);
		}

		return false;
	}

	// "" when the row can be written, else the #key of the first problem. presetsFile: a randompresets row.
	static string CheckRow(VPPXESpwNode row, bool presetsFile)
	{
		if (!row)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (presetsFile)
		{
			if (!VPPXESpwKind.IsBlock(row.Kind))
			{
				return "#VSTR_XMLE_ERR_VALIDATION";
			}

			if (!IsPlainName(row.Name))
			{
				return "#VSTR_XMLE_SPB_ERR_PRESET_NAME";
			}
		}
		else
		{
			if (row.Kind != VPPXESpwKind.TYPE)
			{
				return "#VSTR_XMLE_ERR_VALIDATION";
			}

			if (!VPPXmlText.IsValidClassName(row.Name))
			{
				return "#VSTR_XMLE_SPB_ERR_TYPE_NAME";
			}
		}

		return CheckNode(row, 0);
	}

	protected static string CheckNode(VPPXESpwNode node, int depth)
	{
		if (depth > MAX_DEPTH)
		{
			return "#VSTR_XMLE_SPB_ERR_DEPTH";
		}

		if (node.Kids.Count() > MAX_KIDS)
		{
			return "#VSTR_XMLE_SPB_ERR_TOO_MANY";
		}

		string fieldProblem = CheckFields(node);
		if (fieldProblem != "")
		{
			return fieldProblem;
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (!kid || !AllowsKid(node.Kind, kid.Kind))
			{
				return "#VSTR_XMLE_ERR_VALIDATION";
			}

			// a kept comment or unknown element can only be one that is in the file
			if (kid.Kind == VPPXESpwKind.OTHER && kid.OrigIdx < 0)
			{
				return "#VSTR_XMLE_ERR_VALIDATION";
			}

			if (kid.Kind == VPPXESpwKind.OTHER)
			{
				continue;
			}

			string kidProblem = CheckNode(kid, depth + 1);
			if (kidProblem != "")
			{
				return kidProblem;
			}
		}

		return "";
	}

	// The attributes of one node (numbers readable, names usable).
	protected static string CheckFields(VPPXESpwNode node)
	{
		if (!IsOptionalNumber(node.Chance) || !IsOptionalNumber(node.Min) || !IsOptionalNumber(node.Max))
		{
			return "#VSTR_XMLE_SPB_ERR_NUMBER";
		}

		if (!IsOptionalNumber(node.QuantMin) || !IsOptionalNumber(node.QuantMax))
		{
			return "#VSTR_XMLE_SPB_ERR_NUMBER";
		}

		if (node.Kind == VPPXESpwKind.ITEM && !VPPXmlText.IsValidClassName(node.Name))
		{
			return "#VSTR_XMLE_SPB_ERR_ITEM_NAME";
		}

		if (node.Kind == VPPXESpwKind.BIND && !IsPlainName(node.Name))
		{
			return "#VSTR_XMLE_SPB_ERR_BIND";
		}

		if (node.Preset != "" && !IsPlainName(node.Preset))
		{
			return "#VSTR_XMLE_SPB_ERR_PRESET_NAME";
		}

		if (node.Equip != "" && node.Equip != "1" && node.Equip != "0" && node.Equip != "true" && node.Equip != "false")
		{
			return "#VSTR_XMLE_SPB_ERR_EQUIP";
		}

		return "";
	}

	static bool IsOptionalNumber(string text)
	{
		return text == "" || VPPXESpawnRules.IsNumberText(text);
	}

	// A name an attribute can hold: not empty, at most MAX_NAME_LENGTH, no line break or quote.
	static bool IsPlainName(string text)
	{
		int len = text.Length();
		if (len == 0 || len > VPPXEConst.MAX_NAME_LENGTH)
		{
			return false;
		}

		return text.IndexOf("\"") < 0 && text.IndexOf("\n") < 0 && text.IndexOf("\r") < 0 && text.IndexOf("<") < 0 && text.IndexOf(">") < 0;
	}

	// ---------------------------------------------------------------- engine semantics (for the checks and views)

	// A chance attribute the way the server reads it: missing or unreadable = fallback, below 0 = 1.0.
	static float ChanceOf(string text, float fallback)
	{
		float value = fallback;
		if (text != "" && VPPXESpawnRules.IsNumberText(text))
		{
			value = text.ToFloat();
		}

		if (value < 0)
		{
			value = 1.0;
		}

		return value;
	}

	// The number of an optional attribute, fallback when missing or unreadable.
	static float NumberOf(string text, float fallback)
	{
		if (text == "" || !VPPXESpawnRules.IsNumberText(text))
		{
			return fallback;
		}

		return text.ToFloat();
	}

	// Byte estimate of a node on the wire (chunking).
	static int EstimateNode(VPPXESpwNode node)
	{
		if (!node)
		{
			return 0;
		}

		int size = 60 + node.Name.Length() + node.Preset.Length() + node.Chance.Length() + node.Min.Length() + node.Max.Length();
		size += node.QuantMin.Length() + node.QuantMax.Length() + node.Equip.Length() + node.Extra.Length() + node.Label.Length();
		foreach (VPPXESpwNode kid : node.Kids)
		{
			size += EstimateNode(kid);
		}

		return size;
	}

	// A number for display and for new attributes: at most 3 decimals, no trailing zeros ("0.5", "1", "0.125").
	static string FormatNumber(float value)
	{
		return VPPXEGroupTransform.FormatNumber(value);
	}
};

// The server's runtime roll (memory: dayz-spawnabletypes-spec, "Runtime roll"): a block spawns AT MOST ONE item. One
// uniform roll against the block chance, then a second one walked against the running sum of the item chances in list
// order (a preset's items first, then the block's own): the first item whose running sum reaches the roll spawns;
// when the sum never reaches it, nothing does. Items with the default chance 1 therefore always give the first one.
class VPPXESpwRoll
{
	// The chance of an item in its block's list (missing = 1, not clamped: the server keeps it as written).
	static float ItemChanceOf(VPPXESpwNode item)
	{
		return VPPXESpwRules.NumberOf(item.Chance, 1.0);
	}

	// Per item, how likely the block spawns that item: the block chance times the part of [0, 1) the item covers.
	static void Odds(array<VPPXESpwNode> items, float blockChance, array<float> outOdds)
	{
		outOdds.Clear();
		float gate = Math.Clamp(blockChance, 0, 1);
		float sum = 0;
		float reached = 0;
		foreach (VPPXESpwNode item : items)
		{
			float itemChance = ItemChanceOf(item);
			sum = sum + itemChance;
			float top = Math.Clamp(sum, 0, 1);
			float share = top - reached;
			if (share < 0)
			{
				share = 0;
			}

			outOdds.Insert(gate * share);
			if (top > reached)
			{
				reached = top;
			}
		}
	}

	// One roll of a block like the server does it: the index of the item that spawns, -1 for none.
	static int Pick(array<VPPXESpwNode> items, float blockChance)
	{
		if (items.Count() == 0)
		{
			return -1;
		}

		float gate = Math.RandomFloat01();
		if (gate > blockChance)
		{
			return -1;
		}

		float roll = Math.RandomFloat01();
		float sum = 0;
		for (int i = 0; i < items.Count(); i++)
		{
			float itemChance = ItemChanceOf(items[i]);
			sum = sum + itemChance;
			if (sum >= roll)
			{
				return i;
			}
		}

		return -1;
	}

	// The quantity range (percent) the server applies to an item: false when it applies none. It needs both ends
	// after its own fix-ups: a quantmin alone gets quantmax 100, a quantmax alone is ignored.
	static bool QuantRange(VPPXESpwNode item, out float minPct, out float maxPct)
	{
		float quantMax = VPPXESpwRules.NumberOf(item.QuantMax, -1);
		float quantMin = VPPXESpwRules.NumberOf(item.QuantMin, -1);
		if (quantMax != -1)
		{
			quantMax = Math.Clamp(quantMax, 0, 100);
		}

		if (quantMin != -1)
		{
			if (quantMax == -1)
			{
				quantMax = 100;
			}

			quantMin = Math.Clamp(quantMin, 0, quantMax);
		}

		minPct = quantMin;
		maxPct = quantMax;
		return quantMin >= 0 && quantMax >= 0;
	}

	// A percent as the UI shows it: "45%", "0.5%", "<0.1%".
	static string Percent(float fraction)
	{
		float pct = fraction * 100;
		if (pct <= 0)
		{
			return "0%";
		}

		if (pct < 0.1)
		{
			return "<0.1%";
		}

		if (pct < 10)
		{
			float tenths = Math.Round(pct * 10) / 10;
			string small = VPPXESpwRules.FormatNumber(tenths);
			return small + "%";
		}

		int whole = Math.Round(pct);
		return whole.ToString() + "%";
	}
};

// Where the test spawn put each item (PLACE_ATTACHMENT / PLACE_CARGO / PLACE_OTHER), in inventory order.
class VPPXESpwTestResult : Managed
{
	const static int PLACE_ATTACHMENT = 0;
	const static int PLACE_CARGO = 1;
	const static int PLACE_OTHER = 2;

	int ReqId;
	string TypeName;
	ref array<string> Names;
	ref array<string> Parents;
	ref array<int> Places;
	// quantity as the server set it ("45%", "" when the item has none)
	ref array<string> Details;
	// an infected / animal: only its attachments were spawned, the CE fills its cargo when it dies
	bool CargoOnDeath;

	void VPPXESpwTestResult()
	{
		TypeName = "";
		Names = new array<string>;
		Parents = new array<string>;
		Places = new array<int>;
		Details = new array<string>;
	}
};
