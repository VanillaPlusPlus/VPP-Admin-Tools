modded class DayZPlayerImplement
{
	private bool m_VPPFreeCamActive;

	override int CameraHandler(int pCameraMode)
	{
		if (IsFreeCamActive())
		{
			return DayZPlayerCameras.VPP_FREE_CAMERA;
		}
		return super.CameraHandler(pCameraMode);
	}

	void SetFreeCamActive(bool state)
	{
		m_VPPFreeCamActive = state;
	}

	bool IsFreeCamActive()
	{
		return m_VPPFreeCamActive;
	}
};