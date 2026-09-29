// VPP XML Editor: EVENTS tab. Edits the dynamic events files (db/events.xml and every <ce type="events"> file).
// Left: file dropdown, search, a kind / state filter, the file's events (name, kind, nominal, position, limit) and
// ADD / DUPLICATE / DELETE. Right: the selected event: name, active, the whole numbers, position, limit, flags, the
// secondary event and the children (type, min, max, loot min / max, deloot, spawnsecondary, trace), plus every check
// the server makes (VPPXEEventRules; the merged view of all events files and cfgeventspawns.xml decide what the server
// really runs). The SPAWNS view edits the event's cfgeventspawns.xml entries on a map: positions (click to add, the
// admin's own position, x / z / angle / height / group), the zone. The GROUPS view (VPPXEGroupsView) edits
// cfgeventgroups.xml. Edits stay local until SAVE (XE_SaveEvents for the selected events file, XE_SaveEventSpawns for
// cfgeventspawns.xml, XE_SaveEventGroups for cfgeventgroups.xml, EditEvents); REVERT drops them.

class VPPXEEvtWork : Managed
{
	int OrigIndex;
	bool Deleted;
	string InputError;
	ref VPPXEEventRow Row;
	ref VPPXEEventRow Orig;

	void VPPXEEvtWork()
	{
		OrigIndex = -1;
		InputError = "";
		Row = new VPPXEEventRow();
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

class VPPXEEvtFile : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	bool ServerChanged;
	ref array<ref VPPXEEvtWork> Work;

	void VPPXEEvtFile()
	{
		Key = "";
		ErrorKey = "";
		Work = new array<ref VPPXEEvtWork>;
	}

	int DirtyCount()
	{
		int dirty = 0;
		foreach (VPPXEEvtWork work : Work)
		{
			if (work.IsDirty())
			{
				dirty++;
			}
		}

		return dirty;
	}
};

class VPPXEEvtIncoming : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	int ChunkCount;
	ref map<int, ref array<ref VPPXEEventRow>> ChunkRows;

	void VPPXEEvtIncoming()
	{
		Key = "";
		ErrorKey = "";
		ChunkRows = new map<int, ref array<ref VPPXEEventRow>>;
	}

	int Got()
	{
		return ChunkRows.Count();
	}

	// Rows in chunk order: their position is the event index UPDATE and DELETE address.
	void JoinRows(array<ref VPPXEEventRow> outRows)
	{
		outRows.Clear();
		for (int c = 0; c < ChunkCount; c++)
		{
			array<ref VPPXEEventRow> part = ChunkRows.Get(c);
			if (!part)
			{
				continue;
			}

			foreach (VPPXEEventRow row : part)
			{
				outRows.Insert(row);
			}
		}
	}
};

// cfgeventspawns.xml working copy: one <event name> entry of the file (OrigIndex = its index in the file, -1 = new).
class VPPXESpawnWork : Managed
{
	int OrigIndex;
	bool Deleted;
	ref VPPXESpawnRow Row;
	ref VPPXESpawnRow Orig;

	void VPPXESpawnWork()
	{
		OrigIndex = -1;
		Row = new VPPXESpawnRow();
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

class VPPXESpawnFile : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	string GroupsError;
	bool ServerChanged;
	ref array<ref VPPXESpawnWork> Work;

	void VPPXESpawnFile()
	{
		Key = "";
		ErrorKey = "";
		GroupsError = "";
		Work = new array<ref VPPXESpawnWork>;
	}

	int DirtyCount()
	{
		int dirty = 0;
		foreach (VPPXESpawnWork work : Work)
		{
			// a new entry that was never given a position or a zone is not written
			if (work.OrigIndex < 0 && !work.Deleted && !work.Row.HasZone && work.Row.Positions.Count() == 0)
			{
				continue;
			}

			if (work.OrigIndex < 0 && work.Deleted)
			{
				continue;
			}

			if (work.IsDirty())
			{
				dirty++;
			}
		}

		return dirty;
	}
};

// The SPAWNS view map: the positions of one event (angle tick when a is not negative), the zone radius around each
// position, the selection. Reuses the calibration and drawing of the MAP tab renderer.
class VPPXESpawnRenderer : VPPXEMapRenderer
{
	const static int KIND_PLAIN = 0;
	const static int KIND_GROUP = 1;
	const static int KIND_UNUSABLE = 2;
	// zone rings on screen at once (28 lines each; the markers must stay under the renderer's line budget)
	const static int MAX_RINGS = 150;

	protected ref array<float> m_PosX;
	protected ref array<float> m_PosZ;
	protected ref array<float> m_PosA;
	protected ref array<int> m_PosKind;
	protected float m_ZoneR;

	void VPPXESpawnRenderer(MapWidget mapWidget, CanvasWidget canvasWidget)
	{
		m_PosX = new array<float>;
		m_PosZ = new array<float>;
		m_PosA = new array<float>;
		m_PosKind = new array<int>;
		m_ZoneR = 0;
	}

	void SetSpawns(array<float> xs, array<float> zs, array<float> angles, array<int> kinds, float zoneR)
	{
		m_PosX.Copy(xs);
		m_PosZ.Copy(zs);
		m_PosA.Copy(angles);
		m_PosKind.Copy(kinds);
		m_ZoneR = zoneR;
	}

	override void Redraw()
	{
		ClearHitCache();
		m_LineCount = 0;
		if (!m_Canvas || !m_Map)
		{
			return;
		}

		m_Canvas.Clear();
		if (!Calibrate())
		{
			return;
		}

		int count = m_PosX.Count();
		float zonePx = m_ZoneR * Math.AbsFloat(m_AX);
		int rings = 0;
		if (zonePx >= 4)
		{
			for (int z = 0; z < count && rings < MAX_RINGS; z++)
			{
				float zx = m_S0X + m_PosX[z] * m_AX;
				float zy = m_S0Y + m_PosZ[z] * m_AZ;
				if (zx < -zonePx || zy < -zonePx || zx > m_CW + zonePx || zy > m_CH + zonePx)
				{
					continue;
				}

				DrawRing(zx, zy, zonePx, ARGB(210, 70, 150, 255));
				rings++;
			}
		}

		int outline = ARGB(230, 8, 9, 10);
		float tickWorld = 16 / Math.Max(Math.AbsFloat(m_AX), 0.0001);
		for (int i = 0; i < count; i++)
		{
			float lx = m_S0X + m_PosX[i] * m_AX;
			float ly = m_S0Y + m_PosZ[i] * m_AZ;
			if (lx < -20 || ly < -20 || lx > m_CW + 20 || ly > m_CH + 20)
			{
				continue;
			}

			int color = ARGB(255, 232, 163, 61);
			if (m_PosKind[i] == KIND_GROUP)
			{
				color = ARGB(255, 175, 125, 255);
			}
			else if (m_PosKind[i] == KIND_UNUSABLE)
			{
				color = ARGB(255, 194, 69, 69);
			}

			float angle = m_PosA[i];
			if (angle >= 0)
			{
				float rad = angle * Math.DEG2RAD;
				float ex = m_S0X + (m_PosX[i] + Math.Sin(rad) * tickWorld) * m_AX;
				float ey = m_S0Y + (m_PosZ[i] + Math.Cos(rad) * tickWorld) * m_AZ;
				DrawSegment(lx, ly, ex, ey, 5, outline);
				DrawSegment(lx, ly, ex, ey, 3, ARGB(255, 60, 255, 90));
			}

			DrawDisc(lx, ly, 7, outline);
			DrawDisc(lx, ly, 5, color);
			AddHitMark(0, i, 1, 0, m_PosX[i], m_PosZ[i]);
		}

		DrawSelection();
	}

	// Absolute screen pixels of a world position (false when the view cannot be calibrated).
	bool ScreenOf(float wx, float wz, out float sx, out float sy)
	{
		sx = 0;
		sy = 0;
		if (!Calibrate())
		{
			return false;
		}

		sx = m_CX + m_S0X + wx * m_AX;
		sy = m_CY + m_S0Y + wz * m_AZ;
		return true;
	}

	// The map widget's rect in absolute screen pixels (from the last Calibrate).
	void MapRect(out float x, out float y, out float w, out float h)
	{
		x = m_CX;
		y = m_CY;
		w = m_CW;
		h = m_CH;
	}

	protected void DrawRing(float cx, float cy, float r, int color)
	{
		int steps = 28;
		float prevX = cx + r;
		float prevY = cy;
		for (int s = 1; s <= steps; s++)
		{
			float rad = s * Math.PI2 / steps;
			float nx = cx + Math.Cos(rad) * r;
			float ny = cy + Math.Sin(rad) * r;
			DrawSegment(prevX, prevY, nx, ny, 1.5, color);
			prevX = nx;
			prevY = ny;
		}
	}
};

class VPPXEEventsTab : ScriptedWidgetEventHandler
{
	const static int HEADER_H = 28;
	const static int ROW_H = 28;
	const static int FOOTER_H = 36;
	const static int BAR_H = 40;
	const static int CHIP_LABEL_W = 112;
	const static int NUM_LABEL_W = 118;
	const static int NUM_INPUT_W = 80;

	const static int FILTER_ALL = 0;
	const static int FILTER_UNKNOWN = 9;
	const static int FILTER_OFF = 10;
	const static int FILTER_WARN = 11;
	const static int FILTER_UNSAVED = 12;
	const static int FILTER_COUNT = 13;

	const static int VIEW_FORM = 0;
	const static int VIEW_SPAWNS = 1;
	const static int VIEW_GROUPS = 2;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_Left;
	protected Widget m_ListHeader;
	protected Widget m_FileHost;
	protected Widget m_FilterHost;
	protected Widget m_SearchPanel;
	protected ImageWidget m_ImgSearch;
	protected EditBoxWidget m_InputSearch;
	protected TextListboxWidget m_EvtList;
	protected ButtonWidget m_BtnAdd;
	protected ButtonWidget m_BtnDup;
	protected ButtonWidget m_BtnDelete;
	protected Widget m_Right;
	protected TextWidget m_TxtEditTitle;
	protected TextWidget m_TxtLine;
	protected Widget m_Form;
	protected TextWidget m_LblName;
	protected EditBoxWidget m_InputName;
	protected TextWidget m_TxtKind;
	protected ButtonWidget m_BtnActive;
	protected Widget m_FillActive;
	protected TextWidget m_TxtActive;
	protected TextWidget m_TxtState;
	protected ref array<TextWidget> m_LblNum;
	protected ref array<EditBoxWidget> m_InputNum;
	protected ref array<TextWidget> m_TxtHint;
	protected TextWidget m_LblPosition;
	protected ref array<ButtonWidget> m_BtnPos;
	protected ref array<Widget> m_FillPos;
	protected ref array<TextWidget> m_TxtPos;
	protected TextWidget m_LblLimit;
	protected ref array<ButtonWidget> m_BtnLim;
	protected ref array<Widget> m_FillLim;
	protected ref array<TextWidget> m_TxtLim;
	protected TextWidget m_LblFlags;
	protected ref array<ButtonWidget> m_BtnFlag;
	protected ref array<Widget> m_FillFlag;
	protected ref array<TextWidget> m_TxtFlag;
	protected TextWidget m_LblSecondary;
	protected EditBoxWidget m_InputSecondary;
	protected TextWidget m_TxtSecondaryHint;
	protected ImageWidget m_ImgChildren;
	protected TextWidget m_TxtChildrenTitle;
	protected TextListboxWidget m_ChildList;
	protected TextWidget m_LblChildType;
	protected TextWidget m_LblChildMin;
	protected TextWidget m_LblChildMax;
	protected TextWidget m_LblChildLootMin;
	protected TextWidget m_LblChildLootMax;
	protected EditBoxWidget m_InputChildType;
	protected EditBoxWidget m_InputChildMin;
	protected EditBoxWidget m_InputChildMax;
	protected EditBoxWidget m_InputChildLootMin;
	protected EditBoxWidget m_InputChildLootMax;
	protected TextWidget m_LblChildDeloot;
	protected EditBoxWidget m_InputChildDeloot;
	protected ButtonWidget m_BtnChildSpawnSec;
	protected Widget m_FillChildSpawnSec;
	protected TextWidget m_TxtChildSpawnSec;
	protected ButtonWidget m_BtnChildTrace;
	protected Widget m_FillChildTrace;
	protected TextWidget m_TxtChildTrace;
	protected ButtonWidget m_BtnChildAdd;
	protected ButtonWidget m_BtnChildRemove;
	protected TextWidget m_TxtWarnings;
	protected TextWidget m_TxtEmpty;
	protected Widget m_ActionBar;
	protected TextWidget m_TxtStatus;
	protected ButtonWidget m_BtnRevert;
	protected ButtonWidget m_BtnSave;
	protected ref VPPDropDownMenu m_FileDD;
	protected ref VPPDropDownMenu m_FilterDD;

	protected ref map<string, ref VPPXEEvtFile> m_Files;
	protected ref array<string> m_FileKeys;
	protected string m_FileKey;
	protected string m_WantFile;
	protected ref array<int> m_ListWork;
	protected int m_SelWork;
	protected int m_ListSel;
	protected int m_ChildSel;
	protected int m_ChildListSel;
	protected int m_FilterIdx;
	protected bool m_Loading;
	protected ref array<string> m_LastNum;
	protected string m_LastName;
	protected string m_LastSecondary;
	protected string m_LastSearch;
	protected string m_LastChildType;
	protected string m_LastChildMin;
	protected string m_LastChildMax;
	protected string m_LastChildLootMin;
	protected string m_LastChildLootMax;
	protected string m_LastChildDeloot;
	protected string m_PendName;
	protected int m_PendCopyWork;

	protected ref VPPXEEventMerge m_Merge;
	protected bool m_MergeStale;
	// "fileKey|name" -> events of that name in that file (not deleted), rebuilt with the merge
	protected ref map<string, int> m_NameCounts;
	protected ref VPPXESpawnFile m_Spawns;
	protected ref map<int, ref array<ref VPPXESpawnRow>> m_SpawnChunks;
	protected int m_SpawnChunkCount;
	protected string m_NewSpawnKey;
	protected int m_NewSpawnRevision;
	protected bool m_NewSpawnEditable;
	protected string m_NewSpawnError;
	protected string m_NewGroupsError;
	protected bool m_SpawnForceFresh;
	protected int m_SpawnSaveReqId;
	protected bool m_EvtSavePending;
	protected bool m_SpawnSavePending;
	protected bool m_ReloadAfterSave;

	protected bool m_ViewSpawns;
	protected bool m_MapAllowed;
	protected bool m_SpawnAddMode;
	protected int m_SpawnSel;
	protected int m_SpawnListSel;
	protected ref array<int> m_SpawnRefWork;
	protected ref array<int> m_SpawnRefPos;
	protected ref array<int> m_SpawnDrawnRefs;
	protected ref array<string> m_LastZone;
	protected ref array<string> m_LastPos;
	protected float m_SpawnLastScale;
	protected vector m_SpawnLastPos;
	protected int m_SpawnLastRedraw;
	protected bool m_SpawnRedraw;
	protected bool m_SpawnFitPending;
	// the event the SPAWNS view shows (another one refits the map and drops the position selection)
	protected VPPXEEvtWork m_SpawnSelWork;
	// the cfgeventspawns.xml entries of the selected event, captured when the form loads: a rename in the form renames
	// exactly these (m_RenameSync = the event is the only definition of its name)
	protected ref array<VPPXESpawnWork> m_RenameEntries;
	protected bool m_RenameSync;
	// screen position of the last left press on the SPAWNS map (a pan that ends in a click adds nothing)
	protected int m_MapDownX;
	protected int m_MapDownY;
	protected ref VPPXESpawnRenderer m_SpawnRenderer;
	protected ref VPPXEGroupsView m_GroupsView;
	protected bool m_ViewGroups;
	protected int m_GroupSaveReqId;
	protected bool m_GroupSavePending;
	protected ButtonWidget m_BtnViewGroups;
	protected Widget m_FillViewGroups;
	protected TextWidget m_TxtViewGroups;
	protected ButtonWidget m_BtnSpawnGroup;
	// EDIT GROUP (not the GROUPS chip) opens the view: an unknown group is reported
	protected bool m_PreviewLoud;
	protected ButtonWidget m_BtnViewForm;
	protected Widget m_FillViewForm;
	protected TextWidget m_TxtViewForm;
	protected ButtonWidget m_BtnViewSpawns;
	protected Widget m_FillViewSpawns;
	protected TextWidget m_TxtViewSpawns;
	protected Widget m_SpawnView;
	protected ButtonWidget m_BtnZone;
	protected Widget m_FillZone;
	protected TextWidget m_TxtZone;
	protected ref array<TextWidget> m_LblZone;
	protected ref array<EditBoxWidget> m_InputZone;
	protected Widget m_SpawnMapArea;
	protected MapWidget m_SpawnMap;
	protected CanvasWidget m_SpawnCanvas;
	protected TextListboxWidget m_SpawnList;
	protected ref array<TextWidget> m_LblPos;
	protected ref array<EditBoxWidget> m_InputPos;
	protected ButtonWidget m_BtnSpawnAdd;
	protected Widget m_FillSpawnAdd;
	protected TextWidget m_TxtSpawnAdd;
	protected ButtonWidget m_BtnSpawnHere;
	protected ButtonWidget m_BtnSpawnRemove;
	protected ImageWidget m_ImgSpawnMe;
	protected ImageWidget m_ImgSpawnMeBg;
	// pan: the drawing slides with the map (anchor = screen pos of world 0,0 at the last redraw), redraws on settle
	protected bool m_SpawnAnchorValid;
	protected vector m_SpawnAnchorScreen;
	protected float m_SpawnAnchorScale;
	protected bool m_SpawnSettlePending;
	protected int m_SpawnLastChange;
	protected float m_SpawnLastCX;
	protected float m_SpawnLastCY;
	protected float m_SpawnLastCW;
	protected float m_SpawnLastCH;
	// left button edges, polled: a MapWidget does not pass its clicks on to the tab's handler
	protected bool m_MouseWasDown;
	protected bool m_MapPressed;
	// hover tips: widget, title #key, text #key
	protected ref array<Widget> m_TipWidgets;
	protected ref array<string> m_TipTitles;
	protected ref array<string> m_TipBodies;
	protected ref Widget m_TipRoot;
	protected TextWidget m_TipTitle;
	protected TextWidget m_TipBody;
	protected Widget m_TipOwner;
	protected TextWidget m_TxtSpawnInfo;
	protected bool m_FilesDone;

	protected bool m_Shown;
	protected bool m_Loaded;
	protected int m_ReqId;
	protected bool m_Pending;
	protected int m_SentAt;
	protected int m_ExpectedFiles;
	protected ref map<string, ref VPPXEEvtIncoming> m_Incoming;
	protected string m_LoadedSig;
	protected ref map<string, bool> m_ForceFresh;
	protected int m_SaveReqId;
	protected bool m_Saving;
	protected int m_SaveSentAt;
	protected string m_SaveFile;
	protected string m_StatusText;
	protected bool m_StatusError;

	protected float m_Unit;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_LeftW;
	protected float m_LeftH;
	protected float m_RightW;
	protected float m_RightH;

	void VPPXEEventsTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_LblNum = new array<TextWidget>;
		m_InputNum = new array<EditBoxWidget>;
		m_TxtHint = new array<TextWidget>;
		m_BtnPos = new array<ButtonWidget>;
		m_FillPos = new array<Widget>;
		m_TxtPos = new array<TextWidget>;
		m_BtnLim = new array<ButtonWidget>;
		m_FillLim = new array<Widget>;
		m_TxtLim = new array<TextWidget>;
		m_BtnFlag = new array<ButtonWidget>;
		m_FillFlag = new array<Widget>;
		m_TxtFlag = new array<TextWidget>;
		m_LastNum = new array<string>;
		m_Files = new map<string, ref VPPXEEvtFile>;
		m_FileKeys = new array<string>;
		m_ListWork = new array<int>;
		m_Incoming = new map<string, ref VPPXEEvtIncoming>;
		m_ForceFresh = new map<string, bool>;
		m_Merge = new VPPXEEventMerge();
		m_MergeStale = true;
		m_NameCounts = new map<string, int>;
		m_SpawnChunks = new map<int, ref array<ref VPPXESpawnRow>>;
		m_SpawnChunkCount = -1;
		m_NewSpawnKey = "";
		m_NewSpawnError = "";
		m_NewGroupsError = "";
		m_MapAllowed = true;
		m_SpawnSel = -1;
		m_SpawnListSel = -1;
		m_SpawnRefWork = new array<int>;
		m_SpawnRefPos = new array<int>;
		m_SpawnDrawnRefs = new array<int>;
		m_RenameEntries = new array<VPPXESpawnWork>;
		m_TipWidgets = new array<Widget>;
		m_TipTitles = new array<string>;
		m_TipBodies = new array<string>;
		m_LastZone = new array<string>;
		m_LastPos = new array<string>;
		m_LblZone = new array<TextWidget>;
		m_InputZone = new array<EditBoxWidget>;
		m_LblPos = new array<TextWidget>;
		m_InputPos = new array<EditBoxWidget>;
		m_SelWork = -1;
		m_ListSel = -1;
		m_ChildSel = -1;
		m_ChildListSel = -1;
		m_PendCopyWork = -1;
		m_Unit = 1.0;
		m_FileKey = "";
		m_WantFile = "";
		m_LoadedSig = "";
		m_StatusText = "";
		m_LastSearch = "";
		m_PendName = "";
		for (int n = 0; n < VPPXEEventRules.NUMBER_FIELDS; n++)
		{
			m_LastNum.Insert("");
		}

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorEventsTab, host);
		m_Root.SetHandler(this);
		BindWidgets();
		BindSpawnWidgets();
		m_GroupsView = new VPPXEGroupsView(this, m_Owner, m_Root);
		ReleaseFocusOnLeave();
		RegisterTips();
		m_GroupsView.RegisterTips();
		if (m_SpawnMap && m_SpawnCanvas)
		{
			m_SpawnRenderer = new VPPXESpawnRenderer(m_SpawnMap, m_SpawnCanvas);
		}

