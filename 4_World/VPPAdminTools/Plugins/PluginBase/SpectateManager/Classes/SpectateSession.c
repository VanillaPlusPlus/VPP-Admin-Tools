/*
	Server-only spectate session record (one per spectating admin).
	Deliberately zero net-sync state — everything is server-side or client-local.
*/
class SpectateSession
{
	string m_AdminId;      //steam64 of the spectating admin
	string m_TargetId;     //steam64 of the spectated player
	vector m_ReturnPos;    //admin body position captured at enter (restored on exit)
	vector m_ReturnOri;    //admin body orientation captured at enter
	bool   m_HadGodmode;   //godmode state BEFORE spectate (restored on exit)
	bool   m_WasInvisible; //invisibility state BEFORE spectate (restored on exit)
	bool   m_ClientReady;  //client acked StartSpectate
	int    m_ResendCount;  //StartSpectate resend attempts (bubble race)
	bool   m_TargetDead;      //target died — death-grace countdown running
	float  m_DeathGraceLeft;  //seconds of grace remaining before auto-exit
	int    m_LastSightMode;   //last EVPPSpectateSightMode pushed to the client (edge-trigger)

	void SpectateSession(string adminId, string targetId)
	{
		m_AdminId     = adminId;
		m_TargetId    = targetId;
		m_ClientReady = false;
		m_ResendCount = 0;
		m_TargetDead  = false;
		m_DeathGraceLeft = 0.0;
		m_LastSightMode  = EVPPSpectateSightMode.NONE;
	}
};

//Crash-recovery payload persisted to $profile — maps adminId -> pre-spectate position.
class VPPSpectateRecovery
{
	ref map<string, vector> positions = new map<string, vector>;
};
