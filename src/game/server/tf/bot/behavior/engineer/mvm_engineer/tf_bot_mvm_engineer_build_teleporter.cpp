//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mvm_engineer_build_teleporter.h"
#include "tf_obj_teleporter.h"


ConVar tf_bot_engineer_building_health_multiplier( "tf_bot_engineer_building_health_multiplier", "2", FCVAR_CHEAT );


CTFBotMvMEngineerBuildTeleportExit::CTFBotMvMEngineerBuildTeleportExit( CTFBotHintTeleporterExit *hint )
{
	m_hintEntity = hint;
}

CTFBotMvMEngineerBuildTeleportExit::~CTFBotMvMEngineerBuildTeleportExit()
{
}


const char *CTFBotMvMEngineerBuildTeleportExit::GetName() const
{
	return "MvMEngineerBuildTeleportExit";
}


ActionResult<CTFBot> CTFBotMvMEngineerBuildTeleportExit::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerBuildTeleportExit::Update( CTFBot *me, float dt )
{
	if ( m_hintEntity == nullptr )
		return Action<CTFBot>::Done( "No hint entity" );

	if ( me->IsRangeGreaterThan( m_hintEntity->GetAbsOrigin(), 25.0f ) )
	{
		if ( m_ctRecomputePath.IsElapsed() )
		{
			m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost_func( me, FASTEST_ROUTE );
			m_PathFollower.Compute<CTFBotPathCost>( me, m_hintEntity->GetAbsOrigin(), cost_func, 0.0f, true );
		}

		m_PathFollower.Update( me );
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
	{
		return Action<CTFBot>::Continue();
	}

	me->DetonateOwnedObjectsOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT, true );

	CBaseEntity *ent = CreateEntityByName( "obj_teleporter" );
	if ( ent == nullptr )
		return Action<CTFBot>::Continue();

	CObjectTeleporter *tele = static_cast<CObjectTeleporter *>( ent );
	tele->SetAbsOrigin( this->m_hintEntity->GetAbsOrigin() );
	tele->SetAbsAngles( this->m_hintEntity->GetAbsAngles() );
	tele->SetObjectMode( 1 );
	tele->Spawn();

	FOR_EACH_VEC( me->m_TeleportWhere, i ) {
		tele->m_TeleportWhere.CopyAndAddToTail( me->m_TeleportWhere[i] );
	}

	tele->StartPlacement( me );
	tele->StartBuilding( me );

	int max_health = (int)( (float)tele->GetMaxHealthForCurrentLevel() *
							tf_bot_engineer_building_health_multiplier.GetFloat() );

	tele->SetMaxHealth( max_health );
	tele->SetHealth( max_health );

	m_hintEntity->SetOwnerEntity( tele );

	me->EmitSound( "Engineer.MVM_AutoBuildingTeleporter02" );

	return Action<CTFBot>::Done( "Teleport exit built" );
}
