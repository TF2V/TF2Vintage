//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "../../../tf_bot.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_mvm_engineer_idle.h"
#include "tf_bot_mvm_engineer_teleport_spawn.h"
#include "tf_bot_mvm_engineer_build_sentry.h"
#include "tf_bot_mvm_engineer_build_teleporter.h"
#include "../../tf_bot_retreat_to_cover.h"
#include "entity_capture_flag.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_teleporter.h"

ConVar tf_bot_engineer_mvm_sentry_hint_bomb_forward_range( "tf_bot_engineer_mvm_sentry_hint_bomb_forward_range", "0", FCVAR_CHEAT );
ConVar tf_bot_engineer_mvm_sentry_hint_bomb_backward_range( "tf_bot_engineer_mvm_sentry_hint_bomb_backward_range", "3000", FCVAR_CHEAT );
ConVar tf_bot_engineer_mvm_hint_min_distance_from_bomb( "tf_bot_engineer_mvm_hint_min_distance_from_bomb", "1300", FCVAR_CHEAT );

CON_COMMAND_F( tf_bot_mvm_show_engineer_hint_region, "Show the nav areas MvM engineer bots will consider when selecting sentry and teleporter hints", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	Vector vecForward;
	pPlayer->EyeVectors( &vecForward );

	Vector const vecStart = pPlayer->EyePosition();
	Vector const vecEnd = vecStart + vecForward * 10000;

	trace_t tr;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( !tr.DidHit() )
		return;

	CTFNavArea *pArea = assert_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( tr.endpos ) );
	if ( pArea == NULL )
		return;

	float flBomDistance = pArea->GetBombTargetDistance();
	float flForwardDistance = tf_bot_engineer_mvm_sentry_hint_bomb_forward_range.GetFloat();
	float flBackwardDistance = tf_bot_engineer_mvm_sentry_hint_bomb_backward_range.GetFloat();

	CUtlVector<CTFNavArea *> travelableBombAreas;
	TFNavMesh()->CollectAreasWithinBombTravelRange( &travelableBombAreas, flBomDistance - flForwardDistance, flBomDistance + flBackwardDistance );

	CUtlVector<CTFNavArea *> hintAreas;
	FOR_EACH_VEC( ITFBotHintEntityAutoList::AutoList(), i )
	{
		CBaseTFBotHintEntity *pHintEntity = (CBaseTFBotHintEntity *)ITFBotHintEntityAutoList::AutoList()[i];
		CTFNavArea *pArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( pHintEntity );
		if ( pArea )
			hintAreas.AddToTail( pArea );
	}

	FOR_EACH_VEC( travelableBombAreas, i )
	{
		CTFNavArea *pArea = travelableBombAreas[i];
		if ( pArea->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM | TF_NAV_RED_SPAWN_ROOM ) )
			continue;

		pArea->DrawFilled( 255, 100, 0, 0, 5.0f );

		FOR_EACH_VEC( hintAreas, j )
		{
			if ( pArea == hintAreas[j] )
			{
				CBaseTFBotHintEntity *pHintEntity = (CBaseTFBotHintEntity *)ITFBotHintEntityAutoList::AutoList()[j];

				Color c;
				if ( pHintEntity->GetHintType() == CBaseTFBotHintEntity::HintType::SENTRY_GUN )
				{
					c = Color( 0, 255, 0 );
				}
				else if ( pHintEntity->GetHintType() == CBaseTFBotHintEntity::HintType::TELEPORTER_EXIT )
				{
					c = Color( 0, 0, 255 );
				}
				else
				{
					bool bCloseToBomb = ( tr.endpos - pHintEntity->GetAbsOrigin() ).Length() < tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat();
					c = bCloseToBomb ? Color( 255, 0, 0 ) : Color( 255, 255, 0 );
				}

				NDebugOverlay::Sphere( pHintEntity->GetAbsOrigin(), 50, c.r(), c.g(), c.b(), true, 5.0 );
			}
		}

		NDebugOverlay::Sphere( tr.endpos, tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat(), 255, 255, 0, false, 5.0f );
	}
}


CTFBotMvMEngineerIdle::CTFBotMvMEngineerIdle()
{
}

CTFBotMvMEngineerIdle::~CTFBotMvMEngineerIdle()
{
}


const char *CTFBotMvMEngineerIdle::GetName() const
{
	return "MvMEngineerIdle";
}

