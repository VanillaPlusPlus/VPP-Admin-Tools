class MenuWeatherManager extends AdminHudSubMenu
{
	private Widget 				 			m_WeatherPresetsDropDown;
	private Widget 							m_TimePresetsDropDown;
	
	protected ref VPPDropDownMenu 	 		m_WeatherPresetsDropDownInstance;
	protected ref VPPDropDownMenu			m_TimePresetsInstance;
	
	private SliderWidget m_SliderRain;
	private EditBoxWidget m_RainRate;
	private EditBoxWidget m_RainDur;
	
	private SliderWidget m_SliderOvercast;
	private EditBoxWidget m_OvercastRate;
	private EditBoxWidget m_OvercastDur;
	
	private SliderWidget m_SliderFog;
	private EditBoxWidget m_FogRate;
	private EditBoxWidget m_FogDur;
	
	private SliderWidget m_SliderWindForce;
	
	//World Time Input
	private EditBoxWidget hourInput;
	private EditBoxWidget minInput;
	private EditBoxWidget dayInput;
	private EditBoxWidget monthInput;
	private EditBoxWidget yearInput;
	//End
		
	//Display Data
	private TextWidget weatherTime;
	private TextWidget dateDisplay;
	
	private TextWidget rainDisplay;
	private TextWidget overcastDisplay;
	private TextWidget fogDisplay;
	private TextWidget windDisplay;
	//End
	private ButtonWidget setDateButton;
	private ButtonWidget setWeatherButton;

	//Weather Saving
	private EditBoxWidget weatherName;
	private ButtonWidget  saveWeather;
	private ButtonWidget  deleteWeather;
	private ButtonWidget  applyWeatherPreset;
	//end
	
	//Time Saving
	private ButtonWidget   saveTime;
	private ButtonWidget   deleteTime;
	private ButtonWidget   applyTimePreset;
	private CheckBoxWidget ChkBoxWorldOrCustom;
	//end
	
	private ImageWidget    m_ImgInfoTimePreset;
	private ImageWidget    m_ImgInfoWeatherPreset;
	
	private ref array<string> timePresetNames;
	private ref array<string> weatherPresetNames;
	
	Input m_Controls = GetGame().GetInput();
	
	void MenuWeatherManager()
	{
		Print("MenuWeatherManager()");
		
		timePresetNames = new array<string>;
		weatherPresetNames = new array<string>;
		
		GetRPCManager().AddRPC("RPC_WeatherUI", "HandleTimeData", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WeatherUI", "HandleWeatherData", this, SingleplayerExecutionType.Server);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		//System
		M_SUB_WIDGET  = CreateWidgets( "VPPAdminTools/GUI/Layouts/WeatherManagerUI/MenuWeatherManager.layout");
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		//End
		
		//Weather Changer
		m_SliderRain = SliderWidget.Cast( M_SUB_WIDGET.FindAnyWidget("SliderRain"));
		m_RainRate = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditRainRate"));
		m_RainDur = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditRainDur"));
		
		//
		m_SliderOvercast  = SliderWidget.Cast( M_SUB_WIDGET.FindAnyWidget("SliderOvercast"));
		m_OvercastRate = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditOvercastRate"));
		m_OvercastDur = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditOvercastDur"));
		
		//
		m_SliderFog  	   = SliderWidget.Cast( M_SUB_WIDGET.FindAnyWidget("SliderFog") );
		m_FogRate = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditFogRate"));
		m_FogDur = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditFogDur"));
		
		//
		m_SliderWindForce = SliderWidget.Cast( M_SUB_WIDGET.FindAnyWidget("SliderWindForce") );
		//End
				
		//Time Changer
		hourInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditHour"));
		minInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditMinute"));
		dayInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditDay"));
		monthInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditMonth"));
		yearInput = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditYear"));
		
		setDateButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnSetDate"));
		setWeatherButton = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnSetWeather"));		
		//End
		
		//Data Text Widgets
		weatherTime = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("WeatherTimeInput"));
		dateDisplay = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("WorldDateInput"));
		
		rainDisplay = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("RainValueInput"));
		overcastDisplay = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("OverCastInput"));
		fogDisplay = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("FogInput"));
		windDisplay = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("WindForceInput"));
		//End
		
		//Preset Drop Downs
		m_TimePresetsDropDown  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget("TimePresetsDropDown") );
		m_TimePresetsInstance = new VPPDropDownMenu( m_TimePresetsDropDown, "Time Presets" );
		m_TimePresetsInstance.m_OnSelectItem.Insert( OnSelectTimePreset );
		
			
		m_WeatherPresetsDropDown = Widget.Cast( M_SUB_WIDGET.FindAnyWidget("WeatherPresetsDropDown") );
		m_WeatherPresetsDropDownInstance  = new VPPDropDownMenu( m_WeatherPresetsDropDown, "Weather Presets" );
		m_WeatherPresetsDropDownInstance.m_OnSelectItem.Insert( OnSelectWeatherPreset );
		//end
		
		//Preset Weather Saving, and Applying
		weatherName = EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("EditPresetName"));
		saveWeather = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnSaveWeatherPreset"));
		deleteWeather = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnDeleteTimePreset"));
		//End
				
		//TimePreset Saving, and Applying
		saveTime = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnSaveTimePreset"));
		deleteTime = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnDeleteWeatherPreset"));
		applyTimePreset = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyTimePreset"));
		applyWeatherPreset = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget("BtnApplyWeatherPreset"));
		ChkBoxWorldOrCustom = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ChkBoxWorldOrCustom"));
		//end
		
		m_ImgInfoTimePreset    = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfoTimePreset"));
		autoptr ToolTipHandler toolTip;
		m_ImgInfoTimePreset.GetScript(toolTip);
		toolTip.SetTitle("Information:");
		toolTip.SetContentText("This feature allows you to save presets of current world time OR the time data that you have typed in the 'World Time' section of the menu.\nTick the box `Use Current Time` to save time directly from the world (current time, as shown in the `Information` section of the menu.)");
		
		m_ImgInfoWeatherPreset = ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget("ImgInfoWeatherPreset"));
		m_ImgInfoWeatherPreset.GetScript(toolTip);
		toolTip.SetTitle("Information:");
		toolTip.SetContentText("This feature allows you to save weather presets that you can apply when you wish to. Duplicate names are not allowed!\nOpening and closing this menu will refresh the presets list if other admins made any new presets while you had it open.");
		
		ShowSubMenu();
	}
	
	override void ShowSubMenu()
	{
		RequestTimeData();
		RequestWeatherData();
		
		super.ShowSubMenu();
	}
	
	void RequestTimeData()
	{
		GetRPCManager().SendRPC( "RPC_TimeManager", "SendToClient", null, true);
	}
	
	void RequestWeatherData()
	{
		GetRPCManager().SendRPC( "RPC_WeatherManager", "SendToClient", null, true);
	}
	
	void HandleTimeData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<array<string>> data;
			
			if(!ctx.Read(data)) return;
			
			ref array<string> input = data.param1;
			
			timePresetNames.Clear();
			
			foreach(string s : input)
			{
				timePresetNames.Insert(s);
			}
			
			UpdateTimeSaves();
		}
	}
	
	void HandleWeatherData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<array<string>> data;
			if(!ctx.Read(data)) return;
			
			array<string> input = data.param1;
			
			weatherPresetNames.Clear();
			foreach(string s : input)
			{
				weatherPresetNames.Insert(s);
			}
			
			UpdateWeatherSaves();
		}
	}
	
	void OnSelectTimePreset(int index)
	{
		m_TimePresetsInstance.SetText(timePresetNames.Get(index));
		m_TimePresetsInstance.Close();
	}
	
	void OnSelectWeatherPreset(int index)
	{
		m_WeatherPresetsDropDownInstance.SetText(weatherPresetNames.Get(index));
		m_WeatherPresetsDropDownInstance.Close();
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		float scrollState;
		
		if (IsSubMenuVisible() && M_SUB_WIDGET)
		{
			//Time and Date update
			int year, month, day, hour, minute;
			string formattedTime;
			string formattedDate;
			
			GetGame().GetWorld().GetDate(year, month, day, hour, minute);
			
			if(minute >= 10)
				formattedTime = hour.ToString() + ":" + minute.ToString();
			else
				formattedTime = hour.ToString() + ":" + "0" + minute.ToString();
			
			formattedDate = month.ToString() + "/" + day.ToString() + "/" + year.ToString();
			
			dateDisplay.SetText(formattedDate);
			weatherTime.SetText(formattedTime);
			
			//Weather Update
			Weather w = GetGame().GetWeather();
			
			int overcastInt = w.GetOvercast().GetActual() * 100;
			int rainInt = w.GetRain().GetActual() * 100;
			int fogInt = w.GetFog().GetActual() * 100;
			
			int windforce = (int) w.GetWindSpeed();
			
			overcastDisplay.SetText(overcastInt.ToString());
			rainDisplay.SetText(rainInt.ToString());
			fogDisplay.SetText(fogInt.ToString());
			windDisplay.SetText(windforce.ToString());

			
			autoptr EditBoxWidget editWidget;
			
			if (Class.CastTo(editWidget,GetWidgetUnderCursor()))
			{
				scrollState   = m_Controls.LocalValue("UAPrevAction") - m_Controls.LocalValue("UANextAction");
				if (scrollState > 0)
				{
					if (editWidget.GetText() != string.Empty && editWidget.GetText().ToInt() < 9999)
						editWidget.SetText((editWidget.GetText().ToInt() + 1).ToString());
					
				}else if (scrollState < 0)
				{
					if (editWidget.GetText() != string.Empty && editWidget.GetText().ToInt() > 0)
						editWidget.SetText((editWidget.GetText().ToInt() - 1).ToString());
					
				}
			}
		}
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case setDateButton:
				ApplyDate();
			break;
			
			case setWeatherButton:
				ApplyWeather();
			break;
			
			case saveWeather:
				SaveWeather();
			break;
			
			case deleteWeather:
				DeleteWeather();
			break;
			
			case saveTime:
				SaveDate();
			break;
			
			case deleteTime:
				DeleteDate();
			break;
			
			case applyTimePreset:
				ApplyTimePreset();
			break;
			
			case applyWeatherPreset:
				ApplyWeatherPreset();
			break;
		}
		return false;
	}
	
	private void DeleteWeather()
	{
		string name = m_WeatherPresetsDropDownInstance.GetText();
		
		if(name == "Weather Presets")
		{
			GetVPPUIManager().DisplayError("Please select a weather preset to delete.");
			return;
		}
		
		if(weatherPresetNames.Find(name) < 0)
		{
			GetVPPUIManager().DisplayError("Weather Setting doesn't exist.");
			return;
		}
		
		weatherPresetNames.RemoveItem(name);
		m_WeatherPresetsDropDownInstance.SetText("Weather Presets");

		UpdateWeatherSaves();
		GetRPCManager().SendRPC( "RPC_WeatherManager", "RemoveWeatherSettingsFromServer", new Param1<string>(name), true );
		GetVPPUIManager().DisplayNotification("Deleting Weather Preset...");
	}
	
	private void SaveWeather()
	{
		string name = weatherName.GetText();
		
		if(name == string.Empty || name == "Enter Name")
		{
			GetVPPUIManager().DisplayError("Incorrect Weather Preset Name");
			return;
		}
		
		if(weatherPresetNames.Find(name) >= 0)
		{
			GetVPPUIManager().DisplayError("Name is already in use.");
			return;
		}
		
		if(m_OvercastRate.GetText() == string.Empty || m_OvercastDur.GetText() == string.Empty || m_RainRate.GetText() == string.Empty || m_RainDur.GetText() == string.Empty || m_FogDur.GetText() == string.Empty || m_FogRate.GetText() == string.Empty)
		{
			GetVPPUIManager().DisplayError("Invalid Weather Data");
			return;
		}
		
		weatherPresetNames.Insert(name);
		UpdateWeatherSaves();
		
		
		autoptr array<float> overcast;
		autoptr array<float> fog;
		autoptr array<float> rain;
		float wind;
		
		GetWeatherValues(overcast, fog, rain, wind);
		
		GetRPCManager().SendRPC( "RPC_WeatherManager", "AddWeatherSettingToServer", new Param5<array<float>, array<float>, array<float>, float, string>(overcast, fog, rain, wind, name), true );
		GetVPPUIManager().DisplayNotification("Saving Weather Preset...");
	}
	
	private void ApplyWeather()
	{
		if(m_OvercastRate.GetText() == string.Empty || m_OvercastDur.GetText() == string.Empty || m_RainRate.GetText() == string.Empty || m_RainDur.GetText() == string.Empty || m_FogDur.GetText() == string.Empty || m_FogRate.GetText() == string.Empty)
		{
			GetVPPUIManager().DisplayError("Invalid Weather Data");
			return;
		}
		
		autoptr array<float> overcast;
		autoptr array<float> fog;
		autoptr array<float> rain;
		float wind;
		
		GetWeatherValues(overcast, fog, rain, wind);

		GetRPCManager().SendRPC( "RPC_WeatherManager", "ApplyWeather", new Param4<array<float>, array<float>, array<float>, float>(overcast, fog, rain, wind), true );
		GetVPPUIManager().DisplayNotification("Applying Weather...");
	}
	
	private void ApplyWeatherPreset()
	{
		string name = m_WeatherPresetsDropDownInstance.GetText();
		
		if(name == string.Empty || name == "Weather Presets")
		{
			GetVPPUIManager().DisplayError("Please select a weather preset.");
			return;
		}
		
		GetRPCManager().SendRPC( "RPC_WeatherManager", "ApplyPreset", new Param1<string>(name), true );
		GetVPPUIManager().DisplayNotification("Applying Weather Preset...");
	}
	
	private void DeleteDate()
	{
		string name = m_TimePresetsInstance.GetText();
		
		if(name == "Time Presets")
		{
			GetVPPUIManager().DisplayError("Please select a time preset to delete.");
			return;
		}
		
		if(timePresetNames.Find(name) < 0)
		{
			GetVPPUIManager().DisplayError("Time Preset doesn't exist.");
			return;
		}
		
		m_TimePresetsInstance.SetText("Time Presets");
		timePresetNames.RemoveItem(name);
		GetRPCManager().SendRPC( "RPC_TimeManager", "DeleteTimeSetting", new Param1<string>(name), true );
		UpdateTimeSaves();
		GetVPPUIManager().DisplayNotification("Removing Date Preset...");
	}
	
	private void SaveDate()
	{
		int hour, min, day, month, year;
		string name;
		
		if (ChkBoxWorldOrCustom.IsChecked())
		{
			GetGame().GetWorld().GetDate(year, month, day, hour, min);
		}else{
			year = yearInput.GetText().ToInt();
			month = monthInput.GetText().ToInt();
			day = dayInput.GetText().ToInt();
			min = minInput.GetText().ToInt();
			hour = hourInput.GetText().ToInt();
			
			if(yearInput.GetText() == string.Empty || monthInput.GetText() == string.Empty || dayInput.GetText() == string.Empty || minInput.GetText() == string.Empty || hourInput.GetText() == string.Empty)
			{
				GetVPPUIManager().DisplayError("Missing Date information.");
				return;
			}
		}
		
		
		if(min >= 10)
			name = month.ToString() + "/" + day.ToString() + "/" + year.ToString() + " " + hour.ToString() + ":" + min.ToString();
		else
			name = month.ToString() + "/" + day.ToString() + "/" + year.ToString() + " " + hour.ToString() + ":0" +  min.ToString();
	
		if(timePresetNames.Find(name) >= 0)
		{
			GetVPPUIManager().DisplayError("Duplicate Time Presets");
			return;
		}
		
		timePresetNames.Insert(name);
		UpdateTimeSaves();
		
		GetRPCManager().SendRPC( "RPC_TimeManager", "SaveTimeSettings", new Param5<int, int, int, int, int>(month, day, year, min, hour), true );
		GetVPPUIManager().DisplayNotification("Saving Date Preset...");
	}
	
	private void ApplyDate()
	{
		int hour, min, day, month, year;
		string name;
		
		if(yearInput.GetText() == string.Empty || monthInput.GetText() == string.Empty || dayInput.GetText() == string.Empty || minInput.GetText() == string.Empty || hourInput.GetText() == string.Empty)
		{
			GetVPPUIManager().DisplayError("Missing Date information.");
			return;
		}
		
		year = yearInput.GetText().ToInt();
		month = monthInput.GetText().ToInt();
		day = dayInput.GetText().ToInt();
		min = minInput.GetText().ToInt();
		hour = hourInput.GetText().ToInt();
		
		GetRPCManager().SendRPC( "RPC_TimeManager", "ApplyDate", new Param5<int, int, int, int, int>(month, day, year, min, hour), true );
		GetVPPUIManager().DisplayNotification("Applying...Please Wait, sometimes it takes a few seconds to update!");
	}
	
	private void ApplyTimePreset()
	{
		string name = m_TimePresetsInstance.GetText();
		
		if(name == string.Empty || name == "Time Presets")
		{
			GetVPPUIManager().DisplayError("Please select a time preset.");
			return;
		}
		
		GetRPCManager().SendRPC( "RPC_TimeManager", "ApplyPreset", new Param1<string>(name), true );
		GetVPPUIManager().DisplayNotification("Applying Time Preset...Please wait.");
	}
	
	private void DeleteTimePreset()
	{
		string name = m_TimePresetsInstance.GetText();
		
		if(timePresetNames.Find(name) >= 0)
		{
			GetRPCManager().SendRPC( "RPC_TimeManager", "DeleteTimeSetting", new Param1<string>(name), true );
		 	timePresetNames.Remove(timePresetNames.Find(name));
			UpdateTimeSaves();
			GetVPPUIManager().DisplayNotification("Time Preset...Updated");
		}
	}
	
	private void UpdateTimeSaves()
	{
		m_TimePresetsInstance.RemoveAllElements();
		
		foreach(string s : timePresetNames)
		{
			m_TimePresetsInstance.AddElement(s);
		}
	}
	
	private void UpdateWeatherSaves()
	{
		m_WeatherPresetsDropDownInstance.RemoveAllElements();
		
		foreach(string s : weatherPresetNames)
		{
			m_WeatherPresetsDropDownInstance.AddElement(s);
		}
	}
	
	private void GetWeatherValues(out array<float> overcast, out array<float> fog, out array<float> rain, out float wind)
	{
		overcast = {(m_SliderOvercast.GetCurrent() / 100), (m_OvercastRate.GetText().ToFloat()), (m_OvercastDur.GetText().ToFloat())};
		fog = {(m_SliderFog.GetCurrent() / 100), (m_FogRate.GetText().ToFloat()), (m_FogDur.GetText().ToFloat())};
		rain = {(m_SliderRain.GetCurrent() / 100), (m_RainRate.GetText().ToFloat()), (m_RainDur.GetText().ToFloat())};
		wind = m_SliderWindForce.GetCurrent() / 100;
	}
};