// VPP XML Editor (WP6): MAP tab. Requests the spawn distribution of the selected type, assembles the
// summary and chunks, drives VPPXEMapRenderer over the MapWidget (10 Hz throttled redraws plus a settle
// redraw) and hosts the breakdown drawer: teleport, next instance, live scan and the guarded live delete.
// The map and canvas are hidden whenever the tab is hidden or a dialog is open (MapWidget ignores z-order).

class VPPXEMapTab : ScriptedWidgetEventHandler
{
	const static int REQUEST_DEBOUNCE_MS = 400;
	const static int AUTO_RETRY_MAX = 2;
	const static int REDRAW_MS = 100;
	const static int SETTLE_MS = 150;
	// A same-zoom pan slides the drawn canvas with the map and only redraws once it settles, or earlier when the
	// slide exposes more than this fraction of the view.
	const static float PAN_REDRAW_FRAC = 0.35;
	// Focusing a breakdown entry or a Next instance zooms in until at most this many metres are visible.
	const static float FOCUS_VIEW_M = 600.0;
	const static int TOOLBAR_H = 58;
	const static int TOOLBAR_TOP_H = 30;
	const static int DRAWER_ACTIONS_H = 86;
	const static int ACTION_ROW_H = 26;
	const static int LAYER_CHIP_COUNT = 7;
	// Tier chips follow cfglimitsdefinition <valueflags> (areaflags value plane: one byte, so at most 8).
	const static int TIER_CHIP_MAX = 8;
	const static int TIER_DEFAULT_COUNT = 5;
	const static int TIER_BAR_RESERVED = 128;
	const static int LAYER_TOTAL = 8;
	const static int ALL_TIERS = 255;
	const static int CHIP_LAYERS_MASK = 127;
	const static int DIST_LAYER_MASK = 191;
	const static int DELETE_SEND_MS = 50;
	const static int MAX_DELETE_TRACK = 200;
	const static int MAX_PLAYER_EVENTS = 8;

	protected MenuXMLEditor m_Owner;
	protected Widget m_Root;

	protected Widget m_MapToolbarTop;
	protected TextWidget m_TxtMapType;
	protected Widget m_TierBar;
	protected ref array<Widget> m_BtnTier;
	protected ref array<Widget> m_FillTier;
	protected ref array<TextWidget> m_TxtTier;
	protected ButtonWidget m_BtnRenderMode;
	protected ImageWidget m_ImgRenderMode;
	protected ButtonWidget m_BtnDrawer;
	protected ButtonWidget m_BtnMapRefresh;
	protected ImageWidget m_ImgMapHelp;
	protected Widget m_LayerBar;
	protected ref array<Widget> m_BtnLayer;
	protected ref array<Widget> m_FillLayer;
	protected ref array<TextWidget> m_TxtLayer;

	protected Widget m_MapArea;
	protected MapWidget m_Map;
	protected CanvasWidget m_Canvas;
	protected TextWidget m_TxtMapHud;
	protected TextWidget m_TxtMapLegend;
	protected Widget m_MapBusy;
	protected TextWidget m_TxtMapBusy;
	protected Widget m_MapNotice;
	protected TextWidget m_TxtMapNotice;

	protected Widget m_MapDrawer;
	protected TextWidget m_TxtDrawerSummary;
	protected TextListboxWidget m_BreakdownList;
	protected Widget m_DrawerActions;
	protected ButtonWidget m_BtnTpSelected;
	protected ImageWidget m_ImgTpSelected;
	protected TextWidget m_TxtTpSelected;
	protected ButtonWidget m_BtnNextInstance;
	protected ImageWidget m_ImgNextInstance;
	protected TextWidget m_TxtNextInstance;
	protected ButtonWidget m_BtnLiveScan;
	protected ImageWidget m_ImgLiveScan;
	protected TextWidget m_TxtLiveScan;
	protected ButtonWidget m_BtnDeleteLiveSel;
	protected ImageWidget m_ImgDeleteLiveSel;
	protected TextWidget m_TxtDeleteLiveSel;
	protected ButtonWidget m_BtnDeleteLiveAll;
	protected ImageWidget m_ImgDeleteLiveAll;
	protected TextWidget m_TxtDeleteLiveAll;
	protected Widget m_DrawerModeHost;
	protected ref VPPDropDownMenu m_ModeDD;
	protected ref array<int> m_ModeLayers;
	protected ref array<string> m_TierKeys;
	protected ref array<string> m_TierLabels;
	protected int m_TierCount;
	protected string m_TierSig;
	protected float m_TierBarW;

	protected ref VPPXEMapRenderer m_Renderer;
	protected ref array<ref VPPXEMapLayerData> m_Layers;
	protected ref VPPXEDistSummary m_Summary;

	protected bool m_Shown;
	protected bool m_MapVisible;
	protected bool m_DrawerOpen;
	protected int m_RenderMode;
	protected int m_VisibleMask;
	protected int m_TierMask;
	protected string m_TypeName;
	protected string m_DataType;
	protected bool m_DataDirty;
	protected bool m_HasRequested;
	protected bool m_RequestScheduled;
	protected int m_RequestDue;
	protected ref map<string, int> m_ReqRevs;

	protected int m_DistReqId;
	protected bool m_DistPending;
	protected bool m_SummaryGot;
	protected bool m_DistTimedOut;
	protected int m_DistRetries;
	protected int m_ChunksGot;
	protected int m_ChunksExpected;
	protected int m_LastActivity;
	protected string m_ErrorNotice;

	protected int m_LiveReqId;
	protected bool m_LivePending;
	protected int m_LiveGot;
	protected int m_LiveExpected;
	protected int m_LiveTotal;
	protected bool m_LiveCapped;
	protected bool m_HasLive;
	protected string m_LiveType;
	protected string m_LiveNotice;
	protected int m_LiveGen;

	protected ref array<int> m_PendingDelete;
	protected int m_PendingDeleteGen;
	protected string m_PendingDeleteType;
	protected ref array<ref array<int>> m_DelQueue;
	protected ref array<ref array<int>> m_DelQueueMarkers;
	protected ref array<string> m_DelQueueType;
	protected ref array<int> m_DelQueueGen;
	protected ref array<int> m_DelReqIds;
	protected ref array<ref array<int>> m_DelPartMarkers;
	protected ref array<int> m_DelPartGen;
	protected bool m_DelPumpScheduled;

	protected float m_LastScale;
	protected vector m_LastMapPos;
	protected float m_LastCX;
	protected float m_LastCY;
	protected float m_LastCW;
	protected float m_LastCH;
	protected bool m_ViewDirty;
	protected bool m_SettlePending;
	protected bool m_RedrawNow;
	protected int m_LastChange;
	protected int m_LastRedraw;
	protected bool m_PanAnchorValid;
	protected vector m_PanAnchorScreen;
	protected float m_PanAnchorScale;
	protected string m_NextLabelKey;
	protected float m_LastRootW;
	protected float m_LastRootH;
	protected float m_Unit;
	protected float m_ChipW;
	protected float m_TypeTextW;
	protected float m_AreaW;
	protected float m_DrawerW;
	protected float m_DrawerH;

	protected int m_SelLayer;
	protected int m_SelIndex;
	protected bool m_HasSelPos;
	protected vector m_SelPos;
	protected string m_HudItem;
	protected string m_LastHudText;
	protected int m_ModeLayer;
	protected int m_BreakdownSel;
	protected ref array<int> m_RowMap;
	protected int m_NextCursor;
	protected int m_NextOrderLayer;
	protected ref array<int> m_NextOrder;
	protected ref array<string> m_NoticeParts;
	protected ref array<string> m_SummaryParts;
	protected float m_BbMinX;
	protected float m_BbMaxX;
	protected float m_BbMinZ;
	protected float m_BbMaxZ;
	protected bool m_BbAny;

	void VPPXEMapTab(Widget host, MenuXMLEditor owner)
	{
		m_Owner = owner;
		m_Layers = new array<ref VPPXEMapLayerData>;
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			m_Layers.Insert(new VPPXEMapLayerData());
		}

		m_BtnTier = new array<Widget>;
		m_FillTier = new array<Widget>;
		m_TxtTier = new array<TextWidget>;
		m_BtnLayer = new array<Widget>;
		m_FillLayer = new array<Widget>;
		m_TxtLayer = new array<TextWidget>;
		m_ModeLayers = new array<int>;
		m_TierKeys = {"#VSTR_XMLE_TIER_1", "#VSTR_XMLE_TIER_2", "#VSTR_XMLE_TIER_3", "#VSTR_XMLE_TIER_4", "#VSTR_XMLE_TIER_U"};
		m_TierLabels = new array<string>;
		m_TierCount = 0;
		m_TierSig = "";
		m_ReqRevs = new map<string, int>;
		m_PendingDelete = new array<int>;
		m_DelQueue = new array<ref array<int>>;
		m_DelQueueMarkers = new array<ref array<int>>;
		m_DelQueueType = new array<string>;
		m_DelQueueGen = new array<int>;
		m_DelReqIds = new array<int>;
		m_DelPartMarkers = new array<ref array<int>>;
		m_DelPartGen = new array<int>;
		m_RowMap = new array<int>;
		m_NextOrder = new array<int>;
		m_NoticeParts = new array<string>;
		m_SummaryParts = new array<string>;

