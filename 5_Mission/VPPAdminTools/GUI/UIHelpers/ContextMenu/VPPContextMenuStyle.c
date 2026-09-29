/*
	Context menu metrics + colors (design tokens from GUI/Styles/DesignSystem.md).
	Colors are functions because const members cannot call ARGB().
*/
class VPPContextMenuStyle
{
	const static int   VIEW_SORT                = 1024;
	const static float VIEW_WIDTH               = 280;
	const static float PAD                      = 4;
	const static float HEADER_H                 = 26;
	const static float ROW_H                    = 30;
	const static float ROW_GAP                  = 2;
	const static float SEP_H                    = 9;
	const static float FOOTER_H                 = 20;
	const static int   MAX_VISIBLE_ITEMS        = 14;
	const static float SCROLL_W                 = 3;
	const static float ICON_X                   = 10;
	const static float LABEL_X                  = 34;
	const static float LABEL_GAP                = 4;
	const static float LABEL_TEXT_W             = 600;   //text widget wider than its clip frame; the frame clips
	const static float HINT_W                   = 96;
	const static int   HINT_MAX_CHARS           = 18;    //longer hints -> first 16 chars + ".."
	const static float CHEVRON_W                = 20;
	const static float RIGHT_PAD                = 8;
	const static float CURSOR_OFFSET            = 2;
	const static float LOOK_OFFSET_X            = 36;
	const static float LOOK_OFFSET_Y            = -20;
	const static float CHEVRON_BACK_ROTATION    = 90;    //flip sign if wrong in game
	const static float CHEVRON_SUBMENU_ROTATION = -90;

	//bg-row
	static int RowBg()
	{
		return ARGB(255, 27, 30, 34);
	}

	//accent-indigo
	static int RowFocus()
	{
		return ARGB(255, 71, 70, 180);
	}

	//danger-red
	static int RowConfirm()
	{
		return ARGB(255, 194, 69, 69);
	}

	static int TextPrimary()
	{
		return ARGB(255, 255, 255, 255);
	}

	static int TextSecondary()
	{
		return ARGB(255, 154, 160, 166);
	}

	static int TextMuted()
	{
		return ARGB(255, 92, 97, 102);
	}

	static int Danger()
	{
		return ARGB(255, 194, 69, 69);
	}
};
