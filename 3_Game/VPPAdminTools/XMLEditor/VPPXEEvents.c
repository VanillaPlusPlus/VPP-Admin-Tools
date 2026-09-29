// XML Editor EVENTS tab: wire data and the rules of the dynamic events files (db/events.xml and every
// <ce type="events"> file), taken from the 1.30 server's own loader (memory: dayz-events-xml-spec):
// - files load in order (db/events.xml, then the <ce> files); an event of a name defined earlier is MERGED element by
//   element: an element left out keeps the earlier value, except <flags>, which falls back to 0 when left out.
//   Children merge by type (a later file updates or adds a child, it cannot remove one). Names match exactly.
// - <active>: 1 or left out = the event loads; anything else deletes the event of that name (also one from an earlier
//   file).
// - numbers are whole numbers (unreadable = left out); negatives become 0; min above nominal is lowered to nominal
//   unless limit is custom. <flags> attributes deletable, init_random, remove_damaged are on only as "1".
// - <position> fixed / player / uniform, <limit> child / mixed / custom / parent. <secondary> names another event
//   exactly. A <child> needs a registered type; max below min and lootmax below lootmin are raised; deloot (-1),
//   spawnsecondary (on) and trace (off) are read too.
// - the name prefix picks the spawner (Vehicle, Static, Loot, Infected, Animal, Ambient, Item, Trajectory, case
//   sensitive); any other name is ignored.
// - at start an event is disabled when: Vehicle / Static / Item without position player and without positions in
//   cfgeventspawns.xml; limit parent or mixed with max 0; limit child or mixed with every child max 0; no children
//   (Vehicle / Static / Item may use event groups on their positions instead; Infected / Animal / Ambient /
//   Trajectory always need one). Loot events are always valid.
// The wire classes derive from Managed (held by chunk and working copy, part and assembled save).

enum VPPXEEventField
{
	NOMINAL = 1,
	MIN = 2,
	MAX = 4,
	LIFETIME = 8,
	RESTOCK = 16,
	SAFERADIUS = 32,
	DISTANCERADIUS = 64,
	CLEANUPRADIUS = 128,
	SECONDARY = 256,
	FLAGS = 512,
	POSITION = 1024,
	LIMIT = 2048,
	ACTIVE = 4096,
	CHILDREN = 8192
};

enum VPPXEEventFlag
{
	DELETABLE = 1,
	INIT_RANDOM = 2,
	REMOVE_DAMAGED = 4
};

class VPPXEEventChild : Managed
{
	int Line;
	string Type;
	int Min;
	int Max;
	int LootMin;
	int LootMax;
	// -1 = not written (the server default -1)
	int Deloot;
	// -1 = not written (server default on), else 0 / 1
	int SpawnSecondary;
	// -1 = not written (server default off), else 0 / 1
	int Trace;
	// attributes the editor does not know, written back as they were (' name="value"' pairs)
	string ExtraAttrs;

	void VPPXEEventChild()
	{
		Type = "";
		Deloot = -1;
		SpawnSecondary = -1;
		Trace = -1;
		ExtraAttrs = "";
	}

	void CopyFrom(VPPXEEventChild other)
	{
		if (!other)
		{
			return;
		}

		Line = other.Line;
		Type = other.Type;
		Min = other.Min;
		Max = other.Max;
		LootMin = other.LootMin;
		LootMax = other.LootMax;
		Deloot = other.Deloot;
		SpawnSecondary = other.SpawnSecondary;
		Trace = other.Trace;
		ExtraAttrs = other.ExtraAttrs;
	}

	// Same child content (the line number is ignored).
	bool SameAs(VPPXEEventChild other)
	{
		if (!other)
		{
			return false;
		}

		if (Type != other.Type || Min != other.Min || Max != other.Max)
		{
			return false;
		}

		if (LootMin != other.LootMin || LootMax != other.LootMax || Deloot != other.Deloot)
		{
			return false;
		}

		if (SpawnSecondary != other.SpawnSecondary || Trace != other.Trace)
		{
			return false;
		}

		return ExtraAttrs == other.ExtraAttrs;
	}
};

class VPPXEEventRow : Managed
{
	int Line;
	string Name;
	// attributes of <event> besides name, written back as they were (' name="value"' pairs)
	string EventExtra;
	// VPPXEEventField bits of the elements the event writes (a left-out element keeps an earlier file's value)
	int Present;
	int Nominal;
	int Min;
	int Max;
	int Lifetime;
	int Restock;
	int SafeRadius;
	int DistanceRadius;
	int CleanupRadius;
	string Secondary;
	// VPPXEEventFlag bits
	int Flags;
	// attributes of <flags> the editor does not know (' sec_spawner="0"'), written back as they were
	string FlagsExtra;
	string Position;
	string Limit;
	int Active;
	// elements and comments inside the event the editor does not know (the server keeps them as they are)
	int ExtraCount;
	ref array<ref VPPXEEventChild> Children;

	void VPPXEEventRow()
	{
		Name = "";
		EventExtra = "";
		Secondary = "";
		FlagsExtra = "";
		Position = "";
		Limit = "";
		Active = 1;
		Children = new array<ref VPPXEEventChild>;
	}

	bool Has(int field)
	{
		return (Present & field) != 0;
	}

	void SetPresent(int field, bool present)
	{
		if (present)
		{
			Present = Present | field;
		}
		else
		{
			Present = Present & ~field;
		}
	}

	// The whole-number elements by VPPXEEventField bit (0 for any other bit).
	int GetNumber(int field)
	{
		if (field == VPPXEEventField.NOMINAL)
		{
			return Nominal;
		}

		if (field == VPPXEEventField.MIN)
		{
			return Min;
		}

		if (field == VPPXEEventField.MAX)
		{
			return Max;
		}

		if (field == VPPXEEventField.LIFETIME)
		{
			return Lifetime;
		}

		if (field == VPPXEEventField.RESTOCK)
		{
			return Restock;
		}

		if (field == VPPXEEventField.SAFERADIUS)
		{
			return SafeRadius;
		}

		if (field == VPPXEEventField.DISTANCERADIUS)
		{
			return DistanceRadius;
		}

		if (field == VPPXEEventField.CLEANUPRADIUS)
		{
			return CleanupRadius;
		}

		return 0;
	}

	void SetNumber(int field, int value)
	{
		if (field == VPPXEEventField.NOMINAL)
		{
			Nominal = value;
		}
		else if (field == VPPXEEventField.MIN)
		{
			Min = value;
		}
		else if (field == VPPXEEventField.MAX)
		{
			Max = value;
		}
		else if (field == VPPXEEventField.LIFETIME)
		{
			Lifetime = value;
		}
		else if (field == VPPXEEventField.RESTOCK)
		{
			Restock = value;
		}
		else if (field == VPPXEEventField.SAFERADIUS)
		{
			SafeRadius = value;
		}
		else if (field == VPPXEEventField.DISTANCERADIUS)
		{
			DistanceRadius = value;
		}
		else if (field == VPPXEEventField.CLEANUPRADIUS)
		{
			CleanupRadius = value;
		}
	}