ActionResult<CTFBot> CTFBotMvMEngineerIdle::OnStart( CTFBot *me, Action<CTFBot> *action )
{
	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->StopLookingForEnemies();

	m_hHintSentry = nullptr;
	m_hHintTele   = nullptr;
	m_hHintNest   = nullptr;

	m_nTeleportAttempts = 0;

	m_bTeleportedToHint         = false;
	m_bTriedToDetonateStaleNest = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerIdle::Update( CTFBot *me, float dt )
{
	if ( !me->IsAlive() )
	{
		return Action<CTFBot>::Done();
	}

	CBaseCombatWeapon *w_melee = me->Weapon_GetSlot( TF_LOADOUT_SLOT_MELEE );
	if ( w_melee != nullptr )
	{
		me->Weapon_Switch( w_melee );
	}

	if ( m_hHintNest == nullptr || ShouldAdvanceNestSpot( me ) )
	{
		if ( m_ctFindNestHint.HasStarted() && !m_ctFindNestHint.IsElapsed() )
		{
			return Action<CTFBot>::Continue();
		}
		m_ctFindNestHint.Start( RandomFloat( 1.0f, 2.0f ) );

		bool box_check       = false;
		bool out_of_range_ok = true;
		if ( me->HasAttribute( CTFBot::AttributeType::TELEPORTTOHINT ) )
		{
			if ( !m_bTeleportedToHint )
			{
				box_check = true;
			}
			out_of_range_ok = false;
		}

		CHandle<CTFBotHintEngineerNest> h_nest;
		if ( !CTFBotMvMEngineerHintFinder::FindHint( box_check, out_of_range_ok, &h_nest ) )
		{
			return Action<CTFBot>::Continue();
		}

		if ( m_hHintNest != nullptr )
		{
			m_hHintNest->SetOwnerEntity( nullptr );
		}

		m_hHintNest = h_nest;
		h_nest->SetOwnerEntity( me );

		m_hHintSentry = m_hHintNest->GetSentryHint();
		TakeOverStaleNest( m_hHintSentry, me );

		if ( !me->m_TeleportWhere.IsEmpty() )
		{
			m_hHintTele = m_hHintNest->GetTeleporterHint();
			TakeOverStaleNest( m_hHintTele, me );
		}
	}

	if ( !m_bTeleportedToHint &&
		 me->HasAttribute( CTFBot::AttributeType::TELEPORTTOHINT ) )
	{
		m_bTeleportedToHint = true;
		++m_nTeleportAttempts;

		return Action<CTFBot>::SuspendFor( new CTFBotMvMEngineerTeleportSpawn( m_hHintNest, ( m_nTeleportAttempts == 1 ) ),
			"In spawn area - teleport to the teleporter hint" );
	}

	CObjectSentrygun *sentry = nullptr;

	if ( m_hHintSentry != nullptr )
	{
		if ( m_hHintSentry->GetOwnerEntity() != nullptr &&
			 m_hHintSentry->GetOwnerEntity()->IsBaseObject() )
		{
/* sentry exists; start the retreat timer so we will retreat if we
 * should lose it in the future */
			m_ctSentryCooldown.Start( 3.0f );

			sentry = static_cast<CObjectSentrygun *>( m_hHintSentry->GetOwnerEntity() );
		}
		else
		{
			if ( m_hHintSentry->GetOwnerEntity() != nullptr &&
				 m_hHintSentry->GetOwnerEntity()->IsBaseObject() )
			{
/* not sure under what circumstances this code would actually be
 * reached; perhaps the static_cast in the if statement earlier
 * is actually an assert_cast and they were naively expecting it
 * to return nullptr if the owner wasn't a sentry? */
				sentry = static_cast<CObjectSentrygun *>( m_hHintSentry->GetOwnerEntity() );
				sentry->SetOwnerEntity( me );
			}
			else
			{
					 /* do not have a sentry; retreat for a few seconds if we had a
					  * sentry before this; then build a new sentry */

				if ( this->m_ctSentryCooldown.IsElapsed() )
				{
					return Action<CTFBot>::SuspendFor(
						new CTFBotMvMEngineerBuildSentryGun( m_hHintSentry ),
						"No sentry - building a new one" );
				}
				else
				{
					return Action<CTFBot>::SuspendFor(
						new CTFBotRetreatToCover( 1.0f ),
						"Lost my sentry - retreat!" );
				}
			}
		}

		/* NOTE: this is m_flHealth, not m_iHealth */
		if ( sentry->GetMaxHealth() > sentry->GetHealth() && !sentry->IsBuilding() )
		{
			this->m_ctSentrySafe.Start( 3.0f );
		}
	}

	CObjectTeleporter *tele = nullptr;

	if ( m_hHintTele != nullptr && m_ctSentrySafe.IsElapsed() )
	{
		if ( m_hHintTele->GetOwnerEntity() != nullptr &&
			 m_hHintTele->GetOwnerEntity()->IsBaseObject() )
		{
			m_ctTeleCooldown.Start( 3.0f );

			tele = static_cast<CObjectTeleporter *>( m_hHintTele->GetOwnerEntity() );
		}
		else
		{
			if ( m_ctTeleCooldown.IsElapsed() )
			{
				return Action<CTFBot>::SuspendFor(
					new CTFBotMvMEngineerBuildTeleportExit( m_hHintTele ),
					"Sentry is safe - building a teleport exit" );
			}
		}
	}

	if ( tele != nullptr && m_ctSentrySafe.IsElapsed() )
	{
/* NOTE: this is m_flHealth, not m_iHealth */
		if ( tele->GetMaxHealth() > tele->GetHealth() && !tele->IsBuilding() )
		{
			float dist = me->GetAbsOrigin().DistTo( tele->GetAbsOrigin() );

			if ( dist < 90.0f )
			{
				me->PressCrouchButton();
			}

			if ( m_ctRecomputePath.IsElapsed() )
			{
				m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

				Vector dir = ( tele->GetAbsOrigin() - me->GetAbsOrigin() );
				dir.NormalizeInPlace();

				Vector goal = tele->GetAbsOrigin() - ( 50.0f * dir );

				CTFBotPathCost cost_func( me, SAFEST_ROUTE );
				m_PathFollower.Compute( me, goal, cost_func, 0.0f, true );
			}

			m_PathFollower.Update( me );

			if ( dist < 75.0f )
			{
				me->GetBodyInterface()->AimHeadTowards( tele->WorldSpaceCenter(),
														   IBody::LookAtPriorityType::CRITICAL, 1.0f, nullptr, "Work on my Teleporter" );
				me->PressFireButton();
			}

			TryToDetonateStaleNest();
			return Action<CTFBot>::Continue();
		}
	}

	if ( sentry != nullptr )
	{
		float dist = me->GetAbsOrigin().DistTo( sentry->GetAbsOrigin() );

		if ( dist < 90.0f )
		{
			me->PressCrouchButton();
		}

		if ( m_ctRecomputePath.IsElapsed() )
		{
			m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

			Vector dir;
			AngleVectors( sentry->GetTurretAngles(), &dir );

			Vector goal = sentry->GetAbsOrigin() - ( 50.0f * dir );

			CTFBotPathCost cost_func( me, SAFEST_ROUTE );
			m_PathFollower.Compute( me, goal, cost_func, 0.0f, true );
		}

		m_PathFollower.Update( me );

		if ( dist < 75.0f )
		{
			me->GetBodyInterface()->AimHeadTowards( sentry->WorldSpaceCenter(),
													   IBody::LookAtPriorityType::CRITICAL, 1.0f, nullptr, "Work on my Sentry" );
			me->PressFireButton();
		}

		TryToDetonateStaleNest();
		return Action<CTFBot>::Continue();
	}

	TryToDetonateStaleNest();
	return Action<CTFBot>::Continue();
}

QueryResultType CTFBotMvMEngineerIdle::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}

