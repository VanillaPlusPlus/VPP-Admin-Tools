class WeatherManager : ConfigurablePlugin
{
	private ref array<ref WeatherSetting> weatherSettings;
	
	void WeatherManager()
	{
		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/WeatherManager/WeatherSettingsPresets.json";
		weatherSettings = new array<ref WeatherSetting>;
		
		GetRPCManager().AddRPC("RPC_WeatherManager", "ApplyWeather", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WeatherManager", "AddWeatherSettingToServer", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WeatherManager", "RemoveWeatherSettingsFromServer", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WeatherManager", "SendToClient", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_WeatherManager", "ApplyPreset", this, SingleplayerExecutionType.Server);
	}
	
	override void OnInit()
	{
		Load();
	}
	
	override void Load()
	{
		if(GetGame().IsServer() && !GetGame().IsMultiplayer()) return;
		
		MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins/WeatherManager");
		
		if(FileExist(JSONPATH))
		{
			GetSimpleLogger().Log("[WeatherManager]:: Load(): Loading Json File " + JSONPATH);
			JsonFileLoader<WeatherManager>.JsonLoadFile(JSONPATH, this);
		}else{
			GetSimpleLogger().Log("[WeatherManager]:: Load(): Creating Default Json File " + JSONPATH);
			Save();
		}
	}
	
	override void Save()
	{
		GetSimpleLogger().Log("[WeatherManager]:: Save() to " + JSONPATH);
		JsonFileLoader<WeatherManager>.JsonSaveFile(JSONPATH, this);
	}
	
	void SendToClient(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"MenuWeatherManager")) return;
			
			array<string> input = new array<string>;
			foreach(WeatherSetting ws : weatherSettings)
			{
				input.Insert(ws.GetName());
			}
			
			GetSimpleLogger().Log("[WeatherManager]:: SendToClient(): Send Data Count is " + input.Count().ToString());
			GetRPCManager().SendRPC( "RPC_WeatherUI", "HandleWeatherData", new Param1<array<string>>(input), true, sender );
		}
	}
	
	void AddWeatherSettingToServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param5<array<float>, array<float>, array<float>, float, string> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"WeatherManager:SavePreset")) return;
			
			autoptr array<float> overcast = data.param1;
			autoptr array<float> fog = data.param2;
			autoptr array<float> rain = data.param3;
			
			float wind = data.param4;
			
			string name = data.param5;
			
			weatherSettings.Insert(new WeatherSetting(overcast, rain, fog, wind, name));
			Save();
		}
	}
	
	void RemoveWeatherSettingsFromServer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"WeatherManager:DeletePreset")) return;
			
			string weatherName = data.param1;
			
			foreach(WeatherSetting ws : weatherSettings)
			{
				if(ws.GetName() == weatherName)
				{
					weatherSettings.RemoveItem(ws);
				}
			}
			Save();
		}
	}
	
	void ApplyPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"WeatherManager:ApplyPreset")) return;
			
			string name = data.param1;
			autoptr WeatherSetting requestedSetting = GetSettingByName(name);
			
			if(requestedSetting == null){
				NotificationSystem.SendNotificationToPlayerIdentityExtended(sender, 4, "Error:", "No settings under name: " + name);
				return;
			}
			
			ApplyWeatherSetting(requestedSetting);
		}
	}
	
	private void ApplyWeatherSetting(WeatherSetting s)
	{
		float overcast = s.GetOverrcastActual(), overcastRate = s.GetOverrcastRate(), overcastDur = s.GetOverrcastDur();
		float fog = s.GetFogActual(), fogRate = s.GetFogRate(), fogDur = s.GetFogDur();
		float rain = s.GetRainActual(), rainRate = s.GetRainRate(), rainDur = s.GetRainDur();
		float wind = s.GetWindActual();
	
		GetSimpleLogger().Log("[Weather Manager]:: ApplyWeatherSetting() to: " + s.GetName());

		Weather w = GetGame().GetWeather();
		Overcast o = w.GetOvercast();
		Fog f = w.GetFog();
		Rain r = w.GetRain();
		
		w.SetWindSpeed(wind);
		w.SetWindFunctionParams(0.9, 1.0, 100.0);
		o.Set(overcast, overcastRate, overcastDur);
		f.Set(fog,fogRate,fogDur);
		r.Set(rain,rainRate,rainDur);
	}

	void ApplyWeather(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(),"WeatherManager:ApplyWeather")) return;
			
			Param4<array<float>, array<float>, array<float>, float> data;
			if(!ctx.Read(data)) return;
			
			autoptr array<float> overcast = data.param1;
			autoptr array<float> fog = data.param2;
			autoptr array<float> rain = data.param3;
			
			float wind = data.param4;
			
			GetSimpleLogger().Log("[Weather Manager]:: ApplyWeather()");
			
			Weather w = GetGame().GetWeather();
			Overcast o = w.GetOvercast();
			Fog f = w.GetFog();
			Rain r = w.GetRain();
			
			w.SetWindSpeed(wind);
			w.SetWindFunctionParams(0.9, 1.0, 100.0);
			o.Set(overcast[0], overcast[1], overcast[2]);
			f.Set(fog[0],fog[1],fog[2]);
			r.Set(rain[0],rain[1],rain[2]);
		}
	}
	
	private WeatherSetting GetSettingByName(string name)
	{
		foreach(WeatherSetting setting : weatherSettings)
		{
			if(setting.GetName() == name)
			{
				return setting;
			}
		}
		return null;
	}
}