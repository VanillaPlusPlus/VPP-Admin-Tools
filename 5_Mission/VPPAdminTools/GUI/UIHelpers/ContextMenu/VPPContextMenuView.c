/*
	Context menu popup view.

	Created once at workspace root (never inside a scroller) with a high sort so it
	draws above every HUD window. Renders a VPPContextItems page; it never builds
	pages or executes actions - input is forwarded to the owning controller.

	Sizing: the workspace root is unscaled, so every VPPContextMenuStyle metric and text size is multiplied by
	m_Scale (screen height / 1080, never below 1). The width fits the longest label + hint of the page (the
	confirm-again hint of a Confirm row included), from VIEW_WIDTH up to the screen width; a hint that grows later
	(server state) widens it again. The visible row count shrinks so the menu always fits the screen height.
*/
class VPPContextMenuView : ScriptedWidgetEventHandler
{
	protected VPPContextMenuController m_Owner;   //weak

	protected Widget      m_Root;
	protected Widget      m_Header;
	protected Widget      m_RowHost;
	protected Widget      m_ScrollTrack;
	protected Widget      m_ScrollThumb;
	protected Widget      m_Footer;
	protected ImageWidget m_HeaderIcon;
	protected TextWidget  m_HeaderText;
	protected TextWidget  m_FooterText;

	protected ref VPPContextItems m_Rows;
	protected ref array<Widget>   m_RowWidgets;
	protected ref array<float>    m_HintWidths;   //widest hint seen per row, in screen px

	protected int   m_Focus = -1;
	protected int   m_ConfirmIndex = -1;
	protected int   m_FirstVisible;
	protected bool  m_LookMode;
	protected int   m_PressedIndex = -1;
	protected int   m_PressedButton = -1;
	protected float m_Scale = 1.0;
	protected float m_ViewW;
	protected float m_RowWidth;
	protected float m_Height;
	protected int   m_MaxVisible = VPPContextMenuStyle.MAX_VISIBLE_ITEMS;
	protected float m_NeedW;        //widest row content (label + hint + chevron + pads), screen px
	protected bool  m_WidthDirty;   //a row needs more than m_RowWidth: FlushWidth re-lays out

	void VPPContextMenuView(VPPContextMenuController owner)
	{
		m_Owner = owner;
		m_RowWidgets = new array<Widget>;
		m_HintWidths = new array<float>;
		m_ViewW = VPPContextMenuStyle.VIEW_WIDTH;
		m_RowWidth = VPPContextMenuStyle.VIEW_WIDTH - (2 * VPPContextMenuStyle.PAD);

		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.VPPContextMenu, null);
		if (!m_Root)
			return;

		m_Root.SetSort(VPPContextMenuStyle.VIEW_SORT, true);

		m_Header      = m_Root.FindAnyWidget("CtxHeader");
		m_HeaderIcon  = ImageWidget.Cast(m_Root.FindAnyWidget("CtxHeaderIcon"));
		m_HeaderText  = TextWidget.Cast(m_Root.FindAnyWidget("CtxHeaderText"));
		m_RowHost     = m_Root.FindAnyWidget("CtxRowHost");
		m_ScrollTrack = m_Root.FindAnyWidget("CtxScrollTrack");
		m_ScrollThumb = m_Root.FindAnyWidget("CtxScrollThumb");
		m_Footer      = m_Root.FindAnyWidget("CtxFooter");
		m_FooterText  = TextWidget.Cast(m_Root.FindAnyWidget("CtxFooterText"));

