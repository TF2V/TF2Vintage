//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_mvm_deploy_bomb.h"

extern ConVar tf_deploying_bomb_delay_time;
extern ConVar tf_deploying_bomb_time;


CTFBotMvMDeployBomb::CTFBotMvMDeployBomb()
{
}

CTFBotMvMDeployBomb::~CTFBotMvMDeployBomb()
{
}


const char *CTFBotMvMDeployBomb::GetName() const
{
	return "MvMDeployBomb";
}


ActionResult<CTFBot> CTFBotMvMDeployBomb::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	// TODO: dword actor+0x2830 = 1
	m_ctDelay.Start( tf_deploying_bomb_delay_time.GetFloat() );

	m_vecStand = me->GetAbsOrigin();

	me->GetLocomotionInterface()->Stop();
	me->SetAbsVelocity( Vector( 0.0f, 0.0f, 0.0f ) );

	if ( me->IsMiniBoss() )
	{
		static CSchemaAttributeHandle pAttrDef_AirblastVerticalVulnerability( "airblast vertical vulnerability multiplier" );
		if ( !pAttrDef_AirblastVerticalVulnerability )
		{
			Warning( "TFBotSpawner: Invalid attribute 'airblast vertical vulnerability multiplier'\n" );
		}
		else
		{
			me->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_AirblastVerticalVulnerability, 0.0f );
		}
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMDeployBomb::Update( CTFBot *me, float dt )
{
	// TODO
	return Action<CTFBot>::Continue();
}

void CTFBotMvMDeployBomb::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	//if ( dword actor+0x2830 == 2 )
	{
		me->DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
	}

	if ( me->IsMiniBoss() )
	{
		static CSchemaAttributeHandle pAttrDef_AirblastVerticalVulnerability( "airblast vertical vulnerability multiplier" );
		if ( !pAttrDef_AirblastVerticalVulnerability )
		{
			Warning( "TFBotSpawner: Invalid attribute 'airblast vertical vulnerability multiplier'\n" );
		}
		else
		{
			me->GetAttributeList()->RemoveAttribute( pAttrDef_AirblastVerticalVulnerability );
		}
	}

	// dword actor+0x2830 = 0
}


EventDesiredResult<CTFBot> CTFBotMvMDeployBomb::OnContact( CTFBot *me, CBaseEntity *who, CGameTrace *trace )
{
	return Action<CTFBot>::TryToSustain( RESULT_CRITICAL );
}


QueryResultType CTFBotMvMDeployBomb::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}
