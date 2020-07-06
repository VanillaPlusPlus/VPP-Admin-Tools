modded class PlayerBase
{
	private bool    hasGodmode;
	private bool    hasUnlimitedAmmo;
	private bool 	hasFlyinCar;
	private bool	m_isInvisible;
	private bool    m_freezePlayer;
	
	void PlayerBase()
	{
		RegisterNetSyncVariableBool("m_isInvisible");
		RegisterNetSyncVariableBool("m_freezePlayer");
		GetRPCManager().AddRPC( "RPC_PlayerBase", "InvokeReload", this, SingeplayerExecutionType.Server );
	}

	override void EOnFrame(IEntity other, float timeSlice)
	{
		super.EOnFrame(other, timeSlice);
		//if ( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT )
		//{
	        if ( m_isInvisible )
	        {
	        	this.ClearFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true );
	        }
		//}
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
		VPPSetInvisibility(m_isInvisible);
		FreezePlayer(m_freezePlayer);
	}
	
	void VPPSetInvisibility(bool state)
	{
		if (!state && GetGame().IsClient())
			this.SetFlags(EntityFlags.VISIBLE|EntityFlags.SOLID, true);
		else if ( GetGame().IsServer() ){
			m_isInvisible = state;
			SetSynchDirty();
		}
	}
	
	void setGodMode(bool trigger)
	{
		hasGodmode = trigger;
		SetAllowDamage(!trigger);
	}

	void FreezePlayer(bool state)
	{
		if ( GetGame().IsServer() )
		{
			m_freezePlayer = state;
			SetSynchDirty();
			return;
		}
		
		HumanInputController hic = this.GetInputController();
		if ( hic )
			hic.SetDisabled( state );
	}

	bool IsFreezeControls()
	{
		return m_freezePlayer;
	}
	
	bool GodModeStatus()
	{
		return hasGodmode;
	}
	
	bool InvisibilityStatus()
	{
		return m_isInvisible;
	}
	
	void SetUnlimitedAmmo(bool state)
	{
		hasUnlimitedAmmo = state;
	}
	
	bool IsUnlimitedAmmo()
	{
		return hasUnlimitedAmmo;
	}
	
	void SetCanFlyCars(bool state)
	{
		hasFlyinCar = state;
	}
	
	bool CanFlyCar()
	{
		return hasFlyinCar;
	}

	void UnlimitedAmmoCheck(Weapon_Base weapon)
	{
		if ( IsUnlimitedAmmo() )
		{
			Magazine magazine;
			if ( GetGame().IsServer() )
			{
				magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());

				if ( GetGame().IsMultiplayer() && magazine)
	            	GetGame().RemoteObjectDelete( magazine );

				if(magazine)
					magazine.ServerSetAmmoMax();

				if ( GetGame().IsMultiplayer() && magazine)
            		GetGame().RemoteObjectCreate( magazine );
			}
			else
			{
				magazine = weapon.GetMagazine(weapon.GetCurrentMuzzle());
				if(magazine)
				{
					magazine.LocalSetAmmoMax();
				}
			}
		}
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