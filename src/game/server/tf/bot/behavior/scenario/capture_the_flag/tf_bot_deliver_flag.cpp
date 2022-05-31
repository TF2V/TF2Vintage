//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "NavMeshEntities/func_nav_prerequisite.h"
#include "tf_bot_deliver_flag.h"
#include "../../tf_bot_taunt.h"
#include "../../tf_bot_mvm_deploy_bomb.h"
#include "behavior/nav_entities/tf_bot_nav_ent_wait.h"
#include "behavior/nav_entities/tf_bot_nav_ent_move_to.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "particle_parse.h"

ConVar tf_mvm_bot_allow_flag_carrier_to_fight( "tf_mvm_bot_allow_flag_carrier_to_fight", "1", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_interval_to_1st_upgrade( "tf_mvm_bot_flag_carrier_interval_to_1st_upgrade", "5", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade( "tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade", "15", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade( "tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade", "15", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_health_regen( "tf_mvm_bot_flag_carrier_health_regen", "45.0f", FCVAR_CHEAT );


const char *CTFBotDeliverFlag::GetName() const
{
	return "DeliverFlag";
}


ActionResult<CTFBot> CTFBotDeliverFlag::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_flTotalTravelDistance = -1.0f;

	m_recomputePathTimer.Invalidate();
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( !tf_mvm_bot_allow_flag_carrier_to_fight.GetBool() )
		me->SetAttribute( CTFBot::AttributeType::SUPPRESSFIRE );

	if ( me->IsMiniBoss() )
	{
		m_nUpgradeLevel = -1;
		if ( TFObjectiveResource() )
		{
			// Max it out for bosses
			TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 4 );
			TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
			TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
		}
	}
	else
	{
		m_nUpgradeLevel = 0;
		m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat() );
		if ( TFObjectiveResource() )
		{
			TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
			TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
		}

	}

	return BaseClass::Continue();
}

ActionResult<CTFBot> CTFBotDeliverFlag::Update( CTFBot *me, float dt )
{
	CCaptureFlag *pFlag = me->GetFlagToFetch();
	if ( pFlag == nullptr )
		return BaseClass::Done( "No flag" );

	CTFPlayer *pCarrier = ToTFPlayer( pFlag->GetOwnerEntity() );
	if ( pCarrier == nullptr || !me->IsSelf( pCarrier ) )
		return BaseClass::Done( "I'm no longer carrying the flag" );

	Action<CTFBot> *pBuffAction = me->OpportunisticallyUseWeaponAbilities();
	if ( pBuffAction )
		return BaseClass::SuspendFor( pBuffAction, "Opportunistically using buff item" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CCaptureZone *pZone = me->GetFlagCaptureZone();
		if ( pZone == nullptr )
			return BaseClass::Done( "No flag capture zone exists!" );

		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pZone->WorldSpaceCenter(), func );

		float flTraveledSoFar = m_flTotalTravelDistance;
		m_flTotalTravelDistance = NavAreaTravelDistance( me->GetLastKnownArea(), TheNavMesh->GetNavArea( pZone->WorldSpaceCenter() ), func );

		if ( flTraveledSoFar != -1.0f && m_flTotalTravelDistance - flTraveledSoFar > 2000.0f )
		{
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bomb_Reset" );

			CUtlVector<CTFPlayer *> players;
			CollectPlayers( &players, TF_TEAM_MVM_PLAYERS );
			FOR_EACH_VEC( players, i )
			{
				CTFPlayer *pPlayer = players[i];
				if ( !pPlayer )
					continue;
				
				if ( pPlayer == me->GetDamagerHistory( 0 ).hDamager )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_reset_by_player" );
					if ( event )
					{
						event->SetInt( "player", pPlayer->entindex() );
						gameeventmanager->FireEvent( event );
					}

					CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayer, me, 100 );
				}
			}
		}

		m_recomputePathTimer.Start( RandomFloat( 1.0, 2.0 ) );
	}

	m_PathFollower.Update( me );

	if ( UpgradeOverTime( me ) )
	{
		return SuspendFor( new CTFBotTaunt, "Taunting for our new upgrade" );
	}

	return BaseClass::Continue();
}

void CTFBotDeliverFlag::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->ClearAttribute( CTFBot::AttributeType::SUPPRESSFIRE );

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		me->m_Shared.ResetRageSystem();
	}
}


QueryResultType CTFBotDeliverFlag::ShouldHurry( const INextBot *nextbot ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotDeliverFlag::ShouldRetreat( const INextBot *nextbot ) const
{
	return ANSWER_NO;
}


EventDesiredResult< CTFBot > CTFBotDeliverFlag::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	if ( TFGameRules()->IsMannVsMachineMode() && other && FClassnameIs( other, "func_capturezone" ) )
	{
		return TrySuspendFor( new CTFBotMvMDeployBomb, RESULT_CRITICAL, "Delivering the bomb!" );
	}

	return TryContinue();
}