	void CopyFrom(VPPXEEventRow other)
	{
		if (!other)
		{
			return;
		}

		Line = other.Line;
		Name = other.Name;
		EventExtra = other.EventExtra;
		Present = other.Present;
		Nominal = other.Nominal;
		Min = other.Min;
		Max = other.Max;
		Lifetime = other.Lifetime;
		Restock = other.Restock;
		SafeRadius = other.SafeRadius;
		DistanceRadius = other.DistanceRadius;
		CleanupRadius = other.CleanupRadius;
		Secondary = other.Secondary;
		Flags = other.Flags;
		FlagsExtra = other.FlagsExtra;
		Position = other.Position;
		Limit = other.Limit;
		Active = other.Active;
		ExtraCount = other.ExtraCount;
		Children = new array<ref VPPXEEventChild>;
		if (!other.Children)
		{
			return;
		}

		foreach (VPPXEEventChild otherChild : other.Children)
		{
			VPPXEEventChild copy = new VPPXEEventChild();
			copy.CopyFrom(otherChild);
			Children.Insert(copy);
		}
	}

	// Same event content (line numbers are ignored; children compared in order).
	bool SameAs(VPPXEEventRow other)
	{
		if (!other)
		{
			return false;
		}

		if (Name != other.Name || EventExtra != other.EventExtra || Present != other.Present || Nominal != other.Nominal || Min != other.Min)
		{
			return false;
		}

		if (Max != other.Max || Lifetime != other.Lifetime || Restock != other.Restock || SafeRadius != other.SafeRadius)
		{
			return false;
		}

		if (DistanceRadius != other.DistanceRadius || CleanupRadius != other.CleanupRadius || Secondary != other.Secondary)
		{
			return false;
		}

		if (Flags != other.Flags || FlagsExtra != other.FlagsExtra || Position != other.Position || Limit != other.Limit)
		{
			return false;
		}

		if (Active != other.Active || Children.Count() != other.Children.Count())
		{
			return false;
		}

		for (int i = 0; i < Children.Count(); i++)
		{
			VPPXEEventChild mine = Children[i];
			if (!mine.SameAs(other.Children[i]))
			{
				return false;
			}
		}

		return true;
	}
};

class VPPXEEventsChunk : Managed
{
	int ReqId;
	int FileIdx;
	int FileCount;
	int ChunkIdx;
	int ChunkCount;
	string FileKey;
	int Revision;
	bool Editable;
	string ErrorKey;
	ref array<ref VPPXEEventRow> Rows;

	void VPPXEEventsChunk()
	{
		FileKey = "";
		ErrorKey = "";
		Rows = new array<ref VPPXEEventRow>;
	}
};

// cfgeventspawns.xml (Phase 2): one <event name> entry with its <zone> and <pos> elements. Numbers stay text so an
// untouched position is written back exactly as it was (the server copies it from the original line when its values
// did not change; OrigIdx = its index in the entry as read, -1 = new). A / Y / Group "" = the attribute is left out.
class VPPXESpawnPos : Managed
{
	int OrigIdx;
	string X;
	string Z;
	string A;
	string Y;
	string Group;
	// attributes the editor does not know, written back as they were
	string Extra;

	void VPPXESpawnPos()
	{
		OrigIdx = -1;
		X = "";
		Z = "";
		A = "";
		Y = "";
		Group = "";
		Extra = "";
	}

	void CopyFrom(VPPXESpawnPos other)
	{
		if (!other)
		{
			return;
		}

		OrigIdx = other.OrigIdx;
		X = other.X;
		Z = other.Z;
		A = other.A;
		Y = other.Y;
		Group = other.Group;
		Extra = other.Extra;
	}

	// Same values (OrigIdx is ignored).
	bool SameAs(VPPXESpawnPos other)
	{
		if (!other)
		{
			return false;
		}

		if (X != other.X || Z != other.Z || A != other.A || Y != other.Y)
		{
			return false;
		}

		return Group == other.Group && Extra == other.Extra;
	}
};

class VPPXESpawnRow : Managed
{
	int Line;
	string Name;
	// attributes of <event> besides name
	string NameExtra;
	bool HasZone;
	string SMin;
	string SMax;
	string DMin;
	string DMax;
	string R;
	string ZoneExtra;
	// elements and comments inside the entry the editor does not know (kept as they are)
	int ExtraCount;
	ref array<ref VPPXESpawnPos> Positions;

	void VPPXESpawnRow()
	{
		Name = "";
		NameExtra = "";
		SMin = "";
		SMax = "";
		DMin = "";
		DMax = "";
		R = "";
		ZoneExtra = "";
		Positions = new array<ref VPPXESpawnPos>;
	}

	void CopyFrom(VPPXESpawnRow other)
	{
		if (!other)
		{
			return;
		}

		Line = other.Line;
		Name = other.Name;
		NameExtra = other.NameExtra;
		HasZone = other.HasZone;
		SMin = other.SMin;
		SMax = other.SMax;
		DMin = other.DMin;
		DMax = other.DMax;
		R = other.R;
		ZoneExtra = other.ZoneExtra;
		ExtraCount = other.ExtraCount;
		Positions = new array<ref VPPXESpawnPos>;
		if (!other.Positions)
		{
			return;
		}

		foreach (VPPXESpawnPos otherPos : other.Positions)
		{
			VPPXESpawnPos copy = new VPPXESpawnPos();
			copy.CopyFrom(otherPos);
			Positions.Insert(copy);
		}
	}

	bool ZoneSameAs(VPPXESpawnRow other)
	{
		if (HasZone != other.HasZone)
		{
			return false;
		}

		if (!HasZone)
		{
			return true;
		}

		return SMin == other.SMin && SMax == other.SMax && DMin == other.DMin && DMax == other.DMax && R == other.R && ZoneExtra == other.ZoneExtra;
	}

	bool SameAs(VPPXESpawnRow other)
	{
		if (!other || Name != other.Name || NameExtra != other.NameExtra || !ZoneSameAs(other))
		{
			return false;
		}

		if (Positions.Count() != other.Positions.Count())
		{
			return false;
		}

		for (int i = 0; i < Positions.Count(); i++)
		{
			VPPXESpawnPos mine = Positions[i];
			if (!mine.SameAs(other.Positions[i]))
			{
				return false;
			}
		}

		return true;
	}
};

// XE_GetEvents also sends cfgeventspawns.xml (before the events files), in chunks of rows; Groups carries the
// <group name> list of cfgeventgroups.xml (spread over the chunks, GROUPS_PER_CHUNK each).
class VPPXESpawnsChunk : Managed
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	string FileKey;
	int Revision;
	bool Editable;
	string ErrorKey;
	string GroupsError;
	ref array<ref VPPXESpawnRow> Rows;
	ref array<string> Groups;

	void VPPXESpawnsChunk()
	{
		FileKey = "";
		ErrorKey = "";
		GroupsError = "";
		Rows = new array<ref VPPXESpawnRow>;
		Groups = new array<string>;
	}
};

class VPPXESpawnEdit : Managed
{
	int Op;
	int Index;
	ref VPPXESpawnRow Row;

	void VPPXESpawnEdit()
	{
		Row = new VPPXESpawnRow();
	}
};

class VPPXESpawnsSave : Managed
{
	int ReqId;
	int PartIdx;
	int PartCount;
	string FileKey;
	int BaseRevision;
	ref array<ref VPPXESpawnEdit> Edits;

	void VPPXESpawnsSave()
	{
		FileKey = "";
		Edits = new array<ref VPPXESpawnEdit>;
	}
};

