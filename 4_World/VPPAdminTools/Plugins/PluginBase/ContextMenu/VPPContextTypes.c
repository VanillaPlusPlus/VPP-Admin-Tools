enum EVPPContextKind
{
	ACTION = 0,
	TOGGLE = 1,
	SUBMENU = 2,
	SEPARATOR = 3,
	INFO = 4,
	BACK = 5
};

enum EVPPContextTargetKind
{
	NONE = 0,
	OBJECT = 1,
	PLAYER = 2
};

enum EVPPContextSource
{
	API = 0,
	CROSSHAIR = 1,
	PLAYER_LIST = 2
};

class VPPContextConstants
{
	const static string RPC_SERVER = "RPC_VPPContextMenu";        //documentation only: RPC calls always use string literals
	const static string RPC_CLIENT = "RPC_VPPContextMenuClient";  //documentation only: RPC calls always use string literals

	const static int ORDER_PRIMARY = 100;    //100..999 type-specific rows
	const static int ORDER_GROUP   = 1000;   //1000..1999 submenus
	const static int ORDER_DEFAULT = 4000;
	const static int ORDER_ITEM    = 5000;   //generic item ops
	const static int ORDER_INFO    = 8000;   //copy / info
	const static int ORDER_DANGER  = 9000;   //destructive
	const static int ORDER_MAX     = 999999; //sort keys clamp to [0, ORDER_MAX]
	//auto separator between neighbours whose (Order / 1000) differ

	const static int ORDER_ATTACHMENTS = 1030; //GROUP band

	const static string GROUP_COPY         = "vpp.common.copy";
	const static string ID_UI_BACK         = "vpp.ui.back";
	const static string ID_UI_NONE         = "vpp.ui.none";
	const static string ID_PLAYER_SPECTATE = "vpp.player.spectate";

	const static string ID_ATTACHMENTS      = "vpp.entity.attachments";
	const static string ID_ATTACH_SPAWN     = "vpp.entity.attach_spawn";
	const static string ID_WEAPON_LOAD_MAG  = "vpp.weapon.load_magazine";
	const static string ID_WEAPON_LOAD_AMMO = "vpp.weapon.load_ammo";
	const static string ID_COMMON_DELETE    = "vpp.common.delete";
	const static string PARENT_DYNAMIC      = "vpp.dynamic"; //parent id of actions placed only by OnBuildChildren; no page is ever built for it

	const static string NOTIFY_TITLE = "#VSTR_CTX_NOTIFY_TITLE";
	const static string NOTIFY_ICON  = "set:ccgui_enforce image:MapUserMarker";

	const static string PERM_WEAPON_JAM     = "ContextMenu:WeaponJam";
	const static string PERM_WEAPON_EJECT   = "ContextMenu:WeaponEject";
	const static string PERM_WEAPON_REFILL  = "ContextMenu:WeaponRefill";
	const static string PERM_ITEM_HEALTH    = "ContextMenu:ItemHealth";
	const static string PERM_ITEM_QUANTITY  = "ContextMenu:ItemQuantity";
	const static string PERM_VEHICLE_REFUEL = "ContextMenu:VehicleRefuel";
	const static string PERM_CREATURE_HEAL  = "ContextMenu:CreatureHeal";
	const static string PERM_CREATURE_KILL  = "ContextMenu:CreatureKill";

	const static string PERM_SPAWN_ATTACHMENT = "ContextMenu:SpawnAttachment";
	const static string PERM_WEAPON_LOAD      = "ContextMenu:WeaponLoad";

	const static string ARG_INPUT   = "input";
	const static string STATE_TRUE  = "1";
	const static string STATE_FALSE = "0";

	//attachment pages: target Extras (drill-in pages on CloneTarget) and row Args
	const static string EXTRA_ATT_MODE   = "vpp.att.mode";
	const static string ATT_MODE_SLOT    = "slot";
	const static string ATT_MODE_MAG     = "mag";
	const static string ATT_MODE_AMMO    = "ammo";
	const static string EXTRA_ATT_SLOT   = "vpp.att.slot";
	const static string EXTRA_ATT_MUZZLE = "vpp.att.muzzle";
	const static string EXTRA_ATT_PAGE   = "vpp.att.page";
	const static string ARG_ATT_SLOT     = "slot";
	const static string ARG_ATT_TYPE     = "type";
	const static string ARG_ATT_MUZZLE   = "muzzle";

