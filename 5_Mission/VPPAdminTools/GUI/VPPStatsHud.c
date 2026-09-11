/*
	Minimal, FPS-counter-style admin Stats HUD.

	Owned by VPPAdminHud (created once, deleted in ~VPPAdminHud). The widget is
	parented to the workspace root (not the menu) and driven by the global GUI
	update queue, so it survives the toolbar/root menu closing — the same
	"persist after close" mechanism used by the Server Manager monitor pin.

	Visibility self-gates each tick on: pinned OR the toolbar is showing.
	- pin button     : keep visible after the toolbar closes
	- corner button  : cycle the 4 screen corners
	- orient button  : toggle horizontal <-> vertical
	- copy buttons   : copy own coords / crosshair-object position to clipboard
*/
class VPPStatsHud : ScriptedWidgetEventHandler
{
	protected Widget       m_Root;
	protected bool         m_Pinned;
	protected int          m_Corner;     // 0=bottom-right, 1=bottom-left, 2=top-left, 3=top-right
	protected bool         m_Vertical;
	protected float        m_Tick;

	// cells (repositioned/resized on relayout)
	protected Widget       m_CellControls;
	protected Widget       m_CellPlayers;
	protected Widget       m_CellFlags;
	protected Widget       m_CellPosition;
	protected Widget       m_CellCrosshair;

	// interactive
	protected ButtonWidget m_BtnPin;
	protected ButtonWidget m_BtnCorner;
	protected ButtonWidget m_BtnOrient;
	protected ButtonWidget m_BtnCopyPos;
	protected ButtonWidget m_BtnCopyCross;
	protected ImageWidget  m_ImgPin;

	// values
	protected TextWidget   m_TxtPlayers;
	protected TextWidget   m_TxtPos;
	protected TextWidget   m_TxtCross;
	protected ImageWidget  m_ImgGod;
	protected ImageWidget  m_ImgInvis;
	protected ImageWidget  m_ImgFrozen;
	protected ImageWidget  m_ImgAmmo;

	// cached copy payloads
	protected string       m_OwnPosStr;
	protected string       m_CrossPosStr;
	protected bool         m_HasCross;

	protected const float  REFRESH      = 0.2;   // seconds between data refreshes

