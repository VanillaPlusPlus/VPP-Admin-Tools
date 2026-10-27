/*
	Scale and ragdoll for world entities: players, human AI, animals, infected and buildings (mission-placed and
	static map buildings). Other static map props (trees, rocks) are not EntityAI and are never offered.
	Network ids: 0 = none, a negative high word = static map object (stable id, e.g. vanilla
	cfgundergroundtriggers.json ParentNetworkId), otherwise a dynamic network object.
	- Scale: players / human AI go through the PlayerBase sync (m_VPlayerScale, same steps as
	  PlayerManager.ChangeScale); creatures and buildings through m_VPPEntityScale (modded AnimalBase /
	  ZombieBase / BuildingBase; static buildings are registered with RegisterNetworkStaticObject first).
	  Session only: nothing is saved.
	- Ragdoll: humans only (the engine has no ragdoll for living creatures). Uses the vanilla unconscious
	  sync juncture, the same knockdown vanilla uses for vehicle ejection.
*/
class VPPCA_BodyOps
{
	const static float SCALE_MIN = 0.01;
	const static float SCALE_MAX = 100.0;

	static bool IsDynamic(Object obj)
	{
		if (!obj)
			return false;

		int low;
		int high;
		obj.GetNetworkID(low, high);
		if (high < 0)
			return false;

		return low != 0 || high != 0;
	}

	static bool IsStatic(Object obj)
	{
		if (!obj)
			return false;

		int low;
		int high;
		obj.GetNetworkID(low, high);
		return high < 0;
	}

	static bool IsScalable(Object obj)
	{
		if (!obj)
			return false;

		if (PlayerBase.Cast(obj) != null)
			return IsDynamic(obj);

		EntityAI ent = EntityAI.Cast(obj);
		if (!ent || ent.GetHierarchyParent() != null)
			return false;

		if (AnimalBase.Cast(ent) != null || ZombieBase.Cast(ent) != null)
			return IsDynamic(ent);

		if (BuildingBase.Cast(ent) == null)
			return false;

		return IsDynamic(ent) || IsStatic(ent);
	}

	//-1 = empty or out of range
	static float ParseScale(string input)
	{
		string trimmed = input.Trim();
		if (trimmed == "")
			return -1;

		float scale = trimmed.ToFloat();
		if (scale < SCALE_MIN || scale > SCALE_MAX)
			return -1;

		return scale;
	}

	static float GetCurrentScale(Object obj)
	{
		PlayerBase pb = PlayerBase.Cast(obj);
		if (pb)
		{
			if (pb.m_VScalePlayer && pb.m_VPlayerScale > 0)
				return pb.m_VPlayerScale;
			return 1.0;
		}

		float synced = 0;
		AnimalBase animal = AnimalBase.Cast(obj);
		if (animal)
			synced = animal.VPPGetEntityScale();

		ZombieBase zombie = ZombieBase.Cast(obj);
		if (zombie)
			synced = zombie.VPPGetEntityScale();

		BuildingBase building = BuildingBase.Cast(obj);
		if (building)
			synced = building.VPPGetEntityScale();

		if (synced > 0)
			return synced;

		return obj.GetScale();
	}

	//server
	static void ApplyScale(Object obj, float scale)
	{
		PlayerBase pb = PlayerBase.Cast(obj);
		if (pb)
		{
			pb.m_VScalePlayer = scale != 1.0;
			pb.VPPSetScaleValue(scale);
			pb.SetSynchDirty();
			return;
		}

		AnimalBase animal = AnimalBase.Cast(obj);
		if (animal)
		{
			animal.VPPSetEntityScale(scale);
			return;
		}

		ZombieBase zombie = ZombieBase.Cast(obj);
		if (zombie)
		{
			zombie.VPPSetEntityScale(scale);
			return;
		}

		BuildingBase building = BuildingBase.Cast(obj);
		if (building)
			building.VPPSetEntityScale(scale);
	}

	//server: the online bodies of a player target, or the targeted human (AI or a player the client could not identify)
	static array<PlayerBase> CollectHumans(VPPContextTarget target)
	{
		array<PlayerBase> bodies = new array<PlayerBase>;
		if (!target)
			return bodies;

		if (!target.IsPlayer())
		{
			PlayerBase human = PlayerBase.Cast(target.GetObject());
			if (human)
				bodies.Insert(human);
			return bodies;
		}

		PermissionManager pm = GetPermissionManager();
		if (!pm)
			return bodies;

		array<string> ids = target.GetPlayerIds();
		foreach (string playerId : ids)
		{
			PlayerBase pb = pm.GetPlayerBaseByID(playerId);
			if (pb && bodies.Find(pb) == -1)
				bodies.Insert(pb);
		}

		return bodies;
	}

	//client
	static PlayerBase HumanOf(VPPContextTarget target)
	{
		if (!target)
			return null;

		if (target.IsPlayer())
			return target.GetPlayer();

		return PlayerBase.Cast(target.GetObject());
	}
};

class VPPCA_EntityScale : VPPContextAction
{
	override string GetId()
	{
		return "vpp.entity.scale";
	}

	override int GetOrder()
	{
		return 4100;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_SET_SCALE;
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_SET_SCALE";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_SCALING;
	}

	override string GetInputPrompt(VPPContextTarget target)
	{
		return "#VSTR_CTX_SCALE_PROMPT";
	}

	override bool IsInputNumeric()
	{
		return true;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.IsPlayer())
			return true;

