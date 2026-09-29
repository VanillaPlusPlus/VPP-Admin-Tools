// VPP XML Editor (WP6): distribution map layer storage and the canvas renderer of the MAP tab.
// The renderer draws heat cells (100 m / 500 m / 2 km levels, rebinned to 12 px screen bins when small),
// markers, infected circles and the selection on a CanvasWidget placed after the MapWidget, with a pins
// fallback through AddUserMark. The world-to-canvas transform is calibrated from two MapToScreen samples
// on every redraw, detecting whether MapToScreen answers in absolute or widget-local pixels.

class VPPXEMapLayerData : Managed
{
	ref array<int> Cells;
	ref array<int> Cells500;
	ref array<int> Cells2000;
	ref array<int> MarkRaw;
	ref array<float> MarkX;
	ref array<float> MarkZ;
	ref array<int> MarkMeta;
	ref array<int> MarkNet;
	ref array<float> Circles;
	ref array<ref VPPXEDistTableRow> Rows;
	int PointsTotal;

	// Cells* = [cellIdx, count, tierMask, points]*; MarkRaw = MARKERS ints as received [px, pz, meta]*;
	// MarkMeta = tierMask | ((tableRow + 1) << 8), -1 = removed (live delete); MarkNet = LIVE [low, high]*;
	// Circles = [x, z, r]*.
	void VPPXEMapLayerData()
	{
		Cells = new array<int>;
		Cells500 = new array<int>;
		Cells2000 = new array<int>;
		MarkRaw = new array<int>;
		MarkX = new array<float>;
		MarkZ = new array<float>;
		MarkMeta = new array<int>;
		MarkNet = new array<int>;
		Circles = new array<float>;
		Rows = new array<ref VPPXEDistTableRow>;
		PointsTotal = 0;
	}

	void Clear()
	{
		Cells.Clear();
		Cells500.Clear();
		Cells2000.Clear();
		MarkRaw.Clear();
		MarkX.Clear();
		MarkZ.Clear();
		MarkMeta.Clear();
		MarkNet.Clear();
		Circles.Clear();
		Rows.Clear();
		PointsTotal = 0;
	}

	int MarkerCount()
	{
		return MarkMeta.Count();
	}

	int AliveMarkerCount()
	{
		int alive = 0;
		foreach (int meta : MarkMeta)
		{
			if (meta >= 0)
			{
				alive++;
			}
		}

		return alive;
	}

	bool IsMarkerAlive(int index)
	{
		if (index < 0 || index >= MarkMeta.Count())
		{
			return false;
		}

		return MarkMeta[index] >= 0;
	}

	// Decodes the received marker ints (x = px * PackScale) and totals the cell points.
	void Decode(float packScale)
	{
		int rawCount = MarkRaw.Count();
		for (int i = 0; i + 2 < rawCount; i += 3)
		{
			float wx = MarkRaw[i] * packScale;
			float wz = MarkRaw[i + 1] * packScale;
			MarkX.Insert(wx);
			MarkZ.Insert(wz);
			MarkMeta.Insert(MarkRaw[i + 2]);
		}

		MarkRaw.Clear();
		PointsTotal = 0;
		int cellInts = Cells.Count();
		for (int j = 0; j + 3 < cellInts; j += 4)
		{
			PointsTotal += Cells[j + 3];
		}
	}

	void BuildLevels(int gridCols)
	{
		Aggregate(Cells, gridCols, 5, Cells500);
		Aggregate(Cells, gridCols, 20, Cells2000);
	}

	// Sums counts and points and ORs tiers of the 100 m cells into cells of factor x 100 m.
	protected void Aggregate(array<int> source, int gridCols, int factor, array<int> target)
	{
		target.Clear();
		if (gridCols <= 0 || factor <= 0)
		{
			return;
		}

		int targetCols = (gridCols + factor - 1) / factor;
		map<int, int> slots = new map<int, int>;
		int sourceInts = source.Count();
		for (int i = 0; i + 3 < sourceInts; i += 4)
		{
			int cellIdx = source[i];
			int cx = cellIdx % gridCols;
			int cz = cellIdx / gridCols;
			int key = (cz / factor) * targetCols + (cx / factor);
			int slot;
			if (slots.Find(key, slot))
			{
				target.Set(slot + 1, target[slot + 1] + source[i + 1]);
				target.Set(slot + 2, target[slot + 2] | source[i + 2]);
				target.Set(slot + 3, target[slot + 3] + source[i + 3]);
				continue;
			}

			slots.Insert(key, target.Count());
			target.Insert(key);
			target.Insert(source[i + 1]);
			target.Insert(source[i + 2]);
			target.Insert(source[i + 3]);
		}
	}
};