// cfgeventspawns.xml rules of the 1.30 server (sub_1406BC1A0): a <pos> with x or z below 0 (or missing) is skipped; a
// negative a means a random angle; y is optional; a group that cfgeventgroups.xml does not define is dropped from the
// position. A <zone> is used only when smin and dmin are above -1, smax >= smin, dmax >= dmin and r >= 0.
class VPPXESpawnRules
{
	const static int MAX_ROWS = 5000;
	const static int MAX_POSITIONS = 3000;
	const static int CHUNK_BYTES = 12000;
	const static int EDITS_PER_PART = 10;
	const static int GROUPS_PER_CHUNK = 300;
	const static string DIGITS = "0123456789";

	// A number as the server's strtod reads it here: optional sign, digits, optional fraction (no exponent).
	static bool IsNumberText(string text)
	{
		string trimmed = text.Trim();
		int len = trimmed.Length();
		int digits = 0;
		bool dot = false;
		if (len == 0 || len > 24)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			string ch = trimmed.Get(i);
			if (DIGITS.IndexOf(ch) >= 0)
			{
				digits++;
				continue;
			}

			if ((ch == "-" || ch == "+") && i == 0)
			{
				continue;
			}

			if (ch == "." && !dot)
			{
				dot = true;
				continue;
			}

			return false;
		}

		return digits > 0;
	}

	// A whole number (zone smin / smax / dmin / dmax), sign allowed.
	static bool IsIntText(string text)
	{
		string trimmed = text.Trim();
		int len = trimmed.Length();
		int digits = 0;
		if (len == 0 || len > 10)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			string ch = trimmed.Get(i);
			if (DIGITS.IndexOf(ch) >= 0)
			{
				digits++;
				continue;
			}

			if ((ch == "-" || ch == "+") && i == 0)
			{
				continue;
			}

			return false;
		}

		return digits > 0;
	}

	// "" when the entry can be saved, else the #key of the first problem. orig (may be null) = the entry as read: its
	// zone and positions that did not change are not checked (they are written back as they were).
	static string CheckRow(VPPXESpawnRow row, VPPXESpawnRow orig)
	{
		if (!row)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (!VPPXmlText.IsValidClassName(row.Name))
		{
			return "#VSTR_XMLE_EVT_ERR_NAME";
		}

		if (row.Positions.Count() > MAX_POSITIONS)
		{
			return "#VSTR_XMLE_SPW_ERR_TOO_MANY";
		}

		if (row.HasZone && !(orig && row.ZoneSameAs(orig)))
		{
			if (!IsIntText(row.SMin) || !IsIntText(row.SMax) || !IsIntText(row.DMin) || !IsIntText(row.DMax) || !IsNumberText(row.R))
			{
				return "#VSTR_XMLE_SPW_ERR_ZONE";
			}
		}

		foreach (VPPXESpawnPos pos : row.Positions)
		{
			if (pos && orig && pos.OrigIdx >= 0 && pos.OrigIdx < orig.Positions.Count() && orig.Positions[pos.OrigIdx].SameAs(pos))
			{
				continue;
			}

			if (!pos || !IsNumberText(pos.X) || !IsNumberText(pos.Z))
			{
				return "#VSTR_XMLE_SPW_ERR_POS";
			}

			if ((pos.A != "" && !IsNumberText(pos.A)) || (pos.Y != "" && !IsNumberText(pos.Y)))
			{
				return "#VSTR_XMLE_SPW_ERR_POS";
			}

			if (pos.Group != "" && !VPPXmlText.IsValidClassName(pos.Group))
			{
				return "#VSTR_XMLE_SPW_ERR_GROUP";
			}
		}

		return "";
	}

	// The zone the server keeps ("" = valid or none, else the #key of why it is discarded).
	static string ZoneProblem(VPPXESpawnRow row)
	{
		if (!row.HasZone || !IsIntText(row.SMin) || !IsIntText(row.SMax) || !IsIntText(row.DMin) || !IsIntText(row.DMax) || !IsNumberText(row.R))
		{
			return "";
		}

		int smin = row.SMin.ToInt();
		int smax = row.SMax.ToInt();
		int dmin = row.DMin.ToInt();
		int dmax = row.DMax.ToInt();
		float radius = row.R.ToFloat();
		if (radius < 0 || smin < 0 || dmin < 0 || smax < smin || dmax < dmin)
		{
			return "#VSTR_XMLE_SPW_W_ZONE_DISCARDED";
		}

		return "";
	}

	// The server uses the position (x and z not negative).
	static bool IsUsable(VPPXESpawnPos pos)
	{
		if (!pos || !IsNumberText(pos.X) || !IsNumberText(pos.Z))
		{
			return false;
		}

		return pos.X.ToFloat() >= 0 && pos.Z.ToFloat() >= 0;
	}

	// Byte estimate of a row on the wire (chunking).
	static int EstimateRow(VPPXESpawnRow row)
	{
		int size = 80 + row.Name.Length() + row.NameExtra.Length() + row.ZoneExtra.Length();
		foreach (VPPXESpawnPos pos : row.Positions)
		{
			size += 24 + pos.X.Length() + pos.Z.Length() + pos.A.Length() + pos.Y.Length() + pos.Group.Length() + pos.Extra.Length();
		}

		return size;
	}
};

class VPPXEEventEdit : Managed
{
	int Op;
	int Index;
	ref VPPXEEventRow Row;

	void VPPXEEventEdit()
	{
		Row = new VPPXEEventRow();
	}
};

class VPPXEEventsSave : Managed
{
	int ReqId;
	int PartIdx;
	int PartCount;
	string FileKey;
	int BaseRevision;
	ref array<ref VPPXEEventEdit> Edits;

	void VPPXEEventsSave()
	{
		FileKey = "";
		Edits = new array<ref VPPXEEventEdit>;
	}
};

// One line of the checks: a #key with up to two arguments. Severity 0 = note, 1 = warning, 2 = the server disables
// or ignores the event.
class VPPXEEventNote : Managed
{
	string Key;
	string Arg1;
	string Arg2;
	int Severity;

	void VPPXEEventNote(string key, string arg1, string arg2, int severity)
	{
		Key = key;
		Arg1 = arg1;
		Arg2 = arg2;
		Severity = severity;
	}
};

class VPPXEEventRules
{
	const static int MAX_ROWS = 3000;
	const static int MAX_CHILDREN = 200;
	const static int MAX_NUMBER = 999999999;
	const static int CHUNK_BYTES = 12000;
	const static int EDITS_PER_PART = 20;
	const static int NUMBER_FIELDS = 8;
	const static string DIGITS = "0123456789";

	const static int KIND_UNKNOWN = 0;
	const static int KIND_VEHICLE = 1;
	const static int KIND_STATIC = 2;
	const static int KIND_LOOT = 3;
	const static int KIND_INFECTED = 4;
	const static int KIND_ANIMAL = 5;
	const static int KIND_AMBIENT = 6;
	const static int KIND_ITEM = 7;
	const static int KIND_TRAJECTORY = 8;
	const static int KIND_COUNT = 9;

	const static int SEV_NOTE = 0;
	const static int SEV_WARNING = 1;
	const static int SEV_DISABLED = 2;

	// The whole-number fields in file order.
	static int NumberFieldAt(int index)
	{
		if (index == 0)
		{
			return VPPXEEventField.NOMINAL;
		}

		if (index == 1)
		{
			return VPPXEEventField.MIN;
		}

		if (index == 2)
		{
			return VPPXEEventField.MAX;
		}

		if (index == 3)
		{
			return VPPXEEventField.LIFETIME;
		}

		if (index == 4)
		{
			return VPPXEEventField.RESTOCK;
		}

		if (index == 5)
		{
			return VPPXEEventField.SAFERADIUS;
		}

		if (index == 6)
		{
			return VPPXEEventField.DISTANCERADIUS;
		}

		return VPPXEEventField.CLEANUPRADIUS;
	}

