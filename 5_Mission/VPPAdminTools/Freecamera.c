vector VPPGetCurrentCameraPosition()
{
	vector mat[4];
	GetCamera(0, mat);
	return mat[3];
}

static bool IsFreeCamActive()
{
	DayZPlayerImplement player = DayZPlayerImplement.Cast(GetGame().GetPlayer());
	if (!player)
		return false;

	DayZPlayerCameraFree cam = DayZPlayerCameraFree.Cast(PlayerBase.Cast(GetGame().GetPlayer()).GetCurrentPlayerCamera());
	if (!cam)
		return false;

    return player.IsFreeCamActive();
}

//not used anymore
static VPPFreeCam GetFreeCamInstance()
{
	return NULL;
}