// XML Editor shared constants, enums and text-key helpers (INTERFACES v2.1 section 1 and section 10).
// Shared by the server (4_World) and the client (5_Mission); no dependency on either.

class VPPXEConst
{
	const static int PROTOCOL = 1;
	const static int UNSET = -999999;
	const static int CELL_SIZE = 100;
	const static int TYPE_ROWS_PER_CHUNK = 200;
	const static int DIFF_ROWS_PER_CHUNK = 200;
	const static int ISSUES_PER_CHUNK = 200;
	const static int BACKUPS_PER_CHUNK = 40;
	const static int TABLE_ROWS_PER_CHUNK = 200;
	const static int CELLS_PER_CHUNK = 1000;
	const static int MARKERS_PER_CHUNK = 1500;
	const static int CIRCLES_PER_CHUNK = 1000;
	const static int LIVE_PER_CHUNK = 500;
	const static int EDITS_PER_PART = 100;
	const static int UPLOAD_PARTS_PER_100MS = 2;
	const static int CHUNK_BYTES = 16000;
	const static int MAX_CELL_CHARS = 240;
	const static int PROGRESS_STEP_PCT = 5;
	const static int PROGRESS_INTERVAL_MS = 250;
	const static int INFLIGHT_TIMEOUT_MS = 30000;
	const static int MAX_EDITS_PER_BATCH = 20000;
	const static int DELETE_PER_REQUEST = 500;
	const static int MAX_DIFF_ROWS = 2000;
	const static int MAX_ISSUES = 5000;
	const static int MAX_MARKERS = 1500;
	const static int MAX_TABLE_ROWS_PER_LAYER = 300;
	const static int RAIL_MAX_ROWS = 4000;
	const static int MAX_NAME_LENGTH = 128;
	const static int MAX_NOTE_LENGTH = 120;
	const static int FIELD_COUNT = 12;
	const static int READ_CAP = 100000000;
	const static int JOIN_BLOCK_LINES = 8;   // leaf size of the binary-carry joiner (VPPXmlJoiner)
	const static int SMALL_DOC_LINES = 5000;
	const static int TEMPLATE_LIFETIME = 3600;
	const static int TEMPLATE_COST = 100;
};

enum VPPXEFileKind
{
	TYPES = 0,
	GLOBALS = 1,
	ECONOMY = 2,
	MESSAGES = 3,
	EVENTS = 4,
	SPAWNABLETYPES = 5,
	RANDOMPRESETS = 6,
	ECONOMYCORE = 10,
	LIMITS = 11,
	LIMITSUSER = 12,
	IGNORELIST = 13,
	EVENTSPAWNS = 14,
	EVENTGROUPS = 15,
	PLAYERSPAWNS = 16,
	WEATHER = 17,
	ENVIRONMENT = 18,
	TERRITORY = 19,
	MAPPROTO = 20,
	MAPPOS = 21,
	CLUSTERPROTO = 22,
	CLUSTERPOS = 23
};

enum VPPXEFileFlag
{
	EXISTS = 1,
	EDITABLE = 2,
	PENDING_RESTART = 4,
	PARSE_ERROR = 8,
	OUTSIDE_MISSION = 16,
	FROM_CE = 32,
	INTERRUPTED = 64,
	EXTERNAL_CHANGE = 128,
	READ_ERROR = 256
};

// Bit index 0..11 in this order; element names: nominal, min, lifetime, restock, cost, quantmin, quantmax, flags, category, usage, value, tag.
// Canonical XML element order for insertion: nominal, lifetime, restock, min, quantmin, quantmax, cost, flags, category, usage, value, tag.
enum VPPXEField
{
	NOMINAL = 1,
	MIN = 2,
	LIFETIME = 4,
	RESTOCK = 8,
	COST = 16,
	QUANTMIN = 32,
	QUANTMAX = 64,
	FLAGS = 128,
	CATEGORY = 256,
	USAGE = 512,
	VALUE = 1024,
	TAG = 2048
};

// Attribute names in the same order: count_in_cargo, count_in_hoarder, count_in_map, count_in_player, crafted, deloot.
enum VPPXEFlagBit
{
	CARGO = 1,
	HOARDER = 2,
	MAP = 4,
	PLAYER = 8,
	CRAFTED = 16,
	DELOOT = 32
};

