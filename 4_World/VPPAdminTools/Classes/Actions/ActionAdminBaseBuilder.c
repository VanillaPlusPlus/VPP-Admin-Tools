class ActionAdminBaseBuilder: ActionSingleUseBase
{
	void ActionAdminBaseBuilder()
	{
		m_CommandUID  	= DayZPlayerConstants.CMD_ACTIONFB_CHECKPULSE;
		m_FullBody 		= true;
		m_StanceMask 	= DayZPlayerConstants.STANCEMASK_ERECT;
		m_Text = "[Admin] #build";
	}

	override void CreateConditionComponents()  
	{
		m_ConditionItem   = new CCINone;
		m_ConditionTarget = new CCTSelf;
	}

	override bool UseMainItem()
	{
		return false;
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}
	
	override bool IsInstant()
	{
		return true;
	}

	override bool HasTarget()
	{
		return true;
	}

	override void OnActionInfoUpdate( PlayerBase player, ActionTarget target, ItemBase item )
	{
		ConstructionActionData construction_action_data = player.GetConstructionActionData();
		ConstructionPart constrution_part = construction_action_data.VPPAT_GetBuildPartAtIndex(m_VariantID);
			
		if (constrution_part)
		{
			m_Text = "[Admin] #build " + constrution_part.GetName();
		}
	}
	
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if (player.IsPlacingLocal() || player.IsPlacingServer())
			return false;
		
		//hack - gate
		if (target.GetObject() && !target.GetObject().CanUseConstructionBuild())
			return false;
		
		if ( (!GetGame().IsDedicatedServer()) )
		{
			//check if admin toggled
			MissionBaseWorld mission = MissionBaseWorld.Cast(GetGame().GetMission());
			if (!mission || !mission.VPPAT_AdminToolsToggled())
				return false;

			ConstructionActionData construction_action_data = player.GetConstructionActionData();
			if (!construction_action_data)
				return false;

			return BuildCheckSimpleClient(player, target, item);
		}
		return true;
	}

	protected bool BuildCheckSimpleClient(PlayerBase player, ActionTarget target, ItemBase item)
	{
		BaseBuildingBase base_building = BaseBuildingBase.Cast( target.GetObject() );
		if (base_building)
		{
			Construction construction = base_building.GetConstruction();
			if (construction && BuildCondition(player, target))
			{
				ConstructionActionData construction_action_data = player.GetConstructionActionData();
				string partName = construction_action_data.VPPAT_GetBuildPartAtIndex(m_VariantID).GetPartName();

				if (player.IsPlacingLocal() || player.IsPlacingServer())
					return false;

				return !construction.IsColliding(partName);
			}
		}
		return false;
	}

	protected bool BuildCondition(PlayerBase player, ActionTarget target)
	{
		if (player && !player.IsLeaning())
		{
			Object targetObject = target.GetObject();
			if (targetObject && targetObject.CanUseConstruction())
			{
				BaseBuildingBase base_building = BaseBuildingBase.Cast( targetObject );
				ConstructionActionData construction_action_data = player.GetConstructionActionData();
				construction_action_data.SetTarget( targetObject );
	
				return (construction_action_data.VPPAT_GetBuildPartAtIndex(m_VariantID) != NULL);
			}
		}
		return false;
	}

	override void OnStartServer(ActionData action_data)
	{
		//permission check
		if (!action_data.m_Player || !action_data.m_Player.GetIdentity())
			return;

		string plainId = action_data.m_Player.GetIdentity().GetPlainId();
		if (!GetPermissionManager().VerifyPermission(plainId, "EspToolsMenu:InstantBaseBuild"))
			return;

		BaseBuildingBase base_building = BaseBuildingBase.Cast(action_data.m_Target.GetObject());
		Construction construction = base_building.GetConstruction();
		
		string part_name = BuildPartActionData.Cast(action_data).m_PartType;

		//reset DamageZone health
		string damage_zone;
		if (DamageSystem.GetDamageZoneFromComponentName(construction.VPPAT_GetParent(), part_name, damage_zone))
		{
			construction.VPPAT_GetParent().SetAllowDamage(true);
			construction.VPPAT_GetParent().SetHealthMax(damage_zone);
			construction.VPPAT_GetParent().ProcessInvulnerabilityCheck(construction.VPPAT_GetParent().GetInvulnerabilityTypeString());
		}
		
		//destroy build collision check trigger
		construction.DestroyCollisionTrigger();

		//call event
		construction.VPPAT_GetParent().OnPartBuiltServer(action_data.m_Player, part_name, AT_BUILD_PART);

		GetSimpleLogger().Log(string.Format("Admin \"%1\" (steamId=%2) built part %3 at position: %4", action_data.m_Player.GetIdentity().GetName(), plainId, part_name, action_data.m_Target.GetObject().GetPosition().ToString()));
	}
	
	override ActionData CreateActionData()
	{
		BuildPartActionData action_data = new BuildPartActionData;
		return action_data;
	}
	
	//setup
	override bool SetupAction( PlayerBase player, ActionTarget target, ItemBase item, out ActionData action_data, Param extra_data = NULL )
	{	
		if ( super.SetupAction( player, target, item, action_data, extra_data ) )
		{
			if (!GetGame().IsDedicatedServer())
			{
				ConstructionActionData construction_action_data = action_data.m_Player.GetConstructionActionData();
				BuildPartActionData.Cast(action_data).m_PartType = construction_action_data.VPPAT_GetBuildPartAtIndex(m_VariantID).GetPartName();
			}
			return true;
		}
		return false;
	}

	override void WriteToContext(ParamsWriteContext ctx, ActionData action_data)
	{
		super.WriteToContext(ctx, action_data);
		
		ctx.Write(BuildPartActionData.Cast(action_data).m_PartType);
	}
	
	override bool ReadFromContext(ParamsReadContext ctx, out ActionReciveData action_recive_data )
	{
		action_recive_data = new BuildPartActionReciveData;
		super.ReadFromContext(ctx, action_recive_data);
		
		string part_type;
		if ( ctx.Read(part_type) )
		{
			BuildPartActionReciveData.Cast( action_recive_data ).m_PartType = part_type;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	override void HandleReciveData(ActionReciveData action_recive_data, ActionData action_data)
	{
		super.HandleReciveData(action_recive_data, action_data);
		
		BuildPartActionData.Cast(action_data).m_PartType = BuildPartActionReciveData.Cast( action_recive_data ).m_PartType;
	}
	
	override string GetAdminLogMessage(ActionData action_data)
	{
		ConstructionActionData construction_action_data = action_data.m_Player.GetConstructionActionData();
		string partName = BuildPartActionData.Cast(action_data).m_PartType;
		
		string message = string.Format("Admin Built %1 on %2", partName, action_data.m_Target.GetObject().GetDisplayName());
		return message;
	}
};