QueryResultType CTFBotMvMEngineerIdle::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}

QueryResultType CTFBotMvMEngineerIdle::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	return ANSWER_NO;
}

bool CTFBotMvMEngineerIdle::ShouldAdvanceNestSpot( CTFBot *me )
{
	return false;
}

void CTFBotMvMEngineerIdle::TakeOverStaleNest( CBaseTFBotHintEntity *hint, CTFBot *me )
{
	if ( hint == nullptr || hint->OwnerObjectHasNoOwner() )
	{
		return;
	}

	CBaseObject *obj = static_cast<CBaseObject *>( hint->GetOwnerEntity() );
	obj->SetOwnerEntity( me );
	obj->SetBuilder( me );
	me->AddObject( obj );
}

void CTFBotMvMEngineerIdle::TryToDetonateStaleNest()
{
	if ( m_bTriedToDetonateStaleNest )
	{
		return;
	}

	if ( !m_hHintSentry || m_hHintSentry->OwnerObjectFinishBuilding() )
	{
		if ( !m_hHintTele || m_hHintTele->OwnerObjectFinishBuilding() )
		{
			CUtlVector<CTFBotHintEngineerNest *> nests;

			for ( int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i )
			{
				CBaseTFBotHintEntity *hint = static_cast<CBaseTFBotHintEntity *>(
					ITFBotHintEntityAutoList::AutoList()[i] );
				if ( hint->GetHintType() != CBaseTFBotHintEntity::HintType::ENGINEER_NEST || hint->IsDisabled() || hint->GetOwnerEntity() == nullptr )
				{
					continue;
				}

				nests.AddToTail( static_cast<CTFBotHintEngineerNest *>( hint ) );
			}

			FOR_EACH_VEC( nests, i )
			{
				if ( nests[i]->IsStaleNest() )
					nests[i]->DetonateStaleNest();
			}

			m_bTriedToDetonateStaleNest = true;
		}
	}
}