enum VPPXEOp
{
	UPDATE = 0,
	ADD = 1,
	DUPLICATE = 2,
	DELETE = 3,
	RENAME = 4,
	COPY = 5,
	MOVE = 6
};

enum VPPXEListMode
{
	KEEP = 0,
	REPLACE = 1
};

enum VPPXEPerm
{
	EDIT_TYPES = 1,
	ADD_REMOVE = 2,
	DIST_MAP = 4,
	VIEW_BACKUPS = 8,
	RESTORE = 16,
	DELETE_BACKUPS = 32,
	LIVE_SCAN = 64,
	DELETE_OBJECTS = 128,
	TELEPORT = 256,
	EDIT_MESSAGES = 512,
	EDIT_EVENTS = 1024,
	EDIT_SPAWNABLES = 2048
};

enum VPPXERowIssue
{
	UNKNOWN_NAME = 1,
	MULTI_CATEGORY = 2,
	MIN_GT_NOMINAL = 4,
	QUANT = 8,
	DUPLICATE = 16,
	OVERRIDDEN = 32,
	NOT_IN_CONFIG = 64,
	NOT_PUBLIC = 128,
	IGNORED = 256,
	NO_PLACEMENT = 512,
	NEGATIVE = 1024,
	NOT_NUMBER = 2048
};

enum VPPXESeverity
{
	INFO = 0,
	WARNING = 1,
	ERROR = 2
};

enum VPPXEIssueCode
{
	UNKNOWN_CATEGORY = 1,
	UNKNOWN_USAGE = 2,
	UNKNOWN_VALUE = 3,
	UNKNOWN_TAG = 4,
	MULTI_CATEGORY = 5,
	MIN_GT_NOMINAL = 6,
	QUANT_RANGE = 7,
	QUANT_ORDER = 8,
	NEGATIVE = 9,
	DUP_IN_FILE = 10,
	OVERRIDDEN = 11,
	NOT_IN_CONFIG = 12,
	NOT_PUBLIC = 13,
	IGNORED = 14,
	NO_PLACEMENT = 15,
	NOT_NUMBER = 16,
	UNKNOWN_CHILD = 17,
	TYPE_NO_NAME = 18,
	PARSE = 30,
	FILE_MISSING = 31,
	OUTSIDE = 32,
	CE_TYPE = 33,
	INTERRUPTED = 34,
	PENDING = 35,
	EXTERNAL = 36,
	LIMIT_BITS = 37,
	LIMIT_DUP = 38,
	LIMIT_UNUSED = 39,
	UNSUPPORTED_NAME = 40,
	READ_FAIL = 41,
	EVSPAWN_NO_EVENT = 50,
	EVSPAWN_NO_GROUP = 51,
	PRESET_MISSING = 52,
	PRESET_EMPTY = 53,
	PRESET_UNUSED = 54,
	SPAWNABLE_NO_TYPE = 55,
	EVENT_NO_POS = 56,
	POS_NO_PROTO = 57,
	AREAFLAGS = 58
};

// Request mask bit = 1 << layer; CONTAINERS is table-only.
enum VPPXELayer
{
	BUILDINGS = 0,
	EVENT_OBJECTS = 1,
	EVENT_CHILD = 2,
	INFECTED = 3,
	CLUSTERS = 4,
	DISPATCH = 5,
	LIVE = 6,
	CONTAINERS = 7
};

enum VPPXEChunkKind
{
	CELLS = 0,
	MARKERS = 1,
	CIRCLES = 2,
	TABLE = 3
};

enum VPPXEDistFlag
{
	AREAFLAGS_OK = 1,
	AREAFLAGS_OFF = 2,
	NOMINAL_ZERO = 4,
	IGNORED = 8,
	NO_CE_ENTRY = 16,
	CAPPED = 32,
	PLAYER_EVENTS = 64,
	RULES_INFERRED = 128,
	DELOOT = 256
};

enum VPPXEStage
{
	REGISTRY = 0,
	TYPES = 1,
	SPAWN_LIGHT = 2,
	PROTO = 3,
	POSITIONS = 4,
	AREAFLAGS = 5,
	CLUSTERS = 6,
	MATCH = 7,
	SAVE = 8,
	DIFF = 9,
	ISSUES = 10
};

