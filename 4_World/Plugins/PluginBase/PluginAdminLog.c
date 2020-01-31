modded class PluginAdminLog
{
	/*override void LogPrint( string message )
	{
		super.LogPrint( message );
	}*/

	string VPPGetPlayerPrefix( vector pos, PlayerIdentity identity )
	{
		string PosArray[3] = { pos[0].ToString(), pos[2].ToString(), pos[1].ToString() };
		
		for ( int i = 0; i < 3; i++ )
		{
			m_DotIndex = PosArray[i].IndexOf(".");
			if ( m_DotIndex != -1 )
			{
				PosArray[i] = PosArray[i].Substring( 0, m_DotIndex + 2 );
			}
		}
		
		if ( !identity )
		{
			return string.Format("Player [ Unknown/Dead Entity ]\nID: [ Unknown ]\nPosition: [ %1,%2,%3 ]\n", PosArray[0], PosArray[2], PosArray[1]);
		}
		
		return string.Format("Player: [ %1 ]\nID: [ %2 ]\nPosition: [ %3,%4,%5 ]\n",  identity.GetName(), identity.GetPlainId(), PosArray[0], PosArray[2], PosArray[1]);
	}

	string VPPGetHitMessage( TotalDamageResult damageResult, int component, string zone, string ammo) 
	{	
		if ( damageResult )	
		{
			float dmg = damageResult.GetHighestDamage("Health");
			return " into Zone: [" + zone + "] Component: [" + component.ToString() + "] Damage Dealt: [" + dmg.ToString() + "] Damage Type: [" + ammo + "] ";
		}
		else
		{
			return " into Block: [" + component.ToString() + "] 0 damage dealt ";
		}
	}

	override void PlayerHitBy( TotalDamageResult damageResult, int damageType, PlayerBase player, EntityAI source, int component, string dmgZone, string ammo ) // PlayerBase.c 
	{
		if ( player && source )		
		{
			string PlayerPrefix = VPPGetPlayerPrefix( player.GetPosition() , player.GetIdentity() ) + " HP: [" + player.GetHealth().ToString() + "] ";
			string HitMessage   = VPPGetHitMessage( damageResult, component, dmgZone, ammo );
			autoptr HitDamageMessage rpt = new HitDamageMessage();

			if (player.GetIdentity() == null) return;

			rpt.victimName = player.GetIdentity().GetName();
			rpt.victimId   = player.GetIdentity().GetPlainId();

			switch ( damageType )
			{
				case DT_CLOSE_COMBAT:	// Player melee, animals, infected 
				
					if ( ( source.IsZombie() || source.IsAnimal() ) )  // Infected & Animals
					{
						rpt.sourceName = source.GetType();
						rpt.sourceId   = "_obj";
						rpt.details    = PlayerPrefix + " hit by " + source.GetDisplayName() + HitMessage;
					}			
					else if ( source.IsPlayer() )				// Fists
					{
						m_Source = PlayerBase.Cast( source );
						m_PlayerPrefix2 = VPPGetPlayerPrefix( m_Source.GetPosition() ,  m_Source.GetIdentity() );
						
						rpt.sourceName = m_Source.GetIdentity().GetName();
						rpt.sourceId   = m_Source.GetIdentity().GetPlainId();
						rpt.details    = PlayerPrefix + " hit by " + m_PlayerPrefix2 + HitMessage;
					}
					else if ( source.IsMeleeWeapon() )			// Melee weapons
					{				
						m_ItemInHands = source.GetDisplayName();		
						m_Source = PlayerBase.Cast( source.GetHierarchyParent() );
						m_PlayerPrefix2 = VPPGetPlayerPrefix( m_Source.GetPosition() ,  m_Source.GetIdentity() );

						rpt.sourceName = m_Source.GetIdentity().GetName();
						rpt.sourceId   = m_Source.GetIdentity().GetPlainId();
						rpt.details = PlayerPrefix + " hit by " + m_PlayerPrefix2 + HitMessage + " with " + m_ItemInHands;			
					}
					else
					{
						rpt.sourceName = source.GetType();
						rpt.sourceId   = "_obj";
						rpt.details = PlayerPrefix + " hit by " + source.GetType() + HitMessage;					
					}
					break;
				
				case DT_FIRE_ARM:	// Player ranged
				
					if ( source.IsWeapon() )
					{
						m_ItemInHands = source.GetDisplayName();				
						m_Source = PlayerBase.Cast( source.GetHierarchyParent() );
						m_PlayerPrefix2 = VPPGetPlayerPrefix( m_Source.GetPosition() ,  m_Source.GetIdentity() );
						m_Distance = vector.Distance( player.GetPosition(), m_Source.GetPosition() );
						
						rpt.sourceName = m_Source.GetIdentity().GetName();
						rpt.sourceId   = m_Source.GetIdentity().GetPlainId();

						rpt.details = PlayerPrefix + " hit by " + m_PlayerPrefix2 + HitMessage + " with [" + m_ItemInHands + "] from [" + m_Distance + "] meters ";
					}
					else 
					{
						rpt.sourceName = source.GetType();
						rpt.sourceId   = "_obj";
						rpt.details = PlayerPrefix + " hit by [" + source.GetType() + "] " + HitMessage;		
					}
					break;
				
				case DT_EXPLOSION:	// Explosion
					rpt.sourceName =  ammo;
					rpt.sourceId   = "Explosion";
					rpt.details = PlayerPrefix + " hit by explosion [" + ammo + "] ";
					break;
						
				case DT_STUN: 		// unused atm
					rpt.details = PlayerPrefix + " stunned by [" + ammo + "] ";
					break;
						
				case DT_CUSTOM:		// Others (Vehicle hit, fall, fireplace, barbed wire ...)
					if ( ammo == "FallDamage" )			// Fall
					{
						rpt.details = PlayerPrefix + " hit by [" + ammo + "]";
						rpt.sourceName = player.GetIdentity().GetName();
						rpt.sourceId   = player.GetIdentity().GetPlainId();
					}
					else if ( source.GetType() == "AreaDamageBase" )  
					{
						EntityAI parent = EntityAI.Cast( source );
						if ( parent )
						{
							rpt.sourceName = parent.GetType();
							rpt.sourceId   = "_obj";
							rpt.details = PlayerPrefix + " hit by [" + parent.GetType() + "] with [" + ammo + "] ";
						}
					}
					else
					{
						rpt.sourceName = source.GetType();
						rpt.sourceId   = "_obj";
						rpt.details = PlayerPrefix + " hit by [" + source.GetType() + "] with [" + ammo + "] ";
					}
					break;
											
				default:
					rpt.details ="WARNING: PlayerHitBy Unknown damageType: [" + ammo + "]";
					break;
			}
			//Wrap up and send
			rpt.AddEmbed();
			GetWebHooksManager().PostData(HitDamageMessage, rpt);
		}
		else
		{
			LogPrint("DEBUG: player/source does not exist");
		}
		super.PlayerHitBy( damageResult, damageType, player, source, component, dmgZone, ammo );
	}

	override void PlayerKilled( PlayerBase player, Object source )  
	{
		if ( player && source )
		{
			string PlayerPrefix = VPPGetPlayerPrefix( player.GetPosition(), player.GetIdentity() );
			autoptr KillDeathMessage rpt = new KillDeathMessage();
			
			if (player.GetIdentity() == null) return;

			rpt.victimName = player.GetIdentity().GetName();
			rpt.victimGUID = player.GetIdentity().GetPlainId();

			if( player == source )	// deaths not caused by another object (starvation, dehydration)
			{
				m_StatWater  = player.GetStatWater();
				m_StatEnergy = player.GetStatEnergy();
				m_BleedMgr   = player.GetBleedingManagerServer();
				
				if ( m_StatWater && m_StatEnergy && m_BleedMgr )
				{
					rpt.details = PlayerPrefix + " died. Stats => Water: [" + m_StatWater.Get().ToString() + "] Energy: [" + m_StatEnergy.Get().ToString() + "] Bleed sources: [" + m_BleedMgr.GetBleedingSourcesCount().ToString() + "]";
				}
				else if ( m_StatWater && m_StatEnergy && !m_BleedMgr )
				{
					rpt.details = PlayerPrefix + " died. Stats => Water: " + m_StatWater.Get().ToString() + " Energy: " + m_StatEnergy.Get().ToString() + "]";
				}
				else
				{
					rpt.details = PlayerPrefix + " died. Stats => could not fetch";
				}
				rpt.killerName = rpt.victimName;
				rpt.killerGUID = player.GetIdentity().GetPlainId();
			}
			else if ( source.IsWeapon() || source.IsMeleeWeapon() )  // player
			{
				m_Source = PlayerBase.Cast( EntityAI.Cast( source ).GetHierarchyParent() );
				string name;
				string guid;

				if(m_Source != null && m_Source.GetIdentity() != null)
				{
					name = m_Source.GetIdentity().GetName();
					guid = m_Source.GetIdentity().GetPlainId();
					
					string PlayerPrefix2 = "";

					PlayerPrefix2 = VPPGetPlayerPrefix( m_Source.GetPosition() , m_Source.GetIdentity() );
					
					if ( source.IsMeleeWeapon() )
					{	
						rpt.details = PlayerPrefix + " killed by:\n" + PlayerPrefix2 + " with " + source.GetDisplayName();	
					}
					else
					{
						m_Distance = vector.Distance( player.GetPosition(), m_Source.GetPosition() );
						rpt.details = PlayerPrefix + " killed by:\n" + PlayerPrefix2 + " with [" + source.GetDisplayName() + "] from [" + m_Distance + "] meters ";
					}
					rpt.killerName = name;
					rpt.killerGUID = guid;
				}
				else
				{
					rpt.details = PlayerPrefix + " killed by: " + source.GetType();
				}
			}
			else
			{
				if (source.IsInherited(ZombieBase))
				{
					rpt.killerName = "Zombie";
				}
				rpt.details = PlayerPrefix + " killed by: " + source.GetType();
			}
			rpt.AddEmbed();
			GetWebHooksManager().PostData(KillDeathMessage, rpt);
		}
		super.PlayerKilled( player, source );
	}
};