		m_FileDD = new VPPDropDownMenu(m_FileHost, "");
		m_FileDD.m_OnSelectItem.Insert(OnFilePicked);
		m_FilterDD = new VPPDropDownMenu(m_FilterHost, "");
		m_FilterDD.m_OnSelectItem.Insert(OnFilterPicked);
		BuildFilterDropdown();
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnEventsChunk", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnSpawnsChunk", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnGroupsChunk", this, SingleplayerExecutionType.Client);
		ShowEditor(false);
		UpdateViewChips();
		UpdateActionBar();
	}

	protected void BindSpawnWidgets()
	{
		m_BtnViewForm = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtViewForm"));
		m_FillViewForm = m_Root.FindAnyWidget("FillEvtViewForm");
		m_TxtViewForm = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtViewForm"));
		m_BtnViewSpawns = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtViewSpawns"));
		m_FillViewSpawns = m_Root.FindAnyWidget("FillEvtViewSpawns");
		m_TxtViewSpawns = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtViewSpawns"));
		m_BtnViewGroups = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtViewGroups"));
		m_FillViewGroups = m_Root.FindAnyWidget("FillEvtViewGroups");
		m_TxtViewGroups = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtViewGroups"));
		m_SpawnView = m_Root.FindAnyWidget("EvtSpawnView");
		m_BtnZone = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtZone"));
		m_FillZone = m_Root.FindAnyWidget("FillEvtZone");
		m_TxtZone = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtZone"));
		for (int z = 0; z < 5; z++)
		{
			m_LblZone.Insert(TextWidget.Cast(m_Root.FindAnyWidget("LblEvtZone" + z.ToString())));
			m_InputZone.Insert(EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtZone" + z.ToString())));
			m_LastZone.Insert("");
		}

		m_SpawnMapArea = m_Root.FindAnyWidget("EvtSpawnMapArea");
		m_SpawnMap = MapWidget.Cast(m_Root.FindAnyWidget("EvtSpawnMap"));
		m_SpawnCanvas = CanvasWidget.Cast(m_Root.FindAnyWidget("EvtSpawnCanvas"));
		m_SpawnList = TextListboxWidget.Cast(m_Root.FindAnyWidget("EvtSpawnList"));
		for (int p = 0; p < 5; p++)
		{
			m_LblPos.Insert(TextWidget.Cast(m_Root.FindAnyWidget("LblEvtPos" + p.ToString())));
			m_InputPos.Insert(EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtPos" + p.ToString())));
			m_LastPos.Insert("");
		}

		m_BtnSpawnAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtSpawnAdd"));
		m_FillSpawnAdd = m_Root.FindAnyWidget("FillEvtSpawnAdd");
		m_TxtSpawnAdd = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtSpawnAdd"));
		m_BtnSpawnHere = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtSpawnHere"));
		m_BtnSpawnRemove = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtSpawnRemove"));
		m_BtnSpawnGroup = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtSpawnGroup"));
		m_ImgSpawnMe = ImageWidget.Cast(m_Root.FindAnyWidget("ImgEvtSpawnMe"));
		m_ImgSpawnMeBg = ImageWidget.Cast(m_Root.FindAnyWidget("ImgEvtSpawnMeBg"));
		m_TxtSpawnInfo = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtSpawnInfo"));
	}

	protected void BindWidgets()
	{
		m_Left = m_Root.FindAnyWidget("EvtLeft");
		m_ListHeader = m_Root.FindAnyWidget("EvtListHeader");
		m_FileHost = m_Root.FindAnyWidget("EvtFileHost");
		m_FilterHost = m_Root.FindAnyWidget("EvtFilterHost");
		m_SearchPanel = m_Root.FindAnyWidget("EvtSearchPanel");
		m_ImgSearch = ImageWidget.Cast(m_Root.FindAnyWidget("ImgEvtSearch"));
		m_InputSearch = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtSearch"));
		m_EvtList = TextListboxWidget.Cast(m_Root.FindAnyWidget("EvtList"));
		m_BtnAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtAdd"));
		m_BtnDup = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtDup"));
		m_BtnDelete = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtDelete"));
		m_Right = m_Root.FindAnyWidget("EvtRight");
		m_TxtEditTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtEditTitle"));
		m_TxtLine = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtLine"));
		m_Form = m_Root.FindAnyWidget("EvtForm");
		m_LblName = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtName"));
		m_InputName = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtName"));
		m_TxtKind = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtKind"));
		m_BtnActive = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtActive"));
		m_FillActive = m_Root.FindAnyWidget("FillEvtActive");
		m_TxtActive = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtActive"));
		m_TxtState = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtState"));
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			m_LblNum.Insert(TextWidget.Cast(m_Root.FindAnyWidget("LblEvtNum" + i.ToString())));
			m_InputNum.Insert(EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtNum" + i.ToString())));
			m_TxtHint.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtHint" + i.ToString())));
		}

		m_LblPosition = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtPosition"));
		for (int p = 0; p < 3; p++)
		{
			m_BtnPos.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtPos" + p.ToString())));
			m_FillPos.Insert(m_Root.FindAnyWidget("FillEvtPos" + p.ToString()));
			m_TxtPos.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtPos" + p.ToString())));
		}

		m_LblLimit = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtLimit"));
		for (int l = 0; l < 4; l++)
		{
			m_BtnLim.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtLim" + l.ToString())));
			m_FillLim.Insert(m_Root.FindAnyWidget("FillEvtLim" + l.ToString()));
			m_TxtLim.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtLim" + l.ToString())));
		}

		m_LblFlags = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtFlags"));
		for (int f = 0; f < 3; f++)
		{
			m_BtnFlag.Insert(ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtFlag" + f.ToString())));
			m_FillFlag.Insert(m_Root.FindAnyWidget("FillEvtFlag" + f.ToString()));
			m_TxtFlag.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtFlag" + f.ToString())));
		}

		m_LblSecondary = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtSecondary"));
		m_InputSecondary = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtSecondary"));
		m_TxtSecondaryHint = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtSecondaryHint"));
		m_ImgChildren = ImageWidget.Cast(m_Root.FindAnyWidget("ImgEvtChildren"));
		m_TxtChildrenTitle = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtChildrenTitle"));
		m_ChildList = TextListboxWidget.Cast(m_Root.FindAnyWidget("EvtChildList"));
		m_LblChildType = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildType"));
		m_LblChildMin = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildMin"));
		m_LblChildMax = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildMax"));
		m_LblChildLootMin = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildLootMin"));
		m_LblChildLootMax = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildLootMax"));
		m_InputChildType = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildType"));
		m_InputChildMin = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildMin"));
		m_InputChildMax = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildMax"));
		m_InputChildLootMin = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildLootMin"));
		m_InputChildLootMax = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildLootMax"));
		m_LblChildDeloot = TextWidget.Cast(m_Root.FindAnyWidget("LblEvtChildDeloot"));
		m_InputChildDeloot = EditBoxWidget.Cast(m_Root.FindAnyWidget("InputEvtChildDeloot"));
		m_BtnChildSpawnSec = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtChildSpawnSec"));
		m_FillChildSpawnSec = m_Root.FindAnyWidget("FillEvtChildSpawnSec");
		m_TxtChildSpawnSec = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtChildSpawnSec"));
		m_BtnChildTrace = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtChildTrace"));
		m_FillChildTrace = m_Root.FindAnyWidget("FillEvtChildTrace");
		m_TxtChildTrace = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtChildTrace"));
		m_BtnChildAdd = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtChildAdd"));
		m_BtnChildRemove = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtChildRemove"));
		m_TxtWarnings = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtWarnings"));
		m_TxtEmpty = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtEmpty"));
		m_ActionBar = m_Root.FindAnyWidget("EvtActionBar");
		m_TxtStatus = TextWidget.Cast(m_Root.FindAnyWidget("TxtEvtStatus"));
		m_BtnRevert = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtRevert"));
		m_BtnSave = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnEvtSave"));
	}

	// Every text box of the tab lets go of the keyboard when the mouse leaves it (like VPPXENumericEditHandler).
	protected void ReleaseFocusOnLeave()
	{
		array<EditBoxWidget> boxes = new array<EditBoxWidget>;
		boxes.Insert(m_InputSearch);
		boxes.Insert(m_InputName);
		boxes.Insert(m_InputSecondary);
		boxes.Insert(m_InputChildType);
		boxes.Insert(m_InputChildMin);
		boxes.Insert(m_InputChildMax);
		boxes.Insert(m_InputChildLootMin);
		boxes.Insert(m_InputChildLootMax);
		boxes.Insert(m_InputChildDeloot);
		foreach (EditBoxWidget numberBox : m_InputNum)
		{
			boxes.Insert(numberBox);
		}

		foreach (EditBoxWidget zoneBox : m_InputZone)
		{
			boxes.Insert(zoneBox);
		}

		foreach (EditBoxWidget posBox : m_InputPos)
		{
			boxes.Insert(posBox);
		}

		foreach (EditBoxWidget box : boxes)
		{
			if (box)
			{
				box.SetHandler(this);
			}
		}
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

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		int tipIdx = m_TipWidgets.Find(w);
		if (tipIdx >= 0)
		{
			ShowTip(tipIdx);
		}

		return false;
	}

	// Hover tips of the inputs and toggles (the shared VPPInfoBox, like ToolTipHandler).
	protected void RegisterTips()
	{
		AddTip(m_InputName, "#VSTR_XMLE_EVT_NAME", "#VSTR_XMLE_EVT_TIP_NAME");
		AddTip(m_InputNum[0], "#VSTR_XMLE_EVT_F0", "#VSTR_XMLE_EVT_TIP_F0");
		AddTip(m_InputNum[1], "#VSTR_XMLE_EVT_F1", "#VSTR_XMLE_EVT_TIP_F1");
		AddTip(m_InputNum[2], "#VSTR_XMLE_EVT_F2", "#VSTR_XMLE_EVT_TIP_F2");
		AddTip(m_InputNum[3], "#VSTR_XMLE_EVT_F3", "#VSTR_XMLE_EVT_TIP_F3");
		AddTip(m_InputNum[4], "#VSTR_XMLE_EVT_F4", "#VSTR_XMLE_EVT_TIP_F4");
		AddTip(m_InputNum[5], "#VSTR_XMLE_EVT_F5", "#VSTR_XMLE_EVT_TIP_F5");
		AddTip(m_InputNum[6], "#VSTR_XMLE_EVT_F6", "#VSTR_XMLE_EVT_TIP_F6");
		AddTip(m_InputNum[7], "#VSTR_XMLE_EVT_F7", "#VSTR_XMLE_EVT_TIP_F7");
		AddTip(m_InputSecondary, "#VSTR_XMLE_EVT_SECONDARY", "#VSTR_XMLE_EVT_TIP_SECONDARY");
		AddTip(m_InputChildType, "#VSTR_XMLE_EVT_CHILD_TYPE", "#VSTR_XMLE_EVT_TIP_CHILD_TYPE");
		AddTip(m_InputChildMin, "#VSTR_XMLE_EVT_CHILD_MIN", "#VSTR_XMLE_EVT_TIP_CHILD_MIN");
		AddTip(m_InputChildMax, "#VSTR_XMLE_EVT_CHILD_MAX", "#VSTR_XMLE_EVT_TIP_CHILD_MAX");
		AddTip(m_InputChildLootMin, "#VSTR_XMLE_EVT_CHILD_LOOTMIN", "#VSTR_XMLE_EVT_TIP_LOOTMIN");
		AddTip(m_InputChildLootMax, "#VSTR_XMLE_EVT_CHILD_LOOTMAX", "#VSTR_XMLE_EVT_TIP_LOOTMAX");
		AddTip(m_InputChildDeloot, "#VSTR_XMLE_EVT_CHILD_DELOOT", "#VSTR_XMLE_EVT_TIP_DELOOT");
		AddTip(m_BtnChildSpawnSec, "#VSTR_XMLE_EVT_CHILD_SPAWNSEC", "#VSTR_XMLE_EVT_TIP_SPAWNSEC");
		AddTip(m_BtnChildTrace, "#VSTR_XMLE_EVT_CHILD_TRACE", "#VSTR_XMLE_EVT_TIP_TRACE");
		AddTip(m_BtnZone, "#VSTR_XMLE_SPW_ZONE", "#VSTR_XMLE_SPW_TIP_ZONE");
		AddTip(m_InputZone[0], "#VSTR_XMLE_SPW_Z_SMIN", "#VSTR_XMLE_SPW_TIP_SMIN");
		AddTip(m_InputZone[1], "#VSTR_XMLE_SPW_Z_SMAX", "#VSTR_XMLE_SPW_TIP_SMAX");
		AddTip(m_InputZone[2], "#VSTR_XMLE_SPW_Z_DMIN", "#VSTR_XMLE_SPW_TIP_DMIN");
		AddTip(m_InputZone[3], "#VSTR_XMLE_SPW_Z_DMAX", "#VSTR_XMLE_SPW_TIP_DMAX");
		AddTip(m_InputZone[4], "#VSTR_XMLE_SPW_Z_R", "#VSTR_XMLE_SPW_TIP_R");
		AddTip(m_InputPos[0], "#VSTR_XMLE_SPW_P_X", "#VSTR_XMLE_SPW_TIP_X");
		AddTip(m_InputPos[1], "#VSTR_XMLE_SPW_P_Z", "#VSTR_XMLE_SPW_TIP_Z");
		AddTip(m_InputPos[2], "#VSTR_XMLE_SPW_P_A", "#VSTR_XMLE_SPW_TIP_A");
		AddTip(m_InputPos[3], "#VSTR_XMLE_SPW_P_Y", "#VSTR_XMLE_SPW_TIP_Y");
		AddTip(m_InputPos[4], "#VSTR_XMLE_SPW_P_GROUP", "#VSTR_XMLE_SPW_TIP_GROUP");
		AddTip(m_BtnSpawnAdd, "#VSTR_XMLE_SPW_BTN_ADD", "#VSTR_XMLE_SPW_TIP_ADD");
		AddTip(m_BtnSpawnHere, "#VSTR_XMLE_SPW_BTN_HERE", "#VSTR_XMLE_SPW_TIP_HERE");
		AddTip(m_BtnSpawnGroup, "#VSTR_XMLE_SPW_BTN_GROUP", "#VSTR_XMLE_SPW_TIP_GROUP_BTN");
		AddTip(m_BtnViewGroups, "#VSTR_XMLE_EVT_VIEW_GROUPS", "#VSTR_XMLE_GRP_TIP_VIEW");
	}

	void AddTip(Widget w, string titleKey, string bodyKey)
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

	// Next to the cursor; moved above it when it would cover the SPAWNS or GROUPS map (a MapWidget draws over other
	// widgets).
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
		m_TipTitle.SetText(Tr(m_TipTitles[tipIdx]));
		m_TipBody.SetText(Tr(m_TipBodies[tipIdx]));
		int mx;
		int my;
		GetMousePos(mx, my);
		m_TipRoot.SetPos(mx + 14, my + 18);
		m_TipRoot.Show(true);
		m_TipRoot.Update();
		float tipW;
		float tipH;
		m_TipRoot.GetScreenSize(tipW, tipH);
		MapWidget shownMap = null;
		if (m_SpawnMap && m_SpawnMap.IsVisible())
		{
			shownMap = m_SpawnMap;
		}
		else if (m_GroupsView && m_GroupsView.GetMap() && m_GroupsView.GetMap().IsVisible())
		{
			shownMap = m_GroupsView.GetMap();
		}

		if (shownMap)
		{
			float mapX;
			float mapY;
			float mapW;
			float mapH;
			shownMap.GetScreenPos(mapX, mapY);
			shownMap.GetScreenSize(mapW, mapH);
			bool overlaps = mx + 14 < mapX + mapW && mx + 14 + tipW > mapX && my + 18 < mapY + mapH && my + 18 + tipH > mapY;
			if (overlaps)
			{
				m_TipRoot.SetPos(mx + 14, my - tipH - 8);
			}
		}
	}

	protected void HideTip()
	{
		m_TipOwner = null;
		if (m_TipRoot)
		{
			m_TipRoot.Show(false);
		}
	}

	// ---------------------------------------------------------------- tab API

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			CloseDropdowns();
			HideTip();
			UpdateMapVisibility();
			return;
		}

		OnResize();
		if (!m_Loaded || CurrentSig() != m_LoadedSig)
		{
			RequestList();
		}

		RebuildAll();
		UpdateMapVisibility();
	}

	// True while a list or save request waits for its reply (the window holds XE_CloseSession back meanwhile).
	bool HasInFlight()
	{
		int now = NowMs();
		if (m_Pending && now - m_SentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return true;
		}

		return m_Saving && now - m_SaveSentAt <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void Refresh()
	{
		RequestList();
	}

	// A new file list or new revisions (events files or cfgeventspawns.xml): reload; FinishList keeps files with
	// unsaved edits and flags the ones whose server copy changed.
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

	// MenuXMLEditor.OpenEventsFile: show that file (after the next reply when it is not loaded yet).
	void SelectFile(string fileKey)
	{
		m_WantFile = fileKey;
		if (m_Files.Contains(fileKey))
		{
			SwitchFile(fileKey);
			m_WantFile = "";
		}
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
		PollListSelection();
		PollChildSelection();
		PollSearch();
		PollInputs();
		PollSpawnList();
		PollSpawnInputs();
		PollSpawnMapMouse();
		int now = NowMs();
		UpdateSpawnMap(now);
		m_GroupsView.OnUpdate(now);
		if (m_Pending && now - m_SentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Pending = false;
			SetStatus(Tr("#VSTR_XMLE_ERR_NOT_READY"), true);
		}

		if (m_Saving && now - m_SaveSentAt > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_Saving = false;
			m_EvtSavePending = false;
			m_SpawnSavePending = false;
			m_GroupSavePending = false;
			SetStatus(Tr("#VSTR_XMLE_ERR_NOT_READY"), true);
			if (m_ReloadAfterSave)
			{
				m_ReloadAfterSave = false;
				RequestList();
			}

			AfterSaveState();
		}
	}

	// A save ended (or timed out): the views were greyed while it ran.
	protected void AfterSaveState()
	{
		UpdateActionBar();
		UpdateDetails();
		UpdateSpawnDetails();
		m_GroupsView.Refresh();
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

		if (reqId == m_ReqId)
		{
			m_Pending = false;
			if (!ok)
			{
				SetStatus(string.Format(Tr(textKey), arg), true);
			}

			return true;
		}

		bool isEvents = m_SaveReqId > 0 && reqId == m_SaveReqId;
		bool isSpawns = m_SpawnSaveReqId > 0 && reqId == m_SpawnSaveReqId;
		bool isGroups = m_GroupSaveReqId > 0 && reqId == m_GroupSaveReqId;
		if (!isEvents && !isSpawns && !isGroups)
		{
			return false;
		}

		string savedKey = m_SaveFile;
		bool wasPending = m_EvtSavePending;
		if (isSpawns)
		{
			wasPending = m_SpawnSavePending;
			savedKey = "";
			if (m_Spawns)
			{
				savedKey = m_Spawns.Key;
			}
		}
		else if (isGroups)
		{
			wasPending = m_GroupSavePending;
			savedKey = m_GroupsView.FileKey();
		}

		if (isEvents)
		{
			m_EvtSavePending = false;
		}
		else if (isSpawns)
		{
			m_SpawnSavePending = false;
		}
		else
		{
			m_GroupSavePending = false;
		}

		m_Saving = m_EvtSavePending || m_SpawnSavePending || m_GroupSavePending;
		if (ok)
		{
			// also after the local timeout: the file was written, reload it
			if (isEvents)
			{
				m_ForceFresh.Set(m_SaveFile, true);
			}
			else if (isSpawns)
			{
				m_SpawnForceFresh = true;
			}
			else
			{
				m_GroupsView.ForceFresh();
			}

			m_ReloadAfterSave = true;
			if (wasPending)
			{
				string done = string.Format(Tr(textKey), FileLabel(savedKey));
				m_Owner.Notify(done);
				SetStatus(done, false);
			}
		}
		else if (wasPending)
		{
			string failed = string.Format(Tr(textKey), arg);
			m_Owner.NotifyError(failed);
			SetStatus(failed, true);
			if (textKey == "#VSTR_XMLE_ERR_STALE")
			{
				VPPXEEvtFile staleFile = m_Files.Get(m_SaveFile);
				if (isEvents && staleFile)
				{
					staleFile.ServerChanged = true;
				}
				else if (isSpawns && m_Spawns)
				{
					m_Spawns.ServerChanged = true;
				}
				else if (isGroups)
				{
					m_GroupsView.MarkServerChanged();
				}
			}
		}

		if (!m_Saving && m_ReloadAfterSave)
		{
			m_ReloadAfterSave = false;
			RequestList();
		}

		if (m_Saving)
		{
			UpdateActionBar();
		}
		else
		{
			AfterSaveState();
		}

		return true;
	}

	// ---------------------------------------------------------------- server-to-client receivers

	void XE_OnSpawnsChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXESpawnsChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXESpawnsChunk chunk = data.param1;
		if (!chunk || chunk.ReqId != m_ReqId || !m_Pending)
		{
			return;
		}

		m_SentAt = NowMs();
		m_SpawnChunkCount = chunk.ChunkCount;
		m_NewSpawnKey = chunk.FileKey;
		m_NewSpawnRevision = chunk.Revision;
		m_NewSpawnEditable = chunk.Editable;
		m_NewSpawnError = chunk.ErrorKey;
		m_NewGroupsError = chunk.GroupsError;
		array<ref VPPXESpawnRow> chunkRows = new array<ref VPPXESpawnRow>;
		if (chunk.Rows)
		{
			foreach (VPPXESpawnRow row : chunk.Rows)
			{
				if (row)
				{
					chunkRows.Insert(row);
				}
			}
		}

		m_SpawnChunks.Set(chunk.ChunkIdx, chunkRows);
		TryFinish();
	}

	void XE_OnGroupsChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEGroupsChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEGroupsChunk chunk = data.param1;
		if (!chunk || chunk.ReqId != m_ReqId || !m_Pending)
		{
			return;
		}

		m_SentAt = NowMs();
		m_GroupsView.OnChunk(chunk);
		TryFinish();
	}

	void XE_OnEventsChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEEventsChunk> data;
		if (type != CallType.Client)
		{
			return;
		}

		if (!ctx.Read(data))
		{
			return;
		}

		VPPXEEventsChunk chunk = data.param1;
		if (!chunk || chunk.ReqId != m_ReqId || !m_Pending)
		{
			return;
		}

		m_SentAt = NowMs();
		if (chunk.FileCount <= 0)
		{
			m_ExpectedFiles = 0;
			m_FilesDone = true;
			TryFinish();
			return;
		}

		m_ExpectedFiles = chunk.FileCount;
		VPPXEEvtIncoming incoming = m_Incoming.Get(chunk.FileKey);
		if (!incoming)
		{
			incoming = new VPPXEEvtIncoming();
			incoming.Key = chunk.FileKey;
			m_Incoming.Set(chunk.FileKey, incoming);
		}

		incoming.Revision = chunk.Revision;
		incoming.Editable = chunk.Editable;
		incoming.ErrorKey = chunk.ErrorKey;
		incoming.ChunkCount = chunk.ChunkCount;
		array<ref VPPXEEventRow> chunkRows = new array<ref VPPXEEventRow>;
		if (chunk.Rows)
		{
			foreach (VPPXEEventRow row : chunk.Rows)
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
			VPPXEEvtIncoming check = m_Incoming.GetElement(i);
			if (check.Got() < check.ChunkCount)
			{
				return;
			}
		}

		m_FilesDone = true;
		TryFinish();
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
		m_Incoming = new map<string, ref VPPXEEvtIncoming>;
		m_ExpectedFiles = 0;
		m_FilesDone = false;
		m_SpawnChunkCount = -1;
		m_SpawnChunks = new map<int, ref array<ref VPPXESpawnRow>>;
		m_GroupsView.BeginLoad();
		m_NewSpawnKey = "";
		m_NewSpawnError = "";
		m_NewGroupsError = "";
		SetStatus(Tr("#VSTR_XMLE_STATUS_LOADING"), false);
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetEvents", new Param1<int>(m_ReqId), true, null);
	}

	// Every events file, every cfgeventspawns.xml and every cfgeventgroups.xml chunk arrived.
	protected void TryFinish()
	{
		if (!m_FilesDone || m_SpawnChunkCount < 0 || m_SpawnChunks.Count() < m_SpawnChunkCount || !m_GroupsView.IsLoadDone())
		{
			return;
		}

		FinishList();
	}

	// Files with unsaved edits keep them (flagged when the server copy changed meanwhile) unless a save or revert
	// asked for a fresh copy; every other file takes the server rows.
	protected void FinishList()
	{
		string keepName = "";
		VPPXEEvtWork keepWork = SelectedWork();
		if (keepWork)
		{
			keepName = keepWork.Row.Name;
		}

		m_Pending = false;
		m_Loaded = true;
		m_LoadedSig = CurrentSig();
		FinishSpawns();
		m_GroupsView.FinishLoad();
		map<string, ref VPPXEEvtFile> fresh = new map<string, ref VPPXEEvtFile>;
		for (int i = 0; i < m_Incoming.Count(); i++)
		{
			VPPXEEvtIncoming incoming = m_Incoming.GetElement(i);
			VPPXEEvtFile kept = m_Files.Get(incoming.Key);
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

			VPPXEEvtFile file = new VPPXEEvtFile();
			file.Key = incoming.Key;
			file.Revision = incoming.Revision;
			file.Editable = incoming.Editable;
			file.ErrorKey = incoming.ErrorKey;
			array<ref VPPXEEventRow> rows = new array<ref VPPXEEventRow>;
			incoming.JoinRows(rows);
			int rowCount = rows.Count();
			for (int r = 0; r < rowCount; r++)
			{
				VPPXEEvtWork work = new VPPXEEvtWork();
				work.OrigIndex = r;
				work.Orig = new VPPXEEventRow();
				work.Orig.CopyFrom(rows[r]);
				work.Row.CopyFrom(rows[r]);
				file.Work.Insert(work);
			}

			fresh.Set(incoming.Key, file);
		}

		m_Files = fresh;
		m_ForceFresh.Clear();
		m_Incoming = new map<string, ref VPPXEEvtIncoming>;
		m_MergeStale = true;
		RebuildFileKeys();
		if (m_WantFile != "" && m_Files.Contains(m_WantFile))
		{
			m_FileKey = m_WantFile;
			m_WantFile = "";
			m_SelWork = -1;
		}

		if (!m_Files.Contains(m_FileKey))
		{
			m_FileKey = "";
			if (m_FileKeys.Count() > 0)
			{
				m_FileKey = m_FileKeys[0];
			}

			m_SelWork = -1;
		}

		ReselectByName(keepName);
		SetStatus("", false);
		RebuildAll();
	}

	// After a reload the indexes can shift (adds and deletes were saved): select the event of that name again.
	protected void ReselectByName(string eventName)
	{
		VPPXEEvtFile file = CurrentFile();
		if (!file || eventName == "")
		{
			return;
		}

		VPPXEEvtWork current = SelectedWork();
		if (current && current.Row.Name == eventName)
		{
			return;
		}

		m_SelWork = -1;
		m_ChildSel = -1;
		for (int i = 0; i < file.Work.Count(); i++)
		{
			VPPXEEvtWork work = file.Work[i];
			if (!work.Deleted && work.Row.Name == eventName)
			{
				m_SelWork = i;
				return;
			}
		}
	}

	// Events files in session order (= the server's load order).
	protected void RebuildFileKeys()
	{
		m_FileKeys.Clear();
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (fileInfo && fileInfo.Kind == VPPXEFileKind.EVENTS)
			{
				m_FileKeys.Insert(fileInfo.Key);
			}
		}
	}

	// Events files, cfgeventspawns.xml (its positions feed the checks) and cfgeventgroups.xml.
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
			if (!fileInfo || (fileInfo.Kind != VPPXEFileKind.EVENTS && fileInfo.Kind != VPPXEFileKind.EVENTSPAWNS && fileInfo.Kind != VPPXEFileKind.EVENTGROUPS))
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
			if (fileInfo && fileInfo.Kind == VPPXEFileKind.EVENTSPAWNS && m_Spawns && m_Spawns.Key == fileInfo.Key)
			{
				if (!m_SpawnSavePending && m_Spawns.DirtyCount() > 0 && m_Spawns.Revision != fileInfo.Revision)
				{
					m_Spawns.ServerChanged = true;
				}

				continue;
			}

			if (fileInfo && fileInfo.Kind == VPPXEFileKind.EVENTGROUPS)
			{
				m_GroupsView.FlagServerChanged(fileInfo.Key, fileInfo.Revision, m_GroupSavePending);
				continue;
			}

			if (!fileInfo || fileInfo.Kind != VPPXEFileKind.EVENTS)
			{
				continue;
			}

			VPPXEEvtFile file = m_Files.Get(fileInfo.Key);
			bool ownSave = m_EvtSavePending && fileInfo.Key == m_SaveFile;
			if (file && !ownSave && file.DirtyCount() > 0 && file.Revision != fileInfo.Revision)
			{
				file.ServerChanged = true;
			}
		}
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT || !w)
		{
			return false;
		}

		if (w == m_BtnViewForm || w == m_BtnViewSpawns || w == m_BtnViewGroups)
		{
			int view = VIEW_FORM;
			if (w == m_BtnViewSpawns)
			{
				view = VIEW_SPAWNS;
			}
			else if (w == m_BtnViewGroups)
			{
				view = VIEW_GROUPS;
			}

			SwitchView(view);
			return true;
		}

		if (m_ViewGroups && m_GroupsView.OnClick(w))
		{
			return true;
		}

		if (w == m_BtnSpawnGroup)
		{
			OpenSpawnGroup();
			return true;
		}

		if (w == m_BtnZone)
		{
			ToggleZone();
			return true;
		}

		if (w == m_BtnSpawnAdd)
		{
			ToggleAddMode();
			return true;
		}

		if (w == m_BtnSpawnHere)
		{
			AddSpawnHere();
			return true;
		}

		if (w == m_BtnSpawnRemove)
		{
			RemoveSpawn();
			return true;
		}

		int posIdx = m_BtnPos.Find(ButtonWidget.Cast(w));
		if (posIdx >= 0)
		{
			PickPosition(posIdx);
			return true;
		}

		int limIdx = m_BtnLim.Find(ButtonWidget.Cast(w));
		if (limIdx >= 0)
		{
			PickLimit(limIdx);
			return true;
		}

		int flagIdx = m_BtnFlag.Find(ButtonWidget.Cast(w));
		if (flagIdx >= 0)
		{
			ToggleFlag(flagIdx);
			return true;
		}

		if (w == m_BtnActive)
		{
			ToggleActive();
			return true;
		}

		if (w == m_BtnChildSpawnSec)
		{
			ToggleChildBool(true);
			return true;
		}

		if (w == m_BtnChildTrace)
		{
			ToggleChildBool(false);
			return true;
		}

		if (w == m_BtnChildAdd)
		{
			AddChild();
			return true;
		}

		if (w == m_BtnChildRemove)
		{
			RemoveChild();
			return true;
		}

		if (w == m_BtnAdd)
		{
			StartAdd(-1);
			return true;
		}

		if (w == m_BtnDup)
		{
			if (SelectedWork())
			{
				StartAdd(m_SelWork);
			}

			return true;
		}

		if (w == m_BtnDelete)
		{
			DeleteSelected();
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

	void OnFilePicked(int index)
	{
		m_FileDD.Close();
		if (index < 0 || index >= m_FileKeys.Count())
		{
			RebuildFileDropdown();
			return;
		}

		SwitchFile(m_FileKeys[index]);
	}

	void OnFilterPicked(int index)
	{
		m_FilterDD.Close();
		if (index < 0 || index >= FILTER_COUNT)
		{
			return;
		}

		m_FilterIdx = index;
		m_FilterDD.SetText(FilterLabel(index));
		m_FilterDD.SetIndex(index);
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	protected void SwitchFile(string fileKey)
	{
		m_FileKey = fileKey;
		m_SelWork = -1;
		m_ListSel = -1;
		m_ChildSel = -1;
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
	}

	// ---------------------------------------------------------------- editing

	protected bool CanEdit()
	{
		VPPXEEvtFile file = CurrentFile();
		if (!file || !file.Editable || m_Saving)
		{
			return false;
		}

		// a save or revert reloads this file fresh: an edit made meanwhile would be dropped
		if (m_Pending && m_ForceFresh.Contains(m_FileKey))
		{
			return false;
		}

		return m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_EVENTS);
	}

	// ADD (copyWork -1) and DUPLICATE ask for the name first.
	protected void StartAdd(int copyWork)
	{
		VPPXEEvtFile file = CurrentFile();
		if (!file || !CanEdit())
		{
			return;
		}

		m_PendCopyWork = copyWork;
		string body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_ADD_BODY"), FileLabel(file.Key));
		string title = "#VSTR_XMLE_EVT_DLG_ADD_TITLE";
		if (copyWork >= 0)
		{
			VPPXEEvtWork source = file.Work[copyWork];
			body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_DUP_BODY"), source.Row.Name);
			title = "#VSTR_XMLE_EVT_DLG_DUP_TITLE";
		}

		m_Owner.OpenConfirm(title, body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnAddName", true);
	}

	void OnAddName(int result, string input)
	{
		VPPXEEvtFile file = CurrentFile();
		if (result != DIAGRESULT.OK || !file || !CanEdit())
		{
			m_PendCopyWork = -1;
			return;
		}

		string newName = VPPXmlText.TrimWs(input);
		if (!VPPXmlText.IsValidClassName(newName))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_ERR_NAME_INVALID"), newName));
			m_PendCopyWork = -1;
			return;
		}

		if (NameInFile(file, newName, -1))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_EVT_ERR_NAME_TAKEN"), newName));
			m_PendCopyWork = -1;
			return;
		}

		VPPXEEvtWork work = new VPPXEEvtWork();
		if (m_PendCopyWork >= 0 && m_PendCopyWork < file.Work.Count())
		{
			work.Row.CopyFrom(file.Work[m_PendCopyWork].Row);
		}
		else
		{
			VPPXEEventRules.ApplyTemplate(work.Row, VPPXEEventRules.KindOf(newName));
		}

		m_PendCopyWork = -1;
		work.Row.Name = newName;
		work.Row.Line = 0;
		work.Row.ExtraCount = 0;
		file.Work.Insert(work);
		m_SelWork = file.Work.Count() - 1;
		m_ChildSel = -1;
		m_FilterIdx = FILTER_ALL;
		m_FilterDD.SetText(FilterLabel(FILTER_ALL));
		m_MergeStale = true;
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	protected bool NameInFile(VPPXEEvtFile file, string eventName, int skipWork)
	{
		for (int i = 0; i < file.Work.Count(); i++)
		{
			VPPXEEvtWork work = file.Work[i];
			if (i != skipWork && !work.Deleted && work.Row.Name == eventName)
			{
				return true;
			}
		}

		return false;
	}

	// An event that came from the file is marked deleted (REVERT brings it back); one added here is dropped.
	protected void DeleteSelected()
	{
		VPPXEEvtFile file = CurrentFile();
		VPPXEEvtWork work = SelectedWork();
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

		m_SelWork = -1;
		m_ChildSel = -1;
		m_MergeStale = true;
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// Clicking the lit chip clears the element only where an earlier file defines the event (it then inherits).
	protected void PickPosition(int index)
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		string value = PositionValue(index);
		VPPXEEventRow row = work.Row;
		if (row.Has(VPPXEEventField.POSITION) && row.Position == value)
		{
			if (!HasEarlierDefinition(row.Name))
			{
				return;
			}

			row.SetPresent(VPPXEEventField.POSITION, false);
			row.Position = "";
		}
		else
		{
			row.Position = value;
			row.SetPresent(VPPXEEventField.POSITION, true);
		}

		AfterRowChange(true);
	}

	protected void PickLimit(int index)
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		string value = LimitValue(index);
		VPPXEEventRow row = work.Row;
		if (row.Has(VPPXEEventField.LIMIT) && row.Limit == value)
		{
			if (!HasEarlierDefinition(row.Name))
			{
				return;
			}

			row.SetPresent(VPPXEEventField.LIMIT, false);
			row.Limit = "";
		}
		else
		{
			row.Limit = value;
			row.SetPresent(VPPXEEventField.LIMIT, true);
		}

		AfterRowChange(true);
	}

	protected void ToggleFlag(int index)
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		int bit = FlagBit(index);
		VPPXEEventRow row = work.Row;
		if ((row.Flags & bit) != 0)
		{
			row.Flags = row.Flags & ~bit;
		}
		else
		{
			row.Flags = row.Flags | bit;
		}

		row.SetPresent(VPPXEEventField.FLAGS, true);
		AfterRowChange(true);
	}

	// Off writes <active>0</active>; on goes back to what the file had (1, or no element).
	protected void ToggleActive()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		VPPXEEventRow row = work.Row;
		if (IsActiveRow(row))
		{
			row.Active = 0;
			row.SetPresent(VPPXEEventField.ACTIVE, true);
		}
		else
		{
			row.Active = 1;
			bool keepElement = true;
			if (work.Orig && !work.Orig.Has(VPPXEEventField.ACTIVE))
			{
				keepElement = false;
			}

			row.SetPresent(VPPXEEventField.ACTIVE, keepElement);
		}

		AfterRowChange(true);
	}

	// spawnsecondary (default on) and trace (default off): a value equal to the default is not written.
	protected void ToggleChildBool(bool spawnSecondary)
	{
		VPPXEEventChild child = SelectedChild();
		if (!child || !CanEdit())
		{
			return;
		}

		if (spawnSecondary)
		{
			if (child.SpawnSecondary == 0)
			{
				child.SpawnSecondary = -1;
			}
			else
			{
				child.SpawnSecondary = 0;
			}
		}
		else
		{
			if (child.Trace == 1)
			{
				child.Trace = -1;
			}
			else
			{
				child.Trace = 1;
			}
		}

		AfterChildChange();
	}

	protected void AddChild()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit() || work.Row.Children.Count() >= VPPXEEventRules.MAX_CHILDREN)
		{
			return;
		}

		VPPXEEventChild child = new VPPXEEventChild();
		child.Min = 1;
		child.Max = 1;
		work.Row.Children.Insert(child);
		work.Row.SetPresent(VPPXEEventField.CHILDREN, true);
		m_ChildSel = work.Row.Children.Count() - 1;
		m_MergeStale = true;
		RebuildChildList();
		LoadChildForm();
		if (m_InputChildType)
		{
			SetFocus(m_InputChildType);
		}

		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
	}

	protected void RemoveChild()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || !CanEdit() || !SelectedChild())
		{
			return;
		}

		work.Row.Children.RemoveOrdered(m_ChildSel);
		if (work.Row.Children.Count() == 0 && work.Orig && !work.Orig.Has(VPPXEEventField.CHILDREN))
		{
			work.Row.SetPresent(VPPXEEventField.CHILDREN, false);
		}

		m_ChildSel = -1;
		m_MergeStale = true;
		RebuildChildList();
		LoadChildForm();
		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
	}

	protected void AfterRowChange(bool reloadForm)
	{
		m_MergeStale = true;
		if (reloadForm)
		{
			LoadForm();
		}
		else
		{
			UpdateDetails();
		}

		RefreshSelectedRow();
		UpdateActionBar();
	}

	protected void AfterChildChange()
	{
		m_MergeStale = true;
		RefreshChildRow();
		UpdateChildDetails();
		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
	}

	// ---------------------------------------------------------------- form

	// Fills the form from the selected event (or shows the empty state).
	protected void LoadForm()
	{
		VPPXEEvtWork work = SelectedWork();
		ShowEditor(work != null);
		if (!work)
		{
			UpdateEmptyText();
			if (m_ViewGroups)
			{
				m_GroupsView.Refresh();
			}

			return;
		}

		m_Loading = true;
		VPPXEEventRow row = work.Row;
		m_InputName.SetText(row.Name);
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			EditBoxWidget input = m_InputNum[i];
			if (row.Has(field))
			{
				int value = row.GetNumber(field);
				input.SetText(value.ToString());
			}
			else
			{
				input.SetText("");
			}
		}

		if (row.Has(VPPXEEventField.SECONDARY))
		{
			m_InputSecondary.SetText(row.Secondary);
		}
		else
		{
			m_InputSecondary.SetText("");
		}

		if (m_ChildSel >= row.Children.Count())
		{
			m_ChildSel = -1;
		}

		RebuildChildList();
		LoadChildFormInputs();
		RememberInputs();
		m_Loading = false;
		// the inputs now show the row: an error of an earlier unreadable input is gone
		work.InputError = "";
		UpdateDetails();
		if (work != m_SpawnSelWork)
		{
			m_SpawnSelWork = work;
			m_SpawnSel = -1;
			m_SpawnAddMode = false;
			m_SpawnFitPending = true;
		}

		CaptureRenameEntries(work);
		RefreshSpawnView(true);
		if (m_ViewGroups)
		{
			m_GroupsView.Refresh();
		}
	}

	protected void LoadChildForm()
	{
		m_Loading = true;
		LoadChildFormInputs();
		RememberInputs();
		m_Loading = false;
		VPPXEEvtWork work = SelectedWork();
		if (work && CanEdit())
		{
			// the child inputs changed: validate the whole form again (an event input may still be unreadable)
			ApplyForm(work);
		}

		UpdateChildDetails();
	}

	protected void LoadChildFormInputs()
	{
		VPPXEEventChild child = SelectedChild();
		if (!child)
		{
			m_InputChildType.SetText("");
			m_InputChildMin.SetText("");
			m_InputChildMax.SetText("");
			m_InputChildLootMin.SetText("");
			m_InputChildLootMax.SetText("");
			m_InputChildDeloot.SetText("");
			return;
		}

		m_InputChildType.SetText(child.Type);
		m_InputChildMin.SetText(child.Min.ToString());
		m_InputChildMax.SetText(child.Max.ToString());
		m_InputChildLootMin.SetText(child.LootMin.ToString());
		m_InputChildLootMax.SetText(child.LootMax.ToString());
		if (child.Deloot == -1)
		{
			m_InputChildDeloot.SetText("");
		}
		else
		{
			m_InputChildDeloot.SetText(child.Deloot.ToString());
		}
	}

	protected void RememberInputs()
	{
		m_LastName = m_InputName.GetText();
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			EditBoxWidget input = m_InputNum[i];
			m_LastNum.Set(i, input.GetText());
		}

		m_LastSecondary = m_InputSecondary.GetText();
		m_LastChildType = m_InputChildType.GetText();
		m_LastChildMin = m_InputChildMin.GetText();
		m_LastChildMax = m_InputChildMax.GetText();
		m_LastChildLootMin = m_InputChildLootMin.GetText();
		m_LastChildLootMax = m_InputChildLootMax.GetText();
		m_LastChildDeloot = m_InputChildDeloot.GetText();
	}

	protected bool InputsChanged()
	{
		if (m_InputName.GetText() != m_LastName || m_InputSecondary.GetText() != m_LastSecondary)
		{
			return true;
		}

		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			EditBoxWidget input = m_InputNum[i];
			if (input.GetText() != m_LastNum[i])
			{
				return true;
			}
		}

		if (m_InputChildType.GetText() != m_LastChildType || m_InputChildMin.GetText() != m_LastChildMin)
		{
			return true;
		}

		if (m_InputChildMax.GetText() != m_LastChildMax || m_InputChildLootMin.GetText() != m_LastChildLootMin)
		{
			return true;
		}

		return m_InputChildLootMax.GetText() != m_LastChildLootMax || m_InputChildDeloot.GetText() != m_LastChildDeloot;
	}

	protected void PollInputs()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || m_Loading || !m_InputName)
		{
			return;
		}

		if (!InputsChanged())
		{
			return;
		}

		bool childChanged = m_InputChildType.GetText() != m_LastChildType;
		RememberInputs();
		if (!CanEdit())
		{
			LoadForm();
			return;
		}

		string oldName = work.Row.Name;
		ApplyForm(work);
		m_MergeStale = true;
		if (work.Row.Name != oldName)
		{
			SyncSpawnRename(work.Row.Name);
			UpdateViewChips();
		}

		if (childChanged || SelectedChild())
		{
			RefreshChildRow();
			UpdateChildDetails();
		}

		RefreshSelectedRow();
		UpdateDetails();
		UpdateActionBar();
	}

	// Form -> row. An unreadable field keeps the row's value and sets InputError (SAVE is refused while set). An empty
	// number or secondary is not written (the event inherits it, or the server default 0 applies).
	protected void ApplyForm(VPPXEEvtWork work)
	{
		VPPXEEventRow row = work.Row;
		work.InputError = "";
		string nameText = m_InputName.GetText();
		row.Name = nameText.Trim();
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			EditBoxWidget input = m_InputNum[i];
			string numberText = input.GetText();
			numberText = numberText.Trim();
			if (numberText == "")
			{
				row.SetPresent(field, false);
				row.SetNumber(field, 0);
			}
			else if (VPPXEEventRules.IsNumberText(numberText))
			{
				row.SetNumber(field, VPPXEEventRules.NumberOf(numberText));
				row.SetPresent(field, true);
			}
			else
			{
				work.InputError = "#VSTR_XMLE_EVT_ERR_NUMBER";
			}
		}

		string secondaryText = m_InputSecondary.GetText();
		secondaryText = secondaryText.Trim();
		row.Secondary = secondaryText;
		row.SetPresent(VPPXEEventField.SECONDARY, secondaryText != "");
		VPPXEEventChild child = SelectedChild();
		if (!child)
		{
			return;
		}

		string typeText = m_InputChildType.GetText();
		child.Type = typeText.Trim();
		if (!ApplyChildNumber(child, m_InputChildMin.GetText(), 0) || !ApplyChildNumber(child, m_InputChildMax.GetText(), 1))
		{
			work.InputError = "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
		}

		if (!ApplyChildNumber(child, m_InputChildLootMin.GetText(), 2) || !ApplyChildNumber(child, m_InputChildLootMax.GetText(), 3))
		{
			work.InputError = "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
		}

		string delootText = m_InputChildDeloot.GetText();
		delootText = delootText.Trim();
		if (delootText == "" || delootText == "-1")
		{
			child.Deloot = -1;
		}
		else if (VPPXEEventRules.IsNumberText(delootText))
		{
			child.Deloot = VPPXEEventRules.NumberOf(delootText);
		}
		else
		{
			work.InputError = "#VSTR_XMLE_EVT_ERR_CHILD_NUMBER";
		}
	}

	// slot 0 min, 1 max, 2 lootmin, 3 lootmax; "" = 0. False when unreadable (the value stays).
	protected bool ApplyChildNumber(VPPXEEventChild child, string text, int slot)
	{
		string trimmed = text.Trim();
		int value = 0;
		if (trimmed != "")
		{
			if (!VPPXEEventRules.IsNumberText(trimmed))
			{
				return false;
			}

			value = VPPXEEventRules.NumberOf(trimmed);
		}

		if (slot == 0)
		{
			child.Min = value;
		}
		else if (slot == 1)
		{
			child.Max = value;
		}
		else if (slot == 2)
		{
			child.LootMin = value;
		}
		else
		{
			child.LootMax = value;
		}

		return true;
	}

	// ---------------------------------------------------------------- save and revert

	// Unsaved edits of the selected events file SAVE writes (0 when it cannot be edited).
	protected int EventsDirtyToSave()
	{
		VPPXEEvtFile file = CurrentFile();
		if (!file || !CanEdit())
		{
			return 0;
		}

		return file.DirtyCount();
	}

	protected int SpawnsDirtyToSave()
	{
		if (!CanEditSpawns())
		{
			return 0;
		}

		return SpawnDirtyCount();
	}

	// Checks every edited event of the file, every edited spawn entry and group, then asks once for every file.
	protected void StartSave()
	{
		VPPXEEvtFile file = CurrentFile();
		int dirty = EventsDirtyToSave();
		int spawnDirty = SpawnsDirtyToSave();
		int groupDirty = m_GroupsView.DirtyToSave();
		if (dirty + spawnDirty + groupDirty == 0)
		{
			return;
		}

		if (dirty > 0 && !CheckEventsBeforeSave(file))
		{
			return;
		}

		if (spawnDirty > 0 && !CheckSpawnsBeforeSave())
		{
			return;
		}

		if (groupDirty > 0 && !m_GroupsView.CheckBeforeSave())
		{
			SwitchView(VIEW_GROUPS);
			return;
		}

		string body = "";
		if (groupDirty > 0)
		{
			body = Tr("#VSTR_XMLE_GRP_DLG_SAVE_BODY") + "\n" + ChangeListText(dirty, spawnDirty, groupDirty);
		}
		else if (dirty > 0 && spawnDirty > 0)
		{
			body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_SAVE_BOTH_BODY"), dirty, FileLabel(file.Key), spawnDirty);
		}
		else if (dirty > 0)
		{
			body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_SAVE_BODY"), dirty, FileLabel(file.Key));
		}
		else
		{
			body = string.Format(Tr("#VSTR_XMLE_SPW_DLG_SAVE_BODY"), spawnDirty);
		}

		m_Owner.OpenConfirm("#VSTR_XMLE_EVT_DLG_SAVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmSave", false);
	}

	protected bool CheckEventsBeforeSave(VPPXEEvtFile file)
	{
		int workCount = file.Work.Count();
		for (int i = 0; i < workCount; i++)
		{
			VPPXEEvtWork work = file.Work[i];
			if (work.Deleted || !work.IsDirty())
			{
				continue;
			}

			string problem = work.InputError;
			if (problem == "")
			{
				problem = VPPXEEventRules.CheckRow(work.Row);
			}

			bool nameChanged = !work.Orig || work.Orig.Name != work.Row.Name;
			if (problem == "" && nameChanged && NameInFile(file, work.Row.Name, i))
			{
				problem = "#VSTR_XMLE_EVT_ERR_NAME_TAKEN";
			}

			if (problem != "")
			{
				m_SelWork = i;
				m_FilterIdx = FILTER_ALL;
				m_FilterDD.SetText(FilterLabel(FILTER_ALL));
				SwitchView(VIEW_FORM);
				RebuildList();
				LoadForm();
				m_Owner.NotifyError(string.Format(Tr(problem), work.Row.Name));
				return false;
			}
		}

		return true;
	}

	// A spawn entry that cannot be written: its event is selected (when it is in this file) and the SPAWNS view shown.
	protected bool CheckSpawnsBeforeSave()
	{
		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || !work.IsDirty())
			{
				continue;
			}

			string problem = VPPXESpawnRules.CheckRow(work.Row, work.Orig);
			if (problem == "")
			{
				continue;
			}

			m_FilterIdx = FILTER_ALL;
			m_FilterDD.SetText(FilterLabel(FILTER_ALL));

			VPPXEEvtFile file = CurrentFile();
			if (file)
			{
				for (int i = 0; i < file.Work.Count(); i++)
				{
					VPPXEEvtWork eventWork = file.Work[i];
					if (!eventWork.Deleted && eventWork.Row.Name == work.Row.Name)
					{
						m_SelWork = i;
						RebuildList();
						LoadForm();
						SwitchView(VIEW_SPAWNS);
						break;
					}
				}
			}

			m_Owner.NotifyError(work.Row.Name + ": " + Tr(problem));
			return false;
		}

		return true;
	}

	void OnConfirmSave(int result, string input)
	{
		VPPXEEvtFile file = CurrentFile();
		if (result != DIAGRESULT.YES)
		{
			return;
		}

		int dirty = EventsDirtyToSave();
		int spawnDirty = SpawnsDirtyToSave();
		int groupDirty = m_GroupsView.DirtyToSave();
		if (dirty + spawnDirty + groupDirty == 0)
		{
			return;
		}

		m_ReloadAfterSave = false;
		if (dirty > 0)
		{
			SendEventsSave(file);
		}

		if (spawnDirty > 0)
		{
			SendSpawnSave();
		}

		if (groupDirty > 0)
		{
			m_GroupSaveReqId = m_GroupsView.SendSave();
			m_GroupSavePending = m_GroupSaveReqId > 0;
		}

		m_Saving = m_EvtSavePending || m_SpawnSavePending || m_GroupSavePending;
		if (!m_Saving)
		{
			return;
		}

		m_SaveSentAt = NowMs();
		SetStatus(Tr("#VSTR_XMLE_STATUS_SAVING"), false);
		UpdateActionBar();
		UpdateDetails();
		UpdateSpawnDetails();
		m_GroupsView.Refresh();
	}

	// "file: n change(s)" lines of the files a save or revert covers.
	protected string ChangeListText(int dirty, int spawnDirty, int groupDirty)
	{
		string text = "";
		VPPXEEvtFile file = CurrentFile();
		if (dirty > 0 && file)
		{
			text = AppendChangeLine(text, FileLabel(file.Key), dirty);
		}

		if (spawnDirty > 0 && m_Spawns)
		{
			text = AppendChangeLine(text, FileLabel(m_Spawns.Key), spawnDirty);
		}

		if (groupDirty > 0)
		{
			text = AppendChangeLine(text, FileLabel(m_GroupsView.FileKey()), groupDirty);
		}

		return text;
	}

	protected string AppendChangeLine(string text, string label, int count)
	{
		string line = string.Format(Tr("#VSTR_XMLE_GRP_DLG_LINE"), label, count);
		if (text == "")
		{
			return line;
		}

		return text + "\n" + line;
	}

	protected void SendEventsSave(VPPXEEvtFile file)
	{
		array<ref VPPXEEventEdit> edits = new array<ref VPPXEEventEdit>;
		foreach (VPPXEEvtWork work : file.Work)
		{
			if (!work.IsDirty())
			{
				continue;
			}

			VPPXEEventEdit edit = new VPPXEEventEdit();
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
		array<ref VPPXEEventsSave> parts = new array<ref VPPXEEventsSave>;
		VPPXEEventsSave part = null;
		int partBytes = 0;
		foreach (VPPXEEventEdit queued : edits)
		{
			int size = VPPXEEventRules.EstimateRow(queued.Row) + 16;
			if (!part || part.Edits.Count() >= VPPXEEventRules.EDITS_PER_PART || partBytes + size > VPPXEEventRules.CHUNK_BYTES)
			{
				part = new VPPXEEventsSave();
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

		m_EvtSavePending = true;
		m_SaveFile = file.Key;
		int partCount = parts.Count();
		foreach (VPPXEEventsSave sendPart : parts)
		{
			sendPart.PartCount = partCount;
			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveEvents", new Param1<ref VPPXEEventsSave>(sendPart), true, null);
		}
	}

	protected void StartRevert()
	{
		VPPXEEvtFile file = CurrentFile();
		if (m_Saving)
		{
			return;
		}

		int dirty = 0;
		bool fileChanged = false;
		if (file)
		{
			dirty = file.DirtyCount();
			fileChanged = file.ServerChanged;
		}

		int spawnDirty = SpawnDirtyCount();
		int groupDirty = m_GroupsView.DirtyCount();
		bool spawnsChanged = m_Spawns && m_Spawns.ServerChanged;
		bool groupsChanged = m_GroupsView.IsServerChanged();
		if (dirty + spawnDirty + groupDirty == 0 && !fileChanged && !spawnsChanged && !groupsChanged)
		{
			return;
		}

		string body = "";
		if (groupDirty > 0 || groupsChanged || !file)
		{
			body = Tr("#VSTR_XMLE_GRP_DLG_REVERT_BODY") + "\n" + ChangeListText(dirty, spawnDirty, groupDirty);
		}
		else if (spawnDirty > 0 || spawnsChanged)
		{
			body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_REVERT_BOTH_BODY"), dirty + spawnDirty, FileLabel(file.Key));
		}
		else
		{
			body = string.Format(Tr("#VSTR_XMLE_EVT_DLG_REVERT_BODY"), dirty, FileLabel(file.Key));
		}

		m_Owner.OpenConfirm("#VSTR_XMLE_EVT_DLG_REVERT_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmRevert", false);
	}

	void OnConfirmRevert(int result, string input)
	{
		VPPXEEvtFile file = CurrentFile();
		if (result != DIAGRESULT.YES)
		{
			return;
		}

		if (file)
		{
			m_ForceFresh.Set(file.Key, true);
		}

		if (SpawnDirtyCount() > 0 || (m_Spawns && m_Spawns.ServerChanged))
		{
			m_SpawnForceFresh = true;
		}

		if (m_GroupsView.DirtyCount() > 0 || m_GroupsView.IsServerChanged())
		{
			m_GroupsView.ForceFresh();
		}

		m_SelWork = -1;
		m_ChildSel = -1;
		m_SpawnSel = -1;
		RequestList();
	}

	// ---------------------------------------------------------------- merged view and checks

	// The merged events of every loaded file (working copies, so unsaved edits count), rebuilt when something changed.
	protected VPPXEEventMerge Merge()
	{
		if (!m_MergeStale)
		{
			return m_Merge;
		}

		m_Merge.Clear();
		m_NameCounts.Clear();
		foreach (string fileKey : m_FileKeys)
		{
			VPPXEEvtFile file = m_Files.Get(fileKey);
			if (!file)
			{
				continue;
			}

			foreach (VPPXEEvtWork work : file.Work)
			{
				if (!work.Deleted)
				{
					m_Merge.Apply(fileKey, work.Row);
					string countKey = fileKey + "|" + work.Row.Name;
					m_NameCounts.Set(countKey, m_NameCounts.Get(countKey) + 1);
				}
			}
		}

		m_MergeStale = false;
		return m_Merge;
	}

	// Every check of one event of the current file: hard problems first, then the server's rules.
	protected void BuildNotes(VPPXEEvtFile file, int workIdx, array<ref VPPXEEventNote> outNotes)
	{
		outNotes.Clear();
		VPPXEEvtWork work = file.Work[workIdx];
		VPPXEEventRow row = work.Row;
		VPPXEEventMerge merge = Merge();
		if (work.InputError != "")
		{
			outNotes.Insert(new VPPXEEventNote(work.InputError, row.Name, "", VPPXEEventRules.SEV_DISABLED));
		}

		string rule = VPPXEEventRules.CheckRow(row);
		if (rule != "" && rule != work.InputError)
		{
			outNotes.Insert(new VPPXEEventNote(rule, row.Name, "", VPPXEEventRules.SEV_DISABLED));
		}

		if (m_NameCounts.Get(file.Key + "|" + row.Name) > 1)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_DUP_IN_FILE", "", "", VPPXEEventRules.SEV_WARNING));
		}

		VPPXEEventRow eff = merge.Effective.Get(row.Name);
		string removedBy = "";
		merge.RemovedBy.Find(row.Name, removedBy);
		if (row.Has(VPPXEEventField.ACTIVE) && row.Active != 1)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_REMOVED", row.Active.ToString(), "", VPPXEEventRules.SEV_WARNING));
		}
		else if (removedBy != "" && removedBy != file.Key)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_REMOVED_BY", FileLabel(removedBy), "", VPPXEEventRules.SEV_DISABLED));
		}

		AddOverrideNotes(file.Key, row, eff, outNotes);
		int posCount = -1;
		int groupPosCount = 0;
		if (SpawnsKnown())
		{
			posCount = PosCountOf(row.Name);
			groupPosCount = GroupPosCountOf(row.Name);
		}

		VPPXEEventRules.Check(row, WithoutUnknownChildren(eff), posCount, groupPosCount, outNotes);
		AddSecondaryNotes(row, merge, outNotes);
		AddChildTypeNotes(row, outNotes);
		int kind = VPPXEEventRules.KindOf(row.Name);
		bool needsPositions = kind == VPPXEEventRules.KIND_VEHICLE || kind == VPPXEEventRules.KIND_STATIC || kind == VPPXEEventRules.KIND_ITEM;
		if (!SpawnsKnown() && needsPositions)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_SPAWNS_UNKNOWN", "", "", VPPXEEventRules.SEV_NOTE));
		}

		if (row.ExtraCount > 0)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_EXTRA", row.ExtraCount.ToString(), "", VPPXEEventRules.SEV_NOTE));
		}
	}

	// cfgeventspawns.xml was read, or it does not exist (then the server has no positions either).
	protected bool SpawnsKnown()
	{
		if (!m_Spawns)
		{
			return false;
		}

		return m_Spawns.ErrorKey == "" || m_Spawns.ErrorKey == "#VSTR_XMLE_STATE_MISSING";
	}

	// The merged event as the server checks it: children of types it cannot create are skipped before the checks.
	protected VPPXEEventRow WithoutUnknownChildren(VPPXEEventRow eff)
	{
		if (!eff)
		{
			return null;
		}

		bool anyUnknown = false;
		foreach (VPPXEEventChild child : eff.Children)
		{
			if (IsUnknownType(child.Type))
			{
				anyUnknown = true;
				break;
			}
		}

		if (!anyUnknown)
		{
			return eff;
		}

		VPPXEEventRow checkedRow = new VPPXEEventRow();
		checkedRow.CopyFrom(eff);
		for (int i = checkedRow.Children.Count() - 1; i >= 0; i--)
		{
			VPPXEEventChild copy = checkedRow.Children[i];
			if (IsUnknownType(copy.Type))
			{
				checkedRow.Children.RemoveOrdered(i);
			}
		}

		return checkedRow;
	}

	// Other files defining the same event: which way the merge goes, flags that fall back to 0, children kept.
	protected void AddOverrideNotes(string fileKey, VPPXEEventRow row, VPPXEEventRow eff, array<ref VPPXEEventNote> outNotes)
	{
		array<string> earlier = new array<string>;
		array<string> later = new array<string>;
		CollectDefFiles(fileKey, row.Name, earlier, later);
		if (earlier.Count() > 0)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_OVERRIDES", JoinLabels(earlier), "", VPPXEEventRules.SEV_NOTE));
			if (!row.Has(VPPXEEventField.FLAGS))
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_FLAGS_RESET", "", "", VPPXEEventRules.SEV_WARNING));
			}

			if (eff && eff.Children.Count() > row.Children.Count())
			{
				int kept = eff.Children.Count() - row.Children.Count();
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILDREN_KEPT", kept.ToString(), "", VPPXEEventRules.SEV_NOTE));
			}
		}

		if (later.Count() > 0)
		{
			outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_OVERRIDDEN", JoinLabels(later), "", VPPXEEventRules.SEV_NOTE));
		}
	}

	protected void CollectDefFiles(string fileKey, string eventName, array<string> earlier, array<string> later)
	{
		array<string> defFiles = Merge().DefFiles.Get(eventName);
		int ownPos = m_FileKeys.Find(fileKey);
		if (!defFiles)
		{
			return;
		}

		foreach (string defKey : defFiles)
		{
			if (defKey == fileKey)
			{
				continue;
			}

			if (m_FileKeys.Find(defKey) < ownPos)
			{
				earlier.Insert(defKey);
			}
			else
			{
				later.Insert(defKey);
			}
		}
	}

	protected bool HasEarlierDefinition(string eventName)
	{
		array<string> earlier = new array<string>;
		array<string> later = new array<string>;
		CollectDefFiles(m_FileKey, eventName, earlier, later);
		return earlier.Count() > 0;
	}

	protected void AddSecondaryNotes(VPPXEEventRow row, VPPXEEventMerge merge, array<ref VPPXEEventNote> outNotes)
	{
		if (!row.Has(VPPXEEventField.SECONDARY) || row.Secondary == "" || merge.Effective.Contains(row.Secondary))
		{
			return;
		}

		string lowerWanted = row.Secondary;
		lowerWanted.ToLower();
		for (int i = 0; i < merge.Effective.Count(); i++)
		{
			string candidate = merge.Effective.GetKey(i);
			string lowerCandidate = candidate;
			lowerCandidate.ToLower();
			if (lowerCandidate == lowerWanted)
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_SECONDARY_CASE", row.Secondary, candidate, VPPXEEventRules.SEV_WARNING));
				return;
			}
		}

		outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_SECONDARY_MISSING", row.Secondary, "", VPPXEEventRules.SEV_WARNING));
	}

	// Child types the server cannot create (not in any types file), once the types index is loaded.
	protected void AddChildTypeNotes(VPPXEEventRow row, array<ref VPPXEEventNote> outNotes)
	{
		foreach (VPPXEEventChild child : row.Children)
		{
			if (child && IsUnknownType(child.Type))
			{
				outNotes.Insert(new VPPXEEventNote("#VSTR_XMLE_EVT_W_CHILD_UNKNOWN", child.Type, "", VPPXEEventRules.SEV_WARNING));
			}
		}
	}

	bool IsUnknownType(string typeName)
	{
		if (typeName == "" || !VPPXmlText.IsValidClassName(typeName) || !m_Owner || !m_Owner.GetModel())
		{
			return false;
		}

		VPPXEClientModel model = m_Owner.GetModel();
		if (model.GetIndexVersion() <= 0 || model.HasPendingIndex())
		{
			return false;
		}

		return !model.HasName(typeName);
	}

	protected string NotesText(array<ref VPPXEEventNote> notes)
	{
		string text = "";
		for (int severity = VPPXEEventRules.SEV_DISABLED; severity >= VPPXEEventRules.SEV_NOTE; severity--)
		{
			foreach (VPPXEEventNote note : notes)
			{
				if (note.Severity != severity)
				{
					continue;
				}

				if (text != "")
				{
					text = text + "\n";
				}

				text = text + "- " + string.Format(Tr(note.Key), note.Arg1, note.Arg2);
			}
		}

		return text;
	}

	protected string JoinLabels(array<string> fileKeys)
	{
		string joined = "";
		foreach (string fileKey : fileKeys)
		{
			if (joined != "")
			{
				joined = joined + ", ";
			}

			joined = joined + FileLabel(fileKey);
		}

		return joined;
	}

	// ---------------------------------------------------------------- list, dropdowns, details

	protected void RebuildAll()
	{
		RebuildFileDropdown();
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	protected void RebuildFileDropdown()
	{
		if (!m_FileDD)
		{
			return;
		}

		m_FileDD.RemoveAllElements();
		string current = "";
		foreach (string fileKey : m_FileKeys)
		{
			string label = FileDropdownLabel(fileKey);
			m_FileDD.AddElement(label);
			if (fileKey == m_FileKey)
			{
				current = label;
			}
		}

		if (current == "")
		{
			current = Tr("#VSTR_XMLE_EVT_NO_FILE_SHORT");
		}

		m_FileDD.SetText(current);
	}

	protected string FileDropdownLabel(string fileKey)
	{
		string label = FileLabel(fileKey);
		VPPXEEvtFile file = m_Files.Get(fileKey);
		if (!file)
		{
			return label;
		}

		int shown = 0;
		foreach (VPPXEEvtWork work : file.Work)
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

	protected void BuildFilterDropdown()
	{
		if (!m_FilterDD)
		{
			return;
		}

		m_FilterDD.RemoveAllElements();
		for (int i = 0; i < FILTER_COUNT; i++)
		{
			m_FilterDD.AddElement(FilterLabel(i));
		}

		m_FilterDD.SetText(FilterLabel(m_FilterIdx));
		m_FilterDD.SetIndex(m_FilterIdx);
	}

	protected string FilterLabel(int filter)
	{
		if (filter == FILTER_ALL)
		{
			return Tr("#VSTR_XMLE_EVT_FILTER_ALL");
		}

		if (filter == FILTER_UNKNOWN)
		{
			return Tr("#VSTR_XMLE_EVT_FILTER_UNKNOWN");
		}

		if (filter == FILTER_OFF)
		{
			return Tr("#VSTR_XMLE_EVT_FILTER_OFF");
		}

		if (filter == FILTER_WARN)
		{
			return Tr("#VSTR_XMLE_EVT_FILTER_WARN");
		}

		if (filter == FILTER_UNSAVED)
		{
			return Tr("#VSTR_XMLE_EVT_FILTER_UNSAVED");
		}

		return string.Format(Tr("#VSTR_XMLE_EVT_FILTER_KIND"), VPPXEEventRules.KindName(filter));
	}

	protected bool PassesSearch(VPPXEEvtWork work)
	{
		string search = "";
		if (m_InputSearch)
		{
			search = m_InputSearch.GetText();
			search = search.Trim();
			search.ToLower();
		}

		if (search == "")
		{
			return true;
		}

		string lowerName = work.Row.Name;
		lowerName.ToLower();
		return lowerName.IndexOf(search) >= 0;
	}

	// The filter dropdown on one event; worst = its worst note severity.
	protected bool PassesFilter(VPPXEEvtWork work, int worst)
	{
		if (m_FilterIdx == FILTER_ALL)
		{
			return true;
		}

		if (m_FilterIdx == FILTER_OFF)
		{
			return !IsActiveRow(work.Row);
		}

		if (m_FilterIdx == FILTER_WARN)
		{
			return worst >= VPPXEEventRules.SEV_WARNING;
		}

		if (m_FilterIdx == FILTER_UNSAVED)
		{
			return work.IsDirty();
		}

		int kind = VPPXEEventRules.KindOf(work.Row.Name);
		if (m_FilterIdx == FILTER_UNKNOWN)
		{
			return kind == VPPXEEventRules.KIND_UNKNOWN;
		}

		return kind == m_FilterIdx;
	}

	protected void RebuildList()
	{
		if (!m_EvtList)
		{
			return;
		}

		m_EvtList.ClearItems();
		m_ListWork.Clear();
		m_ListSel = -1;
		VPPXEEvtFile file = CurrentFile();
		if (!file)
		{
			return;
		}

		array<ref VPPXEEventNote> notes = new array<ref VPPXEEventNote>;
		int workCount = file.Work.Count();
		int keepRow = -1;
		for (int i = 0; i < workCount; i++)
		{
			VPPXEEvtWork work = file.Work[i];
			if (work.Deleted)
			{
				continue;
			}

			if (!PassesSearch(work) && i != m_SelWork)
			{
				continue;
			}

			BuildNotes(file, i, notes);
			int worst = VPPXEEventRules.WorstSeverity(notes);
			if (!PassesFilter(work, worst) && i != m_SelWork)
			{
				continue;
			}

			int row = m_EvtList.AddItem("", null, 0);
			m_ListWork.Insert(i);
			FillListRow(row, work, worst);
			if (i == m_SelWork)
			{
				keepRow = row;
			}
		}

		if (keepRow >= 0)
		{
			m_EvtList.SelectRow(keepRow);
			m_EvtList.EnsureVisible(keepRow);
			m_ListSel = keepRow;
		}
		else
		{
			m_SelWork = -1;
		}
	}

	protected void FillListRow(int row, VPPXEEvtWork work, int worst)
	{
		VPPXEEventRow eventRow = work.Row;
		string nameText = eventRow.Name;
		if (work.OrigIndex < 0)
		{
			nameText = nameText + " +";
		}
		else if (work.IsDirty())
		{
			nameText = nameText + " *";
		}

		int kind = VPPXEEventRules.KindOf(eventRow.Name);
		string kindText = VPPXEEventRules.KindName(kind);
		if (kind == VPPXEEventRules.KIND_UNKNOWN)
		{
			kindText = "?";
		}

		string nominalText = "-";
		if (eventRow.Has(VPPXEEventField.NOMINAL))
		{
			nominalText = eventRow.Nominal.ToString();
		}

		string positionText = "-";
		if (eventRow.Has(VPPXEEventField.POSITION))
		{
			positionText = eventRow.Position;
		}

		string limitText = "-";
		if (eventRow.Has(VPPXEEventField.LIMIT))
		{
			limitText = eventRow.Limit;
		}

		m_EvtList.SetItem(row, nameText, null, 0);
		m_EvtList.SetItem(row, kindText, null, 1);
		m_EvtList.SetItem(row, nominalText, null, 2);
		m_EvtList.SetItem(row, positionText, null, 3);
		m_EvtList.SetItem(row, limitText, null, 4);
		int color = ARGB(255, 255, 255, 255);
		if (worst >= VPPXEEventRules.SEV_DISABLED)
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (work.IsDirty())
		{
			color = ARGB(255, 232, 163, 61);
		}
		else if (!IsActiveRow(eventRow))
		{
			color = ARGB(255, 120, 125, 130);
		}
		else if (worst == VPPXEEventRules.SEV_WARNING)
		{
			color = ARGB(255, 217, 178, 61);
		}

		for (int column = 0; column < 5; column++)
		{
			m_EvtList.SetItemColor(row, column, color);
		}
	}

	protected void RefreshSelectedRow()
	{
		VPPXEEvtFile file = CurrentFile();
		VPPXEEvtWork work = SelectedWork();
		if (!file || !work || m_ListSel < 0)
		{
			return;
		}

		array<ref VPPXEEventNote> notes = new array<ref VPPXEEventNote>;
		BuildNotes(file, m_SelWork, notes);
		FillListRow(m_ListSel, work, VPPXEEventRules.WorstSeverity(notes));
		if (m_FileDD)
		{
			m_FileDD.SetText(FileDropdownLabel(m_FileKey));
		}
	}

	protected void PollListSelection()
	{
		if (!m_EvtList)
		{
			return;
		}

		int row = m_EvtList.GetSelectedRow();
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

		m_ChildSel = -1;
		LoadForm();
		UpdateActionBar();
	}

	protected void PollSearch()
	{
		if (!m_InputSearch)
		{
			return;
		}

		string current = m_InputSearch.GetText();
		if (current == m_LastSearch)
		{
			return;
		}

		m_LastSearch = current;
		RebuildList();
		LoadForm();
		UpdateActionBar();
	}

	// ---------------------------------------------------------------- children

	protected void RebuildChildList()
	{
		if (!m_ChildList)
		{
			return;
		}

		m_ChildList.ClearItems();
		m_ChildListSel = -1;
		VPPXEEvtWork work = SelectedWork();
		if (!work)
		{
			UpdateChildTitle();
			return;
		}

		int count = work.Row.Children.Count();
		for (int i = 0; i < count; i++)
		{
			int row = m_ChildList.AddItem("", null, 0);
			FillChildRow(row, work.Row.Children[i]);
		}

		if (m_ChildSel >= 0 && m_ChildSel < count)
		{
			m_ChildList.SelectRow(m_ChildSel);
			m_ChildList.EnsureVisible(m_ChildSel);
			m_ChildListSel = m_ChildSel;
		}
		else
		{
			m_ChildSel = -1;
		}

		UpdateChildTitle();
	}

	protected void FillChildRow(int row, VPPXEEventChild child)
	{
		string typeText = child.Type;
		if (typeText == "")
		{
			typeText = "?";
		}

		m_ChildList.SetItem(row, typeText, null, 0);
		m_ChildList.SetItem(row, child.Min.ToString(), null, 1);
		m_ChildList.SetItem(row, child.Max.ToString(), null, 2);
		m_ChildList.SetItem(row, child.LootMin.ToString(), null, 3);
		m_ChildList.SetItem(row, child.LootMax.ToString(), null, 4);
		int color = ARGB(255, 255, 255, 255);
		if (!VPPXmlText.IsValidClassName(child.Type) || IsUnknownType(child.Type))
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (child.Max <= 0 && child.Min <= 0)
		{
			color = ARGB(255, 120, 125, 130);
		}

		for (int column = 0; column < 5; column++)
		{
			m_ChildList.SetItemColor(row, column, color);
		}
	}

	protected void RefreshChildRow()
	{
		VPPXEEventChild child = SelectedChild();
		if (!child || m_ChildListSel < 0)
		{
			return;
		}

		FillChildRow(m_ChildListSel, child);
	}

	protected void UpdateChildTitle()
	{
		VPPXEEvtWork work = SelectedWork();
		int count = 0;
		if (work)
		{
			count = work.Row.Children.Count();
		}

		m_TxtChildrenTitle.SetText(string.Format(Tr("#VSTR_XMLE_EVT_CHILDREN"), count));
	}

	protected void PollChildSelection()
	{
		if (!m_ChildList || !SelectedWork())
		{
			return;
		}

		int row = m_ChildList.GetSelectedRow();
		if (row == m_ChildListSel)
		{
			return;
		}

		m_ChildListSel = row;
		m_ChildSel = row;
		LoadChildForm();
		UpdateActionBar();
	}

	// Enable states and chip looks of the child editor.
	protected void UpdateChildDetails()
	{
		VPPXEEventChild child = SelectedChild();
		bool editable = CanEdit() && child != null;
		SetInputEnabled(m_InputChildType, m_LblChildType, editable);
		SetInputEnabled(m_InputChildMin, m_LblChildMin, editable);
		SetInputEnabled(m_InputChildMax, m_LblChildMax, editable);
		SetInputEnabled(m_InputChildLootMin, m_LblChildLootMin, editable);
		SetInputEnabled(m_InputChildLootMax, m_LblChildLootMax, editable);
		SetInputEnabled(m_InputChildDeloot, m_LblChildDeloot, editable);
		bool spawnSecOn = true;
		bool traceOn = false;
		if (child)
		{
			spawnSecOn = child.SpawnSecondary != 0;
			traceOn = child.Trace == 1;
		}

		SetChipLook(m_BtnChildSpawnSec, m_FillChildSpawnSec, m_TxtChildSpawnSec, spawnSecOn, editable, false);
		SetChipLook(m_BtnChildTrace, m_FillChildTrace, m_TxtChildTrace, traceOn, editable, false);
		VPPXEEvtWork work = SelectedWork();
		bool canAdd = CanEdit() && work != null && work.Row.Children.Count() < VPPXEEventRules.MAX_CHILDREN;
		m_BtnChildAdd.Enable(canAdd);
		m_BtnChildRemove.Enable(editable);
	}

	// ---------------------------------------------------------------- details

	// Title, state line, hints, chip looks, enable states and the checks of the selected event.
	protected void UpdateDetails()
	{
		VPPXEEvtWork work = SelectedWork();
		VPPXEEvtFile file = CurrentFile();
		if (!work || !file)
		{
			return;
		}

		VPPXEEventRow row = work.Row;
		bool editable = CanEdit();
		if (work.OrigIndex < 0)
		{
			m_TxtEditTitle.SetText(string.Format(Tr("#VSTR_XMLE_EVT_EDIT_NEW"), row.Name));
			m_TxtLine.SetText("");
		}
		else
		{
			m_TxtEditTitle.SetText(row.Name);
			m_TxtLine.SetText(string.Format(Tr("#VSTR_XMLE_MSG_LINE"), work.Orig.Line));
		}

		int kind = VPPXEEventRules.KindOf(row.Name);
		if (kind == VPPXEEventRules.KIND_UNKNOWN)
		{
			m_TxtKind.SetText(Tr("#VSTR_XMLE_EVT_KIND_UNKNOWN"));
			m_TxtKind.SetColor(ARGB(255, 194, 69, 69));
		}
		else
		{
			m_TxtKind.SetText(VPPXEEventRules.KindName(kind));
			m_TxtKind.SetColor(ARGB(255, 232, 163, 61));
		}

		SetChipLook(m_BtnActive, m_FillActive, m_TxtActive, IsActiveRow(row), editable, false);
		SetInputEnabled(m_InputName, m_LblName, editable);
		SetInputEnabled(m_InputSecondary, m_LblSecondary, editable);
		VPPXEEventMerge merge = Merge();
		VPPXEEventRow eff = merge.Effective.Get(row.Name);
		bool inherits = HasEarlierDefinition(row.Name);
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int field = VPPXEEventRules.NumberFieldAt(i);
			EditBoxWidget input = m_InputNum[i];
			TextWidget label = m_LblNum[i];
			SetInputEnabled(input, label, editable);
			TextWidget hint = m_TxtHint[i];
			hint.SetText(NumberHint(row, eff, field, inherits));
		}

		for (int p = 0; p < 3; p++)
		{
			bool posOn = row.Has(VPPXEEventField.POSITION) && row.Position == PositionValue(p);
			SetChipLook(m_BtnPos[p], m_FillPos[p], m_TxtPos[p], posOn, editable, false);
		}

		for (int l = 0; l < 4; l++)
		{
			bool limOn = row.Has(VPPXEEventField.LIMIT) && row.Limit == LimitValue(l);
			SetChipLook(m_BtnLim[l], m_FillLim[l], m_TxtLim[l], limOn, editable, false);
		}

		for (int f = 0; f < 3; f++)
		{
			bool flagOn = (row.Flags & FlagBit(f)) != 0;
			SetChipLook(m_BtnFlag[f], m_FillFlag[f], m_TxtFlag[f], flagOn, editable, false);
		}

		m_TxtSecondaryHint.SetText(SecondaryHint(row, merge));
		RefreshGroupTitle();
		UpdateChildDetails();
		array<ref VPPXEEventNote> notes = new array<ref VPPXEEventNote>;
		BuildNotes(file, m_SelWork, notes);
		int worst = VPPXEEventRules.WorstSeverity(notes);
		UpdateStateLine(row, merge, worst);
		m_TxtWarnings.SetText(NotesText(notes));
		if (worst >= VPPXEEventRules.SEV_DISABLED)
		{
			m_TxtWarnings.SetColor(ARGB(255, 222, 110, 110));
		}
		else if (worst == VPPXEEventRules.SEV_WARNING)
		{
			m_TxtWarnings.SetColor(ARGB(255, 217, 178, 61));
		}
		else
		{
			m_TxtWarnings.SetColor(ARGB(255, 154, 160, 166));
		}
	}

	// What the server does with the event at start, in one line.
	protected void UpdateStateLine(VPPXEEventRow row, VPPXEEventMerge merge, int worst)
	{
		string removedBy = "";
		merge.RemovedBy.Find(row.Name, removedBy);
		int kind = VPPXEEventRules.KindOf(row.Name);
		string text = "";
		int color = ARGB(255, 76, 175, 80);
		if (removedBy != "")
		{
			text = string.Format(Tr("#VSTR_XMLE_EVT_STATE_REMOVED"), FileLabel(removedBy));
			color = ARGB(255, 154, 160, 166);
		}
		else if (kind == VPPXEEventRules.KIND_UNKNOWN)
		{
			text = Tr("#VSTR_XMLE_EVT_STATE_IGNORED");
			color = ARGB(255, 194, 69, 69);
		}
		else if (worst >= VPPXEEventRules.SEV_DISABLED)
		{
			text = Tr("#VSTR_XMLE_EVT_STATE_DISABLED");
			color = ARGB(255, 194, 69, 69);
		}
		else
		{
			text = string.Format(Tr("#VSTR_XMLE_EVT_STATE_OK"), VPPXEEventRules.KindName(kind));
		}

		m_TxtState.SetText(text);
		m_TxtState.SetColor(color);
	}

	// Next to a number: the inherited value when left out in an override file, else the server default, plus the
	// lifetime / restock as a duration.
	protected string NumberHint(VPPXEEventRow row, VPPXEEventRow eff, int field, bool inherits)
	{
		bool isDuration = field == VPPXEEventField.LIFETIME || field == VPPXEEventField.RESTOCK;
		if (!row.Has(field))
		{
			if (inherits && eff && eff.Has(field))
			{
				int inherited = eff.GetNumber(field);
				return string.Format(Tr("#VSTR_XMLE_EVT_INHERITS"), inherited.ToString());
			}

			return Tr("#VSTR_XMLE_EVT_NOT_SET");
		}

		if (isDuration && row.GetNumber(field) > 0)
		{
			return FormatDuration(row.GetNumber(field));
		}

		return "";
	}

	protected string SecondaryHint(VPPXEEventRow row, VPPXEEventMerge merge)
	{
		if (!row.Has(VPPXEEventField.SECONDARY) || row.Secondary == "")
		{
			return Tr("#VSTR_XMLE_EVT_SECONDARY_NONE");
		}

		if (merge.Effective.Contains(row.Secondary))
		{
			return Tr("#VSTR_XMLE_EVT_SECONDARY_FOUND");
		}

		return Tr("#VSTR_XMLE_EVT_SECONDARY_MISSING");
	}

	void SetChipLook(ButtonWidget button, Widget fill, TextWidget label, bool isOn, bool enabled, bool danger)
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

	void SetInputEnabled(EditBoxWidget input, TextWidget label, bool enabled)
	{
		if (input)
		{
			input.Enable(enabled);
			if (enabled)
			{
				input.SetColor(ARGB(255, 255, 255, 255));
			}
			else
			{
				input.SetColor(ARGB(140, 255, 255, 255));
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

	protected void ShowEditor(bool show)
	{
		if (m_Form)
		{
			m_Form.Show(show && !m_ViewSpawns && !m_ViewGroups);
		}

		if (m_SpawnView)
		{
			m_SpawnView.Show(show && m_ViewSpawns);
		}

		UpdateMapVisibility();

		if (m_TxtEmpty)
		{
			m_TxtEmpty.Show(!show && !m_ViewGroups);
		}

		if (!show && m_TxtEditTitle)
		{
			m_TxtEditTitle.SetText(Tr("#VSTR_XMLE_EVT_EDIT_NONE"));
			m_TxtLine.SetText("");
		}

		RefreshGroupTitle();
	}

	protected void UpdateEmptyText()
	{
		if (!m_TxtEmpty)
		{
			return;
		}

		VPPXEEvtFile file = CurrentFile();
		if (m_FileKeys.Count() == 0 && m_Loaded)
		{
			m_TxtEmpty.SetText(Tr("#VSTR_XMLE_EVT_NO_FILE"));
		}
		else if (file && file.ErrorKey != "")
		{
			m_TxtEmpty.SetText(Tr(file.ErrorKey));
		}
		else
		{
			m_TxtEmpty.SetText(Tr("#VSTR_XMLE_EVT_EMPTY"));
		}
	}

	// Status line: request state, read-only reasons, server-side changes, else the unsaved count.
	protected void UpdateActionBar()
	{
		VPPXEEvtFile file = CurrentFile();
		int dirty = 0;
		if (file)
		{
			dirty = file.DirtyCount();
		}

		int spawnDirty = SpawnDirtyCount() + m_GroupsView.DirtyCount();
		bool editable = CanEdit();
		m_BtnAdd.Enable(editable);
		m_BtnDup.Enable(editable && SelectedWork() != null);
		m_BtnDelete.Enable(editable && SelectedWork() != null);
		m_BtnSave.Enable(EventsDirtyToSave() + SpawnsDirtyToSave() + m_GroupsView.DirtyToSave() > 0);
		bool serverChanged = (file && file.ServerChanged) || (m_Spawns && m_Spawns.ServerChanged) || m_GroupsView.IsServerChanged();
		bool revertable = dirty + spawnDirty > 0 || serverChanged;
		m_BtnRevert.Enable(revertable && !m_Saving);
		string text = m_StatusText;
		int color = ARGB(255, 154, 160, 166);
		if (m_StatusError)
		{
			color = ARGB(255, 194, 69, 69);
		}

		if (text == "")
		{
			if (serverChanged)
			{
				text = Tr("#VSTR_XMLE_EVT_SERVER_CHANGED");
				color = ARGB(255, 217, 178, 61);
			}
			else if (m_Owner && !m_Owner.HasPerm(VPPXEPerm.EDIT_EVENTS))
			{
				text = Tr("#VSTR_XMLE_EVT_READONLY");
			}
			else if (file && !file.Editable)
			{
				text = Tr("#VSTR_XMLE_EVT_FILE_READONLY");
			}
			else if (dirty + spawnDirty > 0)
			{
				text = string.Format(Tr("#VSTR_XMLE_DIRTY_FMT"), dirty + spawnDirty);
				color = ARGB(255, 232, 163, 61);
			}
			else
			{
				text = Tr("#VSTR_XMLE_DIRTY_NONE");
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

	// Left: header (title + file dropdown), search and filter, list, footer buttons.
	protected void LayoutLeft()
	{
		if (m_LeftW < 60 || m_LeftH < 120)
		{
			return;
		}

		float hostW = Math.Clamp(m_LeftW * 0.55, 150, 320);
		m_FileHost.SetPos(m_LeftW - hostW - 4, 2);
		m_FileHost.SetSize(hostW, 24);
		float filterW = Math.Clamp(m_LeftW * 0.42, 120, 240);
		m_FilterHost.SetPos(m_LeftW - filterW - 6, 34);
		m_FilterHost.SetSize(filterW, 24);
		m_SearchPanel.SetPos(6, 34);
		m_SearchPanel.SetSize(m_LeftW - filterW - 6 - 6 - 6, 24);
		float footerY = m_LeftH - FOOTER_H;
		float listH = footerY - 66 - 4;
		if (listH < 40)
		{
			listH = 40;
		}

		m_EvtList.SetPos(6, 66);
		m_EvtList.SetSize(m_LeftW - 12, listH);
		float buttonW = (m_LeftW - 12 - 12) / 3;
		m_BtnAdd.SetPos(6, footerY + 3);
		m_BtnAdd.SetSize(buttonW, 30);
		m_BtnDup.SetPos(6 + buttonW + 6, footerY + 3);
		m_BtnDup.SetSize(buttonW, 30);
		m_BtnDelete.SetPos(6 + 2 * (buttonW + 6), footerY + 3);
		m_BtnDelete.SetSize(buttonW, 30);
	}

	// Right: header, then the form rows top down (name, state, the numbers in two columns, position / limit / flags
	// chips, secondary, the children list and its editor, the checks), the action bar at the bottom.
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
		m_TxtEmpty.SetPos(20, 60);
		m_TxtEmpty.SetSize(m_RightW - 40, 80);
		float activeW = 110;
		float kindW = 110;
		float nameW = innerW - 64 - kindW - activeW - 18;
		if (nameW < 120)
		{
			nameW = 120;
		}

		m_LblName.SetPos(10, 0);
		m_LblName.SetSize(60, ROW_H);
		m_InputName.SetPos(74, 0);
		m_InputName.SetSize(nameW, ROW_H);
		m_TxtKind.SetPos(74 + nameW + 8, 0);
		m_TxtKind.SetSize(kindW, ROW_H);
		m_BtnActive.SetPos(m_RightW - 10 - activeW, 0);
		m_BtnActive.SetSize(activeW, ROW_H);
		m_TxtState.SetPos(10, 32);
		m_TxtState.SetSize(innerW, 20);
		float colW = innerW / 2;
		for (int i = 0; i < VPPXEEventRules.NUMBER_FIELDS; i++)
		{
			int rowIdx = i / 2;
			int colIdx = i - rowIdx * 2;
			float x = 10 + colIdx * colW;
			float y = 56 + rowIdx * 32;
			TextWidget label = m_LblNum[i];
			EditBoxWidget input = m_InputNum[i];
			TextWidget hint = m_TxtHint[i];
			label.SetPos(x, y);
			label.SetSize(NUM_LABEL_W, ROW_H);
			input.SetPos(x + NUM_LABEL_W + 2, y);
			input.SetSize(NUM_INPUT_W, ROW_H);
			hint.SetPos(x + NUM_LABEL_W + 2 + NUM_INPUT_W + 4, y);
			hint.SetSize(colW - NUM_LABEL_W - NUM_INPUT_W - 10, ROW_H);
		}

		PlaceChips(m_LblPosition, m_BtnPos, 188, innerW);
		PlaceChips(m_LblLimit, m_BtnLim, 220, innerW);
		PlaceChips(m_LblFlags, m_BtnFlag, 252, innerW);
		m_LblSecondary.SetPos(10, 284);
		m_LblSecondary.SetSize(CHIP_LABEL_W, ROW_H);
		float secondaryW = Math.Clamp(innerW * 0.45, 140, 300);
		m_InputSecondary.SetPos(10 + CHIP_LABEL_W + 2, 284);
		m_InputSecondary.SetSize(secondaryW, ROW_H);
		m_TxtSecondaryHint.SetPos(10 + CHIP_LABEL_W + 2 + secondaryW + 6, 284);
		m_TxtSecondaryHint.SetSize(innerW - CHIP_LABEL_W - secondaryW - 10, ROW_H);
		LayoutChildren(innerW, formH);
		LayoutHeaderChips();
		LayoutSpawnView(innerW, formH);
		m_GroupsView.Layout(m_RightW, innerW, formH, m_Unit);
		m_ActionBar.SetPos(0, m_RightH - BAR_H);
		m_ActionBar.SetSize(m_RightW, BAR_H);
	}

	protected void PlaceChips(TextWidget label, array<ButtonWidget> chips, float y, float innerW)
	{
		label.SetPos(10, y);
		label.SetSize(CHIP_LABEL_W, ROW_H);
		int count = chips.Count();
		float chipW = (innerW - CHIP_LABEL_W - 4 - (count - 1) * 6) / count;
		for (int i = 0; i < count; i++)
		{
			ButtonWidget chip = chips[i];
			if (chip)
			{
				chip.SetPos(10 + CHIP_LABEL_W + 4 + i * (chipW + 6), y);
				chip.SetSize(chipW, ROW_H);
			}
		}
	}

	// Children: title, list (takes the free height), labels, inputs, the second row, then the checks.
	protected void LayoutChildren(float innerW, float formH)
	{
		float titleY = 318;
		float warnH = 84;
		float editorH = 18 + ROW_H + 4 + ROW_H + 6;
		float listY = titleY + 22;
		float listH = formH - listY - editorH - warnH - 8;
		if (listH < 60)
		{
			listH = 60;
		}

		m_ImgChildren.SetPos(10, titleY + 10);
		m_TxtChildrenTitle.SetPos(34, titleY);
		m_TxtChildrenTitle.SetSize(innerW - 24, 20);
		m_ChildList.SetPos(10, listY);
		m_ChildList.SetSize(innerW, listH);
		float labelY = listY + listH + 4;
		float inputY = labelY + 18;
		float gap = 6;
		float numW = Math.Clamp((innerW * 0.55 - 3 * gap) / 4, 50, 90);
		float typeW = innerW - 4 * (numW + gap);
		float x = 10;
		PlaceChildInput(m_LblChildType, m_InputChildType, x, labelY, inputY, typeW);
		x = x + typeW + gap;
		PlaceChildInput(m_LblChildMin, m_InputChildMin, x, labelY, inputY, numW);
		x = x + numW + gap;
		PlaceChildInput(m_LblChildMax, m_InputChildMax, x, labelY, inputY, numW);
		x = x + numW + gap;
		PlaceChildInput(m_LblChildLootMin, m_InputChildLootMin, x, labelY, inputY, numW);
		x = x + numW + gap;
		PlaceChildInput(m_LblChildLootMax, m_InputChildLootMax, x, labelY, inputY, numW);
		float row2Y = inputY + ROW_H + 4;
		m_LblChildDeloot.SetPos(10, row2Y);
		m_LblChildDeloot.SetSize(60, ROW_H);
		m_InputChildDeloot.SetPos(72, row2Y);
		m_InputChildDeloot.SetSize(50, ROW_H);
		float buttonW = Math.Clamp(innerW * 0.18, 80, 140);
		float chipsW = innerW - 118 - 2 * buttonW - 3 * gap;
		float spawnSecW = chipsW * 0.6;
		float traceW = chipsW - spawnSecW - gap;
		x = 128;
		m_BtnChildSpawnSec.SetPos(x, row2Y);
		m_BtnChildSpawnSec.SetSize(spawnSecW, ROW_H);
		x = x + spawnSecW + gap;
		m_BtnChildTrace.SetPos(x, row2Y);
		m_BtnChildTrace.SetSize(traceW, ROW_H);
		x = x + traceW + gap;
		m_BtnChildAdd.SetPos(x, row2Y);
		m_BtnChildAdd.SetSize(buttonW, ROW_H);
		x = x + buttonW + gap;
		m_BtnChildRemove.SetPos(x, row2Y);
		m_BtnChildRemove.SetSize(buttonW, ROW_H);
		float warnY = row2Y + ROW_H + 8;
		float warnRoom = formH - warnY - 4;
		if (warnRoom < 20)
		{
			warnRoom = 20;
		}

		m_TxtWarnings.SetPos(10, warnY);
		m_TxtWarnings.SetSize(innerW, warnRoom);
	}

	protected void PlaceChildInput(TextWidget label, EditBoxWidget input, float x, float labelY, float inputY, float w)
	{
		label.SetPos(x, labelY);
		label.SetSize(w, 18);
		input.SetPos(x, inputY);
		input.SetSize(w, ROW_H);
	}

	// ---------------------------------------------------------------- spawns (cfgeventspawns.xml, SPAWNS view)

	// MenuXMLEditor: false while a dialog is open or another admin window has the priority (MapWidget ignores
	// z-order, like on the MAP tab).
	void SetMapAllowed(bool allowed)
	{
		m_MapAllowed = allowed;
		UpdateMapVisibility();
	}

	protected void UpdateMapVisibility()
	{
		if (m_GroupsView)
		{
			m_GroupsView.SetVisible(m_Shown && m_ViewGroups, m_MapAllowed);
		}

		bool visible = m_Shown && m_ViewSpawns && m_MapAllowed && SelectedWork() != null;
		if (m_SpawnMap)
		{
			m_SpawnMap.Show(visible);
		}

		if (m_SpawnCanvas)
		{
			m_SpawnCanvas.Show(visible);
		}

		if (!visible)
		{
			if (m_SpawnRenderer)
			{
				m_SpawnRenderer.ClearAll();
			}

			return;
		}

		m_SpawnRedraw = true;
	}

	bool CanEditSpawns()
	{
		if (!m_Spawns || !m_Spawns.Editable || m_Saving || m_Spawns.Key == "")
		{
			return false;
		}

		if (m_Pending && m_SpawnForceFresh)
		{
			return false;
		}

		return m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_EVENTS);
	}

	protected int SpawnDirtyCount()
	{
		if (!m_Spawns)
		{
			return 0;
		}

		return m_Spawns.DirtyCount();
	}

	string SelectedEventName()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work)
		{
			return "";
		}

		return work.Row.Name;
	}

	// The positions of the server: every entry of that name counts (the server adds them up), x and z not negative.
	protected int PosCountOf(string eventName)
	{
		int count = 0;
		if (!m_Spawns || eventName == "")
		{
			return 0;
		}

		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (VPPXESpawnRules.IsUsable(pos))
				{
					count++;
				}
			}
		}

		return count;
	}

	// Usable positions with a group cfgeventgroups.xml defines (any group name when that file could not be read).
	protected int GroupPosCountOf(string eventName)
	{
		int count = 0;
		if (!m_Spawns || eventName == "")
		{
			return 0;
		}

		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (VPPXESpawnRules.IsUsable(pos) && pos.Group != "" && IsKnownGroup(pos.Group))
				{
					count++;
				}
			}
		}

		return count;
	}

	// A group cfgeventgroups.xml defines (working copy); any name while that file could not be read.
	protected bool IsKnownGroup(string groupName)
	{
		if (!m_GroupsView || !m_GroupsView.IsKnown())
		{
			return true;
		}

		return m_GroupsView.HasGroup(groupName);
	}

	// The entry that carries the zone of an event (the first with a zone, else the first); create = add an entry.
	protected VPPXESpawnWork SpawnWorkOf(string eventName, bool create)
	{
		VPPXESpawnWork first = null;
		if (!m_Spawns || eventName == "")
		{
			return null;
		}

		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			if (work.Row.HasZone)
			{
				return work;
			}

			if (!first)
			{
				first = work;
			}
		}

		if (first || !create)
		{
			return first;
		}

		VPPXESpawnWork added = new VPPXESpawnWork();
		added.Row.Name = eventName;
		m_Spawns.Work.Insert(added);
		return added;
	}

	// (entry, position) pairs of the selected event in file order.
	protected void RebuildSpawnRefs()
	{
		m_SpawnRefWork.Clear();
		m_SpawnRefPos.Clear();
		string eventName = SelectedEventName();
		if (!m_Spawns || eventName == "")
		{
			m_SpawnSel = -1;
			return;
		}

		for (int w = 0; w < m_Spawns.Work.Count(); w++)
		{
			VPPXESpawnWork work = m_Spawns.Work[w];
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			for (int p = 0; p < work.Row.Positions.Count(); p++)
			{
				m_SpawnRefWork.Insert(w);
				m_SpawnRefPos.Insert(p);
			}
		}

		if (m_SpawnSel >= m_SpawnRefWork.Count())
		{
			m_SpawnSel = -1;
		}
	}

	protected VPPXESpawnPos SpawnPosAt(int refIdx)
	{
		if (!m_Spawns || refIdx < 0 || refIdx >= m_SpawnRefWork.Count())
		{
			return null;
		}

		VPPXESpawnWork work = m_Spawns.Work[m_SpawnRefWork[refIdx]];
		return work.Row.Positions[m_SpawnRefPos[refIdx]];
	}

	// VIEW_FORM, VIEW_SPAWNS or VIEW_GROUPS (the groups the selected event's positions use).
	protected void SwitchView(int view)
	{
		bool spawns = view == VIEW_SPAWNS;
		bool groups = view == VIEW_GROUPS;
		if (m_ViewSpawns == spawns && m_ViewGroups == groups)
		{
			return;
		}

		m_ViewSpawns = spawns;
		m_ViewGroups = groups;
		m_SpawnAddMode = false;
		if (spawns)
		{
			m_SpawnFitPending = true;
		}

		OnResize();
		if (groups)
		{
			ShowEditor(SelectedWork() != null);
			if (!PreviewSpawnGroup(!m_PreviewLoud))
			{
				m_GroupsView.Refresh();
			}
		}
		else
		{
			// the event's own title, form and spawns again
			LoadForm();
		}

		UpdateViewChips();
	}

	protected void UpdateViewChips()
	{
		int count = PosCountOf(SelectedEventName());
		m_TxtViewSpawns.SetText(string.Format(Tr("#VSTR_XMLE_EVT_VIEW_SPAWNS"), count));
		SetChipLook(m_BtnViewForm, m_FillViewForm, m_TxtViewForm, !m_ViewSpawns && !m_ViewGroups, true, false);
		SetChipLook(m_BtnViewSpawns, m_FillViewSpawns, m_TxtViewSpawns, m_ViewSpawns, true, false);
		SetChipLook(m_BtnViewGroups, m_FillViewGroups, m_TxtViewGroups, m_ViewGroups, true, false);
	}

	// SPAWNS: EDIT GROUP. The group of the selected position in the GROUPS view, previewed at that position.
	protected void OpenSpawnGroup()
	{
		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		if (!pos || pos.Group == "")
		{
			return;
		}

		if (m_ViewGroups)
		{
			PreviewSpawnGroup(false);
			return;
		}

		m_PreviewLoud = true;
		SwitchView(VIEW_GROUPS);
		m_PreviewLoud = false;
	}

	// The GROUPS view starts at the position selected in SPAWNS when it has a group; false when there is none. quiet:
	// no message when the group does not exist.
	protected bool PreviewSpawnGroup(bool quiet)
	{
		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		if (!pos || pos.Group == "")
		{
			return false;
		}

		float angle = -1;
		if (pos.A != "" && VPPXESpawnRules.IsNumberText(pos.A) && pos.A.ToFloat() >= 0)
		{
			angle = pos.A.ToFloat();
		}

		int number = m_SpawnSel + 1;
		string label = SelectedEventName() + " #" + number.ToString();
		float height = VPPXEGroupTransform.UNKNOWN_Y;
		if (pos.Y != "" && VPPXESpawnRules.IsNumberText(pos.Y))
		{
			height = pos.Y.ToFloat();
		}

		m_GroupsView.OpenGroup(pos.Group, Vector(pos.X.ToFloat(), angle, pos.Z.ToFloat()), height, label, quiet);
		return true;
	}

	// ---------------------------------------------------------------- builds coming back from the Object Manager

	// MenuXMLEditor.ApplyBuilderEdit: the objects of the local build replace the group's children (matched to the old
	// ones by type and place; unchanged ones stay as written). false when nothing was taken (a dialog is open, the
	// events data is not loaded, the group cannot take them): the Object Manager keeps the build to send it again.
	bool ApplyBuilderEdit(VPPXEGroupImport build, string groupName)
	{
		if (!build)
		{
			return false;
		}

		// a dialog would take the answer meant for it
		if (m_Owner.IsDialogOpen())
		{
			m_Owner.NotifyError(Tr("#VSTR_XMLE_BLD_ERR_BUSY"));
			return false;
		}

		if (!m_Loaded || m_Pending)
		{
			if (!m_Pending)
			{
				RequestList();
			}

			m_Owner.NotifyError(Tr("#VSTR_XMLE_BLD_ERR_NOT_LOADED"));
			return false;
		}

		array<int> counts = new array<int>;
		string problem = m_GroupsView.ReplaceFromBuilder(groupName, build, counts);
		if (problem != "")
		{
			m_Owner.NotifyError(string.Format(Tr(problem), groupName));
			return false;
		}

		m_MergeStale = true;
		bool posAdded = AddBuildPosition(build, groupName);
		if (!ShowGroupAt(groupName, build.AnchorPos))
		{
			if (m_ViewGroups)
			{
				m_GroupsView.Refresh();
			}
			else
			{
				SwitchView(VIEW_GROUPS);
			}
		}

		UpdateActionBar();
		m_Owner.Notify(string.Format(Tr("#VSTR_XMLE_BLD_RETURNED"), groupName, counts[0], counts[1], counts[2]));
		if (posAdded)
		{
			m_Owner.Notify(string.Format(Tr("#VSTR_XMLE_BLD_POS_ADDED"), build.EventName, groupName));
		}

		return true;
	}

	// A group built where none of its event's positions use it (a new group, laid out at the admin's position): that
	// place becomes a spawn position of the event (unsaved), so the group spawns where it was built. false when none
	// was needed or the event cannot take one (unknown, a kind that ignores groups, not in an events file).
	protected bool AddBuildPosition(VPPXEGroupImport build, string groupName)
	{
		string eventName = build.EventName;
		int kind = VPPXEEventRules.KindOf(eventName);
		if (eventName == "" || kind == VPPXEEventRules.KIND_UNKNOWN || VPPXEEventRules.KindIgnoresGroups(kind))
		{
			return false;
		}

		if (!m_Spawns || !CanEditSpawns() || EventFileOf(eventName) == "")
		{
			return false;
		}

		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos used : work.Row.Positions)
			{
				if (used.Group == groupName)
				{
					return false;
				}
			}
		}

		VPPXESpawnWork spawnWork = SpawnWorkOf(eventName, true);
		VPPXESpawnPos pos = new VPPXESpawnPos();
		pos.X = VPPXEGroupTransform.FormatNumber(build.AnchorPos[0]);
		pos.Z = VPPXEGroupTransform.FormatNumber(build.AnchorPos[2]);
		pos.A = VPPXEGroupTransform.FormatAngle(build.AnchorA);
		pos.Y = VPPXEGroupTransform.FormatNumber(build.AnchorPos[1]);
		pos.Group = groupName;
		spawnWork.Row.Positions.Insert(pos);
		return true;
	}

	// Selects an event whose position (nearest to near) uses the group and shows the group there; false when no event
	// of the events files uses it.
	protected bool ShowGroupAt(string groupName, vector near)
	{
		if (!m_Spawns)
		{
			return false;
		}

		string selected = SelectedEventName();
		VPPXESpawnWork bestWork = null;
		int bestPos = -1;
		float bestDist = 0;
		bool bestSelected = false;
		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || EventFileOf(work.Row.Name) == "")
			{
				continue;
			}

			bool isSelected = work.Row.Name == selected;
			for (int p = 0; p < work.Row.Positions.Count(); p++)
			{
				VPPXESpawnPos pos = work.Row.Positions[p];
				if (pos.Group != groupName)
				{
					continue;
				}

				float dx = pos.X.ToFloat() - near[0];
				float dz = pos.Z.ToFloat() - near[2];
				float dist = dx * dx + dz * dz;
				// the selected event first, then the nearest position
				bool better = !bestWork || (isSelected && !bestSelected) || (isSelected == bestSelected && dist < bestDist);
				if (better)
				{
					bestWork = work;
					bestPos = p;
					bestDist = dist;
					bestSelected = isSelected;
				}
			}
		}

		if (!bestWork)
		{
			return false;
		}

		SelectEventForBuild(EventFileOf(bestWork.Row.Name), bestWork.Row.Name, bestWork, bestPos);
		return true;
	}

	// The events file that defines the event (the shown one first), "" when none does.
	protected string EventFileOf(string eventName)
	{
		VPPXEEvtFile current = CurrentFile();
		if (current && NameInFile(current, eventName, -1))
		{
			return current.Key;
		}

		foreach (string fileKey : m_FileKeys)
		{
			VPPXEEvtFile file = m_Files.Get(fileKey);
			if (file && NameInFile(file, eventName, -1))
			{
				return fileKey;
			}
		}

		return "";
	}

	// Shows the event, its new position selected, in the GROUPS view (the group previewed at that position).
	protected void SelectEventForBuild(string fileKey, string eventName, VPPXESpawnWork spawnWork, int posIdx)
	{
		if (fileKey != m_FileKey)
		{
			SwitchFile(fileKey);
		}

		VPPXEEvtFile file = CurrentFile();
		m_SelWork = -1;
		for (int i = 0; i < file.Work.Count(); i++)
		{
			VPPXEEvtWork work = file.Work[i];
			if (!work.Deleted && work.Row.Name == eventName)
			{
				m_SelWork = i;
				break;
			}
		}

		m_ChildSel = -1;
		m_FilterIdx = FILTER_ALL;
		m_FilterDD.SetText(FilterLabel(FILTER_ALL));
		RebuildFileDropdown();
		RebuildList();
		LoadForm();
		int workIdx = m_Spawns.Work.Find(spawnWork);
		for (int r = 0; r < m_SpawnRefWork.Count(); r++)
		{
			if (m_SpawnRefWork[r] == workIdx && m_SpawnRefPos[r] == posIdx)
			{
				m_SpawnSel = r;
			}
		}

		RebuildSpawnList();
		UpdateSpawnDetails();
		PushSpawnsToRenderer();
		UpdateViewChips();
		if (m_ViewGroups)
		{
			PreviewSpawnGroup(true);
		}
		else
		{
			SwitchView(VIEW_GROUPS);
		}

		UpdateActionBar();
	}

	// ---------------------------------------------------------------- GROUPS view hooks

	VPPXESpawnFile GetSpawns()
	{
		return m_Spawns;
	}

	// An edit of a side file (spawns, groups) is allowed now: the permission, no save running, no fresh reload of
	// that file pending (an edit made meanwhile would be dropped).
	bool CanEditSide(bool forceFresh)
	{
		if (m_Saving)
		{
			return false;
		}

		if (m_Pending && forceFresh)
		{
			return false;
		}

		return m_Owner && m_Owner.HasPerm(VPPXEPerm.EDIT_EVENTS);
	}

	// A group was edited; structural = added, renamed or removed (positions and the event checks look at names).
	void OnGroupsEdited(bool structural)
	{
		if (structural)
		{
			m_MergeStale = true;
			RebuildSpawnRefs();
			RebuildSpawnList();
			UpdateSpawnDetails();
			PushSpawnsToRenderer();
			RebuildList();
		}

		UpdateActionBar();
	}

	void OnGroupsViewChanged()
	{
		UpdateActionBar();
	}

	void RefreshGroupTitle()
	{
		if (!m_ViewGroups || !m_GroupsView)
		{
			return;
		}

		m_TxtEditTitle.SetText(m_GroupsView.TitleText());
		m_TxtLine.SetText(m_GroupsView.LineText());
	}

	// Everything of the SPAWNS view for the selected event; reloadInputs when the selection changed.
	protected void RefreshSpawnView(bool reloadInputs)
	{
		RebuildSpawnRefs();
		RebuildSpawnList();
		if (reloadInputs)
		{
			LoadSpawnInputs();
		}

		UpdateSpawnDetails();
		PushSpawnsToRenderer();
		UpdateViewChips();
		UpdateMapVisibility();
	}

	protected void RebuildSpawnList()
	{
		if (!m_SpawnList)
		{
			return;
		}

		m_SpawnList.ClearItems();
		m_SpawnListSel = -1;
		int count = m_SpawnRefWork.Count();
		for (int i = 0; i < count; i++)
		{
			int row = m_SpawnList.AddItem("", null, 0);
			FillSpawnRow(row, i);
		}

		if (m_SpawnSel >= 0 && m_SpawnSel < count)
		{
			m_SpawnList.SelectRow(m_SpawnSel);
			m_SpawnList.EnsureVisible(m_SpawnSel);
			m_SpawnListSel = m_SpawnSel;
		}
	}

	protected void FillSpawnRow(int row, int refIdx)
	{
		VPPXESpawnPos pos = SpawnPosAt(refIdx);
		if (!pos)
		{
			return;
		}

		int number = refIdx + 1;
		string angleText = pos.A;
		if (angleText == "")
		{
			angleText = "-";
		}

		m_SpawnList.SetItem(row, number.ToString(), null, 0);
		m_SpawnList.SetItem(row, pos.X, null, 1);
		m_SpawnList.SetItem(row, pos.Z, null, 2);
		m_SpawnList.SetItem(row, angleText, null, 3);
		int color = ARGB(255, 255, 255, 255);
		if (!VPPXESpawnRules.IsUsable(pos))
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (pos.Group != "")
		{
			color = ARGB(255, 110, 180, 235);
			if (!IsKnownGroup(pos.Group))
			{
				color = ARGB(255, 217, 178, 61);
			}
		}

		for (int column = 0; column < 4; column++)
		{
			m_SpawnList.SetItemColor(row, column, color);
		}
	}

	protected void PollSpawnList()
	{
		if (!m_ViewSpawns || !m_SpawnList || !SelectedWork())
		{
			return;
		}

		int row = m_SpawnList.GetSelectedRow();
		if (row == m_SpawnListSel)
		{
			return;
		}

		m_SpawnListSel = row;
		SelectSpawn(row, true);
	}

	protected void SelectSpawn(int refIdx, bool centerMap)
	{
		m_SpawnSel = refIdx;
		if (refIdx >= m_SpawnRefWork.Count())
		{
			m_SpawnSel = -1;
		}

		if (m_SpawnList && m_SpawnList.GetSelectedRow() != m_SpawnSel)
		{
			m_SpawnList.SelectRow(m_SpawnSel);
			if (m_SpawnSel >= 0)
			{
				m_SpawnList.EnsureVisible(m_SpawnSel);
			}
		}

		m_SpawnListSel = m_SpawnList.GetSelectedRow();
		LoadSpawnInputs();
		UpdateSpawnDetails();
		PushSpawnsToRenderer();
		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		if (centerMap && pos && VPPXESpawnRules.IsUsable(pos) && m_SpawnMap)
		{
			m_SpawnMap.SetMapPos(Vector(pos.X.ToFloat(), 0, pos.Z.ToFloat()));
			m_SpawnRedraw = true;
		}
	}

	protected void LoadSpawnInputs()
	{
		m_Loading = true;
		VPPXESpawnWork zoneWork = SpawnWorkOf(SelectedEventName(), false);
		array<string> zoneValues = new array<string>;
		if (zoneWork && zoneWork.Row.HasZone)
		{
			zoneValues.Insert(zoneWork.Row.SMin);
			zoneValues.Insert(zoneWork.Row.SMax);
			zoneValues.Insert(zoneWork.Row.DMin);
			zoneValues.Insert(zoneWork.Row.DMax);
			zoneValues.Insert(zoneWork.Row.R);
		}

		for (int z = 0; z < m_InputZone.Count(); z++)
		{
			EditBoxWidget zoneBox = m_InputZone[z];
			if (z < zoneValues.Count())
			{
				zoneBox.SetText(zoneValues[z]);
			}
			else
			{
				zoneBox.SetText("");
			}
		}

		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		array<string> posValues = new array<string>;
		if (pos)
		{
			posValues.Insert(pos.X);
			posValues.Insert(pos.Z);
			posValues.Insert(pos.A);
			posValues.Insert(pos.Y);
			posValues.Insert(pos.Group);
		}

		for (int p = 0; p < m_InputPos.Count(); p++)
		{
			EditBoxWidget posBox = m_InputPos[p];
			if (p < posValues.Count())
			{
				posBox.SetText(posValues[p]);
			}
			else
			{
				posBox.SetText("");
			}
		}

		RememberSpawnInputs();
		m_Loading = false;
	}

	protected void RememberSpawnInputs()
	{
		for (int z = 0; z < m_InputZone.Count(); z++)
		{
			EditBoxWidget zoneBox = m_InputZone[z];
			m_LastZone.Set(z, zoneBox.GetText());
		}

		for (int p = 0; p < m_InputPos.Count(); p++)
		{
			EditBoxWidget posBox = m_InputPos[p];
			m_LastPos.Set(p, posBox.GetText());
		}
	}

	protected bool SpawnInputsChanged()
	{
		for (int z = 0; z < m_InputZone.Count(); z++)
		{
			EditBoxWidget zoneBox = m_InputZone[z];
			if (zoneBox.GetText() != m_LastZone[z])
			{
				return true;
			}
		}

		for (int p = 0; p < m_InputPos.Count(); p++)
		{
			EditBoxWidget posBox = m_InputPos[p];
			if (posBox.GetText() != m_LastPos[p])
			{
				return true;
			}
		}

		return false;
	}

	protected void PollSpawnInputs()
	{
		if (!m_ViewSpawns || m_Loading || !SelectedWork() || m_InputZone.Count() == 0)
		{
			return;
		}

		if (!SpawnInputsChanged())
		{
			return;
		}

		RememberSpawnInputs();
		if (!CanEditSpawns())
		{
			LoadSpawnInputs();
			return;
		}

		ApplySpawnInputs();
		AfterSpawnChange(false);
	}

	// The texts go to the entry as typed (trimmed); the checks show what the server would make of them and SAVE
	// refuses what it cannot write (VPPXESpawnRules.CheckRow).
	protected void ApplySpawnInputs()
	{
		VPPXESpawnWork zoneWork = SpawnWorkOf(SelectedEventName(), false);
		if (zoneWork && zoneWork.Row.HasZone)
		{
			zoneWork.Row.SMin = TrimmedText(m_InputZone[0]);
			zoneWork.Row.SMax = TrimmedText(m_InputZone[1]);
			zoneWork.Row.DMin = TrimmedText(m_InputZone[2]);
			zoneWork.Row.DMax = TrimmedText(m_InputZone[3]);
			zoneWork.Row.R = TrimmedText(m_InputZone[4]);
		}

		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		if (pos)
		{
			string oldGroup = pos.Group;
			pos.X = TrimmedText(m_InputPos[0]);
			pos.Z = TrimmedText(m_InputPos[1]);
			pos.A = TrimmedText(m_InputPos[2]);
			pos.Y = TrimmedText(m_InputPos[3]);
			pos.Group = TrimmedText(m_InputPos[4]);
			// a group given to a position without y would be laid out from 0 (underground): take the terrain height
			bool placeKnown = VPPXESpawnRules.IsNumberText(pos.X) && VPPXESpawnRules.IsNumberText(pos.Z);
			if (oldGroup == "" && pos.Group != "" && pos.Y == "" && placeKnown)
			{
				float posX = pos.X.ToFloat();
				float posZ = pos.Z.ToFloat();
				float groundY = GetGame().SurfaceY(posX, posZ);
				pos.Y = VPPXEGroupTransform.FormatNumber(groundY);
				EditBoxWidget heightBox = m_InputPos[3];
				heightBox.SetText(pos.Y);
				m_LastPos.Set(3, pos.Y);
			}
		}
	}

	// Some position of the event names a group.
	protected bool EventUsesGroups(string eventName)
	{
		if (!m_Spawns || eventName == "")
		{
			return false;
		}

		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.Deleted || work.Row.Name != eventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group != "")
				{
					return true;
				}
			}
		}

		return false;
	}

	protected string TrimmedText(EditBoxWidget box)
	{
		string text = box.GetText();
		return text.Trim();
	}

	protected void AfterSpawnChange(bool rebuildList)
	{
		m_MergeStale = true;
		if (rebuildList)
		{
			RebuildSpawnRefs();
			RebuildSpawnList();
		}
		else if (m_SpawnListSel >= 0)
		{
			FillSpawnRow(m_SpawnListSel, m_SpawnSel);
		}

		UpdateSpawnDetails();
		PushSpawnsToRenderer();
		UpdateViewChips();
		RefreshSelectedRow();
		UpdateActionBar();
	}

	protected void ToggleZone()
	{
		if (!CanEditSpawns() || !SelectedWork())
		{
			return;
		}

		VPPXESpawnWork zoneWork = SpawnWorkOf(SelectedEventName(), true);
		if (zoneWork.Row.HasZone)
		{
			zoneWork.Row.HasZone = false;
		}
		else
		{
			zoneWork.Row.HasZone = true;
			if (zoneWork.Row.SMin == "" && zoneWork.Row.R == "")
			{
				// StaticHeliCrash's vanilla zone
				zoneWork.Row.SMin = "1";
				zoneWork.Row.SMax = "3";
				zoneWork.Row.DMin = "3";
				zoneWork.Row.DMax = "5";
				zoneWork.Row.R = "45";
			}
		}

		LoadSpawnInputs();
		AfterSpawnChange(false);
	}

	protected void ToggleAddMode()
	{
		if (!CanEditSpawns() || !SelectedWork())
		{
			return;
		}

		m_SpawnAddMode = !m_SpawnAddMode;
		UpdateSpawnDetails();
	}

	// A new position of the selected event (x, z with 2 decimals; angle "-1" = random, like vanilla; height "" = none).
	// The server lays a group out from the position's y (child y is added to it, a missing y counts as 0), so a
	// position that has or gets a group needs its height or the group spawns underground.
	protected void AddSpawnAt(float x, float z, string angle, string height)
	{
		if (!CanEditSpawns() || !SelectedWork())
		{
			return;
		}

		VPPXESpawnWork work = SpawnWorkOf(SelectedEventName(), true);
		if (work.Row.Positions.Count() >= VPPXESpawnRules.MAX_POSITIONS)
		{
			return;
		}

		VPPXESpawnPos pos = new VPPXESpawnPos();
		pos.X = FormatCoord(x);
		pos.Z = FormatCoord(z);
		pos.A = angle;
		pos.Y = height;
		work.Row.Positions.Insert(pos);
		RebuildSpawnRefs();
		int workIdx = m_Spawns.Work.Find(work);
		m_SpawnSel = -1;
		for (int i = 0; i < m_SpawnRefWork.Count(); i++)
		{
			if (m_SpawnRefWork[i] == workIdx && m_SpawnRefPos[i] == work.Row.Positions.Count() - 1)
			{
				m_SpawnSel = i;
			}
		}

		AfterSpawnChange(true);
		LoadSpawnInputs();
	}

	// The admin's own position, with the character's heading as the angle and its height (exact on floors, roofs and
	// bridges, where the terrain height is not).
	protected void AddSpawnHere()
	{
		Man player = GetGame().GetPlayer();
		if (!player)
		{
			m_Owner.NotifyError(Tr("#VSTR_XMLE_SPW_ERR_NO_PLAYER"));
			return;
		}

		vector position = player.GetPosition();
		vector orientation = player.GetOrientation();
		float yaw = orientation[0];
		if (yaw < 0)
		{
			yaw = yaw + 360;
		}

		int heading = Math.Round(yaw);
		if (heading >= 360)
		{
			heading = heading - 360;
		}

		string height = VPPXEGroupTransform.FormatNumber(position[1]);
		AddSpawnAt(position[0], position[2], heading.ToString(), height);
		if (m_SpawnMap)
		{
			m_SpawnMap.SetMapPos(position);
			m_SpawnRedraw = true;
		}
	}

	protected void RemoveSpawn()
	{
		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		if (!CanEditSpawns() || !pos)
		{
			return;
		}

		VPPXESpawnWork work = m_Spawns.Work[m_SpawnRefWork[m_SpawnSel]];
		work.Row.Positions.RemoveOrdered(m_SpawnRefPos[m_SpawnSel]);
		m_SpawnSel = -1;
		AfterSpawnChange(true);
		LoadSpawnInputs();
	}

	// Left button edges over the SPAWNS map: a press and release within a few pixels is a click (a longer move was a
	// pan). Polled because the MapWidget keeps its mouse events.
	protected void PollSpawnMapMouse()
	{
		bool down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = down && !m_MouseWasDown;
		bool released = !down && m_MouseWasDown;
		m_MouseWasDown = down;
		if (!m_SpawnMap || !m_SpawnMap.IsVisible())
		{
			m_MapPressed = false;
			return;
		}

		int mx;
		int my;
		GetMousePos(mx, my);
		if (pressed)
		{
			m_MapPressed = GetWidgetUnderCursor() == m_SpawnMap;
			m_MapDownX = mx;
			m_MapDownY = my;
			return;
		}

		if (!released || !m_MapPressed)
		{
			return;
		}

		m_MapPressed = false;
		int moveX = mx - m_MapDownX;
		int moveY = my - m_MapDownY;
		if (moveX * moveX + moveY * moveY > 36 || GetWidgetUnderCursor() != m_SpawnMap)
		{
			return;
		}

		OnSpawnMapClicked(mx, my);
	}

	protected void OnSpawnMapClicked(int mx, int my)
	{
		if (m_SpawnAddMode && CanEditSpawns())
		{
			vector cursor = m_SpawnMap.ScreenToMap(Vector(mx, my, 0));
			// the terrain height when the event lays out groups (else no y, like vanilla positions without a group)
			string clickHeight = "";
			if (EventUsesGroups(SelectedEventName()))
			{
				float groundY = GetGame().SurfaceY(cursor[0], cursor[2]);
				clickHeight = VPPXEGroupTransform.FormatNumber(groundY);
			}

			AddSpawnAt(cursor[0], cursor[2], "-1", clickHeight);
			return;
		}

		int hitLayer;
		int hitIndex;
		vector hitPos;
		if (m_SpawnRenderer.HitTest(mx, my, hitLayer, hitIndex, hitPos) && hitIndex >= 0 && hitIndex < m_SpawnDrawnRefs.Count())
		{
			SelectSpawn(m_SpawnDrawnRefs[hitIndex], false);
			return;
		}

		SelectSpawn(-1, false);
	}

	// Centres the map on the event's positions and zooms so they all fit (at least 400 m visible).
	protected bool FitSpawns()
	{
		float minX = 0;
		float maxX = 0;
		float minZ = 0;
		float maxZ = 0;
		bool any = false;
		for (int i = 0; i < m_SpawnRefWork.Count(); i++)
		{
			VPPXESpawnPos pos = SpawnPosAt(i);
			if (!VPPXESpawnRules.IsUsable(pos))
			{
				continue;
			}

			float px = pos.X.ToFloat();
			float pz = pos.Z.ToFloat();
			if (!any)
			{
				minX = px;
				maxX = px;
				minZ = pz;
				maxZ = pz;
				any = true;
				continue;
			}

			minX = Math.Min(minX, px);
			maxX = Math.Max(maxX, px);
			minZ = Math.Min(minZ, pz);
			maxZ = Math.Max(maxZ, pz);
		}

		if (!any || !m_SpawnMap || !m_SpawnRenderer)
		{
			return true;
		}

		m_SpawnMap.SetMapPos(Vector((minX + maxX) * 0.5, 0, (minZ + maxZ) * 0.5));
		if (!m_SpawnRenderer.Calibrate() || m_SpawnRenderer.GetPixelsPerMetre() <= 0)
		{
			return false;
		}

		float wantM = Math.Max(Math.Max(maxX - minX, maxZ - minZ) * 1.3, 400);
		float visibleM = Math.Min(m_SpawnRenderer.GetCanvasWidth(), m_SpawnRenderer.GetCanvasHeight()) / m_SpawnRenderer.GetPixelsPerMetre();
		if (visibleM > 1)
		{
			float newScale = m_SpawnMap.GetScale() * wantM / visibleM;
			m_SpawnMap.SetScale(Math.Clamp(newScale, 0.02, 1.0));
		}

		m_SpawnRedraw = true;
		return true;
	}

	protected void PushSpawnsToRenderer()
	{
		if (!m_SpawnRenderer)
		{
			return;
		}

		array<float> xs = new array<float>;
		array<float> zs = new array<float>;
		array<float> angles = new array<float>;
		array<int> kinds = new array<int>;
		m_SpawnDrawnRefs.Clear();
		for (int i = 0; i < m_SpawnRefWork.Count(); i++)
		{
			VPPXESpawnPos pos = SpawnPosAt(i);
			if (!pos || !VPPXESpawnRules.IsNumberText(pos.X) || !VPPXESpawnRules.IsNumberText(pos.Z))
			{
				continue;
			}

			xs.Insert(pos.X.ToFloat());
			zs.Insert(pos.Z.ToFloat());
			float angle = -1;
			if (pos.A != "" && VPPXESpawnRules.IsNumberText(pos.A))
			{
				angle = pos.A.ToFloat();
			}

			angles.Insert(angle);
			int kind = VPPXESpawnRenderer.KIND_PLAIN;
			if (!VPPXESpawnRules.IsUsable(pos))
			{
				kind = VPPXESpawnRenderer.KIND_UNUSABLE;
			}
			else if (pos.Group != "")
			{
				kind = VPPXESpawnRenderer.KIND_GROUP;
			}

			kinds.Insert(kind);
			m_SpawnDrawnRefs.Insert(i);
		}

		float zoneR = 0;
		VPPXESpawnWork zoneWork = SpawnWorkOf(SelectedEventName(), false);
		if (zoneWork && zoneWork.Row.HasZone && VPPXESpawnRules.IsNumberText(zoneWork.Row.R))
		{
			zoneR = zoneWork.Row.R.ToFloat();
		}

		m_SpawnRenderer.SetSpawns(xs, zs, angles, kinds, zoneR);
		VPPXESpawnPos selected = SpawnPosAt(m_SpawnSel);
		if (selected && VPPXESpawnRules.IsNumberText(selected.X) && VPPXESpawnRules.IsNumberText(selected.Z))
		{
			m_SpawnRenderer.SetSelection(Vector(selected.X.ToFloat(), 0, selected.Z.ToFloat()), true);
		}
		else
		{
			m_SpawnRenderer.SetSelection(vector.Zero, false);
		}

		m_SpawnRedraw = true;
		if (m_SpawnMap && m_SpawnMap.IsVisible())
		{
			DoSpawnRedraw(NowMs());
		}
	}

	// Like the MAP tab: a pure pan slides the last drawing with the map (redrawn once the view settles or the slide
	// uncovers too much), a zoom or a resize redraws throttled, a data change redraws at once. A pending fit waits for
	// the layout. The player marker follows every frame.
	protected void UpdateSpawnMap(int now)
	{
		if (!m_ViewSpawns || !m_SpawnMap || !m_SpawnMap.IsVisible() || !m_SpawnRenderer)
		{
			ShowPlayerMarker(false);
			return;
		}

		if (m_SpawnFitPending && FitSpawns())
		{
			m_SpawnFitPending = false;
		}

		float scale = m_SpawnMap.GetScale();
		vector mapPos = m_SpawnMap.GetMapPos();
		float cx;
		float cy;
		float cw;
		float ch;
		m_SpawnMap.GetScreenPos(cx, cy);
		m_SpawnMap.GetScreenSize(cw, ch);
		bool viewChanged = scale != m_SpawnLastScale || mapPos != m_SpawnLastPos;
		bool rectChanged = cx != m_SpawnLastCX || cy != m_SpawnLastCY || cw != m_SpawnLastCW || ch != m_SpawnLastCH;
		if (viewChanged || rectChanged)
		{
			m_SpawnLastScale = scale;
			m_SpawnLastPos = mapPos;
			m_SpawnLastCX = cx;
			m_SpawnLastCY = cy;
			m_SpawnLastCW = cw;
			m_SpawnLastCH = ch;
			m_SpawnSettlePending = true;
			m_SpawnLastChange = now;
			if (rectChanged || !m_SpawnAnchorValid || scale != m_SpawnAnchorScale)
			{
				m_SpawnRedraw = true;
			}
			else
			{
				vector anchorNow = m_SpawnMap.MapToScreen(Vector(0, 0, 0));
				float slideX = anchorNow[0] - m_SpawnAnchorScreen[0];
				float slideY = anchorNow[1] - m_SpawnAnchorScreen[1];
				m_SpawnCanvas.SetPos(slideX / m_Unit, slideY / m_Unit);
				if (Math.AbsFloat(slideX) > cw * 0.35 || Math.AbsFloat(slideY) > ch * 0.35)
				{
					m_SpawnRedraw = true;
				}
			}
		}

		if (m_SpawnRedraw && now - m_SpawnLastRedraw >= 50)
		{
			DoSpawnRedraw(now);
		}
		else if (m_SpawnSettlePending && now - m_SpawnLastChange >= 150)
		{
			DoSpawnRedraw(now);
		}

		UpdatePlayerMarker();
	}

	protected void DoSpawnRedraw(int now)
	{
		m_SpawnCanvas.SetPos(0, 0);
		m_SpawnRenderer.Redraw();
		m_SpawnAnchorScreen = m_SpawnMap.MapToScreen(Vector(0, 0, 0));
		m_SpawnAnchorScale = m_SpawnMap.GetScale();
		m_SpawnAnchorValid = true;
		m_SpawnLastRedraw = now;
		m_SpawnRedraw = false;
		m_SpawnSettlePending = false;
	}

	// The admin's own character on the SPAWNS map (hidden when off the view or without a character).
	protected void UpdatePlayerMarker()
	{
		Man player = GetGame().GetPlayer();
		float sx;
		float sy;
		float mapX;
		float mapY;
		float mapW;
		float mapH;
		vector playerPos = vector.Zero;
		if (player)
		{
			playerPos = player.GetPosition();
		}

		if (!player || !m_ImgSpawnMe || !m_SpawnRenderer.ScreenOf(playerPos[0], playerPos[2], sx, sy))
		{
			ShowPlayerMarker(false);
			return;
		}

		m_SpawnRenderer.MapRect(mapX, mapY, mapW, mapH);
		if (sx < mapX || sy < mapY || sx > mapX + mapW || sy > mapY + mapH)
		{
			ShowPlayerMarker(false);
			return;
		}

		float bgW;
		float bgH;
		float meW;
		float meH;
		m_ImgSpawnMeBg.GetScreenSize(bgW, bgH);
		m_ImgSpawnMe.GetScreenSize(meW, meH);
		m_ImgSpawnMeBg.SetScreenPos(sx - bgW * 0.5, sy - bgH * 0.5);
		m_ImgSpawnMe.SetScreenPos(sx - meW * 0.5, sy - meH * 0.5);
		ShowPlayerMarker(true);
	}

	protected void ShowPlayerMarker(bool show)
	{
		if (m_ImgSpawnMe)
		{
			m_ImgSpawnMe.Show(show);
		}

		if (m_ImgSpawnMeBg)
		{
			m_ImgSpawnMeBg.Show(show);
		}
	}

	// Enable states, chip looks and the info / checks text of the SPAWNS view.
	protected void UpdateSpawnDetails()
	{
		bool editable = CanEditSpawns() && SelectedWork() != null;
		VPPXESpawnWork zoneWork = SpawnWorkOf(SelectedEventName(), false);
		bool hasZone = zoneWork && zoneWork.Row.HasZone;
		VPPXESpawnPos pos = SpawnPosAt(m_SpawnSel);
		SetChipLook(m_BtnZone, m_FillZone, m_TxtZone, hasZone, editable, false);
		SetChipLook(m_BtnSpawnAdd, m_FillSpawnAdd, m_TxtSpawnAdd, m_SpawnAddMode, editable, false);
		for (int z = 0; z < m_InputZone.Count(); z++)
		{
			SetInputEnabled(m_InputZone[z], m_LblZone[z], editable && hasZone);
		}

		for (int p = 0; p < m_InputPos.Count(); p++)
		{
			SetInputEnabled(m_InputPos[p], m_LblPos[p], editable && pos != null);
		}

		m_BtnSpawnHere.Enable(editable);
		m_BtnSpawnRemove.Enable(editable && pos != null);
		m_BtnSpawnGroup.Enable(pos != null && pos.Group != "");
		m_TxtSpawnInfo.SetText(SpawnInfoText());
	}

	protected string SpawnInfoText()
	{
		string eventName = SelectedEventName();
		array<string> lines = new array<string>;
		if (!m_Spawns || (m_Spawns.ErrorKey != "" && m_Spawns.Key == ""))
		{
			lines.Insert(Tr("#VSTR_XMLE_SPW_NO_FILE"));
		}
		else if (m_Spawns.ErrorKey != "")
		{
			lines.Insert(Tr(m_Spawns.ErrorKey));
		}

		int total = m_SpawnRefWork.Count();
		lines.Insert(string.Format(Tr("#VSTR_XMLE_SPW_INFO_COUNT"), total, eventName));
		if (m_SpawnAddMode)
		{
			lines.Insert(Tr("#VSTR_XMLE_SPW_INFO_ADD"));
		}

		int unusable = 0;
		map<string, bool> unknownGroups = new map<string, bool>;
		for (int i = 0; i < total; i++)
		{
			VPPXESpawnPos pos = SpawnPosAt(i);
			if (!VPPXESpawnRules.IsUsable(pos))
			{
				unusable++;
			}
			else if (pos.Group != "" && !IsKnownGroup(pos.Group))
			{
				unknownGroups.Set(pos.Group, true);
			}
		}

		if (unusable > 0)
		{
			lines.Insert(string.Format(Tr("#VSTR_XMLE_SPW_W_UNUSABLE"), unusable));
		}

		for (int g = 0; g < unknownGroups.Count(); g++)
		{
			lines.Insert(string.Format(Tr("#VSTR_XMLE_SPW_W_GROUP_UNKNOWN"), unknownGroups.GetKey(g)));
		}

		VPPXESpawnWork zoneWork = SpawnWorkOf(eventName, false);
		if (zoneWork)
		{
			string zoneKey = VPPXESpawnRules.ZoneProblem(zoneWork.Row);
			if (zoneKey != "")
			{
				lines.Insert(Tr(zoneKey));
			}

			string rowKey = VPPXESpawnRules.CheckRow(zoneWork.Row, zoneWork.Orig);
			if (rowKey != "")
			{
				lines.Insert(Tr(rowKey));
			}
		}

		string text = "";
		foreach (string line : lines)
		{
			if (text != "")
			{
				text = text + "\n";
			}

			text = text + line;
		}

		return text;
	}

	// The cfgeventspawns.xml entries of the event the form shows. They follow a rename only when this event is the one
	// definition of its name (another definition keeps using them otherwise).
	protected void CaptureRenameEntries(VPPXEEvtWork work)
	{
		m_RenameEntries.Clear();
		m_RenameSync = false;
		if (!work || !m_Spawns || work.Row.Name == "")
		{
			return;
		}

		VPPXEEventMerge merge = Merge();
		array<string> defFiles = merge.DefFiles.Get(work.Row.Name);
		if (!defFiles || defFiles.Count() != 1 || m_NameCounts.Get(m_FileKey + "|" + work.Row.Name) != 1)
		{
			return;
		}

		foreach (VPPXESpawnWork entry : m_Spawns.Work)
		{
			if (!entry.Deleted && entry.Row.Name == work.Row.Name)
			{
				m_RenameEntries.Insert(entry);
			}
		}

		m_RenameSync = true;
	}

	// An event renamed in the form takes its captured cfgeventspawns.xml entries along (an empty name is skipped; the
	// entries keep the last name until a real one is typed).
	protected void SyncSpawnRename(string newName)
	{
		if (!m_RenameSync || newName == "" || !CanEditSpawns())
		{
			return;
		}

		foreach (VPPXESpawnWork entry : m_RenameEntries)
		{
			if (entry)
			{
				entry.Row.Name = newName;
			}
		}
	}

	// The spawns file of the reply: kept when it has unsaved edits (flagged when the server copy changed meanwhile),
	// unless a save or revert asked for a fresh copy.
	protected void FinishSpawns()
	{
		if (m_Spawns && !m_SpawnForceFresh && m_Spawns.DirtyCount() > 0)
		{
			if (m_Spawns.Revision != m_NewSpawnRevision)
			{
				m_Spawns.ServerChanged = true;
			}

			m_SpawnForceFresh = false;
			return;
		}

		VPPXESpawnFile file = new VPPXESpawnFile();
		file.Key = m_NewSpawnKey;
		file.Revision = m_NewSpawnRevision;
		file.Editable = m_NewSpawnEditable;
		file.ErrorKey = m_NewSpawnError;
		file.GroupsError = m_NewGroupsError;
		for (int c = 0; c < m_SpawnChunkCount; c++)
		{
			array<ref VPPXESpawnRow> part = m_SpawnChunks.Get(c);
			if (!part)
			{
				continue;
			}

			foreach (VPPXESpawnRow row : part)
			{
				VPPXESpawnWork work = new VPPXESpawnWork();
				work.OrigIndex = file.Work.Count();
				work.Orig = new VPPXESpawnRow();
				work.Orig.CopyFrom(row);
				work.Row.CopyFrom(row);
				file.Work.Insert(work);
			}
		}

		m_Spawns = file;
		m_SpawnForceFresh = false;
		m_SpawnSel = -1;
	}

	protected void SendSpawnSave()
	{
		array<ref VPPXESpawnEdit> edits = new array<ref VPPXESpawnEdit>;
		foreach (VPPXESpawnWork work : m_Spawns.Work)
		{
			if (work.OrigIndex < 0 && (work.Deleted || (!work.Row.HasZone && work.Row.Positions.Count() == 0)))
			{
				continue;
			}

			if (!work.IsDirty())
			{
				continue;
			}

			VPPXESpawnEdit edit = new VPPXESpawnEdit();
			if (work.OrigIndex < 0)
			{
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

		m_SpawnSaveReqId = m_Owner.NextReqId();
		array<ref VPPXESpawnsSave> parts = new array<ref VPPXESpawnsSave>;
		VPPXESpawnsSave part = null;
		int partBytes = 0;
		foreach (VPPXESpawnEdit queued : edits)
		{
			int size = VPPXESpawnRules.EstimateRow(queued.Row) + 16;
			if (!part || part.Edits.Count() >= VPPXESpawnRules.EDITS_PER_PART || partBytes + size > VPPXESpawnRules.CHUNK_BYTES)
			{
				part = new VPPXESpawnsSave();
				part.ReqId = m_SpawnSaveReqId;
				part.PartIdx = parts.Count();
				part.FileKey = m_Spawns.Key;
				part.BaseRevision = m_Spawns.Revision;
				parts.Insert(part);
				partBytes = 0;
			}

			part.Edits.Insert(queued);
			partBytes += size;
		}

		m_SpawnSavePending = true;
		int partCount = parts.Count();
		foreach (VPPXESpawnsSave sendPart : parts)
		{
			sendPart.PartCount = partCount;
			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveEventSpawns", new Param1<ref VPPXESpawnsSave>(sendPart), true, null);
		}
	}

	// Header: EVENT / SPAWNS / GROUPS chips at the right, the line number left of them.
	protected void LayoutHeaderChips()
	{
		float groupsW = 90;
		float spawnsW = 118;
		float formW = 84;
		float groupsX = m_RightW - 6 - groupsW;
		m_BtnViewGroups.SetPos(groupsX, 3);
		m_BtnViewGroups.SetSize(groupsW, 22);
		float spawnsX = groupsX - 6 - spawnsW;
		m_BtnViewSpawns.SetPos(spawnsX, 3);
		m_BtnViewSpawns.SetSize(spawnsW, 22);
		m_BtnViewForm.SetPos(spawnsX - 6 - formW, 3);
		m_BtnViewForm.SetSize(formW, 22);
		m_TxtLine.SetPos(6 + groupsW + 6 + spawnsW + 6 + formW + 10, 0);
	}

	// SPAWNS view: zone row, the map with the positions list at its right, the position editor, the buttons, the
	// info / checks text.
	protected void LayoutSpawnView(float innerW, float formH)
	{
		m_SpawnView.SetPos(0, 32);
		m_SpawnView.SetSize(m_RightW, formH);
		float zoneX = 10;
		m_BtnZone.SetPos(zoneX, 0);
		m_BtnZone.SetSize(80, ROW_H);
		zoneX = zoneX + 86;
		float zoneStep = Math.Clamp((innerW - 86) / 5, 70, 110);
		for (int z = 0; z < m_InputZone.Count(); z++)
		{
			TextWidget zoneLabel = m_LblZone[z];
			EditBoxWidget zoneBox = m_InputZone[z];
			zoneLabel.SetPos(zoneX, 0);
			zoneLabel.SetSize(36, ROW_H);
			zoneBox.SetPos(zoneX + 36, 0);
			zoneBox.SetSize(zoneStep - 42, ROW_H);
			zoneX = zoneX + zoneStep;
		}

		float mapTop = 34;
		float bottomH = 18 + ROW_H + 6 + ROW_H + 6 + 54;
		float mapH = formH - mapTop - bottomH;
		if (mapH < 120)
		{
			mapH = 120;
		}

		float listW = Math.Clamp(innerW * 0.3, 150, 230);
		float mapW = innerW - listW - 6;
		m_SpawnMapArea.SetPos(10, mapTop);
		m_SpawnMapArea.SetSize(mapW, mapH);
		m_SpawnList.SetPos(10 + mapW + 6, mapTop);
		m_SpawnList.SetSize(listW, mapH);
		float labelY = mapTop + mapH + 6;
		float inputY = labelY + 18;
		// x and z get the most room, angle and height a bit less, the group the rest (at most 160)
		float groupW = Math.Clamp(innerW * 0.18, 90, 160);
		float coordW = (innerW - groupW - 24) * 0.3;
		float smallW = (innerW - groupW - 24) * 0.2;
		float posX = 10;
		for (int p = 0; p < m_InputPos.Count(); p++)
		{
			float boxW = coordW;
			if (p == 2 || p == 3)
			{
				boxW = smallW;
			}
			else if (p == m_InputPos.Count() - 1)
			{
				boxW = groupW;
			}

			TextWidget posLabel = m_LblPos[p];
			EditBoxWidget posBox = m_InputPos[p];
			posLabel.SetPos(posX, labelY);
			posLabel.SetSize(boxW, 18);
			posBox.SetPos(posX, inputY);
			posBox.SetSize(boxW, ROW_H);
			posX = posX + boxW + 6;
		}

		float buttonY = inputY + ROW_H + 6;
		float buttonW = (innerW - 18) / 4;
		m_BtnSpawnAdd.SetPos(10, buttonY);
		m_BtnSpawnAdd.SetSize(buttonW, ROW_H);
		m_BtnSpawnHere.SetPos(10 + buttonW + 6, buttonY);
		m_BtnSpawnHere.SetSize(buttonW, ROW_H);
		m_BtnSpawnRemove.SetPos(10 + 2 * (buttonW + 6), buttonY);
		m_BtnSpawnRemove.SetSize(buttonW, ROW_H);
		m_BtnSpawnGroup.SetPos(10 + 3 * (buttonW + 6), buttonY);
		m_BtnSpawnGroup.SetSize(buttonW, ROW_H);
		float infoY = buttonY + ROW_H + 6;
		float infoH = formH - infoY - 2;
		if (infoH < 20)
		{
			infoH = 20;
		}

		m_TxtSpawnInfo.SetPos(10, infoY);
		m_TxtSpawnInfo.SetSize(innerW, infoH);
		m_SpawnRedraw = true;
	}

	// x / z with two decimals (a new position; untouched ones keep their text).
	protected string FormatCoord(float value)
	{
		int cents = Math.Round(value * 100);
		if (cents < 0)
		{
			cents = 0;
		}

		int whole = cents / 100;
		int frac = cents - whole * 100;
		return whole.ToString() + "." + VPPXmlText.Pad2(frac);
	}

	// ---------------------------------------------------------------- helpers

	protected string PositionValue(int index)
	{
		if (index == 0)
		{
			return "fixed";
		}

		if (index == 1)
		{
			return "player";
		}

		return "uniform";
	}

	protected string LimitValue(int index)
	{
		if (index == 0)
		{
			return "child";
		}

		if (index == 1)
		{
			return "mixed";
		}

		if (index == 2)
		{
			return "custom";
		}

		return "parent";
	}

	protected int FlagBit(int index)
	{
		if (index == 0)
		{
			return VPPXEEventFlag.DELETABLE;
		}

		if (index == 1)
		{
			return VPPXEEventFlag.INIT_RANDOM;
		}

		return VPPXEEventFlag.REMOVE_DAMAGED;
	}

	protected bool IsActiveRow(VPPXEEventRow row)
	{
		return !row.Has(VPPXEEventField.ACTIVE) || row.Active == 1;
	}

	// seconds -> "2d 3h" style text with the translated unit keys (as the TYPES tab shows lifetimes)
	protected string FormatDuration(int seconds)
	{
		int days = seconds / 86400;
		int hours = (seconds % 86400) / 3600;
		int mins = (seconds % 3600) / 60;
		int secs = seconds % 60;
		string unitD = Tr("#VSTR_XMLE_UNIT_D");
		string unitH = Tr("#VSTR_XMLE_UNIT_H");
		string unitM = Tr("#VSTR_XMLE_UNIT_M");
		string unitS = Tr("#VSTR_XMLE_UNIT_S");
		string result = "";
		if (days > 0)
		{
			result = days.ToString() + unitD;
			if (hours > 0)
			{
				result = result + " " + hours.ToString() + unitH;
			}

			return result;
		}

		if (hours > 0)
		{
			result = hours.ToString() + unitH;
			if (mins > 0)
			{
				result = result + " " + mins.ToString() + unitM;
			}

			return result;
		}

		if (mins > 0)
		{
			result = mins.ToString() + unitM;
			if (secs > 0)
			{
				result = result + " " + secs.ToString() + unitS;
			}

			return result;
		}

		return secs.ToString() + unitS;
	}

	protected VPPXEEvtFile CurrentFile()
	{
		if (m_FileKey == "")
		{
			return null;
		}

		return m_Files.Get(m_FileKey);
	}

	protected VPPXEEvtWork SelectedWork()
	{
		VPPXEEvtFile file = CurrentFile();
		if (!file || m_SelWork < 0 || m_SelWork >= file.Work.Count())
		{
			return null;
		}

		VPPXEEvtWork work = file.Work[m_SelWork];
		if (work.Deleted)
		{
			return null;
		}

		return work;
	}

	protected VPPXEEventChild SelectedChild()
	{
		VPPXEEvtWork work = SelectedWork();
		if (!work || m_ChildSel < 0 || m_ChildSel >= work.Row.Children.Count())
		{
			return null;
		}

		return work.Row.Children[m_ChildSel];
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