enum VPPXEIndexStage
{
	LIGHT = 1,
	MAP = 2,
	CLUSTERS = 3
};

enum VPPXELane
{
	INTERACTIVE = 0,
	BACKGROUND = 1
};

enum VPPXEEolMode
{
	BINARY = 0,
	WRITE_CRLF = 1,
	READ_STRIP = 2,
	UNKNOWN = 3
};

enum VPPXEBackupReason
{
	EDIT = 0,
	RESTORE_PRE = 1,
	MANUAL = 2,
	BULK = 3,
	STRUCT = 4
};

enum VPPXEDiffKind
{
	ADDED = 0,
	REMOVED = 1,
	CHANGED = 2,
	INFO = 3
};

// VS_NEXT = the backup vs the next newer backup of the same file, or vs the current file when none is newer ("changes after it").
enum VPPXEDiffMode
{
	VS_CURRENT = 0,
	VS_NEXT = 1
};

enum VPPXETab
{
	TYPES = 0,
	MAP = 1,
	BACKUPS = 2,
	FILES = 3,
	CREATE = 4,
	MESSAGES = 5,
	EVENTS = 6,
	SPAWNABLES = 7
};

enum VPPXEConfigState
{
	MISSING = 0,
	NOT_PUBLIC = 1,
	PUBLIC = 2
};

