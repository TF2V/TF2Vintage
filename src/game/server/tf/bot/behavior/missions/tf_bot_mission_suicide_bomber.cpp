//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mission_suicide_bomber.h"
#include "tf_obj_sentrygun.h"
#include "particle_parse.h"
#include "tf_team.h"
#include "player_vs_environment/tf_populators.h"
#include "player_vs_environment/tf_population_manager.h"


ConVar tf_bot_suicide_bomb_range( "tf_bot_suicide_bomb_range", "300", FCVAR_CHEAT );
ConVar tf_bot_suicide_bomb_friendly_fire( "tf_bot_suicide_bomb_friendly_fire", "1", FCVAR_CHEAT );


CTFBotMissionSuicideBomber::CTFBotMissionSuicideBomber()
{
}

CTFBotMissionSuicideBomber::~CTFBotMissionSuicideBomber()
{
}

const char *CTFBotMissionSuicideBomber::GetName() const
{
	return "MissionSuicideBomber";
}


ActionResult<CTFBot> CTFBotMissionSuicideBomber::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_bDetonating       = false;
	m_bDetReachedGoal   = false;
	m_bDetLostAllHealth = false;

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_ctDetonation.Invalidate();
	m_nConsecutivePathFailures = 0;

	m_hTarget = me->GetMissionTarget();
	if ( m_hTarget != nullptr )
		m_vecTargetPos = m_hTarget->GetAbsOrigin();

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMissionSuicideBomber::Update( CTFBot *me, float dt )
{
	if ( m_ctDetonation.HasStarted() )
	{
		if ( !m_ctDetonation.IsElapsed() )
			return Action<CTFBot>::Continue();

		m_vecDetonatePos = me->GetAbsOrigin();
		Detonate( me );

		if ( m_bDetReachedGoal && m_hTarget != nullptr &&  m_hTarget->IsBaseObject() )
		{
			CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( m_hTarget.Get() );
			if ( sentry != nullptr && sentry->GetOwner() != nullptr )
			{
				CTFPlayer *owner = ToTFPlayer( sentry->GetOwner() );
				if ( owner != nullptr )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_sentrybuster_detonate" );
					if ( event != nullptr )
					{
						event->SetInt( "player", ENTINDEX( owner ) );
						event->SetFloat( "det_x", this->m_vecDetonatePos.x );
						event->SetFloat( "det_y", this->m_vecDetonatePos.y );
						event->SetFloat( "det_z", this->m_vecDetonatePos.z );
						gameeventmanager->FireEvent( event );
					}
				}
			}
		}

		return Action<CTFBot>::Done( "KABOOM!" );
	}

	if ( me->GetHealth() == 1 )
	{
		StartDetonate( me, false, true );
		return Action<CTFBot>::Continue();
	}

	if ( m_hTarget != nullptr )
	{
		if ( m_hTarget->IsAlive() && !m_hTarget->IsEffectActive( EF_NODRAW ) )
		{
			m_vecTargetPos = m_hTarget->GetAbsOrigin();
		}

		if ( m_hTarget->IsBaseObject() )
		{
			CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( m_hTarget.Get() );
			if ( sentry != nullptr && sentry->IsBeingCarried() && sentry->GetOwner() != nullptr )
			{
				m_vecTargetPos = sentry->GetOwner()->GetAbsOrigin();
			}
		}
	}

	float goal_range = ( 1.0f / 3.0f ) * tf_bot_suicide_bomb_range.GetFloat();
	if ( me->IsDistanceBetweenLessThan( m_vecTargetPos, goal_range ) && 
		 me->IsLineOfFireClear( m_vecTargetPos + Vector( 0, 0, StepHeight ) ) )
	{
		StartDetonate( me, true, false );
	}

	if ( m_ctPlaySound.IsElapsed() )
	{
		m_ctPlaySound.Start( 4.0f );
		me->EmitSound( "MVM.SentryBusterIntro" );
	}

	if ( m_ctRecomputePath.IsElapsed() )
	{
		m_ctRecomputePath.Start( RandomFloat( 0.5f, 1.0f ) );

		CTFBotPathCost cost_func( me, FASTEST_ROUTE );
		if ( m_PathFollower.Compute( me, m_vecTargetPos, cost_func, 0.0f, true ) )
		{
			m_nConsecutivePathFailures = 0;
		}
		else
		{
			if ( ++m_nConsecutivePathFailures > 2 )
				StartDetonate( me, false, false );
		}
	}

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}

void CTFBotMissionSuicideBomber::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
}


