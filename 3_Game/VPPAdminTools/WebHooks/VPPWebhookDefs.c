/*
	Webhook events and the variables each one offers to templates. Shared (3_Game) so the server renders and validates
	with them and the admin client can list them (variable reference, previews).

	Every event also gets the common variables (server, players, time, event id). A variable's Sample is what previews
	and validation render with.
*/
class VPPWebhookVar : Managed
{
	string Name;
	string Sample;

	void VPPWebhookVar(string name, string sample)
	{
		Name = name;
		Sample = sample;
	}
};

class VPPWebhookEventDef : Managed
{
	string Id;
	// legacy switch this event followed in old configs ("" = none): death, admin, hit, joinleave, status
	string LegacyGroup;
	ref array<ref VPPWebhookVar> Vars;

	void VPPWebhookEventDef(string id, string legacyGroup)
	{
		Id = id;
		LegacyGroup = legacyGroup;
		Vars = new array<ref VPPWebhookVar>();
	}

	void AddVar(string name, string sample)
	{
		Vars.Insert(new VPPWebhookVar(name, sample));
	}

	bool HasVar(string name)
	{
		foreach (VPPWebhookVar webhookVar : Vars)
		{
			if (webhookVar.Name == name)
			{
				return true;
			}
		}

		return false;
	}

	// The sample values of every variable (common ones included), for previews and validation.
	void FillSamples(map<string, string> outVars)
	{
		foreach (VPPWebhookVar webhookVar : Vars)
		{
			outVars.Set(webhookVar.Name, webhookVar.Sample);
		}
	}
};

class VPPWebhookDefs
{
	const static string EV_ADMIN = "admin.action";
	const static string EV_KILL = "kill";
	const static string EV_HIT = "hit";
	const static string EV_JOIN = "join";
	const static string EV_LEAVE = "leave";
	const static string EV_LEAVE_EARLY = "leave.early";
	const static string EV_LOGOUT_START = "logout.start";
	const static string EV_LOGOUT_CANCEL = "logout.cancel";
	const static string EV_STATUS = "server.status";
	const static string EV_BOOT = "server.boot";

	const static string GROUP_DEATH = "death";
	const static string GROUP_ADMIN = "admin";
	const static string GROUP_HIT = "hit";
	const static string GROUP_JOINLEAVE = "joinleave";
	const static string GROUP_STATUS = "status";

	protected static ref array<ref VPPWebhookEventDef> s_Defs;

	static array<ref VPPWebhookEventDef> All()
	{
		Ensure();
		return s_Defs;
	}

	static VPPWebhookEventDef Get(string eventId)
	{
		Ensure();
		foreach (VPPWebhookEventDef def : s_Defs)
		{
			if (def.Id == eventId)
			{
				return def;
			}
		}

		return null;
	}

