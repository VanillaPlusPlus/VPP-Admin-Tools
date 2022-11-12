bool VPPAT_FORCE_WIND_OVERRIDE = false;
float VPPAT_WIND_OVERRIDE_VAL;

modded class ChernarusPlusData
{
	override bool WeatherOnBeforeChange(EWeatherPhenomenon type, float actual, float change, float time)
	{
		bool superFirst = super.WeatherOnBeforeChange(type, actual, change, time);
		if (VPPAT_FORCE_WIND_OVERRIDE)
		{
			Weather w = GetGame().GetWeather();
			w.SetWindFunctionParams(VPPAT_WIND_OVERRIDE_VAL, VPPAT_WIND_OVERRIDE_VAL, -1);
			w.SetWindSpeed(VPPAT_WIND_OVERRIDE_VAL);
		}
		return superFirst;
	}
};

modded class EnochData
{
	override bool WeatherOnBeforeChange(EWeatherPhenomenon type, float actual, float change, float time)
	{
		bool superFirst = super.WeatherOnBeforeChange(type, actual, change, time);
		if (VPPAT_FORCE_WIND_OVERRIDE)
		{
			Weather w = GetGame().GetWeather();
			w.SetWindFunctionParams(VPPAT_WIND_OVERRIDE_VAL, VPPAT_WIND_OVERRIDE_VAL, -1);
			w.SetWindSpeed(VPPAT_WIND_OVERRIDE_VAL);
		}
		return superFirst;
	}
};