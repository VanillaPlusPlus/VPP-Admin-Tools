modded class DayZPlayerCameraBase
{
	void ResetAngles();
};

// *************************************************************************************
// ! DayZPlayerCamera1stPerson - 1st person camera
// *************************************************************************************
modded class DayZPlayerCamera1stPerson
{
	override void ResetAngles()
	{
		m_fUpDownAngle		= 0; 
		m_fLeftRightAngle	= 0; 
		m_fUpDownAngleAdd	= 0;
	}
};

// *************************************************************************************
// ! DayZPlayerCamera1stPerson - 3rd person camera
// *************************************************************************************
modded class DayZPlayerCamera3rdPerson 
{
	override void ResetAngles()
	{
		m_fUpDownAngle		= 0; 
		m_fLeftRightAngle	= 0; 
		m_fUpDownAngleAdd	= 0;
	}
};