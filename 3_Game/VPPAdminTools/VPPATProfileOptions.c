enum EVPPATProfileOptions
{
	CAM_SPEED,
	CAM_BOOST,
	CAM_MOVE_DRAG,
	CAM_MOUSE_SENSE,
	CAM_SMOOTHNESS,
	CAM_FOV
};

typedef Param3<string, float, float> VPPProfileOptionDta;

class VPPATProfileOptions
{
	private ref map<EVPPATProfileOptions, ref VPPProfileOptionDta> m_CustomOptions;

	void RegisterProfileOption(EVPPATProfileOptions option, string profileOptionName, float def = 0.0)
	{
		if ( !m_CustomOptions )
		{
			m_CustomOptions = new map<EVPPATProfileOptions, ref VPPProfileOptionDta>;
		}
		
		if ( !m_CustomOptions.Contains(option) )
		{
			string value;
 			GetGame().GetProfileString(profileOptionName, value);
 			if (value == string.Empty || value == "0")
 			{
 				m_CustomOptions.Set(option, new VPPProfileOptionDta(profileOptionName, def, def));
				SetProfileOptionVal(option, def);
 			}
 			else
 			{
 				m_CustomOptions.Set(option, new VPPProfileOptionDta(profileOptionName, value.ToFloat(), def));
				SetProfileOptionVal(option, value.ToFloat());
 			}
		}
	}
	
	void ResetOptions()
	{
		if ( !m_CustomOptions )
		{
			m_CustomOptions = new map<EVPPATProfileOptions, ref VPPProfileOptionDta>;
		}

		foreach (EVPPATProfileOptions e_opt, VPPProfileOptionDta r_opt : m_CustomOptions)
		{
			string value;
 			if (GetGame().GetProfileString(r_opt.param1, value))
    		{
    			SetProfileOptionVal(e_opt, value.ToFloat());
    		}else{
    			SetProfileOptionVal(e_opt, r_opt.param3);
    		}
		}	
	}
	
	void SetProfileOptionVal(EVPPATProfileOptions option, float value)
	{
		if ( m_CustomOptions && m_CustomOptions.Contains(option) )
		{
			VPPProfileOptionDta po = m_CustomOptions.Get(option);
			
			po.param2 = value;
			GetGame().SetProfileString(po.param1, value.ToString());
			GetGame().SaveProfile();
		}	
	}

	float GetProfileOptionVal(EVPPATProfileOptions option)
	{
		if ( m_CustomOptions && m_CustomOptions.Contains(option) )
		{
			VPPProfileOptionDta po = m_CustomOptions.Get(option);
			return po.param2;
		}
		return 0.0;
	}

	map<EVPPATProfileOptions, ref VPPProfileOptionDta> GetCustomProfileOptionMap()
	{
		return m_CustomOptions;
	}
};