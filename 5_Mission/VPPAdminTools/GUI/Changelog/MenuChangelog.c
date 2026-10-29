/*
	Changelog window: a fixed 760x660 AdminHudSubMenu, in layout units.
	Opened by VPPAdminHud.OpenChangelog, both on toolbar open and from the Stats HUD button.
	Rows come from VPPChangelogContent.Build and are measured again every frame for MEASURE_FRAMES frames after a build or show.
	Resolution: Main is "scaled 1" (one layout unit is one pixel at 1080p, more above) but rootFrame is an exact pixel
	size on the unscaled Panel_Content, so ApplyUiScale sizes rootFrame and the drag Header to WINDOW_W x WINDOW_H units
	(pixels per unit measured on the 36-unit HeaderTitle). Script sizes inside Main go through VPPXEScale, like the XML
	Editor, since the engine does not read them with the layout's scale above 1080p.
*/
class MenuChangelog extends AdminHudSubMenu
{
	protected const float WINDOW_W = 760.0;
	protected const float WINDOW_H = 660.0;
	protected const float HEADER_H = 36.0;
	protected const float DRAG_HEADER_W = 0.82;
	protected const float REF_SCREEN_H = 1080.0;
	protected const float ROW_W = 712.0;
	protected const float BODY_Y = 4.0;
	protected const float BODY_X_CHIP = 146.0;
	protected const float BODY_X_BULLET = 166.0;
	protected const float BODY_X_TEXT = 8.0;
	protected const float BODY_RIGHT_PAD = 8.0;
	//RichTextWidget line-breaks with a narrower measure than it renders with (lines drew ~6% past the box and slid under the scrollbar), so it wraps inside this fraction of the column
	protected const float WRAP_FIT = 0.9;
	//line pitch of the 17 px body text; GetContentHeight read about twice the drawn height, so heights come from GetNumLines instead
	protected const float LINE_PITCH = 25.0;
	protected const float ROW_PAD_V = 8.0;
	protected const float MIN_ROW_H = 30.0;
	protected const float VERSION_X = 12.0;
	protected const float LATEST_GAP = 10.0;
	//line breaks settle only after the first layout passes, so rows keep being re-measured for about a second
	protected const int MEASURE_FRAMES = 60;
	//screensaver-style image bouncing around the window, one of BUDDY_IMAGES picked at random each time it opens: height
	//as a fraction of the screen, speed per axis in screen heights per second, gap kept to the window
	protected const int BUDDY_IMAGES = 3;
	protected const float BUDDY_H_FRAC = 0.1;
	protected const float BUDDY_SPEED_FRAC = 0.12;
	protected const float BUDDY_GAP = 6.0;
	protected const int BUDDY_TINTS = 6;

	protected ScrollWidget m_ScrollBody;
	protected WrapSpacerWidget m_WrapBody;
	protected CheckBoxWidget m_ChkDontShow;
	protected ButtonWidget m_BtnCloseFooter;
	protected TextWidget m_TxtSubTitle;

	//LATEST chip of the first section, re-placed after the version text has been measured
	protected Widget m_LatestChip;
	protected TextWidget m_LatestLabel;
	protected TextWidget m_LatestVersion;

	protected ref array<ref VPPChangelogLinkRow> m_LinkRows;	//keeps the link handlers alive
	protected ref array<Widget> m_TextRows;
	protected ref array<RichTextWidget> m_TextBodies;
	protected ref array<float> m_RowHeights;	//last height set per text row, in layout units
	protected int m_MeasureFrames;

	//screen pixels per layout unit inside Main (1 at 1080p); 0 = not applied yet
	protected Widget m_HeaderTitle;
	protected Widget m_ScaleProbe;
	protected float m_UnitPx;
	//VPPXEScale ratio the rows were built with (the XML Editor can measure it first)
	protected float m_BuiltRatio = 1.0;