class VPPXEMapRenderer : Managed
{
	const static int MODE_HEAT = 0;
	const static int MODE_PINS = 1;
	const static int MAX_LINES = 16000;
	const static int HEAT_LINES = 12000;
	const static int MARKER_LIMIT = 600;
	const static int PIN_LIMIT = 300;
	const static int LAYER_TOTAL = 8;
	const static float MIN_CELL_PX = 6.0;
	const static float BIN_PX = 12.0;
	const static float HIT_PX = 12.0;
	const static float MARKER_PX = 9.0;
	// Heat blobs are drawn this much larger than their cell so neighbours overlap and the grid disappears.
	const static float BLOB_SCALE = 1.3;
	const static string PIN_TEXTURE = "VPPAdminTools\\GUI\\Textures\\CustomMapIcons\\waypoint_CA.paa";

	protected MapWidget m_Map;
	protected CanvasWidget m_Canvas;
	protected ref array<ref VPPXEMapLayerData> m_Layers;
	protected int m_CellSize;
	protected int m_GridCols;
	protected int m_VisibleMask;
	protected int m_TierMask;
	protected int m_Mode;
	protected bool m_PinsShown;
	protected bool m_HasSelPos;
	protected vector m_SelPos;

	protected float m_CX;
	protected float m_CY;
	protected float m_CW;
	protected float m_CH;
	protected float m_S0X;
	protected float m_S0Y;
	protected float m_AX;
	protected float m_AZ;
	protected int m_LineCount;

	protected ref array<int> m_HitMarkLayer;
	protected ref array<int> m_HitMarkIdx;
	protected ref array<int> m_HitMarkCount;
	protected ref array<int> m_HitMarkPoints;
	protected ref array<float> m_HitMarkX;
	protected ref array<float> m_HitMarkZ;
	protected ref array<int> m_HitCellLayer;
	protected ref array<int> m_HitCellCount;
	protected ref array<int> m_HitCellPoints;
	protected ref array<int> m_HitCellTier;
	protected ref array<float> m_HitCellX0;
	protected ref array<float> m_HitCellZ0;
	protected ref array<float> m_HitCellX1;
	protected ref array<float> m_HitCellZ1;
	protected int m_LastHitCount;
	protected int m_LastHitPoints;
	protected int m_LastHitTier;

	protected ref array<float> m_TmpX0;
	protected ref array<float> m_TmpY0;
	protected ref array<float> m_TmpX1;
	protected ref array<float> m_TmpY1;
	protected ref array<int> m_TmpCount;
	protected ref array<int> m_TmpPoints;
	protected ref array<int> m_TmpTier;
	protected ref map<int, int> m_BinSlots;

	void VPPXEMapRenderer(MapWidget mapWidget, CanvasWidget canvasWidget)
	{
		m_Map = mapWidget;
		m_Canvas = canvasWidget;
		m_CellSize = VPPXEConst.CELL_SIZE;
		m_GridCols = 0;
		m_VisibleMask = 127;
		m_TierMask = 255;
		m_Mode = MODE_HEAT;
		m_HitMarkLayer = new array<int>;
		m_HitMarkIdx = new array<int>;
		m_HitMarkCount = new array<int>;
		m_HitMarkPoints = new array<int>;
		m_HitMarkX = new array<float>;
		m_HitMarkZ = new array<float>;
		m_HitCellLayer = new array<int>;
		m_HitCellCount = new array<int>;
		m_HitCellPoints = new array<int>;
		m_HitCellTier = new array<int>;
		m_HitCellX0 = new array<float>;
		m_HitCellZ0 = new array<float>;
		m_HitCellX1 = new array<float>;
		m_HitCellZ1 = new array<float>;
		m_TmpX0 = new array<float>;
		m_TmpY0 = new array<float>;
		m_TmpX1 = new array<float>;
		m_TmpY1 = new array<float>;
		m_TmpCount = new array<int>;
		m_TmpPoints = new array<int>;
		m_TmpTier = new array<int>;
		m_BinSlots = new map<int, int>;
	}

	// summary may be null (live-only data); layers holds 8 entries indexed by VPPXELayer.
	void SetData(VPPXEDistSummary summary, array<ref VPPXEMapLayerData> layers)
	{
		m_Layers = layers;
		if (!summary)
		{
			return;
		}

		if (summary.CellSize > 0)
		{
			m_CellSize = summary.CellSize;
		}

		m_GridCols = summary.GridCols;
	}

	void SetVisibleLayers(int mask)
	{
		m_VisibleMask = mask;
	}

	void SetTierFilter(int mask)
	{
		m_TierMask = mask;
	}

	void SetMode(int mode)
	{
		m_Mode = mode;
	}

	int GetMode()
	{
		return m_Mode;
	}

	// HEAT redraws follow the view; PINS are world-space user marks and only change with the data.
	bool IsViewDependent()
	{
		return m_Mode == MODE_HEAT;
	}