	protected static void Ensure()
	{
		if (s_Defs)
		{
			return;
		}

		s_Defs = new array<ref VPPWebhookEventDef>();
		VPPWebhookEventDef def = NewDef(EV_ADMIN, GROUP_ADMIN);
		def.AddVar("admin.name", "SurvivorAdmin");
		def.AddVar("admin.id", "76561198000000000");
		def.AddVar("action", "[ItemManager] Spawned Item: M4A1 on self");
		def.AddVar("action.text", "Spawned Item: M4A1 on self");
		def.AddVar("module", "ItemManager");

		def = NewDef(EV_KILL, GROUP_DEATH);
		def.AddVar("victim.name", "Survivor");
		def.AddVar("victim.id", "76561198000000001");
		def.AddVar("victim.pos", "7500.1, 290.5, 7700.2");
		def.AddVar("killer.name", "Bandit");
		def.AddVar("killer.id", "76561198000000002");
		def.AddVar("killer.pos", "7560.3, 291.0, 7712.8");
		def.AddVar("cause", "player");
		def.AddVar("weapon", "M4-A1");
		def.AddVar("distance", "61.4");
		def.AddVar("details", "Survivor (7500, 7700) killed by: Bandit (7560, 7712) with [M4-A1] from [61.4] meters");

		def = NewDef(EV_HIT, GROUP_HIT);
		def.AddVar("victim.name", "Survivor");
		def.AddVar("victim.id", "76561198000000001");
		def.AddVar("victim.pos", "7500.1, 290.5, 7700.2");
		def.AddVar("source.name", "Bandit");
		def.AddVar("source.id", "76561198000000002");
		def.AddVar("source.kind", "player");
		def.AddVar("weapon", "M4-A1");
		def.AddVar("ammo", "Bullet_556x45");
		def.AddVar("zone", "Torso");
		def.AddVar("distance", "61.4");
		def.AddVar("health", "72.5");
		def.AddVar("details", "Survivor (7500, 7700) hit by Bandit (7560, 7712) into Torso(72.5 HP left) with (M4-A1) from (61.4) meters");

		AddPlayerEvent(EV_JOIN, "joined the server!");
		AddPlayerEvent(EV_LEAVE, "left the server!");
		AddPlayerEvent(EV_LEAVE_EARLY, "disconnected early from server (EXIT NOW).");
		AddPlayerEvent(EV_LOGOUT_START, "initiated disconnect process..");
		AddPlayerEvent(EV_LOGOUT_CANCEL, "canceled logout");

		def = NewDef(EV_STATUS, GROUP_STATUS);
		def.AddVar("server.fps", "58");
		def.AddVar("uptime", "0d 5h 12m 40s");
		def.AddVar("uptime.seconds", "18760");

		NewDef(EV_BOOT, GROUP_STATUS);

		foreach (VPPWebhookEventDef common : s_Defs)
		{
			AddCommonVars(common);
		}
	}

	protected static VPPWebhookEventDef NewDef(string eventId, string legacyGroup)
	{
		VPPWebhookEventDef def = new VPPWebhookEventDef(eventId, legacyGroup);
		s_Defs.Insert(def);
		return def;
	}

	protected static void AddPlayerEvent(string eventId, string actionSample)
	{
		VPPWebhookEventDef def = NewDef(eventId, GROUP_JOINLEAVE);
		def.AddVar("player.name", "Survivor");
		def.AddVar("player.id", "76561198000000001");
		def.AddVar("action", actionSample);
	}

	protected static void AddCommonVars(VPPWebhookEventDef def)
	{
		def.AddVar("event", def.Id);
		def.AddVar("server.name", "My DayZ Server");
		def.AddVar("server.ip", "203.0.113.10");
		def.AddVar("server.port", "27016");
		def.AddVar("server.map", "chernarusplus");
		def.AddVar("players", "12");
		def.AddVar("time.iso", "2026-09-29T18:04:00.000Z");
		def.AddVar("time.unix", "1790705040");
		def.AddVar("time.date", "2026-09-29");
		def.AddVar("time.time", "18:04:00");
	}

	// "hook.*" variables come from the webhook's own custom variables; they are always allowed in templates.
	static bool IsHookVar(string name)
	{
		return name.IndexOf("hook.") == 0;
	}
};

// What an admin may do in the webhooks menu (the server sends the mask with the list; the server checks again).
class VPPWebhookAccess
{
	const static int CREATE = 1;
	const static int EDIT = 2;
	const static int DELETE = 4;
	const static int EDIT_TEMPLATES = 8;
	const static int TEST_SEND = 16;
	const static int VIEW_URL = 32;
};

class VPPWebhookVarNames
{
	// Letters, digits, "_" and "." (template variable and hook.* names).
	static bool IsPlain(string name)
	{
		int total = name.Length();
		int i = 0;
		int code = 0;
		string ch = "";
		if (total == 0)
		{
			return false;
		}

		for (i = 0; i < total; i++)
		{
			ch = name.Get(i);
			code = ch.ToAscii();
			if (code >= 97 && code <= 122)
			{
				continue;
			}

			if (code >= 65 && code <= 90)
			{
				continue;
			}

			if (code >= 48 && code <= 57)
			{
				continue;
			}

			if (ch != "." && ch != "_")
			{
				return false;
			}
		}

		return true;
	}
};