// Static #VSTR key and severity tables (INTERFACES section 10).
class VPPXEText
{
	// The ISSUE string key of a code (section 10 table), or "" if the code is unknown.
	static string IssueKey(int code)
	{
		switch (code)
		{
			case VPPXEIssueCode.UNKNOWN_CATEGORY:
				return "#VSTR_XMLE_ISSUE_UNKNOWN_CATEGORY";
			case VPPXEIssueCode.UNKNOWN_USAGE:
				return "#VSTR_XMLE_ISSUE_UNKNOWN_USAGE";
			case VPPXEIssueCode.UNKNOWN_VALUE:
				return "#VSTR_XMLE_ISSUE_UNKNOWN_VALUE";
			case VPPXEIssueCode.UNKNOWN_TAG:
				return "#VSTR_XMLE_ISSUE_UNKNOWN_TAG";
			case VPPXEIssueCode.MULTI_CATEGORY:
				return "#VSTR_XMLE_ISSUE_MULTI_CATEGORY";
			case VPPXEIssueCode.MIN_GT_NOMINAL:
				return "#VSTR_XMLE_ISSUE_MIN_GT_NOMINAL";
			case VPPXEIssueCode.QUANT_RANGE:
				return "#VSTR_XMLE_ISSUE_QUANT_RANGE";
			case VPPXEIssueCode.QUANT_ORDER:
				return "#VSTR_XMLE_ISSUE_QUANT_ORDER";
			case VPPXEIssueCode.NEGATIVE:
				return "#VSTR_XMLE_ISSUE_NEGATIVE";
			case VPPXEIssueCode.DUP_IN_FILE:
				return "#VSTR_XMLE_ISSUE_DUP_IN_FILE";
			case VPPXEIssueCode.OVERRIDDEN:
				return "#VSTR_XMLE_ISSUE_OVERRIDDEN";
			case VPPXEIssueCode.NOT_IN_CONFIG:
				return "#VSTR_XMLE_ISSUE_NOT_IN_CONFIG";
			case VPPXEIssueCode.NOT_PUBLIC:
				return "#VSTR_XMLE_ISSUE_NOT_PUBLIC";
			case VPPXEIssueCode.IGNORED:
				return "#VSTR_XMLE_ISSUE_IGNORED";
			case VPPXEIssueCode.NO_PLACEMENT:
				return "#VSTR_XMLE_ISSUE_NO_PLACEMENT";
			case VPPXEIssueCode.NOT_NUMBER:
				return "#VSTR_XMLE_ISSUE_NOT_NUMBER";
			case VPPXEIssueCode.UNKNOWN_CHILD:
				return "#VSTR_XMLE_ISSUE_UNKNOWN_CHILD";
			case VPPXEIssueCode.TYPE_NO_NAME:
				return "#VSTR_XMLE_ISSUE_TYPE_NO_NAME";
			case VPPXEIssueCode.PARSE:
				return "#VSTR_XMLE_ISSUE_PARSE";
			case VPPXEIssueCode.FILE_MISSING:
				return "#VSTR_XMLE_ISSUE_FILE_MISSING";
			case VPPXEIssueCode.OUTSIDE:
				return "#VSTR_XMLE_ISSUE_OUTSIDE";
			case VPPXEIssueCode.CE_TYPE:
				return "#VSTR_XMLE_ISSUE_CE_TYPE";
			case VPPXEIssueCode.INTERRUPTED:
				return "#VSTR_XMLE_ISSUE_INTERRUPTED";
			case VPPXEIssueCode.PENDING:
				return "#VSTR_XMLE_ISSUE_PENDING";
			case VPPXEIssueCode.EXTERNAL:
				return "#VSTR_XMLE_ISSUE_EXTERNAL";
			case VPPXEIssueCode.LIMIT_BITS:
				return "#VSTR_XMLE_ISSUE_LIMIT_BITS";
			case VPPXEIssueCode.LIMIT_DUP:
				return "#VSTR_XMLE_ISSUE_LIMIT_DUP";
			case VPPXEIssueCode.LIMIT_UNUSED:
				return "#VSTR_XMLE_ISSUE_LIMIT_UNUSED";
			case VPPXEIssueCode.UNSUPPORTED_NAME:
				return "#VSTR_XMLE_ISSUE_UNSUPPORTED_NAME";
			case VPPXEIssueCode.READ_FAIL:
				return "#VSTR_XMLE_ISSUE_READ_FAIL";
			case VPPXEIssueCode.EVSPAWN_NO_EVENT:
				return "#VSTR_XMLE_ISSUE_EVSPAWN_NO_EVENT";
			case VPPXEIssueCode.EVSPAWN_NO_GROUP:
				return "#VSTR_XMLE_ISSUE_EVSPAWN_NO_GROUP";
			case VPPXEIssueCode.PRESET_MISSING:
				return "#VSTR_XMLE_ISSUE_PRESET_MISSING";
			case VPPXEIssueCode.PRESET_EMPTY:
				return "#VSTR_XMLE_ISSUE_PRESET_EMPTY";
			case VPPXEIssueCode.PRESET_UNUSED:
				return "#VSTR_XMLE_ISSUE_PRESET_UNUSED";
			case VPPXEIssueCode.SPAWNABLE_NO_TYPE:
				return "#VSTR_XMLE_ISSUE_SPAWNABLE_NO_TYPE";
			case VPPXEIssueCode.EVENT_NO_POS:
				return "#VSTR_XMLE_ISSUE_EVENT_NO_POS";
			case VPPXEIssueCode.POS_NO_PROTO:
				return "#VSTR_XMLE_ISSUE_POS_NO_PROTO";
			case VPPXEIssueCode.AREAFLAGS:
				return "#VSTR_XMLE_ISSUE_AREAFLAGS";
		}

		return "";
	}

	// VPPXESeverity of an issue code (section 10 table); unknown codes are INFO.
	static int IssueSeverity(int code)
	{
		switch (code)
		{
			case VPPXEIssueCode.QUANT_RANGE:
			case VPPXEIssueCode.NEGATIVE:
			case VPPXEIssueCode.NOT_NUMBER:
			case VPPXEIssueCode.PARSE:
			case VPPXEIssueCode.FILE_MISSING:
			case VPPXEIssueCode.OUTSIDE:
			case VPPXEIssueCode.INTERRUPTED:
			case VPPXEIssueCode.LIMIT_BITS:
			case VPPXEIssueCode.READ_FAIL:
			case VPPXEIssueCode.EVSPAWN_NO_GROUP:
			case VPPXEIssueCode.PRESET_MISSING:
				return VPPXESeverity.ERROR;
			case VPPXEIssueCode.UNKNOWN_CATEGORY:
			case VPPXEIssueCode.UNKNOWN_USAGE:
			case VPPXEIssueCode.UNKNOWN_VALUE:
			case VPPXEIssueCode.UNKNOWN_TAG:
			case VPPXEIssueCode.MIN_GT_NOMINAL:
			case VPPXEIssueCode.QUANT_ORDER:
			case VPPXEIssueCode.DUP_IN_FILE:
			case VPPXEIssueCode.NOT_IN_CONFIG:
			case VPPXEIssueCode.NOT_PUBLIC:
			case VPPXEIssueCode.TYPE_NO_NAME:
			case VPPXEIssueCode.CE_TYPE:
			case VPPXEIssueCode.LIMIT_DUP:
			case VPPXEIssueCode.UNSUPPORTED_NAME:
			case VPPXEIssueCode.EVSPAWN_NO_EVENT:
			case VPPXEIssueCode.PRESET_EMPTY:
			case VPPXEIssueCode.EVENT_NO_POS:
			case VPPXEIssueCode.AREAFLAGS:
				return VPPXESeverity.WARNING;
		}

		return VPPXESeverity.INFO;
	}