	// Element name of a field.
	static string FieldTag(int field)
	{
		if (field == VPPXEEventField.NOMINAL)
		{
			return "nominal";
		}

		if (field == VPPXEEventField.MIN)
		{
			return "min";
		}

		if (field == VPPXEEventField.MAX)
		{
			return "max";
		}

		if (field == VPPXEEventField.LIFETIME)
		{
			return "lifetime";
		}

		if (field == VPPXEEventField.RESTOCK)
		{
			return "restock";
		}

		if (field == VPPXEEventField.SAFERADIUS)
		{
			return "saferadius";
		}

		if (field == VPPXEEventField.DISTANCERADIUS)
		{
			return "distanceradius";
		}

		if (field == VPPXEEventField.CLEANUPRADIUS)
		{
			return "cleanupradius";
		}

		if (field == VPPXEEventField.SECONDARY)
		{
			return "secondary";
		}

		if (field == VPPXEEventField.FLAGS)
		{
			return "flags";
		}

		if (field == VPPXEEventField.POSITION)
		{
			return "position";
		}

		if (field == VPPXEEventField.LIMIT)
		{
			return "limit";
		}

		if (field == VPPXEEventField.ACTIVE)
		{
			return "active";
		}

		if (field == VPPXEEventField.CHILDREN)
		{
			return "children";
		}

		return "";
	}

	// Spawners that log event groups on their positions as unsupported and spawn only their children.
	static bool KindIgnoresGroups(int kind)
	{
		return kind == KIND_INFECTED || kind == KIND_ANIMAL || kind == KIND_AMBIENT || kind == KIND_TRAJECTORY;
	}

	// The spawner prefix of a kind ("" for KIND_UNKNOWN).
	static string KindName(int kind)
	{
		if (kind == KIND_VEHICLE)
		{
			return "Vehicle";
		}

		if (kind == KIND_STATIC)
		{
			return "Static";
		}

		if (kind == KIND_LOOT)
		{
			return "Loot";
		}

		if (kind == KIND_INFECTED)
		{
			return "Infected";
		}

		if (kind == KIND_ANIMAL)
		{
			return "Animal";
		}

		if (kind == KIND_AMBIENT)
		{
			return "Ambient";
		}

		if (kind == KIND_ITEM)
		{
			return "Item";
		}

		if (kind == KIND_TRAJECTORY)
		{
			return "Trajectory";
		}

		return "";
	}

	// The spawner of an event name: the first prefix it starts with (case sensitive, like the server).
	static int KindOf(string name)
	{
		for (int kind = 1; kind < KIND_COUNT; kind++)
		{
			string prefix = KindName(kind);
			if (name.IndexOf(prefix) == 0)
			{
				return kind;
			}
		}

		return KIND_UNKNOWN;
	}

	// The prefix the name would have with the case fixed ("" when it has none even ignoring case).
	static string CaseFixedKind(string name)
	{
		string lowerName = name;
		lowerName.ToLower();
		for (int kind = 1; kind < KIND_COUNT; kind++)
		{
			string lowerPrefix = KindName(kind);
			lowerPrefix.ToLower();
			if (lowerName.IndexOf(lowerPrefix) == 0)
			{
				return KindName(kind);
			}
		}

		return "";
	}

	static bool IsPositionValue(string value)
	{
		return value == "fixed" || value == "player" || value == "uniform";
	}

	static bool IsLimitValue(string value)
	{
		return value == "child" || value == "mixed" || value == "custom" || value == "parent";
	}

	// Vanilla-like values of a new event of that kind (1.30 chernarus), no children.
	static void ApplyTemplate(VPPXEEventRow row, int kind)
	{
		row.Present = VPPXEEventField.NOMINAL | VPPXEEventField.MIN | VPPXEEventField.MAX | VPPXEEventField.LIFETIME | VPPXEEventField.RESTOCK;
		row.Present = row.Present | VPPXEEventField.SAFERADIUS | VPPXEEventField.DISTANCERADIUS | VPPXEEventField.CLEANUPRADIUS;
		row.Present = row.Present | VPPXEEventField.FLAGS | VPPXEEventField.POSITION | VPPXEEventField.LIMIT | VPPXEEventField.ACTIVE | VPPXEEventField.CHILDREN;
		row.Active = 1;
		row.Restock = 0;
		row.Secondary = "";
		row.FlagsExtra = "";
		row.Children = new array<ref VPPXEEventChild>;
		if (kind == KIND_VEHICLE)
		{
			SetValues(row, 8, 5, 11, 300, 500, 500, 200);
			row.Flags = VPPXEEventFlag.REMOVE_DAMAGED;
			row.Position = "fixed";
			row.Limit = "mixed";
		}
		else if (kind == KIND_INFECTED)
		{
			SetValues(row, 50, 25, 100, 3, 100, 50, 100);
			row.Flags = VPPXEEventFlag.REMOVE_DAMAGED;
			row.Position = "player";
			row.Limit = "custom";
		}
		else if (kind == KIND_ANIMAL)
		{
			SetValues(row, 9, 2, 4, 180, 200, 0, 0);
			row.Flags = VPPXEEventFlag.REMOVE_DAMAGED;
			row.Position = "fixed";
			row.Limit = "child";
		}
		else if (kind == KIND_AMBIENT)
		{
			SetValues(row, 3, 0, 50, 33, 0, 60, 100);
			row.Restock = 15;
			row.Flags = 0;
			row.Position = "fixed";
			row.Limit = "mixed";
		}
		else if (kind == KIND_TRAJECTORY)
		{
			SetValues(row, 140, 2, 4, 180, 25, 100, 25);
			row.Flags = 0;
			row.Position = "player";
			row.Limit = "mixed";
		}
		else if (kind == KIND_ITEM)
		{
			SetValues(row, 50, 40, 50, 7200, 250, 20, 100);
			row.Flags = 0;
			row.Position = "fixed";
			row.Limit = "mixed";
		}
		else if (kind == KIND_LOOT)
		{
			SetValues(row, 0, 0, 0, 0, 0, 0, 50);
			row.Flags = 0;
			row.Position = "fixed";
			row.Limit = "custom";
		}
		else
		{
			SetValues(row, 3, 0, 0, 2100, 1000, 1000, 1000);
			row.Flags = VPPXEEventFlag.DELETABLE;
			row.Position = "fixed";
			row.Limit = "child";
		}
	}

	protected static void SetValues(VPPXEEventRow row, int nominal, int min, int max, int lifetime, int safeRadius, int distanceRadius, int cleanupRadius)
	{
		row.Nominal = nominal;
		row.Min = min;
		row.Max = max;
		row.Lifetime = lifetime;
		row.SafeRadius = safeRadius;
		row.DistanceRadius = distanceRadius;
		row.CleanupRadius = cleanupRadius;
	}