	void SetSelection(vector worldPos, bool hasPos)
	{
		m_SelPos = worldPos;
		m_HasSelPos = hasPos;
	}

	int GetLastHitCount()
	{
		return m_LastHitCount;
	}

	int GetLastHitPoints()
	{
		return m_LastHitPoints;
	}

	int GetLastHitTier()
	{
		return m_LastHitTier;
	}

	float GetCanvasWidth()
	{
		return m_CW;
	}

	float GetCanvasHeight()
	{
		return m_CH;
	}

	float GetPixelsPerMetre()
	{
		return Math.AbsFloat(m_AX);
	}

	// Canvas rect in screen pixels, read from the map widget: the canvas has the same rect at rest, but the map
	// tab slides it while a pan is previewed. MapWidget.MapToScreen answers ABSOLUTE screen pixels and ScreenToMap
	// takes them (engine: both add / remove the widget's own position), so the canvas offset is always the widget's
	// screen position. The projection is sampled at the world point in the middle of the widget (GetMapPos is the
	// point under the widget's internal anchor, not necessarily the middle): two samples 100 m apart give the
	// per-axis scale (z is flipped on screen) and the screen position of world 0,0 is extrapolated from them.
	bool Calibrate()
	{
		if (!m_Map || !m_Canvas)
		{
			return false;
		}

		m_Map.GetScreenPos(m_CX, m_CY);
		m_Map.GetScreenSize(m_CW, m_CH);
		if (m_CW < 2 || m_CH < 2)
		{
			return false;
		}

		vector centre = m_Map.ScreenToMap(Vector(m_CX + m_CW * 0.5, m_CY + m_CH * 0.5, 0));
		vector sample0 = m_Map.MapToScreen(centre);
		vector sample1 = m_Map.MapToScreen(Vector(centre[0] + 100, 0, centre[2] + 100));
		m_AX = (sample1[0] - sample0[0]) / 100.0;
		m_AZ = (sample1[1] - sample0[1]) / 100.0;
		if (Math.AbsFloat(m_AX) < 0.000001 || Math.AbsFloat(m_AZ) < 0.000001)
		{
			return false;
		}

		m_S0X = sample0[0] - m_CX - centre[0] * m_AX;
		m_S0Y = sample0[1] - m_CY - centre[2] * m_AZ;
		return true;
	}


	void ClearAll()
	{
		if (m_Canvas)
		{
			m_Canvas.Clear();
		}

		if (m_PinsShown && m_Map)
		{
			m_Map.ClearUserMarks();
		}

		m_PinsShown = false;
		ClearHitCache();
	}

	void Redraw()
	{
		ClearHitCache();
		m_LineCount = 0;
		if (!m_Canvas || !m_Map)
		{
			return;
		}

		m_Canvas.Clear();
		if (m_Mode == MODE_PINS)
		{
			DrawPins();
			return;
		}

		if (m_PinsShown)
		{
			m_Map.ClearUserMarks();
			m_PinsShown = false;
		}

		if (!m_Layers || !Calibrate())
		{
			return;
		}

		int markerMask = MarkerLayersMask();
		DrawHeat(VPPXELayer.CLUSTERS, markerMask, true);
		DrawHeat(VPPXELayer.BUILDINGS, markerMask, false);
		DrawHeat(VPPXELayer.DISPATCH, markerMask, false);
		DrawHeat(VPPXELayer.EVENT_CHILD, markerMask, false);
		DrawHeat(VPPXELayer.EVENT_OBJECTS, markerMask, false);
		DrawCircles(VPPXELayer.INFECTED);
		DrawMarkers(VPPXELayer.CLUSTERS, markerMask);
		DrawMarkers(VPPXELayer.BUILDINGS, markerMask);
		DrawMarkers(VPPXELayer.DISPATCH, markerMask);
		DrawMarkers(VPPXELayer.EVENT_CHILD, markerMask);
		DrawMarkers(VPPXELayer.EVENT_OBJECTS, markerMask);
		DrawMarkers(VPPXELayer.LIVE, markerMask);
		DrawSelection();
	}