	// -1 = no issue bit; ERROR if QUANT|NEGATIVE|NOT_NUMBER; WARNING if UNKNOWN_NAME|MIN_GT_NOMINAL|DUPLICATE|NOT_IN_CONFIG|NOT_PUBLIC; else INFO.
	static int RowSeverity(int issueBits)
	{
		if (issueBits == 0)
		{
			return -1;
		}

		int errorBits = VPPXERowIssue.QUANT | VPPXERowIssue.NEGATIVE | VPPXERowIssue.NOT_NUMBER;
		if ((issueBits & errorBits) != 0)
		{
			return VPPXESeverity.ERROR;
		}

		int warningBits = VPPXERowIssue.UNKNOWN_NAME | VPPXERowIssue.MIN_GT_NOMINAL | VPPXERowIssue.DUPLICATE | VPPXERowIssue.NOT_IN_CONFIG | VPPXERowIssue.NOT_PUBLIC;
		if ((issueBits & warningBits) != 0)
		{
			return VPPXESeverity.WARNING;
		}

		return VPPXESeverity.INFO;
	}

	static string KindKey(int kind)
	{
		switch (kind)
		{
			case VPPXEFileKind.TYPES:
				return "#VSTR_XMLE_KIND_TYPES";
			case VPPXEFileKind.GLOBALS:
				return "#VSTR_XMLE_KIND_GLOBALS";
			case VPPXEFileKind.ECONOMY:
				return "#VSTR_XMLE_KIND_ECONOMY";
			case VPPXEFileKind.MESSAGES:
				return "#VSTR_XMLE_KIND_MESSAGES";
			case VPPXEFileKind.EVENTS:
				return "#VSTR_XMLE_KIND_EVENTS";
			case VPPXEFileKind.SPAWNABLETYPES:
				return "#VSTR_XMLE_KIND_SPAWNABLE";
			case VPPXEFileKind.RANDOMPRESETS:
				return "#VSTR_XMLE_KIND_PRESETS";
			case VPPXEFileKind.ECONOMYCORE:
				return "#VSTR_XMLE_KIND_CORE";
			case VPPXEFileKind.LIMITS:
				return "#VSTR_XMLE_KIND_LIMITS";
			case VPPXEFileKind.LIMITSUSER:
				return "#VSTR_XMLE_KIND_LIMITSUSER";
			case VPPXEFileKind.IGNORELIST:
				return "#VSTR_XMLE_KIND_IGNORE";
			case VPPXEFileKind.EVENTSPAWNS:
				return "#VSTR_XMLE_KIND_EVSPAWNS";
			case VPPXEFileKind.EVENTGROUPS:
				return "#VSTR_XMLE_KIND_EVGROUPS";
			case VPPXEFileKind.PLAYERSPAWNS:
				return "#VSTR_XMLE_KIND_PLAYERSPAWNS";
			case VPPXEFileKind.WEATHER:
				return "#VSTR_XMLE_KIND_WEATHER";
			case VPPXEFileKind.ENVIRONMENT:
				return "#VSTR_XMLE_KIND_ENV";
			case VPPXEFileKind.TERRITORY:
				return "#VSTR_XMLE_KIND_TERRITORY";
			case VPPXEFileKind.MAPPROTO:
				return "#VSTR_XMLE_KIND_MAPPROTO";
			case VPPXEFileKind.MAPPOS:
				return "#VSTR_XMLE_KIND_MAPPOS";
			case VPPXEFileKind.CLUSTERPROTO:
				return "#VSTR_XMLE_KIND_CLUSTERPROTO";
			case VPPXEFileKind.CLUSTERPOS:
				return "#VSTR_XMLE_KIND_CLUSTERPOS";
		}

		return "";
	}