	// "" when the row can be saved, else the #key of the first problem.
	static string CheckRow(VPPXEEventRow row)
	{
		if (!row)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (!VPPXmlText.IsValidClassName(row.Name))
		{
			return "#VSTR_XMLE_EVT_ERR_NAME";
		}

		for (int i = 0; i < NUMBER_FIELDS; i++)
		{
			int field = NumberFieldAt(i);
			int value = row.GetNumber(field);
			if (row.Has(field) && (value < 0 || value > MAX_NUMBER))
			{
				return "#VSTR_XMLE_EVT_ERR_NUMBER";
			}
		}

		if (row.Has(VPPXEEventField.POSITION) && !IsPositionValue(row.Position))
		{
			return "#VSTR_XMLE_EVT_ERR_POSITION";
		}

		if (row.Has(VPPXEEventField.LIMIT) && !IsLimitValue(row.Limit))
		{
			return "#VSTR_XMLE_EVT_ERR_LIMIT";
		}

		if (row.Has(VPPXEEventField.SECONDARY) && !VPPXmlText.IsValidClassName(row.Secondary))
		{
			return "#VSTR_XMLE_EVT_ERR_SECONDARY";
		}

		if (row.Has(VPPXEEventField.ACTIVE) && row.Active != 0 && row.Active != 1)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (row.Children.Count() > MAX_CHILDREN)
		{
			return "#VSTR_XMLE_EVT_ERR_CHILDREN_MAX";
		}

		foreach (VPPXEEventChild child : row.Children)
		{
			if (!child || !VPPXmlText.IsValidClassName(child.Type))
			{
				return "#VSTR_XMLE_EVT_ERR_CHILD_TYPE";
			}

			if (child.Min < 0 || child.Max < 0 || child.LootMin < 0 || child.LootMax < 0 || child.Deloot < -1)
			{
				return "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
			}

			if (child.Min > MAX_NUMBER || child.Max > MAX_NUMBER || child.LootMin > MAX_NUMBER || child.LootMax > MAX_NUMBER || child.Deloot > MAX_NUMBER)
			{
				return "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
			}

			if (child.SpawnSecondary < -1 || child.SpawnSecondary > 1 || child.Trace < -1 || child.Trace > 1)
			{
				return "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
			}
		}

		return "";
	}

	// A whole-number form field: 0..MAX_NUMBER written with digits only (read it with NumberOf). No out parameter:
	// see VPPXEMessagesIO.ParseFile.
	static bool IsNumberText(string text)
	{
		string trimmed = text.Trim();
		int len = trimmed.Length();
		if (len == 0 || len > 9)
		{
			return false;
		}

		for (int i = 0; i < len; i++)
		{
			if (DIGITS.IndexOf(trimmed.Get(i)) < 0)
			{
				return false;
			}
		}

		return true;
	}

	static int NumberOf(string text)
	{
		string trimmed = text.Trim();
		return trimmed.ToInt();
	}

	// Byte estimate of a row on the wire (chunking).
	static int EstimateRow(VPPXEEventRow row)
	{
		int size = 160 + row.Name.Length() + row.EventExtra.Length() + row.Secondary.Length() + row.FlagsExtra.Length() + row.Position.Length() + row.Limit.Length();
		foreach (VPPXEEventChild child : row.Children)
		{
			size += 56 + child.Type.Length() + child.ExtraAttrs.Length();
		}

		return size;
	}

	// Checks of one event against the server's rules. eff = the event after every file merged (null when a later
	// file deletes it). posCount / groupPosCount = its <pos> in cfgeventspawns.xml (-1 = unknown). Row-level notes
	// (this file's definition) first, then what the server does with the merged event at start.
	static void Check(VPPXEEventRow row, VPPXEEventRow eff, int posCount, int groupPosCount, array<ref VPPXEEventNote> outNotes)
	{
		if (!row)
		{
			return;
		}

		int kind = KindOf(row.Name);
		if (kind == KIND_UNKNOWN && row.Name != "")
		{
			string fixedPrefix = CaseFixedKind(row.Name);
			if (fixedPrefix != "")
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_PREFIX_CASE", fixedPrefix, "", SEV_DISABLED));
			}
			else
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_PREFIX", "", "", SEV_DISABLED));
			}
		}

		if (row.Has(VPPXEEventField.POSITION) && !IsPositionValue(row.Position))
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_ERR_POSITION", "", "", SEV_WARNING));
		}

		if (row.Has(VPPXEEventField.LIMIT) && !IsLimitValue(row.Limit))
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_ERR_LIMIT", "", "", SEV_WARNING));
		}

		map<string, bool> seenTypes = new map<string, bool>;
		foreach (VPPXEEventChild child : row.Children)
		{
			if (!child)
			{
				continue;
			}

			string lowerType = child.Type;
			lowerType.ToLower();
			if (seenTypes.Contains(lowerType))
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_DUP", child.Type, "", SEV_WARNING));
			}

			seenTypes.Set(lowerType, true);
			if (child.Max < child.Min)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_MAX_MIN", child.Type, "", SEV_WARNING));
			}

			if (child.LootMax < child.LootMin)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_LOOT", child.Type, "", SEV_WARNING));
			}
		}

		if (!eff)
		{
			return;
		}

		CheckEffective(eff, posCount, groupPosCount, outNotes);
	}

	// What the server does with the merged event at start (sub_1406BAD70 and the load clamps).
	static void CheckEffective(VPPXEEventRow eff, int posCount, int groupPosCount, array<ref VPPXEEventNote> outNotes)
	{
		int kind = KindOf(eff.Name);
		if (kind == KIND_UNKNOWN)
		{
			return;
		}

		string limit = "";
		if (eff.Has(VPPXEEventField.LIMIT))
		{
			limit = eff.Limit;
		}

		bool isPlayer = eff.Has(VPPXEEventField.POSITION) && eff.Position == "player";
		if (limit != "custom" && eff.Min > eff.Nominal)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_MIN_CLAMP", eff.Min.ToString(), eff.Nominal.ToString(), SEV_WARNING));
		}

		if (kind == KIND_LOOT)
		{
			return;
		}

		int childCount = eff.Children.Count();
		bool hasGroups = groupPosCount > 0;
		if (kind == KIND_VEHICLE || kind == KIND_STATIC || kind == KIND_ITEM)
		{
			if (!isPlayer && posCount == 0)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_NO_POSITIONS", "", "", SEV_DISABLED));
			}

			if ((limit == "parent" || limit == "mixed") && eff.Max <= 0)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_MAX_ZERO", limit, "", SEV_DISABLED));
			}

			if ((limit == "child" || limit == "mixed") && !hasGroups)
			{
				CheckChildMax(eff, limit, outNotes);
			}

			if (hasGroups && isPlayer)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_GROUPS_PLAYER", "", "", SEV_WARNING));
			}

			if (childCount == 0 && (!hasGroups || isPlayer))
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_NO_CHILDREN_GROUPS", "", "", SEV_DISABLED));
			}

			return;
		}

		if (hasGroups)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_GROUPS_UNSUPPORTED", KindName(kind), "", SEV_WARNING));
		}

		if (childCount == 0)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_NO_CHILDREN", "", "", SEV_DISABLED));
		}
	}

	protected static void CheckChildMax(VPPXEEventRow eff, string limit, array<ref VPPXEEventNote> outNotes)
	{
		int positive = 0;
		foreach (VPPXEEventChild child : eff.Children)
		{
			if (child.Max > 0 || child.Min > 0)
			{
				positive++;
			}
		}

		if (eff.Children.Count() > 0 && positive == 0)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_MAX_ALL", limit, "", SEV_DISABLED));
			return;
		}

		foreach (VPPXEEventChild zeroChild : eff.Children)
		{
			if (zeroChild.Max <= 0 && zeroChild.Min <= 0)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_MAX_ONE", zeroChild.Type, limit, SEV_WARNING));
			}
		}
	}

	// Worst severity of a list (-1 when empty).
	static int WorstSeverity(array<ref VPPXEEventNote> notes)
	{
		int worst = -1;
		foreach (VPPXEEventNote note : notes)
		{
			if (note.Severity > worst)
			{
				worst = note.Severity;
			}
		}

		return worst;
	}
};