	//sibling of the window on Panel_Content (hides with the HUD), positions in Panel_Content pixels
	protected ImageWidget m_Buddy;
	protected float m_BuddyX;
	protected float m_BuddyY;
	protected float m_BuddyW;
	protected float m_BuddyH;
	protected float m_BuddyVX;
	protected float m_BuddyVY;
	protected int m_BuddyTint;
	protected int m_BuddyImage = -1;
	//refreshed by RefreshBuddyRects: the area size and the window rect relative to the area
	protected float m_AreaW;
	protected float m_AreaH;
	protected float m_WinX;
	protected float m_WinY;
	protected float m_WinW;
	protected float m_WinH;

	void MenuChangelog()
	{
		m_LinkRows = new array<ref VPPChangelogLinkRow>;
		m_TextRows = new array<Widget>;
		m_TextBodies = new array<RichTextWidget>;
		m_RowHeights = new array<float>;
	}

	void ~MenuChangelog()
	{
		if (m_Buddy)
			m_Buddy.Unlink();
	}

	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET = CreateWidgets(VPPATUIConstants.ChangelogMenu);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel = M_SUB_WIDGET.FindAnyWidget("Header");
		m_closeButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnClose"));

		m_TxtSubTitle = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("TxtSubTitle"));
		m_ScrollBody = ScrollWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ScrollBody"));
		m_WrapBody = WrapSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget("WrapBody"));
		m_ChkDontShow = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkDontShow"));
		m_BtnCloseFooter = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnCloseFooter"));
		m_HeaderTitle = M_SUB_WIDGET.FindAnyWidget("HeaderTitle");
		m_ScaleProbe = M_SUB_WIDGET.FindAnyWidget("ScaleProbe");

		ApplyUiScale();
		Rebuild();
		CreateBuddy();
	}

	//Panel_Content is full screen and unscaled, so these pixels are screen pixels
	protected void CenterWindow()
	{
		if (!M_SUB_WIDGET)
			return;

		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float px = (sw - WINDOW_W * m_UnitPx) * 0.5;
		float py = (sh - WINDOW_H * m_UnitPx) * 0.5;
		if (px < 0)
			px = 0;

		if (py < 0)
			py = 0;

		M_SUB_WIDGET.SetPos(px, py);
	}

	//rootFrame and the drag Header follow Main's scale; the rows are rebuilt when the script ratio changes.
	//HeaderTitle has no size before the first layout pass, so the first call estimates from the screen height
	protected void ApplyUiScale()
	{
		if (!M_SUB_WIDGET)
			return;

		float unitPx = 0;
		float titleW;
		float titleH;
		if (m_HeaderTitle)
		{
			m_HeaderTitle.GetScreenSize(titleW, titleH);
			if (titleH > 1)
				unitPx = titleH / HEADER_H;
		}

		if (unitPx <= 0)
		{
			int sw;
			int sh;
			GetScreenSize(sw, sh);
			unitPx = sh / REF_SCREEN_H;
		}

		if (unitPx <= 0)
			unitPx = 1.0;

		if (Math.AbsFloat(unitPx - m_UnitPx) >= 0.001)
		{
			m_UnitPx = unitPx;
			float windowW = WINDOW_W * unitPx;
			float windowH = WINDOW_H * unitPx;
			M_SUB_WIDGET.SetSize(windowW, windowH);
			if (m_TitlePanel)
			{
				float dragH = HEADER_H * unitPx;
				m_TitlePanel.SetSize(DRAG_HEADER_W, dragH);
			}

			CenterWindow();
		}

		if (!m_HeaderTitle || !m_ScaleProbe)
			return;

		VPPXEScale.Measure(m_HeaderTitle, HEADER_H, m_ScaleProbe);
		float ratio = VPPXEScale.Ratio();
		if (Math.AbsFloat(ratio - m_BuiltRatio) >= 0.001)
			Rebuild();
	}

	void Rebuild()
	{
		ClearRows();
		m_BuiltRatio = VPPXEScale.Ratio();
		VPPChangelogBuilder builder = new VPPChangelogBuilder();
		VPPChangelogContent content = new VPPChangelogContent();
		content.Build(builder);

		string subtitle = "";
		int sections = 0;
		int total = builder.Count();
		for (int i = 0; i < total; i++)
		{
			VPPChangelogEntry entry = builder.Get(i);
			if (!entry)
				continue;

			switch (entry.Kind)
			{
				case EVPPChangelogKind.SECTION:
				{
					if (sections > 0)
						AddGapRow();

					AddSectionRow(entry.Text, entry.Extra, sections == 0);
					if (sections == 0)
						subtitle = VPPChangelogMarkup.FormatSubtitle(entry.Text);

					sections++;
					break;
				}
				case EVPPChangelogKind.LINK:
				{
					AddLinkRow(entry.Text, entry.Extra);
					break;
				}
				case EVPPChangelogKind.GAP:
				{
					AddGapRow();
					break;
				}
				default:
				{
					AddTextRow(entry.Kind, entry.Text);
					break;
				}
			}
		}

		if (total == 0)
			AddTextRow(EVPPChangelogKind.PARAGRAPH, "#VSTR_CHANGELOG_EMPTY");

		if (m_TxtSubTitle)
			m_TxtSubTitle.SetText(subtitle);

		MeasureRows();
		m_MeasureFrames = MEASURE_FRAMES;
		if (m_ScrollBody)
			m_ScrollBody.VScrollToPos(0);
	}

	protected void ClearRows()
	{
		//unlink the row widgets first, then drop the handlers and references that point into them
		if (m_WrapBody)
		{
			Widget child = m_WrapBody.GetChildren();
			while (child)
			{
				Widget sibling = child.GetSibling();
				child.Unlink();
				child = sibling;
			}
		}

		m_LinkRows.Clear();
		m_TextRows.Clear();
		m_TextBodies.Clear();
		m_RowHeights.Clear();
		m_LatestChip = null;
		m_LatestLabel = null;
		m_LatestVersion = null;
	}

	//rows are parented to WrapBody through the workspace; AdminHudSubMenu.CreateWidgets would parent them to Panel_Content
	protected void AddSectionRow(string version, string dateText, bool latest)
	{
		if (!m_WrapBody)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ChangelogSectionRow, m_WrapBody);
		if (!row)
			return;

		TextWidget txtVersion = TextWidget.Cast(row.FindAnyWidget("TxtVersion"));
		TextWidget txtDate = TextWidget.Cast(row.FindAnyWidget("TxtDate"));
		Widget chip = row.FindAnyWidget("ChipLatest");
		TextWidget txtLatest = TextWidget.Cast(row.FindAnyWidget("TxtLatest"));
		if (txtVersion)
			txtVersion.SetText(VPPChangelogMarkup.FormatVersion(version));

		if (txtDate)
			txtDate.SetText(dateText);

		if (chip)
			chip.Show(latest);

		if (!latest)
			return;

		m_LatestChip = chip;
		m_LatestLabel = txtLatest;
		m_LatestVersion = txtVersion;
		PlaceLatestChip();
	}

	//text sizes can read 0 before the first layout pass, so MeasureRows calls this again every re-measure frame.
	//GetTextSize is in screen pixels, the chip is placed in layout units
	protected void PlaceLatestChip()
	{
		if (!m_LatestChip || !m_LatestVersion)
			return;

		float unitPx = m_UnitPx;
		if (unitPx <= 0)
			unitPx = 1.0;

		int versionW;
		int versionH;
		m_LatestVersion.Update();
		m_LatestVersion.GetTextSize(versionW, versionH);
		float versionUnits = 48;
		if (versionW > 0)
			versionUnits = versionW / unitPx;

		int labelW;
		int labelH;
		float labelUnits = 44;
		if (m_LatestLabel)
		{
			m_LatestLabel.Update();
			m_LatestLabel.GetTextSize(labelW, labelH);
			if (labelW > 0)
				labelUnits = labelW / unitPx;
		}

		float chipW = labelUnits + 16;
		float chipX = VERSION_X + versionUnits + LATEST_GAP;
		VPPXEScale.Size(m_LatestChip, chipW, 20);
		VPPXEScale.Pos(m_LatestChip, chipX, 0);
	}

	//chip entry (Added/Changed/Fixed/Removed/Known), bullet or paragraph; the body keeps the layout's text-primary colour
	protected void AddTextRow(int kind, string text)
	{
		if (!m_WrapBody)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ChangelogEntryRow, m_WrapBody);
		if (!row)
			return;

		Widget chip = row.FindAnyWidget("Chip");
		TextWidget txtChip = TextWidget.Cast(row.FindAnyWidget("TxtChip"));
		Widget bullet = row.FindAnyWidget("ImgBullet");
		RichTextWidget body = RichTextWidget.Cast(row.FindAnyWidget("TxtBody"));
		if (!body)
			return;

		if (chip)
			chip.Show(false);

		if (bullet)
			bullet.Show(false);

		float bodyX = BODY_X_CHIP;
		string chipLabel = VPPChangelogMarkup.GetChipLabel(kind);
		if (chipLabel != "")
		{
			if (chip)
			{
				chip.Show(true);
				chip.SetColor(VPPChangelogMarkup.GetChipColor(kind));
			}

			if (txtChip)
			{
				txtChip.SetText(chipLabel);
				txtChip.SetColor(VPPChangelogMarkup.GetChipTextColor(kind));
			}
		}
		else if (kind == EVPPChangelogKind.BULLET)
		{
			if (bullet)
				bullet.Show(true);

			bodyX = BODY_X_BULLET;
		}
		else
		{
			bodyX = BODY_X_TEXT;
		}

		float bodyW = (ROW_W - bodyX - BODY_RIGHT_PAD) * WRAP_FIT;
		VPPXEScale.Pos(body, bodyX, BODY_Y);
		VPPXEScale.Size(body, bodyW, LINE_PITCH);
		body.SetText(VPPChangelogMarkup.ToRichText(text));
		m_TextRows.Insert(row);
		m_TextBodies.Insert(body);
		m_RowHeights.Insert(-1);
	}

	protected void AddLinkRow(string label, string url)
	{
		if (!m_WrapBody)
			return;

		Widget row = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ChangelogLinkRow, m_WrapBody);
		if (!row)
			return;

		m_LinkRows.Insert(new VPPChangelogLinkRow(row, label, url));
	}

	protected void AddGapRow()
	{
		if (!m_WrapBody)
			return;

		GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ChangelogGapRow, m_WrapBody);
	}

	//row height = the engine's own line count * LINE_PITCH + padding; the spacer and scroller only re-layout when a height changed
	protected void MeasureRows()
	{
		bool changed = false;
		int rowCount = m_TextRows.Count();
		for (int i = 0; i < rowCount; i++)
		{
			Widget row = m_TextRows[i];
			RichTextWidget body = m_TextBodies[i];
			if (!row || !body)
				continue;

			body.Update();
			int lines = body.GetNumLines();
			if (lines < 1)
				lines = 1;

			float rowH = lines * LINE_PITCH + ROW_PAD_V;
			if (rowH < MIN_ROW_H)
				rowH = MIN_ROW_H;

			if (Math.AbsFloat(m_RowHeights[i] - rowH) < 0.5)
				continue;

			m_RowHeights.Set(i, rowH);
			VPPXEScale.Size(row, ROW_W, rowH);
			changed = true;
		}

		PlaceLatestChip();
		if (!changed)
			return;

		if (m_WrapBody)
			m_WrapBody.Update();

		if (m_ScrollBody)
			m_ScrollBody.Update();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible())
			return;

		ApplyUiScale();
		MoveBuddy(timeslice);
		if (m_MeasureFrames <= 0)
			return;

		m_MeasureFrames--;
		MeasureRows();
	}

	override void OnMenuShow()
	{
		super.OnMenuShow();
		if (m_ChkDontShow)
			m_ChkDontShow.SetChecked(VPPChangelogState.IsDontShow());

		VPPChangelogState.MarkSeen();
		if (m_ScrollBody)
			m_ScrollBody.VScrollToPos(0);

		m_MeasureFrames = MEASURE_FRAMES;
		m_BuddyW = 0;
		StartBuddy();
	}

	override void OnMenuHide()
	{
		super.OnMenuHide();
		if (m_Buddy)
			m_Buddy.Show(false);
	}

	/*
		Bouncing image: drifts at a constant speed, bounces off the screen edges and off the window (never overlaps it),
		and changes tint on every bounce, like the old DVD screensaver.
	*/
	protected void CreateBuddy()
	{
		if (!M_SUB_WIDGET || !M_SUB_WIDGET.GetParent())
			return;

		Widget created = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.ChangelogBuddy, M_SUB_WIDGET.GetParent());
		m_Buddy = ImageWidget.Cast(created);
		if (!m_Buddy)
			return;

		m_Buddy.Show(false);
		m_Buddy.SetSort(1);
	}

	//the area it moves in (Panel_Content) and the window relative to it; false while the area has no size yet
	protected bool RefreshBuddyRects()
	{
		Widget area = m_Buddy.GetParent();
		if (!area)
			return false;

		float areaX;
		float areaY;
		area.GetScreenPos(areaX, areaY);
		area.GetScreenSize(m_AreaW, m_AreaH);
		if (m_AreaW <= 1 || m_AreaH <= 1)
			return false;

		float screenX;
		float screenY;
		M_SUB_WIDGET.GetScreenPos(screenX, screenY);
		M_SUB_WIDGET.GetScreenSize(m_WinW, m_WinH);
		m_WinX = screenX - areaX;
		m_WinY = screenY - areaY;
		return true;
	}

	protected void StartBuddy()
	{
		if (!m_Buddy || !M_SUB_WIDGET)
			return;

		if (!RefreshBuddyRects())
			return;

		//a different image than last time
		int image = Math.RandomIntInclusive(0, BUDDY_IMAGES - 1);
		if (image == m_BuddyImage)
			image = (image + 1) % BUDDY_IMAGES;

		m_BuddyImage = image;
		string imagePath = BuddyImagePath(image);
		m_Buddy.LoadImageFile(0, imagePath);
		m_Buddy.SetImage(0);

		float aspect = BuddyAspect(image);
		m_BuddyH = Math.Round(m_AreaH * BUDDY_H_FRAC);
		m_BuddyW = Math.Round(m_BuddyH * aspect);
		m_Buddy.SetSize(m_BuddyW, m_BuddyH);

		float speed = m_AreaH * BUDDY_SPEED_FRAC;
		m_BuddyVX = speed;
		m_BuddyVY = speed;
		if (Math.RandomIntInclusive(0, 1) == 0)
			m_BuddyVX = -speed;

		if (Math.RandomIntInclusive(0, 1) == 0)
			m_BuddyVY = -speed;

		//first corner that is clear of the window; PushBuddyOut handles a window that covers all four
		m_BuddyX = BUDDY_GAP;
		m_BuddyY = BUDDY_GAP;
		int corner = Math.RandomIntInclusive(0, 3);
		for (int i = 0; i < 4; i++)
		{
			int pick = (corner + i) % 4;
			m_BuddyX = BUDDY_GAP;
			m_BuddyY = BUDDY_GAP;
			if (pick == 1 || pick == 3)
				m_BuddyX = m_AreaW - m_BuddyW - BUDDY_GAP;

			if (pick == 2 || pick == 3)
				m_BuddyY = m_AreaH - m_BuddyH - BUDDY_GAP;

			if (!BuddyHitsWindow(m_BuddyX, m_BuddyY))
				break;
		}

		m_BuddyTint = 0;
		int tint = BuddyTint(m_BuddyTint);
		m_Buddy.SetColor(tint);
		if (!PushBuddyOut())
		{
			m_Buddy.Show(false);
			return;
		}

		m_Buddy.SetPos(m_BuddyX, m_BuddyY);
		m_Buddy.Show(true);
	}

	protected void MoveBuddy(float timeslice)
	{
		if (!m_Buddy || !M_SUB_WIDGET)
			return;

		//the area can have no size yet on the first show: start once it has one
		if (m_BuddyW <= 0)
		{
			StartBuddy();
			return;
		}

		if (!RefreshBuddyRects())
			return;

		float dt = timeslice;
		if (dt > 0.1)
			dt = 0.1;

		float maxX = m_AreaW - m_BuddyW;
		float maxY = m_AreaH - m_BuddyH;
		bool bounced = false;

		//one axis at a time, so a corner hit flips both and a side hit only the one it touched
		float nextX = m_BuddyX + m_BuddyVX * dt;
		if (nextX < 0)
		{
			nextX = 0;
			m_BuddyVX = Math.AbsFloat(m_BuddyVX);
			bounced = true;
		}
		else if (nextX > maxX)
		{
			nextX = maxX;
			m_BuddyVX = -Math.AbsFloat(m_BuddyVX);
			bounced = true;
		}

		if (BuddyHitsWindow(nextX, m_BuddyY))
		{
			if (m_BuddyVX > 0)
				nextX = m_WinX - m_BuddyW - BUDDY_GAP;
			else
				nextX = m_WinX + m_WinW + BUDDY_GAP;

			m_BuddyVX = -m_BuddyVX;
			bounced = true;
		}

		m_BuddyX = nextX;

		float nextY = m_BuddyY + m_BuddyVY * dt;
		if (nextY < 0)
		{
			nextY = 0;
			m_BuddyVY = Math.AbsFloat(m_BuddyVY);
			bounced = true;
		}
		else if (nextY > maxY)
		{
			nextY = maxY;
			m_BuddyVY = -Math.AbsFloat(m_BuddyVY);
			bounced = true;
		}

		if (BuddyHitsWindow(m_BuddyX, nextY))
		{
			if (m_BuddyVY > 0)
				nextY = m_WinY - m_BuddyH - BUDDY_GAP;
			else
				nextY = m_WinY + m_WinH + BUDDY_GAP;

			m_BuddyVY = -m_BuddyVY;
			bounced = true;
		}

		m_BuddyY = nextY;

		//the window was dragged onto it (or a bounce left no room on that side)
		if (!PushBuddyOut())
		{
			m_Buddy.Show(false);
			return;
		}

		if (bounced)
		{
			m_BuddyTint = (m_BuddyTint + 1) % BUDDY_TINTS;
			int tint = BuddyTint(m_BuddyTint);
			m_Buddy.SetColor(tint);
		}

		m_Buddy.SetPos(m_BuddyX, m_BuddyY);
		if (!m_Buddy.IsVisible())
			m_Buddy.Show(true);
	}

	//uses the rects from the last RefreshBuddyRects
	protected bool BuddyHitsWindow(float x, float y)
	{
		if (x >= m_WinX + m_WinW + BUDDY_GAP || x + m_BuddyW <= m_WinX - BUDDY_GAP)
			return false;

		if (y >= m_WinY + m_WinH + BUDDY_GAP || y + m_BuddyH <= m_WinY - BUDDY_GAP)
			return false;

		return true;
	}

	//moves it to the closest free side of the window and back inside the area; false when it fits nowhere
	protected bool PushBuddyOut()
	{
		float maxX = m_AreaW - m_BuddyW;
		float maxY = m_AreaH - m_BuddyH;
		m_BuddyX = Math.Clamp(m_BuddyX, 0, maxX);
		m_BuddyY = Math.Clamp(m_BuddyY, 0, maxY);
		if (!BuddyHitsWindow(m_BuddyX, m_BuddyY))
			return true;

		float leftX = m_WinX - m_BuddyW - BUDDY_GAP;
		float rightX = m_WinX + m_WinW + BUDDY_GAP;
		float topY = m_WinY - m_BuddyH - BUDDY_GAP;
		float bottomY = m_WinY + m_WinH + BUDDY_GAP;

		float bestMove = -1;
		float bestX = m_BuddyX;
		float bestY = m_BuddyY;
		if (leftX >= 0)
		{
			bestMove = m_BuddyX - leftX;
			bestX = leftX;
			bestY = m_BuddyY;
		}

		if (rightX <= maxX && (bestMove < 0 || rightX - m_BuddyX < bestMove))
		{
			bestMove = rightX - m_BuddyX;
			bestX = rightX;
			bestY = m_BuddyY;
		}

		if (topY >= 0 && (bestMove < 0 || m_BuddyY - topY < bestMove))
		{
			bestMove = m_BuddyY - topY;
			bestX = m_BuddyX;
			bestY = topY;
		}

		if (bottomY <= maxY && (bestMove < 0 || bottomY - m_BuddyY < bestMove))
		{
			bestMove = bottomY - m_BuddyY;
			bestX = m_BuddyX;
			bestY = bottomY;
		}

		if (bestMove < 0)
			return false;

		m_BuddyX = bestX;
		m_BuddyY = bestY;
		return true;
	}

	protected string BuddyImagePath(int index)
	{
		int number = index + 1;
		return "VPPAdminTools/GUI/Textures/vpp_changelog_buddy_" + number.ToString() + ".paa";
	}

	//width / height of the original image (the textures are stretched to power-of-two sizes)
	protected float BuddyAspect(int index)
	{
		switch (index)
		{
			case 0:
				return 228.0 / 128.0;
			case 2:
				return 152.0 / 128.0;
		}

		return 1.0;
	}

	protected int BuddyTint(int index)
	{
		switch (index)
		{
			case 1:
				return ARGB(255, 255, 170, 170);
			case 2:
				return ARGB(255, 170, 210, 255);
			case 3:
				return ARGB(255, 180, 255, 180);
			case 4:
				return ARGB(255, 255, 235, 150);
			case 5:
				return ARGB(255, 225, 175, 255);
		}

		return ARGB(255, 255, 255, 255);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_ChkDontShow)
		{
			//the engine has already toggled the box: store its new state, never SetChecked here
			VPPChangelogState.SetDontShow(m_ChkDontShow.IsChecked());
			return false;
		}

		if (w == m_BtnCloseFooter)
		{
			HideSubMenu();
			return true;
		}

		//the base class handles the title-bar X through m_closeButton
		return super.OnClick(w, x, y, button);
	}

	//fixed-size window: the rows are laid out for its exact width
	override void ResizeWindow(bool expand)
	{
	}
};

