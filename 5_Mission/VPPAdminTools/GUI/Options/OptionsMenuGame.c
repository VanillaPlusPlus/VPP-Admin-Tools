modded class OptionsMenuGame 
{
	protected Widget 						m_CustomSetting;
	protected Widget 						m_VPPATSection;

	protected Widget 						m_widgetCamSpeed;
	protected Widget 						m_widgetCamBoost;
	protected Widget 						m_widgetCamMoveDrag;
	protected Widget 						m_widgetCamMouseSense;
	protected Widget 						m_widgetCamSmoothness;
	protected Widget 						m_widgetCamFOV;

	protected ref OptionSelectorSlider		m_CamSpeed;
	protected ref OptionSelectorSlider		m_CamBoost;
	protected ref OptionSelectorSlider		m_CamMoveDrag;
	protected ref OptionSelectorSlider		m_CamMouseSense;
	protected ref OptionSelectorSlider		m_CamSmoothness;
	protected ref OptionSelectorSlider		m_CamFOV;

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
		if (m_CamSpeed)
			m_CamSpeed.SetHasChanged(false);
	}

	override bool IsChanged()
  	{
  		if (m_CamSpeed && m_CamSpeed.HasChanged())
  		{
  			return true;
  		}

  		if (m_CamBoost)
  		{
  			if (m_CamBoost.GetValue() != g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_BOOST)){
  				return true;
  			}
  		}
  		
  		if (m_CamMoveDrag)
  		{
  			if (m_CamMoveDrag.GetValue() != g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOVE_DRAG)){
  				return true;
  			}
  		}
  		
  		if (m_CamMouseSense)
  		{
  			if (m_CamMouseSense.GetValue() != g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_MOUSE_SENSE)){
  				return true;
  			}
  		}
  		
  		if (m_CamSmoothness)
  		{
  			if (m_CamSmoothness.GetValue() != g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SMOOTHNESS)){
  				return true;
  			}
  		}
  		
  		if (m_CamFOV)
  		{
  			if (m_CamFOV.GetValue() != g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_FOV)){
  				return true;
  			}
  		}
  		return super.IsChanged();
  	}

	override void Revert()
	{
		super.Revert();

		float oldVal = g_Game.GetVPPATProfileVal(EVPPATProfileOptions.CAM_SPEED);
		if (m_CamSpeed){
			m_CamSpeed.SetValue(oldVal, false);
			m_CamSpeed.SetHasChanged(false);
		}

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
	}
	
	void OnCamSettingChanged(float new_value)
	{
		m_Menu.OnChanged();
	}
};
