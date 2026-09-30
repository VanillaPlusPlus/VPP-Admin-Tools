// VPP XML Editor: EVENTS tab, GROUPS view (cfgeventgroups.xml). The groups the selected event's spawn positions use,
// plus groups added here and not saved yet (list with ADD / DUPLICATE / RENAME / DELETE), the children of the selected group (list and editor: type, offset x / z, angle, height, loot
// min / max, deloot, spawnsecondary, trace) and a map preview of the group the way the server lays it out at a spawn
// position (offsets turned by the position's angle, VPPXEGroupRules.Place). The preview anchor cycles through the
// position selected in SPAWNS, every position of the selected event that uses the group, the note in the file, the
// admin's own position and the map centre. A rename follows into cfgeventspawns.xml. Edits stay local until the tab's SAVE
// (XE_SaveEventGroups, EditEvents).

class VPPXEGroupWork : Managed
{
	int OrigIndex;
	bool Deleted;
	ref VPPXEGroupRow Row;
	ref VPPXEGroupRow Orig;

	void VPPXEGroupWork()
	{
		OrigIndex = -1;
		Row = new VPPXEGroupRow();
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

class VPPXEGroupFile : Managed
{
	string Key;
	int Revision;
	bool Editable;
	string ErrorKey;
	bool ServerChanged;
	ref array<ref VPPXEGroupWork> Work;

	void VPPXEGroupFile()
	{
		Key = "";
		ErrorKey = "";
		Work = new array<ref VPPXEGroupWork>;
	}

	int DirtyCount()
	{
		int dirty = 0;
		foreach (VPPXEGroupWork work : Work)
		{
			// a group added here and removed again, or still without children, is not written
			if (work.OrigIndex < 0 && (work.Deleted || work.Row.Children.Count() == 0))
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

// The GROUPS view map: the anchor (orange, angle tick when not random), a thin spoke to every child, the children
// (teal; red when the server skips them) with their angle ticks, the selection.
class VPPXEGroupRenderer : VPPXESpawnRenderer
{
	const static int KIND_OK = 0;
	const static int KIND_BAD = 1;

	protected bool m_HasAnchor;
	protected float m_AnchorX;
	protected float m_AnchorZ;
	protected float m_AnchorA;
	protected ref array<float> m_KidX;
	protected ref array<float> m_KidZ;
	protected ref array<float> m_KidA;
	protected ref array<int> m_KidKind;

	void VPPXEGroupRenderer(MapWidget mapWidget, CanvasWidget canvasWidget)
	{
		m_KidX = new array<float>;
		m_KidZ = new array<float>;
		m_KidA = new array<float>;
		m_KidKind = new array<int>;
	}

	void SetGroup(bool hasAnchor, vector anchor, array<float> xs, array<float> zs, array<float> angles, array<int> kinds)
	{
		m_HasAnchor = hasAnchor;
		m_AnchorX = anchor[0];
		m_AnchorA = anchor[1];
		m_AnchorZ = anchor[2];
		m_KidX.Copy(xs);
		m_KidZ.Copy(zs);
		m_KidA.Copy(angles);
		m_KidKind.Copy(kinds);
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

		int outline = ARGB(230, 8, 9, 10);
		float ppm = Math.Max(Math.AbsFloat(m_AX), 0.0001);
		float tickWorld = 14 / ppm;
		float ax = m_S0X + m_AnchorX * m_AX;
		float ay = m_S0Y + m_AnchorZ * m_AZ;
		int count = m_KidX.Count();
		if (m_HasAnchor)
		{
			for (int s = 0; s < count; s++)
			{
				float sx = m_S0X + m_KidX[s] * m_AX;
				float sy = m_S0Y + m_KidZ[s] * m_AZ;
				DrawSegment(ax, ay, sx, sy, 1, ARGB(110, 200, 205, 210));
			}
		}

		for (int i = 0; i < count; i++)
		{
			float lx = m_S0X + m_KidX[i] * m_AX;
			float ly = m_S0Y + m_KidZ[i] * m_AZ;
			if (lx < -20 || ly < -20 || lx > m_CW + 20 || ly > m_CH + 20)
			{
				continue;
			}

			int color = ARGB(255, 80, 200, 220);
			if (m_KidKind[i] == KIND_BAD)
			{
				color = ARGB(255, 194, 69, 69);
			}

			float angle = m_KidA[i];
			if (angle >= 0)
			{
				float rad = angle * Math.DEG2RAD;
				float ex = m_S0X + (m_KidX[i] + Math.Sin(rad) * tickWorld) * m_AX;
				float ey = m_S0Y + (m_KidZ[i] + Math.Cos(rad) * tickWorld) * m_AZ;
				DrawSegment(lx, ly, ex, ey, 4, outline);
				DrawSegment(lx, ly, ex, ey, 2, ARGB(255, 60, 255, 90));
			}

			DrawDisc(lx, ly, 6, outline);
			DrawDisc(lx, ly, 4, color);
			AddHitMark(0, i, 1, 0, m_KidX[i], m_KidZ[i]);
		}

		if (m_HasAnchor)
		{
			if (m_AnchorA >= 0)
			{
				float anchorRad = m_AnchorA * Math.DEG2RAD;
				float fx = m_S0X + (m_AnchorX + Math.Sin(anchorRad) * tickWorld * 1.6) * m_AX;
				float fy = m_S0Y + (m_AnchorZ + Math.Cos(anchorRad) * tickWorld * 1.6) * m_AZ;
				DrawSegment(ax, ay, fx, fy, 5, outline);
				DrawSegment(ax, ay, fx, fy, 3, ARGB(255, 232, 163, 61));
			}

			DrawDisc(ax, ay, 8, outline);
			DrawDisc(ax, ay, 6, ARGB(255, 232, 163, 61));
			DrawDisc(ax, ay, 2.5, outline);
		}

		DrawSelection();
	}
};

class VPPXEGroupsView : Managed
{
	const static int ROW_H = 28;
	const static int INPUT_COUNT = 8;
	const static int IN_TYPE = 0;
	const static int IN_X = 1;
	const static int IN_Z = 2;
	const static int IN_A = 3;
	const static int IN_Y = 4;
	const static int IN_LOOTMIN = 5;
	const static int IN_LOOTMAX = 6;
	const static int IN_DELOOT = 7;

	// the tab owns this view
	protected VPPXEEventsTab m_Tab;
	protected MenuXMLEditor m_Owner;

	protected Widget m_View;
	protected Widget m_MapArea;
	protected MapWidget m_Map;
	protected CanvasWidget m_Canvas;
	protected ImageWidget m_ImgMe;
	protected ImageWidget m_ImgMeBg;
	protected TextWidget m_TxtGroupsTitle;
	protected TextListboxWidget m_GroupList;
	protected ButtonWidget m_BtnAdd;
	protected ButtonWidget m_BtnDup;
	protected ButtonWidget m_BtnRename;
	protected ButtonWidget m_BtnDelete;
	protected ButtonWidget m_BtnAnchor;
	protected ButtonWidget m_BtnBuilder;
	protected TextWidget m_TxtChildrenTitle;
	protected TextListboxWidget m_ChildList;
	protected ref array<TextWidget> m_Lbl;
	protected ref array<EditBoxWidget> m_Input;
	protected ref array<string> m_Last;
	protected ButtonWidget m_BtnSpawnSec;
	protected Widget m_FillSpawnSec;
	protected TextWidget m_TxtSpawnSec;
	protected ButtonWidget m_BtnTrace;
	protected Widget m_FillTrace;
	protected TextWidget m_TxtTrace;
	protected ButtonWidget m_BtnAddMode;
	protected Widget m_FillAddMode;
	protected TextWidget m_TxtAddMode;
	protected ButtonWidget m_BtnChildAdd;
	protected ButtonWidget m_BtnChildRemove;
	protected TextWidget m_TxtInfo;
	protected ref VPPXEGroupRenderer m_Renderer;

	protected ref VPPXEGroupFile m_File;
	protected bool m_Known;
	protected ref map<int, ref array<ref VPPXEGroupRow>> m_Chunks;
	protected int m_ChunkCount;
	protected string m_NewKey;
	protected int m_NewRevision;
	protected bool m_NewEditable;
	protected string m_NewError;
	protected bool m_ForceFresh;

	protected ref array<int> m_ListWork;
	// positions per group name, counted when the list is rebuilt
	protected ref map<string, int> m_UseCounts;
	// the event whose groups are listed ("" = none selected) and the group names its positions use
	protected string m_EventName;
	protected ref map<string, bool> m_EventGroups;
	protected int m_SelWork;
	protected int m_ListSel;
	protected int m_ChildSel;
	protected int m_ChildListSel;
	protected bool m_Loading;
	protected bool m_AddMode;
	// the group a pending DUPLICATE / DELETE dialog is about (null when a reload replaced it meanwhile)
	protected VPPXEGroupWork m_PendCopy;
	protected bool m_PendIsCopy;
	protected VPPXEGroupWork m_PendDelete;

	// preview anchors of the selected group: x, angle (-1 = random), z and a label each
	protected ref array<vector> m_Anchors;
	protected ref array<string> m_AnchorLabels;
	protected ref array<float> m_AnchorYs;
	protected float m_CameFromY;
	// the group an EDIT IN BUILDER confirm dialog is about
	protected VPPXEGroupWork m_PendBuilder;
	protected int m_AnchorIdx;
	// the spawn position the view was opened from (SPAWNS: EDIT GROUP), for that group only
	protected string m_CameFromGroup;
	protected vector m_CameFrom;
	protected string m_CameFromLabel;
	protected VPPXEGroupWork m_AnchorWork;
	protected ref array<int> m_DrawnChildren;

	protected bool m_Visible;
	protected float m_Unit;
	protected bool m_Redraw;
	protected bool m_FitPending;
	protected float m_LastScale;
	protected vector m_LastPos;
	protected int m_LastRedraw;
	protected bool m_AnchorValid;
	protected vector m_AnchorScreen;
	// the world point the slide is measured at (the view centre of the last drawing)
	protected vector m_AnchorWorld;
	// ms after which a moved view is drawn again whatever the change detection saw (0 = none)
	protected int m_RedrawAt;
	protected float m_AnchorScale;
	// the projection of the last drawing (pixels per metre along x) and of the previous frame (screen position of
	// m_AnchorWorld, pixels per metre): the widget may take a few frames to reach a SetScale / SetMapPos while
	// GetScale / GetMapPos already answer the target, so view changes are detected on the projection itself
	protected float m_AnchorPpm;
	protected vector m_FrameProbe;
	protected float m_FramePpm;
	protected bool m_SettlePending;
	protected int m_LastChange;
	protected float m_LastCX;
	protected float m_LastCY;
	protected float m_LastCW;
	protected float m_LastCH;
	protected bool m_MouseWasDown;
	protected bool m_MapPressed;
	protected int m_MapDownX;
	protected int m_MapDownY;

	void VPPXEGroupsView(VPPXEEventsTab tab, MenuXMLEditor owner, Widget root)
	{
		m_Tab = tab;
		m_Owner = owner;
		m_Lbl = new array<TextWidget>;
		m_Input = new array<EditBoxWidget>;
		m_Last = new array<string>;
		m_Chunks = new map<int, ref array<ref VPPXEGroupRow>>;
		m_ChunkCount = -1;
		m_NewKey = "";
		m_NewError = "";
		m_ListWork = new array<int>;
		m_UseCounts = new map<string, int>;
		m_EventName = "";
		m_EventGroups = new map<string, bool>;
		m_SelWork = -1;
		m_ListSel = -1;
		m_ChildSel = -1;
		m_ChildListSel = -1;
		m_Anchors = new array<vector>;
		m_AnchorLabels = new array<string>;
		m_AnchorYs = new array<float>;
		m_CameFromY = VPPXEGroupTransform.UNKNOWN_Y;
		m_CameFromGroup = "";
		m_CameFromLabel = "";
		m_DrawnChildren = new array<int>;
		m_Unit = 1.0;
		Bind(root);
		if (m_Map && m_Canvas)
		{
			m_Renderer = new VPPXEGroupRenderer(m_Map, m_Canvas);
		}
	}

	protected void Bind(Widget root)
	{
		m_View = root.FindAnyWidget("EvtGroupView");
		m_MapArea = root.FindAnyWidget("EvtGroupMapArea");
		m_Map = MapWidget.Cast(root.FindAnyWidget("EvtGroupMap"));
		m_Canvas = CanvasWidget.Cast(root.FindAnyWidget("EvtGroupCanvas"));
		m_ImgMe = ImageWidget.Cast(root.FindAnyWidget("ImgEvtGroupMe"));
		m_ImgMeBg = ImageWidget.Cast(root.FindAnyWidget("ImgEvtGroupMeBg"));
		m_TxtGroupsTitle = TextWidget.Cast(root.FindAnyWidget("TxtEvtGroupsTitle"));
		m_GroupList = TextListboxWidget.Cast(root.FindAnyWidget("EvtGroupList"));
		m_BtnAdd = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpAdd"));
		m_BtnDup = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpDup"));
		m_BtnRename = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpRename"));
		m_BtnDelete = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpDelete"));
		m_BtnAnchor = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpAnchor"));
		m_BtnBuilder = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpBuilder"));
		m_TxtChildrenTitle = TextWidget.Cast(root.FindAnyWidget("TxtEvtGrpChildrenTitle"));
		m_ChildList = TextListboxWidget.Cast(root.FindAnyWidget("EvtGrpChildList"));
		for (int i = 0; i < INPUT_COUNT; i++)
		{
			m_Lbl.Insert(TextWidget.Cast(root.FindAnyWidget("LblEvtGrpC" + i.ToString())));
			EditBoxWidget box = EditBoxWidget.Cast(root.FindAnyWidget("InputEvtGrpC" + i.ToString()));
			m_Input.Insert(box);
			m_Last.Insert("");
			if (box)
			{
				// the tab lets go of the keyboard when the mouse leaves a box
				box.SetHandler(m_Tab);
			}
		}

		m_BtnSpawnSec = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpSpawnSec"));
		m_FillSpawnSec = root.FindAnyWidget("FillEvtGrpSpawnSec");
		m_TxtSpawnSec = TextWidget.Cast(root.FindAnyWidget("TxtEvtGrpSpawnSec"));
		m_BtnTrace = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpTrace"));
		m_FillTrace = root.FindAnyWidget("FillEvtGrpTrace");
		m_TxtTrace = TextWidget.Cast(root.FindAnyWidget("TxtEvtGrpTrace"));
		m_BtnAddMode = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpAddMode"));
		m_FillAddMode = root.FindAnyWidget("FillEvtGrpAddMode");
		m_TxtAddMode = TextWidget.Cast(root.FindAnyWidget("TxtEvtGrpAddMode"));
		m_BtnChildAdd = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpChildAdd"));
		m_BtnChildRemove = ButtonWidget.Cast(root.FindAnyWidget("BtnEvtGrpChildRemove"));
		m_TxtInfo = TextWidget.Cast(root.FindAnyWidget("TxtEvtGrpInfo"));
	}

	// Hover tips (the tab shows them).
	void RegisterTips()
	{
		m_Tab.AddTip(m_BtnAdd, "#VSTR_XMLE_GRP_BTN_ADD", "#VSTR_XMLE_GRP_TIP_ADD");
		m_Tab.AddTip(m_BtnDup, "#VSTR_XMLE_GRP_BTN_DUP", "#VSTR_XMLE_GRP_TIP_DUP");
		m_Tab.AddTip(m_BtnRename, "#VSTR_XMLE_GRP_BTN_RENAME", "#VSTR_XMLE_GRP_TIP_RENAME");
		m_Tab.AddTip(m_BtnDelete, "#VSTR_XMLE_GRP_BTN_DELETE", "#VSTR_XMLE_GRP_TIP_DELETE");
		m_Tab.AddTip(m_BtnAnchor, "#VSTR_XMLE_GRP_BTN_ANCHOR", "#VSTR_XMLE_GRP_TIP_ANCHOR");
		m_Tab.AddTip(m_BtnBuilder, "#VSTR_XMLE_GRP_BTN_BUILDER", "#VSTR_XMLE_GRP_TIP_BUILDER");
		m_Tab.AddTip(m_Input[IN_TYPE], "#VSTR_XMLE_GRP_C_TYPE", "#VSTR_XMLE_GRP_TIP_TYPE");
		m_Tab.AddTip(m_Input[IN_X], "#VSTR_XMLE_GRP_C_X", "#VSTR_XMLE_GRP_TIP_X");
		m_Tab.AddTip(m_Input[IN_Z], "#VSTR_XMLE_GRP_C_Z", "#VSTR_XMLE_GRP_TIP_Z");
		m_Tab.AddTip(m_Input[IN_A], "#VSTR_XMLE_GRP_C_A", "#VSTR_XMLE_GRP_TIP_A");
		m_Tab.AddTip(m_Input[IN_Y], "#VSTR_XMLE_GRP_C_Y", "#VSTR_XMLE_GRP_TIP_Y");
		m_Tab.AddTip(m_Input[IN_LOOTMIN], "#VSTR_XMLE_GRP_C_LOOTMIN", "#VSTR_XMLE_EVT_TIP_LOOTMIN");
		m_Tab.AddTip(m_Input[IN_LOOTMAX], "#VSTR_XMLE_GRP_C_LOOTMAX", "#VSTR_XMLE_EVT_TIP_LOOTMAX");
		m_Tab.AddTip(m_Input[IN_DELOOT], "#VSTR_XMLE_GRP_C_DELOOT", "#VSTR_XMLE_EVT_TIP_DELOOT");
		m_Tab.AddTip(m_BtnSpawnSec, "#VSTR_XMLE_EVT_CHILD_SPAWNSEC", "#VSTR_XMLE_EVT_TIP_SPAWNSEC");
		m_Tab.AddTip(m_BtnTrace, "#VSTR_XMLE_EVT_CHILD_TRACE", "#VSTR_XMLE_GRP_TIP_TRACE");
		m_Tab.AddTip(m_BtnAddMode, "#VSTR_XMLE_SPW_BTN_ADD", "#VSTR_XMLE_GRP_TIP_ADDMODE");
		m_Tab.AddTip(m_BtnChildAdd, "#VSTR_XMLE_EVT_BTN_CHILD_ADD", "#VSTR_XMLE_GRP_TIP_CHILD_ADD");
		m_Tab.AddTip(m_BtnChildRemove, "#VSTR_XMLE_EVT_BTN_CHILD_REMOVE", "#VSTR_XMLE_GRP_TIP_CHILD_REMOVE");
	}

	// ---------------------------------------------------------------- loading (XE_GetEvents sends the groups first)

	void BeginLoad()
	{
		m_Chunks = new map<int, ref array<ref VPPXEGroupRow>>;
		m_ChunkCount = -1;
		m_NewKey = "";
		m_NewRevision = 0;
		m_NewEditable = false;
		m_NewError = "";
	}

	void OnChunk(VPPXEGroupsChunk chunk)
	{
		m_ChunkCount = chunk.ChunkCount;
		m_NewKey = chunk.FileKey;
		m_NewRevision = chunk.Revision;
		m_NewEditable = chunk.Editable;
		m_NewError = chunk.ErrorKey;
		array<ref VPPXEGroupRow> chunkRows = new array<ref VPPXEGroupRow>;
		if (chunk.Rows)
		{
			foreach (VPPXEGroupRow row : chunk.Rows)
			{
				if (row)
				{
					chunkRows.Insert(row);
				}
			}
		}

		m_Chunks.Set(chunk.ChunkIdx, chunkRows);
	}

	bool IsLoadDone()
	{
		return m_ChunkCount >= 0 && m_Chunks.Count() >= m_ChunkCount;
	}

	// The groups of the reply: kept when they have unsaved edits (flagged when the server copy changed meanwhile),
	// unless a save or revert asked for a fresh copy.
	void FinishLoad()
	{
		string keepName = "";
		VPPXEGroupWork keepWork = SelectedWork();
		if (keepWork)
		{
			keepName = keepWork.Row.Name;
		}

		m_Known = m_NewError == "" || m_NewError == "#VSTR_XMLE_STATE_MISSING";
		if (m_File && !m_ForceFresh && m_File.DirtyCount() > 0)
		{
			if (m_File.Revision != m_NewRevision)
			{
				m_File.ServerChanged = true;
			}

			m_ForceFresh = false;
			Refresh();
			return;
		}

		VPPXEGroupFile file = new VPPXEGroupFile();
		file.Key = m_NewKey;
		file.Revision = m_NewRevision;
		file.Editable = m_NewEditable;
		file.ErrorKey = m_NewError;
		for (int c = 0; c < m_ChunkCount; c++)
		{
			array<ref VPPXEGroupRow> part = m_Chunks.Get(c);
			if (!part)
			{
				continue;
			}

			foreach (VPPXEGroupRow row : part)
			{
				VPPXEGroupWork work = new VPPXEGroupWork();
				work.OrigIndex = file.Work.Count();
				work.Orig = new VPPXEGroupRow();
				work.Orig.CopyFrom(row);
				work.Row.CopyFrom(row);
				file.Work.Insert(work);
			}
		}

		m_File = file;
		m_ForceFresh = false;
		m_Chunks = new map<int, ref array<ref VPPXEGroupRow>>;
		m_SelWork = -1;
		m_ChildSel = -1;
		SelectByName(keepName);
		m_FitPending = true;
		Refresh();
	}

	// SAVE or REVERT: the next reply replaces the working copy.
	void ForceFresh()
	{
		m_ForceFresh = true;
	}

	bool IsForceFresh()
	{
		return m_ForceFresh;
	}

	void FlagServerChanged(string fileKey, int revision, bool ownSave)
	{
		if (!m_File || m_File.Key != fileKey || ownSave)
		{
			return;
		}

		if (m_File.DirtyCount() > 0 && m_File.Revision != revision)
		{
			m_File.ServerChanged = true;
		}
	}

	void MarkServerChanged()
	{
		if (m_File)
		{
			m_File.ServerChanged = true;
		}
	}

	bool IsServerChanged()
	{
		return m_File && m_File.ServerChanged;
	}

	string FileKey()
	{
		if (!m_File)
		{
			return "";
		}

		return m_File.Key;
	}

	int DirtyCount()
	{
		if (!m_File)
		{
			return 0;
		}

		return m_File.DirtyCount();
	}

	// Unsaved edits SAVE writes (0 when the file cannot be edited now).
	int DirtyToSave()
	{
		if (!CanEdit())
		{
			return 0;
		}

		return DirtyCount();
	}

	bool CanEdit()
	{
		if (!m_File || !m_File.Editable || m_File.Key == "")
		{
			return false;
		}

		return m_Tab.CanEditSide(m_ForceFresh);
	}

	// cfgeventgroups.xml could be read (or does not exist): the group names below are what the server knows.
	bool IsKnown()
	{
		return m_Known;
	}

	// A group of that name the server would use (defined, not removed here, with a usable child).
	bool HasGroup(string groupName)
	{
		if (!m_File || groupName == "")
		{
			return false;
		}

		foreach (VPPXEGroupWork work : m_File.Work)
		{
			if (!work.Deleted && work.Row.Name == groupName && ValidChildCount(work.Row) > 0)
			{
				return true;
			}
		}

		return false;
	}

	int GroupCount()
	{
		int count = 0;
		if (!m_File)
		{
			return 0;
		}

		foreach (VPPXEGroupWork work : m_File.Work)
		{
			if (!work.Deleted)
			{
				count++;
			}
		}

		return count;
	}

	// A group that cannot be written: selected and reported; false stops the save.
	bool CheckBeforeSave()
	{
		if (!m_File)
		{
			return true;
		}

		for (int i = 0; i < m_File.Work.Count(); i++)
		{
			VPPXEGroupWork work = m_File.Work[i];
			if (work.Deleted || !work.IsDirty())
			{
				continue;
			}

			string problem = VPPXEGroupRules.CheckRow(work.Row, work.Orig);
			bool nameChanged = !work.Orig || work.Orig.Name != work.Row.Name;
			if (problem == "" && nameChanged && NameCount(work.Row.Name, i) > 0)
			{
				problem = "#VSTR_XMLE_GRP_ERR_NAME_TAKEN";
			}

			if (problem != "")
			{
				SelectWork(i);
				m_Owner.NotifyError(work.Row.Name + ": " + Tr(problem));
				return false;
			}
		}

		return true;
	}

	// Sends every edit in parts; the request id (0 = nothing to send).
	int SendSave()
	{
		array<ref VPPXEGroupEdit> edits = new array<ref VPPXEGroupEdit>;
		foreach (VPPXEGroupWork work : m_File.Work)
		{
			if (!work.IsDirty() || (work.OrigIndex < 0 && (work.Deleted || work.Row.Children.Count() == 0)))
			{
				continue;
			}

			VPPXEGroupEdit edit = new VPPXEGroupEdit();
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
			return 0;
		}

		int reqId = m_Owner.NextReqId();
		array<ref VPPXEGroupsSave> parts = new array<ref VPPXEGroupsSave>;
		VPPXEGroupsSave part = null;
		int partBytes = 0;
		foreach (VPPXEGroupEdit queued : edits)
		{
			int size = VPPXEGroupRules.EstimateRow(queued.Row) + 16;
			if (!part || part.Edits.Count() >= VPPXEGroupRules.EDITS_PER_PART || partBytes + size > VPPXEGroupRules.CHUNK_BYTES)
			{
				part = new VPPXEGroupsSave();
				part.ReqId = reqId;
				part.PartIdx = parts.Count();
				part.FileKey = m_File.Key;
				part.BaseRevision = m_File.Revision;
				parts.Insert(part);
				partBytes = 0;
			}

			part.Edits.Insert(queued);
			partBytes += size;
		}

		int partCount = parts.Count();
		foreach (VPPXEGroupsSave sendPart : parts)
		{
			sendPart.PartCount = partCount;
			GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_SaveEventGroups", new Param1<ref VPPXEGroupsSave>(sendPart), true, null);
		}

		return reqId;
	}

	// ---------------------------------------------------------------- spawn positions that use a group

	// Positions per group name over all of cfgeventspawns.xml (working copy), in one pass.
	protected void CountUses()
	{
		m_UseCounts.Clear();
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		if (!spawns)
		{
			return;
		}

		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group != "")
				{
					m_UseCounts.Set(pos.Group, m_UseCounts.Get(pos.Group) + 1);
				}
			}
		}
	}

	// How many positions of cfgeventspawns.xml (working copy) name the group.
	protected int UseCount(string groupName)
	{
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		int total = 0;
		if (!spawns || groupName == "")
		{
			return 0;
		}

		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group == groupName)
				{
					total++;
				}
			}
		}