	const static float MAX_OBJECT_RANGE          = 3000.0;
	const static int   MAX_PERMISSION_QUERY      = 512;
	const static int   MAX_BUILD_DEPTH           = 4;
	const static int   SERVER_ACTION_COOLDOWN_MS = 150;
	const static int   SERVER_STATE_COOLDOWN_MS  = 100;
	const static int   WEAPON_OP_COOLDOWN_MS     = 1000;
	const static int   PERMISSION_REFRESH_MS     = 30000;
	const static int   ATT_PAGE_SIZE             = 40;

	const static string ICON_CHECK_ON     = "set:vpp_ui image:vpp_check_on";
	const static string ICON_CHECK_OFF    = "set:vpp_ui image:vpp_check_off";
	const static string ICON_CHEVRON      = "set:vpp_icons image:chevron_down";   //rotated by the view
	const static string ICON_DOT          = "set:vpp_icons image:dot";
	const static string ICON_INFO         = "set:vpp_icons image:info";
	const static string ICON_USER         = "set:vpp_icons image:user";
	const static string ICON_USERS        = "set:vpp_icons image:users";
	const static string ICON_SWORD        = "set:vpp_icons image:sword";
	const static string ICON_CAR          = "set:vpp_icons image:car";
	const static string ICON_PAW          = "set:vpp_icons image:paw_print";
	const static string ICON_BIOHAZARD    = "set:vpp_icons image:biohazard";
	const static string ICON_PACKAGE      = "set:vpp_icons image:package";
	const static string ICON_PACKAGE_PLUS = "set:vpp_icons image:package_plus";
	const static string ICON_PACKAGE_X    = "set:vpp_icons image:package_x";
	const static string ICON_BOXES        = "set:vpp_icons image:boxes";
	const static string ICON_SHIRT        = "set:vpp_icons image:shirt";
	const static string ICON_APPLE        = "set:vpp_icons image:apple";
	const static string ICON_HOUSE        = "set:vpp_icons image:house";
	const static string ICON_HEART        = "set:vpp_icons image:heart";
	const static string ICON_BANDAGE      = "set:vpp_icons image:bandage";
	const static string ICON_SKULL        = "set:vpp_icons image:skull";
	const static string ICON_ACTIVITY     = "set:vpp_icons image:activity";
	const static string ICON_MOVE         = "set:vpp_icons image:move";
	const static string ICON_LOCATE       = "set:vpp_icons image:locate_fixed";
	const static string ICON_BRING        = "set:vpp_icons image:arrow_down_to_line";
	const static string ICON_RETURN       = "set:vpp_icons image:rotate_ccw";
	const static string ICON_EYE          = "set:vpp_icons image:eye";
	const static string ICON_GAVEL        = "set:vpp_icons image:gavel";
	const static string ICON_MESSAGE      = "set:vpp_icons image:message_square";
	const static string ICON_KICK         = "set:vpp_icons image:log_out";
	const static string ICON_BAN          = "set:vpp_icons image:ban";
	const static string ICON_COPY         = "set:vpp_icons image:copy";
	const static string ICON_MAP_PIN      = "set:vpp_icons image:map_pin";
	const static string ICON_FILE_TEXT    = "set:vpp_icons image:file_text";
	const static string ICON_TRASH        = "set:vpp_icons image:trash_2";
	const static string ICON_HAMMER       = "set:vpp_icons image:hammer";
	const static string ICON_SETTINGS     = "set:vpp_icons image:settings";
	const static string ICON_FLAME        = "set:vpp_icons image:flame";
	const static string ICON_PLUS         = "set:vpp_icons image:plus";
	const static string ICON_MINUS        = "set:vpp_icons image:minus";
	const static string ICON_DROPLETS     = "set:vpp_icons image:droplets";
	const static string ICON_EJECT        = "set:vpp_icons image:arrow_up_from_line";
	const static string ICON_CROSSHAIR    = "set:vpp_icons image:crosshair";
	const static string ICON_LIST         = "set:vpp_icons image:list";
};

//look-mode input capture flag, read by 4_World DayZPlayerCameraFree.CanControl
class VPPContextMenuState
{
	protected static bool s_Capturing;

	static void SetCapturing(bool state)
	{
		s_Capturing = state;
	}

	static bool IsCapturing()
	{
		return s_Capturing;
	}
};
