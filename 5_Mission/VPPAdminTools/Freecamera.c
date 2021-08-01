ref VPPFreeCam m_freecam;
bool m_FreeCamStatus;
static void CreateFreeCamInstance()
{
	vector pos;
	MiscGameplayFunctions.GetHeadBonePos(PlayerBase.Cast(GetGame().GetPlayer()), pos);
	m_freecam = VPPFreeCam.Cast(GetGame().CreateObjectEx("VPPFreeCam", pos, ECE_LOCAL));
	m_freecam.SetActive(true);
	m_FreeCamStatus = true;
	GetGame().GetMission().GetHud().Show(false);
	GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
}

static bool IsFreeCamActive()
{
	return m_FreeCamStatus;
}

static VPPFreeCam GetFreeCamInstance()
{
	return m_freecam;
}

static void DestroyFreeCam()
{
	if (m_freecam == null) return;
	m_freecam.SetActive(false);
	m_FreeCamStatus = false;
	
	VPPAdminHud adminMenu;
	if (Class.CastTo(adminMenu,GetVPPUIManager().GetMenuByType(VPPAdminHud)))
	{
		if (adminMenu != null)
			adminMenu.HideMenu();
	}

	GetGame().ObjectDelete(m_freecam);
	GetGame().SelectPlayer(null,GetGame().GetPlayer());
	GetGame().GetMission().GetHud().Show( true );
	
	PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
	if(player)
	{
		DayZPlayerCameraBase cam = player.GetCurrentPlayerCamera();
		if (cam)
			cam.ResetAngles();
	}
	
	GetGame().GetInput().ResetGameFocus();
	GetGame().GetUIManager().ShowCursor(false);
	GetGame().GetMission().PlayerControlEnable(false);
}