//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_spy_leave_spawn_room.h"
#include "tf_bot_spy_hide.h"


CTFBotSpyLeaveSpawnRoom::CTFBotSpyLeaveSpawnRoom()
{
}

CTFBotSpyLeaveSpawnRoom::~CTFBotSpyLeaveSpawnRoom()
{
}


const char *CTFBotSpyLeaveSpawnRoom::GetName() const
{
	return "SpyLeaveSpawnRoom";
}


ActionResult<CTFBot> CTFBotSpyLeaveSpawnRoom::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	/* BUG: doesn't set PathFollower's min lookahead distance */

	me->DisguiseAsEnemy();
	me->PressAltFireButton();

	m_ctTeleport.Start( 2.0f + RandomFloat( 0.0f, 1.0f ) );
	m_nDistance = 0;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyLeaveSpawnRoom::Update( CTFBot *me, float dt )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( !m_ctTeleport.IsElapsed() )
		return Action<CTFBot>::Continue();

	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers<CTFPlayer>( &enemies, GetEnemyTeam( me ), true, false );

	enemies.Shuffle();

	FOR_EACH_VEC( enemies, i )
	{
		CTFPlayer *enemy = enemies[i];
		if ( TeleportNearVictim( me, enemy, m_nDistance ) )
		{
			return ChangeTo( new CTFBotSpyHide( enemy ), "Hiding!" );
		}
	}

	/* retry later */
	++m_nDistance;
	m_ctTeleport.Start( 1.0f );

	return Action<CTFBot>::Continue();
}


QueryResultType CTFBotSpyLeaveSpawnRoom::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}


bool TeleportNearVictim( CTFBot *spy, CTFPlayer *victim, int dist )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( victim == nullptr || victim->GetLastKnownArea() == nullptr )
		return false;

	float dist_limit = Min( ( 500.0f * dist ) + 1500.0f, 6000.0f );

	CUtlVector<CTFNavArea *> good_areas;

	CUtlVector<CTFNavArea *> near_areas;
	CollectSurroundingAreas( &near_areas, victim->GetLastKnownArea(), dist_limit,
							 StepHeight, StepHeight );

	FOR_EACH_VEC( near_areas, i ) {
		CTFNavArea *area = near_areas[i];

		if ( area->IsValidForWanderingPopulation() &&
			 !area->IsPotentiallyVisibleToTeam( victim->GetTeamNumber() ) )
		{
			good_areas.AddToTail( area );
		}
	}

	if ( good_areas.IsEmpty() )
		return false;

	int limit = Max( good_areas.Count(), 10 );
	for ( int i = 0; i < limit; ++i )
	{
		CTFNavArea *area = good_areas.Random();

		Vector pos = area->GetCenter() + Vector( 0, 0, StepHeight );
		if ( IsSpaceToSpawnHere( pos ) )
		{
			spy->Teleport( &pos, &vec3_angle, &vec3_origin );
			return true;
		}
	}

	return false;
}
