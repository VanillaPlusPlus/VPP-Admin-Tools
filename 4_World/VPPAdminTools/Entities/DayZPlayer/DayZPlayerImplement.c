modded class DayZPlayerImplement
{
	private bool m_VPPFreeCamActive;
	private bool m_VPPSpectateCamActive;

	override int CameraHandler(int pCameraMode)
	{
		if (IsVPPSpectateCamActive())
		{
			return DayZPlayerCameras.VPPAT_SPECTATE_CAMERA;
		}
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

	void SetVPPSpectateCamActive(bool state)
	{
		m_VPPSpectateCamActive = state;
	}

	bool IsVPPSpectateCamActive()
	{
		return m_VPPSpectateCamActive;
	}
};