bool CTFBotDeliverFlag::UpgradeOverTime( CTFBot *me )
{
	if ( !TFGameRules()->IsMannVsMachineMode() || m_nUpgradeLevel == -1 )
		return false;

	CTFNavArea *pArea = me->GetLastKnownArea();
	if ( pArea && pArea->HasTFAttributes( me->GetTeamNumber() == TF_TEAM_RED ? TF_NAV_RED_SPAWN_ROOM : TF_NAV_BLUE_SPAWN_ROOM ) )
	{
		m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat() );

		TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
		TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
	}

	if ( m_nUpgradeLevel > 0 && m_buffPulseTimer.IsElapsed() )
	{
		m_buffPulseTimer.Start( 1.0f );

		CUtlVector< CTFPlayer * > aliveBots;
		CollectPlayers( &aliveBots, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );
		FOR_EACH_VEC( aliveBots, i )
		{
			if ( me->IsRangeLessThan( aliveBots[i], 450.0f ) )
				aliveBots[i]->m_Shared.AddCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, 1.2f );
		}
	}

	if ( !m_upgradeTimer.IsElapsed() )
		return false;

	if ( m_nUpgradeLevel < 3 )
	{
		++m_nUpgradeLevel;

		TFGameRules()->BroadcastSound( 255, "MVM.Warning" );

		switch ( m_nUpgradeLevel )
		{
			case 1:
			{
				m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade.GetFloat() );

				if ( TFObjectiveResource() )
				{
					TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 1 );
					TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
					TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE1, TF_TEAM_MVM_PLAYERS );
					DispatchParticleEffect( "mvm_levelup1", PATTACH_POINT_FOLLOW, me, "head" );
				}

				return true;
			}
			case 2:
			{
				static CSchemaAttributeHandle pAttrDef_HealthRegen( "health regen" );

				m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade.GetFloat() );

				if ( !pAttrDef_HealthRegen )
				{
					Warning( "TFBotSpawner: Invalid attribute 'health regen'\n" );
				}
				else
				{
					CAttributeList *pAttrList = me->GetAttributeList();
					if ( pAttrList )
					{
						pAttrList->SetRuntimeAttributeValue( pAttrDef_HealthRegen, tf_mvm_bot_flag_carrier_health_regen.GetFloat() );
					}
				}

				if ( TFObjectiveResource() )
				{
					TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 2 );
					TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
					TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE2, TF_TEAM_MVM_PLAYERS );
					DispatchParticleEffect( "mvm_levelup2", PATTACH_POINT_FOLLOW, me, "head" );
				}

				return true;
			}
			case 3:
			{
				me->m_Shared.AddCond( TF_COND_CRITBOOSTED );

				if ( TFObjectiveResource() )
				{
					TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 3 );
					TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
					TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE3, TF_TEAM_MVM_PLAYERS );
					DispatchParticleEffect( "mvm_levelup3", PATTACH_POINT_FOLLOW, me, "head" );
				}

				return true;
			}
		}
	}

	return false;
}



const char *CTFBotPushToCapturePoint::GetName() const
{
	return "PushToCapturePoint";
}


ActionResult<CTFBot> CTFBotPushToCapturePoint::Update( CTFBot *me, float dt )
{
	CCaptureZone *pZone = me->GetFlagCaptureZone();
	if ( pZone == nullptr )
	{
		if ( m_pDoneAction )
			return BaseClass::ChangeTo( m_pDoneAction, "No flag capture zone exists!" );

		return BaseClass::Done( "No flag capture zone exists!" );
	}

	if ( ( pZone->WorldSpaceCenter() - me->GetAbsOrigin() ).LengthSqr() < Square( 50.0f ) )
	{
		if ( m_pDoneAction )
			return BaseClass::ChangeTo( m_pDoneAction, "At destination" );

		return BaseClass::Done( "At destination" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_recomputePathTimer.IsElapsed() )
	{
		CTFBotPathCost func( me, FASTEST_ROUTE );
		m_PathFollower.Compute( me, pZone->WorldSpaceCenter(), func );

		m_recomputePathTimer.Start( RandomFloat( 1.0, 2.0 ) );
	}

	m_PathFollower.Update( me );

	return BaseClass::Continue();
}


EventDesiredResult<CTFBot> CTFBotPushToCapturePoint::OnNavAreaChanged( CTFBot *me, CNavArea *area1, CNavArea *area2 )
{
	if ( area1 == nullptr || !area1->HasPrerequisite() )
		return BaseClass::TryContinue();

	FOR_EACH_VEC( area1->GetPrerequisiteVector(), i )
	{
		CFuncNavPrerequisite *prereq = area1->GetPrerequisiteVector()[ i ];
		if ( prereq == nullptr || !prereq->IsEnabled() || !prereq->PassesTriggerFilters( me ) )
			continue;

		if ( prereq->IsTask( CFuncNavPrerequisite::TASK_WAIT ) )
			return BaseClass::TrySuspendFor( new CTFBotNavEntMoveTo( prereq ), RESULT_TRY, "Prerequisite commands me to move to an entity" );

		if ( prereq->IsTask( CFuncNavPrerequisite::TASK_MOVE_TO_ENTITY ) )
			return BaseClass::TrySuspendFor( new CTFBotNavEntWait( prereq ), RESULT_TRY, "Prerequisite commands me to wait" );
	}

	return BaseClass::TryContinue();
}
