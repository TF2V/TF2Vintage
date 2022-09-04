//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "particle_parse.h"
#include "tf_obj.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_gamerules.h"
#include "tf_bot.h"
#include "tf_bot_components.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotBody::GetHeadAimTrackingInterval( void ) const
{
	CTFBot *me = static_cast<CTFBot *>( GetBot() );

	switch( me->GetDifficulty() )
	{
		case CTFBot::DifficultyType::NORMAL:
			return 0.30f;
		case CTFBot::DifficultyType::HARD:
			return 0.10f;
		case CTFBot::DifficultyType::EXPERT:
			return 0.05f;

		default:
			return 1.00f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotLocomotion::Update( void )
{
	CTFBot *me = ToTFBot( GetEntity() );
	if ( me->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		return;

	BaseClass::Update();

	if ( !IsOnGround() )
		me->PressCrouchButton( 0.3f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotLocomotion::Approach( const Vector &pos, float goalWeight )
{
	if ( !IsOnGround() && !IsClimbingOrJumping() )
		return;

	BaseClass::Approach( pos, goalWeight );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotLocomotion::Jump( void )
{
	BaseClass::Jump();

	CTFBot *me = ToTFBot( GetEntity() );

	int nCustomJumpParticle = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( me, nCustomJumpParticle, bot_custom_jump_particle );
	if ( nCustomJumpParticle != 0 )
	{
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, me, "foot_L" );
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, me, "foot_R" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotLocomotion::GetMaxJumpHeight( void ) const
{
	return 72.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotLocomotion::GetDeathDropHeight( void ) const
{
	return 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotLocomotion::GetRunSpeed( void ) const
{
	CTFBot *me = ToTFBot( GetEntity() );
	return me->GetPlayerClass()->GetMaxSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotLocomotion::IsAreaTraversable( const CNavArea *baseArea ) const
{
	if ( !BaseClass::IsAreaTraversable( baseArea ) )
		return false;

	const CTFNavArea *tfArea = static_cast<const CTFNavArea *>( baseArea );
	if ( tfArea == nullptr )
		return false;

	// unless the round is over and we are the winning team, we can't enter the other teams spawn
	if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		switch ( GetEntity()->GetTeamNumber() )
		{
			case TF_TEAM_RED:
			{
				if ( tfArea->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM ) )
					return false;

				break;
			}
			case TF_TEAM_BLUE:
			{
				if ( tfArea->HasTFAttributes( TF_NAV_RED_SPAWN_ROOM ) )
					return false;

				break;
			}
			default:
				break;
		}
	}
	else
	{
		if ( TFGameRules()->GetWinningTeam() != GetEntity()->GetTeamNumber() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotLocomotion::IsEntityTraversable( CBaseEntity *ent, TraverseWhenType when ) const
{
	if ( ent )
	{
		// always assume we can walk through players, we'll try to avoid them if needed later
		if ( ent->IsPlayer() )
			return true;

		if ( ent->IsBaseObject() )
		{
			CBaseObject *pObject = static_cast<CBaseObject *>( ent );
			if ( pObject->GetBuilder() == GetEntity() ) // we can't walk through our own buildings...
				return false;
		}
	}

	return BaseClass::IsEntityTraversable( ent, when );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBotVision::CTFBotVision( INextBot *bot )
	: BaseClass( bot )
{
	m_updateTimer.Start( RandomFloat( 2.0f, 4.0f ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBotVision::~CTFBotVision()
{
	m_PVNPCs.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotVision::Reset( void )
{
	BaseClass::Reset();
	m_PVNPCs.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotVision::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	BaseClass::Update();

	CTFBot *me = ToTFBot( GetBot()->GetEntity() );
	if ( me == nullptr )
		return;

	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( me ), COLLECT_ONLY_LIVING_PLAYERS );

	FOR_EACH_VEC( enemies, i )
	{
		CTFPlayer *pPlayer = enemies[i];
		if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		{
			const CKnownEntity *known = GetKnown( pPlayer );

			if ( !known || !known->IsVisibleRecently() || pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
				me->ForgetSpy( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotVision::CollectPotentiallyVisibleEntities( CUtlVector<CBaseEntity *> *ents )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ents->RemoveAll();

	for ( int i=0; i < gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			ents->AddToTail( pPlayer );
		}
	}

	UpdatePotentiallyVisibleNPCs();
	for ( int i=0; i < m_PVNPCs.Count(); ++i )
	{
		CBaseEntity *pEntity = m_PVNPCs[i];
		ents->AddToTail( pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotVision::IsVisibleEntityNoticed( CBaseEntity *ent ) const
{
	CTFBot *me = ToTFBot( GetBot()->GetEntity() );

	CTFPlayer *pPlayer = ToTFPlayer( ent );
	if ( pPlayer == nullptr )
	{
		return true;
	}

	// we should always be aware of our "friends"
	if ( !me->IsEnemy( pPlayer ) )
	{
		return true;
	}


	if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) ||
		 pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
		 pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		{
			me->RealizeSpy( pPlayer );
		}

		return true;
	}

	if ( pPlayer->m_Shared.IsStealthed() )
	{
		if ( pPlayer->m_Shared.GetPercentInvisible() < 0.75f )
		{
			me->RealizeSpy( pPlayer );
			return true;
		}
		else
		{
			me->ForgetSpy( pPlayer );
			return false;
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		CTFBot::SuspectedSpyInfo *pSuspectInfo = me->IsSuspectedSpy( pPlayer );
		if ( !pSuspectInfo || !pSuspectInfo->IsCurrentlySuspected() )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
			{
				me->ForgetSpy( pPlayer );
				return false;
			}
		}
	}

	if ( me->IsKnownSpy( pPlayer ) )
	{
		return true;
	}

	// No es MvM
	if ( !TFGameRules()->IsMannVsMachineMode() )
	{
		if ( pPlayer->IsPlacingSapper() )
		{
			me->RealizeSpy( pPlayer );
			return true;
		}
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotVision::IsIgnored( CBaseEntity *ent ) const
{
	CTFBot *me = ToTFBot( GetBot()->GetEntity() );
	if ( !me->IsEnemy( ent ) )
		return false;

	if ( ( ent->GetEffects() & EF_NODRAW ) != 0 )
		return true;

	CTFPlayer *pPlayer = ToTFPlayer( ent );
	if ( pPlayer )
	{
		switch ( pPlayer->GetPlayerClass()->GetClassIndex() )
		{
			case TF_CLASS_MEDIC:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_MEDICS ) )
				{
					return true;
				}
				break;

			case TF_CLASS_ENGINEER:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_ENGINEERS ) )
				{
					return true;
				}
				break;

			case TF_CLASS_SNIPER:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_SNIPERS ) )
				{
					return true;
				}
				break;

			case TF_CLASS_SCOUT:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_SCOUTS ) )
				{
					return true;
				}
				break;

			case TF_CLASS_SPY:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_SPIES ) )
				{
					return true;
				}
				break;

			case TF_CLASS_DEMOMAN:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_DEMOMEN ) )
				{
					return true;
				}
				break;

			case TF_CLASS_SOLDIER:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_SOLDIERS ) )
				{
					return true;
				}
				break;

			case TF_CLASS_HEAVYWEAPONS:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_HEAVIES ) )
				{
					return true;
				}
				break;

			case TF_CLASS_PYRO:
				if ( me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_PYROS ) )
				{
					return true;
				}
				break;
		}

		if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) ||
			 pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
			 pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) ||
			 pPlayer->m_Shared.InCond( TF_COND_URINE ) )
		{
			return false;
		}

		if ( pPlayer->m_Shared.IsStealthed() )
		{
			return ( pPlayer->m_Shared.GetPercentInvisible() >= 0.75f );
		}

		if ( pPlayer->IsPlacingSapper() )
		{
			return false;
		}

		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}

		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			return ( pPlayer->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() );
		}
	}

	if ( ent->IsBaseObject() )
	{
		CBaseObject *pObject = static_cast<CBaseObject *>( ent );
		if ( pObject->HasSapper() )
		{
			if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
				return false;

			return true;
		}

		if ( pObject->IsPlacing() || pObject->IsBeingCarried() )
			return true;

		if ( pObject->GetType() == OBJ_SENTRYGUN && me->IsBehaviorFlagSet( TF_BOT_IGNORE_ENEMY_SENTRY_GUNS ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotVision::GetMaxVisionRange() const
{
	return 6000.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBotVision::GetMinRecognizeTime( void ) const
{
	CTFBot *me = static_cast<CTFBot *>( GetBot() );

	switch ( me->GetDifficulty() )
	{
		case CTFBot::DifficultyType::NORMAL:
			return 0.50f;
		case CTFBot::DifficultyType::HARD:
			return 0.30f;
		case CTFBot::DifficultyType::EXPERT:
			return 0.15f;

		default:
			return 1.00f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBotVision::UpdatePotentiallyVisibleNPCs()
{
	if ( !m_updatePVNPCsTimer.IsElapsed() )
		return;

	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_updatePVNPCsTimer.Start( RandomFloat( 2.0f, 4.0f ) );

	m_PVNPCs.RemoveAll();

	for ( int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );

		if ( pObj->GetType() == OBJ_SENTRYGUN || ( pObj->GetType() == OBJ_DISPENSER && pObj->ClassMatches( "obj_dispenser" ) ) || pObj->GetType() == OBJ_TELEPORTER )
		{
			m_PVNPCs.AddToTail( pObj );
		}
	}

	CUtlVector<INextBot *> nextbots;
	TheNextBots().CollectAllBots( &nextbots );
	FOR_EACH_VEC( nextbots, i )
	{
		CBaseCombatCharacter *pEntity = nextbots[i]->GetEntity();
		if ( pEntity && !pEntity->IsPlayer() )
			m_PVNPCs.AddToTail( pEntity );
	}
}