	// sx/sy are absolute screen pixels (GetMousePos). Nearest drawn marker within 12 px, else the drawn
	// cell under the cursor. markerIndex is the marker (or circle) index in its layer, -1 for cells.
	bool HitTest(int sx, int sy, out int layer, out int markerIndex, out vector worldPos)
	{
		layer = -1;
		markerIndex = -1;
		worldPos = vector.Zero;
		m_LastHitCount = 0;
		m_LastHitPoints = 0;
		m_LastHitTier = 0;
		if (!Calibrate())
		{
			return false;
		}

		float lx = sx - m_CX;
		float ly = sy - m_CY;
		if (lx < 0 || ly < 0 || lx > m_CW || ly > m_CH)
		{
			return false;
		}

		int best = -1;
		float bestDist = HIT_PX * HIT_PX;
		int markCount = m_HitMarkIdx.Count();
		for (int i = 0; i < markCount; i++)
		{
			float dx = m_S0X + m_HitMarkX[i] * m_AX - lx;
			float dy = m_S0Y + m_HitMarkZ[i] * m_AZ - ly;
			float dist = dx * dx + dy * dy;
			if (dist <= bestDist)
			{
				bestDist = dist;
				best = i;
			}
		}

		if (best >= 0)
		{
			layer = m_HitMarkLayer[best];
			markerIndex = m_HitMarkIdx[best];
			worldPos = Vector(m_HitMarkX[best], 0, m_HitMarkZ[best]);
			m_LastHitCount = m_HitMarkCount[best];
			m_LastHitPoints = m_HitMarkPoints[best];
			return true;
		}

		float wx = (lx - m_S0X) / m_AX;
		float wz = (ly - m_S0Y) / m_AZ;
		for (int j = m_HitCellLayer.Count() - 1; j >= 0; j--)
		{
			if (wx < m_HitCellX0[j] || wx > m_HitCellX1[j] || wz < m_HitCellZ0[j] || wz > m_HitCellZ1[j])
			{
				continue;
			}

			layer = m_HitCellLayer[j];
			worldPos = Vector((m_HitCellX0[j] + m_HitCellX1[j]) * 0.5, 0, (m_HitCellZ0[j] + m_HitCellZ1[j]) * 0.5);
			m_LastHitCount = m_HitCellCount[j];
			m_LastHitPoints = m_HitCellPoints[j];
			m_LastHitTier = m_HitCellTier[j];
			return true;
		}

		return false;
	}

	protected VPPXEMapLayerData LayerAt(int layer)
	{
		if (!m_Layers || layer < 0 || layer >= m_Layers.Count())
		{
			return null;
		}

		return m_Layers[layer];
	}

	protected bool LayerVisible(int layer)
	{
		return (m_VisibleMask & (1 << layer)) != 0;
	}

	// A cell or marker without tier information always passes.
	protected bool TierPasses(int tier)
	{
		if (tier == 0)
		{
			return true;
		}

		return (tier & m_TierMask) != 0;
	}

	// Public so the map tab can colour its legend swatches with the exact draw colours.
	static int LayerColor(int layer)
	{
		if (layer == VPPXELayer.BUILDINGS)
		{
			return ARGB(255, 217, 178, 61);
		}

		if (layer == VPPXELayer.EVENT_OBJECTS)
		{
			return ARGB(255, 232, 163, 61);
		}

		if (layer == VPPXELayer.EVENT_CHILD)
		{
			return ARGB(255, 76, 175, 80);
		}

		if (layer == VPPXELayer.INFECTED)
		{
			return ARGB(255, 194, 69, 69);
		}

		if (layer == VPPXELayer.CLUSTERS)
		{
			return ARGB(255, 120, 200, 110);
		}

		if (layer == VPPXELayer.DISPATCH)
		{
			return ARGB(255, 61, 125, 214);
		}

		if (layer == VPPXELayer.LIVE)
		{
			return ARGB(255, 0, 229, 255);
		}

		return ARGB(255, 236, 64, 200);
	}

	// Heat ramp accent-blue, warning-yellow, accent-orange, danger-red (alpha 140 on the map, opaque in the
	// legend); clusters use a green ramp.
	static int RampColor(int step, bool green, int alpha = 140)
	{
		if (green)
		{
			if (step <= 0)
			{
				return ARGB(alpha, 46, 94, 50);
			}

			if (step == 1)
			{
				return ARGB(alpha, 76, 175, 80);
			}

			if (step == 2)
			{
				return ARGB(alpha, 120, 200, 110);
			}

			return ARGB(alpha, 175, 230, 150);
		}

		if (step <= 0)
		{
			return ARGB(alpha, 61, 125, 214);
		}

		if (step == 1)
		{
			return ARGB(alpha, 217, 178, 61);
		}

		if (step == 2)
		{
			return ARGB(alpha, 232, 163, 61);
		}

		return ARGB(alpha, 194, 69, 69);
	}

	protected void ClearHitCache()
	{
		m_HitMarkLayer.Clear();
		m_HitMarkIdx.Clear();
		m_HitMarkCount.Clear();
		m_HitMarkPoints.Clear();
		m_HitMarkX.Clear();
		m_HitMarkZ.Clear();
		m_HitCellLayer.Clear();
		m_HitCellCount.Clear();
		m_HitCellPoints.Clear();
		m_HitCellTier.Clear();
		m_HitCellX0.Clear();
		m_HitCellZ0.Clear();
		m_HitCellX1.Clear();
		m_HitCellZ1.Clear();
	}

