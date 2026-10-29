/*
	Context menu metrics + colors (design tokens from GUI/Styles/DesignSystem.md).
	Metrics are 1080p pixels: the view sits at the (unscaled) workspace root and multiplies them by its UI scale
	(screen height / REF_SCREEN_H, never below MIN_SCALE). VIEW_WIDTH is the minimum width; the view grows to fit
	its longest label + hint, up to the screen width.
	Colors are functions because const members cannot call ARGB().
*/
class VPPContextMenuStyle
{
	const static int   VIEW_SORT                = 1024;
	const static float REF_SCREEN_H             = 1080;
	const static float MIN_SCALE                = 1.0;
	const static float VIEW_WIDTH               = 280;
	const static float SCREEN_MARGIN            = 8;
	const static float PAD                      = 4;
	const static float HEADER_H                 = 26;
	const static float ROW_H                    = 30;
	const static float ROW_GAP                  = 2;
	const static float SEP_H                    = 9;
	const static float SEP_LINE_X               = 8;
	const static float FOOTER_H                 = 20;
	const static int   MAX_VISIBLE_ITEMS        = 14;
	const static float SCROLL_W                 = 3;
	const static float SCROLL_GAP               = 3;
	const static float ICON_X                   = 10;
	const static float ICON_SIZE                = 16;
	const static float HEADER_ICON_X            = 8;
	const static float HEADER_TEXT_X            = 30;
	const static float LABEL_X                  = 34;
	const static float LABEL_GAP                = 10;   //between the label and its hint
	const static float CHEVRON_W                = 20;
	const static float CHEVRON_SIZE             = 12;
	const static float RIGHT_PAD                = 8;
	const static float TEXT_PAD                 = 4;    //slack past a measured text width
	const static int   HEADER_TEXT_SIZE         = 14;
	const static int   LABEL_TEXT_SIZE          = 15;
	const static int   HINT_TEXT_SIZE           = 12;
	const static int   FOOTER_TEXT_SIZE         = 12;
	const static float TEXT_EM_ESTIMATE         = 0.6;  //width per char / text size, only when GetTextSize reads 0
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
