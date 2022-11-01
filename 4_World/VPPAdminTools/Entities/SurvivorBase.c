modded class SurvivorBase
{
	static const int FLAGS_INVIS = EntityFlags.VISIBLE|EntityFlags.SOLID|EntityFlags.ACTIVE;
	private float m_defualt_scale;
	private bool  m_setLocalScale;
	private bool  m_EntityYeeted;

	float m_HealthClient;
	float m_BloodClient;

	const int TYPE_HEALTH 	= 1;
   	const int TYPE_BLOOD 	= 2;

	void SurvivorBase()
	{
		if (GetGame().IsClient())
		{
			GetDayZGame().Event_OnRPC.Insert(OnRPC_Call);
			SetEventMask(EntityEvent.POSTFRAME|EntityEvent.FRAME);
			m_EntityYeeted = false;
		}
	}

	void ~SurvivorBase()
	{
		if (GetGame() && GetGame().IsClient())
		{
			GetDayZGame().Event_OnRPC.Remove(OnRPC_Call);
			ClearEventMask(EntityEvent.POSTFRAME|EntityEvent.INIT);
		}
	}

	//Jumping after change of scale is causing a client crash :(
	override bool CanJump()
	{
		//if (m_setLocalScale /*&& (m_VPlayerScale > 1.0 || m_VPlayerScale < 1.0)*/)
			//return false;

		return super.CanJump();
	}

	override void EOnFrame(IEntity other, float timeSlice) // | EntityEvent.FRAME
    {
        if (!GetGame().IsClient())
            return;

        /*
        if (m_isInvisible)
		{
			if (!IsControlledPlayer())
			{
				SetPosition("0 -6000 0");
				Update();
			}
		}*/

        if (m_VScalePlayer)
        	SetYawPitchRoll(Vector(GetYawPitchRoll()[0], 0, 0));
    }

	override void EOnPostFrame(IEntity other, int extra) //!EntityEvent.POSTFRAME
	{
		super.EOnPostFrame(other, extra);
		if (!GetGame().IsClient())
			return;

		if (m_isInvisible)
		{
			if (!IsControlledPlayer())
			{
				/*
				if (!m_EntityYeeted)
				{
					m_EntityYeeted = true;
					SetPosition("0 -6000 0");
					Update();
					DeleteOnClient();
				}*/

				ClearFlags(FLAGS_INVIS, true);
				SetScale(0);
				dBodySetInteractionLayer(this, PhxInteractionLayers.NOCOLLISION);
				DisableSimulation(true);
			}
			SetInvisible(true);
		}
		
		if (m_VScalePlayer && m_VPlayerScale > 0.0)
		{
			SetScale(m_VPlayerScale);
		}
	}

	//for transfer values from server
	void ReceiveValue(int value_type, float value)
	{
		if(value_type == TYPE_HEALTH)
		{
			m_HealthClient = value;
		}
		else if(value_type == TYPE_BLOOD)
		{
			m_BloodClient = value;
		}
	}

	void OnRPC_Call(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (!GetGame().IsClient())
			return;

		SurvivorBase survivor;
		if (rpc_type == VPPATRPCs.RPC_SYNC_INVISIBILITY)
		{
			bool is_enabled;
			if (ctx.Read(is_enabled))
			{
				if (Class.CastTo(survivor, target))
				{
					survivor.m_isInvisible = is_enabled;
					if (is_enabled)
					{
						survivor.ClearFlags(FLAGS_INVIS, true);

						//Ty Wardog for this snippet :)
						//interactionLayer &= ~PhxInteractionLayers.CHARACTER;
						//dBodySetInteractionLayer(survivor, interactionLayer|PhxInteractionLayers.NOCOLLISION|PhxInteractionLayers.AI_NO_COLLISION|PhxInteractionLayers.RAGDOLL);
					}
					else
					{
						survivor.SetFlags(FLAGS_INVIS, true);
						dBodySetInteractionLayer(survivor, PhxInteractionLayers.CHARACTER);
					}

					if (!survivor.IsControlledPlayer())
					{
						//survivor.SetPosition("0 -6000 0");
						dBodySetInteractionLayer(survivor, PhxInteractionLayers.NOCOLLISION);
						survivor.Update();
						survivor.DisableSimulation(is_enabled);
						//survivor.DeleteOnClient();
					}
					survivor.SetInvisible(is_enabled);
				}
			}
		}

		//Scale
		if (rpc_type == VPPATRPCs.RPC_SYNC_SCALEVAL)
		{
			float scale_value;
			if (ctx.Read(scale_value))
			{
				if (Class.CastTo(survivor, target))
				{
					m_defualt_scale = survivor.GetScale();
					m_setLocalScale = true;
					survivor.SetScale(Math.Clamp(scale_value, 0.01, 100.0));
					if (scale_value == 1.0){
						dBodyEnableGravity(survivor, true);
					}else{
						dBodyEnableGravity(survivor, false);
					}
				}
			}
		}

		//transfer values from server
		if (rpc_type == VPPATRPCs.RPC_SYNC_TRANSFER_VALUES)
		{
			Param2<int,float> params;
			if (!ctx.Read(params))
				return;

			ReceiveValue(params.param1, params.param2);
		}
	}
};