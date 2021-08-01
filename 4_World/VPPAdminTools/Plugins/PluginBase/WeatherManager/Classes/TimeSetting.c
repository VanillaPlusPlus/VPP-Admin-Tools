class TimeSetting
{
	private int hour, minute;
	private int month, day, year;
	private string name;
	
	void TimeSetting(int h, int min, int mon, int d, int y)
	{
		hour = h;
		minute = min;
		month = mon;
		day = d;
		year = y;
		
		if(minute > 10)
			name = month.ToString() + "/" + day.ToString() + "/" + year.ToString() + " " + hour.ToString() + ":" + minute.ToString();
		else
			name = month.ToString() + "/" + day.ToString() + "/" + year.ToString() + " " + hour.ToString() + ":0" +  minute.ToString();
	
	}
	
	string GetName()
	{
		return name;
	}
	
	int GetMonth()
	{
		return month;
	}
	
	int GetDay()
	{
		return day;
	}
	
	int GetYear()
	{
		return year;
	}
	
	int GetHour()
	{
		return hour;
	}
	
	int GetMinute()
	{
		return minute;
	}
}