//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_robot_destruction_robot_behaviors.h"
#include "tf_robot_destruction_robot.h"
#include "particle_parse.h"


Action<CTFRobotDestruction_Robot> *CRobotBehavior::InitialContainedAction( CTFRobotDestruction_Robot *me )
{
	return new CRobotMaterialize;
}

ActionResult<CTFRobotDestruction_Robot> CRobotBehavior::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotBehavior::Update( CTFRobotDestruction_Robot *me, float interval )
{
	if ( m_idleTimer.IsElapsed() && m_speakTimer.IsElapsed() )
	{
		m_speakTimer.Start( 1.0 );
		m_idleTimer.Start( RandomFloat( 6.0, 10.0 ) );

		me->EmitSound( g_RobotData[ me->GetType() ]->m_pszIdleSound ); 
	}

	return Continue();
}

QueryResultType CRobotBehavior::IsPositionAllowed( const INextBot *me, const Vector &pos ) const
{
	return ANSWER_YES;
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotBehavior::OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info )
{
	return TrySuspendFor( new CRobotEnterPanic, RESULT_TRY, "Ouch! I've been hit!" );
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotBehavior::OnContact( CTFRobotDestruction_Robot *me, CBaseEntity *pOther, CGameTrace *result )
{
	if ( m_speakTimer.IsElapsed() && ( pOther->IsPlayer() || dynamic_cast<CTFRobotDestruction_Robot *>( pOther ) ) )
	{
		m_speakTimer.Start( 3.0 );

		me->EmitSound( g_RobotData[ me->GetType() ]->m_pszCollideSound );
	}

	return TryContinue();
}



ActionResult<CTFRobotDestruction_Robot> CRobotMaterialize::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
    return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotMaterialize::Update( CTFRobotDestruction_Robot *me, float dt )
{
	return ChangeTo( new CRobotSpawn, "I've fully materialized" );
}



ActionResult<CTFRobotDestruction_Robot> CRobotSpawn::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_BOT_SPAWN );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotSpawn::Update( CTFRobotDestruction_Robot *me, float dt )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CRobotPatrol, "I've finished my spawn sequence" );
	}

	return Continue();
}

void CRobotSpawn::OnEnd( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *nextAction )
{
	me->SetBodygroup( me->FindBodygroupByName( "head_shell" ), 1 );
	me->SetBodygroup( me->FindBodygroupByName( "body_shell" ), 1 );
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotSpawn::OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL, "I'm spawning and being attacked!" );
}



ActionResult<CTFRobotDestruction_Robot> CRobotPatrol::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_BOT_PRIMARY_MOVEMENT );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotPatrol::OnResume( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *interruptingAction )
{
	me->GetBodyInterface()->StartActivity( ACT_BOT_PRIMARY_MOVEMENT );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotPatrol::Update( CTFRobotDestruction_Robot *me, float dt )
{
	CPathTrack *pNextPath = me->GetGoalPath();
	if ( me->IsRangeGreaterThan( pNextPath, 50.0f ) )
	{
		if ( m_PathFollower.GetAge() > 0.5f )
		{
			CRobotPathCost cost( me );
			m_PathFollower.Compute( me, pNextPath->GetAbsOrigin(), cost );
		}

		m_PathFollower.Update( me );
	}
	else
	{
		me->ArriveAtPath();
	}

	return Continue();
}



ActionResult<CTFRobotDestruction_Robot> CRobotEnterPanic::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_BOT_PANIC_START );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotEnterPanic::Update( CTFRobotDestruction_Robot *me, float dt )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CRobotPanic, "I've finished my enter panic sequence" );
	}

	return Continue();
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotEnterPanic::OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL, "I'm entering panic and being attacked!" );
}



ActionResult<CTFRobotDestruction_Robot> CRobotLeavePanic::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_BOT_PANIC_END );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotLeavePanic::Update( CTFRobotDestruction_Robot *me, float dt )
{
	if ( me->IsActivityFinished() )
	{
		return Done( "I've finished my leave panic sequence" );
	}

	return Continue();
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotLeavePanic::OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info )
{
	return TryToSustain( RESULT_CRITICAL, "I'm leaving panic and being attacked!" );
}



ActionResult<CTFRobotDestruction_Robot> CRobotPanic::OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction )
{
	me->SetPanicking( true );
	me->GetBodyInterface()->StartActivity( ACT_BOT_PANIC );
	me->GetLocomotionInterface()->SetDesiredSpeed( 150.0f );

	m_attackedTimer.Start( 5.0f );
	m_bSpinRight = RandomInt(0, 1) == 1; // Don't spin the saem direction
	m_spinTimer.Start( RandomFloat( 0.75f, 1.25f ) ); // Spin around in a fit

	// Screech our tires, we're out of there!
	DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, me, "wheel" );

	m_speakTimer.Start( 3.0f );
	me->EmitSound( g_RobotData[ me->GetType() ]->m_pszHurtSound );

	return Continue();
}

ActionResult<CTFRobotDestruction_Robot> CRobotPanic::Update( CTFRobotDestruction_Robot *me, float dt )
{
	if ( m_attackedTimer.IsElapsed() )
	{
		return ChangeTo( new CRobotLeavePanic, "I'm done panicking" );
	}

	QAngle vecAngles = me->GetLocalAngles();
	if ( m_spinTimer.GetRemainingTime() < ( m_spinTimer.GetCountdownDuration() / 2 ) )
	{
		float flSpinAmt = 2500.f * gpGlobals->frametime;
		flSpinAmt *= m_bSpinRight ? 1.f : -1.f;
		vecAngles.y += flSpinAmt;
		me->SetLocalAngles( vecAngles );
	}

	Vector vecFwd;
	AngleVectors( vecAngles, &vecFwd );

	// We only go forward in a panic
	Vector vecToTravelTo = me->GetAbsOrigin() + vecFwd * 30.0f;

	trace_t tr;
	UTIL_TraceLine( vecToTravelTo, vecToTravelTo - Vector( 0, 0, 30 ), MASK_PLAYERSOLID, me, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	if ( tr.fraction < 1.0 )
	{
		// Make sure we can actually move here
		me->GetLocomotionInterface()->Approach( vecToTravelTo );
	}

	if ( m_spinTimer.IsElapsed() )
	{
		m_bSpinRight = RandomInt(0, 1) == 1;
		m_spinTimer.Start( RandomFloat( 0.75f, 1.25f ) );

		// Make sure we're still smoking
		DispatchParticleEffect( "rocketjump_smoke", PATTACH_POINT_FOLLOW, me, "wheel" );
	}

	return Continue();
}

void CRobotPanic::OnEnd( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *nextAction )
{
	me->SetPanicking( false );
	me->GetLocomotionInterface()->SetDesiredSpeed( 80.f );
}

EventDesiredResult<CTFRobotDestruction_Robot> CRobotPanic::OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info )
{
	if ( m_speakTimer.IsElapsed() )
	{
		m_speakTimer.Start( RandomFloat( 1.5f, 2.f ) );
		me->EmitSound( g_RobotData[ me->GetType() ]->m_pszHurtSound );
	}

	m_attackedTimer.Start( 5.0f );

	return TryToSustain( RESULT_IMPORTANT, "I'm panicking and getting attacked!" );
}
