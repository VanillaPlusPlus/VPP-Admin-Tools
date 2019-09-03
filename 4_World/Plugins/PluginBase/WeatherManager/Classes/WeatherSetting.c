class WeatherSetting
{
	private string name;

	private ref array<float> overcastActual;
	private ref array<float> rainActual;
	private ref array<float> fogActual;
	private float windForceActual;
	
	
	void WeatherSetting(array<float> overrcast, array<float> rain, array<float> fog, float wind, string n)
	{
		overcastActual = overrcast;
		rainActual = rain;
		fogActual = fog;
		windForceActual = wind;
		name = n;
	}
	
	float GetOverrcastActual()
	{
		return overcastActual[0];
	}
	
	float GetOverrcastRate()
	{
		return overcastActual[1];
	}
	
	float GetOverrcastDur()
	{
		return overcastActual[2];
	}
	
	float GetRainActual()
	{
		return rainActual[0];
	}
	
	float GetRainRate()
	{
		return rainActual[1];
	}
	
	float GetRainDur()
	{
		return rainActual[2];
	}

	float GetFogActual()
	{
		return fogActual[0];
	}
	
	float GetFogRate()
	{
		return fogActual[1];
	}
	
	float GetFogDur()
	{
		return fogActual[2];
	}
	
	float GetWindActual()
	{
		return windForceActual;
	}
	
	string GetName()
	{
		return name;
	}
}