	protected void ClearTmp()
	{
		m_TmpX0.Clear();
		m_TmpY0.Clear();
		m_TmpX1.Clear();
		m_TmpY1.Clear();
		m_TmpCount.Clear();
		m_TmpPoints.Clear();
		m_TmpTier.Clear();
	}

	protected void AddTmp(float x0, float y0, float x1, float y1, int count, int points, int tier)
	{
		m_TmpX0.Insert(x0);
		m_TmpY0.Insert(y0);
		m_TmpX1.Insert(x1);
		m_TmpY1.Insert(y1);
		m_TmpCount.Insert(count);
		m_TmpPoints.Insert(points);
		m_TmpTier.Insert(tier);
	}

	protected void AddHitMark(int layer, int index, int count, int points, float wx, float wz)
	{
		m_HitMarkLayer.Insert(layer);
		m_HitMarkIdx.Insert(index);
		m_HitMarkCount.Insert(count);
		m_HitMarkPoints.Insert(points);
		m_HitMarkX.Insert(wx);
		m_HitMarkZ.Insert(wz);
	}

	// Stores the world rect of a drawn canvas-local rect for hit testing.
	protected void AddHitCell(int layer, float x0, float y0, float x1, float y1, int count, int points, int tier)
	{
		float wxa = (x0 - m_S0X) / m_AX;
		float wxb = (x1 - m_S0X) / m_AX;
		float wza = (y0 - m_S0Y) / m_AZ;
		float wzb = (y1 - m_S0Y) / m_AZ;
		m_HitCellLayer.Insert(layer);
		m_HitCellCount.Insert(count);
		m_HitCellPoints.Insert(points);
		m_HitCellTier.Insert(tier);
		m_HitCellX0.Insert(Math.Min(wxa, wxb));
		m_HitCellX1.Insert(Math.Max(wxa, wxb));
		m_HitCellZ0.Insert(Math.Min(wza, wzb));
		m_HitCellZ1.Insert(Math.Max(wza, wzb));
	}

	// Filled rect as one wide vertical line (VPPUIManager pattern), clipped to the canvas.
	protected void DrawRect(float x0, float y0, float x1, float y1, int color)
	{
		if (m_LineCount >= MAX_LINES)
		{
			return;
		}

		float left = Math.Max(Math.Min(x0, x1), 0);
		float right = Math.Min(Math.Max(x0, x1), m_CW);
		float top = Math.Max(Math.Min(y0, y1), 0);
		float bottom = Math.Min(Math.Max(y0, y1), m_CH);
		if (right - left < 0.5 || bottom - top < 0.5)
		{
			return;
		}

		float mid = (left + right) * 0.5;
		m_Canvas.DrawLine(mid, top, mid, bottom, right - left, color);
		m_LineCount++;
	}

	// Filled disc as horizontal bands (3, 5 or 7 by radius; one square under 3 px). Bands never overlap, so a
	// translucent colour stays even inside one disc.
	protected void DrawDisc(float cx, float cy, float r, int color)
	{
		if (r < 3)
		{
			DrawRect(cx - r, cy - r, cx + r, cy + r, color);
			return;
		}

		int strips = 3;
		if (r >= 16)
		{
			strips = 7;
		}
		else if (r >= 7)
		{
			strips = 5;
		}

		float band = r * 2 / strips;
		for (int s = 0; s < strips; s++)
		{
			float top = cy - r + band * s;
			float offset = top + band * 0.5 - cy;
			float half = Math.Sqrt(Math.Max(r * r - offset * offset, 0));
			DrawRect(cx - half, top, cx + half, top + band, color);
		}
	}

	// Line segment; skipped when both ends are outside the canvas, otherwise its ends are clamped to it.
	protected void DrawSegment(float x0, float y0, float x1, float y1, float width, int color)
	{
		if (m_LineCount >= MAX_LINES)
		{
			return;
		}

		bool inside0 = x0 >= 0 && y0 >= 0 && x0 <= m_CW && y0 <= m_CH;
		bool inside1 = x1 >= 0 && y1 >= 0 && x1 <= m_CW && y1 <= m_CH;
		if (!inside0 && !inside1)
		{
			return;
		}

		float ax = Math.Clamp(x0, 0, m_CW);
		float ay = Math.Clamp(y0, 0, m_CH);
		float bx = Math.Clamp(x1, 0, m_CW);
		float by = Math.Clamp(y1, 0, m_CH);
		m_Canvas.DrawLine(ax, ay, bx, by, width, color);
		m_LineCount++;
	}

