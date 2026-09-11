class VPPSpectateOverlay : ScriptedWidgetEventHandler
{
	protected Widget      m_Root;
	protected TextWidget  m_TxtName;
	protected TextWidget  m_TxtMode;
	protected TextWidget  m_TxtHP;
	protected TextWidget  m_TxtBlood;
	protected TextWidget  m_TxtHints;
	protected float       m_Tick;
	protected bool        m_TargetLostFlash;
	protected int         m_FlashUntil; //GetGame().GetTime() deadline for the target-lost flash

	protected const float REFRESH = 0.2;

	void VPPSpectateOverlay()
	{
		m_Root = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SpectateOverlay, null);
		if (!m_Root)
			return;

		m_Root.SetSort(900, true);

		m_TxtName  = TextWidget.Cast( m_Root.FindAnyWidget("TxtOverlayName") );
		m_TxtMode  = TextWidget.Cast( m_Root.FindAnyWidget("TxtOverlayMode") );
		m_TxtHP    = TextWidget.Cast( m_Root.FindAnyWidget("TxtVitalsHP") );
		m_TxtBlood = TextWidget.Cast( m_Root.FindAnyWidget("TxtVitalsBlood") );
		m_TxtHints = TextWidget.Cast( m_Root.FindAnyWidget("TxtOverlayHints") );

		//static hint line (default keybinds; runtime-localized verbs)
		string hints = "PgUp " + Widget.TranslateString("#VSTR_SPECTATE_HINT_EXIT");
		hints = hints + " V " + Widget.TranslateString("#VSTR_SPECTATE_HINT_VIEW");
		hints = hints + " N " + Widget.TranslateString("#VSTR_SPECTATE_HINT_NEXT");
		hints = hints + " B " + Widget.TranslateString("#VSTR_SPECTATE_HINT_PREV");
		m_TxtHints.SetText(hints);

		GetSpectateClient().m_OnTargetLost.Insert(OnTargetLostEvt);

		m_Root.Show(false);
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Insert(this.DoUpdate);
	}

	void ~VPPSpectateOverlay()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
		if (m_Root)
		{
			m_Root.Unlink();
			m_Root = null;
		}
	}

	void DoUpdate(float dt)
	{
		if (!m_Root)
			return;

		m_Tick += dt;
		if (m_Tick < REFRESH)
			return;
		m_Tick = 0.0;

		VPPSpectateClientHandler client = GetSpectateClient();
		//keep the overlay up briefly after target-lost so the red flash is readable
		bool flashHold = (m_TargetLostFlash && GetGame().GetTime() < m_FlashUntil);
		bool visible = (client != null && client.IsSpectating()) || flashHold;
		m_Root.Show(visible);
		if (!visible)
		{
			m_TargetLostFlash = false;
			return;
		}

		if (flashHold && (client == null || !client.IsSpectating()))
			return; //hold the flash frame; skip live-data refresh

		//top-center anchor, resolution-safe
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float w;
		float h;
		m_Root.GetSize(w, h);
		m_Root.SetPos((sw - w) / 2, 14);

		if (!m_TargetLostFlash)
		{
			m_TxtName.SetColor(ARGB(255, 243, 245, 247));
			m_TxtName.SetText(ResolveName(client.GetTargetId()));
		}

		if (client.IsFirstPerson())
		{
			if (client.IsTargetADS())
				m_TxtMode.SetText("ADS");
			else
				m_TxtMode.SetText("1PP");
		}
		else
		{
			m_TxtMode.SetText("3PP");
		}

		m_TxtHP.SetText(Math.Round(client.GetVitalsHP()).ToString() + "%");
		m_TxtBlood.SetText(Math.Round(client.GetVitalsBlood()).ToString());
	}

	protected string ResolveName(string userId)
	{
		if (GetPlayerListManager())
		{
			array<ref VPPUser> users = GetPlayerListManager().GetUsers();
			foreach (VPPUser user : users)
			{
				if (user.GetUserId() == userId)
					return user.GetUserName();
			}
		}
		return userId;
	}

	void OnTargetLostEvt(string targetId)
	{
		//brief danger flash, held ~2s past teardown so it's actually readable
		m_TargetLostFlash = true;
		m_FlashUntil = GetGame().GetTime() + 2000;
		if (m_TxtName)
		{
			m_TxtName.SetColor(ARGB(255, 194, 69, 69));
			m_TxtName.SetText(Widget.TranslateString("#VSTR_SPECTATE_TARGET_LOST"));
		}
	}
};