EventDesiredResult<CTFBot> CTFBotMissionSuicideBomber::OnStuck( CTFBot *me )
{
	if ( !m_bDetonating && !m_ctDetonation.HasStarted() )
		StartDetonate( me, false, false );

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotMissionSuicideBomber::OnKilled( CTFBot *me, const CTakeDamageInfo &info )
{
	/* how we get here:
	 * CBaseCombatCharacter::Event_Killed
	 * NextBotManager::OnKilled
	 * Action<CTFBot>::OnKilled
	 * CTFBotMissionSuicideBomber::OnKilled
	 */

	if ( !m_bDetonating )
	{
		if ( !m_ctDetonation.HasStarted() )
		{
			StartDetonate( me, false, false );
		}
		else
		{
			 /* BUG: probably bad to call Detonate when m_bDetonating is false
			  * and we haven't called StartDetonate... */
			if ( m_ctDetonation.IsElapsed() )
			{
				Detonate( me );
			}
			else
			{
				if ( me->GetTeamNumber() != TEAM_SPECTATOR )
				{
					me->m_lifeState = LIFE_ALIVE;
					me->SetHealth( 1 );
				}
			}
		}
	}

	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotMissionSuicideBomber::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


void CTFBotMissionSuicideBomber::StartDetonate( CTFBot *me, bool reached_goal, bool lost_all_health )
{
	if ( this->m_ctDetonation.HasStarted() )
	{
		return;
	}

	if ( ( !me->IsAlive() || me->GetHealth() <= 0 ) &&  me->GetTeamNumber() != TEAM_SPECTATOR )
	{
		me->m_lifeState = LIFE_ALIVE;
		me->SetHealth( 1 );
	}

	m_bDetReachedGoal   = reached_goal;
	m_bDetLostAllHealth = lost_all_health;

	me->m_takedamage = DAMAGE_NO;

	// TODO: enum/default values for CTFPlayer::Taunt(taunts_t, int)
	me->Taunt( TAUNT_NORMAL, MP_CONCEPT_FIREWEAPON );

	m_ctDetonation.Start( 2.0f );

	me->EmitSound( "MvM.SentryBusterSpin" );
}

void CTFBotMissionSuicideBomber::Detonate( CTFBot *me )
{
	m_bDetonating = true;

	DispatchParticleEffect( "explosionTrail_seeds_mvm", me->GetAbsOrigin(), me->GetAbsAngles() );
	DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", me->GetAbsOrigin(), me->GetAbsAngles() );

	me->EmitSound( "MvM.SentryBusterExplode" );

	UTIL_ScreenShake( me->GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

	if ( !m_bDetReachedGoal && TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() )
	{
		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SENTRY_BUSTER_DOWN, TF_TEAM_MVM_PLAYERS );

		// Achievement stuff here
	}

	CUtlVector<CTFPlayer *> players_bothteams;
	CollectPlayers<CTFPlayer>( &players_bothteams, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers<CTFPlayer>( &players_bothteams, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	CUtlVector<CBaseCombatCharacter *> potential_victims;
	FOR_EACH_VEC( players_bothteams, i )
	{
		CTFPlayer *player = players_bothteams[i];
		potential_victims.AddToTail( static_cast<CBaseCombatCharacter *>( player ) );
	}

	CTFTeam *team_blu = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( team_blu != nullptr )
	{
		for ( int i = 0; i < team_blu->GetNumObjects(); ++i )
		{
			CBaseObject *obj = team_blu->GetObject( i );
			if ( obj != nullptr )
			{
				potential_victims.AddToTail( static_cast<CBaseCombatCharacter *>( obj ) );
			}
		}
	}

	CTFTeam *team_red = GetGlobalTFTeam( TF_TEAM_RED );
	if ( team_red != nullptr )
	{
		for ( int i = 0; i < team_red->GetNumObjects(); ++i )
		{
			CBaseObject *obj = team_red->GetObject( i );
			if ( obj != nullptr )
			{
				potential_victims.AddToTail( static_cast<CBaseCombatCharacter *>( obj ) );
			}
		}
	}

	CUtlVector<INextBot *> nextbots;
	TheNextBots().CollectAllBots( &nextbots );
	FOR_EACH_VEC( nextbots, i )
	{
		INextBot *nextbot = nextbots[i];
		CBaseCombatCharacter *ent = nextbot->GetEntity();

		if ( !ent->IsPlayer() && ent->IsAlive() )
		{
			potential_victims.AddToTail( ent );
		}
	}

	if ( m_bDetLostAllHealth )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_sentrybuster_killed" );
		if ( event != nullptr )
		{
			event->SetInt( "sentry_buster", me->entindex() );
			gameeventmanager->FireEvent( event );
		}
	}

	me->SetMission( CTFBot::MissionType::NONE, false );

	me->m_takedamage = DAMAGE_YES;

	FOR_EACH_VEC( potential_victims, i )
	{
		CBaseCombatCharacter *victim = potential_victims[i];

		Vector delta_wsc = victim->WorldSpaceCenter() - me->WorldSpaceCenter();
		if ( delta_wsc.IsLengthGreaterThan( tf_bot_suicide_bomb_range.GetFloat() ) )
			continue;

		if ( victim->IsPlayer() )
		{
			UTIL_ScreenFade( victim, {255, 255, 255, 255}, 1.0f, 0.1f, FFADE_IN );
		}

		if ( me->IsLineOfFireClear( victim ) )
		{
			int damage = Max( victim->GetMaxHealth(), victim->GetHealth() );

			/* NOTE: CTFPlayer::OnTakeDamage reduces the damage to 600 if:
			 * victim is a bot
			 * m_bForceFriendlyFire is true
			 * victim in same team as attacker
			 * victim->IsMiniBoss
			 */

			CTakeDamageInfo dmginfo( me, me, 4 * damage, DMG_BLAST );
			if ( tf_bot_suicide_bomb_friendly_fire.GetBool() )
			{
				dmginfo.SetForceFriendlyFire( true );
			}

			CalculateMeleeDamageForce( &dmginfo, delta_wsc.Normalized(), me->WorldSpaceCenter() );
			victim->TakeDamage( dmginfo );
		}
	}

	me->CommitSuicide( false, true );

	if ( me->IsAlive() )
	{
		me->ForceChangeTeam( TEAM_SPECTATOR );
	}

	if ( m_bDetLostAllHealth )
	{
		CWave *wave = g_pPopulationManager ? g_pPopulationManager->GetCurrentWave() : NULL;
		if ( wave != nullptr )
		{
			wave->m_nNumSentryBustersKilled++;
		}
	}
}
