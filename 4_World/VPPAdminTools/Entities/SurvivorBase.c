modded class SurvivorBase
{
	static const int FLAGS_INVIS = EntityFlags.VISIBLE|EntityFlags.SOLID|EntityFlags.ACTIVE;
	private float m_defualt_scale;
	private bool  m_setLocalScale;

	void SurvivorBase()
	{
		if (GetGame().IsClient())
		{
			GetDayZGame().Event_OnRPC.Insert(OnRPC_Call);
			SetEventMask(EntityEvent.POSTFRAME);
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
		if (m_setLocalScale /*&& (m_VPlayerScale > 1.0 || m_VPlayerScale < 1.0)*/)
			return false;

		return super.CanJump();
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
				ClearFlags(FLAGS_INVIS, true);
				DisableSimulation(true);
				SetScale(0);
				dBodySetInteractionLayer(this, PhxInteractionLayers.RAGDOLL);
			}
			SetInvisible(true);
		}
		
		if (m_VScalePlayer && m_VPlayerScale > 0.0)
		{
			SetScale(m_VPlayerScale);
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
					if (is_enabled){
						survivor.ClearFlags(FLAGS_INVIS, true);
					}else{
						survivor.SetFlags(FLAGS_INVIS, true);
						dBodySetInteractionLayer(survivor, PhxInteractionLayers.CHARACTER);
					}

					if (!survivor.IsControlledPlayer())
					{
						survivor.DisableSimulation(is_enabled);
						dBodySetInteractionLayer(survivor, PhxInteractionLayers.NOCOLLISION);
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
						survivor.DisableSimulation(false);
					}else{
						survivor.DisableSimulation(true);
					}
				}
			}
		}
	}
};