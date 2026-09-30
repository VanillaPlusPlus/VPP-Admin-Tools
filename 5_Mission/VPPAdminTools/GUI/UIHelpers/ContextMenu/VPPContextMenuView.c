/*
	Context menu popup view.

	Created once at workspace root (never inside a scroller) with a high sort so it
	draws above every HUD window. Renders a VPPContextItems page; it never builds
	pages or executes actions - input is forwarded to the owning controller.
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

	protected int   m_Focus = -1;
	protected int   m_ConfirmIndex = -1;
	protected int   m_FirstVisible;
	protected bool  m_LookMode;
	protected int   m_PressedIndex = -1;
	protected int   m_PressedButton = -1;
	protected float m_RowWidth;
	protected float m_Height;

	void VPPContextMenuView(VPPContextMenuController owner)
	{
		m_Owner = owner;
		m_RowWidgets = new array<Widget>;
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

		if (m_HeaderText)
			m_HeaderText.SetText(Widget.TranslateString(title));

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

		int count = 0;
		if (m_Rows)
			count = m_Rows.Count();

		m_RowWidth = VPPContextMenuStyle.VIEW_WIDTH - (2 * VPPContextMenuStyle.PAD);
		if (count > VPPContextMenuStyle.MAX_VISIBLE_ITEMS)
			m_RowWidth = m_RowWidth - (VPPContextMenuStyle.SCROLL_W + 3);

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
				if (isSeparator)
				{
					Widget sepLine = w.FindAnyWidget("CtxSepLine");
					if (sepLine)
						sepLine.SetSize(m_RowWidth - 16, 1);
				}
			}
			//keep the list index-aligned with m_Rows even if a prefab failed to load
			m_RowWidgets.Insert(w);
		}

		if (m_Footer)
			m_Footer.Show(lookMode);

		LayoutRows();

		for (int j = 0; j < count; j++)
		{
			ApplyRowVisual(j);
		}
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
		string labelText;
		if (item.Kind == EVPPContextKind.BACK)
			labelText = Widget.TranslateString("#VSTR_CTX_BACK");
		else
			labelText = Widget.TranslateString(item.Label);

		int fgColor;
		if (!item.Enabled || item.Kind == EVPPContextKind.INFO)
			fgColor = VPPContextMenuStyle.TextMuted();
		else if (item.Danger && !focused && !armed)
			fgColor = VPPContextMenuStyle.Danger();
		else
			fgColor = VPPContextMenuStyle.TextPrimary();

		if (label)
		{
			label.SetText(labelText);
			label.SetColor(fgColor);
		}

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

		//hint
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

		if (hintText.LengthUtf8() > VPPContextMenuStyle.HINT_MAX_CHARS)
		{
			string shortHint = hintText.SubstringUtf8(0, VPPContextMenuStyle.HINT_MAX_CHARS - 2);
			hintText = shortHint + "..";
		}

		if (hint)
		{
			hint.SetText(hintText);
			hint.SetColor(hintColor);
		}

		//geometry
		bool isSubmenu = (item.Kind == EVPPContextKind.SUBMENU);
		float chevronReserve = 0;
		if (isSubmenu)
			chevronReserve = VPPContextMenuStyle.CHEVRON_W;

		float hintW = 0;
		if (hintText != "")
			hintW = VPPContextMenuStyle.HINT_W;

		if (hintClip)
		{
			float hintX = m_RowWidth - VPPContextMenuStyle.RIGHT_PAD - chevronReserve - hintW;
			hintClip.SetPos(hintX, 0);
			hintClip.SetSize(hintW, VPPContextMenuStyle.ROW_H);
			hintClip.Show(hintW > 0);
		}

		if (hint)
		{
			hint.SetPos(0, 0);
			hint.SetSize(hintW, VPPContextMenuStyle.ROW_H);
		}

		if (chevron)
		{
			//the prefab chevron is valign center_ref, so y = 0 is vertically centred
			float chevronX = m_RowWidth - VPPContextMenuStyle.RIGHT_PAD - 12;
			chevron.SetPos(chevronX, 0);
			chevron.SetRotation(0, 0, VPPContextMenuStyle.CHEVRON_SUBMENU_ROTATION, true);
			chevron.Show(isSubmenu);
		}

		float labelW = m_RowWidth - VPPContextMenuStyle.LABEL_X - VPPContextMenuStyle.RIGHT_PAD - chevronReserve - hintW - VPPContextMenuStyle.LABEL_GAP;
		if (labelW < 20)
			labelW = 20;

		if (labelClip)
		{
			labelClip.SetPos(VPPContextMenuStyle.LABEL_X, 0);
			labelClip.SetSize(labelW, VPPContextMenuStyle.ROW_H);
		}

		if (label)
		{
			label.SetPos(0, 0);
			label.SetSize(VPPContextMenuStyle.LABEL_TEXT_W, VPPContextMenuStyle.ROW_H);
		}
	}

	protected void LayoutRows()
	{
		if (!m_Root)
			return;

		int total = m_RowWidgets.Count();
		int maxFirst = total - VPPContextMenuStyle.MAX_VISIBLE_ITEMS;
		if (maxFirst < 0)
			maxFirst = 0;
		if (m_FirstVisible > maxFirst)
			m_FirstVisible = maxFirst;
		if (m_FirstVisible < 0)
			m_FirstVisible = 0;

		int windowEnd = m_FirstVisible + VPPContextMenuStyle.MAX_VISIBLE_ITEMS;
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

			float h = VPPContextMenuStyle.ROW_H;
			if (IsSeparatorIndex(i))
				h = VPPContextMenuStyle.SEP_H;

			if (visibleCount > 0)
				y = y + VPPContextMenuStyle.ROW_GAP;

			w.SetPos(0, y);
			w.SetSize(m_RowWidth, h);
			w.Show(true);
			y = y + h;
			visibleCount++;
		}

		float contentH = y;
		float innerW = VPPContextMenuStyle.VIEW_WIDTH - (2 * VPPContextMenuStyle.PAD);
		float hostY = VPPContextMenuStyle.PAD + VPPContextMenuStyle.HEADER_H + VPPContextMenuStyle.PAD;

		if (m_Header)
		{
			m_Header.SetPos(VPPContextMenuStyle.PAD, VPPContextMenuStyle.PAD);
			m_Header.SetSize(innerW, VPPContextMenuStyle.HEADER_H);
		}

		if (m_RowHost)
		{
			m_RowHost.SetPos(VPPContextMenuStyle.PAD, hostY);
			m_RowHost.SetSize(innerW, contentH);
		}

		bool overflow = (total > VPPContextMenuStyle.MAX_VISIBLE_ITEMS);
		if (m_ScrollTrack)
		{
			m_ScrollTrack.Show(overflow);
			if (overflow)
			{
				float trackH = contentH;
				float trackX = VPPContextMenuStyle.VIEW_WIDTH - VPPContextMenuStyle.PAD - VPPContextMenuStyle.SCROLL_W;
				m_ScrollTrack.SetPos(trackX, hostY);
				m_ScrollTrack.SetSize(VPPContextMenuStyle.SCROLL_W, trackH);

				float totalF = total;
				float visibleF = visibleCount;
				float firstF = m_FirstVisible;
				float thumbH = trackH * visibleF / totalF;
				if (thumbH < 12)
					thumbH = 12;
				float thumbY = trackH * firstF / totalF;
				if (thumbY + thumbH > trackH)
					thumbY = trackH - thumbH;
				if (thumbY < 0)
					thumbY = 0;

				if (m_ScrollThumb)
				{
					m_ScrollThumb.SetPos(0, thumbY);
					m_ScrollThumb.SetSize(VPPContextMenuStyle.SCROLL_W, thumbH);
				}
			}
		}

		m_Height = hostY + contentH + VPPContextMenuStyle.PAD;
		if (m_LookMode)
		{
			if (m_Footer)
			{
				m_Footer.SetPos(VPPContextMenuStyle.PAD, hostY + contentH + VPPContextMenuStyle.PAD);
				m_Footer.SetSize(innerW, VPPContextMenuStyle.FOOTER_H);
			}
			m_Height = m_Height + VPPContextMenuStyle.FOOTER_H + VPPContextMenuStyle.PAD;
		}

		m_Root.SetSize(VPPContextMenuStyle.VIEW_WIDTH, m_Height);
	}

	void RefreshRow(int index)
	{
		ApplyRowVisual(index);
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
			else if (index >= first + VPPContextMenuStyle.MAX_VISIBLE_ITEMS)
				first = index - VPPContextMenuStyle.MAX_VISIBLE_ITEMS + 1;

			if (first != m_FirstVisible)
			{
				m_FirstVisible = first;
				LayoutRows();
			}
		}

		ApplyRowVisual(prev);
		ApplyRowVisual(index);
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
	}

	protected void ClampAndSetPos(float x, float y)
	{
		if (!m_Root)
			return;

		int sw, sh;
		GetScreenSize(sw, sh);

		float maxX = sw - VPPContextMenuStyle.VIEW_WIDTH;
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

		float x = (sw / 2.0) + VPPContextMenuStyle.LOOK_OFFSET_X;
		float y = (sh / 2.0) + VPPContextMenuStyle.LOOK_OFFSET_Y - VPPContextMenuStyle.PAD - VPPContextMenuStyle.HEADER_H;
		ClampAndSetPos(x, y);
	}

	void PlaceAtCursor(int x, int y)
	{
		int sw, sh;
		GetScreenSize(sw, sh);

		float px;
		if (x + VPPContextMenuStyle.CURSOR_OFFSET + VPPContextMenuStyle.VIEW_WIDTH > sw)
			px = x - VPPContextMenuStyle.VIEW_WIDTH - VPPContextMenuStyle.CURSOR_OFFSET;
		else
			px = x + VPPContextMenuStyle.CURSOR_OFFSET;

		float py;
		if (y + VPPContextMenuStyle.CURSOR_OFFSET + m_Height > sh)
			py = y - m_Height - VPPContextMenuStyle.CURSOR_OFFSET;
		else
			py = y + VPPContextMenuStyle.CURSOR_OFFSET;

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