		m_VisibleMask = CHIP_LAYERS_MASK;
		m_TierMask = ALL_TIERS;
		m_RenderMode = VPPXEMapRenderer.MODE_HEAT;
		m_ChunksExpected = -1;
		m_LiveExpected = -1;
		m_SelLayer = -1;
		m_SelIndex = -1;
		m_ModeLayer = -1;
		m_BreakdownSel = -1;
		m_NextCursor = -1;
		m_NextOrderLayer = -1;
		m_Unit = 1.0;
		m_ChipW = 90;
		m_TypeTextW = 220;
		m_AreaW = 600;

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.XMLEditorMapTab, host);
		m_Root.SetHandler(this);
		BindWidgets();

		m_Renderer = new VPPXEMapRenderer(m_Map, m_Canvas);
		m_Renderer.SetData(null, m_Layers);
		m_Renderer.SetVisibleLayers(m_VisibleMask);
		m_Renderer.SetTierFilter(m_TierMask);
		m_Renderer.SetMode(m_RenderMode);

		m_ModeDD = new VPPDropDownMenu(m_DrawerModeHost, "");
		m_ModeDD.m_OnSelectItem.Insert(OnModeSelected);

		ToolTipHandler helpTip;
		if (m_ImgMapHelp)
		{
			m_ImgMapHelp.GetScript(helpTip);
		}

		if (helpTip)
		{
			helpTip.SetTitle("#VSTR_TOOLTIP_TITLE");
			helpTip.SetContentText("#VSTR_XMLE_TOOLTIP_MAP");
		}

		if (m_ImgRenderMode)
		{
			m_ImgRenderMode.LoadImageFile(0, "set:vpp_icons image:flame");
			m_ImgRenderMode.LoadImageFile(1, "set:vpp_icons image:map_pin");
			m_ImgRenderMode.SetImage(0);
		}

		m_MapDrawer.Show(false);
		m_MapBusy.Show(false);
		m_MapNotice.Show(false);
		m_TxtMapLegend.SetText(Tr("#VSTR_XMLE_MAP_LEGEND"));
		SetTypeText("");
		SetMapVisible(false);
		UpdateTierChips();
		RebuildTierChips();
		UpdateLayerChips();
		UpdateActionButtons();
		RefreshNotice();

		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnDistSummary", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnDistChunk", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("RPC_MenuXMLEditor", "XE_OnLiveChunk", this, SingleplayerExecutionType.Client);
	}

	void ~VPPXEMapTab()
	{
		if (GetGame())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.PumpDeleteQueue);
		}

		m_DelPumpScheduled = false;
		if (m_Renderer)
		{
			m_Renderer.ClearAll();
		}
	}

	protected void BindWidgets()
	{
		m_MapToolbarTop = m_Root.FindAnyWidget("MapToolbarTop");
		m_TxtMapType = TextWidget.Cast(m_Root.FindAnyWidget("TxtMapType"));
		m_TierBar = m_Root.FindAnyWidget("TierBar");
		m_BtnRenderMode = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnRenderMode"));
		m_ImgRenderMode = ImageWidget.Cast(m_Root.FindAnyWidget("ImgRenderMode"));
		m_BtnDrawer = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDrawer"));
		m_BtnMapRefresh = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnMapRefresh"));
		m_ImgMapHelp = ImageWidget.Cast(m_Root.FindAnyWidget("ImgMapHelp"));
		m_LayerBar = m_Root.FindAnyWidget("LayerBar");

		m_MapArea = m_Root.FindAnyWidget("MapArea");
		m_Map = MapWidget.Cast(m_Root.FindAnyWidget("XeMapWidget"));
		m_Canvas = CanvasWidget.Cast(m_Root.FindAnyWidget("XeMapCanvas"));
		m_TxtMapHud = TextWidget.Cast(m_Root.FindAnyWidget("TxtMapHud"));
		m_TxtMapLegend = TextWidget.Cast(m_Root.FindAnyWidget("TxtMapLegend"));
		m_MapBusy = m_Root.FindAnyWidget("MapBusy");
		m_TxtMapBusy = TextWidget.Cast(m_Root.FindAnyWidget("TxtMapBusy"));
		m_MapNotice = m_Root.FindAnyWidget("MapNotice");
		m_TxtMapNotice = TextWidget.Cast(m_Root.FindAnyWidget("TxtMapNotice"));

		m_MapDrawer = m_Root.FindAnyWidget("MapDrawer");
		m_TxtDrawerSummary = TextWidget.Cast(m_Root.FindAnyWidget("TxtDrawerSummary"));
		m_BreakdownList = TextListboxWidget.Cast(m_Root.FindAnyWidget("BreakdownList"));
		m_DrawerActions = m_Root.FindAnyWidget("DrawerActions");
		m_BtnTpSelected = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnTpSelected"));
		m_ImgTpSelected = ImageWidget.Cast(m_Root.FindAnyWidget("ImgTpSelected"));
		m_TxtTpSelected = TextWidget.Cast(m_Root.FindAnyWidget("TxtTpSelected"));
		m_BtnNextInstance = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnNextInstance"));
		m_ImgNextInstance = ImageWidget.Cast(m_Root.FindAnyWidget("ImgNextInstance"));
		m_TxtNextInstance = TextWidget.Cast(m_Root.FindAnyWidget("TxtNextInstance"));
		m_BtnLiveScan = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnLiveScan"));
		m_ImgLiveScan = ImageWidget.Cast(m_Root.FindAnyWidget("ImgLiveScan"));
		m_TxtLiveScan = TextWidget.Cast(m_Root.FindAnyWidget("TxtLiveScan"));
		m_BtnDeleteLiveSel = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDeleteLiveSel"));
		m_ImgDeleteLiveSel = ImageWidget.Cast(m_Root.FindAnyWidget("ImgDeleteLiveSel"));
		m_TxtDeleteLiveSel = TextWidget.Cast(m_Root.FindAnyWidget("TxtDeleteLiveSel"));
		m_BtnDeleteLiveAll = ButtonWidget.Cast(m_Root.FindAnyWidget("BtnDeleteLiveAll"));
		m_ImgDeleteLiveAll = ImageWidget.Cast(m_Root.FindAnyWidget("ImgDeleteLiveAll"));
		m_TxtDeleteLiveAll = TextWidget.Cast(m_Root.FindAnyWidget("TxtDeleteLiveAll"));
		m_DrawerModeHost = m_Root.FindAnyWidget("DrawerModeHost");

		for (int ti = 0; ti < TIER_CHIP_MAX; ti++)
		{
			string ts = ti.ToString();
			m_BtnTier.Insert(m_Root.FindAnyWidget("BtnTier" + ts));
			m_FillTier.Insert(m_Root.FindAnyWidget("FillTier" + ts));
			m_TxtTier.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtTier" + ts)));
		}

		array<string> layerSuffix = {"Buildings", "EventObj", "EventChild", "Infected", "Clusters", "Dispatch", "Live"};
		foreach (string ls : layerSuffix)
		{
			m_BtnLayer.Insert(m_Root.FindAnyWidget("BtnLayer" + ls));
			m_FillLayer.Insert(m_Root.FindAnyWidget("FillLayer" + ls));
			m_TxtLayer.Insert(TextWidget.Cast(m_Root.FindAnyWidget("TxtLayer" + ls)));
		}

		// Legend: each layer chip carries a swatch in its draw colour, the HUD a low-to-high density ramp.
		for (int li = 0; li < layerSuffix.Count(); li++)
		{
			Widget layerSwatch = m_Root.FindAnyWidget("SwatchLayer" + layerSuffix[li]);
			if (layerSwatch)
			{
				layerSwatch.SetColor(VPPXEMapRenderer.LayerColor(li));
			}
		}

		for (int step = 0; step < 4; step++)
		{
			Widget rampSwatch = m_Root.FindAnyWidget("LegendStep" + step.ToString());
			if (rampSwatch)
			{
				rampSwatch.SetColor(VPPXEMapRenderer.RampColor(step, false, 235));
			}
		}
	}

	// ---------------------------------------------------------------- tab API (INTERFACES section 6)

	void Show(bool show)
	{
		m_Shown = show;
		if (!show)
		{
			SetMapVisible(false);
			return;
		}

		if (m_Owner && !m_Owner.IsDialogOpen())
		{
			SetMapVisible(true);
		}

		// OnUpdate does not run while hidden: an expired distribution request is dropped silently and asked again,
		// an expired live scan is only dropped (a scan is never repeated without its confirmation)
		int now = NowMs();
		bool distExpired = m_DistPending && now - m_LastActivity > VPPXEConst.INFLIGHT_TIMEOUT_MS;
		if (m_LivePending && now - m_LastActivity > VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			m_LivePending = false;
		}

		if (distExpired)
		{
			m_DistPending = false;
		}

		HideBusyIfIdle();
		OnResize();
		UpdateTierChips();
		UpdateLayerChips();
		UpdateActionButtons();
		RefreshNotice();
		if (m_TypeName == "" || !HasPerm(VPPXEPerm.DIST_MAP))
		{
			return;
		}

		if (m_DataDirty || m_DataType != m_TypeName || distExpired || m_DistTimedOut)
		{
			ScheduleRequest(0);
		}
	}

	// True while a distribution or live-scan reply is awaited (bounded by the in-flight timeout, since OnUpdate
	// does not expire it while the tab is hidden) or live-delete parts are still queued. The window holds
	// XE_CloseSession back while this is true.
	bool HasInFlight()
	{
		if (m_DelQueue.Count() > 0)
		{
			return true;
		}

		if (!m_DistPending && !m_LivePending)
		{
			return false;
		}

		return NowMs() - m_LastActivity <= VPPXEConst.INFLIGHT_TIMEOUT_MS;
	}

	void OnResize()
	{
		if (!m_Root)
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
		float w = rootW / m_Unit;
		float h = rootH / m_Unit;
		LayoutToolbar(w);

		float areaW = w;
		if (m_DrawerOpen)
		{
			areaW = Math.Floor(w * 0.70);
		}

		float bodyH = h - TOOLBAR_H;
		if (bodyH < 40)
		{
			bodyH = 40;
		}

		m_AreaW = areaW;
		m_MapArea.SetPos(0, TOOLBAR_H);
		m_MapArea.SetSize(areaW, bodyH);
		if (m_DrawerOpen)
		{
			m_DrawerW = w - areaW - 4;
			m_DrawerH = bodyH;
			m_MapDrawer.SetPos(areaW + 4, TOOLBAR_H);
			m_MapDrawer.SetSize(m_DrawerW, m_DrawerH);
			LayoutDrawer();
		}

		LayoutNotice();
		RequestRedraw();
	}

	void OnUpdate(float timeslice)
	{
		if (!m_Shown)
		{
			return;
		}

		int now = NowMs();
		CheckRootResize();
		if (m_RequestScheduled && now >= m_RequestDue)
		{
			SendDistRequest();
		}

		CheckTimeouts(now);
		PollBreakdownSelection();
		if (!m_MapVisible || !m_Map || !m_Canvas)
		{
			return;
		}

		PollView(now);
		if (m_RedrawNow)
		{
			DoRedraw(now);
		}
		else if (m_ViewDirty && now - m_LastRedraw >= REDRAW_MS)
		{
			DoRedraw(now);
		}
		else if (m_SettlePending && now - m_LastChange >= SETTLE_MS)
		{
			DoRedraw(now);
			m_SettlePending = false;
		}

		UpdateHud();
	}

	// Re-requests only when a TYPES file revision changed since the last request (a save changed the data) or
	// the last distribution request timed out.
	void OnSessionChanged()
	{
		RebuildTierChips();
		UpdateTierChips();
		UpdateActionButtons();
		RefreshNotice();
		if (m_TypeName == "" || !m_HasRequested || (!RevisionsChanged() && !m_DistTimedOut))
		{
			return;
		}

		if (m_Shown && HasPerm(VPPXEPerm.DIST_MAP))
		{
			ScheduleRequest(REQUEST_DEBOUNCE_MS);
		}
		else
		{
			m_DataDirty = true;
		}
	}

	void Refresh()
	{
		if (m_TypeName == "" || !HasPerm(VPPXEPerm.DIST_MAP))
		{
			return;
		}

		m_DistRetries = 0;
		SendDistRequest();
	}

	void OnTypeSelected(string typeName)
	{
		if (typeName == m_TypeName)
		{
			return;
		}

		m_TypeName = typeName;
		m_DistRetries = 0;
		m_DistTimedOut = false;
		SetTypeText(typeName);
		if (m_LiveType != "" && m_LiveType != typeName)
		{
			ClearLive();
		}

		if (typeName == "")
		{
			m_RequestScheduled = false;
			m_DistPending = false;
			m_DataType = "";
			HideBusyIfIdle();
			ClearDistData();
			return;
		}

		if (m_Shown && HasPerm(VPPXEPerm.DIST_MAP))
		{
			ScheduleRequest(REQUEST_DEBOUNCE_MS);
		}
		else
		{
			m_DataDirty = true;
		}

		UpdateActionButtons();
		RefreshNotice();
	}

	// Called by MenuXMLEditor (OpenConfirm, HideBrokenWidgets, dialog close) and by Show.
	void SetMapVisible(bool visible)
	{
		m_MapVisible = visible;
		if (m_Map)
		{
			m_Map.Show(visible);
		}

		if (m_Canvas)
		{
			m_Canvas.Show(visible);
		}

		if (!visible)
		{
			if (m_Renderer)
			{
				m_Renderer.ClearAll();
			}

			return;
		}

		m_RedrawNow = true;
	}

	bool HandleProgress(VPPXEProgress p)
	{
		if (!p || p.ReqId <= 0)
		{
			return false;
		}

		bool distOwn = m_DistPending && p.ReqId == m_DistReqId;
		bool liveOwn = m_LivePending && p.ReqId == m_LiveReqId;
		if (!distOwn && !liveOwn)
		{
			return false;
		}

		m_LastActivity = NowMs();
		int percent = p.Percent;
		string pct = percent.ToString() + "%";
		string stage = Tr(VPPXEText.StageKey(p.Stage));
		ShowBusy(string.Format(Tr("#VSTR_XMLE_PROGRESS_FMT"), stage, pct));
		return true;
	}

	bool HandleActionResult(int reqId, bool ok, string key, string arg)
	{
		if (reqId <= 0)
		{
			return false;
		}

		string message = string.Format(Tr(key), arg);
		if (reqId == m_DistReqId)
		{
			if (!ok)
			{
				m_DistPending = false;
				if (key == "#VSTR_XMLE_ERR_NOT_READY")
				{
					m_DistTimedOut = true;
				}

				m_ErrorNotice = message;
				HideBusyIfIdle();
				RefreshNotice();
			}

			return true;
		}

		if (reqId == m_LiveReqId)
		{
			if (!ok)
			{
				m_LivePending = false;
				m_ErrorNotice = message;
				HideBusyIfIdle();
				UpdateActionButtons();
				RefreshNotice();
			}

			return true;
		}

		int part = m_DelReqIds.Find(reqId);
		if (part < 0)
		{
			return false;
		}

		if (!ok)
		{
			m_Owner.NotifyError(message);
			return true;
		}

		m_Owner.Notify(message);
		if (key == "#VSTR_XMLE_MAP_DELETED")
		{
			ApplyDeleted(part, arg.ToInt());
		}

		return true;
	}

	// ---------------------------------------------------------------- server-to-client receivers (section 4)

	void XE_OnDistSummary(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEDistSummary> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		VPPXEDistSummary summary = data.param1;
		if (!summary || !m_DistPending || summary.ReqId != m_DistReqId)
		{
			return;
		}

		m_Summary = summary;
		m_SummaryGot = true;
		m_ChunksExpected = summary.ChunkCount;
		m_LastActivity = NowMs();
		TryFinishDist();
	}

	void XE_OnDistChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXEDistChunk> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		VPPXEDistChunk chunk = data.param1;
		if (!chunk || !m_DistPending || chunk.ReqId != m_DistReqId)
		{
			return;
		}

		m_LastActivity = NowMs();
		m_ChunksGot++;
		if (!m_SummaryGot && chunk.ChunkCount > m_ChunksExpected)
		{
			m_ChunksExpected = chunk.ChunkCount;
		}

		if (chunk.Layer >= 0 && chunk.Layer < LAYER_TOTAL && chunk.Layer != VPPXELayer.LIVE)
		{
			AppendDistChunk(m_Layers[chunk.Layer], chunk);
		}

		TryFinishDist();
	}

	void XE_OnLiveChunk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref VPPXELiveChunk> data;
		if (type != CallType.Client || !ctx.Read(data))
		{
			return;
		}

		VPPXELiveChunk chunk = data.param1;
		if (!chunk || !m_LivePending || chunk.ReqId != m_LiveReqId)
		{
			return;
		}

		m_LastActivity = NowMs();
		VPPXEMapLayerData live = m_Layers[VPPXELayer.LIVE];
		if (chunk.Coords)
		{
			int pairs = chunk.Coords.Count() / 2;
			for (int i = 0; i < pairs; i++)
			{
				int markerIdx = live.MarkMeta.Count();
				int rowNumber = markerIdx + 1;
				float wx = chunk.Coords[i * 2];
				float wz = chunk.Coords[i * 2 + 1];
				live.MarkX.Insert(wx);
				live.MarkZ.Insert(wz);
				live.MarkMeta.Insert(rowNumber << 8);
				int netLow = 0;
				int netHigh = 0;
				if (chunk.Net && chunk.Net.Count() > i * 2 + 1)
				{
					netLow = chunk.Net[i * 2];
					netHigh = chunk.Net[i * 2 + 1];
				}

				live.MarkNet.Insert(netLow);
				live.MarkNet.Insert(netHigh);
				int roundX = Math.Round(wx);
				int roundZ = Math.Round(wz);
				string coords = string.Format(Tr("#VSTR_XMLE_MAP_HUD_FMT"), roundX, roundZ);
				VPPXEDistTableRow liveRow = new VPPXEDistTableRow();
				liveRow.Layer = VPPXELayer.LIVE;
				liveRow.Label = rowNumber.ToString() + "  " + coords;
				liveRow.Count = 1;
				liveRow.X = wx;
				liveRow.Z = wz;
				live.Rows.Insert(liveRow);
			}
		}

		if (chunk.Total > m_LiveTotal)
		{
			m_LiveTotal = chunk.Total;
		}

		if (chunk.Capped)
		{
			m_LiveCapped = true;
		}

		m_LiveGot++;
		m_LiveExpected = chunk.ChunkCount;
		if (m_LiveGot >= m_LiveExpected)
		{
			FinishLive();
		}
	}

	// ---------------------------------------------------------------- widget events

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != MouseState.LEFT)
		{
			return false;
		}

		if (w == m_Map)
		{
			OnMapClicked();
			return true;
		}

		int layerIdx = m_BtnLayer.Find(w);
		if (layerIdx >= 0)
		{
			ToggleLayer(layerIdx);
			return true;
		}

		int tierIdx = m_BtnTier.Find(w);
		if (tierIdx >= 0)
		{
			ToggleTier(tierIdx);
			return true;
		}

		if (w == m_BtnRenderMode)
		{
			ToggleRenderMode();
			return true;
		}

		if (w == m_BtnDrawer)
		{
			SetDrawerOpen(!m_DrawerOpen);
			return true;
		}

		if (w == m_BtnMapRefresh)
		{
			Refresh();
			return true;
		}

		if (w == m_BtnTpSelected)
		{
			TeleportToSelection();
			return true;
		}

		if (w == m_BtnNextInstance)
		{
			CycleNext();
			return true;
		}

		if (w == m_BtnLiveScan)
		{
			BeginLiveScan();
			return true;
		}

		if (w == m_BtnDeleteLiveSel)
		{
			BeginDeleteLive(false);
			return true;
		}

		if (w == m_BtnDeleteLiveAll)
		{
			BeginDeleteLive(true);
			return true;
		}

		return false;
	}

	// Double-click on the map teleports to the hit marker, else to the point under the cursor.
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w != m_Map || button != MouseState.LEFT)
		{
			return false;
		}

		if (!HasPerm(VPPXEPerm.TELEPORT))
		{
			return true;
		}

		int mx;
		int my;
		GetMousePos(mx, my);
		int hitLayer;
		int hitIndex;
		vector hitPos;
		if (m_Renderer.HitTest(mx, my, hitLayer, hitIndex, hitPos) && hitIndex >= 0 && hitLayer != VPPXELayer.INFECTED)
		{
			DoTeleport(hitPos);
			return true;
		}

		vector cursorPos = m_Map.ScreenToMap(Vector(mx, my, 0));
		DoTeleport(cursorPos);
		return true;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		return false;
	}

	// ---------------------------------------------------------------- dialog and dropdown callbacks

	void OnConfirmScan(int result, string input)
	{
		if (result == DIAGRESULT.YES)
		{
			SendLiveScan();
		}
	}

	void OnConfirmDeleteLive(int result, string input)
	{
		if (result != DIAGRESULT.YES || m_PendingDeleteGen != m_LiveGen)
		{
			m_PendingDelete.Clear();
			return;
		}

		// markers of this scan that are already queued or sent (a second confirmation while a chain runs) are skipped
		map<int, bool> taken = new map<int, bool>;
		for (int q = 0; q < m_DelQueueMarkers.Count(); q++)
		{
			array<int> queuedMarkers = m_DelQueueMarkers[q];
			if (!queuedMarkers || m_DelQueueGen[q] != m_PendingDeleteGen)
			{
				continue;
			}

			foreach (int queuedIdx : queuedMarkers)
			{
				taken.Set(queuedIdx, true);
			}
		}

		for (int s = 0; s < m_DelPartMarkers.Count(); s++)
		{
			array<int> sentMarkers = m_DelPartMarkers[s];
			if (!sentMarkers || m_DelPartGen[s] != m_PendingDeleteGen)
			{
				continue;
			}

			foreach (int sentIdx : sentMarkers)
			{
				taken.Set(sentIdx, true);
			}
		}

		VPPXEMapLayerData live = m_Layers[VPPXELayer.LIVE];
		array<int> pairs;
		array<int> markers;
		foreach (int markerIdx : m_PendingDelete)
		{
			if (taken.Contains(markerIdx) || !live.IsMarkerAlive(markerIdx) || live.MarkNet.Count() <= markerIdx * 2 + 1)
			{
				continue;
			}

			taken.Set(markerIdx, true);
			if (!pairs || markers.Count() >= VPPXEConst.DELETE_PER_REQUEST)
			{
				pairs = new array<int>;
				markers = new array<int>;
				m_DelQueue.Insert(pairs);
				m_DelQueueMarkers.Insert(markers);
				m_DelQueueType.Insert(m_PendingDeleteType);
				m_DelQueueGen.Insert(m_PendingDeleteGen);
			}

			pairs.Insert(live.MarkNet[markerIdx * 2]);
			pairs.Insert(live.MarkNet[markerIdx * 2 + 1]);
			markers.Insert(markerIdx);
		}

		m_PendingDelete.Clear();
		if (!m_DelPumpScheduled)
		{
			PumpDeleteQueue();
		}
	}

	void OnModeSelected(int index)
	{
		if (index < 0 || index >= m_ModeLayers.Count())
		{
			return;
		}

		m_ModeLayer = m_ModeLayers[index];
		m_ModeDD.SetText(Tr(VPPXEText.LayerKey(m_ModeLayer)));
		m_ModeDD.SetIndex(index);
		m_ModeDD.Close();
		m_NextCursor = -1;
		RebuildBreakdown();
		UpdateActionButtons();
	}

	// Sends one DELETE_PER_REQUEST part; parts are paced at 2 per 100 ms (CallLater 50 ms).
	void PumpDeleteQueue()
	{
		m_DelPumpScheduled = false;
		if (m_DelQueue.Count() == 0 || !m_Owner)
		{
			return;
		}

		array<int> pairs = m_DelQueue[0];
		array<int> markers = m_DelQueueMarkers[0];
		string typeName = m_DelQueueType[0];
		int gen = m_DelQueueGen[0];
		m_DelQueue.RemoveOrdered(0);
		m_DelQueueMarkers.RemoveOrdered(0);
		m_DelQueueType.RemoveOrdered(0);
		m_DelQueueGen.RemoveOrdered(0);

		int reqId = m_Owner.NextReqId();
		m_DelReqIds.Insert(reqId);
		m_DelPartMarkers.Insert(markers);
		m_DelPartGen.Insert(gen);
		while (m_DelReqIds.Count() > MAX_DELETE_TRACK)
		{
			m_DelReqIds.RemoveOrdered(0);
			m_DelPartMarkers.RemoveOrdered(0);
			m_DelPartGen.RemoveOrdered(0);
		}

		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_DeleteLiveObjects", new Param3<int, string, ref array<int>>(reqId, typeName, pairs), true, null);
		if (m_DelQueue.Count() > 0)
		{
			m_DelPumpScheduled = true;
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.PumpDeleteQueue, DELETE_SEND_MS, false);
		}
	}

	// ---------------------------------------------------------------- distribution requests and assembly

	protected void ScheduleRequest(int delayMs)
	{
		m_RequestScheduled = true;
		m_RequestDue = NowMs() + delayMs;
	}

	protected void SendDistRequest()
	{
		m_RequestScheduled = false;
		m_DataDirty = false;
		if (m_TypeName == "" || !HasPerm(VPPXEPerm.DIST_MAP) || !m_Owner)
		{
			return;
		}

		m_DistReqId = m_Owner.NextReqId();
		m_DistPending = true;
		m_DistTimedOut = false;
		m_HasRequested = true;
		m_ErrorNotice = "";
		ClearDistData();
		m_DataType = m_TypeName;
		RememberRevisions();
		m_LastActivity = NowMs();
		ShowBusy(Tr("#VSTR_XMLE_STATUS_LOADING"));
		RefreshNotice();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_GetDistribution", new Param3<int, string, int>(m_DistReqId, m_TypeName, DIST_LAYER_MASK), true, null);
	}

	protected void ClearDistData()
	{
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			if (i != VPPXELayer.LIVE)
			{
				m_Layers[i].Clear();
			}
		}

		m_Summary = null;
		m_SummaryGot = false;
		m_ChunksGot = 0;
		m_ChunksExpected = -1;
		m_NextOrderLayer = -1;
		m_NextCursor = -1;
		if (m_SelLayer != VPPXELayer.LIVE)
		{
			ResetSelection();
		}

		m_Renderer.SetData(null, m_Layers);
		UpdateTierChips();
		UpdateLayerChips();
		UpdateDrawerSummary();
		UpdateModeDropdown(false);
		RebuildBreakdown();
		UpdateActionButtons();
		RefreshNotice();
		RequestRedraw();
	}

	protected void AppendDistChunk(VPPXEMapLayerData layerData, VPPXEDistChunk chunk)
	{
		if (chunk.Kind == VPPXEChunkKind.CELLS)
		{
			if (chunk.Ints)
			{
				layerData.Cells.InsertAll(chunk.Ints);
			}

			return;
		}

		if (chunk.Kind == VPPXEChunkKind.MARKERS)
		{
			if (chunk.Ints)
			{
				layerData.MarkRaw.InsertAll(chunk.Ints);
			}

			return;
		}

		if (chunk.Kind == VPPXEChunkKind.CIRCLES)
		{
			if (chunk.Floats)
			{
				layerData.Circles.InsertAll(chunk.Floats);
			}

			return;
		}

		if (chunk.Kind == VPPXEChunkKind.TABLE && chunk.TableRows)
		{
			foreach (VPPXEDistTableRow tableRow : chunk.TableRows)
			{
				if (tableRow)
				{
					layerData.Rows.Insert(tableRow);
				}
			}
		}
	}

	protected void TryFinishDist()
	{
		if (!m_SummaryGot || m_ChunksExpected < 0 || m_ChunksGot < m_ChunksExpected)
		{
			return;
		}

		m_DistPending = false;
		m_DistRetries = 0;
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			if (i == VPPXELayer.LIVE)
			{
				continue;
			}

			m_Layers[i].Decode(m_Summary.PackScale);
			m_Layers[i].BuildLevels(m_Summary.GridCols);
		}

		m_Renderer.SetData(m_Summary, m_Layers);
		FitView();
		HideBusyIfIdle();
		UpdateTierChips();
		UpdateLayerChips();
		UpdateDrawerSummary();
		UpdateModeDropdown(false);
		RebuildBreakdown();
		UpdateActionButtons();
		RefreshNotice();
		RequestRedraw();
	}

	protected void RememberRevisions()
	{
		m_ReqRevs.Clear();
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return;
		}

		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (fileInfo && fileInfo.Kind == VPPXEFileKind.TYPES)
			{
				m_ReqRevs.Set(fileInfo.Key, fileInfo.Revision);
			}
		}
	}

	protected bool RevisionsChanged()
	{
		VPPXESessionInfo session = GetSession();
		if (!session || !session.Files)
		{
			return false;
		}

		int typesFiles = 0;
		foreach (VPPXEFileInfo fileInfo : session.Files)
		{
			if (!fileInfo || fileInfo.Kind != VPPXEFileKind.TYPES)
			{
				continue;
			}

			typesFiles++;
			int known;
			if (!m_ReqRevs.Find(fileInfo.Key, known) || known != fileInfo.Revision)
			{
				return true;
			}
		}

		return typesFiles != m_ReqRevs.Count();
	}

	protected void CheckTimeouts(int now)
	{
		if (!m_DistPending && !m_LivePending)
		{
			return;
		}

		if (now - m_LastActivity <= VPPXEConst.INFLIGHT_TIMEOUT_MS)
		{
			return;
		}

		// a silent distribution request is asked again up to AUTO_RETRY_MAX times (the new request replaces this
		// admin's older pending query on the server); a live scan is never repeated without its confirmation
		if (m_DistPending && !m_LivePending && m_DistRetries < AUTO_RETRY_MAX)
		{
			m_DistRetries++;
			m_DistPending = false;
			ScheduleRequest(0);
			return;
		}

		if (m_DistPending)
		{
			m_DistTimedOut = true;
			m_DistRetries = 0;
		}

		m_DistPending = false;
		m_LivePending = false;
		m_ErrorNotice = Tr("#VSTR_XMLE_ERR_NOT_READY");
		HideBusyIfIdle();
		UpdateActionButtons();
		RefreshNotice();
	}

	// Fits the view to the bounding box of every record; falls back to the world centre at scale 0.8.
	protected void FitView()
	{
		if (!m_Map)
		{
			return;
		}

		float worldSize = 0;
		if (m_Owner && m_Owner.GetModel())
		{
			worldSize = m_Owner.GetModel().GetWorldSize();
		}

		if (worldSize <= 0)
		{
			worldSize = 15360;
		}

		m_BbAny = false;
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			if (i != VPPXELayer.LIVE)
			{
				ExtendBounds(m_Layers[i]);
			}
		}

		if (!m_BbAny)
		{
			m_Map.SetMapPos(Vector(worldSize * 0.5, 0, worldSize * 0.5));
			m_Map.SetScale(0.8);
			return;
		}

		float extent = Math.Max(m_BbMaxX - m_BbMinX, m_BbMaxZ - m_BbMinZ) + 400;
		m_Map.SetMapPos(Vector((m_BbMinX + m_BbMaxX) * 0.5, 0, (m_BbMinZ + m_BbMaxZ) * 0.5));
		float zoom = extent / worldSize;
		if (m_Renderer.Calibrate() && m_Renderer.GetPixelsPerMetre() > 0)
		{
			float visibleM = Math.Min(m_Renderer.GetCanvasWidth(), m_Renderer.GetCanvasHeight()) / m_Renderer.GetPixelsPerMetre();
			if (visibleM > 1)
			{
				zoom = m_Map.GetScale() * extent / visibleM;
			}
		}

		m_Map.SetScale(Math.Clamp(zoom, 0.02, 1.0));
	}

	protected void ExtendBounds(VPPXEMapLayerData layerData)
	{
		if (!layerData || !m_Summary || m_Summary.GridCols <= 0)
		{
			return;
		}

		int cols = m_Summary.GridCols;
		int size = m_Summary.CellSize;
		if (size <= 0)
		{
			size = VPPXEConst.CELL_SIZE;
		}

		int cellInts = layerData.Cells.Count();
		for (int i = 0; i + 3 < cellInts; i += 4)
		{
			//integer column/row first: inside a float expression Enforce evaluates / as float division and has no float %
			int cellIdx = layerData.Cells[i];
			int cellCol = cellIdx % cols;
			int cellRow = cellIdx / cols;
			float cellX = cellCol * size;
			float cellZ = cellRow * size;
			AddBoundsPoint(cellX, cellZ);
			AddBoundsPoint(cellX + size, cellZ + size);
		}

		int markers = layerData.MarkX.Count();
		for (int m = 0; m < markers; m++)
		{
			AddBoundsPoint(layerData.MarkX[m], layerData.MarkZ[m]);
		}

		int circleFloats = layerData.Circles.Count();
		for (int c = 0; c + 2 < circleFloats; c += 3)
		{
			float radius = layerData.Circles[c + 2];
			AddBoundsPoint(layerData.Circles[c] - radius, layerData.Circles[c + 1] - radius);
			AddBoundsPoint(layerData.Circles[c] + radius, layerData.Circles[c + 1] + radius);
		}
	}

	protected void AddBoundsPoint(float wx, float wz)
	{
		if (!m_BbAny)
		{
			m_BbMinX = wx;
			m_BbMaxX = wx;
			m_BbMinZ = wz;
			m_BbMaxZ = wz;
			m_BbAny = true;
			return;
		}

		m_BbMinX = Math.Min(m_BbMinX, wx);
		m_BbMaxX = Math.Max(m_BbMaxX, wx);
		m_BbMinZ = Math.Min(m_BbMinZ, wz);
		m_BbMaxZ = Math.Max(m_BbMaxZ, wz);
	}

	// ---------------------------------------------------------------- live layer

	protected void BeginLiveScan()
	{
		if (!HasPerm(VPPXEPerm.LIVE_SCAN) || m_TypeName == "" || m_LivePending)
		{
			return;
		}

		// Same test as the server's Physics-scan switch: config-only classes (every Land_* type) have no script
		// class, so check config inheritance first and fall back to the typename for script classes.
		bool needsConfirm = false;
		if (GetGame().IsKindOf(m_TypeName, "House") || GetGame().IsKindOf(m_TypeName, "HouseNoDestruct"))
		{
			needsConfirm = true;
		}

		typename scanType = m_TypeName.ToType();
		if (scanType && (scanType.IsInherited(House) || scanType.IsInherited(BuildingSuper) || scanType.IsInherited(CrashBase)))
		{
			needsConfirm = true;
		}

		if (needsConfirm)
		{
			m_Owner.OpenConfirm("#VSTR_TOOLTIP_TITLE_NOTICE", "#VSTR_XMLE_SCAN_WARNING", DIAGTYPE.DIAG_YESNO, this, "OnConfirmScan", false);
			return;
		}

		SendLiveScan();
	}

	protected void SendLiveScan()
	{
		if (m_TypeName == "" || !m_Owner || !HasPerm(VPPXEPerm.LIVE_SCAN))
		{
			return;
		}

		ClearLive();
		m_LiveReqId = m_Owner.NextReqId();
		m_LivePending = true;
		m_LiveType = m_TypeName;
		m_LiveGot = 0;
		m_LiveExpected = -1;
		m_ErrorNotice = "";
		m_LastActivity = NowMs();
		ShowBusy(Tr("#VSTR_XMLE_STATUS_LOADING"));
		UpdateActionButtons();
		GetRPCManager().VSendRPC("RPC_XMLEditor", "XE_LiveScan", new Param2<int, string>(m_LiveReqId, m_TypeName), true, null);
	}

	protected void FinishLive()
	{
		m_LivePending = false;
		m_HasLive = true;
		m_LiveGen++;
		m_VisibleMask = m_VisibleMask | (1 << VPPXELayer.LIVE);
		m_Renderer.SetVisibleLayers(m_VisibleMask);
		int received = m_Layers[VPPXELayer.LIVE].MarkerCount();
		if (m_LiveCapped)
		{
			m_LiveNotice = string.Format(Tr("#VSTR_XMLE_MAP_LIVE_CAPPED"), received);
		}
		else
		{
			m_LiveNotice = string.Format(Tr("#VSTR_XMLE_MAP_LIVE_FMT"), m_LiveTotal);
		}

		HideBusyIfIdle();
		UpdateLayerChips();
		UpdateDrawerSummary();
		UpdateModeDropdown(true);
		RebuildBreakdown();
		UpdateActionButtons();
		RefreshNotice();
		RequestRedraw();
	}

	// Drops the live layer; queued delete parts keep their own type name and are still sent.
	protected void ClearLive()
	{
		m_Layers[VPPXELayer.LIVE].Clear();
		m_HasLive = false;
		m_LivePending = false;
		m_LiveNotice = "";
		m_LiveTotal = 0;
		m_LiveCapped = false;
		m_LiveType = "";
		m_LiveGen++;
		m_PendingDelete.Clear();
		if (m_SelLayer == VPPXELayer.LIVE)
		{
			ResetSelection();
		}

		HideBusyIfIdle();
		UpdateLayerChips();
		UpdateDrawerSummary();
		UpdateModeDropdown(false);
		RebuildBreakdown();
		UpdateActionButtons();
		RefreshNotice();
		RequestRedraw();
	}

	protected void BeginDeleteLive(bool all)
	{
		if (!HasPerm(VPPXEPerm.DELETE_OBJECTS) || !m_HasLive || !m_Owner)
		{
			return;
		}

		VPPXEMapLayerData live = m_Layers[VPPXELayer.LIVE];
		m_PendingDelete.Clear();
		if (all)
		{
			int total = live.MarkerCount();
			for (int i = 0; i < total; i++)
			{
				if (live.IsMarkerAlive(i))
				{
					m_PendingDelete.Insert(i);
				}
			}
		}
		else if (m_SelLayer == VPPXELayer.LIVE && live.IsMarkerAlive(m_SelIndex))
		{
			m_PendingDelete.Insert(m_SelIndex);
		}

		if (m_PendingDelete.Count() == 0)
		{
			return;
		}

		m_PendingDeleteGen = m_LiveGen;
		m_PendingDeleteType = m_LiveType;
		string body = string.Format(Tr("#VSTR_XMLE_DLG_DELLIVE_BODY"), m_PendingDelete.Count(), m_LiveType);
		m_Owner.OpenConfirm("#VSTR_XMLE_DLG_DELLIVE_TITLE", body, DIAGTYPE.DIAG_YESNO, this, "OnConfirmDeleteLive", false);
	}

	// The server answers MAP_DELETED with the count of deleted objects of a part (and ERR_REFUSED for the rest) but
	// not which ones, so the part's markers are removed only when every one of them was deleted and the live layer
	// is still the one they came from; otherwise they stay (and can be queued again) until the next scan.
	protected void ApplyDeleted(int part, int deletedCount)
	{
		if (part < 0 || part >= m_DelPartMarkers.Count())
		{
			return;
		}

		array<int> markers = m_DelPartMarkers[part];
		if (!markers || m_DelPartGen[part] != m_LiveGen)
		{
			return;
		}

		if (deletedCount != markers.Count())
		{
			m_DelPartMarkers.Set(part, null);
			return;
		}

		VPPXEMapLayerData live = m_Layers[VPPXELayer.LIVE];
		foreach (int markerIdx : markers)
		{
			if (markerIdx >= 0 && markerIdx < live.MarkMeta.Count())
			{
				live.MarkMeta.Set(markerIdx, -1);
			}
		}

		m_DelPartMarkers.Set(part, null);
		if (m_SelLayer == VPPXELayer.LIVE && !live.IsMarkerAlive(m_SelIndex))
		{
			ResetSelection();
		}

		UpdateLayerChips();
		if (m_ModeLayer == VPPXELayer.LIVE)
		{
			RebuildBreakdown();
		}

		UpdateActionButtons();
		RequestRedraw();
	}

	// ---------------------------------------------------------------- selection, breakdown and teleport

	protected void OnMapClicked()
	{
		int mx;
		int my;
		GetMousePos(mx, my);
		int hitLayer;
		int hitIndex;
		vector hitPos;
		if (!m_Renderer.HitTest(mx, my, hitLayer, hitIndex, hitPos))
		{
			ResetSelection();
			UpdateActionButtons();
			RequestRedraw();
			return;
		}

		SetSelection(hitLayer, hitIndex, hitPos);
		m_HudItem = DescribeHit(hitLayer, hitIndex, m_Renderer.GetLastHitCount(), m_Renderer.GetLastHitPoints(), hitPos);
		UpdateActionButtons();
		RequestRedraw();
	}

	protected void SetSelection(int layer, int index, vector pos)
	{
		m_SelLayer = layer;
		m_SelIndex = index;
		m_SelPos = pos;
		m_HasSelPos = true;
		m_Renderer.SetSelection(pos, true);
		if (layer == m_ModeLayer && index >= 0)
		{
			m_NextCursor = index;
		}
	}

	protected void ResetSelection()
	{
		m_SelLayer = -1;
		m_SelIndex = -1;
		m_HasSelPos = false;
		m_HudItem = "";
		if (m_Renderer)
		{
			m_Renderer.SetSelection(vector.Zero, false);
		}
	}

	protected string DescribeHit(int layer, int index, int count, int points, vector pos)
	{
		string layerName = Tr(VPPXEText.LayerKey(layer));
		int wx = Math.Round(pos[0]);
		int wz = Math.Round(pos[2]);
		string coords = string.Format(Tr("#VSTR_XMLE_MAP_HUD_FMT"), wx, wz);
		if (layer == VPPXELayer.LIVE && index >= 0)
		{
			int liveNumber = index + 1;
			return layerName + " #" + liveNumber.ToString() + "  " + coords;
		}

		if (layer == VPPXELayer.INFECTED)
		{
			return layerName + "  " + coords;
		}

		VPPXEMapLayerData layerData = LayerAt(layer);
		if (index >= 0 && layerData)
		{
			int rowIdx = RowOfMarker(layer, index);
			if (rowIdx >= 0 && rowIdx < layerData.Rows.Count())
			{
				return DescribeRow(layer, layerData.Rows[rowIdx]);
			}
		}

		return string.Format(Tr("#VSTR_XMLE_MAP_HUD_ITEM"), layerName, count, points);
	}

	// Table rows aggregate the instances of one key: points per instance = Points / Count.
	protected string DescribeRow(int layer, VPPXEDistTableRow row)
	{
		string layerName = Tr(VPPXEText.LayerKey(layer));
		if (!row)
		{
			return layerName;
		}

		int perInstance = row.Points;
		if (row.Count > 1)
		{
			perInstance = row.Points / row.Count;
		}

		return string.Format(Tr("#VSTR_XMLE_MAP_HUD_ITEM"), layerName, row.Label, perInstance);
	}

	protected int RowOfMarker(int layer, int markerIdx)
	{
		VPPXEMapLayerData layerData = LayerAt(layer);
		if (!layerData || markerIdx < 0 || markerIdx >= layerData.MarkMeta.Count())
		{
			return -1;
		}

		int meta = layerData.MarkMeta[markerIdx];
		if (meta < 0)
		{
			return -1;
		}

		return (meta >> 8) - 1;
	}

	protected int FirstMarkerOfRow(int layer, int rowIdx)
	{
		VPPXEMapLayerData layerData = LayerAt(layer);
		if (!layerData)
		{
			return -1;
		}

		int total = layerData.MarkMeta.Count();
		for (int i = 0; i < total; i++)
		{
			if (RowOfMarker(layer, i) == rowIdx)
			{
				return i;
			}
		}

		return -1;
	}

	protected void PollBreakdownSelection()
	{
		if (!m_BreakdownList)
		{
			return;
		}

		int row = m_BreakdownList.GetSelectedRow();
		if (row == m_BreakdownSel)
		{
			return;
		}

		m_BreakdownSel = row;
		if (row < 0 || row >= m_RowMap.Count())
		{
			return;
		}

		OnBreakdownRow(m_RowMap[row]);
	}

	// Centres the map on the row (its first marker when it has one) and selects it.
	protected void OnBreakdownRow(int rowIdx)
	{
		VPPXEMapLayerData layerData = LayerAt(m_ModeLayer);
		if (!layerData || rowIdx < 0 || rowIdx >= layerData.Rows.Count())
		{
			return;
		}

		VPPXEDistTableRow row = layerData.Rows[rowIdx];
		if (!row)
		{
			return;
		}

		vector pos = Vector(row.X, 0, row.Z);
		int marker = FirstMarkerOfRow(m_ModeLayer, rowIdx);
		if (marker >= 0)
		{
			pos = Vector(layerData.MarkX[marker], 0, layerData.MarkZ[marker]);
		}

		if (marker < 0 && row.X == 0 && row.Z == 0)
		{
			m_HudItem = DescribeRow(m_ModeLayer, row);
			return;
		}

		FocusMapOn(pos);
		SetSelection(m_ModeLayer, marker, pos);
		m_NextCursor = marker;
		m_HudItem = DescribeRow(m_ModeLayer, row);
		UpdateActionButtons();
		RequestRedraw();
	}

	// Cycles the markers of the selected breakdown row (every marker of the layer when no row is selected);
	// a layer without markers cycles its 100 m cells by count.
	protected void CycleNext()
	{
		int layer = m_ModeLayer;
		if (layer < 0)
		{
			layer = m_SelLayer;
		}

		VPPXEMapLayerData layerData = LayerAt(layer);
		if (!layerData)
		{
			return;
		}

		if (m_NextOrderLayer != layer && layerData.MarkMeta.Count() == 0)
		{
			m_NextCursor = -1;
		}

		int rowIdx = -1;
		if (layer == m_ModeLayer && m_BreakdownSel >= 0 && m_BreakdownSel < m_RowMap.Count())
		{
			rowIdx = m_RowMap[m_BreakdownSel];
		}

		int total = layerData.MarkMeta.Count();
		if (total > 0)
		{
			for (int step = 1; step <= total; step++)
			{
				int candidate = (m_NextCursor + step) % total;
				if (candidate < 0 || !layerData.IsMarkerAlive(candidate))
				{
					continue;
				}

				if (rowIdx >= 0 && RowOfMarker(layer, candidate) != rowIdx)
				{
					continue;
				}

				FocusMarker(layer, candidate);
				return;
			}

			return;
		}

		if (m_NextOrderLayer != layer)
		{
			BuildCellOrder(layer);
		}

		int cells = m_NextOrder.Count();
		if (cells == 0 || !m_Summary || m_Summary.GridCols <= 0)
		{
			return;
		}

		m_NextCursor = m_NextCursor + 1;
		if (m_NextCursor < 0 || m_NextCursor >= cells)
		{
			m_NextCursor = 0;
		}

		int cellPos = m_NextOrder[m_NextCursor] * 4;
		int cellIdx = layerData.Cells[cellPos];
		int size = m_Summary.CellSize;
		if (size <= 0)
		{
			size = VPPXEConst.CELL_SIZE;
		}

		int nextCol = cellIdx % m_Summary.GridCols;
		int nextRow = cellIdx / m_Summary.GridCols;
		float cx = nextCol * size + size * 0.5;
		float cz = nextRow * size + size * 0.5;
		vector centre = Vector(cx, 0, cz);
		FocusMapOn(centre);

		SetSelection(layer, -1, centre);
		string layerName = Tr(VPPXEText.LayerKey(layer));
		m_HudItem = string.Format(Tr("#VSTR_XMLE_MAP_HUD_ITEM"), layerName, layerData.Cells[cellPos + 1], layerData.Cells[cellPos + 3]);
		UpdateActionButtons();
		RequestRedraw();
	}

	protected void FocusMarker(int layer, int markerIdx)
	{
		VPPXEMapLayerData layerData = LayerAt(layer);
		if (!layerData)
		{
			return;
		}

		vector pos = Vector(layerData.MarkX[markerIdx], 0, layerData.MarkZ[markerIdx]);
		FocusMapOn(pos);

		SetSelection(layer, markerIdx, pos);
		m_NextCursor = markerIdx;
		m_HudItem = DescribeHit(layer, markerIdx, 1, 0, pos);
		UpdateActionButtons();
		RequestRedraw();
	}

	// Centres the map on pos, zooming in first when more than FOCUS_VIEW_M metres are visible (never zooms out).
	protected void FocusMapOn(vector pos)
	{
		if (!m_Map)
		{
			return;
		}

		if (m_Renderer && m_Renderer.Calibrate() && m_Renderer.GetPixelsPerMetre() > 0)
		{
			float visibleM = Math.Min(m_Renderer.GetCanvasWidth(), m_Renderer.GetCanvasHeight()) / m_Renderer.GetPixelsPerMetre();
			if (visibleM > FOCUS_VIEW_M * 1.05)
			{
				float focusScale = m_Map.GetScale() * FOCUS_VIEW_M / visibleM;
				m_Map.SetScale(Math.Max(focusScale, 0.02));
			}
		}

		m_Map.SetMapPos(pos);
	}

	// Cells of the layer ordered by count (descending) with one native Sort over fixed-width keys.
	protected void BuildCellOrder(int layer)
	{
		m_NextOrder.Clear();
		m_NextOrderLayer = layer;
		m_NextCursor = -1;
		VPPXEMapLayerData layerData = LayerAt(layer);
		if (!layerData)
		{
			return;
		}

		array<string> keys = new array<string>;
		int cells = layerData.Cells.Count() / 4;
		for (int i = 0; i < cells; i++)
		{
			int count = layerData.Cells[i * 4 + 1];
			if (count > 999999999)
			{
				count = 999999999;
			}

			string sortKey = VPPXmlText.PadInt(999999999 - count, 9) + VPPXmlText.PadInt(i, 7);
			keys.Insert(sortKey);
		}

		keys.Sort();
		foreach (string picked : keys)
		{
			m_NextOrder.Insert(picked.Substring(9, 7).ToInt());
		}
	}

	protected void TeleportToSelection()
	{
		if (!HasPerm(VPPXEPerm.TELEPORT) || !m_HasSelPos)
		{
			return;
		}

		DoTeleport(m_SelPos);
	}

	protected void DoTeleport(vector pos)
	{
		array<string> ids = new array<string>;
		ids.Insert("self");
		vector dest = Vector(pos[0], 0, pos[2]);
		GetRPCManager().VSendRPC("RPC_TeleportManager", "RemoteTeleportPlayers", new Param3<ref array<string>, string, vector>(ids, "", dest), true, null);
	}

	protected void RebuildBreakdown()
	{
		if (!m_BreakdownList)
		{
			return;
		}

		m_BreakdownList.ClearItems();
		m_RowMap.Clear();
		m_BreakdownSel = -1;
		VPPXEMapLayerData layerData = LayerAt(m_ModeLayer);
		if (!layerData)
		{
			return;
		}

		int total = layerData.Rows.Count();
		int shown = 0;
		for (int i = 0; i < total && shown < VPPXEConst.RAIL_MAX_ROWS; i++)
		{
			VPPXEDistTableRow row = layerData.Rows[i];
			if (!row)
			{
				continue;
			}

			if (m_ModeLayer == VPPXELayer.LIVE && !layerData.IsMarkerAlive(i))
			{
				continue;
			}

			string label = row.Label;
			if (m_ModeLayer == VPPXELayer.CONTAINERS && row.Detail != "")
			{
				label = label + " - " + row.Detail;
			}

			int count = row.Count;
			int listRow = m_BreakdownList.AddItem(label, null, 0);
			m_BreakdownList.SetItem(listRow, count.ToString(), null, 1);
			m_BreakdownList.SetItem(listRow, TierText(row.TierMask), null, 2);
			m_RowMap.Insert(i);
			shown++;
		}
	}

	protected string TierText(int tierMask)
	{
		string text = "";
		for (int i = 0; i < m_TierCount && i < m_TierLabels.Count(); i++)
		{
			int tierBit = 1 << i;
			if ((tierMask & tierBit) == 0)
			{
				continue;
			}

			if (text != "")
			{
				text = text + " ";
			}

			text = text + m_TierLabels[i];
		}

		return text;
	}

	// Drawer dropdown: every layer that has table rows (CONTAINERS and LIVE included).
	protected void UpdateModeDropdown(bool preferLive)
	{
		if (!m_ModeDD)
		{
			return;
		}

		m_ModeLayers.Clear();
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			if (m_Layers[i].Rows.Count() > 0)
			{
				m_ModeLayers.Insert(i);
			}
		}

		m_ModeDD.RemoveAllElements();
		foreach (int modeLayer : m_ModeLayers)
		{
			m_ModeDD.AddElement(Tr(VPPXEText.LayerKey(modeLayer)));
		}

		int pick = m_ModeLayers.Find(m_ModeLayer);
		if (preferLive && m_ModeLayers.Find(VPPXELayer.LIVE) >= 0)
		{
			pick = m_ModeLayers.Find(VPPXELayer.LIVE);
		}

		if (pick < 0 && m_ModeLayers.Count() > 0)
		{
			pick = 0;
		}

		if (pick >= 0)
		{
			m_ModeLayer = m_ModeLayers[pick];
			m_ModeDD.SetText(Tr(VPPXEText.LayerKey(m_ModeLayer)));
			m_ModeDD.SetIndex(pick);
		}
		else
		{
			m_ModeLayer = -1;
			m_ModeDD.SetText("");
			m_ModeDD.SetIndex(-1);
		}

		m_ModeDD.Close();
		m_NextCursor = -1;
	}

	// ---------------------------------------------------------------- toolbar chips

	protected void ToggleLayer(int layer)
	{
		int bit = 1 << layer;
		if ((m_VisibleMask & bit) != 0)
		{
			m_VisibleMask = m_VisibleMask - bit;
		}
		else
		{
			m_VisibleMask = m_VisibleMask | bit;
		}

		m_Renderer.SetVisibleLayers(m_VisibleMask);
		UpdateLayerChips();
		RequestRedraw();
	}

	protected void ToggleTier(int tier)
	{
		int bit = 1 << tier;
		if ((m_TierMask & bit) != 0)
		{
			m_TierMask = m_TierMask - bit;
		}
		else
		{
			m_TierMask = m_TierMask | bit;
		}

		UpdateTierChips();
		RequestRedraw();
	}

	protected void ToggleRenderMode()
	{
		if (m_RenderMode == VPPXEMapRenderer.MODE_HEAT)
		{
			m_RenderMode = VPPXEMapRenderer.MODE_PINS;
		}
		else
		{
			m_RenderMode = VPPXEMapRenderer.MODE_HEAT;
		}

		if (m_ImgRenderMode)
		{
			m_ImgRenderMode.SetImage(m_RenderMode);
		}

		m_Renderer.SetMode(m_RenderMode);
		RequestRedraw();
	}

	protected void SetDrawerOpen(bool open)
	{
		m_DrawerOpen = open;
		m_MapDrawer.Show(open);
		OnResize();
	}

	// Layer chip label: the name, plus the count when both fit in the chip (7 px per character basis).
	protected void UpdateLayerChips()
	{
		for (int i = 0; i < LAYER_CHIP_COUNT && i < m_BtnLayer.Count(); i++)
		{
			int count = LayerCount(i);
			string name = Tr(VPPXEText.LayerKey(i));
			string label = name;
			if (count > 0)
			{
				string both = name + " " + count.ToString();
				if (EstimateTextPx(both, 7) <= m_ChipW - 20)
				{
					label = both;
				}
			}

			bool enabled = count > 0;
			int layerBit = 1 << i;
			bool isOn = false;
			if (enabled && (m_VisibleMask & layerBit) != 0)
			{
				isOn = true;
			}

			Widget layerButton = Widget.Cast(m_BtnLayer[i]);
			Widget layerFill = Widget.Cast(m_FillLayer[i]);
			TextWidget layerLabel = TextWidget.Cast(m_TxtLayer[i]);
			SetChipLook(layerButton, layerFill, layerLabel, isOn, enabled);
			if (m_TxtLayer[i])
			{
				m_TxtLayer[i].SetText(label);
			}
		}
	}

	// One chip per base value flag of cfglimitsdefinition (session Limits.Values, in bit order, at most 8); the
	// vanilla Tier1-4 + Unique labels when the limits are not known yet. A new set of names turns every tier on.
	protected void RebuildTierChips()
	{
		array<string> names = new array<string>;
		VPPXESessionInfo session = null;
		if (m_Owner && m_Owner.GetModel())
		{
			session = m_Owner.GetModel().GetSession();
		}

		if (session && session.Limits && session.Limits.Values)
		{
			foreach (string valueName : session.Limits.Values)
			{
				if (names.Count() < TIER_CHIP_MAX && valueName != "")
				{
					names.Insert(valueName);
				}
			}
		}

		if (names.Count() == 0)
		{
			for (int k = 0; k < TIER_DEFAULT_COUNT; k++)
			{
				names.Insert(Tr(m_TierKeys[k]));
			}
		}

		string sig = "";
		foreach (string sigName : names)
		{
			sig = sig + sigName + "|";
		}

		if (sig == m_TierSig)
		{
			return;
		}

		m_TierSig = sig;
		m_TierLabels = names;
		m_TierCount = names.Count();
		int allBits = (1 << m_TierCount) - 1;
		m_TierMask = allBits;
		for (int i = 0; i < m_BtnTier.Count(); i++)
		{
			Widget chip = m_BtnTier[i];
			if (!chip)
			{
				continue;
			}

			chip.Show(i < m_TierCount);
			TextWidget chipLabel = m_TxtTier[i];
			if (chipLabel && i < m_TierCount)
			{
				chipLabel.SetText(m_TierLabels[i]);
			}
		}

		LayoutTierChips();
	}

	// Chips sized to their labels inside the width left between the type name and the right-aligned buttons.
	protected void LayoutTierChips()
	{
		if (m_TierCount <= 0 || !m_TierBar)
		{
			return;
		}

		float avail = m_TierBarW;
		if (avail < 60)
		{
			avail = 60;
		}

		array<float> widths = new array<float>;
		float total = 0;
		for (int i = 0; i < m_TierCount; i++)
		{
			string chipText = m_TierLabels[i];
			float chipW = Math.Clamp(16 + 7 * chipText.LengthUtf8(), 34, 96);
			widths.Insert(chipW);
			total = total + chipW + 2;
		}

		float scale = 1;
		if (total > avail)
		{
			scale = avail / total;
		}

		float x = 0;
		for (int j = 0; j < m_TierCount; j++)
		{
			Widget chip = m_BtnTier[j];
			if (!chip)
			{
				continue;
			}

			float w = widths[j] * scale;
			chip.SetPos(x, 0);
			chip.SetSize(w, 24);
			x = x + w + 2;
		}

		m_TierBar.SetSize(x, 24);
	}

	// Tier chips are disabled (and the filter passes everything) when the server has no map tiers.
	protected void UpdateTierChips()
	{
		bool tiersOff = false;
		if (m_Summary && (m_Summary.Flags & VPPXEDistFlag.AREAFLAGS_OFF) != 0)
		{
			tiersOff = true;
		}

		for (int i = 0; i < m_TierCount && i < m_BtnTier.Count(); i++)
		{
			bool tierEnabled = !tiersOff;
			int tierBit = 1 << i;
			bool isOn = false;
			if (tierEnabled && (m_TierMask & tierBit) != 0)
			{
				isOn = true;
			}

			Widget tierButton = Widget.Cast(m_BtnTier[i]);
			Widget tierFill = Widget.Cast(m_FillTier[i]);
			TextWidget tierLabel = TextWidget.Cast(m_TxtTier[i]);
			SetChipLook(tierButton, tierFill, tierLabel, isOn, tierEnabled);
		}

		int effective = m_TierMask;
		if (tiersOff)
		{
			effective = ALL_TIERS;
		}

		if (m_Renderer)
		{
			m_Renderer.SetTierFilter(effective);
		}
	}

	protected void SetChipLook(Widget button, Widget fill, TextWidget label, bool isOn, bool enabled)
	{
		int fillColor = ARGB(255, 27, 30, 34);
		int textColor = ARGB(255, 154, 160, 166);
		if (!enabled)
		{
			textColor = ARGB(255, 92, 97, 102);
		}
		else if (isOn)
		{
			fillColor = ARGB(255, 232, 163, 61);
			textColor = ARGB(255, 255, 255, 255);
		}

		if (button)
		{
			button.Enable(enabled);
		}

		if (fill)
		{
			fill.SetColor(fillColor);
		}

		if (label)
		{
			label.SetColor(textColor);
		}
	}

	protected int LayerCount(int layer)
	{
		if (layer == VPPXELayer.LIVE)
		{
			return m_Layers[VPPXELayer.LIVE].AliveMarkerCount();
		}

		if (!m_Summary || !m_Summary.LayerCounts || layer < 0 || layer >= m_Summary.LayerCounts.Count())
		{
			return 0;
		}

		return m_Summary.LayerCounts[layer];
	}

	protected int LayerExtra(int layer)
	{
		if (!m_Summary || !m_Summary.LayerExtra || layer < 0 || layer >= m_Summary.LayerExtra.Count())
		{
			return 0;
		}

		return m_Summary.LayerExtra[layer];
	}

	protected void UpdateActionButtons()
	{
		bool hasType = m_TypeName != "";
		VPPXEMapLayerData live = m_Layers[VPPXELayer.LIVE];
		bool canDelete = HasPerm(VPPXEPerm.DELETE_OBJECTS) && m_HasLive;
		bool hasBrowse = m_ModeLayer >= 0 || m_SelLayer >= 0;
		EnableWidget(m_BtnTpSelected, HasPerm(VPPXEPerm.TELEPORT) && m_HasSelPos);
		EnableWidget(m_BtnNextInstance, hasBrowse);
		EnableWidget(m_BtnLiveScan, HasPerm(VPPXEPerm.LIVE_SCAN) && hasType && !m_LivePending);
		EnableWidget(m_BtnDeleteLiveSel, canDelete && m_SelLayer == VPPXELayer.LIVE && live.IsMarkerAlive(m_SelIndex));
		EnableWidget(m_BtnDeleteLiveAll, canDelete && live.AliveMarkerCount() > 0);
		EnableWidget(m_BtnMapRefresh, HasPerm(VPPXEPerm.DIST_MAP) && hasType);
		UpdateNextLabel();
	}

	// "Next 3/12": the focused instance's position among the ones Next cycles through (same layer and row filter
	// as CycleNext); "Next (12)" before one is focused. Recounted only when layer, row, cursor or size change.
	protected void UpdateNextLabel()
	{
		if (!m_TxtNextInstance)
		{
			return;
		}

		int layer = m_ModeLayer;
		if (layer < 0)
		{
			layer = m_SelLayer;
		}

		int rowIdx = -1;
		if (layer >= 0 && layer == m_ModeLayer && m_BreakdownSel >= 0 && m_BreakdownSel < m_RowMap.Count())
		{
			rowIdx = m_RowMap[m_BreakdownSel];
		}

		VPPXEMapLayerData layerData = LayerAt(layer);
		int markerTotal = 0;
		if (layerData)
		{
			markerTotal = layerData.MarkMeta.Count();
		}

		string labelKey = layer.ToString() + ":" + rowIdx.ToString() + ":" + m_NextCursor.ToString() + ":" + markerTotal.ToString() + ":" + m_NextOrderLayer.ToString();
		if (labelKey == m_NextLabelKey)
		{
			return;
		}

		m_NextLabelKey = labelKey;
		int position = 0;
		int instances = 0;
		if (layerData && markerTotal > 0)
		{
			for (int i = 0; i < markerTotal; i++)
			{
				int meta = layerData.MarkMeta[i];
				if (meta < 0)
				{
					continue;
				}

				int metaRow = (meta >> 8) - 1;
				if (rowIdx >= 0 && metaRow != rowIdx)
				{
					continue;
				}

				instances++;
				if (i == m_NextCursor)
				{
					position = instances;
				}
			}
		}
		else if (layerData && m_NextOrderLayer == layer)
		{
			instances = m_NextOrder.Count();
			if (m_NextCursor >= 0 && m_NextCursor < instances)
			{
				position = m_NextCursor + 1;
			}
		}
		else if (layerData)
		{
			instances = layerData.Cells.Count() / 4;
		}

		string label = Tr("#VSTR_XMLE_MAP_NEXT");
		if (instances > 0)
		{
			if (position > 0)
			{
				label = label + " " + position.ToString() + "/" + instances.ToString();
			}
			else
			{
				label = label + " (" + instances.ToString() + ")";
			}
		}

		m_TxtNextInstance.SetText(label);
	}

	protected void EnableWidget(Widget w, bool enabled)
	{
		if (w)
		{
			w.Enable(enabled);
		}
	}

	// ---------------------------------------------------------------- texts: type, HUD, busy, notice, summary

	protected void SetTypeText(string typeName)
	{
		if (!m_TxtMapType)
		{
			return;
		}

		string shown = typeName;
		float limit = m_TypeTextW;
		if (limit < 40)
		{
			limit = 40;
		}

		if (EstimateTextPx(shown, 8) > limit)
		{
			int keep = Math.Floor((limit - 16) / 8);
			if (keep < 4)
			{
				keep = 4;
			}

			if (keep < shown.Length())
			{
				shown = shown.Substring(0, keep) + "...";
			}
		}

		m_TxtMapType.SetText(shown);
	}

	protected void UpdateHud()
	{
		if (!m_TxtMapHud)
		{
			return;
		}

		string text = m_HudItem;
		if (text == "")
		{
			Widget under = GetWidgetUnderCursor();
			if (under && under == m_Map)
			{
				int mx;
				int my;
				GetMousePos(mx, my);
				vector cursorPos = m_Map.ScreenToMap(Vector(mx, my, 0));
				int wx = Math.Round(cursorPos[0]);
				int wz = Math.Round(cursorPos[2]);
				text = string.Format(Tr("#VSTR_XMLE_MAP_HUD_FMT"), wx, wz);
			}
			else
			{
				text = Tr("#VSTR_XMLE_MAP_HUD_HINT");
			}
		}

		if (text != m_LastHudText)
		{
			m_LastHudText = text;
			m_TxtMapHud.SetText(text);
		}
	}

	protected void ShowBusy(string text)
	{
		if (m_TxtMapBusy)
		{
			m_TxtMapBusy.SetText(text);
		}

		if (m_MapBusy)
		{
			m_MapBusy.Show(true);
		}
	}

	protected void HideBusyIfIdle()
	{
		if (m_DistPending || m_LivePending)
		{
			return;
		}

		if (m_MapBusy)
		{
			m_MapBusy.Show(false);
		}
	}

	// Notices from the summary flags, an error from the last request and the live scan line.
	protected void RefreshNotice()
	{
		if (!m_MapNotice || !m_TxtMapNotice)
		{
			return;
		}

		m_NoticeParts.Clear();
		if (m_ErrorNotice != "")
		{
			m_NoticeParts.Insert(m_ErrorNotice);
		}

		if (m_TypeName == "")
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTYPE"));
		}
		else if (m_Summary && !m_DistPending && m_DataType == m_TypeName)
		{
			AddFlagNotices();
		}

		if (m_LiveNotice != "")
		{
			m_NoticeParts.Insert(m_LiveNotice);
		}

		if (m_NoticeParts.Count() == 0)
		{
			m_MapNotice.Show(false);
			return;
		}

		string text = m_NoticeParts[0];
		for (int i = 1; i < m_NoticeParts.Count(); i++)
		{
			text = text + "\n" + m_NoticeParts[i];
		}

		m_TxtMapNotice.SetText(text);
		LayoutNotice();
		m_MapNotice.Show(true);
	}

	protected void AddFlagNotices()
	{
		int flags = m_Summary.Flags;
		bool nothing = true;
		for (int i = 0; i < LAYER_TOTAL; i++)
		{
			if (i != VPPXELayer.LIVE && LayerCount(i) > 0)
			{
				nothing = false;
			}
		}

		if (nothing)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTHING"));
		}

		if ((flags & VPPXEDistFlag.AREAFLAGS_OFF) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_NOTIERS"));
		}

		if ((flags & VPPXEDistFlag.DELOOT) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_DELOOT"));
		}

		if ((flags & VPPXEDistFlag.NOMINAL_ZERO) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_NOMINAL0"));
		}

		if ((flags & VPPXEDistFlag.IGNORED) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_IGNORED"));
		}

		if ((flags & VPPXEDistFlag.NO_CE_ENTRY) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_NOENTRY"));
		}

		if ((flags & VPPXEDistFlag.PLAYER_EVENTS) != 0)
		{
			m_NoticeParts.Insert(string.Format(Tr("#VSTR_XMLE_MAP_NOTE_PLAYER"), JoinPlayerEvents()));
		}

		if ((flags & VPPXEDistFlag.CAPPED) != 0)
		{
			m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_CAPPED"));
		}

		m_NoticeParts.Insert(Tr("#VSTR_XMLE_MAP_NOTE_RULES"));
	}

	protected string JoinPlayerEvents()
	{
		string joined = "";
		if (!m_Summary || !m_Summary.PlayerEvents)
		{
			return joined;
		}

		int total = m_Summary.PlayerEvents.Count();
		for (int i = 0; i < total && i < MAX_PLAYER_EVENTS; i++)
		{
			if (i > 0)
			{
				joined = joined + ", ";
			}

			joined = joined + m_Summary.PlayerEvents[i];
		}

		if (total > MAX_PLAYER_EVENTS)
		{
			joined = joined + ", ...";
		}

		return joined;
	}

	// Drawer summary lines (the SUM_ keys) for the layers with records, plus the live scan line.
	protected void UpdateDrawerSummary()
	{
		m_SummaryParts.Clear();
		if (m_Summary && m_DataType == m_TypeName)
		{
			int buildings = LayerCount(VPPXELayer.BUILDINGS);
			if (buildings > 0)
			{
				m_SummaryParts.Insert(string.Format(Tr("#VSTR_XMLE_SUM_BUILDINGS"), buildings, LayerExtra(VPPXELayer.BUILDINGS), m_Layers[VPPXELayer.BUILDINGS].PointsTotal));
			}

			AddSummaryLine(VPPXELayer.EVENT_OBJECTS, "#VSTR_XMLE_SUM_EVENTOBJ");
			AddSummaryLine(VPPXELayer.EVENT_CHILD, "#VSTR_XMLE_SUM_EVENTCHILD");
			AddSummaryLine(VPPXELayer.INFECTED, "#VSTR_XMLE_SUM_INFECTED");
			AddSummaryLine(VPPXELayer.CLUSTERS, "#VSTR_XMLE_SUM_CLUSTERS");
			AddSummaryLine(VPPXELayer.DISPATCH, "#VSTR_XMLE_SUM_DISPATCH");
			AddSummaryLine(VPPXELayer.CONTAINERS, "#VSTR_XMLE_SUM_CONTAINERS");
		}

		if (m_LiveNotice != "")
		{
			m_SummaryParts.Insert(m_LiveNotice);
		}

		string text = "";
		for (int i = 0; i < m_SummaryParts.Count(); i++)
		{
			if (i > 0)
			{
				text = text + "\n";
			}

			text = text + m_SummaryParts[i];
		}

		if (m_TxtDrawerSummary)
		{
			m_TxtDrawerSummary.SetText(text);
		}

		if (m_DrawerOpen)
		{
			LayoutDrawer();
		}
	}

	protected void AddSummaryLine(int layer, string key)
	{
		int count = LayerCount(layer);
		if (count > 0)
		{
			m_SummaryParts.Insert(string.Format(Tr(key), count));
		}
	}

	// ---------------------------------------------------------------- layout

	// Layout units per screen pixel ratio, measured on the 30 px toolbar row (exact size in the layout).
	protected void UpdateUnit()
	{
		m_Unit = 1.0;
		if (!m_MapToolbarTop)
		{
			return;
		}

		float topW;
		float topH;
		m_MapToolbarTop.GetScreenSize(topW, topH);
		if (topH > 1)
		{
			m_Unit = topH / TOOLBAR_TOP_H;
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

	// Row 1: TxtMapType 0.34 of the width, then the tier chips; the 3 ghost buttons and the info icon are
	// right-aligned in the layout. Row 2: 7 chips of 1/7 of the width minus 4 px.
	protected void LayoutToolbar(float w)
	{
		float typeW = w * 0.34;
		m_TypeTextW = typeW - 8;
		if (m_TxtMapType)
		{
			m_TxtMapType.SetSize(m_TypeTextW, TOOLBAR_TOP_H);
		}

		if (m_TierBar)
		{
			m_TierBar.SetPos(typeW + 4, 3);
			m_TierBarW = w - typeW - 4 - TIER_BAR_RESERVED;
			LayoutTierChips();
		}

		SetTypeText(m_TypeName);
		float slot = w / LAYER_CHIP_COUNT;
		m_ChipW = slot - 4;
		for (int i = 0; i < m_BtnLayer.Count(); i++)
		{
			Widget chip = m_BtnLayer[i];
			if (chip)
			{
				chip.SetPos(i * slot + 2, 2);
				chip.SetSize(m_ChipW, 24);
			}

			// label sits right of the 8 px legend swatch
			if (i < m_TxtLayer.Count() && m_TxtLayer[i])
			{
				m_TxtLayer[i].SetSize(Math.Max(m_ChipW - 20, 10), 1);
			}
		}

		UpdateLayerChips();
	}

	// Drawer: header 28, mode dropdown host at 32 (24 high; its popup stays inside the drawer), summary,
	// breakdown list, then the 3-row action grid at the bottom.
	protected void LayoutDrawer()
	{
		if (!m_DrawerOpen || m_DrawerW < 40)
		{
			return;
		}

		float innerW = m_DrawerW - 12;
		m_DrawerModeHost.SetPos(6, 32);
		m_DrawerModeHost.SetSize(innerW, 24);
		int summaryLines = EstimateLines(m_SummaryParts, m_DrawerW - 16);
		float summaryH = summaryLines * 15 + 4;
		if (summaryLines == 0)
		{
			summaryH = 1;
		}

		m_TxtDrawerSummary.SetPos(8, 60);
		m_TxtDrawerSummary.SetSize(m_DrawerW - 16, summaryH);
		float listY = 60 + summaryH + 4;
		float actionsY = m_DrawerH - DRAWER_ACTIONS_H - 6;
		float listH = actionsY - listY - 4;
		if (listH < 30)
		{
			listH = 30;
		}

		m_BreakdownList.SetPos(6, listY);
		m_BreakdownList.SetSize(innerW, listH);
		m_DrawerActions.SetPos(6, actionsY);
		m_DrawerActions.SetSize(innerW, DRAWER_ACTIONS_H);
		float half = innerW * 0.5 - 2;
		PlaceActionButton(m_BtnTpSelected, m_ImgTpSelected, m_TxtTpSelected, "#VSTR_XMLE_MAP_TP", 0, 0, half);
		PlaceActionButton(m_BtnNextInstance, m_ImgNextInstance, m_TxtNextInstance, "#VSTR_XMLE_MAP_NEXT", innerW - half, 0, half);
		PlaceActionButton(m_BtnLiveScan, m_ImgLiveScan, m_TxtLiveScan, "#VSTR_XMLE_MAP_LIVESCAN", 0, 30, innerW);
		PlaceActionButton(m_BtnDeleteLiveSel, m_ImgDeleteLiveSel, m_TxtDeleteLiveSel, "#VSTR_XMLE_MAP_DEL_SEL", 0, 60, half);
		PlaceActionButton(m_BtnDeleteLiveAll, m_ImgDeleteLiveAll, m_TxtDeleteLiveAll, "#VSTR_XMLE_MAP_DEL_ALL", innerW - half, 60, half);
	}

	// Hides the icon when 24 + 7 px per character of the translated label exceeds the button width.
	protected void PlaceActionButton(Widget button, Widget iconWidget, TextWidget label, string key, float x, float y, float width)
	{
		if (!button)
		{
			return;
		}

		button.SetPos(x, y);
		button.SetSize(width, ACTION_ROW_H);
		string text = Tr(key);
		bool showIcon = 24 + EstimateTextPx(text, 7) <= width;
		if (iconWidget)
		{
			iconWidget.Show(showIcon);
		}

		if (!label)
		{
			return;
		}

		if (showIcon)
		{
			label.SetPos(24, 0);
			label.SetSize(width - 28, ACTION_ROW_H);
		}
		else
		{
			label.SetPos(4, 0);
			label.SetSize(width - 8, ACTION_ROW_H);
		}
	}

	protected void LayoutNotice()
	{
		if (!m_MapNotice || !m_TxtMapNotice)
		{
			return;
		}

		float noticeW = m_AreaW * 0.6;
		if (noticeW > 460)
		{
			noticeW = 460;
		}

		if (noticeW < 160)
		{
			noticeW = 160;
		}

		int lines = EstimateLines(m_NoticeParts, noticeW - 14);
		if (lines < 1)
		{
			lines = 1;
		}

		float noticeH = lines * 15 + 10;
		m_MapNotice.SetSize(noticeW, noticeH);
		m_TxtMapNotice.SetPos(7, 5);
		m_TxtMapNotice.SetSize(noticeW - 14, noticeH - 8);
	}

	// ---------------------------------------------------------------- view polling and redraw

	protected void PollView(int now)
	{
		float scale = m_Map.GetScale();
		vector mapPos = m_Map.GetMapPos();
		float cx;
		float cy;
		float cw;
		float ch;
		m_Map.GetScreenPos(cx, cy);
		m_Map.GetScreenSize(cw, ch);
		bool changed = false;
		if (scale != m_LastScale || mapPos != m_LastMapPos)
		{
			changed = true;
		}

		bool rectChanged = false;
		if (cx != m_LastCX || cy != m_LastCY || cw != m_LastCW || ch != m_LastCH)
		{
			changed = true;
			rectChanged = true;
		}

		if (!changed)
		{
			return;
		}

		m_LastScale = scale;
		m_LastMapPos = mapPos;
		m_LastCX = cx;
		m_LastCY = cy;
		m_LastCW = cw;
		m_LastCH = ch;
		if (!m_Renderer.IsViewDependent())
		{
			return;
		}

		m_SettlePending = true;
		m_LastChange = now;
		if (rectChanged || !m_PanAnchorValid || scale != m_PanAnchorScale)
		{
			m_ViewDirty = true;
			return;
		}

		// Pure pan: slide the last drawing by the screen movement of a fixed world point.
		vector anchorNow = m_Map.MapToScreen(Vector(0, 0, 0));
		float slideX = anchorNow[0] - m_PanAnchorScreen[0];
		float slideY = anchorNow[1] - m_PanAnchorScreen[1];
		m_Canvas.SetPos(slideX, slideY);
		if (Math.AbsFloat(slideX) > cw * PAN_REDRAW_FRAC || Math.AbsFloat(slideY) > ch * PAN_REDRAW_FRAC)
		{
			m_ViewDirty = true;
		}
	}

	protected void DoRedraw(int now)
	{
		m_Canvas.SetPos(0, 0);
		m_Renderer.Redraw();
		m_PanAnchorScreen = m_Map.MapToScreen(Vector(0, 0, 0));
		m_PanAnchorScale = m_Map.GetScale();
		m_PanAnchorValid = true;
		m_LastRedraw = now;
		m_ViewDirty = false;
		m_RedrawNow = false;
	}

	protected void RequestRedraw()
	{
		m_RedrawNow = true;
	}

	// ---------------------------------------------------------------- helpers

	protected VPPXEMapLayerData LayerAt(int layer)
	{
		if (layer < 0 || layer >= m_Layers.Count())
		{
			return null;
		}

		return m_Layers[layer];
	}

	protected VPPXESessionInfo GetSession()
	{
		if (!m_Owner || !m_Owner.GetModel())
		{
			return null;
		}

		return m_Owner.GetModel().GetSession();
	}

	protected bool HasPerm(int permBit)
	{
		if (!m_Owner)
		{
			return false;
		}

		return m_Owner.HasPerm(permBit);
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

	// Width estimate in layout px: pxPerChar per character plus 2 px per extra UTF-8 byte (Cyrillic and
	// CJK glyphs are wider than Latin ones).
	protected int EstimateTextPx(string text, int pxPerChar)
	{
		int bytes = text.Length();
		int chars = text.LengthUtf8();
		return pxPerChar * chars + 2 * (bytes - chars);
	}

	protected int EstimateLines(array<string> parts, float width)
	{
		float usable = width;
		if (usable < 20)
		{
			usable = 20;
		}

		int total = 0;
		foreach (string part : parts)
		{
			int px = EstimateTextPx(part, 7);
			int lines = Math.Ceil(px / usable);
			if (lines < 1)
			{
				lines = 1;
			}

			total += lines;
		}

		return total;
	}
};
