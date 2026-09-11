enum EVPPSpectateView
{
	THIRD_PERSON = 0,
	FIRST_PERSON
};

enum EVPPSpectateEndReason
{
	ADMIN_REQUEST = 0,
	TARGET_DISCONNECTED,
	TARGET_DIED,
	SERVER_FORCED,
	INIT_FAILED
};

//what the spectated target is sighting through (server-maintained, pushed edge-triggered)
enum EVPPSpectateSightMode
{
	NONE = 0,
	IRONSIGHTS,
	OPTICS
};

class VPPSpectateConstants
{
	//server follow tick
	const static float FOLLOW_TICK_SEC       = 1.0;   //cadence of the follow/vitals tick
	const static float FOLLOW_Y_OFFSET       = 0.0;  //admin body parked this far UNDER the target
	const static float FOLLOW_RETELEPORT_DIST = 12.5; //re-teleport only when drift exceeds this
	const static float STREAM_GRACE_SEC      = 2.0;   //wait for the target to stream in on the admin client
	const static int   INIT_MAX_RETRIES      = 3;     //client-side StartSpectate null-object retries

	//target-death grace: how long spectate lingers on the corpse before auto-exit
	const static int   DEATH_GRACE_DEFAULT   = 15;    //seconds (client preset dropdown can override)
	const static int   DEATH_GRACE_MAX       = 300;   //server-side clamp

	//third person (vanilla DayZPlayerCamera3rdPersonErc-derived)
	const static float CAM_3PP_DISTANCE      = 1.6;
	const static float CAM_3PP_SHOULDER      = 0.3;
	const static float CAM_3PP_PIVOT_UP      = 0.25;
	const static float CAM_3PP_MIN_DIST      = 0.4;
	const static float CAM_3PP_MAX_DIST      = 4.0;
	const static float CAM_3PP_ZOOM_STEP     = 0.25;

	//angle smoothing (VPPBR-proven velocity+acceleration filter)
	const static float CAM_SMOOTH_COEF       = 4.3;
	const static float CAM_PITCH_CLAMP       = 85.0;

	//first person
	const static float CAM_1PP_FWD_OFFSET    = -0.04;
	const static float CAM_1PP_FWD_OFFSET_SPRINT = 0.15;
	const static float CAM_1PP_UP_OFFSET     = 0.03;
	const static float CAM_1PP_NEARPLANE     = 0.04;
	const static float CAM_1PP_POS_SMOOTH    = 0.05;  //Math.SmoothCD time constant

	//client exit failsafe
	const static float EXIT_FAILSAFE_SEC     = 5.0;

	//ADS (target aiming down sights)
	const static float ADS_TICK_SEC          = 0.25;  //server sight-mode poll cadence
	const static float CAM_ADS_FOV_SMOOTH    = 0.1;   //FOV SmoothCD time (vanilla ironsights value — the ONLY steady-state ADS filter, matching vanilla)
	const static float CAM_ADS_SMOOTH_COEF   = 12.0;
	//2D-SCOPE PATH ONLY: per-axis SmoothCD time on the eye position (weapon model
	//hidden — nothing rendered to stay rigid with). DEAD for 3D sights.
	const static float CAM_ADS_POS_SMOOTH    = 0.02;
	const static float CAM_ADS_BACK_OFFSET   = 0.05;  //3D sights: eye offset along -sightDir on the RAW weapon basis (near-plane clearance off the optic housing)
	const static float CAM_ADS_RIGHT_OFFSET  = 0.0;   //3D sights: BAKE TARGET — eye offset (m) along the NORMALIZED weapon RIGHT axis (+right / -left). Tune via the spectate-menu debug sliders (ADS_TUNE_UI), then write the found value HERE; VPPSpectateADSTuning below seeds from it so the dynamic default stays in sync
	const static float CAM_ADS_UP_OFFSET     = 0.0;   //3D sights: BAKE TARGET — eye offset (m) along the NORMALIZED weapon UP axis (+up / -down). Same bake flow as RIGHT_OFFSET
	const static float CAM_ADS_CORR_STRENGTH = 1.0;  //master scale (0 = correction off entirely)
	const static float CAM_ADS_CORR_FULL_DEG = 3.0;  //|streamYaw-weaponYaw| at/below this: FULL correction (covers measured 0.1-1.2 deg slow-aim divergence + 1.65-3.3 deg median-hold quantization sawtooth)
	const static float CAM_ADS_CORR_ZERO_DEG = 10.0; //|streamYaw-weaponYaw| at/above this: ZERO correction (under the 12-deg measured fast-turn divergence floor)
	const static bool  ADS_DEBUG             = false;  //[VPPADS] diagnostics in script.log (flip off for release)
	const static bool  ADS_DEBUG_SHOW_BODY   = false;  //DEBUG: keep the admin body VISIBLE while spectating (shows the client-local link; visible to EVERYONE incl. the target — flip off for release)
	const static bool  ADS_TUNE_UI           = false;  //show the ADS trim tuning sliders in the Spectate menu rail (flip off for release; separate from ADS_DEBUG so log spam and tuning UI toggle independently)

	//1PP true-look streaming: the target's REAL camera direction only exists on
	//their client (mouse-look/freelook never replicate) — their client samples and
	//ships it via the server while spectated (VPPBR-proven design, 20Hz)
	const static float LOOK_STREAM_SEC      = 0.05;  //target-side sample/send cadence
	const static int   LOOK_STREAM_STALE_MS = 400;   //fall back to replicated aim beyond this
};

class VPPSpectateADSTuning
{
	static float s_RightOffset = VPPSpectateConstants.CAM_ADS_RIGHT_OFFSET; //m, +right / -left on the normalized weapon basis
	static float s_UpOffset    = VPPSpectateConstants.CAM_ADS_UP_OFFSET;   //m, +up / -down on the normalized weapon basis

	static void Reset()
	{
		s_RightOffset = VPPSpectateConstants.CAM_ADS_RIGHT_OFFSET;
		s_UpOffset    = VPPSpectateConstants.CAM_ADS_UP_OFFSET;
	}
};
