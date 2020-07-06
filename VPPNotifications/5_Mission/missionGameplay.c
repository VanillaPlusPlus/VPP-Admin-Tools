#ifndef VPPNOTIFICATIONS
modded class MissionGameplay
{
	private ref VPPNotificationUI m_NotificationUI;

	void MissionGameplay()
	{
		m_NotificationUI = new VPPNotificationUI();
	}
	
	void ~MissionGameplay()
	{
		if (m_NotificationUI != null)
			delete m_NotificationUI;
	}
}
#endif