	protected int CountVisibleMarkers(VPPXEMapLayerData data)
	{
		int visible = 0;
		int total = data.MarkMeta.Count();
		for (int i = 0; i < total; i++)
		{
			int meta = data.MarkMeta[i];
			if (meta < 0 || !TierPasses(meta & 255))
			{
				continue;
			}

			float lx = m_S0X + data.MarkX[i] * m_AX;
			float ly = m_S0Y + data.MarkZ[i] * m_AZ;
			if (lx < 0 || ly < 0 || lx > m_CW || ly > m_CH)
			{
				continue;
			}

			visible++;
			if (visible >= MARKER_LIMIT)
			{
				return visible;
			}
		}

		return visible;
	}

	// Bit per layer that is drawn as markers this frame: markers exist and fewer than 600 are visible
	// (LIVE has no cells, so its markers are always drawn up to the line guard).
	protected int MarkerLayersMask()
	{
		int mask = 0;
		for (int layer = 0; layer < LAYER_TOTAL; layer++)
		{
			VPPXEMapLayerData data = LayerAt(layer);
			if (!data || data.MarkMeta.Count() == 0 || !LayerVisible(layer))
			{
				continue;
			}

			if (layer == VPPXELayer.LIVE || CountVisibleMarkers(data) < MARKER_LIMIT)
			{
				mask = mask | (1 << layer);
			}
		}

		return mask;
	}

	// Finest level whose cell is at least 6 px; below 12 px the chosen level is rebinned to 12 px screen bins.
	protected void DrawHeat(int layer, int markerMask, bool green)
	{
		if (!LayerVisible(layer) || (markerMask & (1 << layer)) != 0 || m_LineCount >= HEAT_LINES)
		{
			return;
		}

		VPPXEMapLayerData data = LayerAt(layer);
		if (!data || data.Cells.Count() == 0 || m_GridCols <= 0)
		{
			return;
		}

		float pxPerM = Math.AbsFloat(m_AX);
		int factor = 1;
		array<int> cells = data.Cells;
		if (m_CellSize * pxPerM < MIN_CELL_PX)
		{
			factor = 5;
			cells = data.Cells500;
			if (m_CellSize * 5 * pxPerM < MIN_CELL_PX)
			{
				factor = 20;
				cells = data.Cells2000;
			}
		}

		int cols = (m_GridCols + factor - 1) / factor;
		float sizeM = m_CellSize * factor;
		bool rebin = sizeM * pxPerM < BIN_PX;
		ClearTmp();
		m_BinSlots.Clear();
		int cellInts = cells.Count();
		for (int i = 0; i + 3 < cellInts; i += 4)
		{
			int tier = cells[i + 2];
			if (!TierPasses(tier))
			{
				continue;
			}

			int cellIdx = cells[i];
			int cx = cellIdx % cols;
			int cz = cellIdx / cols;
			float lx0 = m_S0X + cx * sizeM * m_AX;
			float lx1 = lx0 + sizeM * m_AX;
			float ly0 = m_S0Y + cz * sizeM * m_AZ;
			float ly1 = ly0 + sizeM * m_AZ;
			float left = Math.Min(lx0, lx1);
			float right = Math.Max(lx0, lx1);
			float top = Math.Min(ly0, ly1);
			float bottom = Math.Max(ly0, ly1);
			if (right < 0 || left > m_CW || bottom < 0 || top > m_CH)
			{
				continue;
			}

			if (!rebin)
			{
				AddTmp(left, top, right, bottom, cells[i + 1], cells[i + 3], tier);
				continue;
			}

			float midX = (left + right) * 0.5;
			float midY = (top + bottom) * 0.5;
			if (midX < 0 || midY < 0 || midX >= m_CW || midY >= m_CH)
			{
				continue;
			}

			int bx = Math.Floor(midX / BIN_PX);
			int by = Math.Floor(midY / BIN_PX);
			int key = by * 10000 + bx;
			int slot;
			if (m_BinSlots.Find(key, slot))
			{
				m_TmpCount.Set(slot, m_TmpCount[slot] + cells[i + 1]);
				m_TmpPoints.Set(slot, m_TmpPoints[slot] + cells[i + 3]);
				m_TmpTier.Set(slot, m_TmpTier[slot] | tier);
				continue;
			}

			m_BinSlots.Insert(key, m_TmpCount.Count());
			AddTmp(bx * BIN_PX, by * BIN_PX, bx * BIN_PX + BIN_PX, by * BIN_PX + BIN_PX, cells[i + 1], cells[i + 3], tier);
		}

		FlushTmp(layer, green);
	}

