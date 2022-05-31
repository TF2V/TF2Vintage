//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_escort.h"
#include "demoman/tf_bot_prepare_stickybomb_trap.h"
#include "tf_bot_attack.h"
#include "tf_bot_destroy_enemy_sentry.h"
#include "tf_obj_sentrygun.h"


ConVar tf_bot_escort_range( "tf_bot_escort_range", "300", FCVAR_CHEAT );


CTFBotEscort::CTFBotEscort( CBaseCombatCharacter *who )
{
	m_hWho = who;
}

CTFBotEscort::~CTFBotEscort()
{
}


const char *CTFBotEscort::GetName() const
{
	return "Escort";
}


ActionResult<CTFBot> CTFBotEscort::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEscort::Update( CTFBot *me, float dt )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat != nullptr && threat->IsVisibleInFOVNow() )
		return Action<CTFBot>::SuspendFor( new CTFBotAttack, "Attacking nearby threat" );

	if ( m_hWho != nullptr )
	{
		if ( me->IsRangeGreaterThan( m_hWho, tf_bot_escort_range.GetFloat() ) )
		{
			if ( m_ctRecomputePath.IsElapsed() )
			{
				CTFBotPathCost cost_func( me, FASTEST_ROUTE );
				m_PathFollower.Compute( me, m_hWho, cost_func, 0.0f, true );

				m_ctRecomputePath.Start( RandomFloat( 2.0f, 3.0f ) );
			}

			m_PathFollower.Update( me );
		}
		else if ( CTFBotPrepareStickybombTrap::IsPossible( me ) )
		{
			return Action<CTFBot>::SuspendFor( new CTFBotPrepareStickybombTrap, "Laying sticky bombs!" );
		}
	}

	if ( me->GetTargetSentry() != nullptr && CTFBotDestroyEnemySentry::IsPossible( me ) )
		return Action<CTFBot>::SuspendFor( new CTFBotDestroyEnemySentry, "Going after an enemy sentry to destroy it" );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEscort::OnMoveToSuccess( CTFBot *actor, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnMoveToFailure( CTFBot *actor, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnStuck( CTFBot *actor )
{
	m_ctRecomputePath.Invalidate();

	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnCommandApproach( CTFBot *actor, const Vector &v1, float f1 )
{
	return Action<CTFBot>::TryContinue();
}


QueryResultType CTFBotEscort::ShouldRetreat( const INextBot *nextbot ) const
{
	return ANSWER_NO;
}


CBaseCombatCharacter *CTFBotEscort::GetWho() const
{
	return m_hWho;
}

void CTFBotEscort::SetWho( CBaseCombatCharacter *who )
{
	m_hWho = who;
}
