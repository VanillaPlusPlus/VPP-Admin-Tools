// VPP XML Editor: SPAWNABLES tab. Edits cfgspawnabletypes.xml and cfgrandompresets.xml (the vanilla files and every
// <ce type="spawnabletypes"> / <ce type="randompresets"> file), which together decide what an item spawns with.
// Left: TYPES / PRESETS mode, the file of that kind, search (names and item names) and filters, the rows and ADD /
// DUPLICATE / DELETE. Right (EDIT): the row (name, hoarder, unique, damage, advanced bind / subcounter / spawn batch;
// for a preset its namespace and chance), its cargo and attachments blocks, the selected block (namespace, chance,
// preset), its items and the selected item (name with class completion, chance, quantity, equip, damage) with the
// item's own nested blocks one level down (BACK goes up). The checks list what the server does with the row (rules in
// VPPXESpwRules and the spec at the top of VPPXESpawnables.c). MERGED shows a type as the server builds it from every
// spawnabletypes file. Edits stay local until SAVE (XE_SaveSpawnables, one file per save, EditSpawnables); REVERT drops
// them. PREVIEW (types) rolls the entry like the server does (VPPXESpwRoll: one item per block) with the edits, shows
// the result as local 3D objects plus a list, and TEST SPAWN AT ME asks the server for a real one (XE_TestSpawnable,
// the engine's own roll of the config it loaded at start).

class VPPXESpwWork : Managed
{
	int OrigIndex;
	bool Deleted;
	ref VPPXESpwNode Row;
	ref VPPXESpwNode Orig;

	void VPPXESpwWork()
	{
		OrigIndex = -1;
		Row = new VPPXESpwNode();
	}

	bool IsDirty()
	{
		if (OrigIndex < 0 || Deleted || !Orig)
		{
			return true;
		}

		return !Row.SameAs(Orig);
	}
};

class VPPXESpwFile : Managed
{
	string Key;
	int Kind;
	int Revision;
	bool Editable;
	string ErrorKey;
	bool ServerChanged;
	string FileDmgMin;
	string FileDmgMax;
	ref array<ref VPPXESpwWork> Work;

	void VPPXESpwFile()
	{
		Key = "";
		ErrorKey = "";
		FileDmgMin = "";
		FileDmgMax = "";
		Work = new array<ref VPPXESpwWork>;
	}

	int DirtyCount()
	{
		int dirty = 0;
		foreach (VPPXESpwWork work : Work)
		{
			if (work.IsDirty())
			{
				dirty++;
			}
		}

		return dirty;
	}
};

class VPPXESpwIncoming : Managed
{
	string Key;
	int Kind;
	int Revision;
	bool Editable;
	string ErrorKey;
	string FileDmgMin;
	string FileDmgMax;
	int ChunkCount;
	ref map<int, ref array<ref VPPXESpwNode>> ChunkRows;

	void VPPXESpwIncoming()
	{
		Key = "";
		ErrorKey = "";
		FileDmgMin = "";
		FileDmgMax = "";
		ChunkRows = new map<int, ref array<ref VPPXESpwNode>>;
	}

	int Got()
	{
		return ChunkRows.Count();
	}

	// Rows in chunk order: their position is the row index UPDATE and DELETE address.
	void JoinRows(array<ref VPPXESpwNode> outRows)
	{
		outRows.Clear();
		for (int c = 0; c < ChunkCount; c++)
		{
			array<ref VPPXESpwNode> part = ChunkRows.Get(c);
			if (!part)
			{
				continue;
			}

			foreach (VPPXESpwNode row : part)
			{
				outRows.Insert(row);
			}
		}
	}
};

// Where a preset name resolves: the first definition in load order (the server's rule).
class VPPXESpwPresetRef : Managed
{
	string FileKey;
	int WorkIdx;
	int Order;
	VPPXESpwNode Row;
};

// One level of the block navigation: the node holding blocks (a type or an item) and the selections in it.
class VPPXESpwNavFrame : Managed
{
	VPPXESpwNode Holder;
	int BlockIdx;
	int ItemIdx;
};

// Attachment slots from the game config (client): the slots an item fits (inventorySlot), the slots a holder offers
// (attachments[]), a weapon's magazines (magazines[]) and, built once on first use, every public class per slot.
class VPPXESlotIndex
{
	protected static ref map<string, ref array<string>> s_SlotsByClass;
	protected static ref map<string, ref array<string>> s_ClassesBySlot;

	// CfgVehicles / CfgWeapons / CfgMagazines of a class ("" when none), also for classes the public scan skips
	static string RootPathOf(string className)
	{
		string scanned = VPPXEConfigClasses.RootOf(className);
		if (scanned != "")
		{
			return scanned;
		}

		for (int r = 0; r < VPPXEConfigClasses.ROOT_COUNT; r++)
		{
			string rootPath = VPPXEConfigClasses.RootName(r);
			if (GetGame().ConfigIsExisting(rootPath + " " + className))
			{
				return rootPath;
			}
		}

		return "";
	}

	// The lower-case values of a config entry that can be one text or an array of texts.
	static void ReadTexts(string path, array<string> outValues)
	{
		outValues.Clear();
		if (!GetGame().ConfigIsExisting(path))
		{
			return;
		}

		int entryType = GetGame().ConfigGetType(path);
		if (entryType == CT_ARRAY)
		{
			TStringArray raw = new TStringArray();
			GetGame().ConfigGetTextArray(path, raw);
			foreach (string value : raw)
			{
				string lowerValue = value;
				lowerValue.ToLower();
				if (lowerValue != "")
				{
					outValues.Insert(lowerValue);
				}
			}

			return;
		}

		if (entryType != CT_CLASS)
		{
			string single = "";
			GetGame().ConfigGetText(path, single);
			single.ToLower();
			if (single != "")
			{
				outValues.Insert(single);
			}
		}
	}

	// The slots an item fits (lower case, cached per class).
	static void SlotsOf(string className, array<string> outSlots)
	{
		if (!s_SlotsByClass)
		{
			s_SlotsByClass = new map<string, ref array<string>>();
		}

		string lower = className;
		lower.ToLower();
		array<string> cached = s_SlotsByClass.Get(lower);
		if (!cached)
		{
			cached = new array<string>();
			string rootPath = RootPathOf(className);
			if (rootPath != "")
			{
				ReadTexts(rootPath + " " + className + " inventorySlot", cached);
			}

			s_SlotsByClass.Set(lower, cached);
		}

		outSlots.Copy(cached);
	}

	// The attachment slots a holder class offers (lower case).
	static void HolderSlots(string className, array<string> outSlots)
	{
		outSlots.Clear();
		string rootPath = RootPathOf(className);
		if (rootPath != "")
		{
			ReadTexts(rootPath + " " + className + " attachments", outSlots);
		}
	}

	// The magazines a weapon takes (lower case; none for anything but CfgWeapons).
	static void WeaponMagazines(string className, array<string> outNames)
	{
		outNames.Clear();
		string rootPath = RootPathOf(className);
		if (rootPath == "CfgWeapons")
		{
			ReadTexts("CfgWeapons " + className + " magazines", outNames);
		}
	}

	// 1 = the item fits an attachment slot (or the magazine well) of the holder, 0 = none, -1 = unknown (the holder
	// has no attachment slots or magazines in config).
	static int Fits(string holderClass, string itemClass)
	{
		array<string> holderSlots = new array<string>();
		HolderSlots(holderClass, holderSlots);
		array<string> magazines = new array<string>();
		WeaponMagazines(holderClass, magazines);
		if (holderSlots.Count() == 0 && magazines.Count() == 0)
		{
			return -1;
		}

		string itemLower = itemClass;
		itemLower.ToLower();
		if (magazines.Find(itemLower) >= 0)
		{
			return 1;
		}

		array<string> itemSlots = new array<string>();
		SlotsOf(itemClass, itemSlots);
		foreach (string slot : itemSlots)
		{
			if (holderSlots.Find(slot) >= 0)
			{
				return 1;
			}
		}

		return 0;
	}

	// Every public config class that fits the slot (config spelling, sorted); the index is built on first use.
	static void ClassesFor(string slotLower, array<string> outNames)
	{
		EnsureIndex();
		outNames.Clear();
		array<string> found = s_ClassesBySlot.Get(slotLower);
		if (found)
		{
			outNames.Copy(found);
		}
	}

	protected static void EnsureIndex()
	{
		if (s_ClassesBySlot)
		{
			return;
		}

		s_ClassesBySlot = new map<string, ref array<string>>();
		VPPXEConfigClasses.EnsureScanned();
		array<string> slots = new array<string>();
		int total = VPPXEConfigClasses.Count();
		for (int i = 0; i < total; i++)
		{
			string lower = VPPXEConfigClasses.LowerAt(i);
			string shown = VPPXEConfigClasses.DisplayOf(lower);
			SlotsOf(shown, slots);
			foreach (string slot : slots)
			{
				array<string> members = s_ClassesBySlot.Get(slot);
				if (!members)
				{
					members = new array<string>();
					s_ClassesBySlot.Set(slot, members);
				}

				members.Insert(shown);
			}
		}

		for (int s = 0; s < s_ClassesBySlot.Count(); s++)
		{
			array<string> sorted = s_ClassesBySlot.GetElement(s);
			sorted.Sort();
		}
	}
};

class VPPXESpawnablesTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int ROW_H = 28;
	const static int FOOTER_H = 36;
	const static int BAR_H = 40;
	const static int MODE_TYPES = 0;
	const static int MODE_PRESETS = 1;
	const static int VIEW_EDIT = 0;
	const static int VIEW_MERGED = 1;
	const static int VIEW_PREVIEW = 2;
	// a preview roll: equip / nested levels followed, rows listed at most
	const static int MAX_ROLL_DEPTH = 4;
	const static int MAX_ROLL_ITEMS = 60;
	// the "Add by slot" list: classes shown for one slot at most; the weapon magazine pseudo slot
	const static int MAX_SLOT_CLASSES = 48;
	// TYPES inspector texts: items shown per block, lines of "found in" at most
	const static int MAX_SPAWNS_WITH_ITEMS = 8;
	const static int MAX_FOUND_LINES = 40;
	// bulk operations on the listed rows
	const static int BULK_SCALE = 0;
	const static int BULK_REPLACE = 1;
	const static int BULK_REMOVE = 2;
	const static int BULK_CLEAR_DAMAGE = 3;
	const static string MAGAZINE_SLOT = "#magazines";
	const static int MAX_CHECK_LINES = 14;
	const static int MAX_SUGGESTIONS = 3;
	// the editable inputs, in m_Inputs order
	const static int IN_NAME = 0;
	const static int IN_ROWCHANCE = 1;
	const static int IN_DMGMIN = 2;
	const static int IN_DMGMAX = 3;
	const static int IN_BIND = 4;
	const static int IN_SUBMIN = 5;
	const static int IN_SUBMAX = 6;
	const static int IN_BATCH = 7;
	const static int IN_BLKCHANCE = 8;
	const static int IN_BLKPRESET = 9;
	const static int IN_ITEMNAME = 10;
	const static int IN_ITEMCHANCE = 11;
	const static int IN_QMIN = 12;
	const static int IN_QMAX = 13;
	const static int IN_IDMGMIN = 14;
	const static int IN_IDMGMAX = 15;
	const static int INPUT_COUNT = 16;
	// list filters (per mode)
	const static int FILTER_ALL = 0;
	const static int FILTER_PROBLEMS = 1;
	const static int FILTER_UNSAVED = 2;
	const static int FILTER_A = 3;
	const static int FILTER_B = 4;
	const static int FILTER_C = 5;
	const static int FILTER_D = 6;
	const static int FILTER_COUNT = 7;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_Left;
	protected Widget m_ListHeader;
	protected Widget m_FileHost;
	protected Widget m_FilterHost;
	protected Widget m_SearchPanel;
	protected EditBoxWidget m_InputSearch;
	protected ButtonWidget m_BtnModeTypes;
	protected Widget m_FillModeTypes;
	protected TextWidget m_TxtModeTypes;
	protected ButtonWidget m_BtnModePresets;
	protected Widget m_FillModePresets;
	protected TextWidget m_TxtModePresets;
	protected TextListboxWidget m_List;
	protected ButtonWidget m_BtnAdd;
	protected ButtonWidget m_BtnDup;
	protected ButtonWidget m_BtnDelete;
	protected Widget m_Right;
	protected TextWidget m_TxtEditTitle;
	protected TextWidget m_TxtLine;
	protected ButtonWidget m_BtnViewEdit;
	protected Widget m_FillViewEdit;
	protected TextWidget m_TxtViewEdit;
	protected ButtonWidget m_BtnViewMerged;
	protected Widget m_FillViewMerged;
	protected TextWidget m_TxtViewMerged;
	protected ButtonWidget m_BtnViewPreview;
	protected Widget m_FillViewPreview;
	protected TextWidget m_TxtViewPreview;
	protected Widget m_PreviewView;
	protected Widget m_PreviewCard;
	protected ItemPreviewWidget m_Preview;
	protected TextWidget m_TxtRollTitle;
	protected TextListboxWidget m_RollList;
	protected ButtonWidget m_BtnReroll;
	protected ButtonWidget m_BtnTestSpawn;
	protected TextWidget m_TxtPreviewNote;
	protected Widget m_Form;
	protected ref array<EditBoxWidget> m_Inputs;
	protected ref array<string> m_Last;
	protected TextWidget m_LblName;
	protected ButtonWidget m_BtnHoarder;
	protected Widget m_FillHoarder;
	protected TextWidget m_TxtHoarder;
	protected ButtonWidget m_BtnUnique;
	protected Widget m_FillUnique;
	protected TextWidget m_TxtUnique;
	protected ButtonWidget m_BtnKindCargo;
	protected Widget m_FillKindCargo;
	protected TextWidget m_TxtKindCargo;
	protected ButtonWidget m_BtnKindAtt;
	protected Widget m_FillKindAtt;
	protected TextWidget m_TxtKindAtt;
	protected TextWidget m_LblRowChance;
	protected ButtonWidget m_BtnDamage;
	protected Widget m_FillDamage;
	protected TextWidget m_TxtDamage;
	protected TextWidget m_TxtDmgHint;
	protected ButtonWidget m_BtnAdvanced;
	protected Widget m_FillAdvanced;
	protected TextWidget m_TxtAdvanced;
	protected TextWidget m_LblBind;
	protected TextWidget m_LblSub;
	protected TextWidget m_LblBatch;
	protected ButtonWidget m_BtnBack;
	protected TextWidget m_TxtPath;
	protected TextWidget m_TxtBlocksTitle;
	protected TextListboxWidget m_BlockList;
	protected ButtonWidget m_BtnBlockAddCargo;
	protected ButtonWidget m_BtnBlockAddAtt;
	protected ButtonWidget m_BtnBlockDup;
	protected ButtonWidget m_BtnBlockUp;
	protected ButtonWidget m_BtnBlockDown;
	protected ButtonWidget m_BtnBlockRemove;
	protected ButtonWidget m_BtnBlkCargo;
	protected Widget m_FillBlkCargo;
	protected TextWidget m_TxtBlkCargo;
	protected ButtonWidget m_BtnBlkAtt;
	protected Widget m_FillBlkAtt;
	protected TextWidget m_TxtBlkAtt;
	protected TextWidget m_LblBlkChance;
	protected TextWidget m_LblBlkPreset;
	protected Widget m_PresetHost;
	protected Widget m_SlotHost;
	protected ButtonWidget m_BtnBlockExtract;
	protected ButtonWidget m_BtnBlockInline;
	protected ButtonWidget m_BtnRenameRefs;
	protected TextWidget m_TxtItemsTitle;
	protected TextListboxWidget m_ItemList;
	protected ButtonWidget m_BtnItemAdd;
	protected ButtonWidget m_BtnItemUp;
	protected ButtonWidget m_BtnItemDown;
	protected ButtonWidget m_BtnItemRemove;
	protected TextWidget m_LblItemName;
	protected ButtonWidget m_BtnItemComplete;
	protected TextWidget m_TxtItemComplete;
	protected TextWidget m_TxtItemHint;
	protected TextWidget m_LblItemChance;
	protected TextWidget m_LblItemQuant;
	protected ButtonWidget m_BtnItemEquip;
	protected Widget m_FillItemEquip;
	protected TextWidget m_TxtItemEquip;
	protected ButtonWidget m_BtnItemDamage;
	protected Widget m_FillItemDamage;
	protected TextWidget m_TxtItemDamage;
	protected ButtonWidget m_BtnItemOpen;
	protected TextWidget m_TxtItemOpen;
	protected TextWidget m_TxtChecks;
	protected Widget m_MergedView;
	protected TextListboxWidget m_MergedList;
	protected TextWidget m_TxtMergedNote;
	protected TextWidget m_TxtEmpty;
	protected Widget m_ActionBar;
	protected TextWidget m_TxtStatus;
	protected ButtonWidget m_BtnRevert;
	protected ButtonWidget m_BtnSave;
	protected ref VPPDropDownMenu m_FileDD;
	protected ref VPPDropDownMenu m_FilterDD;
	protected ref VPPDropDownMenu m_PresetDD;
	protected ref VPPDropDownMenu m_SlotDD;
	protected Widget m_BulkHost;
	protected ref VPPDropDownMenu m_BulkDD;
	protected ref array<int> m_BulkOps;
	protected int m_PendBulkOp;
	protected string m_BulkFrom;
	// TYPES inspector: data revision, the effective blocks per type at that revision, a type to show once loaded
	protected int m_DataRev;
	protected int m_EffRev;
	protected ref map<string, VPPXESpwNode> m_EffAtt;
	protected ref map<string, VPPXESpwNode> m_EffCargo;
	protected ref map<string, int> m_EffDefs;
	protected int m_FoundHidden;
	protected string m_WantType;
	protected string m_PendAddName;
	// 0 = the holder's slots, 1 = the classes of m_SlotPicked
	protected int m_SlotLevel;
	protected string m_SlotPicked;
	protected string m_SlotHolder;
	protected ref array<string> m_SlotChoices;
	// a pending extract / rename dialog is about the shown block / the selected preset
	protected bool m_PendExtract;
	protected bool m_PendRename;

	protected ref array<Widget> m_TipWidgets;
	protected ref array<string> m_TipTitles;
	protected ref array<string> m_TipBodies;
	protected ref Widget m_TipRoot;
	protected TextWidget m_TipTitle;
	protected TextWidget m_TipBody;
	protected Widget m_TipOwner;

	// loaded files by key; the keys of each kind in load order
	protected ref map<string, ref VPPXESpwFile> m_Files;
	protected ref array<string> m_TypeFileKeys;
	protected ref array<string> m_PresetFileKeys;
	protected string m_TypeFileKey;
	protected string m_PresetFileKey;
	protected string m_WantFile;
	protected int m_Mode;
	protected int m_View;
	protected int m_FilterIdx;
	protected string m_LastSearch;
	protected bool m_ShowAdvanced;

	protected ref array<int> m_ListWork;
	protected int m_SelWork;
	protected int m_ListSel;
	// the navigation: the holder of the shown blocks (a type or an item; null for a preset, which is its own block)
	protected VPPXESpwNode m_Holder;
	protected int m_BlockIdx;
	protected int m_ItemIdx;
	protected ref array<ref VPPXESpwNavFrame> m_Stack;
	protected ref array<int> m_BlockRows;
	protected ref array<int> m_ItemRows;
	protected int m_BlockListSel;
	protected int m_ItemListSel;
	protected bool m_Loading;
	protected ref array<string> m_Suggestions;
	protected ref array<string> m_PresetChoices;

	// the index of every loaded file: presets (first definition per namespace), their definitions and uses
	protected bool m_IndexStale;
	protected ref map<string, ref VPPXESpwPresetRef> m_PresetFirst;
	protected ref map<string, int> m_PresetDefs;
	protected ref map<string, int> m_PresetUses;
	protected ref map<string, bool> m_Ignored;

	protected bool m_Shown;
	protected bool m_Loaded;
	protected int m_ReqId;
	protected bool m_Pending;
	protected int m_SentAt;
	protected int m_ExpectedFiles;
	protected ref map<string, ref VPPXESpwIncoming> m_Incoming;
	protected string m_LoadedSig;
	protected ref map<string, bool> m_ForceFresh;
	// the preview: the local objects in creation order (children after their parent), the type they show
	protected ref array<EntityAI> m_PreviewEntities;
	protected string m_PreviewType;
	// the previewed type is an infected / animal (DZ_LightAI): its own cargo blocks roll when it dies, not at spawn
	protected bool m_PreviewIsAi;
	protected int m_RollCount;
	protected bool m_ShowingTest;
	protected int m_RotationX;
	protected int m_RotationY;
	protected vector m_PreviewOrientation;
	protected int m_TestReqId;
	protected bool m_TestPending;
	protected int m_TestSentAt;
	protected string m_PendTestName;
	protected int m_SaveReqId;
	protected bool m_Saving;
	protected int m_SaveSentAt;
	protected string m_SaveFile;
	protected string m_StatusText;
	protected bool m_StatusError;
	// the row a pending ADD / DUPLICATE name dialog is about (null for ADD)
	protected VPPXESpwWork m_PendCopy;
	protected bool m_PendAdd;

	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXESpawnablesTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_Inputs = new array<EditBoxWidget>;
		m_Last = new array<string>;
		m_TipWidgets = new array<Widget>;
		m_TipTitles = new array<string>;
		m_TipBodies = new array<string>;
		m_Files = new map<string, ref VPPXESpwFile>;
		m_TypeFileKeys = new array<string>;
		m_PresetFileKeys = new array<string>;
		m_ListWork = new array<int>;
		m_Stack = new array<ref VPPXESpwNavFrame>;
		m_BlockRows = new array<int>;
		m_ItemRows = new array<int>;
		m_Suggestions = new array<string>;
		m_PresetChoices = new array<string>;
		m_PresetFirst = new map<string, ref VPPXESpwPresetRef>;
		m_PresetDefs = new map<string, int>;
		m_PresetUses = new map<string, int>;
		m_Ignored = new map<string, bool>;
		m_Incoming = new map<string, ref VPPXESpwIncoming>;
		m_ForceFresh = new map<string, bool>;
		m_PreviewEntities = new array<EntityAI>;
		m_SlotChoices = new array<string>;
		m_BulkOps = new array<int>;
		m_BulkFrom = "";
		m_WantType = "";
		m_PendAddName = "";
		m_DataRev = 1;
		m_EffRev = 0;
		m_SlotPicked = "";
		m_SlotHolder = "";
		m_PreviewType = "";
		m_PendTestName = "";
		m_TypeFileKey = "";
		m_PresetFileKey = "";
		m_WantFile = "";
		m_LastSearch = "";
		m_LoadedSig = "";
		m_StatusText = "";
		m_SelWork = -1;
		m_ListSel = -1;
		m_BlockIdx = -1;
		m_ItemIdx = -1;
		m_BlockListSel = -1;
		m_ItemListSel = -1;
		m_Unit = 1.0;
		m_IndexStale = true;

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorSpawnablesTab, host);
		m_Root.SetHandler(this);
		BindWidgets();
		m_FileDD = new VPPDropDownMenu(m_FileHost, "");
		m_FileDD.m_OnSelectItem.Insert(OnFilePicked);
		m_FilterDD = new VPPDropDownMenu(m_FilterHost, "");
		m_FilterDD.m_OnSelectItem.Insert(OnFilterPicked);
		m_PresetDD = new VPPDropDownMenu(m_PresetHost, "");
		m_PresetDD.m_OnSelectItem.Insert(OnPresetPicked);
		m_SlotDD = new VPPDropDownMenu(m_SlotHost, "");
		m_SlotDD.m_OnSelectItem.Insert(OnSlotPicked);
		m_BulkDD = new VPPDropDownMenu(m_BulkHost, "");
		m_BulkDD.m_OnSelectItem.Insert(OnBulkPicked);
		RegisterTips();
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnSpawnablesChunk", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnSpawnableTest", this, SingleplayerExecutionType.Client);
		RebuildFilterDropdown();
		ShowEditor(false);
		UpdateActionBar();
	}

	void ~VPPXESpawnablesTab()
	{
		ReleasePreview();
		if (m_TipRoot)
		{
			m_TipRoot.Unlink();
		}
	}

	protected void BindWidgets()
	{
		m_Left = m_Root.FindAnyWidget("SpwLeft");
		m_ListHeader = m_Root.FindAnyWidget("SpwListHeader");
		m_FileHost = m_Root.FindAnyWidget("SpwFileHost");
		m_FilterHost = m_Root.FindAnyWidget("SpwFilterHost");
		m_SearchPanel = m_Root.FindAnyWidget("SpwSearchPanel");
		m_InputSearch = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputSpwSearch"));
		m_BtnModeTypes = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwModeTypes"));
		m_FillModeTypes = m_Root.FindAnyWidget("FillSpwModeTypes");
		m_TxtModeTypes = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwModeTypes"));
		m_BtnModePresets = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwModePresets"));
		m_FillModePresets = m_Root.FindAnyWidget("FillSpwModePresets");
		m_TxtModePresets = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwModePresets"));
		m_List = TextListboxWidget.Cast(m_Root.FindAnyWidget("SpwList"));
		m_BtnAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwAdd"));
		m_BtnDup = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwDup"));
		m_BtnDelete = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwDelete"));
		m_Right = m_Root.FindAnyWidget("SpwRight");
		m_TxtEditTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwEditTitle"));
		m_TxtLine = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwLine"));
		m_BtnViewEdit = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwViewEdit"));
		m_FillViewEdit = m_Root.FindAnyWidget("FillSpwViewEdit");
		m_TxtViewEdit = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwViewEdit"));
		m_BtnViewMerged = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwViewMerged"));
		m_FillViewMerged = m_Root.FindAnyWidget("FillSpwViewMerged");
		m_TxtViewMerged = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwViewMerged"));
		m_BtnViewPreview = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwViewPreview"));
		m_FillViewPreview = m_Root.FindAnyWidget("FillSpwViewPreview");
		m_TxtViewPreview = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwViewPreview"));
		m_PreviewView = m_Root.FindAnyWidget("SpwPreviewView");
		m_PreviewCard = m_Root.FindAnyWidget("SpwPreviewCard");
		m_Preview = ItemPreviewWidget.Cast(m_Root.FindAnyWidget("SpwPreview"));
		m_TxtRollTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwRollTitle"));
		m_RollList = TextListboxWidget.Cast(m_Root.FindAnyWidget("SpwRollList"));
		m_BtnReroll = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwReroll"));
		m_BtnTestSpawn = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwTestSpawn"));
		m_TxtPreviewNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwPreviewNote"));
		m_Form = m_Root.FindAnyWidget("SpwForm");
		array<string> inputNames = {"InputSpwName", "InputSpwRowChance", "InputSpwDmgMin", "InputSpwDmgMax", "InputSpwBind", "InputSpwSubMin", "InputSpwSubMax", "InputSpwBatch", "InputSpwBlkChance", "InputSpwBlkPreset", "InputSpwItemName", "InputSpwItemChance", "InputSpwItemQMin", "InputSpwItemQMax", "InputSpwItemDmgMin", "InputSpwItemDmgMax"};
		foreach (string inputName : inputNames)
		{
			EditBoxWidget inputBox = EditBoxWidget.Cast(m_Root.FindAnyWidget(inputName));
			m_Inputs.Insert(inputBox);
			m_Last.Insert("");
			if (inputBox)
			{
				inputBox.SetHandler(this);
			}
		}

		m_LblName = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwName"));
		m_BtnHoarder = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwHoarder"));
		m_FillHoarder = m_Root.FindAnyWidget("FillSpwHoarder");
		m_TxtHoarder = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwHoarder"));
		m_BtnUnique = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwUnique"));
		m_FillUnique = m_Root.FindAnyWidget("FillSpwUnique");
		m_TxtUnique = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwUnique"));
		m_BtnKindCargo = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwKindCargo"));
		m_FillKindCargo = m_Root.FindAnyWidget("FillSpwKindCargo");
		m_TxtKindCargo = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwKindCargo"));
		m_BtnKindAtt = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwKindAtt"));
		m_FillKindAtt = m_Root.FindAnyWidget("FillSpwKindAtt");
		m_TxtKindAtt = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwKindAtt"));
		m_LblRowChance = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwRowChance"));
		m_BtnDamage = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwDamage"));
		m_FillDamage = m_Root.FindAnyWidget("FillSpwDamage");
		m_TxtDamage = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwDamage"));
		m_TxtDmgHint = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwDmgHint"));
		m_BtnAdvanced = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwAdvanced"));
		m_FillAdvanced = m_Root.FindAnyWidget("FillSpwAdvanced");
		m_TxtAdvanced = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwAdvanced"));
		m_LblBind = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwBind"));
		m_LblSub = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwSub"));
		m_LblBatch = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwBatch"));
		m_BtnBack = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBack"));
		m_TxtPath = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwPath"));
		m_TxtBlocksTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwBlocksTitle"));
		m_BlockList = TextListboxWidget.Cast(m_Root.FindAnyWidget("SpwBlockList"));
		m_BtnBlockAddCargo = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockAddCargo"));
		m_BtnBlockAddAtt = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockAddAtt"));
		m_BtnBlockDup = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockDup"));
		m_BtnBlockUp = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockUp"));
		m_BtnBlockDown = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockDown"));
		m_BtnBlockRemove = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockRemove"));
		m_BtnBlkCargo = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlkCargo"));
		m_FillBlkCargo = m_Root.FindAnyWidget("FillSpwBlkCargo");
		m_TxtBlkCargo = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwBlkCargo"));
		m_BtnBlkAtt = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlkAtt"));
		m_FillBlkAtt = m_Root.FindAnyWidget("FillSpwBlkAtt");
		m_TxtBlkAtt = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwBlkAtt"));
		m_LblBlkChance = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwBlkChance"));
		m_LblBlkPreset = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwBlkPreset"));
		m_PresetHost = m_Root.FindAnyWidget("SpwPresetHost");
		m_SlotHost = m_Root.FindAnyWidget("SpwSlotHost");
		m_BulkHost = m_Root.FindAnyWidget("SpwBulkHost");
		m_BtnBlockExtract = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockExtract"));
		m_BtnBlockInline = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwBlockInline"));
		m_BtnRenameRefs = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwRenameRefs"));
		m_TxtItemsTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemsTitle"));
		m_ItemList = TextListboxWidget.Cast(m_Root.FindAnyWidget("SpwItemList"));
		m_BtnItemAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemAdd"));
		m_BtnItemUp = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemUp"));
		m_BtnItemDown = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemDown"));
		m_BtnItemRemove = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemRemove"));
		m_LblItemName = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwItemName"));
		m_BtnItemComplete = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemComplete"));
		m_TxtItemComplete = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemComplete"));
		m_TxtItemHint = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemHint"));
		m_LblItemChance = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwItemChance"));
		m_LblItemQuant = TextWidget.Cast(m_Root.FindAnyWidget("LblSpwItemQuant"));
		m_BtnItemEquip = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemEquip"));
		m_FillItemEquip = m_Root.FindAnyWidget("FillSpwItemEquip");
		m_TxtItemEquip = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemEquip"));
		m_BtnItemDamage = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemDamage"));
		m_FillItemDamage = m_Root.FindAnyWidget("FillSpwItemDamage");
		m_TxtItemDamage = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemDamage"));
		m_BtnItemOpen = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwItemOpen"));
		m_TxtItemOpen = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwItemOpen"));
		m_TxtChecks = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwChecks"));
		m_MergedView = m_Root.FindAnyWidget("SpwMergedView");
		m_MergedList = TextListboxWidget.Cast(m_Root.FindAnyWidget("SpwMergedList"));
		m_TxtMergedNote = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwMergedNote"));
		m_TxtEmpty = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwEmpty"));
		m_ActionBar = m_Root.FindAnyWidget("SpwActionBar");
		m_TxtStatus = TextWidget.Cast(m_Root.FindAnyWidget("TxtSpwStatus"));
		m_BtnRevert = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwRevert"));
		m_BtnSave = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnSpwSave"));
	}

	// ---------------------------------------------------------------- tips

	protected void RegisterTips()
	{
		AddTip(m_Inputs[IN_NAME], "#VSTR_XMLE_SPB_NAME", "#VSTR_XMLE_SPB_TIP_NAME");
		AddTip(m_Inputs[IN_ROWCHANCE], "#VSTR_XMLE_SPB_CHANCE", "#VSTR_XMLE_SPB_TIP_PRESET_CHANCE");
		AddTip(m_BtnHoarder, "#VSTR_XMLE_SPB_HOARDER", "#VSTR_XMLE_SPB_TIP_HOARDER");
		AddTip(m_BtnUnique, "#VSTR_XMLE_SPB_UNIQUE", "#VSTR_XMLE_SPB_TIP_UNIQUE");
		AddTip(m_BtnDamage, "#VSTR_XMLE_SPB_DAMAGE", "#VSTR_XMLE_SPB_TIP_DAMAGE");
		AddTip(m_BtnKindCargo, "#VSTR_XMLE_SPB_CARGO", "#VSTR_XMLE_SPB_TIP_NAMESPACE");
		AddTip(m_BtnKindAtt, "#VSTR_XMLE_SPB_ATTACHMENTS", "#VSTR_XMLE_SPB_TIP_NAMESPACE");
		AddTip(m_Inputs[IN_BIND], "#VSTR_XMLE_SPB_BIND", "#VSTR_XMLE_SPB_TIP_BIND");
		AddTip(m_Inputs[IN_SUBMIN], "#VSTR_XMLE_SPB_SUBCOUNTER", "#VSTR_XMLE_SPB_TIP_SUBCOUNTER");
		AddTip(m_Inputs[IN_SUBMAX], "#VSTR_XMLE_SPB_SUBCOUNTER", "#VSTR_XMLE_SPB_TIP_SUBCOUNTER");
		AddTip(m_Inputs[IN_BATCH], "#VSTR_XMLE_SPB_BATCH", "#VSTR_XMLE_SPB_TIP_BATCH");
		AddTip(m_Inputs[IN_BLKCHANCE], "#VSTR_XMLE_SPB_CHANCE", "#VSTR_XMLE_SPB_TIP_BLOCK_CHANCE");
		AddTip(m_Inputs[IN_BLKPRESET], "#VSTR_XMLE_SPB_PRESET", "#VSTR_XMLE_SPB_TIP_PRESET");
		AddTip(m_BtnBlkCargo, "#VSTR_XMLE_SPB_CARGO", "#VSTR_XMLE_SPB_TIP_CARGO");
		AddTip(m_BtnBlkAtt, "#VSTR_XMLE_SPB_ATTACHMENTS", "#VSTR_XMLE_SPB_TIP_ATTACHMENTS");
		AddTip(m_Inputs[IN_ITEMNAME], "#VSTR_XMLE_SPB_ITEM", "#VSTR_XMLE_SPB_TIP_ITEM");
		AddTip(m_Inputs[IN_ITEMCHANCE], "#VSTR_XMLE_SPB_CHANCE", "#VSTR_XMLE_SPB_TIP_ITEM_CHANCE");
		AddTip(m_Inputs[IN_QMIN], "#VSTR_XMLE_SPB_QUANT", "#VSTR_XMLE_SPB_TIP_QUANT");
		AddTip(m_Inputs[IN_QMAX], "#VSTR_XMLE_SPB_QUANT", "#VSTR_XMLE_SPB_TIP_QUANT");
		AddTip(m_BtnItemEquip, "#VSTR_XMLE_SPB_EQUIP", "#VSTR_XMLE_SPB_TIP_EQUIP");
		AddTip(m_BtnItemDamage, "#VSTR_XMLE_SPB_DAMAGE", "#VSTR_XMLE_SPB_TIP_ITEM_DAMAGE");
		AddTip(m_BtnItemOpen, "#VSTR_XMLE_SPB_NESTED", "#VSTR_XMLE_SPB_TIP_NESTED");
		AddTip(m_BtnViewMerged, "#VSTR_XMLE_SPB_VIEW_MERGED", "#VSTR_XMLE_SPB_TIP_MERGED");
		AddTip(m_BtnViewPreview, "#VSTR_XMLE_SPB_VIEW_PREVIEW", "#VSTR_XMLE_SPB_TIP_PREVIEW");
		AddTip(m_BtnBlockExtract, "#VSTR_XMLE_SPB_EXTRACT", "#VSTR_XMLE_SPB_TIP_EXTRACT");
		AddTip(m_BtnBlockInline, "#VSTR_XMLE_SPB_INLINE", "#VSTR_XMLE_SPB_TIP_INLINE");
		AddTip(m_BtnRenameRefs, "#VSTR_XMLE_SPB_RENAME_REFS", "#VSTR_XMLE_SPB_TIP_RENAME_REFS");
		AddTip(m_SlotHost, "#VSTR_XMLE_SPB_SLOT_ADD", "#VSTR_XMLE_SPB_TIP_SLOT");
		AddTip(m_BulkHost, "#VSTR_XMLE_SPB_BULK_TITLE", "#VSTR_XMLE_SPB_TIP_BULK");
		AddTip(m_BtnReroll, "#VSTR_XMLE_SPB_BTN_REROLL", "#VSTR_XMLE_SPB_TIP_REROLL");
		AddTip(m_BtnTestSpawn, "#VSTR_XMLE_SPB_BTN_TEST", "#VSTR_XMLE_SPB_TIP_TEST");
	}

	protected void AddTip(Widget w, string titleKey, string bodyKey)
	{
		if (!w)
		{
			return;
		}

		w.SetHandler(this);
		m_TipWidgets.Insert(w);
		m_TipTitles.Insert(titleKey);
		m_TipBodies.Insert(bodyKey);
	}

	protected void ShowTip(int tipIdx)
	{
		if (!m_TipRoot)
		{
			VPPScriptedMenu hud = GetVPPUIManager().GetMenuByType(VPPAdminHud);
			if (!hud || !hud.layoutRoot)
			{
				return;
			}

			m_TipRoot = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPInfoBox, hud.layoutRoot);
			m_TipTitle = TextWidget.Cast(m_TipRoot.FindAnyWidget("Title"));
			m_TipBody = TextWidget.Cast(m_TipRoot.FindAnyWidget("ContentText"));
		}

		m_TipOwner = m_TipWidgets[tipIdx];
		string titleText = Tr(m_TipTitles[tipIdx]);
		string bodyText = Tr(m_TipBodies[tipIdx]);
		m_TipTitle.SetText(titleText);
		m_TipBody.SetText(bodyText);
		int mx;
		int my;
		GetMousePos(mx, my);
		m_TipRoot.SetPos(mx + 14, my + 18);
		m_TipRoot.Show(true);
		m_TipRoot.Update();
	}

	protected void HideTip()
	{
		m_TipOwner = null;
		if (m_TipRoot)
		{
			m_TipRoot.Show(false);
		}
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		int tipIdx = m_TipWidgets.Find(w);
		if (tipIdx >= 0)
		{
			ShowTip(tipIdx);
		}

		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w && w == m_TipOwner)
		{
			HideTip();
		}

		if (w && EditBoxWidget.Cast(w) && GetFocus() == w)
		{
			SetFocus(null);
			return true;
		}

		return false;
	}

	// ---------------------------------------------------------------- tab API

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			CloseDropdowns();
			HideTip();
			ReleasePreview();
			return;
		}

		OnResize();
		if (!m_Loaded || CurrentSig() != m_LoadedSig)
		{
			RequestList();
		}

		RebuildAll();
	}

	protected void CloseDropdowns()
	{
		if (m_FileDD)
		{
			m_FileDD.Close();
		}

		if (m_FilterDD)
		{
			m_FilterDD.Close();
		}

		if (m_PresetDD)
		{
			m_PresetDD.Close();
		}

		if (m_SlotDD)
		{
			m_SlotDD.Close();
		}

		if (m_BulkDD)
		{
			m_BulkDD.Close();
		}
	}

	// True while a list or save request waits for its reply (the window holds XE_CloseSession back meanwhile).
	bool HasInFlight()
	{
		int now = NowMs();
		if (m_Pending && now - m_SentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return true;
		}

		if (m_TestPending && now - m_TestSentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return true;
		}

		return m_Saving && now - m_SaveSentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void Refresh()
	{
		RequestList();
	}

	// A new file list or new revisions: reload (FinishList keeps files with unsaved edits and flags the ones whose
	// server copy changed).
	void OnSessionChanged()
	{
		RebuildFileKeys();
		string sig = CurrentSig();
		if (m_Loaded && sig != m_LoadedSig)
		{
			FlagServerChanges();
			if (m_Shown)
			{
				RequestList();
			}
			else
			{
				m_Loaded = false;
			}
		}

		if (m_Shown)
		{
			RebuildAll();
		}
	}

	// MenuXMLEditor.OpenSpawnablesFile: show that file (after the next reply when it is not loaded yet).
	void SelectFile(string fileKey)
	{
		m_WantFile = fileKey;
		if (m_Files.Contains(fileKey))
		{
			ShowFile(fileKey);
			m_WantFile = "";
		}
	}

	// Switches to the mode of the file and shows it.
	protected void ShowFile(string fileKey)
	{
		VPPXESpwFile file = m_Files.Get(fileKey);
		if (!file)
		{
			return;
		}

		if (file.Kind == VPPXEFileKind.RANDOMPRESETS)
		{
			m_Mode = MODE_PRESETS;
			m_PresetFileKey = fileKey;
		}
		else
		{
			m_Mode = MODE_TYPES;
			m_TypeFileKey = fileKey;
		}

		m_View = VIEW_EDIT;
		m_SelWork = -1;
		ResetNav();
		RebuildFilterDropdown();
		RebuildAll();
	}

	void OnResize()
	{
		if (!m_Root || !m_Left || !m_Right)
		{
			return;
		}

		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW < 20 || rootH < 20)
		{
			return;
		}

		m_LastRootW = rootW;
		m_LastRootH = rootH;
		UpdateUnit();
		float leftW;
		float leftH;
		m_Left.GetScreenSize(leftW, leftH);
		m_LeftW = leftW / m_Unit;
		m_LeftH = leftH / m_Unit;
		float rightW;
		float rightH;
		m_Right.GetScreenSize(rightW, rightH);
		m_RightW = rightW / m_Unit;
		m_RightH = rightH / m_Unit;
		LayoutLeft();
		LayoutRight();
	}

	void OnUpdate(float timeslice)
	{
		if (!m_Shown)
		{
			return;
		}

		CheckRootResize();
		PollSearch();
		PollListSelection();
		PollBlockList();
		PollItemList();
		PollInputs();
		int now = NowMs();
		if (m_Pending && now - m_SentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Pending = false;
			string notReady = Tr("#VSTR_XMLE_ERR_NOT_READY");
			SetStatus(notReady, true);
		}

		if (m_Saving && now - m_SaveSentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Saving = false;
			string saveTimeout = Tr("#VSTR_XMLE_ERR_NOT_READY");
			SetStatus(saveTimeout, true);
		}

		if (m_TestPending && now - m_TestSentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_TestPending = false;
			string testTimeout = Tr("#VSTR_XMLE_ERR_NOT_READY");
			SetStatus(testTimeout, true);
		}
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId <= 0)
		{
			return false;
		}

		string textKey = key;
		if (textKey == "")
		{
			textKey = "#VSTR_XMLE_ERR_INTERNAL";
		}

		string pattern = Tr(textKey);
		if (reqId == m_TestReqId)
		{
			m_TestPending = false;
			if (!ok)
			{
				string testFail = string.Format(pattern, arg);
				m_Owner.NotifyError(testFail);
				SetStatus(testFail, true);
			}

			return true;
		}

		if (reqId == m_ReqId)
		{
			m_Pending = false;
			if (!ok)
			{
				string failText = string.Format(pattern, arg);
				SetStatus(failText, true);
			}

			return true;
		}

		if (reqId != m_SaveReqId)
		{
			return false;
		}

		if (!m_Saving)
		{
			return true;
		}

		m_Saving = false;
		if (ok)
		{
			string savedLabel = FileLabel(m_SaveFile);
			string done = string.Format(pattern, savedLabel);
			m_Owner.Notify(done);
			SetStatus(done, false);
			m_ForceFresh.Set(m_SaveFile, true);
			RequestList();
		}
		else
		{
			string failed = string.Format(pattern, arg);
			m_Owner.NotifyError(failed);
			SetStatus(failed, true);
			if (textKey == "#VSTR_XMLE_ERR_STALE")
			{
				VPPXESpwFile staleFile = m_Files.Get(m_SaveFile);
				if (staleFile)
				{
					staleFile.ServerChanged = true;
				}
			}
		}

		UpdateActionBar();
		return true;
	}

	// ---------------------------------------------------------------- server-to-client receiver

	void XE_OnSpawnablesChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXESpawnablesChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESpawnablesChunk chunk = data.param1;
		if (!chunk || chunk.ReqId != m_ReqId || !m_Pending)
		{
			return;
		}

		m_SentAt = NowMs();
		if (chunk.Ignored && chunk.Ignored.Count() > 0)
		{
			m_Ignored.Clear();
			foreach (string ignoredName : chunk.Ignored)
			{
				m_Ignored.Set(ignoredName, true);
			}
		}

		if (chunk.FileCount <= 0)
		{
			m_ExpectedFiles = 0;
			FinishList();
			return;
		}

		m_ExpectedFiles = chunk.FileCount;
		VPPXESpwIncoming incoming = m_Incoming.Get(chunk.FileKey);
		if (!incoming)
		{
			incoming = new VPPXESpwIncoming();
			incoming.Key = chunk.FileKey;
			m_Incoming.Set(chunk.FileKey, incoming);
		}

		incoming.Kind = chunk.Kind;
		incoming.Revision = chunk.Revision;
		incoming.Editable = chunk.Editable;
		incoming.ErrorKey = chunk.ErrorKey;
		incoming.FileDmgMin = chunk.FileDmgMin;
		incoming.FileDmgMax = chunk.FileDmgMax;
		incoming.ChunkCount = chunk.ChunkCount;
		array<ref VPPXESpwNode> chunkRows = new array<ref VPPXESpwNode>;
		if (chunk.Rows)
		{
			foreach (VPPXESpwNode row : chunk.Rows)
			{
				if (row)
				{
					chunkRows.Insert(row);
				}
			}
		}

		incoming.ChunkRows.Set(chunk.ChunkIdx, chunkRows);
		if (m_Incoming.Count() < m_ExpectedFiles)
		{
			return;
		}

		for (int i = 0; i < m_Incoming.Count(); i++)
		{
			VPPXESpwIncoming check = m_Incoming.GetElement(i);
			if (check.Got() < check.ChunkCount)
			{
				return;
			}
		}

		FinishList();
	}

	// ---------------------------------------------------------------- loading

	protected void RequestList()
	{
		if (!m_Owner)
		{
			return;
		}

		m_ReqId = m_Owner.NextReqId();
		m_Pending = true;
		m_SentAt = NowMs();
		m_Incoming = new map<string, ref VPPXESpwIncoming>;
		m_ExpectedFiles = 0;
		string loadingText = Tr("#VSTR_XMLE_STATUS_LOADING");
		SetStatus(loadingText, false);
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetSpawnables", new Param1<int>(m_ReqId), true, null);
	}

	// Files with unsaved edits keep them (flagged when the server copy changed meanwhile) unless a save or revert
	// asked for a fresh copy; every other file takes the server rows.
	protected void FinishList()
	{
		m_Pending = false;
		m_Loaded = true;
		m_LoadedSig = CurrentSig();
		string keepName = SelectedName();
		map<string, ref VPPXESpwFile> fresh = new map<string, ref VPPXESpwFile>;
		for (int i = 0; i < m_Incoming.Count(); i++)
		{
			VPPXESpwIncoming incoming = m_Incoming.GetElement(i);
			VPPXESpwFile kept = m_Files.Get(incoming.Key);
			bool forced = m_ForceFresh.Contains(incoming.Key);
			if (kept && !forced && kept.DirtyCount() > 0)
			{
				if (kept.Revision != incoming.Revision)
				{
					kept.ServerChanged = true;
				}

				fresh.Set(incoming.Key, kept);
				continue;
			}

			VPPXESpwFile file = new VPPXESpwFile();
			file.Key = incoming.Key;
			file.Kind = incoming.Kind;
			file.Revision = incoming.Revision;
			file.Editable = incoming.Editable;
			file.ErrorKey = incoming.ErrorKey;
			file.FileDmgMin = incoming.FileDmgMin;
			file.FileDmgMax = incoming.FileDmgMax;
			array<ref VPPXESpwNode> rows = new array<ref VPPXESpwNode>;
			incoming.JoinRows(rows);
			int rowCount = rows.Count();
			for (int r = 0; r < rowCount; r++)
			{
				VPPXESpwWork work = new VPPXESpwWork();
				work.OrigIndex = r;
				work.Orig = new VPPXESpwNode();
				work.Orig.CopyFrom(rows[r]);
				work.Row.CopyFrom(rows[r]);
				file.Work.Insert(work);
			}

			fresh.Set(incoming.Key, file);
		}

		m_Files = fresh;
		m_ForceFresh.Clear();
		m_Incoming = new map<string, ref VPPXESpwIncoming>;
		m_IndexStale = true;
		RebuildFileKeys();
		if (m_WantFile != "" && m_Files.Contains(m_WantFile))
		{
			string wanted = m_WantFile;
			m_WantFile = "";
			ShowFile(wanted);
		}

		EnsureFileKeys();
		ReselectByName(keepName);
		SetStatus("", false);
		BumpData();
		RebuildAll();
		if (m_WantType != "")
		{
			string wantedType = m_WantType;
			SelectTypeNow(wantedType);
		}
	}

	// Every mode shows a loaded file of its kind (the first one when the shown one is gone).
	protected void EnsureFileKeys()
	{
		if (!m_Files.Contains(m_TypeFileKey))
		{
			m_TypeFileKey = "";
			if (m_TypeFileKeys.Count() > 0)
			{
				m_TypeFileKey = m_TypeFileKeys[0];
			}
		}

		if (!m_Files.Contains(m_PresetFileKey))
		{
			m_PresetFileKey = "";
			if (m_PresetFileKeys.Count() > 0)
			{
				m_PresetFileKey = m_PresetFileKeys[0];
			}
		}
	}

	protected string SelectedName()
	{
		VPPXESpwWork work = SelectedWork();
		if (!work)
		{
			return "";
		}

		return work.Row.Name;
	}

	// After a reload the indexes can shift (adds and deletes were saved): select the row of that name again.
	protected void ReselectByName(string rowName)
	{
		VPPXESpwFile file = CurrentFile();
		m_SelWork = -1;
		if (!file || rowName == "")
		{
			ResetNav();
			return;
		}

		for (int i = 0; i < file.Work.Count(); i++)
		{
			VPPXESpwWork work = file.Work[i];
			if (!work.Deleted && work.Row.Name == rowName)
			{
				m_SelWork = i;
				break;
			}
		}

		ResetNav();
	}

	// Files of each kind in session order (the server's load order within a kind).
	protected void RebuildFileKeys()
	{
		m_TypeFileKeys.Clear();
		m_PresetFileKeys.Clear();
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo)
			{
				continue;
			}

			if (fileInfo.Kind == VPPXEFileKind.SPAWNABLETYPES)
			{
				m_TypeFileKeys.Insert(fileInfo.Key);
			}
			else if (fileInfo.Kind == VPPXEFileKind.RANDOMPRESETS)
			{
				m_PresetFileKeys.Insert(fileInfo.Key);
			}
		}
	}

	protected string CurrentSig()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return "";
		}

		string sig = "";
		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || (fileInfo.Kind != VPPXEFileKind.SPAWNABLETYPES && fileInfo.Kind != VPPXEFileKind.RANDOMPRESETS))
			{
				continue;
			}

			int revision = fileInfo.Revision;
			int flags = fileInfo.Flags;
			sig = sig + "|" + fileInfo.Key + ":" + revision.ToString() + ":" + flags.ToString();
		}

		return sig;
	}

	protected void FlagServerChanges()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo)
			{
				continue;
			}

			VPPXESpwFile file = m_Files.Get(fileInfo.Key);
			if (file && file.DirtyCount() > 0 && file.Revision != fileInfo.Revision)
			{
				file.ServerChanged = true;
			}
		}
	}

	// ---------------------------------------------------------------- index (presets, their uses, type definitions)

	protected string PresetKey(int blockKind, string presetName)
	{
		string lower = presetName;
		lower.ToLower();
		if (blockKind == VPPXESpwKind.ATTACHMENTS)
		{
			return "a:" + lower;
		}

		return "c:" + lower;
	}

	// Presets in load order (the first of a name per namespace wins) and every preset reference of every row.
	protected void EnsureIndex()
	{
		if (!m_IndexStale)
		{
			return;
		}

		m_IndexStale = false;
		m_PresetFirst.Clear();
		m_PresetDefs.Clear();
		m_PresetUses.Clear();
		int order = 0;
		foreach (string presetFileKey : m_PresetFileKeys)
		{
			VPPXESpwFile presetFile = m_Files.Get(presetFileKey);
			if (!presetFile)
			{
				continue;
			}

			for (int w = 0; w < presetFile.Work.Count(); w++)
			{
				VPPXESpwWork presetWork = presetFile.Work[w];
				if (presetWork.Deleted || presetWork.Row.Name == "")
				{
					continue;
				}

				string key = PresetKey(presetWork.Row.Kind, presetWork.Row.Name);
				int defs = m_PresetDefs.Get(key);
				m_PresetDefs.Set(key, defs + 1);
				if (!m_PresetFirst.Contains(key))
				{
					VPPXESpwPresetRef first = new VPPXESpwPresetRef();
					first.FileKey = presetFileKey;
					first.WorkIdx = w;
					first.Order = order;
					first.Row = presetWork.Row;
					m_PresetFirst.Set(key, first);
				}

				order++;
				CountUses(presetWork.Row);
			}
		}

		foreach (string typeFileKey : m_TypeFileKeys)
		{
			VPPXESpwFile typeFile = m_Files.Get(typeFileKey);
			if (!typeFile)
			{
				continue;
			}

			foreach (VPPXESpwWork typeWork : typeFile.Work)
			{
				if (!typeWork.Deleted)
				{
					CountUses(typeWork.Row);
				}
			}
		}
	}

	protected void CountUses(VPPXESpwNode node)
	{
		if (VPPXESpwKind.IsBlock(node.Kind) && node.Preset != "")
		{
			string key = PresetKey(node.Kind, node.Preset);
			int uses = m_PresetUses.Get(key);
			m_PresetUses.Set(key, uses + 1);
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (kid && kid.Kind != VPPXESpwKind.OTHER)
			{
				CountUses(kid);
			}
		}
	}

	protected VPPXESpwPresetRef FindPreset(int blockKind, string presetName)
	{
		EnsureIndex();
		string key = PresetKey(blockKind, presetName);
		return m_PresetFirst.Get(key);
	}

	// Load order of a preset row (-1 when it is not the first of its name).
	protected int PresetOrderOf(string fileKey, int workIdx)
	{
		EnsureIndex();
		for (int i = 0; i < m_PresetFirst.Count(); i++)
		{
			VPPXESpwPresetRef first = m_PresetFirst.GetElement(i);
			if (first.FileKey == fileKey && first.WorkIdx == workIdx)
			{
				return first.Order;
			}
		}

		return -1;
	}

	// The chance a block has when the server spawns (a preset's chance unless the block sets its own).
	protected float EffectiveChance(VPPXESpwNode blockNode)
	{
		float fallback = -1;
		if (blockNode.Preset != "")
		{
			VPPXESpwPresetRef found = FindPreset(blockNode.Kind, blockNode.Preset);
			if (found && found.Row)
			{
				fallback = VPPXESpwRules.ChanceOf(found.Row.Chance, -1);
			}
		}

		return VPPXESpwRules.ChanceOf(blockNode.Chance, fallback);
	}

	protected bool IsCeType(string typeName)
	{
		if (!m_Owner || !m_Owner.GetModel())
		{
			return true;
		}

		VPPXEClientModel model = m_Owner.GetModel();
		if (model.GetIndexVersion() <= 0 || model.HasPendingIndex())
		{
			return true;
		}

		return model.HasName(typeName);
	}

	protected bool IsIgnoredName(string className)
	{
		string lower = className;
		lower.ToLower();
		return m_Ignored.Contains(lower);
	}

	protected bool ExistsInConfig(string className)
	{
		if (!VPPXmlText.IsValidClassName(className))
		{
			return false;
		}

		if (GetGame().ConfigIsExisting("CfgVehicles " + className))
		{
			return true;
		}

		if (GetGame().ConfigIsExisting("CfgWeapons " + className))
		{
			return true;
		}

		return GetGame().ConfigIsExisting("CfgMagazines " + className);
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT || !w)
		{
			return false;
		}

		if (OnModeOrViewClick(w))
		{
			return true;
		}

		if (OnRowClick(w))
		{
			return true;
		}

		if (OnBlockClick(w))
		{
			return true;
		}

		if (OnItemClick(w))
		{
			return true;
		}

		if (w == m_BtnSave)
		{
			StartSave();
			return true;
		}

		if (w == m_BtnRevert)
		{
			StartRevert();
			return true;
		}

		return false;
	}

	protected bool OnModeOrViewClick(Widget w)
	{
		if (w == m_BtnModeTypes || w == m_BtnModePresets)
		{
			int mode = MODE_TYPES;
			if (w == m_BtnModePresets)
			{
				mode = MODE_PRESETS;
			}

			if (mode != m_Mode)
			{
				m_Mode = mode;
				m_View = VIEW_EDIT;
				ReleasePreview();
				m_FilterIdx = FILTER_ALL;
				m_SelWork = -1;
				ResetNav();
				RebuildFilterDropdown();
				RebuildAll();
			}

			return true;
		}

		if (w == m_BtnViewEdit)
		{
			SetView(VIEW_EDIT);
			return true;
		}

		if (w == m_BtnViewMerged)
		{
			SetView(VIEW_MERGED);
			return true;
		}

		if (w == m_BtnViewPreview)
		{
			SetView(VIEW_PREVIEW);
			return true;
		}

		if (w == m_BtnReroll)
		{
			RollPreview();
			return true;
		}

		if (w == m_BtnTestSpawn)
		{
			StartTestSpawn();
			return true;
		}

		if (w == m_BtnAdvanced)
		{
			m_ShowAdvanced = !m_ShowAdvanced;
			LayoutRight();
			UpdateDetails();
			return true;
		}

		if (w == m_BtnBack)
		{
			NavBack();
			return true;
		}

		return false;
	}

	protected bool OnRowClick(Widget w)
	{
		if (w == m_BtnAdd)
		{
			StartAdd(null);
			return true;
		}

		if (w == m_BtnDup)
		{
			VPPXESpwWork source = SelectedWork();
			if (source)
			{
				StartAdd(source);
			}

			return true;
		}

		if (w == m_BtnDelete)
		{
			DeleteSelected();
			return true;
		}

		if (w == m_BtnHoarder)
		{
			ToggleLeaf(VPPXESpwKind.HOARDER);
			return true;
		}

		if (w == m_BtnUnique)
		{
			ToggleLeaf(VPPXESpwKind.UNIQUE);
			return true;
		}

		if (w == m_BtnDamage)
		{
			ToggleLeaf(VPPXESpwKind.DAMAGE);
			return true;
		}

		if (w == m_BtnKindCargo)
		{
			SetRowKind(VPPXESpwKind.CARGO);
			return true;
		}

		if (w == m_BtnKindAtt)
		{
			SetRowKind(VPPXESpwKind.ATTACHMENTS);
			return true;
		}

		return false;
	}

	protected bool OnBlockClick(Widget w)
	{
		if (w == m_BtnBlockAddCargo)
		{
			AddBlock(VPPXESpwKind.CARGO);
			return true;
		}

		if (w == m_BtnBlockAddAtt)
		{
			AddBlock(VPPXESpwKind.ATTACHMENTS);
			return true;
		}

		if (w == m_BtnBlockDup)
		{
			DuplicateBlock();
			return true;
		}

		if (w == m_BtnBlockUp)
		{
			MoveBlock(-1);
			return true;
		}

		if (w == m_BtnBlockDown)
		{
			MoveBlock(1);
			return true;
		}

		if (w == m_BtnBlockRemove)
		{
			RemoveBlock();
			return true;
		}

		if (w == m_BtnBlkCargo)
		{
			SetBlockKind(VPPXESpwKind.CARGO);
			return true;
		}

		if (w == m_BtnBlkAtt)
		{
			SetBlockKind(VPPXESpwKind.ATTACHMENTS);
			return true;
		}

		if (w == m_BtnBlockExtract)
		{
			StartExtract();
			return true;
		}

		if (w == m_BtnBlockInline)
		{
			InlinePreset();
			return true;
		}

		if (w == m_BtnRenameRefs)
		{
			StartRenamePreset();
			return true;
		}

		return false;
	}

	protected bool OnItemClick(Widget w)
	{
		if (w == m_BtnItemAdd)
		{
			AddItem();
			return true;
		}

		if (w == m_BtnItemUp)
		{
			MoveItem(-1);
			return true;
		}

		if (w == m_BtnItemDown)
		{
			MoveItem(1);
			return true;
		}

		if (w == m_BtnItemRemove)
		{
			RemoveItem();
			return true;
		}

		if (w == m_BtnItemComplete)
		{
			CompleteItemName();
			return true;
		}

		if (w == m_BtnItemEquip)
		{
			ToggleItemEquip();
			return true;
		}

		if (w == m_BtnItemDamage)
		{
			ToggleItemDamage();
			return true;
		}

		if (w == m_BtnItemOpen)
		{
			NavIntoItem();
			return true;
		}

		return false;
	}

	void OnFilePicked(int index)
	{
		m_FileDD.Close();
		array<string> keys = ModeFileKeys();
		if (index < 0 || index >= keys.Count())
		{
			RebuildFileDropdown();
			return;
		}

		string fileKey = keys[index];
		ShowFile(fileKey);
	}

	void OnFilterPicked(int index)
	{
		m_FilterDD.Close();
		if (index < 0 || index >= FILTER_COUNT)
		{
			return;
		}

		m_FilterIdx = index;
		string label = FilterLabel(index);
		m_FilterDD.SetText(label);
		RebuildList();
		LoadForm();
	}

	// The preset picker: index 0 = no preset, then the preset names of the block's namespace.
	void OnPresetPicked(int index)
	{
		m_PresetDD.Close();
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || !CanEdit() || index < 0 || index > m_PresetChoices.Count())
		{
			return;
		}

		string chosen = "";
		if (index > 0)
		{
			chosen = m_PresetChoices[index - 1];
		}

		blockNode.Preset = chosen;
		m_Loading = true;
		m_Inputs[IN_BLKPRESET].SetText(chosen);
		m_Last.Set(IN_BLKPRESET, chosen);
		m_Loading = false;
		AfterEdit(true);
	}

	// ---------------------------------------------------------------- rows

	protected bool CanEdit()
	{
		VPPXESpwFile file = CurrentFile();
		if (!file || !file.Editable || m_Saving)
		{
			return false;
		}

		// a save or revert reloads this file fresh: an edit made meanwhile would be dropped
		if (m_Pending && m_ForceFresh.Contains(file.Key))
		{
			return false;
		}

		return m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_SPAWNABLES);
	}

	// ADD and DUPLICATE ask for the name first.
	protected void StartAdd(VPPXESpwWork copy)
	{
		if (!CurrentFile() || !CanEdit())
		{
			return;
		}

		m_PendCopy = copy;
		m_PendAdd = true;
		string prefill = "";
		if (copy)
		{
			prefill = copy.Row.Name + "_2";
		}
		else if (m_PendAddName != "")
		{
			prefill = m_PendAddName;
		}

		m_PendAddName = "";

		string titleKey = "#VSTR_XMLE_SPB_DLG_ADD_TYPE_TITLE";
		string bodyKey = "#VSTR_XMLE_SPB_DLG_ADD_TYPE_BODY";
		if (m_Mode == MODE_PRESETS)
		{
			titleKey = "#VSTR_XMLE_SPB_DLG_ADD_PRESET_TITLE";
			bodyKey = "#VSTR_XMLE_SPB_DLG_ADD_PRESET_BODY";
		}

		string body = Tr(bodyKey);
		m_Owner.OpenConfirmInput(titleKey, body, this, "OnAddName", prefill);
	}

	void OnAddName(int result, string input)
	{
		VPPXESpwWork copy = m_PendCopy;
		bool pending = m_PendAdd;
		m_PendCopy = null;
		m_PendAdd = false;
		VPPXESpwFile file = CurrentFile();
		if (result != DIAGRESULT.OK || !pending || !file || !CanEdit())
		{
			return;
		}

		string newName = input.Trim();
		bool usable = VPPXESpwRules.IsPlainName(newName);
		if (m_Mode == MODE_TYPES)
		{
			usable = VPPXmlText.IsValidClassName(newName);
		}

		if (!usable)
		{
			string invalidPattern = Tr("#VSTR_XMLE_ERR_NAME_INVALID");
			string invalidText = string.Format(invalidPattern, newName);
			m_Owner.NotifyError(invalidText);
			return;
		}

		VPPXESpwWork work = new VPPXESpwWork();
		if (copy)
		{
			work.Row.CopyFrom(copy.Row);
			AsNew(work.Row);
		}
		else if (m_Mode == MODE_PRESETS)
		{
			work.Row.Kind = VPPXESpwKind.CARGO;
			work.Row.Chance = "1";
		}
		else
		{
			work.Row.Kind = VPPXESpwKind.TYPE;
		}

		work.Row.Name = newName;
		work.Row.Line = 0;
		file.Work.Insert(work);
		BumpData();
		m_SelWork = file.Work.Count() - 1;
		m_IndexStale = true;
		ResetNav();
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// A copy that is new in the file: no original indexes, no kept comments (they belong to the original).
	protected void AsNew(VPPXESpwNode node)
	{
		node.OrigIdx = -1;
		node.Line = 0;
		for (int i = node.Kids.Count() - 1; i >= 0; i--)
		{
			VPPXESpwNode kid = node.Kids[i];
			if (!kid || kid.Kind == VPPXESpwKind.OTHER)
			{
				node.Kids.Remove(i);
				continue;
			}

			AsNew(kid);
		}
	}

	// A row that came from the file is marked deleted (REVERT brings it back); one added here is dropped.
	protected void DeleteSelected()
	{
		VPPXESpwFile file = CurrentFile();
		VPPXESpwWork work = SelectedWork();
		if (!file || !work || !CanEdit())
		{
			return;
		}

		if (work.OrigIndex < 0)
		{
			file.Work.RemoveOrdered(m_SelWork);
		}
		else
		{
			work.Deleted = true;
		}

		BumpData();

		m_SelWork = -1;
		m_IndexStale = true;
		ResetNav();
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// HOARDER / UNIQUE / DAMAGE on the type (on or off).
	protected void ToggleLeaf(int kind)
	{
		VPPXESpwWork work = SelectedWork();
		if (!work || !CanEdit() || work.Row.Kind != VPPXESpwKind.TYPE)
		{
			return;
		}

		VPPXESpwNode found = work.Row.FirstKid(kind);
		if (found)
		{
			work.Row.Kids.RemoveItem(found);
		}
		else
		{
			VPPXESpwNode leaf = new VPPXESpwNode();
			leaf.Kind = kind;
			if (kind == VPPXESpwKind.DAMAGE)
			{
				leaf.Min = "0";
				leaf.Max = "0.3";
			}

			InsertLeaf(work.Row, leaf);
		}

		LoadForm();
		AfterEdit(false);
	}

	// Leaf elements go before the blocks, in the vanilla order (hoarder, unique, damage, bind, subcounter, spawn).
	protected void InsertLeaf(VPPXESpwNode rowNode, VPPXESpwNode leaf)
	{
		int at = rowNode.Kids.Count();
		for (int i = 0; i < rowNode.Kids.Count(); i++)
		{
			VPPXESpwNode kid = rowNode.Kids[i];
			if (!kid || kid.Kind == VPPXESpwKind.OTHER)
			{
				continue;
			}

			if (VPPXESpwKind.IsBlock(kid.Kind) || kid.Kind > leaf.Kind)
			{
				at = i;
				break;
			}
		}

		rowNode.Kids.InsertAt(leaf, at);
	}

	// A preset's namespace (cargo or attachments).
	protected void SetRowKind(int kind)
	{
		VPPXESpwWork work = SelectedWork();
		if (!work || !CanEdit() || !VPPXESpwKind.IsBlock(work.Row.Kind) || work.Row.Kind == kind)
		{
			return;
		}

		work.Row.Kind = kind;
		m_IndexStale = true;
		AfterEdit(true);
	}

	// ---------------------------------------------------------------- navigation

	protected void ResetNav()
	{
		m_Stack.Clear();
		m_Holder = null;
		m_BlockIdx = -1;
		m_ItemIdx = -1;
		VPPXESpwWork work = SelectedWork();
		if (!work)
		{
			return;
		}

		if (work.Row.Kind == VPPXESpwKind.TYPE)
		{
			m_Holder = work.Row;
			m_BlockIdx = FirstBlockIdx(m_Holder);
		}

		m_ItemIdx = FirstItemIdx(CurrentBlock());
	}

	protected int FirstBlockIdx(VPPXESpwNode holder)
	{
		if (!holder)
		{
			return -1;
		}

		for (int i = 0; i < holder.Kids.Count(); i++)
		{
			VPPXESpwNode kid = holder.Kids[i];
			if (kid && VPPXESpwKind.IsBlock(kid.Kind))
			{
				return i;
			}
		}

		return -1;
	}

	protected int FirstItemIdx(VPPXESpwNode blockNode)
	{
		if (!blockNode)
		{
			return -1;
		}

		for (int i = 0; i < blockNode.Kids.Count(); i++)
		{
			VPPXESpwNode kid = blockNode.Kids[i];
			if (kid && kid.Kind == VPPXESpwKind.ITEM)
			{
				return i;
			}
		}

		return -1;
	}

	// The shown block: a kid of the holder, or the preset row itself at the top of a preset.
	protected VPPXESpwNode CurrentBlock()
	{
		if (!m_Holder)
		{
			VPPXESpwWork work = SelectedWork();
			if (work && VPPXESpwKind.IsBlock(work.Row.Kind))
			{
				return work.Row;
			}

			return null;
		}

		if (m_BlockIdx < 0 || m_BlockIdx >= m_Holder.Kids.Count())
		{
			return null;
		}

		VPPXESpwNode blockNode = m_Holder.Kids[m_BlockIdx];
		if (!blockNode || !VPPXESpwKind.IsBlock(blockNode.Kind))
		{
			return null;
		}

		return blockNode;
	}

	protected VPPXESpwNode CurrentItem()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || m_ItemIdx < 0 || m_ItemIdx >= blockNode.Kids.Count())
		{
			return null;
		}

		VPPXESpwNode itemNode = blockNode.Kids[m_ItemIdx];
		if (!itemNode || itemNode.Kind != VPPXESpwKind.ITEM)
		{
			return null;
		}

		return itemNode;
	}

	// One level down: the selected item's own blocks.
	protected void NavIntoItem()
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || !CanNestDeeper())
		{
			return;
		}

		VPPXESpwNavFrame frame = new VPPXESpwNavFrame();
		frame.Holder = m_Holder;
		frame.BlockIdx = m_BlockIdx;
		frame.ItemIdx = m_ItemIdx;
		m_Stack.Insert(frame);
		m_Holder = itemNode;
		m_BlockIdx = FirstBlockIdx(m_Holder);
		m_ItemIdx = FirstItemIdx(CurrentBlock());
		LoadForm();
	}

	// One level down keeps the deepest node (an item's damage) within VPPXESpwRules.MAX_DEPTH: the items there sit at
	// depth 2 * levels + 2, their damage one deeper.
	protected bool CanNestDeeper()
	{
		int levels = m_Stack.Count() + 1;
		int deepest = 2 * levels + 3;
		return deepest <= VPPXESpwRules.MAX_DEPTH;
	}

	protected void NavBack()
	{
		int depth = m_Stack.Count();
		if (depth == 0)
		{
			return;
		}

		VPPXESpwNavFrame frame = m_Stack[depth - 1];
		m_Holder = frame.Holder;
		m_BlockIdx = frame.BlockIdx;
		m_ItemIdx = frame.ItemIdx;
		m_Stack.Remove(depth - 1);
		LoadForm();
	}

	// "M4A1 > attachments 3 > BUISOptic" of the shown level.
	protected string PathText()
	{
		VPPXESpwWork work = SelectedWork();
		if (!work)
		{
			return "";
		}

		string text = work.Row.Name;
		foreach (VPPXESpwNavFrame frame : m_Stack)
		{
			VPPXESpwNode holder = frame.Holder;
			VPPXESpwNode frameBlock = null;
			if (holder && frame.BlockIdx >= 0 && frame.BlockIdx < holder.Kids.Count())
			{
				frameBlock = holder.Kids[frame.BlockIdx];
			}
			else if (!holder)
			{
				frameBlock = work.Row;
			}

			if (!frameBlock)
			{
				continue;
			}

			string blockLabel = BlockLabel(holder, frame.BlockIdx, frameBlock);
			text = text + "  >  " + blockLabel;
			if (frame.ItemIdx >= 0 && frame.ItemIdx < frameBlock.Kids.Count())
			{
				VPPXESpwNode frameItem = frameBlock.Kids[frame.ItemIdx];
				if (frameItem)
				{
					text = text + "  >  " + frameItem.Name;
				}
			}
		}

		return text;
	}

	// "cargo 2" / "attachments 1" (numbered per kind within its holder).
	protected string BlockLabel(VPPXESpwNode holder, int kidIdx, VPPXESpwNode blockNode)
	{
		string kindText = KindText(blockNode.Kind);
		if (!holder)
		{
			return kindText;
		}

		int number = 0;
		for (int i = 0; i <= kidIdx && i < holder.Kids.Count(); i++)
		{
			VPPXESpwNode kid = holder.Kids[i];
			if (kid && kid.Kind == blockNode.Kind)
			{
				number++;
			}
		}

		return kindText + " " + number.ToString();
	}

	protected string KindText(int kind)
	{
		if (kind == VPPXESpwKind.ATTACHMENTS)
		{
			return Tr("#VSTR_XMLE_SPB_ATTACHMENTS_LOW");
		}

		return Tr("#VSTR_XMLE_SPB_CARGO_LOW");
	}

	// ---------------------------------------------------------------- blocks

	protected void AddBlock(int kind)
	{
		if (!m_Holder || !CanEdit())
		{
			return;
		}

		VPPXESpwNode blockNode = new VPPXESpwNode();
		blockNode.Kind = kind;
		m_Holder.Kids.Insert(blockNode);
		m_BlockIdx = m_Holder.Kids.Count() - 1;
		m_ItemIdx = -1;
		LoadForm();
		AfterEdit(false);
	}

	protected void DuplicateBlock()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || !CanEdit())
		{
			return;
		}

		VPPXESpwNode copy = new VPPXESpwNode();
		copy.CopyFrom(blockNode);
		AsNew(copy);
		m_Holder.Kids.InsertAt(copy, m_BlockIdx + 1);
		m_BlockIdx = m_BlockIdx + 1;
		m_ItemIdx = FirstItemIdx(copy);
		LoadForm();
		AfterEdit(true);
	}

	// Swaps the block with the previous (-1) or next (1) block of the holder (other children stay where they are).
	protected void MoveBlock(int moveDelta)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || !CanEdit())
		{
			return;
		}

		int other = m_BlockIdx + moveDelta;
		while (other >= 0 && other < m_Holder.Kids.Count())
		{
			VPPXESpwNode candidate = m_Holder.Kids[other];
			if (candidate && VPPXESpwKind.IsBlock(candidate.Kind))
			{
				break;
			}

			other = other + moveDelta;
		}

		if (other < 0 || other >= m_Holder.Kids.Count())
		{
			return;
		}

		m_Holder.Kids.SwapItems(m_BlockIdx, other);
		m_BlockIdx = other;
		LoadForm();
		AfterEdit(false);
	}

	protected void RemoveBlock()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || !CanEdit())
		{
			return;
		}

		m_Holder.Kids.RemoveOrdered(m_BlockIdx);
		m_BlockIdx = FirstBlockIdx(m_Holder);
		m_ItemIdx = FirstItemIdx(CurrentBlock());
		LoadForm();
		AfterEdit(true);
	}

	protected void SetBlockKind(int kind)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || !CanEdit() || blockNode.Kind == kind)
		{
			return;
		}

		blockNode.Kind = kind;
		LoadForm();
		AfterEdit(true);
	}

	// ---------------------------------------------------------------- items

	protected void AddItem()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || !CanEdit())
		{
			return;
		}

		VPPXESpwNode itemNode = new VPPXESpwNode();
		itemNode.Kind = VPPXESpwKind.ITEM;
		blockNode.Kids.Insert(itemNode);
		m_ItemIdx = blockNode.Kids.Count() - 1;
		LoadForm();
		AfterEdit(false);
		EditBoxWidget nameBox = m_Inputs[IN_ITEMNAME];
		if (nameBox)
		{
			SetFocus(nameBox);
		}
	}

	protected void MoveItem(int moveDelta)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || !CurrentItem() || !CanEdit())
		{
			return;
		}

		int other = m_ItemIdx + moveDelta;
		while (other >= 0 && other < blockNode.Kids.Count())
		{
			VPPXESpwNode candidate = blockNode.Kids[other];
			if (candidate && candidate.Kind == VPPXESpwKind.ITEM)
			{
				break;
			}

			other = other + moveDelta;
		}

		if (other < 0 || other >= blockNode.Kids.Count())
		{
			return;
		}

		blockNode.Kids.SwapItems(m_ItemIdx, other);
		m_ItemIdx = other;
		LoadForm();
		AfterEdit(false);
	}

	protected void RemoveItem()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || !CurrentItem() || !CanEdit())
		{
			return;
		}

		blockNode.Kids.RemoveOrdered(m_ItemIdx);
		m_ItemIdx = FirstItemIdx(blockNode);
		LoadForm();
		AfterEdit(false);
	}

	protected void ToggleItemEquip()
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || !CanEdit())
		{
			return;
		}

		if (IsOnText(itemNode.Equip))
		{
			itemNode.Equip = "";
		}
		else
		{
			itemNode.Equip = "1";
		}

		UpdateDetails();
		AfterEdit(false);
	}

	protected void ToggleItemDamage()
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || !CanEdit())
		{
			return;
		}

		VPPXESpwNode damageNode = itemNode.FirstKid(VPPXESpwKind.DAMAGE);
		if (damageNode)
		{
			itemNode.Kids.RemoveItem(damageNode);
		}
		else
		{
			damageNode = new VPPXESpwNode();
			damageNode.Kind = VPPXESpwKind.DAMAGE;
			damageNode.Min = "0";
			damageNode.Max = "0.3";
			itemNode.Kids.InsertAt(damageNode, 0);
		}

		LoadForm();
		AfterEdit(false);
	}

	// The first suggestion into the name box.
	protected void CompleteItemName()
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || !CanEdit() || m_Suggestions.Count() == 0)
		{
			return;
		}

		string suggestion = m_Suggestions[0];
		itemNode.Name = suggestion;
		m_Loading = true;
		m_Inputs[IN_ITEMNAME].SetText(suggestion);
		m_Last.Set(IN_ITEMNAME, suggestion);
		m_Loading = false;
		AfterEdit(false);
	}

	// Config classes starting with the typed text (case-insensitive), at most MAX_SUGGESTIONS; none when it is a class.
	protected void RebuildSuggestions(string typed)
	{
		m_Suggestions.Clear();
		string prefix = typed.Trim();
		prefix.ToLower();
		if (prefix.Length() < 2)
		{
			return;
		}

		VPPXEConfigClasses.EnsureScanned();
		if (VPPXEConfigClasses.Has(prefix))
		{
			return;
		}

		int total = VPPXEConfigClasses.Count();
		int prefixLen = prefix.Length();
		for (int i = 0; i < total && m_Suggestions.Count() < MAX_SUGGESTIONS; i++)
		{
			string lower = VPPXEConfigClasses.LowerAt(i);
			if (lower.Length() >= prefixLen && lower.Substring(0, prefixLen) == prefix)
			{
				string shown = VPPXEConfigClasses.DisplayOf(lower);
				m_Suggestions.Insert(shown);
			}
		}
	}

	// ---------------------------------------------------------------- inputs

	// Fills every box from the shown row, block and item (or shows the empty state).
	protected void LoadForm()
	{
		VPPXESpwWork work = SelectedWork();
		ShowEditor(work != null);
		if (!work)
		{
			UpdateEmptyText();
			return;
		}

		m_Loading = true;
		VPPXESpwNode rowNode = work.Row;
		SetInput(IN_NAME, rowNode.Name);
		SetInput(IN_ROWCHANCE, rowNode.Chance);
		VPPXESpwNode damageNode = rowNode.FirstKid(VPPXESpwKind.DAMAGE);
		string dmgMin = "";
		string dmgMax = "";
		if (damageNode)
		{
			dmgMin = damageNode.Min;
			dmgMax = damageNode.Max;
		}

		SetInput(IN_DMGMIN, dmgMin);
		SetInput(IN_DMGMAX, dmgMax);
		string bindText = LeafText(rowNode, VPPXESpwKind.BIND, 0);
		string subMinText = LeafText(rowNode, VPPXESpwKind.SUBCOUNTER, 1);
		string subMaxText = LeafText(rowNode, VPPXESpwKind.SUBCOUNTER, 2);
		string batchText = LeafText(rowNode, VPPXESpwKind.SPAWN, 1);
		SetInput(IN_BIND, bindText);
		SetInput(IN_SUBMIN, subMinText);
		SetInput(IN_SUBMAX, subMaxText);
		SetInput(IN_BATCH, batchText);
		VPPXESpwNode blockNode = CurrentBlock();
		string blockChance = "";
		string blockPreset = "";
		if (blockNode)
		{
			blockChance = blockNode.Chance;
			blockPreset = blockNode.Preset;
		}

		SetInput(IN_BLKCHANCE, blockChance);
		SetInput(IN_BLKPRESET, blockPreset);
		LoadItemInputs();
		m_Loading = false;
		RebuildBlockList();
		RebuildItemList();
		RebuildPresetDropdown();
		RebuildSlotDropdown();
		UpdateDetails();
	}

	protected void LoadItemInputs()
	{
		VPPXESpwNode itemNode = CurrentItem();
		string itemName = "";
		string itemChance = "";
		string quantMin = "";
		string quantMax = "";
		string itemDmgMin = "";
		string itemDmgMax = "";
		if (itemNode)
		{
			itemName = itemNode.Name;
			itemChance = itemNode.Chance;
			quantMin = itemNode.QuantMin;
			quantMax = itemNode.QuantMax;
			VPPXESpwNode itemDamage = itemNode.FirstKid(VPPXESpwKind.DAMAGE);
			if (itemDamage)
			{
				itemDmgMin = itemDamage.Min;
				itemDmgMax = itemDamage.Max;
			}
		}

		SetInput(IN_ITEMNAME, itemName);
		SetInput(IN_ITEMCHANCE, itemChance);
		SetInput(IN_QMIN, quantMin);
		SetInput(IN_QMAX, quantMax);
		SetInput(IN_IDMGMIN, itemDmgMin);
		SetInput(IN_IDMGMAX, itemDmgMax);
		RebuildSuggestions(itemName);
	}

	// part: 0 = Name, 1 = Min, 2 = Max of the first child of that kind ("" when none).
	protected string LeafText(VPPXESpwNode rowNode, int kind, int part)
	{
		VPPXESpwNode leaf = rowNode.FirstKid(kind);
		if (!leaf)
		{
			return "";
		}

		if (part == 0)
		{
			return leaf.Name;
		}

		if (part == 1)
		{
			return leaf.Min;
		}

		return leaf.Max;
	}

	protected void SetInput(int idx, string value)
	{
		EditBoxWidget box = m_Inputs[idx];
		if (box)
		{
			box.SetText(value);
		}

		m_Last.Set(idx, value);
	}

	protected void PollInputs()
	{
		VPPXESpwWork work = SelectedWork();
		if (!work || m_Loading || m_View != VIEW_EDIT)
		{
			return;
		}

		bool changed = false;
		bool reloadNeeded = false;
		for (int i = 0; i < INPUT_COUNT; i++)
		{
			EditBoxWidget box = m_Inputs[i];
			if (!box)
			{
				continue;
			}

			string text = box.GetText();
			if (text == m_Last[i])
			{
				continue;
			}

			m_Last.Set(i, text);
			changed = true;
			if (!CanEdit())
			{
				reloadNeeded = true;
				continue;
			}

			string trimmed = text.Trim();
			ApplyInput(work, i, trimmed);
		}

		if (!changed)
		{
			return;
		}

		if (reloadNeeded)
		{
			LoadForm();
			return;
		}

		AfterEdit(false);
	}

	protected void ApplyInput(VPPXESpwWork work, int idx, string value)
	{
		VPPXESpwNode rowNode = work.Row;
		if (idx == IN_NAME)
		{
			rowNode.Name = value;
			m_IndexStale = true;
		}
		else if (idx == IN_ROWCHANCE)
		{
			rowNode.Chance = value;
		}
		else if (idx == IN_DMGMIN || idx == IN_DMGMAX)
		{
			VPPXESpwNode damageNode = rowNode.FirstKid(VPPXESpwKind.DAMAGE);
			if (damageNode && idx == IN_DMGMIN)
			{
				damageNode.Min = value;
			}
			else if (damageNode)
			{
				damageNode.Max = value;
			}
		}
		else if (idx == IN_BIND || idx == IN_SUBMIN || idx == IN_SUBMAX || idx == IN_BATCH)
		{
			ApplyAdvanced(rowNode, idx, value);
		}
		else if (idx == IN_BLKCHANCE || idx == IN_BLKPRESET)
		{
			ApplyBlockInput(idx, value);
		}
		else
		{
			ApplyItemInput(idx, value);
		}
	}

	// BIND / SUBCOUNTER / SPAWN: added with the first value, removed when every value is cleared.
	protected void ApplyAdvanced(VPPXESpwNode rowNode, int idx, string value)
	{
		int kind = VPPXESpwKind.SPAWN;
		if (idx == IN_BIND)
		{
			kind = VPPXESpwKind.BIND;
		}
		else if (idx == IN_SUBMIN || idx == IN_SUBMAX)
		{
			kind = VPPXESpwKind.SUBCOUNTER;
		}

		VPPXESpwNode leaf = rowNode.FirstKid(kind);
		if (!leaf)
		{
			if (value == "")
			{
				return;
			}

			leaf = new VPPXESpwNode();
			leaf.Kind = kind;
			InsertLeaf(rowNode, leaf);
		}

		if (idx == IN_BIND)
		{
			leaf.Name = value;
		}
		else if (idx == IN_SUBMAX)
		{
			leaf.Max = value;
		}
		else
		{
			leaf.Min = value;
		}

		if (leaf.Name == "" && leaf.Min == "" && leaf.Max == "")
		{
			rowNode.Kids.RemoveItem(leaf);
		}
	}

	protected void ApplyBlockInput(int idx, string value)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode)
		{
			return;
		}

		if (idx == IN_BLKCHANCE)
		{
			blockNode.Chance = value;
			return;
		}

		blockNode.Preset = value;
		m_IndexStale = true;
	}

	protected void ApplyItemInput(int idx, string value)
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode)
		{
			return;
		}

		if (idx == IN_ITEMNAME)
		{
			itemNode.Name = value;
			RebuildSuggestions(value);
		}
		else if (idx == IN_ITEMCHANCE)
		{
			itemNode.Chance = value;
		}
		else if (idx == IN_QMIN)
		{
			itemNode.QuantMin = value;
		}
		else if (idx == IN_QMAX)
		{
			itemNode.QuantMax = value;
		}
		else
		{
			VPPXESpwNode itemDamage = itemNode.FirstKid(VPPXESpwKind.DAMAGE);
			if (itemDamage && idx == IN_IDMGMIN)
			{
				itemDamage.Min = value;
			}
			else if (itemDamage)
			{
				itemDamage.Max = value;
			}
		}
	}

	// After an edit: the rows of the lists that show it, the checks and the status. listsToo: the block and item lists
	// (and the preset picker) are rebuilt too.
	protected void AfterEdit(bool listsToo)
	{
		if (listsToo)
		{
			RebuildBlockList();
			RebuildItemList();
			RebuildPresetDropdown();
		}
		else
		{
			RefreshBlockRow();
			RefreshItemRow();
		}

		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
		BumpData();
	}

	// ---------------------------------------------------------------- save and revert

	protected void StartSave()
	{
		VPPXESpwFile file = CurrentFile();
		if (!file || !CanEdit())
		{
			return;
		}

		int dirty = file.DirtyCount();
		if (dirty == 0)
		{
			return;
		}

		bool presetsFile = file.Kind == VPPXEFileKind.RANDOMPRESETS;
		int workCount = file.Work.Count();
		for (int i = 0; i < workCount; i++)
		{
			VPPXESpwWork work = file.Work[i];
			if (work.Deleted || !work.IsDirty())
			{
				continue;
			}

			string problem = VPPXESpwRules.CheckRow(work.Row, presetsFile);
			if (problem != "")
			{
				m_SelWork = i;
				ResetNav();
				RebuildList();
				LoadForm();
				string problemText = Tr(problem);
				m_Owner.NotifyError(work.Row.Name + ": " + problemText);
				return;
			}
		}

		string fileLabel = FileLabel(file.Key);
		string pattern = Tr("#VSTR_XMLE_SPB_DLG_SAVE_BODY");
		string body = string.Format(pattern, dirty, fileLabel);
		m_Owner.OpenConfirm("#VSTR_XMLE_SPB_DLG_SAVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmSave", false);
	}

	void OnConfirmSave(int result, string input)
	{
		VPPXESpwFile file = CurrentFile();
		if (result != DIAGRESULT.YES || !file || !CanEdit())
		{
			return;
		}

		array<ref VPPXESpwEdit> edits = new array<ref VPPXESpwEdit>;
		foreach (VPPXESpwWork work : file.Work)
		{
			if (!work.IsDirty())
			{
				continue;
			}

			VPPXESpwEdit edit = new VPPXESpwEdit();
			if (work.OrigIndex < 0)
			{
				if (work.Deleted)
				{
					continue;
				}

				edit.Op = VPPXEOp.ADD;
				edit.Index = -1;
			}
			else if (work.Deleted)
			{
				edit.Op = VPPXEOp.DELETE;
				edit.Index = work.OrigIndex;
			}
			else
			{
				edit.Op = VPPXEOp.UPDATE;
				edit.Index = work.OrigIndex;
			}

			edit.Row.CopyFrom(work.Row);
			edits.Insert(edit);
		}

		if (edits.Count() == 0)
		{
			return;
		}

		// parts of at most EDITS_PER_PART edits and about CHUNK_BYTES each
		m_SaveReqId = m_Owner.NextReqId();
		array<ref VPPXESpawnablesSave> parts = new array<ref VPPXESpawnablesSave>;
		VPPXESpawnablesSave part = null;
		int partBytes = 0;
		foreach (VPPXESpwEdit queued : edits)
		{
			int size = VPPXESpwRules.EstimateNode(queued.Row) + 16;
			if (!part || part.Edits.Count() >= VPPXESpwRules.EDITS_PER_PART || partBytes + size > VPPXESpwRules.CHUNK_BYTES)
			{
				part = new VPPXESpawnablesSave();
				part.ReqId = m_SaveReqId;
				part.PartIdx = parts.Count();
				part.FileKey = file.Key;
				part.BaseRevision = file.Revision;
				parts.Insert(part);
				partBytes = 0;
			}

			part.Edits.Insert(queued);
			partBytes += size;
		}

		m_Saving = true;
		m_SaveSentAt = NowMs();
		m_SaveFile = file.Key;
		int partCount = parts.Count();
		foreach (VPPXESpawnablesSave sendPart : parts)
		{
			sendPart.PartCount = partCount;
			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveSpawnables", new Param1<ref VPPXESpawnablesSave>(sendPart), true, null);
		}

		string savingText = Tr("#VSTR_XMLE_STATUS_SAVING");
		SetStatus(savingText, false);
		UpdateDetails();
	}

	protected void StartRevert()
	{
		VPPXESpwFile file = CurrentFile();
		if (!file || m_Saving)
		{
			return;
		}

		int dirty = file.DirtyCount();
		if (dirty == 0 && !file.ServerChanged)
		{
			return;
		}

		string fileLabel = FileLabel(file.Key);
		string pattern = Tr("#VSTR_XMLE_MSG_DLG_REVERT_BODY");
		string body = string.Format(pattern, dirty, fileLabel);
		m_Owner.OpenConfirm("#VSTR_XMLE_MSG_DLG_REVERT_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRevert", false);
	}

	void OnConfirmRevert(int result, string input)
	{
		VPPXESpwFile file = CurrentFile();
		if (result != DIAGRESULT.YES || !file)
		{
			return;
		}

		m_ForceFresh.Set(file.Key, true);
		RequestList();
	}

	// ---------------------------------------------------------------- lists and dropdowns

	protected void RebuildAll()
	{
		RebuildFileDropdown();
		UpdateModeChips();
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	protected array<string> ModeFileKeys()
	{
		if (m_Mode == MODE_PRESETS)
		{
			return m_PresetFileKeys;
		}

		return m_TypeFileKeys;
	}

	protected void RebuildFileDropdown()
	{
		if (!m_FileDD)
		{
			return;
		}

		m_FileDD.RemoveAllElements();
		string current = "";
		string shownKey = CurrentFileKey();
		array<string> keys = ModeFileKeys();
		foreach (string fileKey : keys)
		{
			string label = FileDropdownLabel(fileKey);
			m_FileDD.AddElement(label);
			if (fileKey == shownKey)
			{
				current = label;
			}
		}

		if (current == "")
		{
			current = Tr("#VSTR_XMLE_SPB_NO_FILE_SHORT");
		}

		m_FileDD.SetText(current);
	}

	protected string FileDropdownLabel(string fileKey)
	{
		string label = FileLabel(fileKey);
		VPPXESpwFile file = m_Files.Get(fileKey);
		if (!file)
		{
			return label;
		}

		int shown = 0;
		foreach (VPPXESpwWork work : file.Work)
		{
			if (!work.Deleted)
			{
				shown++;
			}
		}

		label = label + " (" + shown.ToString() + ")";
		if (file.DirtyCount() > 0)
		{
			label = label + " *";
		}

		return label;
	}

	protected void RebuildFilterDropdown()
	{
		if (!m_FilterDD)
		{
			return;
		}

		m_FilterDD.RemoveAllElements();
		for (int i = 0; i < FILTER_COUNT; i++)
		{
			string label = FilterLabel(i);
			m_FilterDD.AddElement(label);
		}

		string currentLabel = FilterLabel(m_FilterIdx);
		m_FilterDD.SetText(currentLabel);
	}

	protected string FilterLabel(int filterIdx)
	{
		if (filterIdx == FILTER_PROBLEMS)
		{
			return Tr("#VSTR_XMLE_SPB_F_PROBLEMS");
		}

		if (filterIdx == FILTER_UNSAVED)
		{
			return Tr("#VSTR_XMLE_SPB_F_UNSAVED");
		}

		if (m_Mode == MODE_PRESETS)
		{
			if (filterIdx == FILTER_A)
			{
				return Tr("#VSTR_XMLE_SPB_F_CARGO_PRESETS");
			}

			if (filterIdx == FILTER_B)
			{
				return Tr("#VSTR_XMLE_SPB_F_ATT_PRESETS");
			}

			if (filterIdx == FILTER_C)
			{
				return Tr("#VSTR_XMLE_SPB_F_UNUSED");
			}

			if (filterIdx == FILTER_D)
			{
				return Tr("#VSTR_XMLE_SPB_F_SHADOWED");
			}

			return Tr("#VSTR_XMLE_SPB_F_ALL");
		}

		if (filterIdx == FILTER_A)
		{
			return Tr("#VSTR_XMLE_SPB_F_HOARDER");
		}

		if (filterIdx == FILTER_B)
		{
			return Tr("#VSTR_XMLE_SPB_F_DAMAGE");
		}

		if (filterIdx == FILTER_C)
		{
			return Tr("#VSTR_XMLE_SPB_F_CARGO");
		}

		if (filterIdx == FILTER_D)
		{
			return Tr("#VSTR_XMLE_SPB_F_ATTACHMENTS");
		}

		return Tr("#VSTR_XMLE_SPB_F_ALL");
	}

	protected void PollSearch()
	{
		if (!m_InputSearch)
		{
			return;
		}

		string text = m_InputSearch.GetText();
		string searchDefault = Tr("#VSTR_SEARCH");
		if (text == searchDefault)
		{
			text = "";
		}

		text = text.Trim();
		text.ToLower();
		if (text == m_LastSearch)
		{
			return;
		}

		m_LastSearch = text;
		RebuildList();
		LoadForm();
	}

	protected void RebuildList()
	{
		RebuildListRows();
		RebuildBulkDropdown();
	}

	protected void RebuildListRows()
	{
		if (!m_List)
		{
			return;
		}

		m_List.ClearItems();
		m_ListWork.Clear();
		m_ListSel = -1;
		VPPXESpwFile file = CurrentFile();
		if (!file)
		{
			return;
		}

		int keepRow = -1;
		for (int i = 0; i < file.Work.Count(); i++)
		{
			VPPXESpwWork work = file.Work[i];
			if (work.Deleted || !PassesFilter(file, i, work) || !MatchesSearch(work.Row))
			{
				continue;
			}

			int row = m_List.AddItem("", null, 0);
			m_ListWork.Insert(i);
			FillListRow(row, file, i, work);
			if (i == m_SelWork)
			{
				keepRow = row;
			}
		}

		if (keepRow >= 0)
		{
			m_List.SelectRow(keepRow);
			m_List.EnsureVisible(keepRow);
			m_ListSel = keepRow;
		}
		else
		{
			m_SelWork = -1;
			ResetNav();
		}
	}

	protected bool PassesFilter(VPPXESpwFile file, int workIdx, VPPXESpwWork work)
	{
		if (m_FilterIdx == FILTER_ALL)
		{
			return true;
		}

		if (m_FilterIdx == FILTER_UNSAVED)
		{
			return work.IsDirty();
		}

		if (m_FilterIdx == FILTER_PROBLEMS)
		{
			return RowSeverity(file, workIdx, work) > 0;
		}

		VPPXESpwNode rowNode = work.Row;
		if (m_Mode == MODE_PRESETS)
		{
			if (m_FilterIdx == FILTER_A)
			{
				return rowNode.Kind == VPPXESpwKind.CARGO;
			}

			if (m_FilterIdx == FILTER_B)
			{
				return rowNode.Kind == VPPXESpwKind.ATTACHMENTS;
			}

			string key = PresetKey(rowNode.Kind, rowNode.Name);
			EnsureIndex();
			if (m_FilterIdx == FILTER_C)
			{
				return m_PresetUses.Get(key) == 0;
			}

			return PresetOrderOf(file.Key, workIdx) < 0;
		}

		if (m_FilterIdx == FILTER_A)
		{
			return rowNode.HasKid(VPPXESpwKind.HOARDER);
		}

		if (m_FilterIdx == FILTER_B)
		{
			return rowNode.HasKid(VPPXESpwKind.DAMAGE);
		}

		if (m_FilterIdx == FILTER_C)
		{
			return rowNode.HasKid(VPPXESpwKind.CARGO);
		}

		return rowNode.HasKid(VPPXESpwKind.ATTACHMENTS);
	}

	// The search text in the row's name or in any item name (deep).
	protected bool MatchesSearch(VPPXESpwNode node)
	{
		if (m_LastSearch == "")
		{
			return true;
		}

		string lower = node.Name;
		lower.ToLower();
		if (lower.IndexOf(m_LastSearch) >= 0)
		{
			return true;
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (kid && kid.Kind != VPPXESpwKind.OTHER && MatchesSearch(kid))
			{
				return true;
			}
		}

		return false;
	}

	protected void FillListRow(int row, VPPXESpwFile file, int workIdx, VPPXESpwWork work)
	{
		VPPXESpwNode rowNode = work.Row;
		string nameText = rowNode.Name;
		if (work.OrigIndex < 0)
		{
			nameText = nameText + " +";
		}
		else if (work.IsDirty())
		{
			nameText = nameText + " *";
		}

		string contentText = ContentText(rowNode);
		string infoText = InfoText(file, workIdx, rowNode);
		m_List.SetItem(row, nameText, null, 0);
		m_List.SetItem(row, contentText, null, 1);
		m_List.SetItem(row, infoText, null, 2);
		int color = ARGB(255, 255, 255, 255);
		int severity = RowSeverity(file, workIdx, work);
		if (severity >= 2)
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (work.IsDirty())
		{
			color = ARGB(255, 232, 163, 61);
		}
		else if (severity == 1)
		{
			color = ARGB(255, 217, 178, 61);
		}

		for (int column = 0; column < 3; column++)
		{
			m_List.SetItemColor(row, column, color);
		}
	}

	// Types: "2 cargo, 3 att, hoarder"; presets: "12 items" (with its own preset reference).
	protected string ContentText(VPPXESpwNode rowNode)
	{
		array<string> parts = new array<string>;
		if (rowNode.Kind == VPPXESpwKind.TYPE)
		{
			int cargo = rowNode.CountKind(VPPXESpwKind.CARGO);
			int attCount = rowNode.CountKind(VPPXESpwKind.ATTACHMENTS);
			if (cargo > 0)
			{
				string cargoPattern = Tr("#VSTR_XMLE_SPB_N_CARGO");
				parts.Insert(string.Format(cargoPattern, cargo));
			}

			if (attCount > 0)
			{
				string attPattern = Tr("#VSTR_XMLE_SPB_N_ATT");
				parts.Insert(string.Format(attPattern, attCount));
			}

			if (rowNode.HasKid(VPPXESpwKind.HOARDER))
			{
				parts.Insert(Tr("#VSTR_XMLE_SPB_HOARDER_LOW"));
			}

			if (rowNode.HasKid(VPPXESpwKind.UNIQUE))
			{
				parts.Insert(Tr("#VSTR_XMLE_SPB_UNIQUE_LOW"));
			}
		}
		else
		{
			int items = rowNode.CountKind(VPPXESpwKind.ITEM);
			string itemsPattern = Tr("#VSTR_XMLE_SPB_N_ITEMS");
			parts.Insert(string.Format(itemsPattern, items));
			if (rowNode.Preset != "")
			{
				parts.Insert("+" + rowNode.Preset);
			}
		}

		return JoinWith(parts, ", ");
	}

	// Types: the damage range; presets: the chance and how many blocks use it.
	protected string InfoText(VPPXESpwFile file, int workIdx, VPPXESpwNode rowNode)
	{
		if (rowNode.Kind == VPPXESpwKind.TYPE)
		{
			VPPXESpwNode damageNode = rowNode.FirstKid(VPPXESpwKind.DAMAGE);
			if (damageNode)
			{
				return damageNode.Min + "-" + damageNode.Max;
			}

			return "";
		}

		EnsureIndex();
		float chance = VPPXESpwRules.ChanceOf(rowNode.Chance, -1);
		string chanceText = VPPXESpwRules.FormatNumber(chance);
		string key = PresetKey(rowNode.Kind, rowNode.Name);
		int uses = m_PresetUses.Get(key);
		return chanceText + " / " + uses.ToString();
	}

	protected void RefreshSelectedRow()
	{
		VPPXESpwWork work = SelectedWork();
		VPPXESpwFile file = CurrentFile();
		if (!work || !file || m_ListSel < 0)
		{
			return;
		}

		FillListRow(m_ListSel, file, m_SelWork, work);
		if (m_FileDD)
		{
			string label = FileDropdownLabel(file.Key);
			m_FileDD.SetText(label);
		}
	}

	protected void PollListSelection()
	{
		if (!m_List)
		{
			return;
		}

		int row = m_List.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		m_ListSel = row;
		m_SelWork = -1;
		if (row >= 0 && row < m_ListWork.Count())
		{
			m_SelWork = m_ListWork[row];
		}

		ResetNav();
		LoadForm();
		UpdateActionBar();
	}

	// The blocks of the holder (hidden at the top of a preset).
	protected void RebuildBlockList()
	{
		if (!m_BlockList)
		{
			return;
		}

		m_BlockList.ClearItems();
		m_BlockRows.Clear();
		m_BlockListSel = -1;
		if (!m_Holder)
		{
			return;
		}

		for (int i = 0; i < m_Holder.Kids.Count(); i++)
		{
			VPPXESpwNode kid = m_Holder.Kids[i];
			if (!kid || !VPPXESpwKind.IsBlock(kid.Kind))
			{
				continue;
			}

			int row = m_BlockList.AddItem("", null, 0);
			m_BlockRows.Insert(i);
			FillBlockRow(row, i, kid);
			if (i == m_BlockIdx)
			{
				m_BlockListSel = row;
			}
		}

		if (m_BlockListSel >= 0)
		{
			m_BlockList.SelectRow(m_BlockListSel);
			m_BlockList.EnsureVisible(m_BlockListSel);
		}
	}

	protected void FillBlockRow(int row, int kidIdx, VPPXESpwNode blockNode)
	{
		int number = row + 1;
		string kindText = KindText(blockNode.Kind);
		string content = BlockContentText(blockNode);
		float chance = EffectiveChance(blockNode);
		string chanceText = VPPXESpwRules.FormatNumber(chance);
		if (blockNode.Chance == "")
		{
			chanceText = chanceText + "*";
		}

		string numberText = number.ToString();
		m_BlockList.SetItem(row, numberText, null, 0);
		m_BlockList.SetItem(row, kindText, null, 1);
		m_BlockList.SetItem(row, content, null, 2);
		m_BlockList.SetItem(row, chanceText, null, 3);
		int color = ARGB(255, 255, 255, 255);
		if (BlockSeverity(blockNode) >= 2)
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (BlockSeverity(blockNode) == 1)
		{
			color = ARGB(255, 217, 178, 61);
		}

		for (int column = 0; column < 4; column++)
		{
			m_BlockList.SetItemColor(row, column, color);
		}
	}

	// "preset mixArmy" and/or the first item names.
	protected string BlockContentText(VPPXESpwNode blockNode)
	{
		array<string> names = new array<string>;
		foreach (VPPXESpwNode kid : blockNode.Kids)
		{
			if (kid && kid.Kind == VPPXESpwKind.ITEM)
			{
				names.Insert(kid.Name);
			}
		}

		string text = "";
		if (blockNode.Preset != "")
		{
			text = Tr("#VSTR_XMLE_SPB_PRESET_LOW") + " " + blockNode.Preset;
		}

		if (names.Count() > 0)
		{
			string itemsText = names[0];
			if (names.Count() > 1)
			{
				int more = names.Count() - 1;
				itemsText = itemsText + " +" + more.ToString();
			}

			if (text != "")
			{
				text = text + ", ";
			}

			text = text + itemsText;
		}

		if (text == "")
		{
			text = Tr("#VSTR_XMLE_SPB_EMPTY_BLOCK");
		}

		return text;
	}

	protected void RefreshBlockRow()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || m_BlockListSel < 0 || !m_Holder)
		{
			return;
		}

		FillBlockRow(m_BlockListSel, m_BlockIdx, blockNode);
	}

	protected void PollBlockList()
	{
		if (!m_BlockList || !m_Holder || m_View != VIEW_EDIT)
		{
			return;
		}

		int row = m_BlockList.GetSelectedRow();
		if (row == m_BlockListSel)
		{
			return;
		}

		m_BlockListSel = row;
		m_BlockIdx = -1;
		if (row >= 0 && row < m_BlockRows.Count())
		{
			m_BlockIdx = m_BlockRows[row];
		}

		m_ItemIdx = FirstItemIdx(CurrentBlock());
		LoadForm();
	}

	protected void RebuildItemList()
	{
		if (!m_ItemList)
		{
			return;
		}

		m_ItemList.ClearItems();
		m_ItemRows.Clear();
		m_ItemListSel = -1;
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode)
		{
			return;
		}

		for (int i = 0; i < blockNode.Kids.Count(); i++)
		{
			VPPXESpwNode kid = blockNode.Kids[i];
			if (!kid || kid.Kind != VPPXESpwKind.ITEM)
			{
				continue;
			}

			int row = m_ItemList.AddItem("", null, 0);
			m_ItemRows.Insert(i);
			FillItemRow(row, kid);
			if (i == m_ItemIdx)
			{
				m_ItemListSel = row;
			}
		}

		if (m_ItemListSel >= 0)
		{
			m_ItemList.SelectRow(m_ItemListSel);
			m_ItemList.EnsureVisible(m_ItemListSel);
		}
	}

	protected void FillItemRow(int row, VPPXESpwNode itemNode)
	{
		string nameText = itemNode.Name;
		if (nameText == "")
		{
			nameText = "?";
		}

		float chance = VPPXESpwRules.ChanceOf(itemNode.Chance, 1.0);
		string chanceText = VPPXESpwRules.FormatNumber(chance);
		string oddsText = OddsTextOf(itemNode);
		if (oddsText != "")
		{
			chanceText = chanceText + "  (" + oddsText + ")";
		}

		string quantText = "";
		if (itemNode.QuantMin != "" || itemNode.QuantMax != "")
		{
			quantText = itemNode.QuantMin + "-" + itemNode.QuantMax + "%";
		}

		array<string> more = new array<string>;
		if (IsOnText(itemNode.Equip))
		{
			more.Insert(Tr("#VSTR_XMLE_SPB_EQUIP_LOW"));
		}

		if (itemNode.HasKid(VPPXESpwKind.DAMAGE))
		{
			more.Insert(Tr("#VSTR_XMLE_SPB_DAMAGE_LOW"));
		}

		int nested = itemNode.BlockCount();
		if (nested > 0)
		{
			more.Insert("+" + nested.ToString());
		}

		string moreText = JoinWith(more, ", ");
		m_ItemList.SetItem(row, nameText, null, 0);
		m_ItemList.SetItem(row, chanceText, null, 1);
		m_ItemList.SetItem(row, quantText, null, 2);
		m_ItemList.SetItem(row, moreText, null, 3);
		int color = ARGB(255, 255, 255, 255);
		int severity = ItemSeverity(itemNode);
		if (severity >= 2)
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (severity == 1)
		{
			color = ARGB(255, 217, 178, 61);
		}

		for (int column = 0; column < 4; column++)
		{
			m_ItemList.SetItemColor(row, column, color);
		}
	}

	protected void RefreshItemRow()
	{
		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || m_ItemListSel < 0)
		{
			return;
		}

		FillItemRow(m_ItemListSel, itemNode);
	}

	protected void PollItemList()
	{
		if (!m_ItemList || !CurrentBlock() || m_View != VIEW_EDIT)
		{
			return;
		}

		int row = m_ItemList.GetSelectedRow();
		if (row == m_ItemListSel)
		{
			return;
		}

		m_ItemListSel = row;
		m_ItemIdx = -1;
		if (row >= 0 && row < m_ItemRows.Count())
		{
			m_ItemIdx = m_ItemRows[row];
		}

		m_Loading = true;
		LoadItemInputs();
		m_Loading = false;
		UpdateDetails();
	}

	// The preset names of the shown block's namespace (sorted), after "(no preset)".
	protected void RebuildPresetDropdown()
	{
		if (!m_PresetDD)
		{
			return;
		}

		m_PresetDD.RemoveAllElements();
		m_PresetChoices.Clear();
		VPPXESpwNode blockNode = CurrentBlock();
		string noneText = Tr("#VSTR_XMLE_SPB_NO_PRESET");
		m_PresetDD.AddElement(noneText);
		if (!blockNode)
		{
			m_PresetDD.SetText(noneText);
			return;
		}

		EnsureIndex();
		string prefix = "c:";
		if (blockNode.Kind == VPPXESpwKind.ATTACHMENTS)
		{
			prefix = "a:";
		}

		for (int i = 0; i < m_PresetFirst.Count(); i++)
		{
			string key = m_PresetFirst.GetKey(i);
			VPPXESpwPresetRef found = m_PresetFirst.GetElement(i);
			if (key.IndexOf(prefix) == 0 && found.Row)
			{
				m_PresetChoices.Insert(found.Row.Name);
			}
		}

		m_PresetChoices.Sort();
		foreach (string choice : m_PresetChoices)
		{
			m_PresetDD.AddElement(choice);
		}

		string shown = noneText;
		if (blockNode.Preset != "")
		{
			shown = blockNode.Preset;
		}

		m_PresetDD.SetText(shown);
	}

	// ---------------------------------------------------------------- checks

	// 0 = fine, 1 = warnings, 2 = errors (a hard problem SAVE refuses, or something the server drops).
	protected int RowSeverity(VPPXESpwFile file, int workIdx, VPPXESpwWork work)
	{
		VPPXESpwNode rowNode = work.Row;
		bool presetsFile = file.Kind == VPPXEFileKind.RANDOMPRESETS;
		if (VPPXESpwRules.CheckRow(rowNode, presetsFile) != "")
		{
			return 2;
		}

		int severity = 0;
		if (rowNode.Kind == VPPXESpwKind.TYPE && !IsCeType(rowNode.Name))
		{
			severity = 2;
		}

		if (presetsFile)
		{
			if (PresetOrderOf(file.Key, workIdx) < 0)
			{
				severity = 2;
			}
			else if (rowNode.CountKind(VPPXESpwKind.ITEM) == 0 && rowNode.Preset == "")
			{
				severity = 2;
			}
		}

		int kidSeverity = NodeSeverity(rowNode);
		if (kidSeverity > severity)
		{
			severity = kidSeverity;
		}

		return severity;
	}

	protected int NodeSeverity(VPPXESpwNode node)
	{
		int severity = 0;
		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (!kid)
			{
				continue;
			}

			int kidSeverity = 0;
			if (VPPXESpwKind.IsBlock(kid.Kind))
			{
				kidSeverity = BlockSeverity(kid);
			}
			else if (kid.Kind == VPPXESpwKind.ITEM)
			{
				kidSeverity = ItemSeverity(kid);
			}

			if (kidSeverity > severity)
			{
				severity = kidSeverity;
			}
		}

		return severity;
	}

	protected int BlockSeverity(VPPXESpwNode blockNode)
	{
		int severity = 0;
		if (blockNode.Preset != "" && !FindPreset(blockNode.Kind, blockNode.Preset))
		{
			severity = 1;
		}

		if (blockNode.Preset == "" && blockNode.CountKind(VPPXESpwKind.ITEM) == 0)
		{
			severity = 1;
		}

		int kidSeverity = NodeSeverity(blockNode);
		if (kidSeverity > severity)
		{
			severity = kidSeverity;
		}

		return severity;
	}

	protected int ItemSeverity(VPPXESpwNode itemNode)
	{
		if (!VPPXmlText.IsValidClassName(itemNode.Name))
		{
			return 2;
		}

		int severity = 0;
		if (IsIgnoredName(itemNode.Name) || !ExistsInConfig(itemNode.Name))
		{
			severity = 1;
		}

		int kidSeverity = NodeSeverity(itemNode);
		if (kidSeverity > severity)
		{
			severity = kidSeverity;
		}

		return severity;
	}

	// What the server does with the row, as lines: the hard problem first, then warnings and notes.
	protected string BuildChecks(VPPXESpwFile file, int workIdx, VPPXESpwWork work)
	{
		array<string> lines = new array<string>;
		VPPXESpwNode rowNode = work.Row;
		bool presetsFile = file.Kind == VPPXEFileKind.RANDOMPRESETS;
		string rule = VPPXESpwRules.CheckRow(rowNode, presetsFile);
		if (rule != "")
		{
			lines.Insert(Tr(rule));
		}

		if (presetsFile)
		{
			PresetChecks(file, workIdx, rowNode, lines);
		}
		else
		{
			TypeChecks(file, rowNode, lines);
		}

		string rowPath = rowNode.Name;
		string rowHolder = "";
		if (rowNode.Kind == VPPXESpwKind.TYPE)
		{
			rowHolder = rowNode.Name;
		}

		NodeChecks(rowNode, rowPath, lines, rowHolder);
		if (lines.Count() == 0)
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_OK"));
		}

		if (lines.Count() > MAX_CHECK_LINES)
		{
			int hidden = lines.Count() - MAX_CHECK_LINES + 1;
			lines.Resize(MAX_CHECK_LINES - 1);
			string morePattern = Tr("#VSTR_XMLE_SPB_C_MORE");
			lines.Insert(string.Format(morePattern, hidden));
		}

		return JoinWith(lines, "\n");
	}

	protected void TypeChecks(VPPXESpwFile file, VPPXESpwNode rowNode, array<string> lines)
	{
		if (VPPXmlText.IsValidClassName(rowNode.Name) && !IsCeType(rowNode.Name))
		{
			string notCePattern = Tr("#VSTR_XMLE_SPB_C_NOT_CE");
			lines.Insert(string.Format(notCePattern, rowNode.Name));
		}

		VPPXESpwNode damageNode = rowNode.FirstKid(VPPXESpwKind.DAMAGE);
		if (damageNode)
		{
			DamageChecks(damageNode, rowNode.Name, lines);
		}
		else if (file.FileDmgMin != "" || file.FileDmgMax != "")
		{
			string fileDmgPattern = Tr("#VSTR_XMLE_SPB_C_FILE_DAMAGE");
			lines.Insert(string.Format(fileDmgPattern, file.FileDmgMin, file.FileDmgMax));
		}

		VPPXESpwNode subNode = rowNode.FirstKid(VPPXESpwKind.SUBCOUNTER);
		if (subNode && VPPXESpwRules.NumberOf(subNode.Min, 0) > VPPXESpwRules.NumberOf(subNode.Max, 0))
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_SUB_ORDER"));
		}

		int defs = DefinitionCount(rowNode.Name);
		if (defs > 1)
		{
			string defsPattern = Tr("#VSTR_XMLE_SPB_C_MULTI_DEF");
			lines.Insert(string.Format(defsPattern, defs));
		}
	}

	protected void PresetChecks(VPPXESpwFile file, int workIdx, VPPXESpwNode rowNode, array<string> lines)
	{
		EnsureIndex();
		string key = PresetKey(rowNode.Kind, rowNode.Name);
		if (PresetOrderOf(file.Key, workIdx) < 0)
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_SHADOWED"));
		}

		if (rowNode.CountKind(VPPXESpwKind.ITEM) == 0 && rowNode.Preset == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_PRESET_EMPTY"));
		}

		int uses = m_PresetUses.Get(key);
		if (uses == 0)
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_UNUSED"));
		}
		else
		{
			string usesPattern = Tr("#VSTR_XMLE_SPB_C_USED");
			lines.Insert(string.Format(usesPattern, uses));
		}

		float chance = VPPXESpwRules.ChanceOf(rowNode.Chance, -1);
		if (rowNode.Chance == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_PRESET_NO_CHANCE"));
		}
		else if (chance == 0)
		{
			lines.Insert(Tr("#VSTR_XMLE_SPB_C_PRESET_NEVER"));
		}
	}

	// Blocks and items of a node, deep; path names the place ("M4A1 > attachments 2 > BUISOptic").
	// holderClass: the class the blocks of node belong to (the type, or the item they are nested in; "" in a preset).
	protected void NodeChecks(VPPXESpwNode node, string path, array<string> lines, string holderClass)
	{
		for (int i = 0; i < node.Kids.Count(); i++)
		{
			VPPXESpwNode kid = node.Kids[i];
			if (!kid)
			{
				continue;
			}

			if (VPPXESpwKind.IsBlock(kid.Kind))
			{
				string blockLabel = BlockLabel(node, i, kid);
				string blockPath = path + " > " + blockLabel;
				BlockChecks(kid, blockPath, lines);
				NodeChecks(kid, blockPath, lines, holderClass);
			}
			else if (kid.Kind == VPPXESpwKind.ITEM)
			{
				string itemPath = path + " > " + kid.Name;
				ItemChecks(kid, itemPath, lines);
				if (node.Kind == VPPXESpwKind.ATTACHMENTS && holderClass != "")
				{
					SlotCheck(holderClass, kid, itemPath, lines);
				}

				NodeChecks(kid, itemPath, lines, kid.Name);
			}
		}
	}

	protected void BlockChecks(VPPXESpwNode blockNode, string path, array<string> lines)
	{
		UnreachableChecks(blockNode, path, lines);
		if (blockNode.Preset != "")
		{
			VPPXESpwPresetRef found = FindPreset(blockNode.Kind, blockNode.Preset);
			if (!found)
			{
				int otherKind = VPPXESpwKind.CARGO;
				if (blockNode.Kind == VPPXESpwKind.CARGO)
				{
					otherKind = VPPXESpwKind.ATTACHMENTS;
				}

				string missingKey = "#VSTR_XMLE_SPB_C_PRESET_MISSING";
				if (FindPreset(otherKind, blockNode.Preset))
				{
					missingKey = "#VSTR_XMLE_SPB_C_PRESET_WRONG_KIND";
				}

				string missingPattern = Tr(missingKey);
				string missingText = string.Format(missingPattern, blockNode.Preset);
				lines.Insert(path + ": " + missingText);
			}
		}
		else if (blockNode.CountKind(VPPXESpwKind.ITEM) == 0)
		{
			string emptyText = Tr("#VSTR_XMLE_SPB_C_BLOCK_EMPTY");
			lines.Insert(path + ": " + emptyText);
		}

		if (blockNode.Chance != "")
		{
			float chance = VPPXESpwRules.NumberOf(blockNode.Chance, 1);
			if (chance < 0)
			{
				string negText = Tr("#VSTR_XMLE_SPB_C_CHANCE_NEG");
				lines.Insert(path + ": " + negText);
			}
			else if (chance == 0)
			{
				string zeroText = Tr("#VSTR_XMLE_SPB_C_CHANCE_ZERO");
				lines.Insert(path + ": " + zeroText);
			}
		}
	}

	protected void ItemChecks(VPPXESpwNode itemNode, string path, array<string> lines)
	{
		if (!VPPXmlText.IsValidClassName(itemNode.Name))
		{
			return;
		}

		if (itemNode.QuantMax != "" && itemNode.QuantMin == "")
		{
			string loneMaxText = Tr("#VSTR_XMLE_SPB_C_QUANT_LONE_MAX");
			lines.Insert(path + ": " + loneMaxText);
		}

		if (IsIgnoredName(itemNode.Name))
		{
			string ignoredText = Tr("#VSTR_XMLE_SPB_C_ITEM_IGNORED");
			lines.Insert(path + ": " + ignoredText);
		}
		else if (!ExistsInConfig(itemNode.Name))
		{
			string missingText = Tr("#VSTR_XMLE_SPB_C_ITEM_NO_CLASS");
			lines.Insert(path + ": " + missingText);
		}

		float quantMin = VPPXESpwRules.NumberOf(itemNode.QuantMin, -1);
		float quantMax = VPPXESpwRules.NumberOf(itemNode.QuantMax, -1);
		if ((itemNode.QuantMin != "" && (quantMin < 0 || quantMin > 100)) || (itemNode.QuantMax != "" && (quantMax < 0 || quantMax > 100)))
		{
			string rangeText = Tr("#VSTR_XMLE_SPB_C_QUANT_RANGE");
			lines.Insert(path + ": " + rangeText);
		}
		else if (itemNode.QuantMin != "" && itemNode.QuantMax != "" && quantMin > quantMax)
		{
			string orderText = Tr("#VSTR_XMLE_SPB_C_QUANT_ORDER");
			lines.Insert(path + ": " + orderText);
		}

		if (itemNode.Chance != "" && VPPXESpwRules.NumberOf(itemNode.Chance, 1) == 0)
		{
			string zeroText = Tr("#VSTR_XMLE_SPB_C_CHANCE_ZERO");
			lines.Insert(path + ": " + zeroText);
		}

		VPPXESpwNode itemDamage = itemNode.FirstKid(VPPXESpwKind.DAMAGE);
		if (itemDamage)
		{
			DamageChecks(itemDamage, path, lines);
		}
	}

	protected void DamageChecks(VPPXESpwNode damageNode, string path, array<string> lines)
	{
		float minValue = VPPXESpwRules.NumberOf(damageNode.Min, 0);
		float maxValue = VPPXESpwRules.NumberOf(damageNode.Max, 0);
		if (minValue < 0 || minValue > 1 || maxValue < 0 || maxValue > 1)
		{
			string rangeText = Tr("#VSTR_XMLE_SPB_C_DAMAGE_RANGE");
			lines.Insert(path + ": " + rangeText);
		}
		else if (minValue > maxValue)
		{
			string orderText = Tr("#VSTR_XMLE_SPB_C_DAMAGE_ORDER");
			lines.Insert(path + ": " + orderText);
		}
	}

	// Definitions of a type name in every spawnabletypes file (not deleted here).
	protected int DefinitionCount(string typeName)
	{
		string wanted = typeName;
		wanted.ToLower();
		int count = 0;
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			foreach (VPPXESpwWork work : file.Work)
			{
				string lower = work.Row.Name;
				lower.ToLower();
				if (!work.Deleted && lower == wanted)
				{
					count++;
				}
			}
		}

		return count;
	}

	// ---------------------------------------------------------------- merged view (types)

	protected void SetView(int view)
	{
		if (view != VIEW_EDIT && m_Mode != MODE_TYPES)
		{
			return;
		}

		if (view != VIEW_PREVIEW)
		{
			ReleasePreview();
		}

		m_View = view;
		CloseDropdowns();
		LoadForm();
	}

	// A type as the server builds it: every definition in load order, merged by the engine's rules.
	protected void RebuildMerged()
	{
		if (!m_MergedList)
		{
			return;
		}

		m_MergedList.ClearItems();
		VPPXESpwWork selected = SelectedWork();
		if (!selected)
		{
			return;
		}

		string wanted = selected.Row.Name;
		wanted.ToLower();
		bool seen = false;
		bool hoarder = false;
		bool unique = false;
		string flagsFrom = "";
		string damageText = "";
		string damageFrom = "";
		VPPXESpwNode cargoFrom = null;
		string cargoFile = "";
		VPPXESpwNode attFrom = null;
		string attFile = "";
		int defs = 0;
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			string fileLabel = FileLabel(fileKey);
			foreach (VPPXESpwWork work : file.Work)
			{
				string lower = work.Row.Name;
				lower.ToLower();
				if (work.Deleted || lower != wanted)
				{
					continue;
				}

				defs++;
				VPPXESpwNode rowNode = work.Row;
				bool hasHoarder = rowNode.HasKid(VPPXESpwKind.HOARDER);
				bool hasUnique = rowNode.HasKid(VPPXESpwKind.UNIQUE);
				if (!seen || hasHoarder || hasUnique)
				{
					hoarder = hasHoarder;
					unique = hasUnique;
					flagsFrom = fileLabel;
				}

				seen = true;
				VPPXESpwNode damageNode = rowNode.FirstKid(VPPXESpwKind.DAMAGE);
				if (damageNode)
				{
					damageText = damageNode.Min + " - " + damageNode.Max;
					damageFrom = fileLabel;
				}
				else if (file.FileDmgMin != "" || file.FileDmgMax != "")
				{
					damageText = file.FileDmgMin + " - " + file.FileDmgMax;
					damageFrom = fileLabel + " (root)";
				}

				if (rowNode.HasKid(VPPXESpwKind.CARGO))
				{
					cargoFrom = rowNode;
					cargoFile = fileLabel;
				}

				if (rowNode.HasKid(VPPXESpwKind.ATTACHMENTS))
				{
					attFrom = rowNode;
					attFile = fileLabel;
				}
			}
		}

		string defsLabel = Tr("#VSTR_XMLE_SPB_M_DEFS");
		string defsText = defs.ToString();
		AddMergedRow(defsLabel, defsText, "");
		string yesText = Tr("#VSTR_XMLE_SPB_M_YES");
		string noText = Tr("#VSTR_XMLE_SPB_M_NO");
		string hoarderLabel = Tr("#VSTR_XMLE_SPB_HOARDER");
		string hoarderValue = noText;
		if (hoarder)
		{
			hoarderValue = yesText;
		}

		AddMergedRow(hoarderLabel, hoarderValue, flagsFrom);
		string uniqueLabel = Tr("#VSTR_XMLE_SPB_UNIQUE");
		string uniqueValue = noText;
		if (unique)
		{
			uniqueValue = yesText;
		}

		AddMergedRow(uniqueLabel, uniqueValue, flagsFrom);
		string damageLabel = Tr("#VSTR_XMLE_SPB_DAMAGE");
		if (damageText == "")
		{
			damageText = Tr("#VSTR_XMLE_SPB_M_PRISTINE");
		}

		AddMergedRow(damageLabel, damageText, damageFrom);
		AddMergedBlocks(cargoFrom, VPPXESpwKind.CARGO, cargoFile);
		AddMergedBlocks(attFrom, VPPXESpwKind.ATTACHMENTS, attFile);
		string noteText = Tr("#VSTR_XMLE_SPB_M_NOTE");
		m_TxtMergedNote.SetText(noteText);
	}

	protected void AddMergedBlocks(VPPXESpwNode rowNode, int kind, string fileLabel)
	{
		string kindLabel = KindText(kind);
		if (!rowNode)
		{
			string noneText = Tr("#VSTR_XMLE_SPB_M_NONE");
			AddMergedRow(kindLabel, noneText, "");
			return;
		}

		int count = rowNode.CountKind(kind);
		string countPattern = Tr("#VSTR_XMLE_SPB_M_BLOCKS");
		string countText = string.Format(countPattern, count);
		AddMergedRow(kindLabel, countText, fileLabel);
		int number = 0;
		foreach (VPPXESpwNode kid : rowNode.Kids)
		{
			if (!kid || kid.Kind != kind)
			{
				continue;
			}

			number++;
			float chance = EffectiveChance(kid);
			string chanceText = VPPXESpwRules.FormatNumber(chance);
			string content = BlockContentText(kid);
			string numberLabel = "  " + number.ToString();
			string blockValue = content + "  (" + chanceText + ")";
			AddMergedRow(numberLabel, blockValue, fileLabel);
		}
	}

	protected void AddMergedRow(string what, string value, string from)
	{
		int row = m_MergedList.AddItem(what, null, 0);
		m_MergedList.SetItem(row, value, null, 1);
		m_MergedList.SetItem(row, from, null, 2);
	}

	// ---------------------------------------------------------------- TYPES tab queries (spawns with / found in)

	// Changes whenever the loaded rows or their edits change (the TYPES inspector re-renders on a new value).
	int DataRevision()
	{
		return m_DataRev;
	}

	bool IsDataLoaded()
	{
		return m_Loaded;
	}

	// Loads the files when nothing is loaded yet (the TYPES inspector asks while this tab is hidden).
	void EnsureLoaded()
	{
		if (!m_Loaded && !m_Pending)
		{
			RequestList();
		}
	}

	protected void BumpData()
	{
		m_DataRev++;
	}

	// Per type (lower case): the rows whose attachments / cargo blocks the server keeps, and how many definitions.
	protected void EnsureEffective()
	{
		if (m_EffRev == m_DataRev && m_EffAtt)
		{
			return;
		}

		m_EffRev = m_DataRev;
		m_EffAtt = new map<string, VPPXESpwNode>();
		m_EffCargo = new map<string, VPPXESpwNode>();
		m_EffDefs = new map<string, int>();
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			foreach (VPPXESpwWork work : file.Work)
			{
				if (work.Deleted || work.Row.Name == "")
				{
					continue;
				}

				string lower = work.Row.Name;
				lower.ToLower();
				int defs = m_EffDefs.Get(lower);
				m_EffDefs.Set(lower, defs + 1);
				if (work.Row.HasKid(VPPXESpwKind.ATTACHMENTS))
				{
					m_EffAtt.Set(lower, work.Row);
				}

				if (work.Row.HasKid(VPPXESpwKind.CARGO))
				{
					m_EffCargo.Set(lower, work.Row);
				}
			}
		}
	}

	// "Spawns with": every block the server keeps for the type, with its chance and the odds of each item.
	void DescribeSpawnsWith(string typeName, array<string> outLines)
	{
		outLines.Clear();
		EnsureIndex();
		EnsureEffective();
		string lower = typeName;
		lower.ToLower();
		if (!m_EffDefs.Contains(lower))
		{
			outLines.Insert(Tr("#VSTR_XMLE_SPB_T_NONE"));
			return;
		}

		int defs = m_EffDefs.Get(lower);
		if (defs > 1)
		{
			string mergedPattern = Tr("#VSTR_XMLE_SPB_T_MERGED");
			string mergedText = string.Format(mergedPattern, defs);
			outLines.Insert(mergedText);
		}

		string flagsText = MergedFlagsText(lower);
		if (flagsText != "")
		{
			outLines.Insert(flagsText);
		}

		VPPXESpwNode attSource = m_EffAtt.Get(lower);
		VPPXESpwNode cargoSource = m_EffCargo.Get(lower);
		if (!attSource && !cargoSource)
		{
			outLines.Insert(Tr("#VSTR_XMLE_SPB_T_NO_BLOCKS"));
			return;
		}

		bool onDeath = GetGame().IsKindOf(lower, "dz_lightai");
		SpawnsWithBlocks(attSource, VPPXESpwKind.ATTACHMENTS, false, outLines);
		SpawnsWithBlocks(cargoSource, VPPXESpwKind.CARGO, onDeath, outLines);
	}

	// "hoarder, unique, damage 0 - 0.3" of the merged definitions ("" when none applies).
	protected string MergedFlagsText(string lowerName)
	{
		bool seen = false;
		bool hoarder = false;
		bool unique = false;
		string damageText = "";
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			foreach (VPPXESpwWork work : file.Work)
			{
				string rowLower = work.Row.Name;
				rowLower.ToLower();
				if (work.Deleted || rowLower != lowerName)
				{
					continue;
				}

				bool hasHoarder = work.Row.HasKid(VPPXESpwKind.HOARDER);
				bool hasUnique = work.Row.HasKid(VPPXESpwKind.UNIQUE);
				if (!seen || hasHoarder || hasUnique)
				{
					hoarder = hasHoarder;
					unique = hasUnique;
				}

				seen = true;
				VPPXESpwNode damageNode = work.Row.FirstKid(VPPXESpwKind.DAMAGE);
				if (damageNode)
				{
					damageText = damageNode.Min + " - " + damageNode.Max;
				}
				else if (file.FileDmgMin != "" || file.FileDmgMax != "")
				{
					damageText = file.FileDmgMin + " - " + file.FileDmgMax;
				}
			}
		}

		array<string> parts = new array<string>();
		if (hoarder)
		{
			parts.Insert(Tr("#VSTR_XMLE_SPB_HOARDER_LOW"));
		}

		if (unique)
		{
			parts.Insert(Tr("#VSTR_XMLE_SPB_UNIQUE_LOW"));
		}

		if (damageText != "")
		{
			string damageLabel = Tr("#VSTR_XMLE_SPB_DAMAGE_LOW");
			parts.Insert(damageLabel + " " + damageText);
		}

		return JoinWith(parts, ", ");
	}

	protected void SpawnsWithBlocks(VPPXESpwNode holder, int kind, bool onDeath, array<string> outLines)
	{
		if (!holder)
		{
			return;
		}

		for (int i = 0; i < holder.Kids.Count(); i++)
		{
			VPPXESpwNode blockNode = holder.Kids[i];
			if (!blockNode || blockNode.Kind != kind)
			{
				continue;
			}

			array<VPPXESpwNode> items = new array<VPPXESpwNode>();
			ResolveBlockItems(blockNode, items, 0);
			float blockChance = EffectiveChance(blockNode);
			array<float> odds = new array<float>();
			VPPXESpwRoll.Odds(items, blockChance, odds);
			string label = BlockLabel(holder, i, blockNode);
			if (blockNode.Preset != "")
			{
				string viaPattern = Tr("#VSTR_XMLE_SPB_T_VIA");
				string viaText = string.Format(viaPattern, blockNode.Preset);
				label = label + " " + viaText;
			}

			if (onDeath)
			{
				string deathText = Tr("#VSTR_XMLE_SPB_T_DEATH");
				label = label + " " + deathText;
			}

			array<string> parts = new array<string>();
			int shown = items.Count();
			if (shown > MAX_SPAWNS_WITH_ITEMS)
			{
				shown = MAX_SPAWNS_WITH_ITEMS;
			}

			for (int j = 0; j < shown; j++)
			{
				string itemOdds = VPPXESpwRoll.Percent(odds[j]);
				string itemPart = items[j].Name + " " + itemOdds;
				parts.Insert(itemPart);
			}

			int hidden = items.Count() - shown;
			if (hidden > 0)
			{
				string hiddenPart = "+" + hidden.ToString();
				parts.Insert(hiddenPart);
			}

			if (items.Count() == 0)
			{
				parts.Insert(Tr("#VSTR_XMLE_SPB_EMPTY_BLOCK"));
			}

			float gate = Math.Clamp(blockChance, 0, 1);
			string gateText = VPPXESpwRoll.Percent(gate);
			string itemsText = JoinWith(parts, ", ");
			outLines.Insert(label + " (" + gateText + "): " + itemsText);
		}
	}

	// "Found in": every block that can roll the class, with the odds for one spawn of its type (nested blocks multiply
	// by their item's odds), then the presets that hold it.
	void DescribeFoundIn(string className, array<string> outLines)
	{
		outLines.Clear();
		EnsureIndex();
		EnsureEffective();
		string wanted = className;
		wanted.ToLower();
		m_FoundHidden = 0;
		for (int t = 0; t < m_EffDefs.Count(); t++)
		{
			string typeLower = m_EffDefs.GetKey(t);
			VPPXESpwNode attSource = m_EffAtt.Get(typeLower);
			VPPXESpwNode cargoSource = m_EffCargo.Get(typeLower);
			string typeShown = typeLower;
			if (attSource)
			{
				typeShown = attSource.Name;
			}
			else if (cargoSource)
			{
				typeShown = cargoSource.Name;
			}

			FoundInBlocks(attSource, VPPXESpwKind.ATTACHMENTS, typeShown, 1.0, wanted, outLines, 0);
			FoundInBlocks(cargoSource, VPPXESpwKind.CARGO, typeShown, 1.0, wanted, outLines, 0);
		}

		string presetPattern = Tr("#VSTR_XMLE_SPB_F_PRESET");
		for (int p = 0; p < m_PresetFirst.Count(); p++)
		{
			VPPXESpwPresetRef presetRef = m_PresetFirst.GetElement(p);
			if (!presetRef.Row)
			{
				continue;
			}

			array<VPPXESpwNode> items = new array<VPPXESpwNode>();
			ResolveBlockItems(presetRef.Row, items, 0);
			float presetChance = EffectiveChance(presetRef.Row);
			array<float> odds = new array<float>();
			VPPXESpwRoll.Odds(items, presetChance, odds);
			for (int j = 0; j < items.Count(); j++)
			{
				string itemLower = items[j].Name;
				itemLower.ToLower();
				if (itemLower != wanted)
				{
					continue;
				}

				string presetKey = m_PresetFirst.GetKey(p);
				int uses = m_PresetUses.Get(presetKey);
				string kindText = KindText(presetRef.Row.Kind);
				string oddsText = VPPXESpwRoll.Percent(odds[j]);
				string presetLine = string.Format(presetPattern, presetRef.Row.Name, kindText, oddsText, uses);
				AddFoundLine(presetLine, outLines);
			}
		}

		if (m_FoundHidden > 0)
		{
			string morePattern = Tr("#VSTR_XMLE_SPB_C_MORE");
			string moreText = string.Format(morePattern, m_FoundHidden);
			outLines.Insert(moreText);
		}

		if (outLines.Count() == 0)
		{
			outLines.Insert(Tr("#VSTR_XMLE_SPB_F_NONE"));
		}
	}

	protected void AddFoundLine(string line, array<string> outLines)
	{
		if (outLines.Count() >= MAX_FOUND_LINES)
		{
			m_FoundHidden++;
			return;
		}

		outLines.Insert(line);
	}

	protected void FoundInBlocks(VPPXESpwNode holder, int onlyKind, string path, float parentOdds, string wanted, array<string> outLines, int depth)
	{
		if (!holder || depth > MAX_ROLL_DEPTH)
		{
			return;
		}

		for (int i = 0; i < holder.Kids.Count(); i++)
		{
			VPPXESpwNode blockNode = holder.Kids[i];
			if (!blockNode || !VPPXESpwKind.IsBlock(blockNode.Kind))
			{
				continue;
			}

			if (onlyKind >= 0 && blockNode.Kind != onlyKind)
			{
				continue;
			}

			array<VPPXESpwNode> items = new array<VPPXESpwNode>();
			ResolveBlockItems(blockNode, items, 0);
			float blockChance = EffectiveChance(blockNode);
			array<float> odds = new array<float>();
			VPPXESpwRoll.Odds(items, blockChance, odds);
			string blockLabel = BlockLabel(holder, i, blockNode);
			string blockPath = path + " > " + blockLabel;
			for (int j = 0; j < items.Count(); j++)
			{
				VPPXESpwNode itemNode = items[j];
				float itemOdds = parentOdds * odds[j];
				string itemLower = itemNode.Name;
				itemLower.ToLower();
				if (itemLower == wanted)
				{
					string line = blockPath;
					if (blockNode.Kids.Find(itemNode) < 0)
					{
						string viaPattern = Tr("#VSTR_XMLE_SPB_T_VIA");
						string viaText = string.Format(viaPattern, blockNode.Preset);
						line = line + " " + viaText;
					}

					string oddsText = VPPXESpwRoll.Percent(itemOdds);
					AddFoundLine(line + ": " + oddsText, outLines);
				}

				if (itemOdds > 0 && itemNode.BlockCount() > 0)
				{
					string itemPath = blockPath + " > " + itemNode.Name;
					FoundInBlocks(itemNode, -1, itemPath, itemOdds, wanted, outLines, depth + 1);
				}
			}
		}
	}

	// TYPES inspector, OPEN IN SPAWNABLES: shows the type's last definition (the one the server merges last), or
	// offers to add it to the shown file.
	void SelectType(string typeName)
	{
		if (!m_Loaded || m_Pending)
		{
			m_WantType = typeName;
			if (!m_Pending)
			{
				RequestList();
			}

			return;
		}

		SelectTypeNow(typeName);
	}

	protected void SelectTypeNow(string typeName)
	{
		m_WantType = "";
		string wanted = typeName;
		wanted.ToLower();
		string foundKey = "";
		int foundIdx = -1;
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			for (int w = 0; w < file.Work.Count(); w++)
			{
				VPPXESpwWork work = file.Work[w];
				string lower = work.Row.Name;
				lower.ToLower();
				if (!work.Deleted && lower == wanted)
				{
					foundKey = fileKey;
					foundIdx = w;
				}
			}
		}

		ReleasePreview();
		m_Mode = MODE_TYPES;
		m_View = VIEW_EDIT;
		m_FilterIdx = FILTER_ALL;
		m_LastSearch = "";
		if (m_InputSearch)
		{
			m_InputSearch.SetText("");
		}

		if (foundKey != "")
		{
			m_TypeFileKey = foundKey;
			m_SelWork = foundIdx;
			ResetNav();
			RebuildFilterDropdown();
			RebuildAll();
			return;
		}

		m_SelWork = -1;
		ResetNav();
		RebuildFilterDropdown();
		RebuildAll();
		m_PendAddName = typeName;
		StartAdd(null);
	}

	// ---------------------------------------------------------------- bulk (the rows the list shows)

	// "Bulk (N)": the operations for the listed rows of this mode.
	protected void RebuildBulkDropdown()
	{
		if (!m_BulkDD)
		{
			return;
		}

		m_BulkDD.Close();
		m_BulkDD.RemoveAllElements();
		m_BulkOps.Clear();
		int listed = m_ListWork.Count();
		string labelPattern = Tr("#VSTR_XMLE_SPB_BULK");
		string label = string.Format(labelPattern, listed);
		m_BulkDD.SetText(label);
		AddBulkOp(BULK_SCALE, "#VSTR_XMLE_SPB_BULK_SCALE");
		AddBulkOp(BULK_REPLACE, "#VSTR_XMLE_SPB_BULK_REPLACE");
		AddBulkOp(BULK_REMOVE, "#VSTR_XMLE_SPB_BULK_REMOVE");
		if (m_Mode == MODE_TYPES)
		{
			AddBulkOp(BULK_CLEAR_DAMAGE, "#VSTR_XMLE_SPB_BULK_CLEAR_DMG");
		}
	}

	protected void AddBulkOp(int op, string key)
	{
		string text = Tr(key);
		m_BulkOps.Insert(op);
		m_BulkDD.AddElement(text);
	}

	void OnBulkPicked(int index)
	{
		m_BulkDD.Close();
		if (index < 0 || index >= m_BulkOps.Count() || !CanEdit() || m_ListWork.Count() == 0)
		{
			return;
		}

		m_PendBulkOp = m_BulkOps[index];
		// the dialog opens after the dropdown finished handling its click
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.OpenBulkDialog, 1, false);
	}

	void OpenBulkDialog()
	{
		if (!m_Owner || !CanEdit())
		{
			return;
		}

		int listed = m_ListWork.Count();
		if (m_PendBulkOp == BULK_SCALE)
		{
			string scalePattern = Tr("#VSTR_XMLE_SPB_DLG_BULK_SCALE_BODY");
			string scaleBody = string.Format(scalePattern, listed);
			m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_BULK_SCALE_TITLE", scaleBody, this, "OnBulkScale", "0.5");
			return;
		}

		string selectedItem = "";
		VPPXESpwNode itemNode = CurrentItem();
		if (itemNode)
		{
			selectedItem = itemNode.Name;
		}

		if (m_PendBulkOp == BULK_REPLACE)
		{
			string replacePattern = Tr("#VSTR_XMLE_SPB_DLG_BULK_REPLACE_BODY");
			string replaceBody = string.Format(replacePattern, listed);
			m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_BULK_REPLACE_TITLE", replaceBody, this, "OnBulkReplaceFrom", selectedItem);
			return;
		}

		if (m_PendBulkOp == BULK_REMOVE)
		{
			string removePattern = Tr("#VSTR_XMLE_SPB_DLG_BULK_REMOVE_BODY");
			string removeBody = string.Format(removePattern, listed);
			m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_BULK_REMOVE_TITLE", removeBody, this, "OnBulkRemove", selectedItem);
			return;
		}

		string damagePattern = Tr("#VSTR_XMLE_SPB_DLG_BULK_DMG_BODY");
		string damageBody = string.Format(damagePattern, listed);
		m_Owner.OpenConfirm("#VSTR_XMLE_SPB_DLG_BULK_DMG_TITLE", damageBody, DIAGTYPE.DIAG_YESNO, this, "OnBulkClearDamage", false);
	}

	void OnBulkScale(int result, string input)
	{
		if (result != DIAGRESULT.OK || !CanEdit())
		{
			return;
		}

		string trimmed = input.Trim();
		float factor = VPPXESpwRules.NumberOf(trimmed, -1);
		if (factor < 0 || factor > 10)
		{
			string badFactor = Tr("#VSTR_XMLE_SPB_ERR_FACTOR");
			m_Owner.NotifyError(badFactor);
			return;
		}

		int changed = 0;
		foreach (int workIdx : m_ListWork)
		{
			VPPXESpwWork work = ListedWork(workIdx);
			if (work && ScaleBlocksIn(work.Row, factor) > 0)
			{
				changed++;
			}
		}

		FinishBulk(changed);
	}

	protected int ScaleBlocksIn(VPPXESpwNode node, float factor)
	{
		int changed = 0;
		if (VPPXESpwKind.IsBlock(node.Kind))
		{
			float current = EffectiveChance(node);
			float scaled = Math.Clamp(current * factor, 0, 1);
			string scaledText = VPPXESpwRules.FormatNumber(scaled);
			if (node.Chance != scaledText)
			{
				node.Chance = scaledText;
				changed++;
			}
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (kid && kid.Kind != VPPXESpwKind.OTHER)
			{
				changed += ScaleBlocksIn(kid, factor);
			}
		}

		return changed;
	}

	void OnBulkReplaceFrom(int result, string input)
	{
		if (result != DIAGRESULT.OK || !CanEdit())
		{
			return;
		}

		string fromName = input.Trim();
		if (!VPPXmlText.IsValidClassName(fromName))
		{
			string invalidPattern = Tr("#VSTR_XMLE_ERR_NAME_INVALID");
			string invalidText = string.Format(invalidPattern, fromName);
			m_Owner.NotifyError(invalidText);
			return;
		}

		m_BulkFrom = fromName;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.OpenBulkWithDialog, 1, false);
	}

	void OpenBulkWithDialog()
	{
		if (!m_Owner || m_BulkFrom == "")
		{
			return;
		}

		string pattern = Tr("#VSTR_XMLE_SPB_DLG_BULK_WITH_BODY");
		string body = string.Format(pattern, m_BulkFrom);
		m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_BULK_REPLACE_TITLE", body, this, "OnBulkReplaceWith", m_BulkFrom);
	}

	void OnBulkReplaceWith(int result, string input)
	{
		string fromName = m_BulkFrom;
		m_BulkFrom = "";
		if (result != DIAGRESULT.OK || fromName == "" || !CanEdit())
		{
			return;
		}

		string toName = input.Trim();
		if (!VPPXmlText.IsValidClassName(toName))
		{
			string invalidPattern = Tr("#VSTR_XMLE_ERR_NAME_INVALID");
			string invalidText = string.Format(invalidPattern, toName);
			m_Owner.NotifyError(invalidText);
			return;
		}

		string fromLower = fromName;
		fromLower.ToLower();
		int changed = 0;
		foreach (int workIdx : m_ListWork)
		{
			VPPXESpwWork work = ListedWork(workIdx);
			if (work && ReplaceItemsIn(work.Row, fromLower, toName) > 0)
			{
				changed++;
			}
		}

		FinishBulk(changed);
	}

	protected int ReplaceItemsIn(VPPXESpwNode node, string fromLower, string toName)
	{
		int changed = 0;
		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (!kid || kid.Kind == VPPXESpwKind.OTHER)
			{
				continue;
			}

			if (kid.Kind == VPPXESpwKind.ITEM)
			{
				string kidLower = kid.Name;
				kidLower.ToLower();
				if (kidLower == fromLower)
				{
					kid.Name = toName;
					changed++;
				}
			}

			changed += ReplaceItemsIn(kid, fromLower, toName);
		}

		return changed;
	}

	void OnBulkRemove(int result, string input)
	{
		if (result != DIAGRESULT.OK || !CanEdit())
		{
			return;
		}

		string removeName = input.Trim();
		if (!VPPXmlText.IsValidClassName(removeName))
		{
			string invalidPattern = Tr("#VSTR_XMLE_ERR_NAME_INVALID");
			string invalidText = string.Format(invalidPattern, removeName);
			m_Owner.NotifyError(invalidText);
			return;
		}

		string removeLower = removeName;
		removeLower.ToLower();
		int changed = 0;
		foreach (int workIdx : m_ListWork)
		{
			VPPXESpwWork work = ListedWork(workIdx);
			if (work && RemoveItemsIn(work.Row, removeLower) > 0)
			{
				changed++;
			}
		}

		ResetNav();
		FinishBulk(changed);
	}

	// Removes the items of that class under node; a block this leaves without items and without a preset goes too.
	protected int RemoveItemsIn(VPPXESpwNode node, string removeLower)
	{
		int removed = 0;
		for (int i = node.Kids.Count() - 1; i >= 0; i--)
		{
			VPPXESpwNode kid = node.Kids[i];
			if (!kid || kid.Kind == VPPXESpwKind.OTHER)
			{
				continue;
			}

			if (kid.Kind == VPPXESpwKind.ITEM)
			{
				string kidLower = kid.Name;
				kidLower.ToLower();
				if (kidLower == removeLower)
				{
					node.Kids.RemoveOrdered(i);
					removed++;
					continue;
				}
			}

			int kidRemoved = RemoveItemsIn(kid, removeLower);
			removed += kidRemoved;
			if (kidRemoved > 0 && VPPXESpwKind.IsBlock(kid.Kind) && kid.Preset == "" && !kid.HasKid(VPPXESpwKind.ITEM))
			{
				node.Kids.RemoveOrdered(i);
			}
		}

		return removed;
	}

	void OnBulkClearDamage(int result, string input)
	{
		if (result != DIAGRESULT.YES || !CanEdit() || m_Mode != MODE_TYPES)
		{
			return;
		}

		int changed = 0;
		foreach (int workIdx : m_ListWork)
		{
			VPPXESpwWork work = ListedWork(workIdx);
			if (!work)
			{
				continue;
			}

			VPPXESpwNode damageNode = work.Row.FirstKid(VPPXESpwKind.DAMAGE);
			if (damageNode)
			{
				work.Row.Kids.RemoveItem(damageNode);
				changed++;
			}
		}

		FinishBulk(changed);
	}

	// A listed row of the shown file (null when gone or deleted).
	protected VPPXESpwWork ListedWork(int workIdx)
	{
		VPPXESpwFile file = CurrentFile();
		if (!file || workIdx < 0 || workIdx >= file.Work.Count())
		{
			return null;
		}

		VPPXESpwWork work = file.Work[workIdx];
		if (work.Deleted)
		{
			return null;
		}

		return work;
	}

	protected void FinishBulk(int changed)
	{
		int listed = m_ListWork.Count();
		m_IndexStale = true;
		BumpData();
		RebuildList();
		LoadForm();
		UpdateActionBar();
		RebuildFileDropdown();
		string pattern = Tr("#VSTR_XMLE_SPB_BULK_DONE");
		string done = string.Format(pattern, changed, listed);
		m_Owner.Notify(done);
	}

	// ---------------------------------------------------------------- roll model (preview, odds)

	// The row whose blocks of that kind the server keeps for a type: the last definition (load order, with the edits)
	// that has any block of the kind; null when none has.
	protected VPPXESpwNode LastDefinitionWith(string typeName, int kind)
	{
		string wanted = typeName;
		wanted.ToLower();
		VPPXESpwNode last = null;
		foreach (string fileKey : m_TypeFileKeys)
		{
			VPPXESpwFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			foreach (VPPXESpwWork work : file.Work)
			{
				string lower = work.Row.Name;
				lower.ToLower();
				if (!work.Deleted && lower == wanted && work.Row.HasKid(kind))
				{
					last = work.Row;
				}
			}
		}

		return last;
	}

	// The items a block rolls over, in the server's order: its preset's items first (the first preset of that name
	// and kind), then its own. Items in cfgignorelist and nameless ones are left out, as the server drops them.
	protected void ResolveBlockItems(VPPXESpwNode blockNode, array<VPPXESpwNode> outItems, int depth)
	{
		if (blockNode.Preset != "" && depth < 4)
		{
			VPPXESpwPresetRef found = FindPreset(blockNode.Kind, blockNode.Preset);
			if (found && found.Row)
			{
				ResolveBlockItems(found.Row, outItems, depth + 1);
			}
		}

		foreach (VPPXESpwNode kid : blockNode.Kids)
		{
			if (!kid || kid.Kind != VPPXESpwKind.ITEM)
			{
				continue;
			}

			if (VPPXmlText.IsValidClassName(kid.Name) && !IsIgnoredName(kid.Name))
			{
				outItems.Insert(kid);
			}
		}
	}

	// "45%": how likely the shown block spawns this item ("" when the server does not keep the item).
	protected string OddsTextOf(VPPXESpwNode itemNode)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode)
		{
			return "";
		}

		array<VPPXESpwNode> items = new array<VPPXESpwNode>;
		ResolveBlockItems(blockNode, items, 0);
		int itemIdx = items.Find(itemNode);
		if (itemIdx < 0)
		{
			return "";
		}

		float blockChance = EffectiveChance(blockNode);
		array<float> odds = new array<float>;
		VPPXESpwRoll.Odds(items, blockChance, odds);
		float itemOdds = odds[itemIdx];
		return VPPXESpwRoll.Percent(itemOdds);
	}

	// Own items of a block the one-item roll can never reach: the items before them already add up to 1.
	protected void UnreachableChecks(VPPXESpwNode blockNode, string path, array<string> lines)
	{
		float blockChance = EffectiveChance(blockNode);
		if (blockChance <= 0)
		{
			return;
		}

		array<VPPXESpwNode> items = new array<VPPXESpwNode>;
		ResolveBlockItems(blockNode, items, 0);
		if (items.Count() < 2)
		{
			return;
		}

		array<float> odds = new array<float>;
		VPPXESpwRoll.Odds(items, blockChance, odds);
		string pattern = Tr("#VSTR_XMLE_SPB_C_UNREACHABLE");
		for (int i = 0; i < items.Count(); i++)
		{
			VPPXESpwNode itemNode = items[i];
			float itemChance = VPPXESpwRoll.ItemChanceOf(itemNode);
			if (odds[i] > 0 || itemChance <= 0 || blockNode.Kids.Find(itemNode) < 0)
			{
				continue;
			}

			string unreachable = string.Format(pattern, itemNode.Name);
			lines.Insert(path + ": " + unreachable);
		}
	}

	// ---------------------------------------------------------------- attachment slots

	// The class whose slots the shown block fills: the type or item holding it, when it is an attachments block.
	protected string SlotHolderClass()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || blockNode.Kind != VPPXESpwKind.ATTACHMENTS)
		{
			return "";
		}

		if (!VPPXmlText.IsValidClassName(m_Holder.Name))
		{
			return "";
		}

		return m_Holder.Name;
	}

	// "Add by slot": the holder's attachment slots (and its magazines for a weapon) with how many classes fit each.
	void RebuildSlotDropdown()
	{
		if (!m_SlotDD || !m_SlotHost)
		{
			return;
		}

		m_SlotDD.Close();
		m_SlotDD.RemoveAllElements();
		m_SlotChoices.Clear();
		m_SlotLevel = 0;
		m_SlotPicked = "";
		m_SlotHolder = SlotHolderClass();
		string addText = Tr("#VSTR_XMLE_SPB_SLOT_ADD");
		m_SlotDD.SetText(addText);
		m_SlotHost.Show(m_SlotHolder != "");
		if (m_SlotHolder == "")
		{
			return;
		}

		array<string> magazines = new array<string>();
		VPPXESlotIndex.WeaponMagazines(m_SlotHolder, magazines);
		if (magazines.Count() > 0)
		{
			string magazinesLabel = Tr("#VSTR_XMLE_SPB_SLOT_MAGAZINES");
			int magazineCount = magazines.Count();
			m_SlotChoices.Insert(MAGAZINE_SLOT);
			string magazinesEntry = magazinesLabel + " (" + magazineCount.ToString() + ")";
			m_SlotDD.AddElement(magazinesEntry);
		}

		array<string> slots = new array<string>();
		VPPXESlotIndex.HolderSlots(m_SlotHolder, slots);
		array<string> fitting = new array<string>();
		foreach (string slot : slots)
		{
			VPPXESlotIndex.ClassesFor(slot, fitting);
			int fitCount = fitting.Count();
			m_SlotChoices.Insert(slot);
			string slotEntry = slot + " (" + fitCount.ToString() + ")";
			m_SlotDD.AddElement(slotEntry);
		}
	}

	// Level 0: a slot was picked, its classes follow; level 1: "back" or a class to add.
	void OnSlotPicked(int index)
	{
		m_SlotDD.Close();
		if (m_SlotLevel == 0)
		{
			if (index < 0 || index >= m_SlotChoices.Count())
			{
				return;
			}

			m_SlotPicked = m_SlotChoices[index];
			// rebuilt after the dropdown finished handling this click (its element is still in use)
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.ShowSlotClasses, 1, false);
			return;
		}

		if (index > 0 && index <= m_SlotChoices.Count())
		{
			string className = m_SlotChoices[index - 1];
			if (className != "")
			{
				AddItemNamed(className);
			}

		}

		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.RebuildSlotDropdown, 1, false);
	}

	void ShowSlotClasses()
	{
		if (!m_SlotDD || m_SlotPicked == "")
		{
			return;
		}

		m_SlotDD.RemoveAllElements();
		m_SlotChoices.Clear();
		m_SlotLevel = 1;
		string backText = Tr("#VSTR_XMLE_SPB_SLOT_BACK");
		m_SlotDD.AddElement(backText);
		array<string> names = new array<string>();
		string title = m_SlotPicked;
		if (m_SlotPicked == MAGAZINE_SLOT)
		{
			title = Tr("#VSTR_XMLE_SPB_SLOT_MAGAZINES");
			array<string> magazines = new array<string>();
			VPPXESlotIndex.WeaponMagazines(m_SlotHolder, magazines);
			foreach (string magazine : magazines)
			{
				string magazineShown = VPPXEConfigClasses.DisplayOf(magazine);
				names.Insert(magazineShown);
			}
		}
		else
		{
			VPPXESlotIndex.ClassesFor(m_SlotPicked, names);
		}

		// the dropdown prefab lays out 50 rows at most (dropdown_content 'Rows 50'): back, 48 classes, "+N more"
		int shownCount = names.Count();
		if (shownCount > MAX_SLOT_CLASSES)
		{
			shownCount = MAX_SLOT_CLASSES;
		}

		for (int i = 0; i < shownCount; i++)
		{
			string shownName = names[i];
			m_SlotChoices.Insert(shownName);
			m_SlotDD.AddElement(shownName);
		}

		int moreCount = names.Count() - shownCount;
		if (moreCount > 0)
		{
			string morePattern = Tr("#VSTR_XMLE_SPB_SLOT_MORE");
			string moreText = string.Format(morePattern, moreCount);
			m_SlotChoices.Insert("");
			m_SlotDD.AddElement(moreText);
		}

		m_SlotDD.SetText(title);
		m_SlotDD.Toggle();
	}

	// A picked class: into the selected item when it has no name yet, else a new item at the end of the block.
	protected void AddItemNamed(string className)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || !CanEdit())
		{
			return;
		}

		VPPXESpwNode itemNode = CurrentItem();
		if (!itemNode || itemNode.Name != "")
		{
			itemNode = new VPPXESpwNode();
			itemNode.Kind = VPPXESpwKind.ITEM;
			blockNode.Kids.Insert(itemNode);
			m_ItemIdx = blockNode.Kids.Count() - 1;
		}

		itemNode.Name = className;
		LoadForm();
		AfterEdit(true);
	}

	// An attachments block item that fits none of the holder's slots: the server looks for any free place, so it
	// ends up in the cargo or nowhere.
	protected void SlotCheck(string holderClass, VPPXESpwNode itemNode, string path, array<string> lines)
	{
		if (!VPPXmlText.IsValidClassName(itemNode.Name) || IsIgnoredName(itemNode.Name) || !ExistsInConfig(itemNode.Name))
		{
			return;
		}

		if (VPPXESlotIndex.Fits(holderClass, itemNode.Name) != 0)
		{
			return;
		}

		string pattern = Tr("#VSTR_XMLE_SPB_C_NO_SLOT");
		string noSlot = string.Format(pattern, holderClass);
		lines.Insert(path + ": " + noSlot);
	}

	// ---------------------------------------------------------------- preset tools

	// The randompresets file new presets go to: the one shown in PRESETS mode (the first one by default).
	protected VPPXESpwFile PresetTargetFile()
	{
		VPPXESpwFile file = m_Files.Get(m_PresetFileKey);
		if (!file && m_PresetFileKeys.Count() > 0)
		{
			string firstKey = m_PresetFileKeys[0];
			file = m_Files.Get(firstKey);
		}

		return file;
	}

	// EXTRACT: the block's own items become a new preset of its kind (with the block's chance); the block refers to it.
	protected void StartExtract()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!m_Holder || !blockNode || !CanEdit() || !blockNode.HasKid(VPPXESpwKind.ITEM))
		{
			return;
		}

		if (blockNode.Preset != "")
		{
			string hasPreset = Tr("#VSTR_XMLE_SPB_ERR_EXTRACT_HAS_PRESET");
			m_Owner.NotifyError(hasPreset);
			return;
		}

		VPPXESpwFile target = PresetTargetFile();
		if (!target || !target.Editable)
		{
			string noFile = Tr("#VSTR_XMLE_SPB_ERR_NO_PRESET_FILE");
			m_Owner.NotifyError(noFile);
			return;
		}

		m_PendExtract = true;
		string targetLabel = FileLabel(target.Key);
		string pattern = Tr("#VSTR_XMLE_SPB_DLG_EXTRACT_BODY");
		string body = string.Format(pattern, targetLabel);
		string kindTag = VPPXESpwKind.TagOf(blockNode.Kind);
		string suggested = m_Holder.Name + "_" + kindTag;
		m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_EXTRACT_TITLE", body, this, "OnExtractName", suggested);
	}

	void OnExtractName(int result, string input)
	{
		bool pending = m_PendExtract;
		m_PendExtract = false;
		VPPXESpwNode blockNode = CurrentBlock();
		VPPXESpwFile target = PresetTargetFile();
		if (result != DIAGRESULT.OK || !pending || !blockNode || !m_Holder || !CanEdit() || !target || !target.Editable)
		{
			return;
		}

		string presetName = input.Trim();
		if (!VPPXESpwRules.IsPlainName(presetName))
		{
			string badName = Tr("#VSTR_XMLE_SPB_ERR_PRESET_NAME");
			m_Owner.NotifyError(badName);
			return;
		}

		if (FindPreset(blockNode.Kind, presetName))
		{
			string takenPattern = Tr("#VSTR_XMLE_SPB_ERR_PRESET_TAKEN");
			string taken = string.Format(takenPattern, presetName);
			m_Owner.NotifyError(taken);
			return;
		}

		VPPXESpwWork work = new VPPXESpwWork();
		work.Row.Kind = blockNode.Kind;
		work.Row.Name = presetName;
		work.Row.Chance = blockNode.Chance;
		for (int i = blockNode.Kids.Count() - 1; i >= 0; i--)
		{
			VPPXESpwNode kid = blockNode.Kids[i];
			if (!kid || kid.Kind != VPPXESpwKind.ITEM)
			{
				continue;
			}

			VPPXESpwNode moved = new VPPXESpwNode();
			moved.CopyFrom(kid);
			AsNew(moved);
			work.Row.Kids.InsertAt(moved, 0);
			blockNode.Kids.RemoveOrdered(i);
		}

		blockNode.Preset = presetName;
		blockNode.Chance = "";
		target.Work.Insert(work);
		m_IndexStale = true;
		m_ItemIdx = -1;
		LoadForm();
		AfterEdit(true);
		RebuildFileDropdown();
		string targetLabel = FileLabel(target.Key);
		string donePattern = Tr("#VSTR_XMLE_SPB_EXTRACTED");
		string done = string.Format(donePattern, presetName, targetLabel);
		m_Owner.Notify(done);
	}

	// INLINE: the preset's items (the first preset of that name and kind, its own preset first) copied to the front
	// of the block, its chance kept on the block, the reference removed. The server rolls the same list afterwards.
	protected void InlinePreset()
	{
		VPPXESpwNode blockNode = CurrentBlock();
		if (!blockNode || blockNode.Preset == "" || !CanEdit())
		{
			return;
		}

		VPPXESpwPresetRef found = FindPreset(blockNode.Kind, blockNode.Preset);
		if (!found || !found.Row)
		{
			string missingPattern = Tr("#VSTR_XMLE_SPB_ERR_INLINE_MISSING");
			string missing = string.Format(missingPattern, blockNode.Preset);
			m_Owner.NotifyError(missing);
			return;
		}

		if (blockNode.Chance == "")
		{
			float inherited = EffectiveChance(blockNode);
			if (inherited != 1)
			{
				blockNode.Chance = VPPXESpwRules.FormatNumber(inherited);
			}
		}

		array<VPPXESpwNode> presetItems = new array<VPPXESpwNode>();
		ResolveBlockItems(found.Row, presetItems, 0);
		for (int i = presetItems.Count() - 1; i >= 0; i--)
		{
			VPPXESpwNode copied = new VPPXESpwNode();
			copied.CopyFrom(presetItems[i]);
			AsNew(copied);
			blockNode.Kids.InsertAt(copied, 0);
		}

		string presetName = blockNode.Preset;
		blockNode.Preset = "";
		m_IndexStale = true;
		m_ItemIdx = FirstItemIdx(blockNode);
		LoadForm();
		AfterEdit(true);
		string donePattern = Tr("#VSTR_XMLE_SPB_INLINED");
		string done = string.Format(donePattern, presetName);
		m_Owner.Notify(done);
	}

	// RENAME (PRESETS): the preset and, when it is the definition the server uses, every block that refers to it in
	// every loaded file (each changed file is saved on its own).
	protected void StartRenamePreset()
	{
		VPPXESpwWork work = SelectedWork();
		if (!work || !VPPXESpwKind.IsBlock(work.Row.Kind) || !CanEdit())
		{
			return;
		}

		EnsureIndex();
		string key = PresetKey(work.Row.Kind, work.Row.Name);
		int uses = m_PresetUses.Get(key);
		m_PendRename = true;
		string pattern = Tr("#VSTR_XMLE_SPB_DLG_RENAME_BODY");
		string body = string.Format(pattern, uses);
		m_Owner.OpenConfirmInput("#VSTR_XMLE_SPB_DLG_RENAME_TITLE", body, this, "OnRenamePreset", work.Row.Name);
	}

	void OnRenamePreset(int result, string input)
	{
		bool pending = m_PendRename;
		m_PendRename = false;
		VPPXESpwWork work = SelectedWork();
		VPPXESpwFile file = CurrentFile();
		if (result != DIAGRESULT.OK || !pending || !work || !file || !VPPXESpwKind.IsBlock(work.Row.Kind) || !CanEdit())
		{
			return;
		}

		string newName = input.Trim();
		string oldName = work.Row.Name;
		if (newName == oldName)
		{
			return;
		}

		if (!VPPXESpwRules.IsPlainName(newName))
		{
			string badName = Tr("#VSTR_XMLE_SPB_ERR_PRESET_NAME");
			m_Owner.NotifyError(badName);
			return;
		}

		string oldLower = oldName;
		oldLower.ToLower();
		string newLower = newName;
		newLower.ToLower();
		VPPXESpwPresetRef clash = FindPreset(work.Row.Kind, newName);
		if (clash && newLower != oldLower)
		{
			string takenPattern = Tr("#VSTR_XMLE_SPB_ERR_PRESET_TAKEN");
			string taken = string.Format(takenPattern, newName);
			m_Owner.NotifyError(taken);
			return;
		}

		// the blocks follow only the definition the server uses (a shadowed copy has no users)
		bool isFirst = PresetOrderOf(file.Key, m_SelWork) >= 0;
		int refs = 0;
		int filesTouched = 0;
		if (isFirst)
		{
			string readOnly = ReadOnlyFileWithRefs(work.Row.Kind, oldLower);
			if (readOnly != "")
			{
				string roPattern = Tr("#VSTR_XMLE_SPB_ERR_RENAME_RO");
				string roText = string.Format(roPattern, readOnly);
				m_Owner.NotifyError(roText);
				return;
			}

			for (int f = 0; f < m_Files.Count(); f++)
			{
				VPPXESpwFile refFile = m_Files.GetElement(f);
				int fileRefs = 0;
				foreach (VPPXESpwWork refWork : refFile.Work)
				{
					if (!refWork.Deleted)
					{
						fileRefs += RenameRefsIn(refWork.Row, work.Row.Kind, oldLower, newName);
					}
				}

				if (fileRefs > 0)
				{
					refs += fileRefs;
					filesTouched++;
				}
			}
		}

		work.Row.Name = newName;
		m_IndexStale = true;
		RebuildList();
		LoadForm();
		AfterEdit(true);
		RebuildFileDropdown();
		string donePattern = Tr("#VSTR_XMLE_SPB_RENAMED");
		string done = string.Format(donePattern, refs, filesTouched);
		m_Owner.Notify(done);
	}

	// The label of a loaded file that refers to the preset but cannot be edited ("" when there is none).
	protected string ReadOnlyFileWithRefs(int kind, string oldLower)
	{
		for (int f = 0; f < m_Files.Count(); f++)
		{
			VPPXESpwFile refFile = m_Files.GetElement(f);
			if (refFile.Editable)
			{
				continue;
			}

			foreach (VPPXESpwWork refWork : refFile.Work)
			{
				if (!refWork.Deleted && CountRefsIn(refWork.Row, kind, oldLower) > 0)
				{
					return FileLabel(refFile.Key);
				}
			}
		}

		return "";
	}

	protected int CountRefsIn(VPPXESpwNode node, int kind, string oldLower)
	{
		int found = 0;
		if (node.Kind == kind && node.Preset != "")
		{
			string presetLower = node.Preset;
			presetLower.ToLower();
			if (presetLower == oldLower)
			{
				found++;
			}
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (kid && kid.Kind != VPPXESpwKind.OTHER)
			{
				found += CountRefsIn(kid, kind, oldLower);
			}
		}

		return found;
	}

	// Renames every block of that kind under node that refers to the old name; returns how many.
	protected int RenameRefsIn(VPPXESpwNode node, int kind, string oldLower, string newName)
	{
		int renamed = 0;
		if (node.Kind == kind && node.Preset != "")
		{
			string presetLower = node.Preset;
			presetLower.ToLower();
			if (presetLower == oldLower)
			{
				node.Preset = newName;
				renamed++;
			}
		}

		foreach (VPPXESpwNode kid : node.Kids)
		{
			if (kid && kid.Kind != VPPXESpwKind.OTHER)
			{
				renamed += RenameRefsIn(kid, kind, oldLower, newName);
			}
		}

		return renamed;
	}

	// Labels of the other loaded files with unsaved changes ("" when none): extract and rename change several.
	protected string OtherDirtyFiles()
	{
		string shownKey = CurrentFileKey();
		array<string> labels = new array<string>();
		for (int f = 0; f < m_Files.Count(); f++)
		{
			string fileKey = m_Files.GetKey(f);
			VPPXESpwFile otherFile = m_Files.GetElement(f);
			if (fileKey != shownKey && otherFile.DirtyCount() > 0)
			{
				string label = FileLabel(fileKey);
				labels.Insert(label);
			}
		}

		return JoinWith(labels, ", ");
	}

	// ---------------------------------------------------------------- preview

	// Deletes the local preview objects, children first (they were created after their parent).
	void ReleasePreview()
	{
		if (m_Preview)
		{
			m_Preview.SetItem(null);
			m_Preview.Show(false);
		}

		if (m_PreviewEntities)
		{
			for (int i = m_PreviewEntities.Count() - 1; i >= 0; i--)
			{
				EntityAI previewEntity = m_PreviewEntities[i];
				if (previewEntity && GetGame())
				{
					GetGame().ObjectDelete(previewEntity);
				}
			}

			m_PreviewEntities.Clear();
		}

		m_PreviewType = "";
		m_ShowingTest = false;
	}

	// A new simulated roll of the selected type with the edits: its attachments blocks, then its cargo blocks, each
	// following equip and nested blocks like the server. The list shows every pick; the 3D view what fits locally.
	protected void RollPreview()
	{
		ReleasePreview();
		if (m_RollList)
		{
			m_RollList.ClearItems();
		}

		VPPXESpwWork work = SelectedWork();
		if (!work || work.Row.Kind != VPPXESpwKind.TYPE || !m_Preview)
		{
			return;
		}

		string typeName = work.Row.Name;
		m_PreviewType = typeName;
		m_RollCount = 0;
		string lowerType = typeName;
		lowerType.ToLower();
		m_PreviewIsAi = GetGame().IsKindOf(lowerType, "dz_lightai");
		EntityAI rootEntity = CreatePreviewEntity(typeName);
		if (rootEntity)
		{
			m_PreviewEntities.Insert(rootEntity);
		}

		RollTypeInto(typeName, rootEntity, typeName, 0);
		string titlePattern = Tr("#VSTR_XMLE_SPB_ROLL_TITLE");
		string title = string.Format(titlePattern, m_RollCount);
		m_TxtRollTitle.SetText(title);
		if (m_RollCount == 0)
		{
			string noneText = Tr("#VSTR_XMLE_SPB_ROLL_NONE");
			m_RollList.AddItem(noneText, null, 0);
		}

		string note = Tr("#VSTR_XMLE_SPB_PREVIEW_NOTE");
		if (m_PreviewIsAi)
		{
			string aiNote = Tr("#VSTR_XMLE_SPB_PREVIEW_AI");
			note = aiNote + " " + note;
		}
		else if (!rootEntity)
		{
			string noModel = Tr("#VSTR_XMLE_SPB_PREVIEW_NO_MODEL");
			note = noModel + " " + note;
		}

		m_TxtPreviewNote.SetText(note);
		bool canTest = m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_SPAWNABLES) && !m_TestPending;
		m_BtnTestSpawn.Enable(canTest);
		if (!rootEntity)
		{
			return;
		}

		m_Preview.SetItem(rootEntity);
		m_Preview.SetModelPosition(Vector(0, 0, 0.5));
		m_Preview.SetModelOrientation(Vector(0, 0, 0));
		int viewIndex = rootEntity.GetViewIndex();
		m_Preview.SetView(viewIndex);
		m_Preview.Show(true);
		m_PreviewOrientation = Vector(0, 0, 0);
	}

	// A local (client only) object of the class; null for a class without config, AI or a non-entity.
	protected EntityAI CreatePreviewEntity(string className)
	{
		if (!ExistsInConfig(className))
		{
			return null;
		}

		string lower = className;
		lower.ToLower();
		if (GetGame().IsKindOf(lower, "dz_lightai"))
		{
			return null;
		}

		Object created = GetGame().CreateObject(className, vector.Zero, true, false, false);
		EntityAI createdEntity = EntityAI.Cast(created);
		if (!createdEntity && created)
		{
			GetGame().ObjectDelete(created);
		}

		return createdEntity;
	}

	// A type's own entry as the server keeps it (the last definition with blocks of each kind).
	protected void RollTypeInto(string typeName, EntityAI holderEntity, string holderLabel, int depth)
	{
		if (depth > MAX_ROLL_DEPTH)
		{
			return;
		}

		VPPXESpwNode attSource = LastDefinitionWith(typeName, VPPXESpwKind.ATTACHMENTS);
		VPPXESpwNode cargoSource = LastDefinitionWith(typeName, VPPXESpwKind.CARGO);
		RollBlocksOf(attSource, VPPXESpwKind.ATTACHMENTS, holderEntity, holderLabel, depth);
		RollBlocksOf(cargoSource, VPPXESpwKind.CARGO, holderEntity, holderLabel, depth);
	}

	// Every block of that kind under the holder node (a type row or an item): one roll each.
	protected void RollBlocksOf(VPPXESpwNode holderNode, int kind, EntityAI holderEntity, string holderLabel, int depth)
	{
		if (!holderNode || depth > MAX_ROLL_DEPTH)
		{
			return;
		}

		foreach (VPPXESpwNode blockNode : holderNode.Kids)
		{
			if (!blockNode || blockNode.Kind != kind)
			{
				continue;
			}

			array<VPPXESpwNode> items = new array<VPPXESpwNode>;
			ResolveBlockItems(blockNode, items, 0);
			float blockChance = EffectiveChance(blockNode);
			int picked = VPPXESpwRoll.Pick(items, blockChance);
			if (picked < 0)
			{
				continue;
			}

			array<float> odds = new array<float>;
			VPPXESpwRoll.Odds(items, blockChance, odds);
			VPPXESpwNode pickedItem = items[picked];
			float pickedOdds = odds[picked];
			SpawnRolled(pickedItem, kind, holderEntity, holderLabel, pickedOdds, depth);
		}
	}

	// One picked item: placed like the server places it (attachments block: any free slot or cargo; cargo block: cargo
	// only), listed with where it went, its quantity roll and its odds; then its equip entry and nested blocks.
	protected void SpawnRolled(VPPXESpwNode itemNode, int kind, EntityAI holderEntity, string holderLabel, float odds, int depth)
	{
		if (m_RollCount >= MAX_ROLL_ITEMS)
		{
			return;
		}

		m_RollCount++;
		EntityAI child = null;
		bool placed = false;
		// a magazine goes onto a weapon through the weapon's own logic on the server; locally it is only listed
		bool weaponMagazine = false;
		bool hasClass = ExistsInConfig(itemNode.Name);
		if (holderEntity && hasClass)
		{
			child = CreatePreviewEntity(itemNode.Name);
		}

		if (child)
		{
			if (kind == VPPXESpwKind.ATTACHMENTS)
			{
				FindInventoryLocationType anyPlace = FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO;
				placed = holderEntity.LocalTakeEntityToTargetInventory(holderEntity, anyPlace, child);
			}
			else
			{
				placed = holderEntity.LocalTakeEntityToTargetCargo(holderEntity, child);
			}

			if (!placed && Magazine.Cast(child) && Weapon_Base.Cast(holderEntity))
			{
				weaponMagazine = true;
			}

			if (placed)
			{
				m_PreviewEntities.Insert(child);
			}
			else
			{
				GetGame().ObjectDelete(child);
				child = null;
			}
		}

		string whereKey = "#VSTR_XMLE_SPB_W_CARGO";
		if (kind == VPPXESpwKind.ATTACHMENTS)
		{
			whereKey = "#VSTR_XMLE_SPB_W_ATT";
		}

		// an infected's / animal's own cargo: the CE rolls it when the creature dies
		bool onDeath = depth == 0 && kind == VPPXESpwKind.CARGO && m_PreviewIsAi;
		if (!hasClass)
		{
			whereKey = "#VSTR_XMLE_SPB_W_NOCLASS";
		}
		else if (onDeath)
		{
			whereKey = "#VSTR_XMLE_SPB_W_ON_DEATH";
		}
		else if (weaponMagazine)
		{
			whereKey = "#VSTR_XMLE_SPB_W_HIDDEN";
		}
		else if (holderEntity && !placed)
		{
			whereKey = "#VSTR_XMLE_SPB_W_NOROOM";
		}

		string wherePattern = Tr(whereKey);
		string whereText = string.Format(wherePattern, holderLabel);
		string quantText = "";
		float minPct;
		float maxPct;
		if (VPPXESpwRoll.QuantRange(itemNode, minPct, maxPct))
		{
			float rolledPct = Math.RandomFloatInclusive(minPct, maxPct);
			float rolledFraction = rolledPct / 100;
			quantText = VPPXESpwRoll.Percent(rolledFraction);
		}

		string oddsText = VPPXESpwRoll.Percent(odds);
		string indent = "";
		for (int level = 0; level < depth; level++)
		{
			indent = indent + "  ";
		}

		string nameText = indent + itemNode.Name;
		int row = m_RollList.AddItem(nameText, null, 0);
		m_RollList.SetItem(row, whereText, null, 1);
		m_RollList.SetItem(row, quantText, null, 2);
		m_RollList.SetItem(row, oddsText, null, 3);
		if (!hasClass || (holderEntity && !placed && !weaponMagazine))
		{
			for (int column = 0; column < 4; column++)
			{
				m_RollList.SetItemColor(row, column, ARGB(255, 194, 69, 69));
			}

			// the server stops here too: nothing to equip or fill
			return;
		}

		if (IsOnText(itemNode.Equip))
		{
			RollTypeInto(itemNode.Name, child, itemNode.Name, depth + 1);
		}

		RollBlocksOf(itemNode, VPPXESpwKind.ATTACHMENTS, child, itemNode.Name, depth + 1);
		RollBlocksOf(itemNode, VPPXESpwKind.CARGO, child, itemNode.Name, depth + 1);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (w && w == m_Preview && m_PreviewEntities.Count() > 0)
		{
			GetGame().GetDragQueue().Call(this, "UpdatePreviewRotation");
			GetMousePos(m_RotationX, m_RotationY);
			return true;
		}

		return false;
	}

	void UpdatePreviewRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		if (!m_Preview)
		{
			return;
		}

		vector orientation = m_PreviewOrientation;
		orientation[0] = orientation[0] + (m_RotationY - mouse_y);
		orientation[1] = orientation[1] - (m_RotationX - mouse_x);
		m_Preview.SetModelOrientation(orientation);
		if (!is_dragging)
		{
			m_PreviewOrientation = orientation;
		}
	}

	// ---------------------------------------------------------------- test spawn (server)

	protected void StartTestSpawn()
	{
		VPPXESpwWork work = SelectedWork();
		if (!work || work.Row.Kind != VPPXESpwKind.TYPE || m_TestPending || !m_Owner)
		{
			return;
		}

		m_PendTestName = work.Row.Name;
		string pattern = Tr("#VSTR_XMLE_SPB_DLG_TEST_BODY");
		string body = string.Format(pattern, m_PendTestName);
		m_Owner.OpenConfirm("#VSTR_XMLE_SPB_DLG_TEST_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmTestSpawn", false);
	}

	void OnConfirmTestSpawn(int result, string input)
	{
		if (result != DIAGRESULT.YES || m_PendTestName == "" || m_TestPending || !m_Owner)
		{
			return;
		}

		m_TestReqId = m_Owner.NextReqId();
		m_TestPending = true;
		m_TestSentAt = NowMs();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_TestSpawnable", new Param2<int, string>(m_TestReqId, m_PendTestName), true, null);
		string sentText = Tr("#VSTR_XMLE_SPB_TEST_SENT");
		SetStatus(sentText, false);
		m_BtnTestSpawn.Enable(false);
	}

	void XE_OnSpawnableTest(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXESpwTestResult> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESpwTestResult result = data.param1;
		if (!result || result.ReqId != m_TestReqId || !m_TestPending)
		{
			return;
		}

		m_TestPending = false;
		ShowTestResult(result);
	}

	// The real spawn in the roll list (RE-ROLL goes back to the simulation).
	protected void ShowTestResult(VPPXESpwTestResult result)
	{
		int count = result.Names.Count();
		string doneKey = "#VSTR_XMLE_SPB_TEST_DONE";
		if (result.CargoOnDeath)
		{
			doneKey = "#VSTR_XMLE_SPB_TEST_DONE_AI";
		}

		string donePattern = Tr(doneKey);
		string doneText = string.Format(donePattern, result.TypeName, count);
		m_Owner.Notify(doneText);
		SetStatus(doneText, false);
		if (m_BtnTestSpawn)
		{
			m_BtnTestSpawn.Enable(true);
		}

		if (m_View != VIEW_PREVIEW || !m_RollList)
		{
			return;
		}

		m_ShowingTest = true;
		m_RollList.ClearItems();
		string titlePattern = Tr("#VSTR_XMLE_SPB_TEST_TITLE");
		string title = string.Format(titlePattern, count);
		m_TxtRollTitle.SetText(title);
		string attPattern = Tr("#VSTR_XMLE_SPB_W_ATT");
		string cargoPattern = Tr("#VSTR_XMLE_SPB_W_CARGO");
		string otherPattern = Tr("#VSTR_XMLE_SPB_W_OTHER");
		for (int i = 0; i < count; i++)
		{
			string parentName = result.Parents[i];
			int place = result.Places[i];
			string whereText = string.Format(otherPattern, parentName);
			if (place == VPPXESpwTestResult.PLACE_ATTACHMENT)
			{
				whereText = string.Format(attPattern, parentName);
			}
			else if (place == VPPXESpwTestResult.PLACE_CARGO)
			{
				whereText = string.Format(cargoPattern, parentName);
			}

			string childName = result.Names[i];
			string detail = result.Details[i];
			int row = m_RollList.AddItem(childName, null, 0);
			m_RollList.SetItem(row, whereText, null, 1);
			m_RollList.SetItem(row, detail, null, 2);
			m_RollList.SetItem(row, "", null, 3);
		}

		if (count == 0)
		{
			string noneText = Tr("#VSTR_XMLE_SPB_ROLL_NONE");
			m_RollList.AddItem(noneText, null, 0);
		}
	}

	// ---------------------------------------------------------------- details

	// Title, chips, enable states, hints and the checks of the shown row, block and item.
	protected void UpdateDetails()
	{
		UpdateViewChips();
		VPPXESpwWork work = SelectedWork();
		VPPXESpwFile file = CurrentFile();
		if (!work || !file)
		{
			return;
		}

		VPPXESpwNode rowNode = work.Row;
		bool editable = CanEdit();
		bool isType = rowNode.Kind == VPPXESpwKind.TYPE;
		string title = rowNode.Name;
		if (work.OrigIndex < 0)
		{
			string newPattern = Tr("#VSTR_XMLE_SPB_EDIT_NEW");
			title = string.Format(newPattern, rowNode.Name);
		}

		m_TxtEditTitle.SetText(title);
		string lineText = "";
		if (work.Orig && work.OrigIndex >= 0)
		{
			string linePattern = Tr("#VSTR_XMLE_MSG_LINE");
			lineText = string.Format(linePattern, work.Orig.Line);
		}

		m_TxtLine.SetText(lineText);
		SetInputEnabled(IN_NAME, m_LblName, editable);
		bool hoarderOn = rowNode.HasKid(VPPXESpwKind.HOARDER);
		bool uniqueOn = rowNode.HasKid(VPPXESpwKind.UNIQUE);
		SetChipLook(m_BtnHoarder, m_FillHoarder, m_TxtHoarder, hoarderOn, editable, false);
		SetChipLook(m_BtnUnique, m_FillUnique, m_TxtUnique, uniqueOn, editable, false);
		bool hasDamage = rowNode.HasKid(VPPXESpwKind.DAMAGE);
		SetChipLook(m_BtnDamage, m_FillDamage, m_TxtDamage, hasDamage, editable, false);
		SetInputEnabled(IN_DMGMIN, null, editable && hasDamage);
		SetInputEnabled(IN_DMGMAX, null, editable && hasDamage);
		string dmgHint = Tr("#VSTR_XMLE_SPB_DMG_HINT");
		m_TxtDmgHint.SetText(dmgHint);
		SetChipLook(m_BtnAdvanced, m_FillAdvanced, m_TxtAdvanced, m_ShowAdvanced, true, false);
		SetInputEnabled(IN_BIND, m_LblBind, editable);
		SetInputEnabled(IN_SUBMIN, m_LblSub, editable);
		SetInputEnabled(IN_SUBMAX, null, editable);
		SetInputEnabled(IN_BATCH, m_LblBatch, editable);
		SetChipLook(m_BtnKindCargo, m_FillKindCargo, m_TxtKindCargo, rowNode.Kind == VPPXESpwKind.CARGO, editable, false);
		SetChipLook(m_BtnKindAtt, m_FillKindAtt, m_TxtKindAtt, rowNode.Kind == VPPXESpwKind.ATTACHMENTS, editable, false);
		SetInputEnabled(IN_ROWCHANCE, m_LblRowChance, editable);
		UpdateBlockDetails(editable, isType);
		UpdateItemDetails(editable);
		string checks = BuildChecks(file, m_SelWork, work);
		m_TxtChecks.SetText(checks);
		if (m_View == VIEW_MERGED)
		{
			RebuildMerged();
		}

		if (m_Shown && m_View == VIEW_PREVIEW && isType && m_PreviewType != rowNode.Name)
		{
			RollPreview();
		}
	}

	protected void UpdateBlockDetails(bool editable, bool isType)
	{
		VPPXESpwNode blockNode = CurrentBlock();
		bool hasBlock = blockNode != null;
		bool canHold = m_Holder != null;
		m_BtnBack.Enable(m_Stack.Count() > 0);
		string path = PathText();
		m_TxtPath.SetText(path);
		string blocksPattern = Tr("#VSTR_XMLE_SPB_BLOCKS_TITLE");
		int blockCount = 0;
		if (m_Holder)
		{
			blockCount = m_Holder.BlockCount();
		}

		string blocksText = string.Format(blocksPattern, blockCount);
		m_TxtBlocksTitle.SetText(blocksText);
		m_BtnBlockAddCargo.Enable(editable && canHold);
		m_BtnBlockAddAtt.Enable(editable && canHold);
		m_BtnBlockDup.Enable(editable && canHold && hasBlock);
		m_BtnBlockUp.Enable(editable && canHold && hasBlock);
		m_BtnBlockDown.Enable(editable && canHold && hasBlock);
		m_BtnBlockRemove.Enable(editable && canHold && hasBlock);
		bool blockIsCargo = hasBlock && blockNode.Kind == VPPXESpwKind.CARGO;
		bool blockIsAtt = hasBlock && blockNode.Kind == VPPXESpwKind.ATTACHMENTS;
		SetChipLook(m_BtnBlkCargo, m_FillBlkCargo, m_TxtBlkCargo, blockIsCargo, editable && canHold && hasBlock, false);
		SetChipLook(m_BtnBlkAtt, m_FillBlkAtt, m_TxtBlkAtt, blockIsAtt, editable && canHold && hasBlock, false);
		SetInputEnabled(IN_BLKCHANCE, m_LblBlkChance, editable && hasBlock);
		SetInputEnabled(IN_BLKPRESET, m_LblBlkPreset, editable && hasBlock);
		bool canExtract = editable && canHold && hasBlock && blockNode.Preset == "" && blockNode.HasKid(VPPXESpwKind.ITEM);
		m_BtnBlockExtract.Enable(canExtract);
		bool canInline = editable && hasBlock && blockNode.Preset != "";
		m_BtnBlockInline.Enable(canInline);
		bool presetRow = !isType && editable;
		m_BtnRenameRefs.Enable(presetRow);
		string itemsPattern = Tr("#VSTR_XMLE_SPB_ITEMS_TITLE");
		int itemCount = 0;
		if (blockNode)
		{
			itemCount = blockNode.CountKind(VPPXESpwKind.ITEM);
		}

		string itemsText = string.Format(itemsPattern, itemCount);
		m_TxtItemsTitle.SetText(itemsText);
		m_BtnItemAdd.Enable(editable && hasBlock);
	}

	protected void UpdateItemDetails(bool editable)
	{
		VPPXESpwNode itemNode = CurrentItem();
		bool hasItem = itemNode != null;
		m_BtnItemUp.Enable(editable && hasItem);
		m_BtnItemDown.Enable(editable && hasItem);
		m_BtnItemRemove.Enable(editable && hasItem);
		SetInputEnabled(IN_ITEMNAME, m_LblItemName, editable && hasItem);
		SetInputEnabled(IN_ITEMCHANCE, m_LblItemChance, editable && hasItem);
		SetInputEnabled(IN_QMIN, m_LblItemQuant, editable && hasItem);
		SetInputEnabled(IN_QMAX, null, editable && hasItem);
		bool equipOn = hasItem && IsOnText(itemNode.Equip);
		SetChipLook(m_BtnItemEquip, m_FillItemEquip, m_TxtItemEquip, equipOn, editable && hasItem, false);
		bool itemDamageOn = hasItem && itemNode.HasKid(VPPXESpwKind.DAMAGE);
		SetChipLook(m_BtnItemDamage, m_FillItemDamage, m_TxtItemDamage, itemDamageOn, editable && hasItem, false);
		SetInputEnabled(IN_IDMGMIN, null, editable && itemDamageOn);
		SetInputEnabled(IN_IDMGMAX, null, editable && itemDamageOn);
		int nested = 0;
		if (hasItem)
		{
			nested = itemNode.BlockCount();
		}

		string openPattern = Tr("#VSTR_XMLE_SPB_BTN_NESTED");
		string openText = string.Format(openPattern, nested);
		m_TxtItemOpen.SetText(openText);
		bool canNest = CanNestDeeper();
		m_BtnItemOpen.Enable(hasItem && canNest);
		string hint = "";
		string completeText = "";
		if (hasItem && m_Suggestions.Count() > 0)
		{
			completeText = m_Suggestions[0];
			if (m_Suggestions.Count() > 1)
			{
				hint = JoinWith(m_Suggestions, ", ");
			}
		}
		else if (hasItem && VPPXmlText.IsValidClassName(itemNode.Name) && !ExistsInConfig(itemNode.Name))
		{
			hint = Tr("#VSTR_XMLE_SPB_HINT_NO_CLASS");
		}

		m_TxtItemComplete.SetText(completeText);
		m_BtnItemComplete.Show(completeText != "");
		m_BtnItemComplete.Enable(editable && completeText != "");
		m_TxtItemHint.SetText(hint);
	}

	protected void UpdateViewChips()
	{
		bool typesMode = m_Mode == MODE_TYPES;
		SetChipLook(m_BtnViewEdit, m_FillViewEdit, m_TxtViewEdit, m_View == VIEW_EDIT, true, false);
		SetChipLook(m_BtnViewMerged, m_FillViewMerged, m_TxtViewMerged, m_View == VIEW_MERGED, typesMode, false);
		m_BtnViewMerged.Show(typesMode);
		SetChipLook(m_BtnViewPreview, m_FillViewPreview, m_TxtViewPreview, m_View == VIEW_PREVIEW, typesMode, false);
		m_BtnViewPreview.Show(typesMode);
	}

	protected void UpdateModeChips()
	{
		SetChipLook(m_BtnModeTypes, m_FillModeTypes, m_TxtModeTypes, m_Mode == MODE_TYPES, true, false);
		SetChipLook(m_BtnModePresets, m_FillModePresets, m_TxtModePresets, m_Mode == MODE_PRESETS, true, false);
	}

	protected void SetChipLook(ButtonWidget button, Widget fill, TextWidget label, bool isOn, bool enabled, bool danger)
	{
		int fillColor = ARGB(255, 27, 30, 34);
		int textColor = ARGB(255, 154, 160, 166);
		if (isOn)
		{
			if (danger)
			{
				fillColor = ARGB(255, 92, 34, 34);
				textColor = ARGB(255, 240, 120, 120);
			}
			else
			{
				fillColor = ARGB(255, 74, 54, 22);
				textColor = ARGB(255, 232, 163, 61);
			}
		}

		if (!enabled)
		{
			textColor = ARGB(255, 92, 97, 102);
		}

		if (fill)
		{
			fill.SetColor(fillColor);
		}

		if (label)
		{
			label.SetColor(textColor);
		}

		if (button)
		{
			button.Enable(enabled);
		}
	}

	protected void SetInputEnabled(int idx, TextWidget label, bool enabled)
	{
		EditBoxWidget inputBox = m_Inputs[idx];
		if (inputBox)
		{
			inputBox.Enable(enabled);
			if (enabled)
			{
				inputBox.SetColor(ARGB(255, 255, 255, 255));
			}
			else
			{
				inputBox.SetColor(ARGB(140, 255, 255, 255));
			}
		}

		if (label)
		{
			if (enabled)
			{
				label.SetColor(ARGB(255, 204, 208, 212));
			}
			else
			{
				label.SetColor(ARGB(255, 92, 97, 102));
			}
		}
	}

	// EDIT form or MERGED list for the selected row; the empty text when there is none.
	protected void ShowEditor(bool show)
	{
		bool typesMode = m_Mode == MODE_TYPES;
		if (m_Form)
		{
			m_Form.Show(show && (m_View == VIEW_EDIT || !typesMode));
		}

		if (m_MergedView)
		{
			m_MergedView.Show(show && m_View == VIEW_MERGED && typesMode);
		}

		if (m_PreviewView)
		{
			m_PreviewView.Show(show && m_View == VIEW_PREVIEW && typesMode);
		}

		if (!show)
		{
			ReleasePreview();
		}

		if (m_TxtEmpty)
		{
			m_TxtEmpty.Show(!show);
		}

		if (!show && m_TxtEditTitle)
		{
			string noneTitle = Tr("#VSTR_XMLE_SPB_EDIT_NONE");
			m_TxtEditTitle.SetText(noneTitle);
			m_TxtLine.SetText("");
		}

		UpdateViewChips();
		if (show)
		{
			LayoutRight();
		}
	}

	protected void UpdateEmptyText()
	{
		if (!m_TxtEmpty)
		{
			return;
		}

		VPPXESpwFile file = CurrentFile();
		string emptyText = Tr("#VSTR_XMLE_SPB_EMPTY");
		if (ModeFileKeys().Count() == 0 && m_Loaded)
		{
			emptyText = Tr("#VSTR_XMLE_SPB_NO_FILE");
		}
		else if (file && file.ErrorKey != "")
		{
			emptyText = Tr(file.ErrorKey);
		}

		m_TxtEmpty.SetText(emptyText);
	}

	// Status line: request state, read-only reasons, server-side changes, else the unsaved count.
	protected void UpdateActionBar()
	{
		VPPXESpwFile file = CurrentFile();
		int dirty = 0;
		if (file)
		{
			dirty = file.DirtyCount();
		}

		bool editable = CanEdit();
		m_BtnAdd.Enable(editable);
		bool hasSelection = SelectedWork() != null;
		m_BtnDup.Enable(editable && hasSelection);
		m_BtnDelete.Enable(editable && hasSelection);
		m_BtnSave.Enable(editable && dirty > 0);
		bool revertable = dirty > 0;
		if (file && file.ServerChanged)
		{
			revertable = true;
		}

		m_BtnRevert.Enable(revertable && !m_Saving);
		string text = m_StatusText;
		int color = ARGB(255, 154, 160, 166);
		if (m_StatusError)
		{
			color = ARGB(255, 194, 69, 69);
		}

		if (text == "")
		{
			if (file && file.ServerChanged)
			{
				text = Tr("#VSTR_XMLE_MSG_SERVER_CHANGED");
				color = ARGB(255, 217, 178, 61);
			}
			else if (m_Owner && !m_Owner.HasPerm(VPPXEPerm.EDIT_SPAWNABLES))
			{
				text = Tr("#VSTR_XMLE_SPB_READONLY");
			}
			else if (file && !file.Editable)
			{
				text = Tr("#VSTR_XMLE_MSG_FILE_READONLY");
			}
			else if (dirty > 0)
			{
				string dirtyPattern = Tr("#VSTR_XMLE_DIRTY_FMT");
				text = string.Format(dirtyPattern, dirty);
				color = ARGB(255, 232, 163, 61);
			}
			else
			{
				text = Tr("#VSTR_XMLE_DIRTY_NONE");
			}

			string others = OtherDirtyFiles();
			if (others != "")
			{
				string othersPattern = Tr("#VSTR_XMLE_SPB_OTHER_DIRTY");
				string othersText = string.Format(othersPattern, others);
				text = text + "  " + othersText;
				color = ARGB(255, 232, 163, 61);
			}
		}

		m_TxtStatus.SetText(text);
		m_TxtStatus.SetColor(color);
	}

	protected void SetStatus(string text, bool isError)
	{
		m_StatusText = text;
		m_StatusError = isError;
		UpdateActionBar();
	}

	// ---------------------------------------------------------------- layout

	protected void UpdateUnit()
	{
		m_Unit = 1.0;
		if (!m_ListHeader)
		{
			return;
		}

		float headerW;
		float headerH;
		m_ListHeader.GetScreenSize(headerW, headerH);
		if (headerH > 1)
		{
			m_Unit = headerH / HEADER_H;
		}
	}

	protected void CheckRootResize()
	{
		if (!m_Root)
		{
			return;
		}

		float rootW;
		float rootH;
		m_Root.GetScreenSize(rootW, rootH);
		if (rootW != m_LastRootW || rootH != m_LastRootH)
		{
			OnResize();
		}
	}

	// Left: header (title + file dropdown), mode chips, search and filter, the list, footer buttons.
	protected void LayoutLeft()
	{
		if (m_LeftW < 60 || m_LeftH < 100)
		{
			return;
		}

		float hostW = Math.Clamp(m_LeftW * 0.55, 150, 320);
		m_FileHost.SetPos(m_LeftW - hostW - 4, 2);
		m_FileHost.SetSize(hostW, 24);
		float chipW = (m_LeftW - 24) / 3;
		m_BtnModeTypes.SetPos(6, 34);
		m_BtnModeTypes.SetSize(chipW, 26);
		m_BtnModePresets.SetPos(12 + chipW, 34);
		m_BtnModePresets.SetSize(chipW, 26);
		m_BulkHost.SetPos(18 + 2 * chipW, 35);
		m_BulkHost.SetSize(chipW, 24);
		float filterW = Math.Clamp(m_LeftW * 0.38, 120, 190);
		m_SearchPanel.SetPos(6, 66);
		m_SearchPanel.SetSize(m_LeftW - filterW - 18, 24);
		m_FilterHost.SetPos(m_LeftW - filterW - 6, 66);
		m_FilterHost.SetSize(filterW, 24);
		float footerY = m_LeftH - FOOTER_H;
		float listY = 96;
		float listH = footerY - listY - 4;
		if (listH < 40)
		{
			listH = 40;
		}

		m_List.SetPos(6, listY);
		m_List.SetSize(m_LeftW - 12, listH);
		float buttonW = (m_LeftW - 12 - 12) / 3;
		m_BtnAdd.SetPos(6, footerY + 3);
		m_BtnAdd.SetSize(buttonW, 30);
		m_BtnDup.SetPos(6 + buttonW + 6, footerY + 3);
		m_BtnDup.SetSize(buttonW, 30);
		m_BtnDelete.SetPos(6 + 2 * (buttonW + 6), footerY + 3);
		m_BtnDelete.SetSize(buttonW, 30);
	}

	// Right: the row settings, the path, the blocks (list + buttons + block editor), the items (list + buttons + item
	// editor) and the checks; lists share the room left; the action bar at the bottom.
	protected void LayoutRight()
	{
		if (m_RightW < 200 || m_RightH < 200)
		{
			return;
		}

		float innerW = m_RightW - 20;
		float formH = m_RightH - 32 - BAR_H - 4;
		m_Form.SetPos(0, 32);
		m_Form.SetSize(m_RightW, formH);
		m_MergedView.SetPos(0, 32);
		m_MergedView.SetSize(m_RightW, formH);
		m_MergedList.SetPos(10, 0);
		m_MergedList.SetSize(innerW, formH - 70);
		m_TxtMergedNote.SetPos(10, formH - 66);
		m_TxtMergedNote.SetSize(innerW, 62);
		m_TxtEmpty.SetPos(20, 60);
		m_TxtEmpty.SetSize(m_RightW - 40, 80);
		LayoutHeaderChips();
		LayoutPreview(innerW, formH);
		m_ActionBar.SetPos(0, m_RightH - BAR_H);
		m_ActionBar.SetSize(m_RightW, BAR_H);
		float y = LayoutRowSettings(innerW);
		bool presetTop = m_Holder == null;
		float fixedBelow = 24 + 22 + 30 + 34 + 22 + 30 + 18 + 30 + 18 + 30 + 32 + 70;
		if (presetTop)
		{
			fixedBelow = fixedBelow - 22 - 30 - 34;
		}

		float listsRoom = formH - y - fixedBelow;
		if (listsRoom < 120)
		{
			listsRoom = 120;
		}

		float blockListH = listsRoom * 0.42;
		float itemListH = listsRoom - blockListH;
		if (presetTop)
		{
			blockListH = 0;
			itemListH = listsRoom;
		}

		y = LayoutBlocks(innerW, y, blockListH, presetTop);
		y = LayoutItems(innerW, y, itemListH);
		float checksH = formH - y - 4;
		if (checksH < 40)
		{
			checksH = 40;
		}

		m_TxtChecks.SetPos(10, y);
		m_TxtChecks.SetSize(innerW, checksH);
	}

	// EDIT / MERGED / PREVIEW at the right end of the header, left of the line number.
	protected void LayoutHeaderChips()
	{
		float chipX = m_RightW - 176 - 110;
		m_BtnViewPreview.SetPos(chipX, 3);
		m_BtnViewPreview.SetSize(110, 22);
		chipX = chipX - 116;
		m_BtnViewMerged.SetPos(chipX, 3);
		m_BtnViewMerged.SetSize(110, 22);
		chipX = chipX - 96;
		m_BtnViewEdit.SetPos(chipX, 3);
		m_BtnViewEdit.SetSize(90, 22);
	}

	// The 3D card on the left, the roll list and its buttons on the right, the note below.
	protected void LayoutPreview(float innerW, float formH)
	{
		m_PreviewView.SetPos(0, 32);
		m_PreviewView.SetSize(m_RightW, formH);
		float cardW = Math.Clamp(innerW * 0.5, 200, 560);
		float cardH = formH - 96;
		if (cardH < 120)
		{
			cardH = 120;
		}

		m_PreviewCard.SetPos(10, 0);
		m_PreviewCard.SetSize(cardW, cardH);
		float listX = 20 + cardW;
		float listW = innerW - cardW - 10;
		m_TxtRollTitle.SetPos(listX, 0);
		m_TxtRollTitle.SetSize(listW, 20);
		m_RollList.SetPos(listX, 22);
		m_RollList.SetSize(listW, cardH - 60);
		float buttonW = (listW - 6) * 0.5;
		m_BtnReroll.SetPos(listX, cardH - 32);
		m_BtnReroll.SetSize(buttonW, 30);
		m_BtnTestSpawn.SetPos(listX + buttonW + 6, cardH - 32);
		m_BtnTestSpawn.SetSize(buttonW, 30);
		m_TxtPreviewNote.SetPos(10, cardH + 6);
		m_TxtPreviewNote.SetSize(innerW, formH - cardH - 10);
	}

	// Name and flags (types) or namespace and chance (presets), damage and advanced; returns the next y.
	protected float LayoutRowSettings(float innerW)
	{
		bool isType = m_Mode == MODE_TYPES;
		m_LblName.SetPos(10, 0);
		m_LblName.SetSize(200, 18);
		float nameW = Math.Clamp(innerW * 0.4, 140, 300);
		PlaceInput(IN_NAME, 10, 18, nameW);
		float x = 10 + nameW + 6;
		m_BtnHoarder.Show(isType);
		m_BtnUnique.Show(isType);
		m_BtnKindCargo.Show(!isType);
		m_BtnKindAtt.Show(!isType);
		m_LblRowChance.Show(!isType);
		ShowInput(IN_ROWCHANCE, !isType);
		m_BtnRenameRefs.Show(!isType);
		if (!isType)
		{
			m_BtnRenameRefs.SetPos(x, 18);
			m_BtnRenameRefs.SetSize(28, 28);
			x = x + 34;
		}

		if (isType)
		{
			PlaceChip(m_BtnHoarder, x, 18, 96);
			PlaceChip(m_BtnUnique, x + 102, 18, 96);
		}
		else
		{
			PlaceChip(m_BtnKindCargo, x, 18, 110);
			PlaceChip(m_BtnKindAtt, x + 116, 18, 110);
			m_LblRowChance.SetPos(x + 232, 0);
			m_LblRowChance.SetSize(70, 18);
			PlaceInput(IN_ROWCHANCE, x + 232, 18, 64);
		}

		float y = 52;
		m_BtnDamage.Show(isType);
		ShowInput(IN_DMGMIN, isType);
		ShowInput(IN_DMGMAX, isType);
		m_TxtDmgHint.Show(isType);
		m_BtnAdvanced.Show(isType);
		bool advanced = isType && m_ShowAdvanced;
		m_LblBind.Show(advanced);
		m_LblSub.Show(advanced);
		m_LblBatch.Show(advanced);
		ShowInput(IN_BIND, advanced);
		ShowInput(IN_SUBMIN, advanced);
		ShowInput(IN_SUBMAX, advanced);
		ShowInput(IN_BATCH, advanced);
		if (!isType)
		{
			return y;
		}

		PlaceChip(m_BtnDamage, 10, y, 100);
		PlaceInput(IN_DMGMIN, 116, y, 60);
		PlaceInput(IN_DMGMAX, 182, y, 60);
		m_TxtDmgHint.SetPos(248, y);
		m_TxtDmgHint.SetSize(Math.Max(innerW - 248 - 110, 60), ROW_H);
		PlaceChip(m_BtnAdvanced, innerW + 10 - 104, y, 104);
		y = y + 34;
		if (!advanced)
		{
			return y;
		}

		m_LblBind.SetPos(10, y);
		m_LblBind.SetSize(120, 18);
		PlaceInput(IN_BIND, 10, y + 18, 120);
		m_LblSub.SetPos(140, y);
		m_LblSub.SetSize(140, 18);
		PlaceInput(IN_SUBMIN, 140, y + 18, 56);
		PlaceInput(IN_SUBMAX, 202, y + 18, 56);
		m_LblBatch.SetPos(270, y);
		m_LblBatch.SetSize(120, 18);
		PlaceInput(IN_BATCH, 270, y + 18, 56);
		return y + 52;
	}

	// Path, blocks list with its buttons, block editor; returns the next y.
	protected float LayoutBlocks(float innerW, float y, float listH, bool presetTop)
	{
		m_BtnBack.SetPos(10, y);
		m_BtnBack.SetSize(24, 24);
		m_TxtPath.SetPos(40, y);
		m_TxtPath.SetSize(innerW - 30, 24);
		y = y + 26;
		m_TxtBlocksTitle.Show(!presetTop);
		m_BlockList.Show(!presetTop);
		m_BtnBlockAddCargo.Show(!presetTop);
		m_BtnBlockAddAtt.Show(!presetTop);
		m_BtnBlockDup.Show(!presetTop);
		m_BtnBlockUp.Show(!presetTop);
		m_BtnBlockDown.Show(!presetTop);
		m_BtnBlockRemove.Show(!presetTop);
		m_BtnBlkCargo.Show(!presetTop);
		m_BtnBlkAtt.Show(!presetTop);
		m_LblBlkChance.Show(!presetTop);
		m_LblBlkPreset.Show(!presetTop);
		ShowInput(IN_BLKCHANCE, !presetTop);
		ShowInput(IN_BLKPRESET, !presetTop);
		m_PresetHost.Show(true);
		if (presetTop)
		{
			// the preset's own preset reference (rare) is still editable here
			m_LblBlkPreset.Show(true);
			ShowInput(IN_BLKPRESET, true);
			m_LblBlkPreset.SetPos(10, y);
			m_LblBlkPreset.SetSize(160, 18);
			PlaceInput(IN_BLKPRESET, 10, y + 18, 180);
			m_PresetHost.SetPos(196, y + 20);
			m_PresetHost.SetSize(180, 24);
			m_BtnBlockExtract.Show(false);
			m_BtnBlockInline.Show(true);
			m_BtnBlockInline.SetPos(382, y + 18);
			m_BtnBlockInline.SetSize(28, 28);
			return y + 52;
		}

		m_TxtBlocksTitle.SetPos(10, y);
		m_TxtBlocksTitle.SetSize(innerW, 20);
		y = y + 22;
		float buttonsW = 3 * 30;
		float listW = innerW - buttonsW - 6;
		m_BlockList.SetPos(10, y);
		m_BlockList.SetSize(listW, listH);
		float bx = 10 + listW + 6;
		array<ButtonWidget> blockButtons = new array<ButtonWidget>;
		blockButtons.Insert(m_BtnBlockAddCargo);
		blockButtons.Insert(m_BtnBlockAddAtt);
		blockButtons.Insert(m_BtnBlockDup);
		blockButtons.Insert(m_BtnBlockUp);
		blockButtons.Insert(m_BtnBlockDown);
		blockButtons.Insert(m_BtnBlockRemove);
		for (int i = 0; i < blockButtons.Count(); i++)
		{
			ButtonWidget blockButton = blockButtons[i];
			int col = i % 3;
			int rowIdx = i / 3;
			blockButton.SetPos(bx + col * 30, y + rowIdx * 30);
			blockButton.SetSize(28, 28);
		}

		y = y + listH + 4;
		PlaceChip(m_BtnBlkCargo, 10, y + 18, 104);
		PlaceChip(m_BtnBlkAtt, 120, y + 18, 104);
		m_LblBlkChance.SetPos(230, y);
		m_LblBlkChance.SetSize(70, 18);
		PlaceInput(IN_BLKCHANCE, 230, y + 18, 64);
		m_LblBlkPreset.SetPos(300, y);
		m_LblBlkPreset.SetSize(160, 18);
		float presetW = Math.Clamp(innerW - 300 - 6 - 170 - 64, 90, 220);
		PlaceInput(IN_BLKPRESET, 300, y + 18, presetW);
		m_PresetHost.SetPos(300 + presetW + 6, y + 20);
		m_PresetHost.SetSize(170, 24);
		float toolX = 300 + presetW + 6 + 170 + 6;
		m_BtnBlockExtract.Show(true);
		m_BtnBlockInline.Show(true);
		m_BtnBlockExtract.SetPos(toolX, y + 18);
		m_BtnBlockExtract.SetSize(28, 28);
		m_BtnBlockInline.SetPos(toolX + 30, y + 18);
		m_BtnBlockInline.SetSize(28, 28);
		return y + 52;
	}

	// Items list with its buttons, item editor (name + completion, chance / quantity / equip / damage, nested).
	protected float LayoutItems(float innerW, float y, float listH)
	{
		float slotW = Math.Clamp(innerW * 0.4, 160, 260);
		m_TxtItemsTitle.SetPos(10, y);
		m_TxtItemsTitle.SetSize(innerW - slotW - 10, 20);
		m_SlotHost.SetPos(innerW + 10 - slotW, y - 3);
		m_SlotHost.SetSize(slotW, 24);
		y = y + 22;
		float listW = innerW - 36;
		m_ItemList.SetPos(10, y);
		m_ItemList.SetSize(listW, listH);
		float bx = 10 + listW + 6;
		array<ButtonWidget> itemButtons = new array<ButtonWidget>;
		itemButtons.Insert(m_BtnItemAdd);
		itemButtons.Insert(m_BtnItemUp);
		itemButtons.Insert(m_BtnItemDown);
		itemButtons.Insert(m_BtnItemRemove);
		for (int i = 0; i < itemButtons.Count(); i++)
		{
			ButtonWidget itemButton = itemButtons[i];
			itemButton.SetPos(bx, y + i * 30);
			itemButton.SetSize(28, 28);
		}

		y = y + listH + 4;
		m_LblItemName.SetPos(10, y);
		m_LblItemName.SetSize(200, 18);
		float nameW = Math.Clamp(innerW * 0.38, 140, 260);
		PlaceInput(IN_ITEMNAME, 10, y + 18, nameW);
		m_BtnItemComplete.SetPos(16 + nameW, y + 18);
		m_BtnItemComplete.SetSize(150, ROW_H);
		m_TxtItemHint.SetPos(172 + nameW, y + 18);
		m_TxtItemHint.SetSize(Math.Max(innerW - 172 - nameW, 40), ROW_H);
		y = y + 50;
		m_LblItemChance.SetPos(10, y);
		m_LblItemChance.SetSize(70, 18);
		PlaceInput(IN_ITEMCHANCE, 10, y + 18, 60);
		m_LblItemQuant.SetPos(76, y);
		m_LblItemQuant.SetSize(140, 18);
		PlaceInput(IN_QMIN, 76, y + 18, 56);
		PlaceInput(IN_QMAX, 138, y + 18, 56);
		PlaceChip(m_BtnItemEquip, 200, y + 18, 80);
		PlaceChip(m_BtnItemDamage, 286, y + 18, 90);
		PlaceInput(IN_IDMGMIN, 382, y + 18, 56);
		PlaceInput(IN_IDMGMAX, 444, y + 18, 56);
		y = y + 50;
		m_BtnItemOpen.SetPos(10, y);
		m_BtnItemOpen.SetSize(Math.Clamp(innerW * 0.5, 200, 320), ROW_H);
		return y + 34;
	}

	protected void PlaceInput(int idx, float x, float y, float w)
	{
		EditBoxWidget box = m_Inputs[idx];
		if (!box)
		{
			return;
		}

		box.SetPos(x, y);
		box.SetSize(w, ROW_H);
	}

	protected void ShowInput(int idx, bool show)
	{
		EditBoxWidget box = m_Inputs[idx];
		if (box)
		{
			box.Show(show);
		}
	}

	protected void PlaceChip(ButtonWidget chipButton, float x, float y, float w)
	{
		if (!chipButton)
		{
			return;
		}

		chipButton.SetPos(x, y);
		chipButton.SetSize(w, ROW_H);
	}

	// ---------------------------------------------------------------- helpers

	protected bool IsOnText(string text)
	{
		return text == "1" || text == "true";
	}

	protected string JoinWith(array<string> parts, string separator)
	{
		string text = "";
		foreach (string part : parts)
		{
			if (text != "")
			{
				text = text + separator;
			}

			text = text + part;
		}

		return text;
	}

	protected string CurrentFileKey()
	{
		if (m_Mode == MODE_PRESETS)
		{
			return m_PresetFileKey;
		}

		return m_TypeFileKey;
	}

	protected VPPXESpwFile CurrentFile()
	{
		string fileKey = CurrentFileKey();
		if (fileKey == "")
		{
			return null;
		}

		return m_Files.Get(fileKey);
	}

	protected VPPXESpwWork SelectedWork()
	{
		VPPXESpwFile file = CurrentFile();
		if (!file || m_SelWork < 0 || m_SelWork >= file.Work.Count())
		{
			return null;
		}

		VPPXESpwWork work = file.Work[m_SelWork];
		if (work.Deleted)
		{
			return null;
		}

		return work;
	}

	protected VPPXESessionInfo GetSession()
	{
		if (!m_Owner || !m_Owner.GetModel())
		{
			return null;
		}

		return m_Owner.GetModel().GetSession();
	}

	protected string FileLabel(string fileKey)
	{
		if (m_Owner && m_Owner.GetModel())
		{
			return m_Owner.GetModel().FileLabel(fileKey);
		}

		return fileKey;
	}

	protected string Tr(string key)
	{
		if (m_Owner)
		{
			return m_Owner.Tr(key);
		}

		return Widget.TranslateString(key);
	}

	protected int NowMs()
	{
		return GetGame().GetTime();
	}
};
