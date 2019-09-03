class TimeManager : ConfigurablePlugin
{
	private ref array<ref TimeSetting> timeSettings;
	
	void TimeManager()
	{
		JSONPATH = "$profile:VPPAdminTools/ConfigurablePlugins/TimeManager/TimeSettingsPresets.json";
		timeSettings = new array<ref TimeSetting>;
		
		GetRPCManager().AddRPC("RPC_TimeManager", "SaveTimeSettings", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_TimeManager", "ApplyDate", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_TimeManager", "DeleteTimeSetting", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_TimeManager", "SendToClient", this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("RPC_TimeManager", "ApplyPreset", this, SingleplayerExecutionType.Server);
	}
	
	override void OnInit()
	{
		Load();
	}
	
	override void Load()
	{
		if(GetGame().IsServer() && !GetGame().IsMultiplayer()) return;
		
		MakeDirectory("$profile:VPPAdminTools/ConfigurablePlugins/TimeManager/");
		
		if(FileExist(JSONPATH))
		{
			GetSimpleLogger().Log("[TimeManager]:: Load(): Loading Json File " + JSONPATH);
			JsonFileLoader<TimeManager>.JsonLoadFile(JSONPATH, this);
		}else{
			Save();
		}
	}
	
	override void Save()
	{
		GetSimpleLogger().Log("[TimeManager]:: Save(): Saving to " + JSONPATH);
		JsonFileLoader<TimeManager>.JsonSaveFile(JSONPATH, this);
	}
	
	void SendToClient(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "MenuWeatherManager")) return;
			
			array<string> input = new array<string>;
			foreach(TimeSetting ts : timeSettings)
			{
				input.Insert(ts.GetName());
			}
			
			GetSimpleLogger().Log("[TimeManager]:: SendToClient(): Send Data Count is " + input.Count().ToString());
			GetRPCManager().SendRPC( "RPC_WeatherUI", "HandleTimeData", new Param1<array<string>>(input), true, sender );
		}
	}
	
	void SaveTimeSettings(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "WeatherManager:SaveTimePreset")) return;
			
			Param5<int, int, int, int, int> data;
			
			if(!ctx.Read(data)) return;
			
			int month, day, year;
			int hour, min;
			
			month = data.param1;
			day = data.param2;
			year = data.param3;
			
			min = data.param4;
			hour = data.param5;
			
			autoptr TimeSetting settings = new TimeSetting(hour, min, month, day, year);
			timeSettings.Insert(settings);
			
			GetSimpleLogger().Log("[TimeManager]:: SaveTimeSettings(): Saving setting to " + settings.GetName());
			Save();
		}
	}
	
	void DeleteTimeSetting(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;	
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "WeatherManager:DeleteTimePreset")) return;
			
			string name = data.param1;
			
			foreach(TimeSetting ts : timeSettings)
			{
				if(ts != null && ts.GetName() == name)
				{
					GetSimpleLogger().Log("[TimeManager]:: DeleteTimeSetting(): Deleting time setting " + ts.GetName());
					timeSettings.RemoveItem(ts);
				}
			}
			Save();
		}
	}
	
	void ApplyDate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "WeatherManager:ApplyTime")) return;
			
			Param5<int, int, int, int , int> param;
			if(!ctx.Read(param)) return;
			
			int month = param.param1;
			int day = param.param2;
			int year = param.param3;
			int min = param.param4;
			int hour = param.param5;
			
			GetSimpleLogger().Log("[Time Manager]:: ApplyDate(): Setting Date to " + year.ToString() + "/" + month.ToString() + "/" + day.ToString() + " " + hour.ToString() + ":" + min.ToString());
			GetGame().GetWorld().SetDate(year, month, day, hour, min);
		}
	}
	
	void ApplyPreset(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if(!ctx.Read(data)) return;
		
		if(type == CallType.Server)
		{
			if (!GetPermissionManager().VerifyPermission(sender.GetPlainId(), "WeatherManager:ApplyTimePreset")) return;
			
			string name = data.param1;
			TimeSetting requestedSettings = GetTimePresetByName(name);
			
			if(requestedSettings == null)
			{
				NotificationSystem.SendNotificationToPlayerIdentityExtended(sender, 4, "Error:", "No settings under name: " + name);
				return;
			}
			
			ApplyPreset(requestedSettings);
		}
	}
	
	private TimeSetting GetTimePresetByName(string name)
	{
		foreach(TimeSetting settings : timeSettings)
		{
			if(settings.GetName() == name)
			{
				return settings;
			}
		}
		
		return null;
	}
	
	private void ApplyPreset(TimeSetting s)
	{
		int hour = s.GetHour(), min = s.GetMinute();
		int month = s.GetMonth(), day = s.GetDay(), year = s.GetYear();
		
		GetSimpleLogger().Log("[TimeManager]:: ApplyPreset(): Applying preset " + s.GetName());
		GetGame().GetWorld().SetDate(year, month, day, hour, min);
	}
}