		return VPPCA_BodyOps.IsScalable(target.GetObject());
	}

	//current scale; the server state wins once it arrives (a far player has no local body)
	override string GetHint(VPPContextTarget target)
	{
		if (!target || target.IsMulti())
			return "";

		if (target.HasState(GetId()))
			return target.GetState(GetId());

		Object obj = target.GetObject();
		if (!obj)
			return "";

		float current = VPPCA_BodyOps.GetCurrentScale(obj);
		return VPPContextUtils.FormatScale(current);
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		Object obj = target.GetObject();
		if (!obj)
			return;

		string stateKey = GetId();
		float current = VPPCA_BodyOps.GetCurrentScale(obj);
		string scaleText = VPPContextUtils.FormatScale(current);
		outState.Set(stateKey, scaleText);
	}

	//an empty OK does nothing; a bad number never reaches the server
	override bool OnExecuteClient(VPPContextTarget target, VPPContextArgs args)
	{
		string input = args.GetInput();
		if (input.Trim() == "")
			return true;

		if (VPPCA_BodyOps.ParseScale(input) < 0)
		{
			VPPContextActionManager.ClientNotify("#VSTR_CTX_RESULT_SCALE_RANGE");
			return true;
		}

		return false;
	}

	override bool CanExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args)
	{
		string input = args.GetInput();
		return VPPCA_BodyOps.ParseScale(input) > 0;
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		string input = args.GetInput();
		float scale = VPPCA_BodyOps.ParseScale(input);
		if (scale <= 0)
		{
			result.Fail("#VSTR_CTX_RESULT_SCALE_RANGE");
			return;
		}

		string scaleText = VPPContextUtils.FormatScale(scale);
		if (target.IsPlayer())
		{
			array<PlayerBase> bodies = VPPCA_BodyOps.CollectHumans(target);
			if (bodies.Count() == 0)
			{
				result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
				return;
			}

			foreach (PlayerBase pb : bodies)
			{
				VPPCA_BodyOps.ApplyScale(pb, scale);
			}

			result.Ok("#VSTR_CTX_RESULT_SCALED", scaleText);
			result.LogDetail = "scale " + scaleText;
			return;
		}

		Object obj = target.GetObject();
		if (!VPPCA_BodyOps.IsScalable(obj))
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		VPPCA_BodyOps.ApplyScale(obj, scale);
		result.Ok("#VSTR_CTX_RESULT_SCALED", scaleText);
		string detail = VPPCA_EntityDetail.Describe(obj);
		result.LogDetail = detail + " scale " + scaleText;
	}
};

//Toggle per human: knocked down (vanilla unconscious state) <-> back up.
class VPPCA_EntityRagdoll : VPPContextAction
{
	override string GetId()
	{
		return "vpp.entity.ragdoll";
	}

	override int GetOrder()
	{
		return 4110;
	}

	override int GetKind()
	{
		return EVPPContextKind.TOGGLE;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_RAGDOLL;
	}

	override bool SupportsMultiTarget()
	{
		return true;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_RAGDOLL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_ZAP;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target)
			return false;

		if (target.IsPlayer())
			return true;

		PlayerBase pb = PlayerBase.Cast(target.GetObject());
		if (!pb)
			return false;

		return VPPCA_BodyOps.IsDynamic(pb);
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		if (target.IsMulti())
			return true;

		PlayerBase pb = VPPCA_BodyOps.HumanOf(target);
		if (!pb)
			return target.IsPlayer();

		return pb.IsAlive();
	}

	override bool NeedsServerState(VPPContextTarget target)
	{
		return !target.IsMulti();
	}

	override void OnCollectServerState(PlayerIdentity sender, VPPContextTarget target, map<string,string> outState)
	{
		array<PlayerBase> bodies = VPPCA_BodyOps.CollectHumans(target);
		if (bodies.Count() == 0)
			return;

		string stateKey = GetId();
		bool down = bodies[0].IsUnconscious();
		string stateText = VPPContextUtils.BoolToState(down);
		outState.Set(stateKey, stateText);
	}

	override bool IsChecked(VPPContextTarget target)
	{
		if (target.IsMulti())
			return false;

		if (target.HasState(GetId()))
			return target.GetStateBool(GetId(), false);

		PlayerBase pb = VPPCA_BodyOps.HumanOf(target);
		if (pb)
			return pb.IsUnconscious();

		return false;
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		array<PlayerBase> bodies = VPPCA_BodyOps.CollectHumans(target);
		int changed = 0;
		bool lastDown = false;
		foreach (PlayerBase pb : bodies)
		{
			if (!pb || !pb.IsAlive())
				continue;

			bool down = !pb.IsUnconscious();
			SetDown(pb, down);
			lastDown = down;
			changed++;
		}

		if (changed == 0)
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		result.SetState(lastDown);
		if (lastDown)
			result.Ok("#VSTR_CTX_RESULT_RAGDOLL_ON");
		else
			result.Ok("#VSTR_CTX_RESULT_RAGDOLL_OFF");

		string countText = changed.ToString();
		string downText = VPPContextUtils.BoolToState(lastDown);
		result.LogDetail = "ragdoll " + downText + " x" + countText;
	}

	//waking restores shock first, otherwise a low-shock body would stay down with the modifier still active
	protected void SetDown(PlayerBase pb, bool down)
	{
		if (!down)
		{
			float maxShock = pb.GetMaxHealth("", "Shock");
			pb.SetHealth("", "Shock", maxShock);
		}

		DayZPlayerSyncJunctures.SendPlayerUnconsciousness(pb, down);
	}
};
