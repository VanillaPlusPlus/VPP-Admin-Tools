modded class DayZPlayerCameras
{
	static const int VPP_FREE_CAMERA	= 32;		//!< free camera

	static float VPPGetTransitionTime(int pFrom, int pTo)
	{
		int 	key = (pFrom * 65536) + pTo;
		float 	val;

		if (m_TransitionTimes.Find(key, val))
		{
			return val;
		}
		else
		{
			//Init values
			RegisterTransitionTime(DAYZCAMERA_3RD_ERC, VPP_FREE_CAMERA, 0.8, true);
			RegisterTransitionTime(DAYZCAMERA_3RD_ERC_RAISED, VPP_FREE_CAMERA, 0.8, true);
			RegisterTransitionTime(DAYZCAMERA_3RD_ERC_SPR, VPP_FREE_CAMERA, 0.8, true);
			RegisterTransitionTime(DAYZCAMERA_3RD_JUMP, VPP_FREE_CAMERA, 0.8, true);
			RegisterTransitionTime(DAYZCAMERA_3RD_CLIMB, VPP_FREE_CAMERA, 0.8, true);

			RegisterTransitionTime(DAYZCAMERA_3RD_CRO, VPP_FREE_CAMERA, 0.8, true);

			RegisterTransitionTime(DAYZCAMERA_3RD_PRO, VPP_FREE_CAMERA, 0.8, true);
			RegisterTransitionTime(DAYZCAMERA_3RD_PRO_RAISED, VPP_FREE_CAMERA, 0.8, true);

			RegisterTransitionTime(DAYZCAMERA_1ST, VPP_FREE_CAMERA, 0.8, true);

			return 0.8; //default
		}
		return GetTransitionTime(pFrom, pTo);
	}

	override static float OnCameraChange(DayZPlayer pPlayer, int pFrom, int pTo)
	{
		//Print("---transition change---");
		float transTime = GetTransitionTime(pFrom, pTo);;
		if (pFrom == VPP_FREE_CAMERA || pTo == VPP_FREE_CAMERA)
		{
			transTime = VPPGetTransitionTime(pFrom, pTo);
		}
	    //Print("OnCameraChange:" + pFrom.ToString() + "->" + pTo.ToString() + " time:" + transTime.ToString());
		return transTime;		
	}
};