	// Colours by sqrt(count / max) across the 4-step ramp.
	protected void FlushTmp(int layer, bool green)
	{
		int total = m_TmpCount.Count();
		if (total == 0)
		{
			return;
		}

		int maxCount = 1;
		for (int i = 0; i < total; i++)
		{
			if (m_TmpCount[i] > maxCount)
			{
				maxCount = m_TmpCount[i];
			}
		}

		float maxF = maxCount;
		for (int j = 0; j < total; j++)
		{
			if (m_LineCount >= HEAT_LINES)
			{
				return;
			}

			float countF = m_TmpCount[j];
			float ratio = Math.Sqrt(countF / maxF);
			int step = Math.Floor(ratio * 4);
			if (step > 3)
			{
				step = 3;
			}

			float blobX = (m_TmpX0[j] + m_TmpX1[j]) * 0.5;
			float blobY = (m_TmpY0[j] + m_TmpY1[j]) * 0.5;
			float blobR = Math.Max(m_TmpX1[j] - m_TmpX0[j], m_TmpY1[j] - m_TmpY0[j]) * 0.5 * BLOB_SCALE;
			DrawDisc(blobX, blobY, blobR, RampColor(step, green));
			AddHitCell(layer, m_TmpX0[j], m_TmpY0[j], m_TmpX1[j], m_TmpY1[j], m_TmpCount[j], m_TmpPoints[j], m_TmpTier[j]);
		}
	}

	// 32-segment danger-red outlines of width 2 (16 segments under 12 px radius, a dot under 3 px).
	protected void DrawCircles(int layer)
	{
		if (!LayerVisible(layer))
		{
			return;
		}

		VPPXEMapLayerData data = LayerAt(layer);
		if (!data)
		{
			return;
		}

		int color = LayerColor(layer);
		float pxPerM = Math.AbsFloat(m_AX);
		int floats = data.Circles.Count();
		for (int i = 0; i + 2 < floats; i += 3)
		{
			if (m_LineCount >= MAX_LINES)
			{
				return;
			}

			float wx = data.Circles[i];
			float wz = data.Circles[i + 1];
			float lx = m_S0X + wx * m_AX;
			float ly = m_S0Y + wz * m_AZ;
			float radiusPx = data.Circles[i + 2] * pxPerM;
			if (lx + radiusPx < 0 || lx - radiusPx > m_CW || ly + radiusPx < 0 || ly - radiusPx > m_CH)
			{
				continue;
			}

			AddHitMark(layer, i / 3, 1, 0, wx, wz);
			if (radiusPx < 3)
			{
				DrawRect(lx - 2, ly - 2, lx + 2, ly + 2, color);
				continue;
			}

			int segments = 32;
			if (radiusPx < 12)
			{
				segments = 16;
			}

			float angleStep = Math.PI2 / segments;
			float px0 = lx + radiusPx;
			float py0 = ly;
			for (int s = 1; s <= segments; s++)
			{
				float angle = angleStep * s;
				float px1 = lx + radiusPx * Math.Cos(angle);
				float py1 = ly + radiusPx * Math.Sin(angle);
				DrawSegment(px0, py0, px1, py1, 2, color);
				px0 = px1;
				py0 = py1;
			}
		}
	}

	// 9 px discs on a dark outline so they read on the satellite map; LIVE markers carry no tier and are never
	// filtered by it.
	protected void DrawMarkers(int layer, int markerMask)
	{
		if ((markerMask & (1 << layer)) == 0)
		{
			return;
		}

		VPPXEMapLayerData data = LayerAt(layer);
		if (!data)
		{
			return;
		}

		int color = LayerColor(layer);
		int outline = ARGB(230, 8, 9, 10);
		float half = MARKER_PX * 0.5;
		int total = data.MarkMeta.Count();
		for (int i = 0; i < total; i++)
		{
			if (m_LineCount >= MAX_LINES)
			{
				return;
			}

			int meta = data.MarkMeta[i];
			if (meta < 0 || !TierPasses(meta & 255))
			{
				continue;
			}

			float lx = m_S0X + data.MarkX[i] * m_AX;
			float ly = m_S0Y + data.MarkZ[i] * m_AZ;
			if (lx < 0 || ly < 0 || lx > m_CW || ly > m_CH)
			{
				continue;
			}

			DrawDisc(lx, ly, half + 2, outline);
			DrawDisc(lx, ly, half, color);
			AddHitMark(layer, i, 1, 0, data.MarkX[i], data.MarkZ[i]);
		}
	}

