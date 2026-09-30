//DayZCreatureAI covers ZombieBase and AnimalBase.
class VPPCA_CreatureHeal : VPPContextAction
{
	override string GetId()
	{
		return "vpp.creature.heal";
	}

	override int GetOrder()
	{
		return 100;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_CREATURE_HEAL;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_CREATURE_HEAL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_HEART;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return DayZCreatureAI.Cast(target.GetObject()) != null;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		DayZCreatureAI creature = DayZCreatureAI.Cast(target.GetObject());
		if (!creature)
			return false;

		return creature.IsAlive();
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		DayZCreatureAI creature = DayZCreatureAI.Cast(target.GetObject());
		if (!creature)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		if (!creature.IsAlive())
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		creature.SetFullHealth();
		result.Ok("#VSTR_CTX_RESULT_HEALED");
		result.LogDetail = VPPCA_EntityDetail.Describe(creature);
	}
};

class VPPCA_CreatureKill : VPPContextAction
{
	override string GetId()
	{
		return "vpp.creature.kill";
	}

	override int GetOrder()
	{
		return 9100;
	}

	override string GetPermission()
	{
		return VPPContextConstants.PERM_CREATURE_KILL;
	}

	override bool IsDanger(VPPContextTarget target)
	{
		return true;
	}

	override bool RequiresConfirm(VPPContextTarget target)
	{
		return false;
	}

	override string GetLabel(VPPContextTarget target)
	{
		return "#VSTR_CTX_CREATURE_KILL";
	}

	override string GetIcon(VPPContextTarget target)
	{
		return VPPContextConstants.ICON_SKULL;
	}

	override bool IsTargetType(VPPContextTarget target)
	{
		if (!target || target.IsPlayer())
			return false;

		return DayZCreatureAI.Cast(target.GetObject()) != null;
	}

	override bool IsEnabled(VPPContextTarget target)
	{
		DayZCreatureAI creature = DayZCreatureAI.Cast(target.GetObject());
		if (!creature)
			return false;

		return creature.IsAlive();
	}

	override void OnExecuteServer(PlayerIdentity sender, VPPContextTarget target, VPPContextArgs args, VPPContextResult result)
	{
		DayZCreatureAI creature = DayZCreatureAI.Cast(target.GetObject());
		if (!creature)
		{
			result.Fail("#VSTR_CTX_RESULT_TARGET_GONE");
			return;
		}

		if (!creature.IsAlive())
		{
			result.Fail("#VSTR_CTX_RESULT_INVALID");
			return;
		}

		//vanilla BarbedWireTrigger precedent
		creature.SetHealth("", "", 0);
		result.Ok("#VSTR_CTX_RESULT_KILLED");
		result.LogDetail = VPPCA_EntityDetail.Describe(creature);
	}
};