// The merged view of every events file, the way the server loads them (one entry per event name still loaded after
// the last file). Built by the client from the working copies, so unsaved edits count.
class VPPXEEventMerge : Managed
{
	ref map<string, ref VPPXEEventRow> Effective;
	// event name -> file keys that define it (in load order), and the file whose active 0 deleted it last
	ref map<string, ref array<string>> DefFiles;
	ref map<string, string> RemovedBy;

	void VPPXEEventMerge()
	{
		Effective = new map<string, ref VPPXEEventRow>;
		DefFiles = new map<string, ref array<string>>;
		RemovedBy = new map<string, string>;
	}

	void Clear()
	{
		Effective.Clear();
		DefFiles.Clear();
		RemovedBy.Clear();
	}

	// Applies one file's definition of an event (call per row, files in load order, rows in file order).
	void Apply(string fileKey, VPPXEEventRow row)
	{
		if (!row || row.Name == "")
		{
			return;
		}

		array<string> files = DefFiles.Get(row.Name);
		if (!files)
		{
			files = new array<string>;
			DefFiles.Set(row.Name, files);
		}

		if (files.Find(fileKey) < 0)
		{
			files.Insert(fileKey);
		}

		if (row.Has(VPPXEEventField.ACTIVE) && row.Active != 1)
		{
			Effective.Remove(row.Name);
			RemovedBy.Set(row.Name, fileKey);
			return;
		}

		RemovedBy.Remove(row.Name);
		VPPXEEventRow eff = Effective.Get(row.Name);
		if (!eff)
		{
			eff = new VPPXEEventRow();
			eff.Name = row.Name;
			eff.Active = 1;
			Effective.Set(row.Name, eff);
		}

		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			if (row.Has(field))
			{
				eff.SetNumber(field, row.GetNumber(field));
				eff.SetPresent(field, true);
			}
		}

		if (eff.Nominal < 0)
		{
			eff.Nominal = 0;
		}

		if (row.Has(VPPXEEventField.SECONDARY))
		{
			eff.Secondary = row.Secondary;
			eff.SetPresent(VPPXEEventField.SECONDARY, true);
		}

		// flags are always set: a left-out <flags> means 0
		eff.Flags = 0;
		if (row.Has(VPPXEEventField.FLAGS))
		{
			eff.Flags = row.Flags;
		}

		eff.SetPresent(VPPXEEventField.FLAGS, true);
		// an unknown <position> counts as fixed, an unknown <limit> clears the limit (the server's parse)
		if (row.Has(VPPXEEventField.POSITION))
		{
			eff.Position = "fixed";
			if (VPPXEEventRules.IsPositionValue(row.Position))
			{
				eff.Position = row.Position;
			}

			eff.SetPresent(VPPXEEventField.POSITION, true);
		}

		if (row.Has(VPPXEEventField.LIMIT))
		{
			eff.Limit = "";
			eff.SetPresent(VPPXEEventField.LIMIT, false);
			if (VPPXEEventRules.IsLimitValue(row.Limit))
			{
				eff.Limit = row.Limit;
				eff.SetPresent(VPPXEEventField.LIMIT, true);
			}
		}

		foreach (VPPXEEventChild child : row.Children)
		{
			MergeChild(eff, child);
		}
	}

	protected void MergeChild(VPPXEEventRow eff, VPPXEEventChild child)
	{
		if (!child || child.Type == "")
		{
			return;
		}

		string lowerType = child.Type;
		lowerType.ToLower();
		foreach (VPPXEEventChild existing : eff.Children)
		{
			string existingLower = existing.Type;
			existingLower.ToLower();
			if (existingLower == lowerType)
			{
				existing.CopyFrom(child);
				return;
			}
		}

		VPPXEEventChild added = new VPPXEEventChild();
		added.CopyFrom(child);
		eff.Children.Insert(added);
	}
};

// ---------------------------------------------------------------- cfgeventgroups.xml (GROUPS view)
// The 1.30 server (sub_1406BC1A0, spawn: sub_1406BEF00): a <group name> of <child type x z [y] [a] lootmin lootmax
// deloot spawnsecondary trace>. The first valid group of a name wins; a group without a valid child is dropped. A
// child needs a registered type (else "Skipping pos ... with invalid type"); lootmin below 0 is 0, lootmax below
// lootmin is lootmin, deloot below -1 is -1, a below -1 is -1, spawnsecondary is on and trace off unless set; a child
// without y gets y 0 and trace on. At a position with angle a >= 0 each offset turns by a around the up axis
// (x' = x cos a + z sin a, z' = z cos a - x sin a) and the child angle becomes child a + a (0..360); at a random angle
// (a < 0) the offsets stay as written. trace snaps the child to the surface. One child that fails to spawn removes
// the whole group instance.

// One <child> of a group. Every value keeps the text of the file ("" = the attribute is not written), so a child that
// did not change is written back exactly as it was.
class VPPXEGroupChild : Managed
{
	// index among the <child> elements of the group as read, -1 = added here
	int OrigIdx;
	string Type;
	string X;
	string Z;
	string A;
	string Y;
	string LootMin;
	string LootMax;
	string Deloot;
	string SpawnSecondary;
	string Trace;
	// attributes the editor does not know, written back as they were
	string Extra;

	void VPPXEGroupChild()
	{
		OrigIdx = -1;
		Type = "";
		X = "";
		Z = "";
		A = "";
		Y = "";
		LootMin = "";
		LootMax = "";
		Deloot = "";
		SpawnSecondary = "";
		Trace = "";
		Extra = "";
	}

	void CopyFrom(VPPXEGroupChild other)
	{
		if (!other)
		{
			return;
		}

		OrigIdx = other.OrigIdx;
		Type = other.Type;
		X = other.X;
		Z = other.Z;
		A = other.A;
		Y = other.Y;
		LootMin = other.LootMin;
		LootMax = other.LootMax;
		Deloot = other.Deloot;
		SpawnSecondary = other.SpawnSecondary;
		Trace = other.Trace;
		Extra = other.Extra;
	}

	// Same values (OrigIdx is ignored).
	bool SameAs(VPPXEGroupChild other)
	{
		if (!other)
		{
			return false;
		}

		if (Type != other.Type || X != other.X || Z != other.Z || A != other.A || Y != other.Y)
		{
			return false;
		}

		if (LootMin != other.LootMin || LootMax != other.LootMax || Deloot != other.Deloot)
		{
			return false;
		}

		return SpawnSecondary == other.SpawnSecondary && Trace == other.Trace && Extra == other.Extra;
	}
};

class VPPXEGroupRow : Managed
{
	int Line;
	string Name;
	// attributes of <group> besides name
	string NameExtra;
	// elements and comments inside the group the editor does not know (kept as they are)
	int ExtraCount;
	// the <!--pos x=".." z=".." a=".." group="Name"/--> note right above the group (vanilla files have one per group:
	// where the group was built), "" when there is none. Read only.
	string HintX;
	string HintZ;
	string HintA;
	string HintY;
	ref array<ref VPPXEGroupChild> Children;