	protected void DrawSelection()
	{
		if (!m_HasSelPos)
		{
			return;
		}

		float lx = m_S0X + m_SelPos[0] * m_AX;
		float ly = m_S0Y + m_SelPos[2] * m_AZ;
		if (lx < 0 || ly < 0 || lx > m_CW || ly > m_CH)
		{
			return;
		}

		int white = ARGB(255, 255, 255, 255);
		int dark = ARGB(230, 8, 9, 10);
		float r = 12;
		float tick = 9;
		DrawSegment(lx - r, ly - r, lx + r, ly - r, 5, dark);
		DrawSegment(lx + r, ly - r, lx + r, ly + r, 5, dark);
		DrawSegment(lx + r, ly + r, lx - r, ly + r, 5, dark);
		DrawSegment(lx - r, ly + r, lx - r, ly - r, 5, dark);
		DrawSegment(lx - r - tick, ly, lx - r + 3, ly, 5, dark);
		DrawSegment(lx + r - 3, ly, lx + r + tick, ly, 5, dark);
		DrawSegment(lx, ly - r - tick, lx, ly - r + 3, 5, dark);
		DrawSegment(lx, ly + r - 3, lx, ly + r + tick, 5, dark);
		DrawSegment(lx - r, ly - r, lx + r, ly - r, 2, white);
		DrawSegment(lx + r, ly - r, lx + r, ly + r, 2, white);
		DrawSegment(lx + r, ly + r, lx - r, ly + r, 2, white);
		DrawSegment(lx - r, ly + r, lx - r, ly - r, 2, white);
		DrawSegment(lx - r - tick, ly, lx - r + 3, ly, 2, white);
		DrawSegment(lx + r - 3, ly, lx + r + tick, ly, 2, white);
		DrawSegment(lx, ly - r - tick, lx, ly - r + 3, 2, white);
		DrawSegment(lx, ly + r - 3, lx, ly + r + tick, 2, white);
	}

	// Pins fallback: the top 300 cells (label = count) or markers/zone centres of the visible layers,
	// ordered with one native Sort over fixed-width keys (count descending, then candidate index).
	protected void DrawPins()
	{
		m_Map.ClearUserMarks();
		m_PinsShown = true;
		if (!m_Layers)
		{
			return;
		}

		array<float> pinX = new array<float>;
		array<float> pinZ = new array<float>;
		array<int> pinCount = new array<int>;
		array<int> pinPoints = new array<int>;
		array<int> pinLayer = new array<int>;
		array<int> pinIdx = new array<int>;
		for (int layer = 0; layer < LAYER_TOTAL; layer++)
		{
			VPPXEMapLayerData data = LayerAt(layer);
			if (!data || !LayerVisible(layer))
			{
				continue;
			}

			if (data.Cells.Count() > 0 && m_GridCols > 0)
			{
				int cellInts = data.Cells.Count();
				for (int i = 0; i + 3 < cellInts; i += 4)
				{
					if (!TierPasses(data.Cells[i + 2]))
					{
						continue;
					}

					int cellIdx = data.Cells[i];
					int cx = cellIdx % m_GridCols;
					int cz = cellIdx / m_GridCols;
					pinX.Insert(cx * m_CellSize + m_CellSize * 0.5);
					pinZ.Insert(cz * m_CellSize + m_CellSize * 0.5);
					pinCount.Insert(data.Cells[i + 1]);
					pinPoints.Insert(data.Cells[i + 3]);
					pinLayer.Insert(layer);
					pinIdx.Insert(-1);
				}

				continue;
			}

			int markers = data.MarkMeta.Count();
			for (int m = 0; m < markers; m++)
			{
				int meta = data.MarkMeta[m];
				if (meta < 0 || !TierPasses(meta & 255))
				{
					continue;
				}

				pinX.Insert(data.MarkX[m]);
				pinZ.Insert(data.MarkZ[m]);
				pinCount.Insert(1);
				pinPoints.Insert(0);
				pinLayer.Insert(layer);
				pinIdx.Insert(m);
			}

			int circleFloats = data.Circles.Count();
			for (int c = 0; c + 2 < circleFloats; c += 3)
			{
				pinX.Insert(data.Circles[c]);
				pinZ.Insert(data.Circles[c + 1]);
				pinCount.Insert(1);
				pinPoints.Insert(0);
				pinLayer.Insert(layer);
				pinIdx.Insert(c / 3);
			}
		}

		int candidates = pinX.Count();
		if (candidates == 0)
		{
			return;
		}

		array<string> keys = new array<string>;
		for (int k = 0; k < candidates; k++)
		{
			int clamped = pinCount[k];
			if (clamped > 99999999)
			{
				clamped = 99999999;
			}

			string sortKey = VPPXmlText.PadInt(99999999 - clamped, 8) + VPPXmlText.PadInt(k, 7);
			keys.Insert(sortKey);
		}

		keys.Sort();
		int limit = keys.Count();
		if (limit > PIN_LIMIT)
		{
			limit = PIN_LIMIT;
		}

		for (int q = 0; q < limit; q++)
		{
			string picked = keys[q];
			int src = picked.Substring(8, 7).ToInt();
			string label = "";
			int labelCount = pinCount[src];
			if (labelCount > 1)
			{
				label = labelCount.ToString();
			}

			m_Map.AddUserMark(Vector(pinX[src], 0, pinZ[src]), label, LayerColor(pinLayer[src]), PIN_TEXTURE);
			AddHitMark(pinLayer[src], pinIdx[src], pinCount[src], pinPoints[src], pinX[src], pinZ[src]);
		}
	}
};
