const int VPP_ADMIN_HUD 		  = 3215465487;
const int DRAG_TEST = 45454545454;


static bool VPPAT_IsModLoadded(string name)
{
	name.ToLower();
	int mod_count = GetGame().ConfigGetChildrenCount("CfgMods");
	for( int i = 2; i < mod_count; i++ )
	{
		string mod_name;
		GetGame().ConfigGetChildName("CfgMods", i, mod_name);
		mod_name.ToLower();
		if (mod_name == name)
			return true;
	}
	return false;
};

class VPPATProfileConstants
{
	//default
	const static float DEFAULT_CAM_SPEED 		 	= 5.0;
	const static float DEFAULT_CAM_BOOST 		 	= 10.0; //multiplier
	const static float DEFAULT_CAM_MOVE_DRAG   		= 0.7;
	const static float DEFAULT_CAM_MOUSE_SENSE 		= 0.6;
	const static float DEFAULT_CAM_SMOOTHNESS  		= 0.444;
	const static float DEFAULT_CAM_FOV 		 		= 1.0;

	//max
	const static float MAX_CAM_SPEED   				= 75.0;
	const static float MAX_CAM_BOOST 				= 30.0;
	const static float MAX_CAM_MOVE_DRAG 			= 1.5;
	const static float MAX_CAM_MOUSE_SENSE 			= 5.0;
	const static float MAX_CAM_SMOOTHNESS  			= 1.0;
	const static float MAX_CAM_FOV 		 			= 3.0;

	//min
	const static float MIN_CAM_SPEED   				= 0.01;
	const static float MIN_CAM_BOOST 				= 0.01;
	const static float MIN_CAM_MOVE_DRAG 			= 0.01;
	const static float MIN_CAM_MOUSE_SENSE 			= 0.01;
	const static float MIN_CAM_SMOOTHNESS  			= 0.01;
	const static float MIN_CAM_FOV 		 			= 0.01;
};