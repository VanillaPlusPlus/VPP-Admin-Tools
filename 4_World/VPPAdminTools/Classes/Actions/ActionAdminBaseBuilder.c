class ActionAdminBaseBuilder: ActionSingleUseBase
{
	void ActionAdminBaseBuilder()
	{
		m_CommandUID  	= DayZPlayerConstants.CMD_ACTIONFB_CHECKPULSE;
		m_FullBody 		= true;
		m_StanceMask 	= DayZPlayerConstants.STANCEMASK_ERECT;
	}

	override void CreateConditionComponents()  
	{
		m_ConditionItem   = new CCINone;
		m_ConditionTarget = new CCTSelf;
	}

	override typename GetInputType()
	{
		return ContinuousDefaultActionInput;
	}

	override bool IsInstant()
	{
		return true;
	}

	override string GetText()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if ( player )
		{
			ConstructionActionData construction_action_data = player.GetConstructionActionData();
			ConstructionPart constrution_part = construction_action_data.GetCurrentBuildPart();
			
			if ( constrution_part )
			{
				return "[Admin] #build" + " " + constrution_part.GetName();
			}
		}
		return "";
	}

	override bool UseMainItem()
	{
		return false;
	}

	override bool HasTarget()
	{
		return true;
	}
	
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if (player.IsPlacingLocal() || player.IsPlacingServer())
			return false;
		
		//hack - gate
		if (target.GetObject() && (!target.GetObject().CanUseConstructionBuild() || target.GetObject().CanUseHandConstruction()))
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

			int start_index = construction_action_data.m_PartIndex;
			if (construction_action_data.GetConstructionPartsCount() > 0)
			{
				for (int i = 0; i < construction_action_data.GetConstructionPartsCount(); i++)
				{
					if (BuildCheckSimpleClient(player, target, item))
					{
						return true;
					}
					else
					{
						construction_action_data.SetNextIndex();
					}
				}
				construction_action_data.m_PartIndex = start_index;
			}
			return false;
		}
		return true;
	}

	protected bool BuildCheckSimpleClient(PlayerBase player, ActionTarget target, ItemBase item, string partName = "")
	{
		BaseBuildingBase base_building = BaseBuildingBase.Cast(target.GetObject());
		if (base_building)
		{
			Construction construction = base_building.GetConstruction();
			if (construction && BuildCondition(player, target, item))
			{
				ConstructionActionData construction_action_data = player.GetConstructionActionData();
				if (partName == "")
					partName = construction_action_data.GetCurrentBuildPart().GetPartName();

				if (player.IsPlacingLocal() || player.IsPlacingServer())
					return false;

				return !construction.IsColliding(partName);
			}
		}
		return false;
	}

	protected bool BuildCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{	
		if (player && !player.IsLeaning())
		{
			Object targetObject = target.GetObject();
			if (targetObject && targetObject.CanUseConstruction())
			{
				BaseBuildingBase base_building = BaseBuildingBase.Cast( targetObject );
				ConstructionActionData construction_action_data = player.GetConstructionActionData();
				construction_action_data.SetTarget( targetObject );
				
				string main_part_name = targetObject.GetActionComponentName( target.GetComponentIndex() );
				
				if (GetGame().IsMultiplayer() || GetGame().IsServer())
				{
					construction_action_data.VPPAT_RefreshPartsToBuild(main_part_name, item);
				}
				ConstructionPart constrution_part = construction_action_data.GetCurrentBuildPart();
				return constrution_part != NULL;
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
		if (super.SetupAction( player, target, item, action_data, extra_data ))
		{
			if (!GetGame().IsDedicatedServer())
			{
				ConstructionActionData construction_action_data = action_data.m_Player.GetConstructionActionData();
				BuildPartActionData.Cast(action_data).m_PartType = construction_action_data.GetCurrentBuildPart().GetPartName();
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
	
	void SetNextIndex(ActionData action_data)
	{
		ConstructionActionData construction_action_data = action_data.m_Player.GetConstructionActionData();
		construction_action_data.SetNextIndex();
	}
};