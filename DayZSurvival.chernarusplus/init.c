#include "$CurrentDir:\\mpmissions\\DayZSurvival.chernarusplus\\ScriptedMods\\Modules\\ServerMission\\DayZSurvival.c"

Mission CreateCustomMission(string path)
{
	return new DayZSurvival();
}


void main()
{
	//INIT WEATHER BEFORE ECONOMY INIT------------------------
	Weather weather = g_Game.GetWeather();
	weather.MissionWeather(false);    // false = use weather controller from Weather.c
	weather.GetOvercast().Set( Math.RandomFloatInclusive(0.4, 0.6), 1, 0);
	weather.GetRain().Set( 0, 0, 1);
	weather.GetFog().Set( Math.RandomFloatInclusive(0.05, 0.1), 1, 0);

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year, month, day, hour, minute;
	int reset_month = 9, reset_day = 20;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

    if ((month == reset_month) && (day < reset_day))
    {
        GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
    }
    else
    {
        if ((month == reset_month + 1) && (day > reset_day))
        {
            GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
        }
        else
        {
            if ((month < reset_month) || (month > reset_month + 1))
            {
                GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
            }
        }
    }
}