	void VPPXEGroupRow()
	{
		Name = "";
		NameExtra = "";
		HintX = "";
		HintZ = "";
		HintA = "";
		HintY = "";
		Children = new array<ref VPPXEGroupChild>;
	}

	void CopyFrom(VPPXEGroupRow other)
	{
		if (!other)
		{
			return;
		}

		Line = other.Line;
		Name = other.Name;
		NameExtra = other.NameExtra;
		ExtraCount = other.ExtraCount;
		HintX = other.HintX;
		HintZ = other.HintZ;
		HintA = other.HintA;
		HintY = other.HintY;
		Children = new array<ref VPPXEGroupChild>;
		if (!other.Children)
		{
			return;
		}

		foreach (VPPXEGroupChild otherChild : other.Children)
		{
			VPPXEGroupChild copy = new VPPXEGroupChild();
			copy.CopyFrom(otherChild);
			Children.Insert(copy);
		}
	}

	// Same name and children (the hint note is never written).
	bool SameAs(VPPXEGroupRow other)
	{
		if (!other || Name != other.Name || NameExtra != other.NameExtra)
		{
			return false;
		}

		if (Children.Count() != other.Children.Count())
		{
			return false;
		}

		for (int i = 0; i < Children.Count(); i++)
		{
			VPPXEGroupChild mine = Children[i];
			if (!mine.SameAs(other.Children[i]))
			{
				return false;
			}
		}

		return true;
	}
};

// XE_GetEvents sends cfgeventgroups.xml first, in chunks of groups.
class VPPXEGroupsChunk : Managed
{
	int ReqId;
	int ChunkIdx;
	int ChunkCount;
	string FileKey;
	int Revision;
	bool Editable;
	string ErrorKey;
	ref array<ref VPPXEGroupRow> Rows;

	void VPPXEGroupsChunk()
	{
		FileKey = "";
		ErrorKey = "";
		Rows = new array<ref VPPXEGroupRow>;
	}
};

class VPPXEGroupEdit : Managed
{
	int Op;
	int Index;
	ref VPPXEGroupRow Row;

	void VPPXEGroupEdit()
	{
		Row = new VPPXEGroupRow();
	}
};

class VPPXEGroupsSave : Managed
{
	int ReqId;
	int PartIdx;
	int PartCount;
	string FileKey;
	int BaseRevision;
	ref array<ref VPPXEGroupEdit> Edits;

	void VPPXEGroupsSave()
	{
		FileKey = "";
		Edits = new array<ref VPPXEGroupEdit>;
	}
};

class VPPXEGroupRules
{
	const static int MAX_ROWS = 3000;
	const static int MAX_CHILDREN = 300;
	const static int CHUNK_BYTES = 12000;
	const static int EDITS_PER_PART = 10;

	// A spawnsecondary / trace value: 1 on, 0 off, -1 not readable.
	static int BoolOf(string text)
	{
		string lower = text.Trim();
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

	// spawnsecondary as the server reads it (on unless set off).
	static bool SpawnSecondaryOn(VPPXEGroupChild child)
	{
		return child.SpawnSecondary == "" || BoolOf(child.SpawnSecondary) != 0;
	}

	// trace as the server reads it: set on, or forced on by a missing y.
	static bool TraceOn(VPPXEGroupChild child)
	{
		return child.Y == "" || BoolOf(child.Trace) == 1;
	}

	// "" when the child can be written, else the #key of the first problem.
	static string CheckChild(VPPXEGroupChild child)
	{
		if (!child || !VPPXmlText.IsValidClassName(child.Type))
		{
			return "#VSTR_XMLE_GRP_ERR_TYPE";
		}

		if (!VPPXESpawnRules.IsNumberText(child.X) || !VPPXESpawnRules.IsNumberText(child.Z))
		{
			return "#VSTR_XMLE_GRP_ERR_OFFSET";
		}

		if ((child.A != "" && !VPPXESpawnRules.IsNumberText(child.A)) || (child.Y != "" && !VPPXESpawnRules.IsNumberText(child.Y)))
		{
			return "#VSTR_XMLE_GRP_ERR_OFFSET";
		}

		if ((child.LootMin != "" && !VPPXESpawnRules.IsIntText(child.LootMin)) || (child.LootMax != "" && !VPPXESpawnRules.IsIntText(child.LootMax)))
		{
			return "#VSTR_XMLE_GRP_ERR_NUMBER";
		}

		if (child.Deloot != "" && !VPPXESpawnRules.IsIntText(child.Deloot))
		{
			return "#VSTR_XMLE_GRP_ERR_NUMBER";
		}

		if ((child.SpawnSecondary != "" && BoolOf(child.SpawnSecondary) < 0) || (child.Trace != "" && BoolOf(child.Trace) < 0))
		{
			return "#VSTR_XMLE_GRP_ERR_NUMBER";
		}

		return "";
	}

	// "" when the group can be saved, else the #key of the first problem. orig (may be null) = the group as read: its
	// children that did not change are written back as they were and not checked.
	static string CheckRow(VPPXEGroupRow row, VPPXEGroupRow orig)
	{
		if (!row)
		{
			return "#VSTR_XMLE_ERR_VALIDATION";
		}

		if (!VPPXmlText.IsValidClassName(row.Name))
		{
			return "#VSTR_XMLE_GRP_ERR_NAME";
		}

		if (row.Children.Count() > MAX_CHILDREN)
		{
			return "#VSTR_XMLE_GRP_ERR_TOO_MANY";
		}

		foreach (VPPXEGroupChild child : row.Children)
		{
			if (child && orig && child.OrigIdx >= 0 && child.OrigIdx < orig.Children.Count() && orig.Children[child.OrigIdx].SameAs(child))
			{
				continue;
			}

			string problem = CheckChild(child);
			if (problem != "")
			{
				return problem;
			}
		}

		return "";
	}

	// The server keeps the child (type aside, which needs the types index): x and z readable.
	static bool HasOffset(VPPXEGroupChild child)
	{
		return child && VPPXESpawnRules.IsNumberText(child.X) && VPPXESpawnRules.IsNumberText(child.Z);
	}

	// The child angle as written (-1 = random when missing or below -1).
	static float AngleOf(VPPXEGroupChild child)
	{
		if (!child || child.A == "" || !VPPXESpawnRules.IsNumberText(child.A))
		{
			return -1;
		}

		return Math.Max(child.A.ToFloat(), -1);
	}

	// Where the server puts a child of a group spawned at (anchorX, anchorZ) with angle anchorA: the offset turns with
	// a non-negative anchor angle; the result angle is -1 when random (only at a random anchor).
	static vector Place(VPPXEGroupChild child, float anchorX, float anchorZ, float anchorA)
	{
		float offX = child.X.ToFloat();
		float offZ = child.Z.ToFloat();
		if (anchorA < 0)
		{
			return Vector(anchorX + offX, AngleOf(child), anchorZ + offZ);
		}

		float rad = anchorA * Math.DEG2RAD;
		float sinA = Math.Sin(rad);
		float cosA = Math.Cos(rad);
		float worldX = anchorX + offX * cosA + offZ * sinA;
		float worldZ = anchorZ + offZ * cosA - offX * sinA;
		// child a + position a, wrapped into 0..360 (a child -1 at 0 faces 359: only a random position leaves it random)
		float angle = AngleOf(child) + anchorA;
		while (angle >= 360)
		{
			angle = angle - 360;
		}

		while (angle < 0)
		{
			angle = angle + 360;
		}

		return Vector(worldX, angle, worldZ);
	}

	// Byte estimate of a row on the wire (chunking).
	static int EstimateRow(VPPXEGroupRow row)
	{
		int size = 80 + row.Name.Length() + row.NameExtra.Length() + row.HintX.Length() + row.HintZ.Length() + row.HintA.Length() + row.HintY.Length();
		foreach (VPPXEGroupChild child : row.Children)
		{
			size += 40 + child.Type.Length() + child.X.Length() + child.Z.Length() + child.A.Length() + child.Y.Length() + child.Extra.Length();
			size += child.LootMin.Length() + child.LootMax.Length() + child.Deloot.Length() + child.SpawnSecondary.Length() + child.Trace.Length();
		}

		return size;
	}
};

// Event group children <-> world transforms, shared by the XML Editor and the Object Manager. The server's layout
// (VPPXEGroupRules.Place, memory dayz-events-xml-spec): a child at offset (x, y, z) of a position (px, py, pz) with
// angle a >= 0 stands at (px + x cos a + z sin a, py + y, pz + z cos a - x sin a) facing child a + a; a position angle
// below 0 (random) leaves the offsets as written. The angle is the object's yaw (GetOrientation()[0], the same sense)
// normalized to 0..360. A child without y (or with trace on) goes on the surface.
class VPPXEGroupTransform
{
	// A height that is not known (the surface is used instead).
	const static float UNKNOWN_Y = -99999;

