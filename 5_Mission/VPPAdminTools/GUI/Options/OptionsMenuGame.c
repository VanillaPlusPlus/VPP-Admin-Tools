modded class OptionsMenuGame 
{
	protected Widget 						m_VPPATSection;

	protected Widget 						m_widgetCamSpeed;
	protected Widget 						m_widgetCamBoost;
	protected Widget 						m_widgetCamMoveDrag;
	protected Widget 						m_widgetCamMouseSense;
	protected Widget 						m_widgetCamSmoothness;
	protected Widget 						m_widgetCamFOV;
	protected Widget 						m_widgetStatsHud;

	protected ref OptionSelectorSlider		m_CamSpeed;
	protected ref OptionSelectorSlider		m_CamBoost;
	protected ref OptionSelectorSlider		m_CamMoveDrag;
	protected ref OptionSelectorSlider		m_CamMouseSense;
	protected ref OptionSelectorSlider		m_CamSmoothness;
	protected ref OptionSelectorSlider		m_CamFOV;
	protected ref OptionSelectorMultistate	m_StatsHudToggle;

	void OptionsMenuGame( Widget parent, Widget details_root, GameOptions options, OptionsMenu menu )
	{
		//Create VPPAT section
		m_VPPATSection = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuVPPATSection, m_Root.FindAnyWidget("game_settings_root"));
		m_Root.FindAnyWidget("game_settings_root").Update();

		//Speed
		m_widgetCamSpeed = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamSpeed.FindAnyWidget("setting_label")).SetText("MOVEMENT SPEED");
		m_CamSpeed = new OptionSelectorSlider(m_VPPATSection.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED), this, false, VPPATProfileConstants.MIN_CAM_SPEED, VPPATProfileConstants.MAX_CAM_SPEED);
		m_CamSpeed.m_OptionChanged.Insert(OnCamSettingChanged);
		
		//Boost multiplier
		m_widgetCamBoost = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamBoost.FindAnyWidget("setting_label")).SetText("MOVEMENT BOOST MULTIPLIER");
		m_CamBoost = new OptionSelectorSlider(m_widgetCamBoost.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST), this, false, VPPATProfileConstants.MIN_CAM_BOOST, VPPATProfileConstants.MAX_CAM_BOOST);
		m_CamBoost.m_OptionChanged.Insert(OnCamSettingChanged);

		//Move drag
		m_widgetCamMoveDrag = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamMoveDrag.FindAnyWidget("setting_label")).SetText("MOVEMENT DRAG");
		m_CamMoveDrag = new OptionSelectorSlider(m_widgetCamMoveDrag.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG), this, false, VPPATProfileConstants.MIN_CAM_MOVE_DRAG, VPPATProfileConstants.MAX_CAM_MOVE_DRAG);
		m_CamMoveDrag.m_OptionChanged.Insert(OnCamSettingChanged);

		//Mouse sense
		m_widgetCamMouseSense = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamMouseSense.FindAnyWidget("setting_label")).SetText("MOUSE SENSITIVITY");
		m_CamMouseSense = new OptionSelectorSlider(m_widgetCamMouseSense.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE), this, false, VPPATProfileConstants.MIN_CAM_MOUSE_SENSE, VPPATProfileConstants.MAX_CAM_MOUSE_SENSE);
		m_CamMouseSense.m_OptionChanged.Insert(OnCamSettingChanged);

		//Smoothness
		m_widgetCamSmoothness = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamSmoothness.FindAnyWidget("setting_label")).SetText("MOUSE SMOOTHNESS");
		m_CamSmoothness = new OptionSelectorSlider(m_widgetCamSmoothness.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS), this, false, VPPATProfileConstants.MIN_CAM_SMOOTHNESS, VPPATProfileConstants.MAX_CAM_SMOOTHNESS);
		m_CamSmoothness.m_OptionChanged.Insert(OnCamSettingChanged);

		//FOV
		m_widgetCamFOV = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetCamFOV.FindAnyWidget("setting_label")).SetText("FIELD OF VIEW");
		m_CamFOV = new OptionSelectorSlider(m_widgetCamFOV.FindAnyWidget("setting_option"), g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV), this, false, VPPATProfileConstants.MIN_CAM_FOV, VPPATProfileConstants.MAX_CAM_FOV);
		m_CamFOV.m_OptionChanged.Insert(OnCamSettingChanged);

		//Stats HUD global enable/disable (vanilla-style two-state toggle)
		array<string> statsHudOpts = { "#options_controls_disabled", "#options_controls_enabled" };
		m_widgetStatsHud = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.SettingsMenuElement, m_Root.FindAnyWidget("vppat_settings_root"));
		TextWidget.Cast(m_widgetStatsHud.FindAnyWidget("setting_label")).SetText("ADMIN STATS HUD");
		m_StatsHudToggle = new OptionSelectorMultistate(m_widgetStatsHud.FindAnyWidget("setting_option"), StatsHudSavedIndex(), this, false, statsHudOpts);
		m_StatsHudToggle.m_OptionChanged.Insert(OnStatsHudToggleChanged);
	}
	
	override void Apply()
	{
		super.Apply();
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED, m_CamSpeed.GetValue());
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST, m_CamBoost.GetValue());
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG, m_CamMoveDrag.GetValue());
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE, m_CamMouseSense.GetValue());
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS, m_CamSmoothness.GetValue());
		g_Game.SetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV, m_CamFOV.GetValue());
		if (m_StatsHudToggle)
			g_Game.SetStatsHudEnabled(m_StatsHudToggle.GetValue() == 1);
	}

	override bool IsChanged()
  	{
  		float currentVal;
  		float savedVal;

  		if (m_CamSpeed)
  		{
  			currentVal = m_CamSpeed.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}

  		if (m_CamBoost)
  		{
  			currentVal = m_CamBoost.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}
  		
  		if (m_CamMoveDrag)
  		{
  			currentVal = m_CamMoveDrag.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}
  		
  		if (m_CamMouseSense)
  		{
  			currentVal = m_CamMouseSense.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}
  		
  		if (m_CamSmoothness)
  		{
  			currentVal = m_CamSmoothness.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}
  		
  		if (m_CamFOV)
  		{
  			currentVal = m_CamFOV.GetValue();
			savedVal   = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV);

			if (Math.AbsFloat(currentVal - savedVal) > 0.0005)
			    return true;
  		}

  		if (m_StatsHudToggle)
  		{
  			if (m_StatsHudToggle.GetValue() != StatsHudSavedIndex())
  				return true;
  		}
  		return super.IsChanged();
  	}

	override void Revert()
	{
		super.Revert();

		float oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		m_CamSpeed.SetValue(oldVal, false);

		oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST);
		m_CamBoost.SetValue(oldVal, false);

		oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG);
		m_CamMoveDrag.SetValue(oldVal, false);

		oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE);
		m_CamMouseSense.SetValue(oldVal, false);

		oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS);
		m_CamSmoothness.SetValue(oldVal, false);

		oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV);
		m_CamFOV.SetValue(oldVal, false);

		if (m_StatsHudToggle)
			m_StatsHudToggle.SetValue(StatsHudSavedIndex(), false);
	}
	
	override void SetOptions( GameOptions options )
	{
		super.SetOptions( options );

		if(m_CamSpeed)
			m_CamSpeed.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED), false);

		if (m_CamBoost)
			m_CamBoost.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST), false);

		if (m_CamMoveDrag)
			m_CamMoveDrag.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG), false);

		if (m_CamMouseSense)
			m_CamMouseSense.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE), false);

		if (m_CamSmoothness)
			m_CamSmoothness.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS), false);

		if (m_CamFOV)
			m_CamFOV.SetValue(g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV), false);

		if (m_StatsHudToggle)
			m_StatsHudToggle.SetValue(StatsHudSavedIndex(), false);
	}

	void OnCamSettingChanged(float new_value)
	{
		m_Menu.OnChanged();
	}

	void OnStatsHudToggleChanged(int new_value)
	{
		m_Menu.OnChanged();
	}

	//saved toggle state as a multistate index (1 = enabled, 0 = disabled). No ternary — Enforce lacks it.
	int StatsHudSavedIndex()
	{
		if (g_Game.IsStatsHudEnabled())
			return 1;
		return 0;
	}
};