	static string StageKey(int stage)
	{
		switch (stage)
		{
			case VPPXEStage.REGISTRY:
				return "#VSTR_XMLE_STAGE_REGISTRY";
			case VPPXEStage.TYPES:
				return "#VSTR_XMLE_STAGE_TYPES";
			case VPPXEStage.SPAWN_LIGHT:
				return "#VSTR_XMLE_STAGE_SPAWN_LIGHT";
			case VPPXEStage.PROTO:
				return "#VSTR_XMLE_STAGE_PROTO";
			case VPPXEStage.POSITIONS:
				return "#VSTR_XMLE_STAGE_POSITIONS";
			case VPPXEStage.AREAFLAGS:
				return "#VSTR_XMLE_STAGE_AREAFLAGS";
			case VPPXEStage.CLUSTERS:
				return "#VSTR_XMLE_STAGE_CLUSTERS";
			case VPPXEStage.MATCH:
				return "#VSTR_XMLE_STAGE_MATCH";
			case VPPXEStage.SAVE:
				return "#VSTR_XMLE_STAGE_SAVE";
			case VPPXEStage.DIFF:
				return "#VSTR_XMLE_STAGE_DIFF";
			case VPPXEStage.ISSUES:
				return "#VSTR_XMLE_STAGE_ISSUES";
		}

		return "";
	}

	static string ReasonKey(int reason)
	{
		switch (reason)
		{
			case VPPXEBackupReason.EDIT:
				return "#VSTR_XMLE_BK_REASON_EDIT";
			case VPPXEBackupReason.RESTORE_PRE:
				return "#VSTR_XMLE_BK_REASON_RESTORE";
			case VPPXEBackupReason.MANUAL:
				return "#VSTR_XMLE_BK_REASON_MANUAL";
			case VPPXEBackupReason.BULK:
				return "#VSTR_XMLE_BK_REASON_BULK";
			case VPPXEBackupReason.STRUCT:
				return "#VSTR_XMLE_BK_REASON_STRUCT";
		}

		return "";
	}

	static string LayerKey(int layer)
	{
		switch (layer)
		{
			case VPPXELayer.BUILDINGS:
				return "#VSTR_XMLE_LAYER_BUILDINGS";
			case VPPXELayer.EVENT_OBJECTS:
				return "#VSTR_XMLE_LAYER_EVENTOBJ";
			case VPPXELayer.EVENT_CHILD:
				return "#VSTR_XMLE_LAYER_EVENTCHILD";
			case VPPXELayer.INFECTED:
				return "#VSTR_XMLE_LAYER_INFECTED";
			case VPPXELayer.CLUSTERS:
				return "#VSTR_XMLE_LAYER_CLUSTERS";
			case VPPXELayer.DISPATCH:
				return "#VSTR_XMLE_LAYER_DISPATCH";
			case VPPXELayer.LIVE:
				return "#VSTR_XMLE_LAYER_LIVE";
			case VPPXELayer.CONTAINERS:
				return "#VSTR_XMLE_LAYER_CONTAINERS";
		}

		return "";
	}

	static string SeverityKey(int severity)
	{
		switch (severity)
		{
			case VPPXESeverity.INFO:
				return "#VSTR_XMLE_SEV_INFO";
			case VPPXESeverity.WARNING:
				return "#VSTR_XMLE_SEV_WARNING";
			case VPPXESeverity.ERROR:
				return "#VSTR_XMLE_SEV_ERROR";
		}

		return "";
	}

	// BINARY, WRITE_CRLF, READ_STRIP, UNKNOWN; any other value maps to the UNKNOWN text.
	static string EolKey(int mode)
	{
		switch (mode)
		{
			case VPPXEEolMode.BINARY:
				return "#VSTR_XMLE_EOL_BINARY";
			case VPPXEEolMode.WRITE_CRLF:
				return "#VSTR_XMLE_EOL_CRLF";
			case VPPXEEolMode.READ_STRIP:
				return "#VSTR_XMLE_EOL_LF";
		}

		return "#VSTR_XMLE_EOL_UNKNOWN";
	}
};