	static float NormalizeAngle(float angle)
	{
		float result = Math.ModFloat(angle, 360.0);
		if (result < 0)
		{
			result = result + 360.0;
		}

		return result;
	}

	// Three decimals at most, trailing zeros dropped ("12.085", "1.9", "0", "-3.25").
	static string FormatNumber(float value)
	{
		int milli = Math.Round(value * 1000);
		string sign = "";
		if (milli < 0)
		{
			sign = "-";
			milli = -milli;
		}

		int whole = milli / 1000;
		int frac = milli - whole * 1000;
		if (frac == 0)
		{
			return sign + whole.ToString();
		}

		string fracText = frac.ToString();
		if (frac < 10)
		{
			fracText = "00" + fracText;
		}
		else if (frac < 100)
		{
			fracText = "0" + fracText;
		}

		while (fracText.Length() > 1 && fracText.Substring(fracText.Length() - 1, 1) == "0")
		{
			fracText = fracText.Substring(0, fracText.Length() - 1);
		}

		return sign + whole.ToString() + "." + fracText;
	}

	// An angle for a file: 0..360 with three decimals (360 after rounding is 0).
	static string FormatAngle(float angle)
	{
		float normalized = NormalizeAngle(angle);
		if (normalized >= 359.9995)
		{
			normalized = 0;
		}

		return FormatNumber(normalized);
	}

	// Where the server puts a child of a group spawned at anchorPos (absolute y) with angle anchorA. onGround = the
	// server places it on the surface (no y, or trace on); the y returned is then the anchor's.
	static vector WorldPos(VPPXEGroupChild child, vector anchorPos, float anchorA, out bool onGround)
	{
		vector placed = VPPXEGroupRules.Place(child, anchorPos[0], anchorPos[2], anchorA);
		float baseY = Math.Max(anchorPos[1], 0);
		float worldY = baseY;
		onGround = VPPXEGroupRules.TraceOn(child);
		if (child.Y != "" && VPPXESpawnRules.IsNumberText(child.Y))
		{
			worldY = baseY + child.Y.ToFloat();
		}

		return Vector(placed[0], worldY, placed[2]);
	}

	// The yaw the server gives a child of a group spawned with angle anchorA (-1 = random).
	static float WorldYaw(VPPXEGroupChild child, float anchorA)
	{
		vector placed = VPPXEGroupRules.Place(child, 0, 0, anchorA);
		return placed[1];
	}

	// The yaw a child gets in the builder (Object Manager): the server's (a random one laid out as 0).
	static float BuilderYaw(VPPXEGroupChild child, float anchorA)
	{
		float angle = WorldYaw(child, anchorA);
		if (angle < 0)
		{
			angle = 0;
		}

		return NormalizeAngle(angle);
	}

	// Identity of a child as sent to the builder (type and placement text): the children that come back are matched
	// only against the originals with one of these keys.
	static string SentKey(VPPXEGroupChild child)
	{
		return child.Type + "|" + child.X + "|" + child.Z + "|" + child.A + "|" + child.Y;
	}

	// Sets x, z, a and y of child from an object's world position and yaw, relative to a position (anchorPos with
	// absolute y, angle anchorA; below 0 = random, offsets not turned). onGround: y is left out, the server puts the
	// child on the surface.
	static void ToChild(vector worldPos, float worldYaw, vector anchorPos, float anchorA, bool onGround, VPPXEGroupChild child)
	{
		float dx = worldPos[0] - anchorPos[0];
		float dz = worldPos[2] - anchorPos[2];
		float offX = dx;
		float offZ = dz;
		float angle = worldYaw;
		if (anchorA >= 0)
		{
			float rad = anchorA * Math.DEG2RAD;
			float sinA = Math.Sin(rad);
			float cosA = Math.Cos(rad);
			offX = dx * cosA - dz * sinA;
			offZ = dx * sinA + dz * cosA;
			angle = worldYaw - anchorA;
		}

		child.X = FormatNumber(offX);
		child.Z = FormatNumber(offZ);
		child.A = FormatAngle(angle);
		child.Y = "";
		if (!onGround)
		{
			child.Y = FormatNumber(worldPos[1] - Math.Max(anchorPos[1], 0));
		}
	}
};

// XML Editor -> Object Manager (EVENTS > GROUPS > EDIT IN BUILDER): a group laid out in the world at a spawn position,
// edited there as LOCAL objects (only that admin sees them, nothing is saved on the server). The events files stay the
// only saved state: the edit comes back as a VPPXEGroupImport (SEND TO EVENT GROUP) and is saved in the XML Editor.
class VPPXEGroupBuild : Managed
{
	string GroupName;
	// the event whose GROUPS view sent it (a spawn position is added there when none of its positions use the group)
	string EventName;
	// the spawn position: absolute (y resolved), angle 0..360 or -1 random
	vector AnchorPos;
	float AnchorA;
	string AnchorLabel;
	// one entry per object: class, world position, orientation, VPPXEGroupTransform.SentKey of its child
	ref array<string> Types;
	ref array<vector> Positions;
	ref array<vector> Orientations;
	ref array<string> Keys;

	void VPPXEGroupBuild()
	{
		GroupName = "";
		EventName = "";
		AnchorLabel = "";
		Types = new array<string>;
		Positions = new array<vector>;
		Orientations = new array<vector>;
		Keys = new array<string>;
	}
};

// Object Manager -> XML Editor (SEND TO EVENT GROUP): the objects of a local build, relative to the spawn position it
// was laid out at (absolute position, yaw 0..360 or -1 random). The children carry type, x, z, a and y relative to it
// (VPPXEGroupTransform.ToChild).
class VPPXEGroupImport : Managed
{
	string SuggestedName;
	string EventName;
	vector AnchorPos;
	float AnchorA;
	ref array<ref VPPXEGroupChild> Children;
	// SentKey of each group child that became an object of the build
	ref array<string> SentKeys;

	void VPPXEGroupImport()
	{
		SuggestedName = "";
		EventName = "";
		Children = new array<ref VPPXEGroupChild>;
		SentKeys = new array<string>;
	}
};
