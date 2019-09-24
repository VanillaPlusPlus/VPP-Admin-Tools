modded class PlayerBase
{
	private bool    hasGodmode;
	private bool	m_isInvisible;
	
	void PlayerBase()
	{
		RegisterNetSyncVariableBool("m_isInvisible");
		GetRPCManager().AddRPC( "RPC_PlayerBase", "InvokeReload", this, SingeplayerExecutionType.Server );
	}

	override void OnConnect()
	{
		super.OnConnect();
		
		if(GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			if(!GetPlayerListManager().HasPlayerInList(GetIdentity().GetPlainId()))
			{
				GetPlayerListManager().AddUserServer(GetIdentity().GetName(), GetIdentity().GetPlainId());
			}
			
			if(GetPermissionManager().HasUserGroup(GetIdentity().GetPlainId()))
			{
				GetPlayerListManager().SendPlayerList(GetIdentity());
			}
		}
	}
		
	void InvokeReload(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type == CallType.Client)
        {
			if ( !GetGame().IsInventoryOpen() && this )
			{
				EntityAI entity_hands = this.GetHumanInventory().GetEntityInHands();
				
				if ( entity_hands && entity_hands.IsWeapon() )
				{
					Magazine foundMag = Magazine.Cast(entity_hands.GetAttachmentByConfigTypeName( "DefaultMagazine" ));
					if( foundMag && foundMag.IsMagazine())
					{
						foundMag.LocalSetAmmoMax();
					}else{
						this.QuickReloadWeapon( entity_hands );
					}
				}
			}
		}
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
			setInvisibility(m_isInvisible);
	}
	
	void setInvisibility(bool state)
	{
		m_isInvisible = state;

		if (!IsAlive()) return;
		SetInvisible(state);
		if (state){
			ClearFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true );
		}else{
			SetFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true);
		}
		SetSynchDirty();
	}
	
	void setGodMode(bool trigger)
	{
		hasGodmode = trigger;
		SetAllowDamage(!trigger);
	}
	
	bool GodModeStatus()
	{
		return hasGodmode;
	}
	
	bool InvisibilityStatus()
	{
		return m_isInvisible;
	}

	/*Misc functions*/
	vector GetHeadingPosition(int distance)
	{
		float heading = MiscGameplayFunctions.GetHeadingAngle(this);
		vector posModifier = Vector(-(distance * Math.Sin(heading)), 0, distance * Math.Cos(heading));
		return posModifier + GetPosition();
	}

	float GetAimingAngleLR()
	{
		HumanCommandWeapons	hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleLR();
		return 0.0;
	}
	
	float GetAimingAngleUD()
	{
		HumanCommandWeapons	hcw = GetCommandModifier_Weapons();
		if (hcw != null)
			return hcw.GetBaseAimingAngleUD();
		return 0.0;
	}
};