		return total;
	}

	// The events whose positions name the group, with the count each: "Event (n), ...".
	protected string UsedByText(string groupName)
	{
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		if (!spawns || groupName == "")
		{
			return "";
		}

		map<string, int> perEvent = new map<string, int>;
		array<string> order = new array<string>;
		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group != groupName)
				{
					continue;
				}

				if (!perEvent.Contains(work.Row.Name))
				{
					order.Insert(work.Row.Name);
				}

				perEvent.Set(work.Row.Name, perEvent.Get(work.Row.Name) + 1);
			}
		}

		string text = "";
		foreach (string eventName : order)
		{
			if (text != "")
			{
				text = text + ", ";
			}

			int uses = perEvent.Get(eventName);
			text = text + eventName + " (" + uses.ToString() + ")";
		}

		return text;
	}

	// Events that use the group but whose spawner ignores groups (Infected / Animal / Ambient / Trajectory).
	protected string IgnoringEvents(string groupName)
	{
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		string text = "";
		if (!spawns || groupName == "")
		{
			return "";
		}

		map<string, bool> seen = new map<string, bool>;
		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted || seen.Contains(work.Row.Name))
			{
				continue;
			}

			if (!VPPXEEventRules.KindIgnoresGroups(VPPXEEventRules.KindOf(work.Row.Name)))
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group == groupName)
				{
					seen.Set(work.Row.Name, true);
					if (text != "")
					{
						text = text + ", ";
					}

					text = text + work.Row.Name;
					break;
				}
			}
		}

		return text;
	}

	// A renamed group keeps its positions: every position naming the old name gets the new one.
	protected int RenameInSpawns(string oldName, string newName)
	{
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		int renamed = 0;
		if (!spawns || oldName == "" || !m_Tab.CanEditSpawns())
		{
			return 0;
		}

		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group == oldName)
				{
					pos.Group = newName;
					renamed++;
				}
			}
		}

		return renamed;
	}

	// ---------------------------------------------------------------- view API (the tab forwards)

	// The view is the one the tab shows (and the map may be drawn).
	void SetVisible(bool visible, bool mapAllowed)
	{
		m_Visible = visible;
		if (m_View)
		{
			m_View.Show(visible);
		}

		bool mapVisible = visible && mapAllowed;
		if (m_Map)
		{
			m_Map.Show(mapVisible);
		}

		if (m_Canvas)
		{
			m_Canvas.Show(mapVisible);
		}

		if (!mapVisible)
		{
			ShowPlayerMarker(false);
			if (m_Renderer)
			{
				m_Renderer.ClearAll();
			}

			return;
		}

		m_Redraw = true;
	}

	MapWidget GetMap()
	{
		return m_Map;
	}

	// The group of the position selected in SPAWNS, with that position as the preview anchor (anchor = x, angle, z).
	// quiet: no message when the group does not exist.
	void OpenGroup(string groupName, vector anchor, float anchorY, string label, bool quiet)
	{
		m_CameFromGroup = groupName;
		m_CameFrom = anchor;
		m_CameFromY = anchorY;
		m_CameFromLabel = label;
		m_AnchorWork = null;
		m_FitPending = true;
		m_EventName = m_Tab.SelectedEventName();
		CollectEventGroups();
		if (!SelectByName(groupName) && !quiet)
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_SPW_W_GROUP_UNKNOWN"), groupName));
		}

		Refresh();
	}

	// Group names the selected event's positions use.
	protected void CollectEventGroups()
	{
		m_EventGroups.Clear();
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		if (!spawns || m_EventName == "")
		{
			return;
		}

		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted || work.Row.Name != m_EventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				if (pos.Group != "")
				{
					m_EventGroups.Set(pos.Group, true);
				}
			}
		}
	}

	// Listed: used by the selected event, or added here and not saved yet.
	protected bool IsListed(VPPXEGroupWork work)
	{
		if (work.Deleted)
		{
			return false;
		}

		return work.OrigIndex < 0 || m_EventGroups.Contains(work.Row.Name);
	}

	string TitleText()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work)
		{
			return Tr("#VSTR_XMLE_GRP_TITLE");
		}

		if (work.OrigIndex < 0)
		{
			return string.Format(Tr("#VSTR_XMLE_GRP_EDIT_NEW"), work.Row.Name);
		}

		return work.Row.Name;
	}

	string LineText()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || work.OrigIndex < 0 || !work.Orig)
		{
			return "";
		}

		return string.Format(Tr("#VSTR_XMLE_MSG_LINE"), work.Orig.Line);
	}

	void OnUpdate(int now)
	{
		if (!m_Visible)
		{
			return;
		}

		PollGroupList();
		PollChildList();
		PollInputs();
		PollMapMouse();
		UpdateMap(now);
	}

	bool OnClick(Widget w)
	{
		if (w == m_BtnAdd)
		{
			StartAdd(null);
			return true;
		}

		if (w == m_BtnDup)
		{
			if (SelectedWork())
			{
				StartAdd(SelectedWork());
			}

			return true;
		}

		if (w == m_BtnRename)
		{
			StartRename();
			return true;
		}

		if (w == m_BtnDelete)
		{
			StartDelete();
			return true;
		}

		if (w == m_BtnAnchor)
		{
			NextAnchor();
			return true;
		}

		if (w == m_BtnBuilder)
		{
			StartBuilder();
			return true;
		}

		if (w == m_BtnSpawnSec)
		{
			ToggleBool(true);
			return true;
		}

		if (w == m_BtnTrace)
		{
			ToggleBool(false);
			return true;
		}

		if (w == m_BtnAddMode)
		{
			if (CanEdit() && SelectedWork())
			{
				m_AddMode = !m_AddMode;
				UpdateDetails();
			}

			return true;
		}

		if (w == m_BtnChildAdd)
		{
			AddChildAt(0, 0, true);
			return true;
		}

		if (w == m_BtnChildRemove)
		{
			RemoveChild();
			return true;
		}

		return false;
	}

	// Everything of the view again (after a load, a save, another event, a change in the SPAWNS view).
	void Refresh()
	{
		string eventName = m_Tab.SelectedEventName();
		if (eventName != m_EventName)
		{
			m_EventName = eventName;
			m_CameFromGroup = "";
			m_AddMode = false;
		}

		CollectEventGroups();
		RebuildList();
		RebuildAnchors();
		LoadInputs();
		RebuildChildList();
		UpdateDetails();
		PushToRenderer();
		m_Tab.OnGroupsViewChanged();
	}

	// ---------------------------------------------------------------- selection

	protected VPPXEGroupWork SelectedWork()
	{
		if (!m_File || m_SelWork < 0 || m_SelWork >= m_File.Work.Count())
		{
			return null;
		}

		VPPXEGroupWork work = m_File.Work[m_SelWork];
		if (work.Deleted)
		{
			return null;
		}

		return work;
	}

	protected VPPXEGroupChild SelectedChild()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || m_ChildSel < 0 || m_ChildSel >= work.Row.Children.Count())
		{
			return null;
		}

		return work.Row.Children[m_ChildSel];
	}

	// The group of that name the server uses (the first with a usable child, else the first); false when none.
	protected bool SelectByName(string groupName)
	{
		if (!m_File || groupName == "")
		{
			return false;
		}

		int found = -1;
		for (int i = 0; i < m_File.Work.Count(); i++)
		{
			VPPXEGroupWork work = m_File.Work[i];
			if (!IsListed(work) || work.Row.Name != groupName)
			{
				continue;
			}

			if (ValidChildCount(work.Row) > 0)
			{
				found = i;
				break;
			}

			if (found < 0)
			{
				found = i;
			}
		}

		if (found < 0)
		{
			return false;
		}

		if (found != m_SelWork)
		{
			m_SelWork = found;
			m_ChildSel = -1;
			m_AddMode = false;
			m_FitPending = true;
		}

		return true;
	}

	protected void SelectWork(int workIdx)
	{
		m_SelWork = workIdx;
		m_ChildSel = -1;
		m_AddMode = false;
		m_FitPending = true;
		Refresh();
	}

	// Other definitions of that name (not removed), skipWork aside.
	protected int NameCount(string groupName, int skipWork)
	{
		int count = 0;
		for (int i = 0; i < m_File.Work.Count(); i++)
		{
			VPPXEGroupWork work = m_File.Work[i];
			if (i != skipWork && !work.Deleted && work.Row.Name == groupName)
			{
				count++;
			}
		}

		return count;
	}

	// An earlier definition of the name wins on the server: this one is ignored.
	protected bool IsShadowed(int workIdx)
	{
		string groupName = m_File.Work[workIdx].Row.Name;
		for (int i = 0; i < workIdx; i++)
		{
			VPPXEGroupWork work = m_File.Work[i];
			if (!work.Deleted && work.Row.Name == groupName && ValidChildCount(work.Row) > 0)
			{
				return true;
			}
		}

		return false;
	}

	// ---------------------------------------------------------------- group list

	protected void RebuildList()
	{
		if (!m_GroupList)
		{
			return;
		}

		m_GroupList.ClearItems();
		m_ListWork.Clear();
		m_ListSel = -1;
		CountUses();
		int keepRow = -1;
		if (m_File)
		{
			// the selection must be one of the listed groups: else the first listed one
			bool selListed = m_SelWork >= 0 && m_SelWork < m_File.Work.Count() && IsListed(m_File.Work[m_SelWork]);
			if (!selListed)
			{
				m_SelWork = -1;
				for (int f = 0; f < m_File.Work.Count(); f++)
				{
					if (IsListed(m_File.Work[f]))
					{
						m_SelWork = f;
						m_ChildSel = -1;
						m_AddMode = false;
						m_FitPending = true;
						break;
					}
				}
			}

			for (int i = 0; i < m_File.Work.Count(); i++)
			{
				VPPXEGroupWork work = m_File.Work[i];
				if (!IsListed(work))
				{
					continue;
				}

				int row = m_GroupList.AddItem("", null, 0);
				m_ListWork.Insert(i);
				FillListRow(row, i);
				if (i == m_SelWork)
				{
					keepRow = row;
				}
			}
		}

		if (keepRow >= 0)
		{
			m_GroupList.SelectRow(keepRow);
			m_GroupList.EnsureVisible(keepRow);
			m_ListSel = keepRow;
		}
		else
		{
			m_SelWork = -1;
		}

		m_TxtGroupsTitle.SetText(string.Format(Tr("#VSTR_XMLE_GRP_LIST_TITLE"), m_ListWork.Count()));
	}

	protected void FillListRow(int row, int workIdx)
	{
		VPPXEGroupWork work = m_File.Work[workIdx];
		string nameText = work.Row.Name;
		if (work.OrigIndex < 0)
		{
			nameText = nameText + " +";
		}
		else if (work.IsDirty())
		{
			nameText = nameText + " *";
		}

		int kids = work.Row.Children.Count();
		m_GroupList.SetItem(row, nameText, null, 0);
		m_GroupList.SetItem(row, kids.ToString(), null, 1);
		int color = ARGB(255, 255, 255, 255);
		if (ValidChildCount(work.Row) == 0 || IsShadowed(workIdx) || !VPPXmlText.IsValidClassName(work.Row.Name))
		{
			color = ARGB(255, 194, 69, 69);
		}
		else if (work.IsDirty())
		{
			color = ARGB(255, 232, 163, 61);
		}
		else if (m_UseCounts.Get(work.Row.Name) == 0)
		{
			color = ARGB(255, 120, 125, 130);
		}

		m_GroupList.SetItemColor(row, 0, color);
		m_GroupList.SetItemColor(row, 1, color);
	}

	protected void RefreshListRow()
	{
		if (m_ListSel >= 0 && SelectedWork())
		{
			FillListRow(m_ListSel, m_SelWork);
		}
	}

	protected void PollGroupList()
	{
		if (!m_GroupList)
		{
			return;
		}

		int row = m_GroupList.GetSelectedRow();
		if (row == m_ListSel)
		{
			return;
		}

		m_ListSel = row;
		int workIdx = -1;
		if (row >= 0 && row < m_ListWork.Count())
		{
			workIdx = m_ListWork[row];
		}

		SelectWork(workIdx);
	}

	// ---------------------------------------------------------------- children

	// Children the server keeps: a readable offset and a type the types files register (when that is known).
	protected int ValidChildCount(VPPXEGroupRow row)
	{
		int count = 0;
		foreach (VPPXEGroupChild child : row.Children)
		{
			if (IsValidChild(child))
			{
				count++;
			}
		}

		return count;
	}

	protected bool IsValidChild(VPPXEGroupChild child)
	{
		if (!child || !VPPXmlText.IsValidClassName(child.Type) || !VPPXEGroupRules.HasOffset(child))
		{
			return false;
		}

		return !m_Tab.IsUnknownType(child.Type);
	}

	protected void RebuildChildList()
	{
		if (!m_ChildList)
		{
			return;
		}

		m_ChildList.ClearItems();
		m_ChildListSel = -1;
		VPPXEGroupWork work = SelectedWork();
		int count = 0;
		if (work)
		{
			count = work.Row.Children.Count();
			for (int i = 0; i < count; i++)
			{
				int row = m_ChildList.AddItem("", null, 0);
				FillChildRow(row, work.Row.Children[i]);
			}
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

		m_TxtChildrenTitle.SetText(string.Format(Tr("#VSTR_XMLE_EVT_CHILDREN"), count));
	}

	protected void FillChildRow(int row, VPPXEGroupChild child)
	{
		string typeText = child.Type;
		if (typeText == "")
		{
			typeText = "?";
		}

		m_ChildList.SetItem(row, typeText, null, 0);
		m_ChildList.SetItem(row, child.X, null, 1);
		m_ChildList.SetItem(row, child.Z, null, 2);
		int color = ARGB(255, 255, 255, 255);
		if (!IsValidChild(child))
		{
			color = ARGB(255, 194, 69, 69);
		}

		for (int column = 0; column < 3; column++)
		{
			m_ChildList.SetItemColor(row, column, color);
		}
	}

	protected void RefreshChildRow()
	{
		VPPXEGroupChild child = SelectedChild();
		if (child && m_ChildListSel >= 0)
		{
			FillChildRow(m_ChildListSel, child);
		}
	}

	protected void PollChildList()
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
		SelectChild(row, true);
	}

	protected void SelectChild(int childIdx, bool centerMap)
	{
		VPPXEGroupWork work = SelectedWork();
		m_ChildSel = childIdx;
		if (!work || childIdx >= work.Row.Children.Count())
		{
			m_ChildSel = -1;
		}

		if (m_ChildList && m_ChildList.GetSelectedRow() != m_ChildSel)
		{
			m_ChildList.SelectRow(m_ChildSel);
			if (m_ChildSel >= 0)
			{
				m_ChildList.EnsureVisible(m_ChildSel);
			}
		}

		m_ChildListSel = m_ChildList.GetSelectedRow();
		LoadInputs();
		UpdateDetails();
		PushToRenderer();
		VPPXEGroupChild child = SelectedChild();
		vector anchor = CurrentAnchor();
		if (centerMap && child && VPPXEGroupRules.HasOffset(child) && m_Map && HasAnchor())
		{
			vector placed = VPPXEGroupRules.Place(child, anchor[0], anchor[2], anchor[1]);
			CentreOn(placed[0], placed[2]);
			ViewMoved();
		}
	}

	// ---------------------------------------------------------------- child editor

	protected void LoadInputs()
	{
		m_Loading = true;
		VPPXEGroupChild child = SelectedChild();
		array<string> values = new array<string>;
		if (child)
		{
			values.Insert(child.Type);
			values.Insert(child.X);
			values.Insert(child.Z);
			values.Insert(child.A);
			values.Insert(child.Y);
			values.Insert(child.LootMin);
			values.Insert(child.LootMax);
			values.Insert(child.Deloot);
		}

		for (int i = 0; i < m_Input.Count(); i++)
		{
			EditBoxWidget box = m_Input[i];
			if (!box)
			{
				continue;
			}

			if (i < values.Count())
			{
				box.SetText(values[i]);
			}
			else
			{
				box.SetText("");
			}

			m_Last.Set(i, box.GetText());
		}

		m_Loading = false;
	}

	protected void PollInputs()
	{
		VPPXEGroupChild child = SelectedChild();
		if (!child || m_Loading)
		{
			return;
		}

		bool changed = false;
		for (int i = 0; i < m_Input.Count(); i++)
		{
			EditBoxWidget box = m_Input[i];
			if (box && box.GetText() != m_Last[i])
			{
				changed = true;
				m_Last.Set(i, box.GetText());
			}
		}

		if (!changed)
		{
			return;
		}

		if (!CanEdit())
		{
			LoadInputs();
			return;
		}

		// the texts go to the child as typed (trimmed); the checks show what the server makes of them and SAVE
		// refuses what it cannot write
		child.Type = TrimmedText(m_Input[IN_TYPE]);
		child.X = TrimmedText(m_Input[IN_X]);
		child.Z = TrimmedText(m_Input[IN_Z]);
		child.A = TrimmedText(m_Input[IN_A]);
		child.Y = TrimmedText(m_Input[IN_Y]);
		child.LootMin = TrimmedText(m_Input[IN_LOOTMIN]);
		child.LootMax = TrimmedText(m_Input[IN_LOOTMAX]);
		child.Deloot = TrimmedText(m_Input[IN_DELOOT]);
		AfterChange(false);
	}

	protected string TrimmedText(EditBoxWidget box)
	{
		if (!box)
		{
			return "";
		}

		string text = box.GetText();
		return text.Trim();
	}

	// spawnsecondary (on unless set off) and trace (off unless set on): turning back to what the file had restores
	// its text, else the default is left out (spawnsecondary) or written as true (trace).
	protected void ToggleBool(bool spawnSecondary)
	{
		VPPXEGroupChild child = SelectedChild();
		if (!child || !CanEdit())
		{
			return;
		}

		VPPXEGroupChild orig = OrigChildOf(child);
		if (spawnSecondary)
		{
			if (VPPXEGroupRules.SpawnSecondaryOn(child))
			{
				child.SpawnSecondary = "false";
				if (orig && !VPPXEGroupRules.SpawnSecondaryOn(orig))
				{
					child.SpawnSecondary = orig.SpawnSecondary;
				}
			}
			else
			{
				child.SpawnSecondary = "";
				if (orig && VPPXEGroupRules.SpawnSecondaryOn(orig))
				{
					child.SpawnSecondary = orig.SpawnSecondary;
				}
			}
		}
		else
		{
			if (VPPXEGroupRules.BoolOf(child.Trace) == 1)
			{
				child.Trace = "";
				if (orig && VPPXEGroupRules.BoolOf(orig.Trace) != 1)
				{
					child.Trace = orig.Trace;
				}
			}
			else
			{
				child.Trace = "true";
				if (orig && VPPXEGroupRules.BoolOf(orig.Trace) == 1)
				{
					child.Trace = orig.Trace;
				}
			}
		}

		AfterChange(false);
	}

	protected VPPXEGroupChild OrigChildOf(VPPXEGroupChild child)
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !work.Orig || child.OrigIdx < 0 || child.OrigIdx >= work.Orig.Children.Count())
		{
			return null;
		}

		return work.Orig.Children[child.OrigIdx];
	}

	// A new child at a world position (the preview anchor turned back into an offset); atAnchor = offset 0, 0.
	protected void AddChildAt(float worldX, float worldZ, bool atAnchor)
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !CanEdit() || work.Row.Children.Count() >= VPPXEGroupRules.MAX_CHILDREN)
		{
			return;
		}

		float offX = 0;
		float offZ = 0;
		vector anchor = CurrentAnchor();
		if (!atAnchor)
		{
			float dx = worldX - anchor[0];
			float dz = worldZ - anchor[2];
			offX = dx;
			offZ = dz;
			if (anchor[1] >= 0)
			{
				// the inverse of VPPXEGroupRules.Place
				float rad = anchor[1] * Math.DEG2RAD;
				float sinA = Math.Sin(rad);
				float cosA = Math.Cos(rad);
				offX = dx * cosA - dz * sinA;
				offZ = dx * sinA + dz * cosA;
			}
		}

		VPPXEGroupChild child = new VPPXEGroupChild();
		child.X = FormatOffset(offX);
		child.Z = FormatOffset(offZ);
		child.A = "0";
		work.Row.Children.Insert(child);
		m_ChildSel = work.Row.Children.Count() - 1;
		RebuildChildList();
		LoadInputs();
		AfterChange(true);
		if (m_Input[IN_TYPE])
		{
			SetFocus(m_Input[IN_TYPE]);
		}
	}

	protected void RemoveChild()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !CanEdit() || !SelectedChild())
		{
			return;
		}

		work.Row.Children.RemoveOrdered(m_ChildSel);
		m_ChildSel = -1;
		RebuildChildList();
		LoadInputs();
		AfterChange(true);
	}

	// After an edit of the selected group: its list rows, checks, map, the tab's action bar.
	protected void AfterChange(bool childListRebuilt)
	{
		if (!childListRebuilt)
		{
			RefreshChildRow();
		}

		RefreshListRow();
		UpdateDetails();
		PushToRenderer();
		m_Tab.OnGroupsEdited(false);
	}

	// ---------------------------------------------------------------- builder (Object Manager)

	// objects one EDIT IN BUILDER lays out
	const static int BUILDER_MAX = 150;

	// A child that goes to the builder: a class name and an offset (a fixed rule, not the types index state; the
	// Object Manager leaves out classes it cannot create and says which were used).
	protected bool IsSendable(VPPXEGroupChild child)
	{
		return child && VPPXmlText.IsValidClassName(child.Type) && VPPXEGroupRules.HasOffset(child);
	}

	// EDIT IN BUILDER: the group is laid out in the Object Manager as local objects (asks first). A group without
	// usable children opens empty there (a new group is built from scratch that way).
	protected void StartBuilder()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !HasAnchor() || !CanEdit())
		{
			return;
		}

		VPPAdminHud hud = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		if (!hud || !hud.HasPermission("MenuObjectManager"))
		{
			m_Owner.NotifyError(Tr("#VSTR_XMLE_BLD_ERR_NO_OM_PERM"));
			return;
		}

		int sendable = 0;
		foreach (VPPXEGroupChild counted : work.Row.Children)
		{
			if (IsSendable(counted))
			{
				sendable++;
			}
		}

		if (sendable > BUILDER_MAX)
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_BLD_ERR_TOO_BIG"), BUILDER_MAX));
			return;
		}

		m_PendBuilder = work;
		string body = string.Format(Tr("#VSTR_XMLE_GRP_DLG_BUILDER_BODY"), work.Row.Name, sendable, m_AnchorLabels[m_AnchorIdx]);
		m_Owner.OpenConfirm("#VSTR_XMLE_GRP_DLG_BUILDER_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmBuilder", false);
	}

	void OnConfirmBuilder(int result, string input)
	{
		VPPXEGroupWork work = m_PendBuilder;
		m_PendBuilder = null;
		if (result != DIAGRESULT.YES || !work || work != SelectedWork() || !HasAnchor())
		{
			return;
		}

		SendToBuilder(work);
	}

	// The usable children laid out at the preview position (as the server would), to the Object Manager as local
	// objects (nothing goes to the server: the events files stay the saved state). Children left out (no offset, a
	// class the game cannot create) are kept as they are when the build comes back.
	protected void SendToBuilder(VPPXEGroupWork work)
	{
		VPPAdminHud hud = VPPAdminHud.Cast(GetVPPUIManager().GetMenuByType(VPPAdminHud));
		if (!hud)
		{
			return;
		}

		vector anchor = CurrentAnchor();
		float anchorA = anchor[1];
		float anchorY = m_AnchorYs[m_AnchorIdx];
		if (anchorY <= VPPXEGroupTransform.UNKNOWN_Y)
		{
			anchorY = GetGame().SurfaceY(anchor[0], anchor[2]);
		}

		vector anchorPos = Vector(anchor[0], anchorY, anchor[2]);
		VPPXEGroupBuild build = new VPPXEGroupBuild();
		build.GroupName = work.Row.Name;
		build.EventName = m_EventName;
		build.AnchorPos = anchorPos;
		build.AnchorA = anchorA;
		build.AnchorLabel = m_AnchorLabels[m_AnchorIdx];
		foreach (VPPXEGroupChild child : work.Row.Children)
		{
			if (!IsSendable(child))
			{
				continue;
			}

			bool onGround = false;
			vector world = VPPXEGroupTransform.WorldPos(child, anchorPos, anchorA, onGround);
			if (onGround)
			{
				world[1] = GetGame().SurfaceY(world[0], world[2]);
			}

			build.Types.Insert(child.Type);
			build.Keys.Insert(VPPXEGroupTransform.SentKey(child));
			build.Positions.Insert(world);
			build.Orientations.Insert(Vector(VPPXEGroupTransform.BuilderYaw(child, anchorA), 0, 0));
		}

		MenuObjectManager objMenu = MenuObjectManager.Cast(hud.GetSubMenuByType(MenuObjectManager));
		if (!objMenu)
		{
			hud.CreateSubMenu(MenuObjectManager);
			objMenu = MenuObjectManager.Cast(hud.GetSubMenuByType(MenuObjectManager));
		}
		else if (!objMenu.IsSubMenuVisible())
		{
			objMenu.ShowSubMenu();
		}

		if (!objMenu)
		{
			return;
		}

		hud.SetWindowPriorty(objMenu);
		objMenu.StartEventGroupBuild(build);
		m_Owner.HideSubMenu();
	}

	// The objects of the local build replace the group's children. Each object is matched to an old child of the same
	// type (nearest offset first): an unchanged one stays exactly as written, a moved one keeps its loot / deloot /
	// spawnsecondary / trace / other attributes and takes the new offset, angle or height; objects without a match
	// are new children, old children without one are removed. Children never sent to the builder stay as they are.
	// counts gets changed, added, removed. "" or the #key of the problem (%1 = the group).
	string ReplaceFromBuilder(string groupName, VPPXEGroupImport build, array<int> counts)
	{
		counts.Clear();
		if (!m_File || !CanEdit())
		{
			return "#VSTR_XMLE_BLD_ERR_READONLY";
		}

		if (build.Children.Count() > VPPXEGroupRules.MAX_CHILDREN)
		{
			return "#VSTR_XMLE_GRP_ERR_TOO_MANY";
		}

		int workIdx = -1;
		for (int w = 0; w < m_File.Work.Count(); w++)
		{
			VPPXEGroupWork candidate = m_File.Work[w];
			if (candidate.Deleted || candidate.Row.Name != groupName)
			{
				continue;
			}

			if (workIdx < 0 || (ValidChildCount(candidate.Row) > 0 && ValidChildCount(m_File.Work[workIdx].Row) == 0))
			{
				workIdx = w;
			}
		}

		if (workIdx < 0)
		{
			return "#VSTR_XMLE_BLD_ERR_GROUP_GONE";
		}

		VPPXEGroupWork work = m_File.Work[workIdx];
		array<ref VPPXEGroupChild> origs = work.Row.Children;
		int origCount = origs.Count();
		int builtCount = build.Children.Count();
		array<int> matchOf = new array<int>;
		array<bool> builtMatched = new array<bool>;
		// which originals became objects (each key once per object; an original edited here since is no candidate)
		map<string, int> sentLeft = new map<string, int>;
		foreach (string sentKey : build.SentKeys)
		{
			sentLeft.Set(sentKey, sentLeft.Get(sentKey) + 1);
		}

		array<bool> wasSent = new array<bool>;
		for (int o = 0; o < origCount; o++)
		{
			matchOf.Insert(-1);
			string origKey = VPPXEGroupTransform.SentKey(origs[o]);
			bool sent = sentLeft.Get(origKey) > 0;
			if (sent)
			{
				sentLeft.Set(origKey, sentLeft.Get(origKey) - 1);
			}

			wasSent.Insert(sent);
		}

		for (int m = 0; m < builtCount; m++)
		{
			builtMatched.Insert(false);
		}

		for (int b = 0; b < builtCount; b++)
		{
			VPPXEGroupChild built = build.Children[b];
			float builtX = built.X.ToFloat();
			float builtZ = built.Z.ToFloat();
			float builtY = built.Y.ToFloat();
			int best = -1;
			float bestDist = 0;
			for (int c = 0; c < origCount; c++)
			{
				VPPXEGroupChild orig = origs[c];
				if (matchOf[c] >= 0 || !wasSent[c] || !VPPXmlText.EqualsNoCase(orig.Type, built.Type))
				{
					continue;
				}

				float dx = orig.X.ToFloat() - builtX;
				float dz = orig.Z.ToFloat() - builtZ;
				float dy = orig.Y.ToFloat() - builtY;
				float dist = dx * dx + dz * dz + dy * dy;
				if (best < 0 || dist < bestDist)
				{
					best = c;
					bestDist = dist;
				}
			}

			if (best >= 0)
			{
				matchOf.Set(best, b);
				builtMatched.Set(b, true);
			}
		}

		int changed = 0;
		int added = 0;
		int removed = 0;
		array<ref VPPXEGroupChild> result = new array<ref VPPXEGroupChild>;
		for (int k = 0; k < origCount; k++)
		{
			VPPXEGroupChild kept = origs[k];
			VPPXEGroupChild merged = new VPPXEGroupChild();
			merged.CopyFrom(kept);
			if (!wasSent[k])
			{
				result.Insert(merged);
				continue;
			}

			int builtIdx = matchOf[k];
			if (builtIdx < 0)
			{
				removed++;
				continue;
			}

			if (MergeBuilt(merged, kept, build.Children[builtIdx], build))
			{
				changed++;
			}

			result.Insert(merged);
		}

		for (int n = 0; n < builtCount; n++)
		{
			if (builtMatched[n])
			{
				continue;
			}

			VPPXEGroupChild fresh = new VPPXEGroupChild();
			fresh.CopyFrom(build.Children[n]);
			fresh.OrigIdx = -1;
			result.Insert(fresh);
			added++;
		}

		if (result.Count() > VPPXEGroupRules.MAX_CHILDREN)
		{
			return "#VSTR_XMLE_GRP_ERR_TOO_MANY";
		}

		work.Row.Children = result;
		m_SelWork = workIdx;
		m_ChildSel = -1;
		m_AddMode = false;
		m_FitPending = true;
		counts.Insert(changed);
		counts.Insert(added);
		counts.Insert(removed);
		return "";
	}

	// merged (a copy of orig) takes what moved in the builder; false when nothing did (it stays as written).
	protected bool MergeBuilt(VPPXEGroupChild merged, VPPXEGroupChild orig, VPPXEGroupChild built, VPPXEGroupImport build)
	{
		bool samePlace = Math.AbsFloat(orig.X.ToFloat() - built.X.ToFloat()) < 0.01 && Math.AbsFloat(orig.Z.ToFloat() - built.Z.ToFloat()) < 0.01;
		// the angle relative to the position, as laid out (a random one, only at a random position, as 0)
		float origAngle = VPPXEGroupRules.AngleOf(orig);
		if (origAngle < 0 && build.AnchorA < 0)
		{
			origAngle = 0;
		}

		float turn = Math.AbsFloat(VPPXEGroupTransform.NormalizeAngle(built.A.ToFloat() - origAngle));
		if (turn > 180)
		{
			turn = 360 - turn;
		}

		bool sameAngle = turn < 0.05;
		// a child on the surface stays there while its object still sits on it
		bool onGround = VPPXEGroupRules.TraceOn(orig);
		vector placed = VPPXEGroupRules.Place(built, build.AnchorPos[0], build.AnchorPos[2], build.AnchorA);
		float worldY = Math.Max(build.AnchorPos[1], 0) + built.Y.ToFloat();
		bool sameHeight = false;
		if (onGround)
		{
			sameHeight = Math.AbsFloat(worldY - GetGame().SurfaceY(placed[0], placed[2])) < 0.15;
		}
		else if (orig.Y != "" && VPPXESpawnRules.IsNumberText(orig.Y))
		{
			sameHeight = Math.AbsFloat(orig.Y.ToFloat() - built.Y.ToFloat()) < 0.01;
		}

		if (samePlace && sameAngle && sameHeight)
		{
			return false;
		}

		if (!samePlace)
		{
			merged.X = built.X;
			merged.Z = built.Z;
		}

		if (!sameAngle)
		{
			merged.A = built.A;
		}

		if (!sameHeight)
		{
			// lifted off the surface: the height is written and trace no longer pulls it down
			merged.Y = built.Y;
			if (VPPXEGroupRules.BoolOf(orig.Trace) == 1)
			{
				merged.Trace = "";
			}
		}

		return true;
	}

	// ---------------------------------------------------------------- group add / duplicate / rename / delete

	// ADD (copy null) and DUPLICATE ask for the name first.
	protected void StartAdd(VPPXEGroupWork copy)
	{
		if (!m_File || !CanEdit() || m_EventName == "")
		{
			return;
		}

		m_PendCopy = copy;
		m_PendIsCopy = copy != null;
		string title = "#VSTR_XMLE_GRP_DLG_ADD_TITLE";
		string body = Tr("#VSTR_XMLE_GRP_DLG_ADD_BODY");
		if (copy)
		{
			title = "#VSTR_XMLE_GRP_DLG_DUP_TITLE";
			body = string.Format(Tr("#VSTR_XMLE_GRP_DLG_DUP_BODY"), copy.Row.Name);
		}

		m_Owner.OpenConfirm(title, body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnAddName", true);
	}

	void OnAddName(int result, string input)
	{
		bool isCopy = m_PendIsCopy;
		VPPXEGroupWork copy = m_PendCopy;
		m_PendCopy = null;
		m_PendIsCopy = false;
		if (result != DIAGRESULT.OK || !m_File || !CanEdit())
		{
			return;
		}

		// a DUPLICATE whose group went away with a reload meanwhile
		if (isCopy && (!copy || m_File.Work.Find(copy) < 0 || copy.Deleted))
		{
			return;
		}

		string newName = VPPXmlText.TrimWs(input);
		if (!CheckNewName(newName))
		{
			return;
		}

		VPPXEGroupWork work = new VPPXEGroupWork();
		if (copy)
		{
			work.Row.CopyFrom(copy.Row);
			foreach (VPPXEGroupChild copied : work.Row.Children)
			{
				copied.OrigIdx = -1;
			}
		}

		work.Row.Name = newName;
		work.Row.Line = 0;
		work.Row.ExtraCount = 0;
		work.Row.HintX = "";
		work.Row.HintZ = "";
		work.Row.HintA = "";
		m_File.Work.Insert(work);
		SelectWork(m_File.Work.Count() - 1);
		if (!copy)
		{
			m_AddMode = true;
			UpdateDetails();
		}

		m_Tab.OnGroupsEdited(true);
	}

	protected bool CheckNewName(string newName)
	{
		if (!VPPXmlText.IsValidClassName(newName))
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_ERR_NAME_INVALID"), newName));
			return false;
		}

		if (NameCount(newName, -1) > 0)
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_GRP_ERR_NAME_TAKEN_FMT"), newName));
			return false;
		}

		return true;
	}

	protected void StartRename()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		string body = string.Format(Tr("#VSTR_XMLE_GRP_DLG_RENAME_BODY"), work.Row.Name);
		m_Owner.OpenConfirm("#VSTR_XMLE_GRP_DLG_RENAME_TITLE", body, DIAGTYPE.DIAG_OK_CANCEL_INPUT, this, "OnRenameName", true);
	}

	void OnRenameName(int result, string input)
	{
		VPPXEGroupWork work = SelectedWork();
		if (result != DIAGRESULT.OK || !work || !CanEdit())
		{
			return;
		}

		string newName = VPPXmlText.TrimWs(input);
		string oldName = work.Row.Name;
		if (newName == oldName || !CheckNewName(newName))
		{
			return;
		}

		// the positions follow only when no other definition of the old name is left to use them
		bool single = NameCount(oldName, m_SelWork) == 0;
		if (single && UseCount(oldName) > 0 && !m_Tab.CanEditSpawns())
		{
			m_Owner.NotifyError(string.Format(Tr("#VSTR_XMLE_GRP_ERR_RENAME_SPAWNS"), oldName));
			return;
		}

		work.Row.Name = newName;
		if (m_CameFromGroup == oldName)
		{
			m_CameFromGroup = newName;
		}

		int moved = 0;
		if (single)
		{
			moved = RenameInSpawns(oldName, newName);
		}

		if (moved > 0)
		{
			m_Owner.Notify(string.Format(Tr("#VSTR_XMLE_GRP_RENAMED_POS"), moved, newName));
		}

		Refresh();
		m_Tab.OnGroupsEdited(true);
	}

	protected void StartDelete()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !CanEdit())
		{
			return;
		}

		int uses = UseCount(work.Row.Name);
		if (uses == 0 || NameCount(work.Row.Name, m_SelWork) > 0)
		{
			DeleteWork(m_SelWork);
			return;
		}

		m_PendDelete = work;
		string body = string.Format(Tr("#VSTR_XMLE_GRP_DLG_DELETE_BODY"), work.Row.Name, uses);
		m_Owner.OpenConfirm("#VSTR_XMLE_GRP_DLG_DELETE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmDelete", false);
	}

	void OnConfirmDelete(int result, string input)
	{
		VPPXEGroupWork work = m_PendDelete;
		m_PendDelete = null;
		if (result != DIAGRESULT.YES || !m_File || !work || work.Deleted || !CanEdit())
		{
			return;
		}

		int workIdx = m_File.Work.Find(work);
		if (workIdx >= 0)
		{
			DeleteWork(workIdx);
		}
	}

	// A group from the file is marked deleted (REVERT brings it back); one added here is dropped.
	protected void DeleteWork(int workIdx)
	{
		VPPXEGroupWork work = m_File.Work[workIdx];
		if (work.OrigIndex < 0)
		{
			m_File.Work.RemoveOrdered(workIdx);
		}
		else
		{
			work.Deleted = true;
		}

		m_SelWork = -1;
		m_ChildSel = -1;
		m_AddMode = false;
		Refresh();
		m_Tab.OnGroupsEdited(true);
	}

	// ---------------------------------------------------------------- preview anchor

	// Where the preview puts the group: the position the view was opened from, every usable position that names the
	// group, the note in the file, the admin's own position; the map centre when there is none of those.
	protected void RebuildAnchors()
	{
		m_Anchors.Clear();
		m_AnchorLabels.Clear();
		m_AnchorYs.Clear();
		VPPXEGroupWork work = SelectedWork();
		if (work != m_AnchorWork)
		{
			m_AnchorWork = work;
			m_AnchorIdx = 0;
		}

		if (!work)
		{
			return;
		}

		string groupName = work.Row.Name;
		if (m_CameFromGroup != "" && m_CameFromGroup == groupName)
		{
			m_Anchors.Insert(m_CameFrom);
			m_AnchorLabels.Insert(m_CameFromLabel);
			m_AnchorYs.Insert(m_CameFromY);
		}

		AddSpawnAnchors(groupName);
		if (VPPXESpawnRules.IsNumberText(work.Row.HintX) && VPPXESpawnRules.IsNumberText(work.Row.HintZ))
		{
			// every argument in a local first: a script call among the arguments zeroes the ones evaluated before it
			float hintX = work.Row.HintX.ToFloat();
			float hintA = AngleText(work.Row.HintA);
			float hintZ = work.Row.HintZ.ToFloat();
			m_Anchors.Insert(Vector(hintX, hintA, hintZ));
			m_AnchorLabels.Insert(Tr("#VSTR_XMLE_GRP_ANCHOR_NOTE"));
			m_AnchorYs.Insert(HeightText(work.Row.HintY));
		}

		Man player = GetGame().GetPlayer();
		if (player)
		{
			vector position = player.GetPosition();
			vector orientation = player.GetOrientation();
			float yaw = orientation[0];
			if (yaw < 0)
			{
				yaw = yaw + 360;
			}

			m_Anchors.Insert(Vector(position[0], Math.Round(yaw), position[2]));
			m_AnchorLabels.Insert(Tr("#VSTR_XMLE_GRP_ANCHOR_ME"));
			m_AnchorYs.Insert(position[1]);
		}

		if (m_Anchors.Count() == 0 && m_Map)
		{
			vector centre = VisibleCentre();
			m_Anchors.Insert(Vector(centre[0], 0, centre[2]));
			m_AnchorLabels.Insert(Tr("#VSTR_XMLE_GRP_ANCHOR_CENTRE"));
			m_AnchorYs.Insert(VPPXEGroupTransform.UNKNOWN_Y);
		}

		if (m_AnchorIdx >= m_Anchors.Count())
		{
			m_AnchorIdx = 0;
		}
	}

	// The usable positions of the selected event that name the group (at most 50), numbered like the SPAWNS list.
	protected void AddSpawnAnchors(string groupName)
	{
		VPPXESpawnFile spawns = m_Tab.GetSpawns();
		if (!spawns)
		{
			return;
		}

		map<string, int> numbers = new map<string, int>;
		foreach (VPPXESpawnWork work : spawns.Work)
		{
			if (work.Deleted || work.Row.Name != m_EventName)
			{
				continue;
			}

			foreach (VPPXESpawnPos pos : work.Row.Positions)
			{
				int number = numbers.Get(work.Row.Name) + 1;
				numbers.Set(work.Row.Name, number);
				if (pos.Group != groupName || !VPPXESpawnRules.IsUsable(pos) || m_Anchors.Count() >= 50)
				{
					continue;
				}

				float posX = pos.X.ToFloat();
				float posZ = pos.Z.ToFloat();
				bool cameFrom = m_CameFromGroup == groupName && Math.AbsFloat(posX - m_CameFrom[0]) < 0.01 && Math.AbsFloat(posZ - m_CameFrom[2]) < 0.01;
				if (cameFrom)
				{
					continue;
				}

				// the angle in a local first: a script call among the arguments zeroes the ones evaluated before it
				// (x came out 0 here, so every group picked in the list was drawn at the west edge of the map)
				float posA = AngleText(pos.A);
				m_Anchors.Insert(Vector(posX, posA, posZ));
				m_AnchorLabels.Insert(work.Row.Name + " #" + number.ToString());
				m_AnchorYs.Insert(HeightText(pos.Y));
			}
		}
	}

	// A height attribute (absolute), UNKNOWN_Y when missing or unreadable (the surface is used then).
	protected float HeightText(string text)
	{
		if (text == "" || !VPPXESpawnRules.IsNumberText(text))
		{
			return VPPXEGroupTransform.UNKNOWN_Y;
		}

		return text.ToFloat();
	}

	// An angle attribute as the server reads it (-1 = random).
	protected float AngleText(string text)
	{
		if (text == "" || !VPPXESpawnRules.IsNumberText(text))
		{
			return -1;
		}

		float angle = text.ToFloat();
		if (angle < 0)
		{
			return -1;
		}

		return angle;
	}

	protected bool HasAnchor()
	{
		return m_AnchorIdx >= 0 && m_AnchorIdx < m_Anchors.Count();
	}

	// x, angle, z of the preview anchor.
	protected vector CurrentAnchor()
	{
		if (!HasAnchor())
		{
			return vector.Zero;
		}

		return m_Anchors[m_AnchorIdx];
	}

	protected void NextAnchor()
	{
		if (m_Anchors.Count() < 2)
		{
			return;
		}

		m_AnchorIdx = m_AnchorIdx + 1;
		if (m_AnchorIdx >= m_Anchors.Count())
		{
			m_AnchorIdx = 0;
		}

		m_FitPending = true;
		UpdateDetails();
		PushToRenderer();
	}

	// ---------------------------------------------------------------- checks and info

	protected void UpdateDetails()
	{
		VPPXEGroupWork work = SelectedWork();
		VPPXEGroupChild child = SelectedChild();
		bool editable = CanEdit();
		bool hasWork = work != null;
		bool hasChild = child != null;
		m_BtnAdd.Enable(editable && m_EventName != "");
		m_BtnDup.Enable(editable && hasWork);
		m_BtnRename.Enable(editable && hasWork);
		m_BtnDelete.Enable(editable && hasWork);
		m_BtnAnchor.Enable(hasWork && m_Anchors.Count() > 1);
		m_BtnBuilder.Enable(hasWork && HasAnchor() && editable);
		for (int i = 0; i < m_Input.Count(); i++)
		{
			m_Tab.SetInputEnabled(m_Input[i], m_Lbl[i], editable && hasChild);
		}

		bool spawnSecOn = true;
		bool traceOn = false;
		bool traceFree = false;
		if (hasChild)
		{
			spawnSecOn = VPPXEGroupRules.SpawnSecondaryOn(child);
			traceOn = VPPXEGroupRules.TraceOn(child);
			traceFree = child.Y != "";
		}

		m_Tab.SetChipLook(m_BtnSpawnSec, m_FillSpawnSec, m_TxtSpawnSec, spawnSecOn, editable && hasChild, false);
		m_Tab.SetChipLook(m_BtnTrace, m_FillTrace, m_TxtTrace, traceOn, editable && hasChild && traceFree, false);
		m_Tab.SetChipLook(m_BtnAddMode, m_FillAddMode, m_TxtAddMode, m_AddMode, editable && hasWork, false);
		bool canAdd = editable && hasWork && work.Row.Children.Count() < VPPXEGroupRules.MAX_CHILDREN;
		m_BtnChildAdd.Enable(canAdd);
		m_BtnChildRemove.Enable(editable && hasChild);
		m_TxtInfo.SetText(InfoText());
		m_Tab.RefreshGroupTitle();
	}

	protected string InfoText()
	{
		array<string> lines = new array<string>;
		if (!m_File || (m_File.ErrorKey != "" && m_File.Key == ""))
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_NO_FILE"));
		}
		else if (m_File.ErrorKey != "")
		{
			lines.Insert(Tr(m_File.ErrorKey));
		}

		VPPXEGroupWork work = SelectedWork();
		if (!work)
		{
			if (m_File && m_File.ErrorKey == "")
			{
				if (m_EventName == "")
				{
					lines.Insert(Tr("#VSTR_XMLE_GRP_PICK_EVENT"));
				}
				else
				{
					lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_NONE_FOR_EVENT"), m_EventName));
				}
			}

			return JoinLines(lines);
		}

		string problem = VPPXEGroupRules.CheckRow(work.Row, work.Orig);
		if (problem != "")
		{
			lines.Insert(Tr(problem));
		}

		if (IsShadowed(m_SelWork))
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_W_SHADOWED"));
		}

		if (ValidChildCount(work.Row) == 0)
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_W_NO_CHILD"));
		}

		map<string, bool> unknown = new map<string, bool>;
		foreach (VPPXEGroupChild child : work.Row.Children)
		{
			if (child && m_Tab.IsUnknownType(child.Type) && !unknown.Contains(child.Type))
			{
				unknown.Set(child.Type, true);
				lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_W_CHILD_UNKNOWN"), child.Type));
			}
		}

		string ignoredBy = IgnoringEvents(work.Row.Name);
		if (ignoredBy != "")
		{
			lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_W_IGNORED_BY"), ignoredBy));
		}

		if (!m_EventGroups.Contains(work.Row.Name))
		{
			lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_W_NOT_THIS_EVENT"), m_EventName));
		}

		string usedBy = UsedByText(work.Row.Name);
		if (usedBy == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_W_UNUSED"));
		}
		else
		{
			lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_INFO_USED"), usedBy));
		}

		if (HasAnchor())
		{
			vector anchor = CurrentAnchor();
			string angleText = Tr("#VSTR_XMLE_GRP_RANDOM");
			if (anchor[1] >= 0)
			{
				int degrees = Math.Round(anchor[1]);
				angleText = degrees.ToString();
			}

			int shown = m_AnchorIdx + 1;
			string place = FormatOffset(anchor[0]) + ", " + FormatOffset(anchor[2]);
			lines.Insert(string.Format(Tr("#VSTR_XMLE_GRP_INFO_ANCHOR"), m_AnchorLabels[m_AnchorIdx], place, angleText, shown, m_Anchors.Count()));
		}

		VPPXEGroupChild selected = SelectedChild();
		if (selected && selected.Y == "")
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_N_NO_Y"));
		}

		if (m_AddMode)
		{
			lines.Insert(Tr("#VSTR_XMLE_GRP_INFO_ADD"));
		}

		return JoinLines(lines);
	}

	protected string JoinLines(array<string> lines)
	{
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

	// ---------------------------------------------------------------- map

	protected void PushToRenderer()
	{
		if (!m_Renderer)
		{
			return;
		}

		array<float> xs = new array<float>;
		array<float> zs = new array<float>;
		array<float> angles = new array<float>;
		array<int> kinds = new array<int>;
		m_DrawnChildren.Clear();
		VPPXEGroupWork work = SelectedWork();
		vector anchor = CurrentAnchor();
		bool hasAnchor = work != null && HasAnchor();
		vector selectedAt = vector.Zero;
		bool hasSelected = false;
		if (hasAnchor)
		{
			for (int i = 0; i < work.Row.Children.Count(); i++)
			{
				VPPXEGroupChild child = work.Row.Children[i];
				if (!VPPXEGroupRules.HasOffset(child))
				{
					continue;
				}

				vector placed = VPPXEGroupRules.Place(child, anchor[0], anchor[2], anchor[1]);
				xs.Insert(placed[0]);
				zs.Insert(placed[2]);
				angles.Insert(placed[1]);
				int kind = VPPXEGroupRenderer.KIND_OK;
				if (!IsValidChild(child))
				{
					kind = VPPXEGroupRenderer.KIND_BAD;
				}

				kinds.Insert(kind);
				m_DrawnChildren.Insert(i);
				if (i == m_ChildSel)
				{
					selectedAt = Vector(placed[0], 0, placed[2]);
					hasSelected = true;
				}
			}
		}

		m_Renderer.SetGroup(hasAnchor, anchor, xs, zs, angles, kinds);
		m_Renderer.SetSelection(selectedAt, hasSelected);
		m_Redraw = true;
		if (m_Map && m_Map.IsVisible())
		{
			DoRedraw(NowMs());
		}
	}

	// Centres the map on the anchor and the children and zooms so they all fit (at least 60 m visible).
	protected bool Fit()
	{
		VPPXEGroupWork work = SelectedWork();
		if (!work || !HasAnchor() || !m_Map || !m_Renderer)
		{
			return true;
		}

		vector anchor = CurrentAnchor();
		float minX = anchor[0];
		float maxX = anchor[0];
		float minZ = anchor[2];
		float maxZ = anchor[2];
		foreach (VPPXEGroupChild child : work.Row.Children)
		{
			if (!VPPXEGroupRules.HasOffset(child))
			{
				continue;
			}

			vector placed = VPPXEGroupRules.Place(child, anchor[0], anchor[2], anchor[1]);
			minX = Math.Min(minX, placed[0]);
			maxX = Math.Max(maxX, placed[0]);
			minZ = Math.Min(minZ, placed[2]);
			maxZ = Math.Max(maxZ, placed[2]);
		}

		if (!m_Renderer.Calibrate() || m_Renderer.GetPixelsPerMetre() <= 0)
		{
			return false;
		}

		// zoom first, then centre (like the MAP tab's focus): the zoom does not keep the centre
		float wantM = Math.Max(Math.Max(maxX - minX, maxZ - minZ) * 1.4, 60);
		float visibleM = Math.Min(m_Renderer.GetCanvasWidth(), m_Renderer.GetCanvasHeight()) / m_Renderer.GetPixelsPerMetre();
		if (visibleM > 1)
		{
			float newScale = m_Map.GetScale() * wantM / visibleM;
			m_Map.SetScale(Math.Clamp(newScale, 0.02, 1.0));
		}

		CentreOn((minX + maxX) * 0.5, (minZ + maxZ) * 0.5);
		ViewMoved();
		return true;
	}

	// The world point in the middle of the map widget (ScreenToMap takes absolute screen pixels).
	protected vector VisibleCentre()
	{
		float cx;
		float cy;
		float cw;
		float ch;
		m_Map.GetScreenPos(cx, cy);
		m_Map.GetScreenSize(cw, ch);
		return m_Map.ScreenToMap(Vector(cx + cw * 0.5, cy + ch * 0.5, 0));
	}

	// Puts a world point in the middle of the map widget. SetMapPos places its point under the widget's internal
	// anchor, which need not be the middle, so the view is moved by the gap between the point and what the middle
	// shows now (the map is a plain translation at a given zoom).
	protected void CentreOn(float worldX, float worldZ)
	{
		vector middle = VisibleCentre();
		vector mapPos = m_Map.GetMapPos();
		m_Map.SetMapPos(Vector(mapPos[0] + worldX - middle[0], 0, mapPos[2] + worldZ - middle[2]));
	}

	// Like the SPAWNS map: a pure pan slides the last drawing (redrawn once the view settles or the slide uncovers too
	// much), a zoom or a resize redraws throttled, a data change at once. The player marker follows every frame.
	protected void UpdateMap(int now)
	{
		if (!m_Map || !m_Map.IsVisible() || !m_Renderer)
		{
			ShowPlayerMarker(false);
			return;
		}

		if (m_FitPending && Fit())
		{
			m_FitPending = false;
		}

		float scale = m_Map.GetScale();
		vector mapPos = m_Map.GetMapPos();
		float cx;
		float cy;
		float cw;
		float ch;
		m_Map.GetScreenPos(cx, cy);
		m_Map.GetScreenSize(cw, ch);
		// what the map really projects now (see m_FramePpm): a zoom or pan still being applied shows up here
		vector probeNow = m_Map.MapToScreen(m_AnchorWorld);
		vector probeEast = m_Map.MapToScreen(Vector(m_AnchorWorld[0] + 100, 0, m_AnchorWorld[2]));
		float ppmNow = (probeEast[0] - probeNow[0]) / 100.0;
		float ppmTolerance = Math.Max(Math.AbsFloat(m_FramePpm) * 0.002, 0.00001);
		bool projMoved = Math.AbsFloat(ppmNow - m_FramePpm) > ppmTolerance || Math.AbsFloat(probeNow[0] - m_FrameProbe[0]) > 0.5 || Math.AbsFloat(probeNow[1] - m_FrameProbe[1]) > 0.5;
		m_FrameProbe = probeNow;
		m_FramePpm = ppmNow;
		float drawnTolerance = Math.Max(Math.AbsFloat(m_AnchorPpm) * 0.002, 0.00001);
		bool zoomedSinceDrawn = Math.AbsFloat(ppmNow - m_AnchorPpm) > drawnTolerance;
		bool viewChanged = scale != m_LastScale || mapPos != m_LastPos || projMoved;
		bool rectChanged = cx != m_LastCX || cy != m_LastCY || cw != m_LastCW || ch != m_LastCH;
		if (viewChanged || rectChanged)
		{
			m_LastScale = scale;
			m_LastPos = mapPos;
			m_LastCX = cx;
			m_LastCY = cy;
			m_LastCW = cw;
			m_LastCH = ch;
			m_SettlePending = true;
			m_LastChange = now;
			if (rectChanged || !m_AnchorValid || scale != m_AnchorScale || zoomedSinceDrawn)
			{
				m_Redraw = true;
			}
			else
			{
				float slideX = probeNow[0] - m_AnchorScreen[0];
				float slideY = probeNow[1] - m_AnchorScreen[1];
				m_Canvas.SetPos(slideX / m_Unit, slideY / m_Unit);
				if (Math.AbsFloat(slideX) > cw * 0.35 || Math.AbsFloat(slideY) > ch * 0.35)
				{
					m_Redraw = true;
				}
			}
		}

		if (m_RedrawAt > 0 && now >= m_RedrawAt)
		{
			m_RedrawAt = 0;
			DoRedraw(now);
		}
		else if (m_Redraw && now - m_LastRedraw >= 50)
		{
			DoRedraw(now);
		}
		else if (m_SettlePending && now - m_LastChange >= 150)
		{
			DoRedraw(now);
		}

		UpdatePlayerMarker();
	}

	// The view was moved by code (fit, centre on a child): no slide of the old drawing (the map may answer with the old
	// view until its next frame), a redraw at once and another one a moment later.
	protected void ViewMoved()
	{
		m_AnchorValid = false;
		m_Redraw = true;
		m_RedrawAt = GetGame().GetTime() + 120;
	}

	protected void DoRedraw(int now)
	{
		m_Canvas.SetPos(0, 0);
		m_Renderer.Redraw();
		m_AnchorWorld = m_Map.GetMapPos();
		m_AnchorScreen = m_Map.MapToScreen(m_AnchorWorld);
		vector drawnEast = m_Map.MapToScreen(Vector(m_AnchorWorld[0] + 100, 0, m_AnchorWorld[2]));
		m_AnchorPpm = (drawnEast[0] - m_AnchorScreen[0]) / 100.0;
		m_FrameProbe = m_AnchorScreen;
		m_FramePpm = m_AnchorPpm;
		m_AnchorScale = m_Map.GetScale();
		m_AnchorValid = true;
		m_LastRedraw = now;
		m_Redraw = false;
		m_SettlePending = false;
	}

	// Left button edges over the map: a press and release within a few pixels is a click (a longer move was a pan).
	protected void PollMapMouse()
	{
		bool down = (GetMouseState(MouseState.LEFT) & MB_PRESSED_MASK) != 0;
		bool pressed = down && !m_MouseWasDown;
		bool released = !down && m_MouseWasDown;
		m_MouseWasDown = down;
		if (!m_Map || !m_Map.IsVisible())
		{
			m_MapPressed = false;
			return;
		}

		int mx;
		int my;
		GetMousePos(mx, my);
		if (pressed)
		{
			m_MapPressed = GetWidgetUnderCursor() == m_Map;
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
		if (moveX * moveX + moveY * moveY > 36 || GetWidgetUnderCursor() != m_Map)
		{
			return;
		}

		OnMapClicked(mx, my);
	}

	protected void OnMapClicked(int mx, int my)
	{
		if (m_AddMode && CanEdit() && SelectedWork() && HasAnchor())
		{
			vector cursor = m_Map.ScreenToMap(Vector(mx, my, 0));
			AddChildAt(cursor[0], cursor[2], false);
			return;
		}

		int hitLayer;
		int hitIndex;
		vector hitPos;
		if (m_Renderer.HitTest(mx, my, hitLayer, hitIndex, hitPos) && hitIndex >= 0 && hitIndex < m_DrawnChildren.Count())
		{
			SelectChild(m_DrawnChildren[hitIndex], false);
			return;
		}

		SelectChild(-1, false);
	}

	// The admin's own character on the map (hidden when off the view or without a character).
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

		if (!player || !m_ImgMe || !m_ImgMeBg || !m_Renderer.ScreenOf(playerPos[0], playerPos[2], sx, sy))
		{
			ShowPlayerMarker(false);
			return;
		}

		m_Renderer.MapRect(mapX, mapY, mapW, mapH);
		if (sx < mapX || sy < mapY || sx > mapX + mapW || sy > mapY + mapH)
		{
			ShowPlayerMarker(false);
			return;
		}

		float bgW;
		float bgH;
		float meW;
		float meH;
		m_ImgMeBg.GetScreenSize(bgW, bgH);
		m_ImgMe.GetScreenSize(meW, meH);
		m_ImgMeBg.SetScreenPos(sx - bgW * 0.5, sy - bgH * 0.5);
		m_ImgMe.SetScreenPos(sx - meW * 0.5, sy - meH * 0.5);
		ShowPlayerMarker(true);
	}

	protected void ShowPlayerMarker(bool show)
	{
		if (m_ImgMe)
		{
			m_ImgMe.Show(show);
		}

		if (m_ImgMeBg)
		{
			m_ImgMeBg.Show(show);
		}
	}

	// ---------------------------------------------------------------- layout

	// The map at the left (most of the room); groups and children lists at its right; the child editor, the child
	// toggles and the info / checks text below.
	void Layout(float rightW, float innerW, float formH, float unit)
	{
		if (!m_View)
		{
			return;
		}

		m_Unit = unit;
		m_View.SetPos(0, 32);
		m_View.SetSize(rightW, formH);
		float bottomH = 18 + ROW_H + 6 + ROW_H + 6 + 58;
		float mapH = formH - bottomH - 2;
		if (mapH < 140)
		{
			mapH = 140;
		}

		float sideW = Math.Clamp(innerW * 0.32, 170, 260);
		float mapW = innerW - sideW - 6;
		float sideX = 10 + mapW + 6;
		m_MapArea.SetPos(10, 0);
		m_MapArea.SetSize(mapW, mapH);
		float listsH = mapH - 20 - 32 - 20 - 4;
		float groupListH = listsH * 0.5;
		m_TxtGroupsTitle.SetPos(sideX, 0);
		m_TxtGroupsTitle.SetSize(sideW, 20);
		m_GroupList.SetPos(sideX, 20);
		m_GroupList.SetSize(sideW, groupListH);
		float buttonY = 20 + groupListH + 4;
		float gap = (sideW - 5 * ROW_H) / 4;
		array<ButtonWidget> groupButtons = new array<ButtonWidget>;
		groupButtons.Insert(m_BtnAdd);
		groupButtons.Insert(m_BtnDup);
		groupButtons.Insert(m_BtnRename);
		groupButtons.Insert(m_BtnDelete);
		groupButtons.Insert(m_BtnAnchor);
		groupButtons.Insert(m_BtnBuilder);
		// six icons across the column (a bit smaller when it is narrow)
		float buttonSize = Math.Min(ROW_H, (sideW - 5 * 4) / 6);
		gap = (sideW - 6 * buttonSize) / 5;
		for (int b = 0; b < groupButtons.Count(); b++)
		{
			ButtonWidget groupButton = groupButtons[b];
			if (!groupButton)
			{
				continue;
			}

			groupButton.SetPos(sideX + b * (buttonSize + gap), buttonY);
			groupButton.SetSize(buttonSize, buttonSize);
		}

		float childTitleY = buttonY + ROW_H + 4;
		m_TxtChildrenTitle.SetPos(sideX, childTitleY);
		m_TxtChildrenTitle.SetSize(sideW, 20);
		m_ChildList.SetPos(sideX, childTitleY + 20);
		m_ChildList.SetSize(sideW, mapH - childTitleY - 20);
		float labelY = mapH + 6;
		float inputY = labelY + 18;
		// the type gets the most room, the numbers share the rest
		float share = (innerW - (INPUT_COUNT - 1) * 6) / (INPUT_COUNT + 1.6);
		float x = 10;
		for (int i = 0; i < INPUT_COUNT; i++)
		{
			float boxW = share;
			if (i == IN_TYPE)
			{
				boxW = share * 2.6;
			}

			TextWidget label = m_Lbl[i];
			EditBoxWidget box = m_Input[i];
			label.SetPos(x, labelY);
			label.SetSize(boxW, 18);
			box.SetPos(x, inputY);
			box.SetSize(boxW, ROW_H);
			x = x + boxW + 6;
		}

		float rowY = inputY + ROW_H + 6;
		float spawnSecW = Math.Clamp(innerW * 0.26, 110, 170);
		float traceW = Math.Clamp(innerW * 0.14, 70, 110);
		float addModeW = Math.Clamp(innerW * 0.24, 100, 170);
		x = 10;
		m_BtnSpawnSec.SetPos(x, rowY);
		m_BtnSpawnSec.SetSize(spawnSecW, ROW_H);
		x = x + spawnSecW + 6;
		m_BtnTrace.SetPos(x, rowY);
		m_BtnTrace.SetSize(traceW, ROW_H);
		x = x + traceW + 6;
		m_BtnAddMode.SetPos(x, rowY);
		m_BtnAddMode.SetSize(addModeW, ROW_H);
		x = x + addModeW + 6;
		m_BtnChildAdd.SetPos(x, rowY);
		m_BtnChildAdd.SetSize(ROW_H, ROW_H);
		x = x + ROW_H + 6;
		m_BtnChildRemove.SetPos(x, rowY);
		m_BtnChildRemove.SetSize(ROW_H, ROW_H);
		float infoY = rowY + ROW_H + 6;
		float infoH = formH - infoY - 2;
		if (infoH < 20)
		{
			infoH = 20;
		}

		m_TxtInfo.SetPos(10, infoY);
		m_TxtInfo.SetSize(innerW, infoH);
		m_Redraw = true;
	}

	// ---------------------------------------------------------------- helpers

	// An offset with three decimals at most (sign kept).
	protected string FormatOffset(float value)
	{
		return VPPXEGroupTransform.FormatNumber(value);
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
