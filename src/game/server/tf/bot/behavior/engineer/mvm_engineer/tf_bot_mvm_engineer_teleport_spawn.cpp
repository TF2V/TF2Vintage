//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mvm_engineer_teleport_spawn.h"
#include "player_vs_environment/tf_population_manager.h"
#include "player_vs_environment/tf_populators.h"
#include "tf_fx.h"


CTFBotMvMEngineerTeleportSpawn::CTFBotMvMEngineerTeleportSpawn( CBaseTFBotHintEntity *hint, bool non_silent )
{
	m_hintEntity = hint;
	m_bNonSilent = non_silent;
}

CTFBotMvMEngineerTeleportSpawn::~CTFBotMvMEngineerTeleportSpawn()
{
}


const char *CTFBotMvMEngineerTeleportSpawn::GetName() const
{
	return "MvMEngineerTeleportSpawn";
}


ActionResult<CTFBot> CTFBotMvMEngineerTeleportSpawn::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if ( !me->HasAttribute( CTFBot::AttributeType::TELEPORTTOHINT ) )
		return Action<CTFBot>::Done( "Cannot teleport to hint with out Attributes TeleportToHint" );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerTeleportSpawn::Update( CTFBot *me, float dt )
{
	if ( !m_ctPushAway.HasStarted() )
	{
		m_ctPushAway.Start( 0.1f );

		if ( m_hintEntity != nullptr )
		{
			TFGameRules()->PushAllPlayersAway( this->m_hintEntity->GetAbsOrigin(),
											   400.0f, 500.0f, TF_TEAM_RED, nullptr );
		}

		return Action<CTFBot>::Continue();
	}

	if ( !m_ctPushAway.IsElapsed() )
		return Action<CTFBot>::Continue();

	if ( m_hintEntity == nullptr )
		return Action<CTFBot>::Done( "Cannot teleport to hint as m_hintEntity is NULL" );

	Vector tele_pos = this->m_hintEntity->GetAbsOrigin();
	QAngle tele_ang = this->m_hintEntity->GetAbsAngles();

	me->Teleport( &tele_pos, &tele_ang, nullptr );

	CPVSFilter filter( tele_pos );

	TE_TFParticleEffect( filter, 0.0f, "teleported_blue", tele_pos, vec3_angle );
	TE_TFParticleEffect( filter, 0.0f, "player_sparkles_blue", tele_pos, vec3_angle );

	if ( m_bNonSilent )
	{
		TE_TFParticleEffect( filter, 0.0f, "teleported_mvm_bot", tele_pos, vec3_angle );
		me->EmitSound( "Engineer.MvM_BattleCry07" );
		m_hintEntity->EmitSound( "MvM.Robot_Engineer_Spawn" );

		if ( g_pPopulationManager )
		{
			CWave *pWave = g_pPopulationManager->GetCurrentWave();
			if ( pWave != nullptr )
			{
				if ( pWave->m_nNumEngineersTeleportSpawned == 0 )
					TFGameRules()->BroadcastSound( 255, "Announcer.MvM_First_Engineer_Teleport_Spawned" );
				else
					TFGameRules()->BroadcastSound( 255, "Announcer.MvM_Another_Engineer_Teleport_Spawned" );

				++pWave->m_nNumEngineersTeleportSpawned;
			}
		}
	}

	return Action<CTFBot>::Done( "Teleported" );
}