bool CTFBotMvMEngineerHintFinder::FindHint( bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint )
{
	CUtlVector<CTFBotHintEngineerNest *> hints;

	for ( int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i )
	{
		CBaseTFBotHintEntity *hint = static_cast<CBaseTFBotHintEntity *>(
			ITFBotHintEntityAutoList::AutoList()[i] );

		if ( hint->GetHintType() == CBaseTFBotHintEntity::HintType::ENGINEER_NEST &&
			 !hint->IsDisabled() && hint->GetOwnerEntity() == nullptr )
		{
			hints.AddToTail( static_cast<CTFBotHintEngineerNest *>( hint ) );
		}
	}

	BombInfo_t bomb_info;
	GetBombInfo( &bomb_info );

	CUtlVector<CTFBotHintEngineerNest *> hints1; // in fwd~back, stale
	CUtlVector<CTFBotHintEngineerNest *> hints2; // in fwd~back, within min bomb distance
	CUtlVector<CTFBotHintEngineerNest *> hints3; // in fwd~infinity
	CUtlVector<CTFBotHintEngineerNest *> hints4; // others

	FOR_EACH_VEC( hints, i ) {
		CTFBotHintEngineerNest *hint = hints[i];

		CTFNavArea *area = assert_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( hint->GetAbsOrigin() ) );
		if ( area == nullptr )
		{
			Warning( "Sentry hint has NULL nav area!\n" );
			continue;
		}

		float dist = area->GetBombTargetDistance();
		if ( dist > bomb_info.hatch_dist_fwd && dist < bomb_info.hatch_dist_back )
		{
			CBaseEntity *pList[256];
			if ( box_check && UTIL_EntitiesInBox( pList, 256,
												  hint->GetAbsOrigin() + VEC_HULL_MIN,
												  hint->GetAbsOrigin() + VEC_HULL_MAX,
												  FL_OBJECT | FL_FAKECLIENT ) > 0 )
			{
				continue;
			}

			if ( hint->IsStaleNest() )
			{
				hints1.AddToTail( hint );
			}
			else
			{
				if ( hint->GetAbsOrigin().DistTo( bomb_info.closest_pos ) >=
					 tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat() )
				{
					hints2.AddToTail( hint );
				}
			}
		}
		else if ( dist > bomb_info.hatch_dist_back )
		{
			hints3.AddToTail( hint );
		}
		else
		{
			hints4.AddToTail( hint );
		}
	}

	CTFBotHintEngineerNest *hint = nullptr;
	if ( !hints1.IsEmpty() )
	{
		hint = hints1.Random();
	}
	else if ( !hints2.IsEmpty() )
	{
		hint = hints2.Random();
	}
	else if ( out_of_range_ok )
	{
		hint = SelectOutOfRangeNest( hints3 );
		if ( hint == nullptr )
		{
			hint = SelectOutOfRangeNest( hints4 );
		}
	}

	if ( the_hint != nullptr )
	{
		*the_hint = hint;
	}

	return ( hint != nullptr );
}

CTFBotHintEngineerNest *SelectOutOfRangeNest( const CUtlVector<CTFBotHintEngineerNest *> &nests )
{
	if( nests.IsEmpty() )
		return nullptr;

	FOR_EACH_VEC( nests, i )
	{
		CTFBotHintEngineerNest *pHint = nests[i];
		if ( pHint->IsStaleNest() )
			return pHint;
	}

	return nests.Random();
}

bool GetBombInfo( BombInfo_t *info )
{
	float hatch_dist = 0.0f;

	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		if ( area->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM | TF_NAV_RED_SPAWN_ROOM ) )
			continue;

		hatch_dist = Max( Max( area->GetBombTargetDistance(), hatch_dist ), 0.0f );
	}

	CCaptureFlag *closest_flag = nullptr;
	Vector closest_flag_pos;

	for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
	{
		CCaptureFlag *flag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );

		Vector flag_pos;

		CTFPlayer *owner = ToTFPlayer( flag->GetOwnerEntity() );
		if ( owner != nullptr )
		{
			flag_pos = owner->GetAbsOrigin();
		}
		else
		{
			flag_pos = flag->WorldSpaceCenter();
		}

		CTFNavArea *area = assert_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( flag_pos ) );
		if ( area != nullptr && area->GetBombTargetDistance() < hatch_dist )
		{
			closest_flag = flag;
			hatch_dist = area->GetBombTargetDistance();
			closest_flag_pos = flag_pos;
		}
	}

	bool success = ( closest_flag != nullptr );

	float range_back = tf_bot_engineer_mvm_sentry_hint_bomb_backward_range.GetFloat();
	float range_fwd  = tf_bot_engineer_mvm_sentry_hint_bomb_forward_range.GetFloat();

	if ( info != nullptr )
	{
		info->closest_pos = closest_flag_pos;
		info->hatch_dist_back = hatch_dist + range_back;
		info->hatch_dist_fwd  = hatch_dist - range_fwd;
	}

	return success;
}
