class VPPContextUtils
{
	//PLAYERS_CLIENT is keyed by the HASHED identity id
	static VPPUser GetUserForPlayer(Man man)
	{
		if (!man || !man.GetIdentity() || !PlayerListManager.PLAYERS_CLIENT)
			return null;
		return PlayerListManager.PLAYERS_CLIENT.Get(man.GetIdentity().GetId());
	}

	//PlayerBase.VPlayerGetSteamId() is server-only; clients go through the synced player list
	static string GetSteam64ForPlayer(Man man)
	{
		if (!man)
			return "";

		if (GetGame().IsServer())
		{
			PlayerBase pb = PlayerBase.Cast(man);
			if (pb && pb.VPlayerGetSteamId() != "")
				return pb.VPlayerGetSteamId();
			if (man.GetIdentity())
				return man.GetIdentity().GetPlainId();
			return "";
		}

		VPPUser user = GetUserForPlayer(man);
		if (user)
			return user.GetUserId();
		return "";
	}

	static int GetSessionIdForPlayer(Man man)
	{
		if (!man)
			return -1;

		if (GetGame().IsServer())
		{
			if (man.GetIdentity())
				return man.GetIdentity().GetPlayerId();
			return -1;
		}

		VPPUser user = GetUserForPlayer(man);
		if (user)
			return user.GetSessionId();
		return -1;
	}

	//null when the player is not inside the network bubble
	static PlayerBase FindPlayerBySteam64Client(string steam64)
	{
		if (steam64 == "")
			return null;

		//ClientData.m_PlayerBaseList does not reliably contain the local player
		Man localMan = GetGame().GetPlayer();
		if (localMan && GetSteam64ForPlayer(localMan) == steam64)
			return PlayerBase.Cast(localMan);

		if (!ClientData.m_PlayerBaseList)
			return null;

		foreach (Man m : ClientData.m_PlayerBaseList)
		{
			if (m && GetSteam64ForPlayer(m) == steam64)
				return PlayerBase.Cast(m);
		}
		return null;
	}

	static string GetObjectDisplayName(Object obj)
	{
		if (!obj)
			return "#VSTR_CTX_OBJECT";

		EntityAI ent = EntityAI.Cast(obj);
		if (ent)
		{
			string displayName = ent.GetDisplayName();
			if (displayName != "")
				return displayName;
		}

		string typeName = obj.GetType();
		if (typeName != "")
			return typeName;

		return "#VSTR_CTX_OBJECT";
	}

	static string GetTargetIcon(VPPContextTarget target)
	{
		if (!target)
			return VPPContextConstants.ICON_INFO;
		if (target.IsMulti())
			return VPPContextConstants.ICON_USERS;
		if (target.IsPlayer())
			return VPPContextConstants.ICON_USER;

		Object obj = target.GetObject();
		if (!obj)
			return VPPContextConstants.ICON_INFO;

		if (Weapon_Base.Cast(obj))
			return VPPContextConstants.ICON_SWORD;
		if (Transport.Cast(obj))
			return VPPContextConstants.ICON_CAR;
		if (DayZInfected.Cast(obj))
			return VPPContextConstants.ICON_BIOHAZARD;
		if (DayZAnimal.Cast(obj))
			return VPPContextConstants.ICON_PAW;
		if (Clothing.Cast(obj))
			return VPPContextConstants.ICON_SHIRT;
		if (Edible_Base.Cast(obj))
			return VPPContextConstants.ICON_APPLE;
		if (ItemBase.Cast(obj))
			return VPPContextConstants.ICON_PACKAGE;
		if (Building.Cast(obj))
			return VPPContextConstants.ICON_HOUSE;

		return VPPContextConstants.ICON_INFO;
	}

	static bool IsAliveMan(Object obj)
	{
		Man man = Man.Cast(obj);
		if (!man)
			return false;
		return man.IsAlive();
	}

	//ExplosivesBase covers Grenade_Base
	static bool IsExplosive(Object obj)
	{
		return ExplosivesBase.Cast(obj) != null;
	}

	static string BoolToState(bool value)
	{
		if (value)
			return VPPContextConstants.STATE_TRUE;
		return VPPContextConstants.STATE_FALSE;
	}

	static bool StateToBool(string value, bool fallback)
	{
		if (value == VPPContextConstants.STATE_TRUE || value == "true")
			return true;
		if (value == VPPContextConstants.STATE_FALSE || value == "false")
			return false;
		return fallback;
	}

	//x, y, z with 2 decimals; part of GetKey(), so it must be deterministic on client and server
	static string FormatPosition(vector pos)
	{
		string text = FormatFixed2(pos[0]) + ", " + FormatFixed2(pos[1]);
		text = text + ", " + FormatFixed2(pos[2]);
		return text;
	}

	//fixed 2-decimal formatting; float.ToString() has no precision argument and drops trailing zeros
	protected static string FormatFixed2(float value)
	{
		int scaled = Math.Round(value * 100);
		string sign = "";
		if (scaled < 0)
		{
			sign = "-";
			scaled = -scaled;
		}

		int whole = scaled / 100;
		int frac = scaled % 100;
		return sign + whole.ToString() + "." + frac.ToStringLen(2);
	}

	static int NowMs()
	{
		return GetGame().GetTime();
	}
};