	void VPPStatsHud()
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.StatsHud, null);
		if (!m_Root)
			return;

		m_Root.SetSort(900, true); //above the gameplay HUD

		m_CellControls  = m_Root.FindAnyWidget("CellControls");
		m_CellPlayers   = m_Root.FindAnyWidget("CellPlayers");
		m_CellFlags     = m_Root.FindAnyWidget("CellFlags");
		m_CellPosition  = m_Root.FindAnyWidget("CellPosition");
		m_CellCrosshair = m_Root.FindAnyWidget("CellCrosshair");

		m_BtnPin       = ButtonWidget.Cast( m_Root.FindAnyWidget("BtnPin") );
		m_BtnCorner    = ButtonWidget.Cast( m_Root.FindAnyWidget("BtnCorner") );
		m_BtnOrient    = ButtonWidget.Cast( m_Root.FindAnyWidget("BtnOrient") );
		m_BtnCopyPos   = ButtonWidget.Cast( m_Root.FindAnyWidget("BtnCopyPos") );
		m_BtnCopyCross = ButtonWidget.Cast( m_Root.FindAnyWidget("BtnCopyCross") );
		m_ImgPin       = ImageWidget.Cast( m_Root.FindAnyWidget("ImgPin") );

		m_TxtPlayers   = TextWidget.Cast( m_Root.FindAnyWidget("TxtPlayers") );
		m_TxtPos       = TextWidget.Cast( m_Root.FindAnyWidget("TxtPos") );
		m_TxtCross     = TextWidget.Cast( m_Root.FindAnyWidget("TxtCross") );
		m_ImgGod       = ImageWidget.Cast( m_Root.FindAnyWidget("ImgGod") );
		m_ImgInvis     = ImageWidget.Cast( m_Root.FindAnyWidget("ImgInvis") );
		m_ImgFrozen    = ImageWidget.Cast( m_Root.FindAnyWidget("ImgFrozen") );
		m_ImgAmmo      = ImageWidget.Cast( m_Root.FindAnyWidget("ImgAmmo") );

		m_Root.SetHandler(this);

		m_Pinned   = false;
		m_Corner   = 0; //bottom-right
		m_Vertical = false;

		UpdatePinVisual();
		Relayout();
		m_Root.Show(false); //hidden until the toolbar opens (or it gets pinned)

		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.DoUpdate);
	}

	void ~VPPStatsHud()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
		if (m_Root)
		{
			m_Root.Unlink();
			m_Root = null;
		}
	}

	//ticks every frame regardless of menu visibility
	void DoUpdate(float dt)
	{
		if (!m_Root)
			return;

		m_Tick += dt;
		if (m_Tick < REFRESH)
			return;
		m_Tick = 0.0;

		bool open = false;
		VPPScriptedMenu hud = GetVPPUIManager().GetMenuByType(VPPAdminHud);
		if (hud)
			open = hud.IsShowing();

		//global on/off (persisted profile option) gates everything
		bool visible = g_Game.IsStatsHudEnabled() && (m_Pinned || open);
		m_Root.Show(visible);
		if (!visible)
			return;

		RefreshData();
	}

	protected void RefreshData()
	{
		//player count (client-resident, no RPC)
		if (m_TxtPlayers && GetPlayerListManager())
			m_TxtPlayers.SetText( GetPlayerListManager().GetCount().ToString() );

		//flags + position (own entity)
		PlayerBase pb = PlayerBase.Cast( GetGame().GetPlayer() );
		if (pb)
		{
			SetFlag(m_ImgGod,    pb.GodModeStatus());
			SetFlag(m_ImgInvis,  pb.InvisibilityStatus());
			SetFlag(m_ImgFrozen, pb.VPPIsFreezeControls());
			SetFlag(m_ImgAmmo,   pb.VPPIsUnlimitedAmmo());

			vector p = pb.GetPosition();
			m_OwnPosStr = p.ToString();
			if (m_TxtPos)
				m_TxtPos.SetText( string.Format("%1 %2 %3", p[0], p[1], p[2]) );
		}
		else
		{
			//no controlled entity (dead / spectating / freecam): don't show stale data
			m_OwnPosStr = "";
			if (m_TxtPos)
				m_TxtPos.SetText("-");
			SetFlag(m_ImgGod,    false);
			SetFlag(m_ImgInvis,  false);
			SetFlag(m_ImgFrozen, false);
			SetFlag(m_ImgAmmo,   false);
		}

		//object at crosshair
		Object obj = g_Game.getObjectAtCrosshair(1000.0, 0.0, NULL);
		if (obj)
		{
			m_HasCross    = true;
			m_CrossPosStr = obj.GetPosition().ToString();
			if (m_TxtCross)
				m_TxtCross.SetText( obj.GetType() );
		}
		else
		{
			m_HasCross = false;
			if (m_TxtCross)
				m_TxtCross.SetText("-");
		}
	}

	protected void SetFlag(ImageWidget img, bool active)
	{
		if (!img)
			return;
		if (active)
			img.SetColor( ARGB(255, 76, 175, 80) );   //green = active
		else
			img.SetColor( ARGB(255, 92, 97, 102) );    //grey  = inactive
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_BtnPin)
		{
			m_Pinned = !m_Pinned;
			UpdatePinVisual();
			return true;
		}
		if (w == m_BtnCorner)
		{
			m_Corner = (m_Corner + 1) % 4;
			Relayout();
			return true;
		}
		if (w == m_BtnOrient)
		{
			m_Vertical = !m_Vertical;
			Relayout();
			return true;
		}
		if (w == m_BtnCopyPos)
		{
			if (m_OwnPosStr != "")
			{
				GetGame().CopyToClipboard(m_OwnPosStr);
				GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SUCCESS_COPY_TOCLIPBOARD");
			}
			return true;
		}
		if (w == m_BtnCopyCross)
		{
			if (m_HasCross)
			{
				GetGame().CopyToClipboard(m_CrossPosStr);
				GetVPPUIManager().DisplayNotification("#VSTR_NOTIFY_SUCCESS_COPY_TOCLIPBOARD");
			}
			return true;
		}
		return false;
	}

	protected void UpdatePinVisual()
	{
		if (!m_ImgPin)
			return;
		if (m_Pinned)
			m_ImgPin.SetColor( ARGB(255, 232, 163, 61) );   //accent-orange = pinned
		else
			m_ImgPin.SetColor( ARGB(255, 154, 160, 166) );  //secondary-grey = unpinned
	}

	//Position the cells (row vs column), size the card, then anchor it to the chosen corner.
	protected void Relayout()
	{
		if (!m_Root)
			return;

		float wControls = 92.0;
		float wPlayers  = 70.0;
		float wFlags    = 104.0;
		float wPos      = 196.0;
		float wCross    = 210.0;
		float rowH      = 28.0;
		float gap       = 6.0;
		float pad       = 6.0;
		float vW        = 210.0; //uniform cell width when stacked vertically

		float cardW;
		float cardH;

		if (!m_Vertical)
		{
			float x = pad;
			PlaceCell(m_CellControls,  x, pad, wControls, rowH); x += wControls + gap;
			PlaceCell(m_CellPlayers,   x, pad, wPlayers,  rowH); x += wPlayers  + gap;
			PlaceCell(m_CellFlags,     x, pad, wFlags,    rowH); x += wFlags    + gap;
			PlaceCell(m_CellPosition,  x, pad, wPos,      rowH); x += wPos      + gap;
			PlaceCell(m_CellCrosshair, x, pad, wCross,    rowH); x += wCross;
			cardW = x + pad;
			cardH = pad + rowH + pad;
		}
		else
		{
			float y = pad;
			PlaceCell(m_CellControls,  pad, y, vW, rowH); y += rowH + gap;
			PlaceCell(m_CellPlayers,   pad, y, vW, rowH); y += rowH + gap;
			PlaceCell(m_CellFlags,     pad, y, vW, rowH); y += rowH + gap;
			PlaceCell(m_CellPosition,  pad, y, vW, rowH); y += rowH + gap;
			PlaceCell(m_CellCrosshair, pad, y, vW, rowH); y += rowH;
			cardW = pad + vW + pad;
			cardH = y + pad;
		}

		m_Root.SetSize(cardW, cardH);
		AnchorCorner(cardW, cardH);
	}

	protected void PlaceCell(Widget cell, float x, float y, float w, float h)
	{
		if (!cell)
			return;
		cell.SetPos(x, y);
		cell.SetSize(w, h);
	}

	protected void AnchorCorner(float cardW, float cardH)
	{
		int sw, sh;
		GetScreenSize(sw, sh);
		float m = 14.0; //screen-edge margin
		float px = m;
		float py = m;

		switch (m_Corner)
		{
			case 0: px = sw - cardW - m; py = sh - cardH - m; break; //bottom-right
			case 1: px = m;              py = sh - cardH - m; break; //bottom-left
			case 2: px = m;              py = m;              break; //top-left
			case 3: px = sw - cardW - m; py = m;              break; //top-right
		}

		m_Root.SetPos(px, py);
	}
};