/*
	One clickable link row: a ghost button with the external-link icon, a blue label and the URL underneath.
	Hover turns the label lighter and bold; a click opens http/https URLs in the browser (Windows only).
*/
class VPPChangelogLinkRow : ScriptedWidgetEventHandler
{
	protected ButtonWidget m_BtnLink;
	protected ImageWidget m_ImgLink;
	protected RichTextWidget m_TxtLabel;
	protected TextWidget m_TxtUrl;
	protected string m_Label;
	protected string m_Url;

	void VPPChangelogLinkRow(Widget row, string label, string url)
	{
		m_Label = label;
		m_Url = url;
		if (!row)
			return;

		m_BtnLink = ButtonWidget.Cast(row.FindAnyWidget("BtnLink"));
		m_ImgLink = ImageWidget.Cast(row.FindAnyWidget("ImgLink"));
		m_TxtLabel = RichTextWidget.Cast(row.FindAnyWidget("TxtLinkLabel"));
		m_TxtUrl = TextWidget.Cast(row.FindAnyWidget("TxtLinkUrl"));
		if (m_TxtUrl)
			m_TxtUrl.SetText(url);

		if (m_BtnLink)
			m_BtnLink.SetHandler(this);

		SetHover(false);
	}

	protected void SetHover(bool hover)
	{
		string rgba = VPPChangelogMarkup.RGBA_LINK;
		int iconColor = ARGB(255, 96, 158, 235);
		if (hover)
		{
			rgba = VPPChangelogMarkup.RGBA_LINK_HOVER;
			iconColor = ARGB(255, 150, 196, 255);
		}

		if (m_TxtLabel)
		{
			m_TxtLabel.SetText(VPPChangelogMarkup.Colorize(VPPChangelogMarkup.Localize(m_Label), rgba));
			m_TxtLabel.SetBold(hover);
		}

		if (m_ImgLink)
			m_ImgLink.SetColor(iconColor);
	}

	protected void OpenLink()
	{
		if (m_Url.IndexOf("https://") != 0 && m_Url.IndexOf("http://") != 0)
			return;

#ifdef PLATFORM_WINDOWS
		GetGame().OpenURL(m_Url);
		if (GetVPPUIManager())
			GetVPPUIManager().DisplayNotification("#VSTR_CHANGELOG_LINK_OPENED", "#VSTR_CHANGELOG_TITLE", 3.0);
#endif
	}

	//enter and leave return false so the button's own hover highlight is never suppressed
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (w != m_BtnLink)
			return false;

		SetHover(true);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (w != m_BtnLink)
			return false;

		SetHover(false);
		return false;
	}

	//click returns true so the menu never handles it a second time
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w != m_BtnLink)
			return false;

		OpenLink();
		return true;
	}
};
