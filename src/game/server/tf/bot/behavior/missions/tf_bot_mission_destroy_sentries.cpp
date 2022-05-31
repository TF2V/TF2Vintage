//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mission_destroy_sentries.h"
#include "../medic/tf_bot_medic_heal.h"
#include "tf_bot_mission_suicide_bomber.h"
#include "../spy/tf_bot_spy_sap.h"
#include "../tf_bot_destroy_enemy_sentry.h"
#include "tf_obj_sentrygun.h"

// Completely unused

CTFBotMissionDestroySentries::CTFBotMissionDestroySentries( CObjectSentrygun *sentry )
{
	m_hSentry = sentry;
}

CTFBotMissionDestroySentries::~CTFBotMissionDestroySentries()
{
}


const char *CTFBotMissionDestroySentries::GetName() const
{
	return "MissionDestroySentries";
}


ActionResult<CTFBot> CTFBotMissionDestroySentries::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	if ( me->IsPlayerClass( TF_CLASS_MEDIC ) )
		return Action<CTFBot>::ChangeTo( new CTFBotMedicHeal,  "My job is to heal/uber the others in the mission" );

	me->SetAttribute( CTFBot::AttributeType::IGNOREENEMIES );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMissionDestroySentries::Update( CTFBot *me, float dt )
{
	if ( !m_hSentry )
	{
		m_hSentry = me->GetTargetSentry();

		if ( !m_hSentry )
		{
			//m_hSentry = TFGameRules()->FindSentryGunWithMostKills( GetEnemyTeam( me ) );
		}
	}

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		return Action<CTFBot>::SuspendFor( new CTFBotMissionSuicideBomber, "On a suicide mission to blow up a sentry" );

	if ( !m_hSentry )
	{
		me->SetMission( CTFBot::MissionType::NONE, false );
		return Action<CTFBot>::ChangeTo( GetParentAction()->InitialContainedAction( me ), "Mission complete - reverting to normal behavior" );
	}

	if ( m_hSentry != me->GetTargetSentry() )
		me->NoteTargetSentry( m_hSentry );

	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
		return Action<CTFBot>::SuspendFor( new CTFBotSpySap( m_hSentry.Get() ), "On a mission to sap a sentry" );
	
	return Action<CTFBot>::SuspendFor( new CTFBotDestroyEnemySentry, "On a mission to destroy a sentry" );
}

void CTFBotMissionDestroySentries::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	/* BUG: doesn't save/restore old flag value */
	me->ClearAttribute( CTFBot::AttributeType::IGNOREENEMIES );
}


CObjectSentrygun *CTFBotMissionDestroySentries::SelectSentryTarget( CTFBot *actor )
{
	return nullptr;
}
