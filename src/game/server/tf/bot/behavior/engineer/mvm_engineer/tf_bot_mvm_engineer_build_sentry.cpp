//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mvm_engineer_build_sentry.h"
#include "tf_obj_sentrygun.h"


CTFBotMvMEngineerBuildSentryGun::CTFBotMvMEngineerBuildSentryGun( CTFBotHintSentrygun *hint )
{
	m_hintEntity = hint;
}

CTFBotMvMEngineerBuildSentryGun::~CTFBotMvMEngineerBuildSentryGun()
{
}


const char *CTFBotMvMEngineerBuildSentryGun::GetName() const
{
	return "MvMEngineerBuildSentryGun";
}


ActionResult<CTFBot> CTFBotMvMEngineerBuildSentryGun::OnStart( CTFBot *actor, Action<CTFBot> *action )
{
	actor->StartBuildingObjectOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerBuildSentryGun::Update( CTFBot *actor, float dt )
{
	if ( this->m_hintEntity == nullptr )
		return Action<CTFBot>::Done( "No hint entity" );

	float range_to_hint = actor->GetRangeTo( m_hintEntity->GetAbsOrigin() );

	if ( range_to_hint < 200.0f )
	{
		actor->PressCrouchButton();
		actor->GetBodyInterface()->AimHeadTowards( m_hintEntity->GetAbsOrigin(),
												   IBody::MANDATORY, 0.1f, nullptr, "Placing sentry" );
	}

	if ( range_to_hint > 25.0f )
	{
		if ( m_ctRecomputePath.IsElapsed() )
		{
			m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost_func( actor, SAFEST_ROUTE );
			m_PathFollower.Compute<CTFBotPathCost>( actor, m_hintEntity->GetAbsOrigin(), cost_func, 0.0f, true );
		}

		m_PathFollower.Update( actor );
		if ( !m_PathFollower.IsValid() )
		{
/* BUG: one path failure ends the entire behavior...
 * could this be why engiebots sometimes zone out? */
			return Action<CTFBot>::Done( "Path failed" );
		}

		return Action<CTFBot>::Continue();
	}

	if ( !m_ctPushAway.HasStarted() )
	{
		m_ctPushAway.Start( 0.1f );

		if ( m_hintEntity != nullptr )
		{
			TFGameRules()->PushAllPlayersAway( m_hintEntity->GetAbsOrigin(),
											   400.0f, 500.0f, TF_TEAM_RED, nullptr );
		}

		return Action<CTFBot>::Continue();
	}

	if ( !m_ctPushAway.IsElapsed() )
		return Action<CTFBot>::Continue();

	actor->DetonateOwnedObjectsOfType( OBJ_SENTRYGUN, OBJECT_MODE_NONE, true );

	CBaseEntity *ent = CreateEntityByName( "obj_sentrygun" );
	if ( ent == nullptr )
	{
/* BUG: no we didn't */
		return Action<CTFBot>::Done( "Built a sentry" );
	}

	m_hSentry = static_cast<CObjectSentrygun *>( ent );
	m_hSentry->SetName( m_hintEntity->GetEntityName() );

	m_hintEntity->m_nSentriesHere++;

	m_hSentry->SetDefaultUpgradeLevel( 2 );

	m_hSentry->SetAbsOrigin( m_hintEntity->GetAbsOrigin() );
	m_hSentry->SetAbsAngles( m_hintEntity->GetAbsAngles() );
	m_hSentry->Spawn();

	m_hSentry->StartPlacement( actor );
	m_hSentry->StartBuilding( actor );

	m_hintEntity->SetOwnerEntity( m_hSentry );

	m_hSentry = nullptr;
	return Action<CTFBot>::Done( "Built a sentry" );
}

void CTFBotMvMEngineerBuildSentryGun::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( m_hSentry != nullptr )
	{
		m_hSentry->DropCarriedObject( me );
		UTIL_Remove( m_hSentry );
		m_hSentry = nullptr;
	}
}
