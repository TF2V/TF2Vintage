//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_squad.h"
#include "tf_bot_escort_squad_leader.h"
#include "../scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "../scenario/capture_the_flag/tf_bot_fetch_flag.h"


ConVar tf_bot_squad_escort_range( "tf_bot_squad_escort_range", "500", FCVAR_CHEAT );
ConVar tf_bot_formation_debug( "tf_bot_formation_debug", "0", FCVAR_CHEAT );


CTFBotEscortSquadLeader::CTFBotEscortSquadLeader( Action<CTFBot> *done_action )
{
	this->m_DoneAction = done_action;
	m_PathFollower.SetGoalTolerance( 0 );
}

CTFBotEscortSquadLeader::~CTFBotEscortSquadLeader()
{
}


const char *CTFBotEscortSquadLeader::GetName() const
{
	return "EscortSquadLeader";
}


ActionResult<CTFBot> CTFBotEscortSquadLeader::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	/* BUG: doesn't set PathFollower's min lookahead distance */

	m_vecLeaderGoalDirection = vec3_origin;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEscortSquadLeader::Update( CTFBot *me, float dt )
{
	if ( dt <= 0.0f )
		return Action<CTFBot>::Continue();

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	CTFBotSquad *squad = me->GetSquad();
	if ( squad == nullptr )
	{
		if ( m_DoneAction != nullptr )
			return Action<CTFBot>::ChangeTo( m_DoneAction, "Not in a Squad" );

		return Action<CTFBot>::Done( "Not in a Squad" );
	}

	me->FlagForUpdate();

	CTFBot *leader = squad->GetLeader();
	if ( leader == nullptr || !leader->IsAlive() )
	{
		me->LeaveSquad();

		if ( m_DoneAction != nullptr )
			return Action<CTFBot>::ChangeTo( m_DoneAction, "Squad leader is dead" );

		return Action<CTFBot>::Done( "Squad leader is dead" );
	}

	if ( TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() && me == leader )
	{
		if ( me->HasAttribute( CTFBot::AttributeType::AGGRESSIVE ) )
		{
			return Action<CTFBot>::ChangeTo( new CTFBotPushToCapturePoint( new CTFBotFetchFlag( false ) ),
											 "I'm now the squad leader! Going for the flag!" );

			return Action<CTFBot>::ChangeTo( new CTFBotFetchFlag( false ),
											 "I'm now the squad leader! Going for the flag!" );
		}
	}

	CTFWeaponBase *weapon = me->m_Shared.GetActiveTFWeapon();
	if ( weapon != nullptr && weapon->IsMeleeWeapon() )
	{
		if ( me->IsRangeLessThan( leader, tf_bot_squad_escort_range.GetFloat() ) )
		{
			if ( me->IsLineOfSightClear( leader ) )
			{
				ActionResult<CTFBot> result = m_MeleeAttack.Update( me, dt );
				if ( result.m_type == ActionResultType::CONTINUE )
					return Action<CTFBot>::Continue();
			}
		}
	}

	CUtlVector<CTFBot *> members;
	squad->CollectMembers( &members );

	CUtlVector<CTFBot *> non_medics;
	FOR_EACH_VEC( members, i )
	{
		CTFBot *member = members[i];
		if ( !member->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			non_medics.AddToTail( member );
		}
	}

	PathFollower const *leader_path = leader->GetCurrentPath();
	if ( leader_path == nullptr || leader_path->GetCurrentGoal() == nullptr )
	{
		me->SetSquadFormationError( 0.0f );
		me->SetIsInFormation( false );

		return Action<CTFBot>::Continue();
	}

	const Path::Segment *leader_goal = leader_path->GetCurrentGoal();
	Vector vec_to_goal = ( leader_goal->pos - leader->GetAbsOrigin() );
	if ( vec_to_goal.IsLengthLessThan( 25.0f ) )
	{
		const Path::Segment *next_seg = leader_path->NextSegment( leader_goal );
		if ( next_seg != nullptr )
		{
			vec_to_goal = ( next_seg->pos - leader->GetAbsOrigin() );
		}
	}

	vec_to_goal.NormalizeInPlace();
	if ( !m_vecLeaderGoalDirection.IsZero() )
	{
		float yaw_this = UTIL_VecToYaw( vec_to_goal );
		float yaw_prev = UTIL_VecToYaw( m_vecLeaderGoalDirection );

		float yaw_diff = AngleDiff( yaw_this, yaw_prev );
		float yaw_diff_max = dt * 30.0f;

		float yaw_next = yaw_prev +
			Clamp( yaw_diff, -yaw_diff_max, yaw_diff_max );

		FastSinCos( RAD2DEG( yaw_next ),
					&m_vecLeaderGoalDirection.y,
					&m_vecLeaderGoalDirection.x );
		m_vecLeaderGoalDirection.z = 0.0f;
	}
	else
	{
		m_vecLeaderGoalDirection = vec_to_goal;
	}

	float formation_size = Max( squad->GetFormationSize(), 0.0f );

	int idx;
	for ( idx = 0; idx < squad->GetMemberCount(); ++idx )
	{
		if ( me->IsSelf( non_medics[idx] ) )
		{
			break;
		}
	}

	int my_idx = idx - 1;
	float angle = ( M_PI / 6.0f ) * ( idx / 2 );
	if ( my_idx % 2 == 0 )
	{
		angle = -angle;
	}

	float a_sin, a_cos;
	FastSinCos( angle, &a_sin, &a_cos );

	Vector goal_dir;
	goal_dir.x = m_vecLeaderGoalDirection.x * a_cos - m_vecLeaderGoalDirection.y * a_sin;
	goal_dir.y = m_vecLeaderGoalDirection.y * a_cos + m_vecLeaderGoalDirection.x * a_sin;
	goal_dir.z = 0;

	Vector ideal_pos = leader->GetAbsOrigin() + ( formation_size * goal_dir );

	trace_t trace;
	CTraceFilterIgnoreTeammates filter( me, COLLISION_GROUP_NONE, me->GetTeamNumber() );
	UTIL_TraceLine( leader->GetAbsOrigin(), ideal_pos - leader->GetAbsOrigin(),
					MASK_PLAYERSOLID, &filter, &trace );

	if ( trace.DidHitWorld() )
	{
		float scale = 0.6f * me->GetBodyInterface()->GetHullWidth();

		ideal_pos = ( scale * trace.plane.normal ) + trace.endpos;
		ideal_pos.z -= 35.5f;
	}

	if ( tf_bot_formation_debug.GetBool() )
	{
		NDebugOverlay::Circle( ideal_pos, 16.0f, 0, 255, 0, 255, true, 0.1f );

		CFmtStr str_idx;
		NDebugOverlay::Text( ideal_pos, str_idx.sprintf( "%d", my_idx ), false, 0.1f );
	}

	Vector vec_error = ideal_pos - me->GetAbsOrigin();
	float error = vec_error.Length2D();
	me->SetSquadFormationError( Min( error, 100.0f ) * ( 1.0f / 100.0f ) );

	if ( error < 50.0f )
	{
		Vector vec_fix = vec_error * goal_dir;
		if ( vec_fix.Length() == 0.0f )
		{
			me->SetSquadFormationError( 0.0f );
		}
		else
		{
			me->GetLocomotionInterface()->Approach( ideal_pos, 1.0f );
			return Action<CTFBot>::Continue();
		}
	}

	if ( m_ctRecomputePath.IsElapsed() )
	{
		m_ctRecomputePath.Start( RandomFloat( 0.1f, 0.2f ) );
		me->SetIsInFormation( false );

		CTFBotPathCost cost_func( me, FASTEST_ROUTE );
		if ( !m_PathFollower.Compute( me, ideal_pos, cost_func, 0.0f, true ) )
			me->SetIsInFormation( true );

		if ( m_PathFollower.GetLength() > 750.0f )
			me->SetIsInFormation( true );
	}

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}

void CTFBotEscortSquadLeader::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
}


CTFBotWaitForOutOfPositionSquadMember::CTFBotWaitForOutOfPositionSquadMember()
{
}

CTFBotWaitForOutOfPositionSquadMember::~CTFBotWaitForOutOfPositionSquadMember()
{
}


const char *CTFBotWaitForOutOfPositionSquadMember::GetName() const
{
	return "WaitForOutOfPositionSquadMember";
}


ActionResult<CTFBot> CTFBotWaitForOutOfPositionSquadMember::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_ctTimeout.Start( 2.0f );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotWaitForOutOfPositionSquadMember::Update( CTFBot *me, float dt )
{
	if ( m_ctTimeout.IsElapsed() )
		return Action<CTFBot>::Done( "Timeout" );

	CTFBotSquad *squad = me->GetSquad();
	if ( squad == nullptr || me != squad->GetLeader() )
		return Action<CTFBot>::Done( "No squad" );

	if ( squad->IsInFormation() )
		return Action<CTFBot>::Done( "Everyone is in formation. Moving on." );

	return Action<CTFBot>::Continue();
}