		m_Root.SetHandler(this);
		m_Root.Show(false);
	}

	void ~VPPContextMenuView()
	{
		ClearRowWidgets();

		if (m_Root)
			m_Root.Unlink();
	}

	protected void ClearRowWidgets()
	{
		if (!m_RowWidgets)
			return;

		for (int i = 0; i < m_RowWidgets.Count(); i++)
		{
			Widget w = m_RowWidgets[i];
			if (w)
				w.Unlink();
		}
		m_RowWidgets.Clear();
		m_HintWidths.Clear();
	}

	//metric in screen px
	protected float Px(float metric)
	{
		return metric * m_Scale;
	}

	protected int TextPx(int textSize)
	{
		float scaled = Math.Round(textSize * m_Scale);
		int result = scaled;
		if (result < 1)
			result = 1;

		return result;
	}

	protected void UpdateScale()
	{
		int sw, sh;
		GetScreenSize(sw, sh);
		float scale = sh / VPPContextMenuStyle.REF_SCREEN_H;
		if (scale < VPPContextMenuStyle.MIN_SCALE)
			scale = VPPContextMenuStyle.MIN_SCALE;

		m_Scale = scale;
	}

	//text width in screen px; widget and text size must already be set. GetTextSize can read 0 before the widget
	//was ever laid out, then the width is estimated from the character count
	protected float MeasureText(TextWidget w, string text, int textSize)
	{
		if (!w || text == "")
			return 0;

		w.SetText(text);
		w.Update();
		int tw, th;
		w.GetTextSize(tw, th);
		if (tw > 0)
			return tw;

		int chars = text.LengthUtf8();
		float textPx = TextPx(textSize);
		return chars * textPx * VPPContextMenuStyle.TEXT_EM_ESTIMATE;
	}

	//size and place the fixed parts (header, footer texts) for the current scale
	protected void ApplyChromeScale()
	{
		int headerSize = TextPx(VPPContextMenuStyle.HEADER_TEXT_SIZE);
		int footerSize = TextPx(VPPContextMenuStyle.FOOTER_TEXT_SIZE);
		float iconSize = Px(VPPContextMenuStyle.ICON_SIZE);
		float iconX = Px(VPPContextMenuStyle.HEADER_ICON_X);
		if (m_HeaderText)
			m_HeaderText.SetTextExactSize(headerSize);

		if (m_FooterText)
			m_FooterText.SetTextExactSize(footerSize);

		if (m_HeaderIcon)
		{
			m_HeaderIcon.SetPos(iconX, 0);
			m_HeaderIcon.SetSize(iconSize, iconSize);
		}
	}

	protected void ApplyRowScale(Widget row, bool isSeparator)
	{
		if (!row)
			return;

		if (isSeparator)
			return;

		int labelSize = TextPx(VPPContextMenuStyle.LABEL_TEXT_SIZE);
		int hintSize = TextPx(VPPContextMenuStyle.HINT_TEXT_SIZE);
		float iconSize = Px(VPPContextMenuStyle.ICON_SIZE);
		float iconX = Px(VPPContextMenuStyle.ICON_X);
		float chevronSize = Px(VPPContextMenuStyle.CHEVRON_SIZE);

		TextWidget label = TextWidget.Cast(row.FindAnyWidget("CtxRowLabel"));
		TextWidget hint = TextWidget.Cast(row.FindAnyWidget("CtxRowHint"));
		Widget icon = row.FindAnyWidget("CtxRowIcon");
		Widget chevron = row.FindAnyWidget("CtxRowChevron");
		if (label)
			label.SetTextExactSize(labelSize);

		if (hint)
			hint.SetTextExactSize(hintSize);

		if (icon)
		{
			icon.SetPos(iconX, 0);
			icon.SetSize(iconSize, iconSize);
		}

		if (chevron)
			chevron.SetSize(chevronSize, chevronSize);
	}

	void Render(string title, string titleIcon, VPPContextItems rows, bool lookMode)
	{
		m_Rows = rows;
		m_LookMode = lookMode;

		ClearRowWidgets();

		m_Focus = -1;
		m_ConfirmIndex = -1;
		m_FirstVisible = 0;
		m_PressedIndex = -1;
		m_PressedButton = -1;

		if (!m_Root)
			return;

		UpdateScale();
		ApplyChromeScale();
		m_NeedW = 0;

		string headerText = Widget.TranslateString(title);
		if (m_HeaderText)
		{
			float headerPx = MeasureText(m_HeaderText, headerText, VPPContextMenuStyle.HEADER_TEXT_SIZE);
			float headerNeed = Px(VPPContextMenuStyle.HEADER_TEXT_X) + headerPx + Px(VPPContextMenuStyle.TEXT_PAD) + Px(VPPContextMenuStyle.RIGHT_PAD);
			if (headerNeed > m_NeedW)
				m_NeedW = headerNeed;
		}

		if (m_HeaderIcon)
		{
			if (titleIcon != "")
			{
				m_HeaderIcon.LoadImageFile(0, titleIcon);
				m_HeaderIcon.SetImage(0);
				m_HeaderIcon.Show(true);
			}
			else
			{
				m_HeaderIcon.Show(false);
			}
		}

		if (m_FooterText && lookMode)
		{
			//the layout's footer text (TextWidget has no GetText)
			string footerText = Widget.TranslateString("#VSTR_CTX_HINT_LOOK");
			float footerPx = MeasureText(m_FooterText, footerText, VPPContextMenuStyle.FOOTER_TEXT_SIZE);
			float footerNeed = footerPx + (2 * Px(VPPContextMenuStyle.TEXT_PAD));
			if (footerNeed > m_NeedW)
				m_NeedW = footerNeed;
		}

		int count = 0;
		if (m_Rows)
			count = m_Rows.Count();

		for (int i = 0; i < count; i++)
		{
			VPPContextItem item = m_Rows.Get(i);
			string layoutPath = VPPATUIConstants.VPPContextMenuRow;
			bool isSeparator = false;
			if (item && item.Kind == EVPPContextKind.SEPARATOR)
			{
				layoutPath = VPPATUIConstants.VPPContextMenuSeparator;
				isSeparator = true;
			}

			Widget w = GetGame().GetWorkspace().CreateWidgets(layoutPath, m_RowHost);
			if (w)
			{
				w.SetUserID(i);
				w.SetHandler(this);
				ApplyRowScale(w, isSeparator);
			}
			//keep the lists index-aligned with m_Rows even if a prefab failed to load
			m_RowWidgets.Insert(w);
			m_HintWidths.Insert(0);
			MeasureRow(i);
		}

		if (m_Footer)
			m_Footer.Show(lookMode);

		FitToScreen();
		LayoutRows();

		for (int j = 0; j < count; j++)
		{
			ApplyRowVisual(j);
		}

		FlushWidth();
	}

	protected bool IsSeparatorIndex(int i)
	{
		if (!m_Rows || i < 0 || i >= m_Rows.Count())
			return false;

		VPPContextItem item = m_Rows.Get(i);
		if (item && item.Kind == EVPPContextKind.SEPARATOR)
			return true;

		return false;
	}

	protected string LabelTextOf(VPPContextItem item)
	{
		if (item.Kind == EVPPContextKind.BACK)
			return Widget.TranslateString("#VSTR_CTX_BACK");

		return Widget.TranslateString(item.Label);
	}

	//row content width for a label and hint of these widths
	protected float RowNeed(VPPContextItem item, float labelPx, float hintPx)
	{
		float need = Px(VPPContextMenuStyle.LABEL_X) + labelPx + Px(VPPContextMenuStyle.TEXT_PAD) + Px(VPPContextMenuStyle.RIGHT_PAD);
		if (hintPx > 0)
			need = need + Px(VPPContextMenuStyle.LABEL_GAP) + hintPx;

		if (item.Kind == EVPPContextKind.SUBMENU)
			need = need + Px(VPPContextMenuStyle.CHEVRON_W);

		return need;
	}

	//widest label + hint this row can show: its hint now and, for a Confirm row, the confirm-again hint
	protected void MeasureRow(int i)
	{
		if (!m_Rows || i < 0 || i >= m_Rows.Count() || i >= m_RowWidgets.Count())
			return;

		VPPContextItem item = m_Rows.Get(i);
		Widget row = m_RowWidgets[i];
		if (!item || !row || item.Kind == EVPPContextKind.SEPARATOR)
			return;

		TextWidget label = TextWidget.Cast(row.FindAnyWidget("CtxRowLabel"));
		TextWidget hint = TextWidget.Cast(row.FindAnyWidget("CtxRowHint"));
		float rowH = Px(VPPContextMenuStyle.ROW_H);
		float wideText = Px(VPPContextMenuStyle.VIEW_WIDTH) * 4;
		if (label)
			label.SetSize(wideText, rowH);

		if (hint)
			hint.SetSize(wideText, rowH);

		string labelText = LabelTextOf(item);
		float labelPx = MeasureText(label, labelText, VPPContextMenuStyle.LABEL_TEXT_SIZE);

		float hintPx = 0;
		if (item.Hint != "")
		{
			string hintText = Widget.TranslateString(item.Hint);
			hintPx = MeasureText(hint, hintText, VPPContextMenuStyle.HINT_TEXT_SIZE);
		}

		if (item.Confirm)
		{
			string confirmText = Widget.TranslateString("#VSTR_CTX_CONFIRM_AGAIN");
			float confirmPx = MeasureText(hint, confirmText, VPPContextMenuStyle.HINT_TEXT_SIZE);
			if (confirmPx > hintPx)
				hintPx = confirmPx;
		}

		if (hintPx > m_HintWidths[i])
			m_HintWidths.Set(i, hintPx);

		float storedHint = m_HintWidths[i];
		float need = RowNeed(item, labelPx, storedHint);
		if (need > m_NeedW)
			m_NeedW = need;
	}

	//width from m_NeedW (min VIEW_WIDTH, max the screen) and the visible row count from the screen height
	protected void FitToScreen()
	{
		int sw, sh;
		GetScreenSize(sw, sh);
		float margin = Px(VPPContextMenuStyle.SCREEN_MARGIN);
		float pad = Px(VPPContextMenuStyle.PAD);

		//height: header, pads, optional footer, then as many rows as fit (separators are shorter, so this is safe)
		float fixedH = pad + Px(VPPContextMenuStyle.HEADER_H) + pad + pad;
		if (m_LookMode)
			fixedH = fixedH + Px(VPPContextMenuStyle.FOOTER_H) + pad;

		float rowStep = Px(VPPContextMenuStyle.ROW_H) + Px(VPPContextMenuStyle.ROW_GAP);
		float budget = sh - (2 * margin) - fixedH + Px(VPPContextMenuStyle.ROW_GAP);
		float fitF = Math.Floor(budget / rowStep);
		int fit = fitF;
		if (fit > VPPContextMenuStyle.MAX_VISIBLE_ITEMS)
			fit = VPPContextMenuStyle.MAX_VISIBLE_ITEMS;

		if (fit < 1)
			fit = 1;

		m_MaxVisible = fit;

		int total = m_RowWidgets.Count();
		float scrollReserve = 0;
		if (total > m_MaxVisible)
			scrollReserve = Px(VPPContextMenuStyle.SCROLL_W) + Px(VPPContextMenuStyle.SCROLL_GAP);

		float viewW = m_NeedW + (2 * pad) + scrollReserve;
		float minW = Px(VPPContextMenuStyle.VIEW_WIDTH);
		float maxW = sw - (2 * margin);
		if (viewW < minW)
			viewW = minW;

		if (viewW > maxW)
			viewW = maxW;

		m_ViewW = Math.Ceil(viewW);
		m_RowWidth = m_ViewW - (2 * pad) - scrollReserve;
		m_WidthDirty = false;
	}

	//a row asked for more width since the last layout: widen, re-lay out and redraw every row (never recursive:
	//ApplyRowVisual only raises the flag)
	protected void FlushWidth()
	{
		if (!m_WidthDirty)
			return;

		FitToScreen();
		LayoutRows();
		int count = m_RowWidgets.Count();
		for (int i = 0; i < count; i++)
		{
			ApplyRowVisual(i);
		}

		m_WidthDirty = false;
		Reclamp();
	}

	protected void ApplyRowVisual(int i)
	{
		if (!m_Rows || i < 0 || i >= m_Rows.Count() || i >= m_RowWidgets.Count())
			return;

		VPPContextItem item = m_Rows.Get(i);
		Widget row = m_RowWidgets[i];
		if (!item || !row || item.Kind == EVPPContextKind.SEPARATOR)
			return;

		ImageWidget icon    = ImageWidget.Cast(row.FindAnyWidget("CtxRowIcon"));
		Widget labelClip    = row.FindAnyWidget("CtxRowLabelClip");
		TextWidget label    = TextWidget.Cast(row.FindAnyWidget("CtxRowLabel"));
		Widget hintClip     = row.FindAnyWidget("CtxRowHintClip");
		TextWidget hint     = TextWidget.Cast(row.FindAnyWidget("CtxRowHint"));
		ImageWidget chevron = ImageWidget.Cast(row.FindAnyWidget("CtxRowChevron"));

		bool armed = (i == m_ConfirmIndex);
		bool focused = (i == m_Focus);

		//background
		if (armed)
			row.SetColor(VPPContextMenuStyle.RowConfirm());
		else if (focused)
			row.SetColor(VPPContextMenuStyle.RowFocus());
		else
			row.SetColor(VPPContextMenuStyle.RowBg());

		//label text is never replaced while armed: the admin always sees which action is armed
		string labelText = LabelTextOf(item);

		int fgColor;
		if (!item.Enabled || item.Kind == EVPPContextKind.INFO)
			fgColor = VPPContextMenuStyle.TextMuted();
		else if (item.Danger && !focused && !armed)
			fgColor = VPPContextMenuStyle.Danger();
		else
			fgColor = VPPContextMenuStyle.TextPrimary();

		float rowH = Px(VPPContextMenuStyle.ROW_H);
		float wideText = Px(VPPContextMenuStyle.VIEW_WIDTH) * 4;
		if (label)
			label.SetSize(wideText, rowH);

		float labelPx = MeasureText(label, labelText, VPPContextMenuStyle.LABEL_TEXT_SIZE);
		if (label)
			label.SetColor(fgColor);

		//icon
		if (icon)
		{
			string iconPath = item.IconPath;
			int iconColor = fgColor;
			float iconRotation = 0;
			if (item.Kind == EVPPContextKind.TOGGLE)
			{
				if (item.Checked)
					iconPath = VPPContextConstants.ICON_CHECK_ON;
				else
					iconPath = VPPContextConstants.ICON_CHECK_OFF;
				iconColor = VPPContextMenuStyle.TextPrimary();
			}
			else if (item.Kind == EVPPContextKind.BACK)
			{
				iconPath = VPPContextConstants.ICON_CHEVRON;
				iconRotation = VPPContextMenuStyle.CHEVRON_BACK_ROTATION;
			}

			if (iconPath != "")
			{
				icon.LoadImageFile(0, iconPath);
				icon.SetImage(0);
				icon.SetRotation(0, 0, iconRotation, true);
				icon.SetColor(iconColor);
				icon.Show(true);
			}
			else
			{
				icon.Show(false);
			}
		}

		//hint (shown in full: the width was fitted to it)
		string hintText = "";
		int hintColor;
		if (armed)
		{
			hintText = Widget.TranslateString("#VSTR_CTX_CONFIRM_AGAIN");
			hintColor = VPPContextMenuStyle.TextPrimary();
		}
		else
		{
			if (item.Hint != "")
				hintText = Widget.TranslateString(item.Hint);
			hintColor = VPPContextMenuStyle.TextSecondary();
		}

		if (hint)
		{
			//measure at full width, then shrink to the hint column
			hint.SetSize(wideText, rowH);
			float hintNow = MeasureText(hint, hintText, VPPContextMenuStyle.HINT_TEXT_SIZE);
			if (hintNow > m_HintWidths[i])
				m_HintWidths.Set(i, hintNow);

			hint.SetText(hintText);
			hint.SetColor(hintColor);
		}

		//a hint that grew (server state) or a longer label: widen on the next FlushWidth
		float storedHint = m_HintWidths[i];
		float need = RowNeed(item, labelPx, storedHint);
		if (need > m_NeedW)
			m_NeedW = need;

		if (need > m_RowWidth + 0.5)
			m_WidthDirty = true;

		//geometry
		bool isSubmenu = (item.Kind == EVPPContextKind.SUBMENU);
		float chevronReserve = 0;
		if (isSubmenu)
			chevronReserve = Px(VPPContextMenuStyle.CHEVRON_W);

		float rightPad = Px(VPPContextMenuStyle.RIGHT_PAD);
		float hintW = 0;
		if (hintText != "")
			hintW = storedHint + Px(VPPContextMenuStyle.TEXT_PAD);

		if (hintClip)
		{
			float hintX = m_RowWidth - rightPad - chevronReserve - hintW;
			hintClip.SetPos(hintX, 0);
			hintClip.SetSize(hintW, rowH);
			hintClip.Show(hintW > 0);
		}

		if (hint)
		{
			hint.SetPos(0, 0);
			hint.SetSize(hintW, rowH);
		}

		if (chevron)
		{
			//the prefab chevron is valign center_ref, so y = 0 is vertically centred
			float chevronX = m_RowWidth - rightPad - Px(VPPContextMenuStyle.CHEVRON_SIZE);
			chevron.SetPos(chevronX, 0);
			chevron.SetRotation(0, 0, VPPContextMenuStyle.CHEVRON_SUBMENU_ROTATION, true);
			chevron.Show(isSubmenu);
		}

		float labelX = Px(VPPContextMenuStyle.LABEL_X);
		float labelW = m_RowWidth - labelX - rightPad - chevronReserve;
		if (hintW > 0)
			labelW = labelW - hintW - Px(VPPContextMenuStyle.LABEL_GAP);

		float minLabelW = Px(20);
		if (labelW < minLabelW)
			labelW = minLabelW;

		if (labelClip)
		{
			labelClip.SetPos(labelX, 0);
			labelClip.SetSize(labelW, rowH);
		}

		if (label)
		{
			float labelTextW = labelW;
			float labelFull = labelPx + Px(VPPContextMenuStyle.TEXT_PAD);
			if (labelFull > labelTextW)
				labelTextW = labelFull;

			label.SetPos(0, 0);
			label.SetSize(labelTextW, rowH);
		}
	}

	protected void LayoutRows()
	{
		if (!m_Root)
			return;

		int total = m_RowWidgets.Count();
		int maxFirst = total - m_MaxVisible;
		if (maxFirst < 0)
			maxFirst = 0;
		if (m_FirstVisible > maxFirst)
			m_FirstVisible = maxFirst;
		if (m_FirstVisible < 0)
			m_FirstVisible = 0;

		float pad = Px(VPPContextMenuStyle.PAD);
		float headerH = Px(VPPContextMenuStyle.HEADER_H);
		float rowGap = Px(VPPContextMenuStyle.ROW_GAP);
		float sepLineX = Px(VPPContextMenuStyle.SEP_LINE_X);
		int windowEnd = m_FirstVisible + m_MaxVisible;
		float y = 0;
		int visibleCount = 0;
		for (int i = 0; i < total; i++)
		{
			Widget w = m_RowWidgets[i];
			if (!w)
				continue;

			if (i < m_FirstVisible || i >= windowEnd)
			{
				w.Show(false);
				continue;
			}

			float h = Px(VPPContextMenuStyle.ROW_H);
			bool isSeparator = IsSeparatorIndex(i);
			if (isSeparator)
				h = Px(VPPContextMenuStyle.SEP_H);

			if (visibleCount > 0)
				y = y + rowGap;

			w.SetPos(0, y);
			w.SetSize(m_RowWidth, h);
			w.Show(true);
			if (isSeparator)
			{
				Widget sepLine = w.FindAnyWidget("CtxSepLine");
				if (sepLine)
				{
					float sepW = m_RowWidth - (2 * sepLineX);
					float sepY = Math.Floor(h * 0.5);
					sepLine.SetPos(sepLineX, sepY);
					sepLine.SetSize(sepW, 1);
				}
			}

			y = y + h;
			visibleCount++;
		}

		float contentH = y;
		float innerW = m_ViewW - (2 * pad);
		float hostY = pad + headerH + pad;

		if (m_Header)
		{
			m_Header.SetPos(pad, pad);
			m_Header.SetSize(innerW, headerH);
		}

		if (m_HeaderText)
		{
			float headerTextX = Px(VPPContextMenuStyle.HEADER_TEXT_X);
			float headerTextW = innerW - headerTextX - Px(VPPContextMenuStyle.RIGHT_PAD);
			m_HeaderText.SetPos(headerTextX, 0);
			m_HeaderText.SetSize(headerTextW, headerH);
		}

		if (m_RowHost)
		{
			m_RowHost.SetPos(pad, hostY);
			m_RowHost.SetSize(innerW, contentH);
		}

		bool overflow = (total > m_MaxVisible);
		if (m_ScrollTrack)
		{
			m_ScrollTrack.Show(overflow);
			if (overflow)
			{
				float scrollW = Px(VPPContextMenuStyle.SCROLL_W);
				float trackH = contentH;
				float trackX = m_ViewW - pad - scrollW;
				m_ScrollTrack.SetPos(trackX, hostY);
				m_ScrollTrack.SetSize(scrollW, trackH);

				float totalF = total;
				float visibleF = visibleCount;
				float firstF = m_FirstVisible;
				float thumbH = trackH * visibleF / totalF;
				float minThumb = Px(12);
				if (thumbH < minThumb)
					thumbH = minThumb;
				float thumbY = trackH * firstF / totalF;
				if (thumbY + thumbH > trackH)
					thumbY = trackH - thumbH;
				if (thumbY < 0)
					thumbY = 0;

				if (m_ScrollThumb)
				{
					m_ScrollThumb.SetPos(0, thumbY);
					m_ScrollThumb.SetSize(scrollW, thumbH);
				}
			}
		}

		m_Height = hostY + contentH + pad;
		if (m_LookMode)
		{
			float footerH = Px(VPPContextMenuStyle.FOOTER_H);
			if (m_Footer)
			{
				m_Footer.SetPos(pad, hostY + contentH + pad);
				m_Footer.SetSize(innerW, footerH);
			}

			if (m_FooterText)
			{
				m_FooterText.SetPos(0, 0);
				m_FooterText.SetSize(innerW, footerH);
			}
			m_Height = m_Height + footerH + pad;
		}

		m_Root.SetSize(m_ViewW, m_Height);
	}

	void RefreshRow(int index)
	{
		MeasureRow(index);
		ApplyRowVisual(index);
		FlushWidth();
	}

	void SetFocusIndex(int index)
	{
		int prev = m_Focus;
		m_Focus = index;

		if (index >= 0)
		{
			int first = m_FirstVisible;
			if (index < first)
				first = index;
			else if (index >= first + m_MaxVisible)
				first = index - m_MaxVisible + 1;

			if (first != m_FirstVisible)
			{
				m_FirstVisible = first;
				LayoutRows();
			}
		}

		ApplyRowVisual(prev);
		ApplyRowVisual(index);
		FlushWidth();
	}

	int GetFocusIndex()
	{
		return m_Focus;
	}

	void SetConfirmArmed(int index, bool armed)
	{
		int old = m_ConfirmIndex;
		if (armed)
			m_ConfirmIndex = index;
		else
			m_ConfirmIndex = -1;

		ApplyRowVisual(old);
		ApplyRowVisual(m_ConfirmIndex);
		if (!armed)
			ApplyRowVisual(index);

		FlushWidth();
	}

	protected void ClampAndSetPos(float x, float y)
	{
		if (!m_Root)
			return;

		int sw, sh;
		GetScreenSize(sw, sh);

		float maxX = sw - m_ViewW;
		float maxY = sh - m_Height;
		if (x > maxX)
			x = maxX;
		if (x < 0)
			x = 0;
		if (y > maxY)
			y = maxY;
		if (y < 0)
			y = 0;

		m_Root.SetPos(x, y);
	}

	void PlaceAtCrosshair()
	{
		int sw, sh;
		GetScreenSize(sw, sh);

		float offsetX = Px(VPPContextMenuStyle.LOOK_OFFSET_X);
		float offsetY = Px(VPPContextMenuStyle.LOOK_OFFSET_Y);
		float headTop = Px(VPPContextMenuStyle.PAD) + Px(VPPContextMenuStyle.HEADER_H);
		float x = (sw / 2.0) + offsetX;
		float y = (sh / 2.0) + offsetY - headTop;
		ClampAndSetPos(x, y);
	}

	void PlaceAtCursor(int x, int y)
	{
		int sw, sh;
		GetScreenSize(sw, sh);

		float offset = Px(VPPContextMenuStyle.CURSOR_OFFSET);
		float px;
		if (x + offset + m_ViewW > sw)
			px = x - m_ViewW - offset;
		else
			px = x + offset;

		float py;
		if (y + offset + m_Height > sh)
			py = y - m_Height - offset;
		else
			py = y + offset;

		ClampAndSetPos(px, py);
	}

	void Reclamp()
	{
		if (!m_Root)
			return;

		float x, y;
		m_Root.GetPos(x, y);
		ClampAndSetPos(x, y);
	}

	void Show(bool state)
	{
		if (m_Root)
			m_Root.Show(state);
	}

	bool IsShown()
	{
		if (m_Root)
			return m_Root.IsVisible();

		return false;
	}

	Widget GetRoot()
	{
		return m_Root;
	}

	int GetRowCount()
	{
		if (m_Rows)
			return m_Rows.Count();

		return 0;
	}

	bool IsWidgetInside(Widget w)
	{
		if (!m_Root)
			return false;

		Widget cur = w;
		while (cur)
		{
			if (cur == m_Root)
				return true;
			cur = cur.GetParent();
		}
		return false;
	}

	int GetRowIndexFromWidget(Widget w)
	{
		Widget cur = w;
		while (cur)
		{
			if (cur == m_Root)
				return -1;

			string name = cur.GetName();
			if (name == "CtxRow" || name == "CtxSep")
				return cur.GetUserID();

			cur = cur.GetParent();
		}
		return -1;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		int idx = GetRowIndexFromWidget(w);
		if (idx >= 0)
		{
			m_PressedIndex = idx;
			m_PressedButton = button;
		}
		return IsWidgetInside(w);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		int idx = GetRowIndexFromWidget(w);
		bool samePair = (idx >= 0 && idx == m_PressedIndex && button == m_PressedButton);

		m_PressedIndex = -1;
		m_PressedButton = -1;

		if (samePair && m_Owner)
			m_Owner.OnViewRowClicked(idx, button);

		return IsWidgetInside(w);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		int idx = GetRowIndexFromWidget(w);
		if (idx >= 0 && m_Owner)
			m_Owner.OnViewRowHovered(idx);

		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!IsWidgetInside(w))
			return false;

		int dir;
		if (wheel < 0)
			dir = 1;
		else
			dir = -1;

		if (m_Owner)
			m_Owner.OnViewWheel(dir);

		return true;
	}
};
