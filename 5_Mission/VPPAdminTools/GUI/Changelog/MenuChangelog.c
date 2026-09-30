/*
	Changelog window: a fixed 760x660 AdminHudSubMenu.
	Opened by VPPAdminHud.OpenChangelog, both on toolbar open and from the Stats HUD button.
	Rows come from VPPChangelogContent.Build and are measured again every frame for MEASURE_FRAMES frames after a build or show.
*/
class MenuChangelog extends AdminHudSubMenu
{
	protected const float WINDOW_W = 760.0;
	protected const float WINDOW_H = 660.0;
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
	protected int m_MeasureFrames;

	void MenuChangelog()
	{
		m_LinkRows = new array<ref VPPChangelogLinkRow>;
		m_TextRows = new array<Widget>;
		m_TextBodies = new array<RichTextWidget>;
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

		CenterWindow();
		Rebuild();
	}

	//Panel_Content is full screen, so these pixels are screen pixels
	protected void CenterWindow()
	{
		if (!M_SUB_WIDGET)
			return;

		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float px = (sw - WINDOW_W) * 0.5;
		float py = (sh - WINDOW_H) * 0.5;
		if (px < 0)
			px = 0;

		if (py < 0)
			py = 0;

		M_SUB_WIDGET.SetPos(px, py);
	}

	void Rebuild()
	{
		ClearRows();
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

	//text sizes can read 0 before the first layout pass, so MeasureRows calls this again every re-measure frame
	protected void PlaceLatestChip()
	{
		if (!m_LatestChip || !m_LatestVersion)
			return;

		int versionW;
		int versionH;
		m_LatestVersion.Update();
		m_LatestVersion.GetTextSize(versionW, versionH);
		if (versionW <= 0)
			versionW = 48;

		int labelW = 44;
		int labelH;
		if (m_LatestLabel)
		{
			m_LatestLabel.Update();
			m_LatestLabel.GetTextSize(labelW, labelH);
			if (labelW <= 0)
				labelW = 44;
		}

		m_LatestChip.SetSize(labelW + 16, 20);
		m_LatestChip.SetPos(VERSION_X + versionW + LATEST_GAP, 0);
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
		body.SetPos(bodyX, BODY_Y);
		body.SetSize(bodyW, LINE_PITCH);
		body.SetText(VPPChangelogMarkup.ToRichText(text));
		m_TextRows.Insert(row);
		m_TextBodies.Insert(body);
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

			float rowW;
			float oldH;
			row.GetSize(rowW, oldH);
			if (Math.AbsFloat(oldH - rowH) < 0.5)
				continue;

			row.SetSize(rowW, rowH);
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
		if (m_MeasureFrames <= 0 || !IsSubMenuVisible())
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
