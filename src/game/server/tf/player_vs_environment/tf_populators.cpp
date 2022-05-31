//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_team.h"
#include "tf_obj_teleporter.h"
#include "tf_obj_sentrygun.h"
#include "tf_populators.h"
#include "tf_population_manager.h"
#include "eventqueue.h"
#include "tier1/UtlSortVector.h"
#include "tf_objective_resource.h"
#include "tf_tank_boss.h"
#include "tf_mann_vs_machine_stats.h"

extern ConVar tf_populator_debug;
extern ConVar tf_populator_active_buffer_range;

ConVar tf_mvm_engineer_teleporter_uber_duration( "tf_mvm_engineer_teleporter_uber_duration", "5.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_mvm_currency_bonus_ratio_min( "tf_mvm_currency_bonus_ratio_min", "0.95f", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The minimum percentage of wave money players must collect in order to qualify for min bonus - 0.1 to 1.0.  Half the bonus amount will be awarded when reaching min ratio, and half when reaching max.", true, 0.1, true, 1.0 );
ConVar tf_mvm_currency_bonus_ratio_max( "tf_mvm_currency_bonus_ratio_max", "1.f", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "The highest percentage of wave money players must collect in order to qualify for max bonus - 0.1 to 1.0.  Half the bonus amount will be awarded when reaching min ratio, and half when reaching max.", true, 0.1, true, 1.0 );

static CHandle<CBaseEntity> s_lastTeleporter = NULL;
static float s_flLastTeleportTime = -1;

LINK_ENTITY_TO_CLASS( populator_internal_spawn_point, CPopulatorInternalSpawnPoint );
CHandle<CPopulatorInternalSpawnPoint> g_internalSpawnPoint = NULL;

class CTFNavAreaIncursionLess
{
public:
	bool Less( const CTFNavArea *a, const CTFNavArea *b, void *pCtx )
	{
		return a->GetIncursionDistance( TF_TEAM_BLUE ) < b->GetIncursionDistance( TF_TEAM_BLUE );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Fire off output events
//-----------------------------------------------------------------------------
void FireEvent( EventInfo *eventInfo, const char *eventName )
{
	if ( eventInfo )
	{
		CBaseEntity *targetEntity = gEntList.FindEntityByName( NULL, eventInfo->m_target );
		if ( !targetEntity )
		{
			Warning( "WaveSpawnPopulator: Can't find target entity '%s' for %s\n", eventInfo->m_target.Access(), eventName );
		}
		else
		{
			g_EventQueue.AddEvent( targetEntity, eventInfo->m_action, 0.0f, NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create output event pairings
//-----------------------------------------------------------------------------
EventInfo *ParseEvent( KeyValues *data )
{
	EventInfo *eventInfo = new EventInfo();

	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !Q_stricmp( pszKey, "Target" ) )
		{
			eventInfo->m_target.sprintf( "%s", pSubKey->GetString() );
		}
		else if ( !Q_stricmp( pszKey, "Action" ) )
		{
			eventInfo->m_action.sprintf( "%s", pSubKey->GetString() );
		}
		else
		{
			Warning( "Unknown field '%s' in WaveSpawn event definition.\n", pSubKey->GetString() );
			delete eventInfo;
			return NULL;
		}
	}

	return eventInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SpawnLocationResult DoTeleporterOverride( CBaseEntity *spawnEnt, Vector *vSpawnPosition, bool bClosestPointOnNav )
{
	CUtlVector<CBaseEntity *> activeTeleporters;
	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *pObj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->GetType() != OBJ_TELEPORTER || pObj->GetTeamNumber() != TF_TEAM_MVM_BOTS )
			continue;

		if ( pObj->IsBuilding() || pObj->HasSapper() || pObj->IsDisabled() )
			continue;

		CObjectTeleporter *teleporter = assert_cast<CObjectTeleporter *>( pObj );
		const CUtlStringList &teleportWhereNames = teleporter->m_TeleportWhere;

		const char *pszSpawnPointName = STRING( spawnEnt->GetEntityName() );
		for ( int iTelePoints =0; iTelePoints < teleportWhereNames.Count(); ++iTelePoints )
		{
			if ( !V_stricmp( teleportWhereNames[ iTelePoints ], pszSpawnPointName ) )
			{
				activeTeleporters.AddToTail( teleporter );
				break;
			}
		}
	}

	if ( activeTeleporters.Count() > 0 )
	{
		int which = RandomInt( 0, activeTeleporters.Count() - 1 );
		*vSpawnPosition = activeTeleporters[ which ]->WorldSpaceCenter();
		s_lastTeleporter = activeTeleporters[ which ];
		return SPAWN_LOCATION_TELEPORTER;
	}

	CTFNavArea *pArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( spawnEnt->WorldSpaceCenter() );
	if ( pArea )
	{
		if ( bClosestPointOnNav )
		{
			pArea->GetClosestPointOnArea( spawnEnt->WorldSpaceCenter(), vSpawnPosition );
		}
		else
		{
			*vSpawnPosition = pArea->GetCenter();
		}

		return SPAWN_LOCATION_NORMAL;
	}

	return SPAWN_LOCATION_NOT_FOUND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OnBotTeleported( CTFBot *pBot )
{
	if ( gpGlobals->curtime - s_flLastTeleportTime > 0.1f )
	{
		s_lastTeleporter->EmitSound( "MVM.Robot_Teleporter_Deliver" );
		s_flLastTeleportTime = gpGlobals->curtime;
	}

	// force bot to face in the direction specified by the teleporter
	Vector vForward;
	AngleVectors( s_lastTeleporter->GetAbsAngles(), &vForward, NULL, NULL );
	pBot->GetLocomotionInterface()->FaceTowards( pBot->GetAbsOrigin() + 50 * vForward );

	// spy shouldn't get any effect from the teleporter
	if ( !pBot->IsPlayerClass( TF_CLASS_SPY ) )
	{
		pBot->TeleportEffect();

		// invading bots get uber while they leave their spawn so they don't drop their cash where players can't pick it up
		float flUberTime = tf_mvm_engineer_teleporter_uber_duration.GetFloat();
		pBot->m_Shared.AddCond( TF_COND_INVULNERABLE, flUberTime );
		pBot->m_Shared.AddCond( TF_COND_INVULNERABLE_WEARINGOFF, flUberTime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpawnLocation::CSpawnLocation()
{
	m_nRandomSeed = RandomInt( 0, 9999 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSpawnLocation::Parse( KeyValues *data )
{
	const char *pszKey = data->GetName();
	const char *pszValue = data->GetString();

	if ( !V_stricmp( pszKey, "Where" ) || !V_stricmp( pszKey, "ClosestPoint" ) )
	{
		if ( !V_stricmp( pszValue, "Ahead" ) )
		{
			m_eRelative = AHEAD;
		}
		else if ( !V_stricmp( pszValue, "Behind" ) )
		{
			m_eRelative = BEHIND;
		}
		else if ( !V_stricmp( pszValue, "Anywhere" ) )
		{
			m_eRelative = ANYWHERE;
		}
		else
		{
			m_bClosestPointOnNav = V_stricmp( pszKey, "ClosestPoint" ) == 0;

			// collect entities with given name
			bool bFound = false;
			for ( int i=0; i < ITFTeamSpawnAutoList::AutoList().Count(); ++i )
			{
				CTFTeamSpawn *pTeamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[i] );
				if ( !V_stricmp( STRING( pTeamSpawn->GetEntityName() ), pszValue ) )
				{
					m_TeamSpawns.AddToTail( pTeamSpawn );
					bFound = true;
				}
			}

			if ( !bFound )
			{
				Warning( "Invalid Where argument '%s'\n", pszValue );
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SpawnLocationResult CSpawnLocation::FindSpawnLocation( Vector *vSpawnPosition )
{
	CUtlVector< CHandle<CTFTeamSpawn> > activeSpawns;
	FOR_EACH_VEC( m_TeamSpawns, i )
	{
		if ( m_TeamSpawns[i]->IsDisabled() )
			continue;

		activeSpawns.AddToTail( m_TeamSpawns[i] );
	}

	if ( m_nSpawnCount >= activeSpawns.Count() )
	{
		m_nRandomSeed = RandomInt( 0, 9999 );
		m_nSpawnCount = 0;
	}
	CUniformRandomStream randomSpawn;
	randomSpawn.SetSeed( m_nRandomSeed );
	activeSpawns.Shuffle( &randomSpawn );

	if ( activeSpawns.Count() > 0 )
	{
		SpawnLocationResult result = DoTeleporterOverride( activeSpawns[ m_nSpawnCount ], vSpawnPosition, m_bClosestPointOnNav );
		if ( result != SPAWN_LOCATION_NOT_FOUND )
		{
			m_nSpawnCount++;
			return result;
		}
	}

	CTFNavArea *spawnArea = SelectSpawnArea();
	if ( spawnArea )
	{
		*vSpawnPosition = spawnArea->GetCenter();
		return SPAWN_LOCATION_NORMAL;
	}

	return SPAWN_LOCATION_NOT_FOUND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFNavArea *CSpawnLocation::SelectSpawnArea( void ) const
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( m_eRelative == UNDEFINED )
		return nullptr;

	CUtlSortVector<CTFNavArea *, CTFNavAreaIncursionLess> theaterAreas;

	CUtlVector<INextBot *> bots;
	TheNextBots().CollectAllBots( &bots );

	CTFNavArea::MakeNewTFMarker();
	FOR_EACH_VEC( bots, i )
	{
		CTFBot *pBot = ToTFBot( bots[i]->GetEntity() );
		if ( pBot == nullptr )
			continue;

		if ( !pBot->IsAlive() )
			continue;

		if ( !pBot->GetLastKnownArea() )
			continue;

		CUtlVector<CTFNavArea *> nearbyAreas;
		CollectSurroundingAreas( &nearbyAreas, pBot->GetLastKnownArea(), tf_populator_active_buffer_range.GetFloat() );

		FOR_EACH_VEC( nearbyAreas, j )
		{
			CTFNavArea *pArea = nearbyAreas[i];
			if ( !pArea->IsTFMarked() )
			{
				pArea->TFMark();

				if ( pArea->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
					continue;

				if ( !pArea->IsValidForWanderingPopulation() )
					continue;

				theaterAreas.Insert( pArea );

				if ( tf_populator_debug.GetBool() )
					TheNavMesh->AddToSelectedSet( pArea );
			}
		}
	}

	if ( theaterAreas.Count() == 0 )
	{
		if ( tf_populator_debug.GetBool() )
			DevMsg( "%3.2f: SelectSpawnArea: Empty theater!\n", gpGlobals->curtime );

		return nullptr;
	}

	for ( int i=0; i < 5; ++i )
	{
		int which = 0;
		switch ( m_eRelative )
		{
			case AHEAD:
				which = Max( RandomFloat( 0.0f, 1.0f ), RandomFloat( 0.0f, 1.0f ) ) * theaterAreas.Count();
				break;

			case BEHIND:
				which = ( 1.0f - Max( RandomFloat( 0.0f, 1.0f ), RandomFloat( 0.0f, 1.0f ) ) ) * theaterAreas.Count();
				break;

			case ANYWHERE:
				which = RandomFloat( 0.0f, 1.0f ) * theaterAreas.Count();
				break;
		}

		if ( which >= theaterAreas.Count() )
			which = theaterAreas.Count() - 1;

		return theaterAreas[which];
	}

	return nullptr;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMissionPopulator::CMissionPopulator( CPopulationManager *pManager )
	: m_pManager( pManager ), m_pSpawner( NULL )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMissionPopulator::~CMissionPopulator()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMissionPopulator::Parse( KeyValues *data )
{
	int nWaveDuration = 99999;
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "Objective" ) )
		{
			if ( !V_stricmp( pSubKey->GetString(), "DestroySentries" ) )
			{
				m_eMission = CTFBot::MissionType::DESTROY_SENTRIES;
			}
			else if ( !V_stricmp( pSubKey->GetString(), "Sniper" ) )
			{
				m_eMission = CTFBot::MissionType::SNIPER;
			}
			else if ( !V_stricmp( pSubKey->GetString(), "Spy" ) )
			{
				m_eMission = CTFBot::MissionType::SPY;
			}
			else if ( !V_stricmp( pSubKey->GetString(), "Engineer" ) )
			{
				m_eMission = CTFBot::MissionType::ENGINEER;
			}
			else if ( !V_stricmp( pSubKey->GetString(), "SeekAndDestroy" ) )
			{
				m_eMission = CTFBot::MissionType::DESTROY_SENTRIES;
			}
			else
			{
				Warning( "Invalid mission '%s'\n", data->GetString() );
				return false;
			}
		}
		else if ( !V_stricmp( pszKey, "InitialCooldown" ) )
		{
			m_flInitialCooldown = data->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "CooldownTime" ) )
		{
			m_flCooldownDuration = data->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "BeginAtWave" ) )
		{
			m_nStartWave = data->GetInt() - 1;
		}
		else if ( !V_stricmp( pszKey, "RunForThisManyWaves" ) )
		{
			nWaveDuration = data->GetInt();
		}
		else if ( !V_stricmp( pszKey, "DesiredCount" ) )
		{
			m_nDesiredCount = data->GetInt();
		}
		else
		{
			m_pSpawner = IPopulationSpawner::ParseSpawner( this, pSubKey );
			if ( m_pSpawner == NULL )
			{
				Warning( "Unknown attribute '%s' in Mission definition.\n", pszKey );
			}
		}
	}

	m_nEndWave = m_nStartWave + nWaveDuration;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissionPopulator::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( TFGameRules()->InSetup() || TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
	{
		m_eState = NOT_STARTED;
		return;
	}

	if ( m_pManager->m_nCurrentWaveIndex < m_nStartWave || m_pManager->m_nCurrentWaveIndex >= m_nEndWave )
	{
		m_eState = NOT_STARTED;
		return;
	}

	if ( m_eState == NOT_STARTED )
	{
		if ( m_flInitialCooldown > 0.0f )
		{
			m_eState = INITIAL_COOLDOWN;
			m_cooldownTimer.Start( m_flInitialCooldown );
			return;
		}

		m_eState = RUNNING;
		m_cooldownTimer.Invalidate();
	}
	else if ( m_eState == INITIAL_COOLDOWN )
	{
		if ( !m_cooldownTimer.IsElapsed() )
		{
			return;
		}

		m_eState = RUNNING;
		m_cooldownTimer.Invalidate();
	}

	if ( m_eMission == CTFBot::MissionType::DESTROY_SENTRIES )
	{
		UpdateMissionDestroySentries();
	}
	else if ( m_eMission >= CTFBot::MissionType::SNIPER && m_eMission <= CTFBot::MissionType::ENGINEER )
	{
		UpdateMission( m_eMission );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMissionPopulator::UnpauseSpawning( void )
{
	m_cooldownTimer.Start( m_flCooldownDuration );
	m_checkSentriesTimer.Start( RandomFloat( 5.0f, 10.0f ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMissionPopulator::UpdateMission( CTFBot::MissionType mission )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	// TODO: Move away from depending on players
	CUtlVector<CTFPlayer *> bots;
	CollectPlayers( &bots, TF_TEAM_MVM_BOTS, true );

	int nActiveMissions = 0;
	FOR_EACH_VEC( bots, i )
	{
		CTFBot *pBot = ToTFBot( bots[i] );
		if ( pBot )
		{
			if ( pBot->HasMission( mission ) )
				nActiveMissions++;
		}
	}

	if ( g_pPopulationManager->IsSpawningPaused() )
		return false;

	if ( nActiveMissions > 0 )
	{
		m_cooldownTimer.Start( m_flCooldownDuration );

		return false;
	}

	if ( !m_cooldownTimer.IsElapsed() )
		return false;

	int nCurrentBotCount = GetGlobalTeam( TF_TEAM_MVM_BOTS )->GetNumPlayers();
	if ( nCurrentBotCount + m_nDesiredCount > k_nMvMBotTeamSize )
	{
		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "MANN VS MACHINE: %3.2f: Waiting for slots to spawn mission.\n", gpGlobals->curtime );
		}

		return false;
	}

	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "MANN VS MACHINE: %3.2f: <<<< Spawning Mission >>>>\n", gpGlobals->curtime );
	}

	int nSniperCount = 0;
	FOR_EACH_VEC( bots, i )
	{
		CTFBot *pBot = ToTFBot( bots[i] );
		if ( pBot && pBot->IsPlayerClass( TF_CLASS_SNIPER ) )
			nSniperCount++;
	}

	for ( int i=0; i < m_nDesiredCount; ++i )
	{
		Vector vecSpawnPos;
		SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( &vecSpawnPos );
		if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
		{
			CUtlVector<EHANDLE> spawnedBots;
			if ( m_pSpawner && m_pSpawner->Spawn( vecSpawnPos, &spawnedBots ) )
			{
				FOR_EACH_VEC( spawnedBots, j )
				{
					CTFBot *pBot = ToTFBot( spawnedBots[j] );
					if ( pBot == NULL )
						continue;

					pBot->SetFlagTarget( NULL );
					pBot->SetMission( mission );

					if ( TFObjectiveResource() )
					{
						unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
						if ( pBot->IsMiniBoss() )
						{
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;
						}
						if ( pBot->HasAttribute( CTFBot::AttributeType::ALWAYSCRIT ) )
						{
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
						}
						TFObjectiveResource()->IncrementMannVsMachineWaveClassCount( m_pSpawner->GetClassIcon( j ), iFlags );
					}

					if ( TFGameRules()->IsMannVsMachineMode() )
					{
						if ( pBot->HasMission( CTFBot::MissionType::SNIPER ) )
						{
							nSniperCount++;

							if ( nSniperCount == 1 )
								TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SNIPER_CALLOUT, TF_TEAM_MVM_BOTS );
						}
					}

					if ( spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
						OnBotTeleported( pBot );
				}
			}
		}
		else if ( tf_populator_debug.GetBool() )
		{
			Warning( "MissionPopulator: %3.2f: Skipped a member - can't find a place to spawn\n", gpGlobals->curtime );
		}
	}

	m_cooldownTimer.Start( m_flCooldownDuration );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMissionPopulator::UpdateMissionDestroySentries( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( !m_cooldownTimer.IsElapsed() || !m_checkSentriesTimer.IsElapsed() )
		return false;

	if ( g_pPopulationManager->IsSpawningPaused() )
		return false;

	m_checkSentriesTimer.Start( RandomFloat( 5.0f, 10.0f ) );

	int nDmgLimit = 0;
	int nKillLimit = 0;
	m_pManager->GetSentryBusterDamageAndKillThreshold( nDmgLimit, nKillLimit );

	CUtlVector<CObjectSentrygun *> dangerousSentries;
	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *pObj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() != OBJ_SENTRYGUN )
			continue;
		
		if ( pObj->IsDisposableBuilding() )
			continue;

		if ( pObj->GetTeamNumber() != TF_TEAM_MVM_BOTS )
			continue;

		CTFPlayer *pOwner = pObj->GetOwner();
		if ( pOwner )
		{
			int nDmgDone = pOwner->GetAccumulatedSentryGunDamageDealt();
			int nKillsMade = pOwner->GetAccumulatedSentryGunKillCount();

			if ( nDmgDone >= nDmgLimit || nKillsMade >= nKillLimit )
			{
				dangerousSentries.AddToTail( static_cast<CObjectSentrygun *>( pObj ) );
			}
		}
	}

	// TODO: Move away from depending on players
	CUtlVector<CTFPlayer *> bots;
	CollectPlayers( &bots, TF_TEAM_MVM_BOTS, true );

	bool bSpawned = false;
	FOR_EACH_VEC( dangerousSentries, i )
	{
		CObjectSentrygun *pSentry = dangerousSentries[i];

		int nValidCount = 0;
		FOR_EACH_VEC( bots, j )
		{
			CTFBot *pBot = ToTFBot( bots[j] );
			if ( pBot )
			{
				if ( pBot->HasMission( CTFBot::MissionType::DESTROY_SENTRIES ) && pBot->GetMissionTarget() == pSentry )
					break;
			}

			nValidCount++;
		}

		if ( nValidCount < bots.Count() )
			continue;

		Vector vecSpawnPos;
		SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( &vecSpawnPos );
		if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
		{
			CUtlVector<EHANDLE> spawnedBots;
			if ( m_pSpawner && m_pSpawner->Spawn( vecSpawnPos, &spawnedBots ) )
			{
				if ( tf_populator_debug.GetBool() )
				{
					DevMsg( "MANN VS MACHINE: %3.2f: <<<< Spawning Sentry Busting Mission >>>>\n", gpGlobals->curtime );
				}

				FOR_EACH_VEC( spawnedBots, k )
				{
					CTFBot *pBot = ToTFBot( spawnedBots[k] );
					if ( pBot == NULL )
						continue;

					bSpawned = true;

					pBot->SetMission( CTFBot::MissionType::DESTROY_SENTRIES );
					pBot->SetMissionTarget( pSentry );
					pBot->SetFlagTarget( NULL );
					pBot->SetBloodColor( DONT_BLEED );

					pBot->Update();

					pBot->GetPlayerClass()->SetCustomModel( "models/bots/demo/bot_sentry_buster.mdl", true );
					pBot->UpdateModel();

					if ( TFObjectiveResource() )
					{
						unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
						if ( pBot->IsMiniBoss() )
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;

						if ( pBot->HasAttribute( CTFBot::AttributeType::ALWAYSCRIT ) )
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;

						TFObjectiveResource()->IncrementMannVsMachineWaveClassCount( m_pSpawner->GetClassIcon( k ), iFlags );
					}

					if ( TFGameRules() )
						TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_SENTRY_BUSTER, TF_TEAM_MVM_BOTS );

					if ( spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
						OnBotTeleported( pBot );
				}
			}
		}
		else if ( tf_populator_debug.GetBool() )
		{
			Warning( "MissionPopulator: %3.2f: Can't find a place to spawn a sentry destroying squad\n", gpGlobals->curtime );
		}
	}

	if ( bSpawned )
	{
		float flCoolDown = m_flCooldownDuration;

		CWave *pWave = m_pManager->GetCurrentWave();
		if ( pWave )
		{
			pWave->m_nNumSentryBustersKilled++;

			if ( TFGameRules() )
			{
				if ( pWave->m_nNumSentryBustersKilled > 1 )
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Sentry_Buster_Alert_Another" );
				}
				else
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Sentry_Buster_Alert" );
				}
			}

			flCoolDown = m_flCooldownDuration + pWave->m_nNumSentryBustersKilled * m_flCooldownDuration;

			pWave->m_nNumSentryBustersKilled = 0;;
		}

		m_cooldownTimer.Start( flCoolDown );
	}

	return bSpawned;
}


int CWaveSpawnPopulator::sm_reservedPlayerSlotCount = 0;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWaveSpawnPopulator::CWaveSpawnPopulator( CPopulationManager *pManager )
	: m_pManager( pManager ), m_pSpawner( NULL )
{
	m_iMaxActive = 999;
	m_nSpawnCount = 1;
	m_iTotalCurrency = -1;

	m_startWaveEvent = NULL;
	m_firstSpawnEvent = NULL;
	m_lastSpawnEvent = NULL;
	m_doneEvent = NULL;
	m_parentWave = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWaveSpawnPopulator::~CWaveSpawnPopulator()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}

	delete m_startWaveEvent;
	delete m_firstSpawnEvent;
	delete m_lastSpawnEvent;
	delete m_doneEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWaveSpawnPopulator::Parse( KeyValues *data )
{
	KeyValues *pTemplate = data->FindKey( "Template" );
	if ( pTemplate )
	{
		KeyValues *pTemplateKV = m_pManager->GetTemplate( pTemplate->GetString() );
		if ( pTemplateKV )
		{
			if ( !Parse( pTemplateKV ) )
				return false;
		}
		else
		{
			Warning( "Unknown Template '%s' in WaveSpawn definition\n", pTemplate->GetString() );
		}
	}
	
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		if ( m_where.Parse( pSubKey ) )
			continue;

		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "TotalCount" ) )
		{
			m_iTotalCount = data->GetInt();
		}
		else if ( !V_stricmp( pszKey, "MaxActive" ) )
		{
			m_iMaxActive = data->GetInt();
		}
		else if ( !V_stricmp( pszKey, "SpawnCount" ) )
		{
			m_nSpawnCount = data->GetInt();
		}
		else if ( !V_stricmp( pszKey, "WaitBeforeStarting" ) )
		{
			m_flWaitBeforeStarting = data->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "WaitBetweenSpawns" ) )
		{
			if ( m_flWaitBetweenSpawns != 0 && m_bWaitBetweenSpawnsAfterDeath )
			{
				Warning( "Already specified WaitBetweenSpawnsAfterDeath time, WaitBetweenSpawns won't be used\n" );
				continue;
			}

			m_flWaitBetweenSpawns = data->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "WaitBetweenSpawnsAfterDeath" ) )
		{
			if ( m_flWaitBetweenSpawns != 0 )
			{
				Warning( "Already specified WaitBetweenSpawns time, WaitBetweenSpawnsAfterDeath won't be used\n" );
				continue;
			}

			m_bWaitBetweenSpawnsAfterDeath = true;
			m_flWaitBetweenSpawns = data->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "StartWaveWarningSound" ) )
		{
			m_startWaveWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !V_stricmp( pszKey, "StartWaveOutput" ) )
		{
			m_startWaveEvent = ParseEvent( data );
		}
		else if ( !V_stricmp( pszKey, "FirstSpawnWarningSound" ) )
		{
			m_firstSpawnWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !V_stricmp( pszKey, "FirstSpawnOutput" ) )
		{
			m_firstSpawnEvent = ParseEvent( data );
		}
		else if ( !V_stricmp( pszKey, "LastSpawnWarningSound" ) )
		{
			m_lastSpawnWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !V_stricmp( pszKey, "LastSpawnOutput" ) )
		{
			m_lastSpawnEvent = ParseEvent( data );
		}
		else if ( !V_stricmp( pszKey, "DoneWarningSound" ) )
		{
			m_doneWarningSound.sprintf( "%s", data->GetString() );
		}
		else if ( !V_stricmp( pszKey, "DoneOutput" ) )
		{
			m_doneEvent = ParseEvent( data );
		}
		else if ( !V_stricmp( pszKey, "TotalCurrency" ) )
		{
			m_iTotalCurrency = data->GetInt();
		}
		else if ( !V_stricmp( pszKey, "Name" ) )
		{
			m_name = data->GetString();
		}
		else if ( !V_stricmp( pszKey, "WaitForAllSpawned" ) )
		{
			m_szWaitForAllSpawned = data->GetString();
		}
		else if ( !V_stricmp( pszKey, "WaitForAllDead" ) )
		{
			m_szWaitForAllDead = data->GetString();
		}
		else if ( !V_stricmp( pszKey, "Support" ) )
		{
			m_bLimitedSupport = !V_stricmp( data->GetString(), "Limited" );
			m_bSupportWave = true;
		}
		else if ( !V_stricmp( pszKey, "RandomSpawn" ) )
		{
			m_bRandomSpawn = data->GetBool();
		}
		else
		{
			m_pSpawner = IPopulationSpawner::ParseSpawner( this, data );

			if ( m_pSpawner == NULL )
			{
				Warning( "Unknown attribute '%s' in WaveSpawn definition.\n", pszKey );
			}
		}

		m_iRemainingCurrency = m_iTotalCurrency;
		m_iRemainingCount = m_iTotalCount;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveSpawnPopulator::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	switch ( m_eState )
	{
		case PENDING:
		{
			m_timer.Start( m_flWaitBeforeStarting );

			SetState( PRE_SPAWN_DELAY );

			sm_reservedPlayerSlotCount = 0;

			if ( m_startWaveWarningSound.Length() > 0 )
				TFGameRules()->BroadcastSound( 255, m_startWaveWarningSound );

			FireEvent( m_startWaveEvent, "StartWaveOutput" );

			if ( tf_populator_debug.GetBool() )
			{
				DevMsg( "%3.2f: WaveSpawn(%s) started PRE_SPAWN_DELAY\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
			}
		}
		break;
		case PRE_SPAWN_DELAY:
		{
			if ( !m_timer.IsElapsed() )
				return;

			m_nNumSpawnedSoFar = 0;
			m_nReservedPlayerSlots = 0;

			SetState( SPAWNING );

			if ( m_firstSpawnWarningSound.Length() > 0 )
				TFGameRules()->BroadcastSound( 255, m_firstSpawnWarningSound );

			FireEvent( m_firstSpawnEvent, "FirstSpawnOutput" );

			if ( tf_populator_debug.GetBool() )
			{
				DevMsg( "%3.2f: WaveSpawn(%s) started SPAWNING\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
			}
		}
		break;
		case SPAWNING:
		{
			if ( !m_timer.IsElapsed() || g_pPopulationManager->IsSpawningPaused() )
				return;

			if ( !m_pSpawner )
			{
				Warning( "Invalid spawner\n" );
				SetState( DONE );

				return;
			}

			int nNumActive = 0;
			FOR_EACH_VEC( m_activeSpawns, i )
			{
				if ( m_activeSpawns[i] && m_activeSpawns[i]->IsAlive() )
					nNumActive++;
			}

			if ( m_bWaitBetweenSpawnsAfterDeath )
			{
				if ( nNumActive != 0 )
				{
					return;
				}
				else
				{
					if ( m_spawnLocationResult )
					{
						m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;

						if ( m_flWaitBetweenSpawns > 0 )
							m_timer.Start( m_flWaitBetweenSpawns );

						return;
					}
				}
			}

			if ( nNumActive >= m_iMaxActive )
				return;

			if ( m_nReservedPlayerSlots <= 0 )
			{
				if ( nNumActive - m_iMaxActive < m_nSpawnCount )
					return;

				int nTotalBotCount = GetGlobalTeam( TF_TEAM_MVM_BOTS )->GetNumPlayers();
				if ( nTotalBotCount + m_nSpawnCount + sm_reservedPlayerSlotCount > k_nMvMBotTeamSize )
					return;

				sm_reservedPlayerSlotCount += m_nSpawnCount;
				m_nReservedPlayerSlots = m_nSpawnCount;
			}

			Vector vecSpawnPos = vec3_origin;
			if ( m_pSpawner && m_pSpawner->IsWhereRequired() )
			{
				if ( m_spawnLocationResult == SPAWN_LOCATION_NOT_FOUND || m_spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
				{
					m_spawnLocationResult = m_where.FindSpawnLocation( &m_vecSpawnPosition );
					if ( m_spawnLocationResult == SPAWN_LOCATION_NOT_FOUND )
						return;
				}

				vecSpawnPos = m_vecSpawnPosition;

				if ( m_bRandomSpawn )
					m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;
			}

			CUtlVector<EHANDLE> spawnedBots;
			if ( m_pSpawner && m_pSpawner->Spawn( vecSpawnPos, &spawnedBots ) )
			{
				FOR_EACH_VEC( spawnedBots, i )
				{
					CTFBot *pBot = ToTFBot( spawnedBots[i] );
					if ( pBot )
					{
						pBot->SetCurrency( 0 );
						pBot->m_pWaveSpawnPopulator = this;

						TFObjectiveResource()->SetMannVsMachineWaveClassActive( pBot->GetPlayerClass()->GetClassIconName() );

						if ( m_bLimitedSupport )
							pBot->m_bLimitedSupport = true;

						if ( m_spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
							OnBotTeleported( pBot );
					}
					else
					{
						CTFTankBoss *pTank = dynamic_cast<CTFTankBoss *>( spawnedBots[i].Get() );
						if ( pTank )
						{
							pTank->SetCurrencyValue( 0 );
							pTank->SetWaveSpawnPopulator( this );

							m_parentWave->m_nNumTanksSpawned++;
						}
					}
				}

				int nNumSpawned = spawnedBots.Count();
				m_nNumSpawnedSoFar += nNumSpawned;

				int nNumSlotsToFree = ( nNumSpawned <= m_nReservedPlayerSlots ) ? nNumSpawned : m_nReservedPlayerSlots;
				m_nReservedPlayerSlots -= nNumSlotsToFree;
				sm_reservedPlayerSlotCount -= nNumSlotsToFree;

				FOR_EACH_VEC( spawnedBots, i )
				{
					CBaseEntity *pEntity = spawnedBots[i];

					FOR_EACH_VEC( m_activeSpawns, j )
					{
						if ( m_activeSpawns[j] && m_activeSpawns[j]->entindex() == pEntity->entindex() )
						{
							Warning( "WaveSpawn duplicate entry in active vector\n" );
							continue;
						}
					}

					m_activeSpawns.AddToTail( pEntity );
				}

				if ( IsFinishedSpawning() )
				{
					SetState( WAIT_FOR_ALL_DEAD );
					return;
				}

				if ( m_nReservedPlayerSlots <= 0 && !m_bWaitBetweenSpawnsAfterDeath )
				{
					m_spawnLocationResult = SPAWN_LOCATION_NOT_FOUND;

					if ( m_flWaitBetweenSpawns > 0 )
						m_timer.Start( m_flWaitBetweenSpawns );
				}

				return;
			}

			m_timer.Start( 1.0f );
		}
		break;
		case WAIT_FOR_ALL_DEAD:
		{
			FOR_EACH_VEC( m_activeSpawns, i )
			{
				if ( m_activeSpawns[i] && m_activeSpawns[i]->IsAlive() )
					return;
			}

			SetState( DONE );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveSpawnPopulator::OnPlayerKilled( CTFPlayer *pPlayer )
{
	m_activeSpawns.FindAndFastRemove( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveSpawnPopulator::ForceFinish( void )
{
	if ( m_eState < WAIT_FOR_ALL_DEAD )
	{
		SetState( WAIT_FOR_ALL_DEAD );
	}
	else if ( m_eState != WAIT_FOR_ALL_DEAD )
	{
		SetState( DONE );
	}

	FOR_EACH_VEC( m_activeSpawns, i )
	{
		CTFBot *pBot = ToTFBot( m_activeSpawns[i] );
		if ( pBot )
		{
			pBot->ChangeTeam( TEAM_SPECTATOR, false, true );
		}
		else
		{
			m_activeSpawns[i]->Remove();
		}
	}

	m_activeSpawns.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWaveSpawnPopulator::GetCurrencyAmountPerDeath( void )
{
	int nCurrency = 0;

	if ( m_bSupportWave )
	{
		if ( m_eState == WAIT_FOR_ALL_DEAD )
			m_iRemainingCount = m_activeSpawns.Count();
	}

	if ( m_iRemainingCurrency > 0 )
	{
		m_iRemainingCount = m_iRemainingCount <= 0 ? 1 : m_iRemainingCount;

		nCurrency = m_iRemainingCurrency / m_iRemainingCount--;
		m_iRemainingCurrency -= nCurrency;
	}

	return nCurrency;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWaveSpawnPopulator::IsFinishedSpawning( void )
{
	if ( m_bSupportWave && !m_bLimitedSupport )
		return false;

	return ( m_nNumSpawnedSoFar >= m_iTotalCount );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveSpawnPopulator::OnNonSupportWavesDone( void )
{
	if ( m_bSupportWave )
	{
		switch ( m_eState )
		{
			case PENDING:
			case PRE_SPAWN_DELAY:
				SetState( DONE );
				break;
			case SPAWNING:
			case WAIT_FOR_ALL_DEAD:
				if ( TFGameRules() && ( m_iRemainingCurrency > 0 ) )
				{
					TFGameRules()->DistributeCurrencyAmount( m_iRemainingCurrency, NULL, true, true );
					m_iRemainingCurrency = 0;
				}
				SetState( WAIT_FOR_ALL_DEAD );
				break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveSpawnPopulator::SetState( InternalStateType eState )
{
	m_eState = eState;

	if ( eState == WAIT_FOR_ALL_DEAD )
	{
		if ( m_lastSpawnWarningSound.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_lastSpawnWarningSound );
		}

		FireEvent( m_lastSpawnEvent, "LastSpawnOutput" );

		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: WaveSpawn(%s) started WAIT_FOR_ALL_DEAD\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
		}
	}
	else if ( eState == DONE )
	{
		if ( m_doneWarningSound.Length() > 0 )
		{
			TFGameRules()->BroadcastSound( 255, m_doneWarningSound );
		}

		FireEvent( m_doneEvent, "DoneOutput" );

		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: WaveSpawn(%s) DONE\n", gpGlobals->curtime, m_name.IsEmpty() ? "" : m_name.Get() );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPeriodicSpawnPopulator::CPeriodicSpawnPopulator( CPopulationManager *pManager )
	: m_pManager( pManager ), m_pSpawner( NULL )
{
	m_flMinInterval = 30.0f;
	m_flMaxInterval = 30.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPeriodicSpawnPopulator::~CPeriodicSpawnPopulator()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPeriodicSpawnPopulator::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		if ( m_where.Parse( pSubKey ) )
			continue;

		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "When" ) )
		{
			if ( pSubKey->GetFirstSubKey() )
			{
				FOR_EACH_SUBKEY( pSubKey, pWhenSubKey )
				{
					if ( !V_stricmp( pWhenSubKey->GetName(), "MinInterval" ) )
					{
						m_flMinInterval = pWhenSubKey->GetFloat();
					}
					else if ( !V_stricmp( pWhenSubKey->GetName(), "MaxInterval" ) )
					{
						m_flMaxInterval = pWhenSubKey->GetFloat();
					}
					else
					{
						Warning( "Invalid field '%s' encountered in When\n", pWhenSubKey->GetName() );
						return false;
					}
				}
			}
			else
			{
				m_flMinInterval = pSubKey->GetFloat();
				m_flMaxInterval = m_flMinInterval;
			}
		}
		else
		{
			m_pSpawner = IPopulationSpawner::ParseSpawner( this, pSubKey );

			if ( m_pSpawner == NULL )
			{
				Warning( "Unknown attribute '%s' in PeriodicSpawn definition.\n", pszKey );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPeriodicSpawnPopulator::PostInitialize( void )
{
	m_timer.Start( RandomFloat( m_flMinInterval, m_flMaxInterval ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPeriodicSpawnPopulator::Update( void )
{
	if ( !m_timer.IsElapsed() || g_pPopulationManager->IsSpawningPaused() )
		return;

	Vector vecSpawnPos;
	SpawnLocationResult spawnLocationResult = m_where.FindSpawnLocation( &vecSpawnPos );
	if ( spawnLocationResult != SPAWN_LOCATION_NOT_FOUND )
	{
		CUtlVector<EHANDLE> spawnedBots;
		if ( m_pSpawner && m_pSpawner->Spawn( vecSpawnPos, &spawnedBots ) )
		{
			m_timer.Start( RandomFloat( m_flMinInterval, m_flMaxInterval ) );

			FOR_EACH_VEC( spawnedBots, k )
			{
				CTFBot *pBot = ToTFBot( spawnedBots[k] );
				if ( pBot && spawnLocationResult == SPAWN_LOCATION_TELEPORTER )
					OnBotTeleported( pBot );
			}

			return;
		}
	}

	m_timer.Start( 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPeriodicSpawnPopulator::UnpauseSpawning( void )
{
	m_timer.Start( RandomFloat( m_flMinInterval, m_flMaxInterval ) );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRandomPlacementPopulator::CRandomPlacementPopulator( CPopulationManager *pManager )
	: m_pManager( pManager ), m_pSpawner( NULL )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRandomPlacementPopulator::~CRandomPlacementPopulator()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomPlacementPopulator::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "Count" ) )
		{
			m_iCount = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "MinimumSeparation" ) )
		{
			m_flMinSeparation = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "NavAreaFilter" ) )
		{
			if ( !V_stricmp( pSubKey->GetString(), "SENTRY_SPOT" ) )
			{
				m_nNavAreaFilter = TF_NAV_SENTRY_SPOT;
			}
			else if ( !V_stricmp( pSubKey->GetString(), "SNIPER_SPOT" ) )
			{
				m_nNavAreaFilter = TF_NAV_SNIPER_SPOT;
			}
			else
			{
				Warning( "Unknown NavAreaFilter value '%s'\n", pSubKey->GetString() );
			}
		}
		else
		{
			m_pSpawner = IPopulationSpawner::ParseSpawner( this, pSubKey );

			if ( m_pSpawner == NULL )
			{
				Warning( "Unknown attribute '%s' in RandomPlacement definition.\n", pszKey );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRandomPlacementPopulator::PostInitialize( void )
{
	CUtlVector<CTFNavArea *> markedAreas;;
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *pArea = (CTFNavArea *)TheNavAreas[i];
		if ( pArea->HasTFAttributes( m_nNavAreaFilter ) )
			markedAreas.AddToTail( pArea );
	}

	CUtlVector<CTFNavArea *> selectedAreas;
	SelectSeparatedShuffleSet< CTFNavArea >( m_iCount, m_flMinSeparation, markedAreas, &selectedAreas );

	if ( m_pSpawner )
	{
		FOR_EACH_VEC( selectedAreas, i )
		{
			m_pSpawner->Spawn( selectedAreas[i]->GetCenter() );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWave::CWave( CPopulationManager *pManager )
	: m_pManager( pManager ), m_pSpawner( NULL )
{
	m_startWaveEvent = NULL;
	m_doneEvent = NULL;
	m_initWaveEvent = NULL;

	m_bCheckBonusCreditsMin = true;
	m_bCheckBonusCreditsMax = true;

	m_doneTimer.Invalidate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWave::~CWave()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWave::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "WaveSpawn" ) )
		{
			CWaveSpawnPopulator *pWavePopulator = new CWaveSpawnPopulator( m_pManager );
			if ( !pWavePopulator->Parse( pSubKey ) )
			{
				Warning( "Error reading WaveSpawn definition\n" );
				return false;
			}

			if ( !pWavePopulator->m_bSupportWave )
				m_nTotalEnemyCount += pWavePopulator->m_iTotalCount;
			m_iTotalCurrency += pWavePopulator->m_iTotalCurrency;
			pWavePopulator->m_parentWave = this;

			m_WaveSpawns.AddToTail( pWavePopulator );

			if ( pWavePopulator->GetSpawner() )
			{
				if ( pWavePopulator->GetSpawner()->IsVarious() )
				{
					for ( int i = 0; i < pWavePopulator->m_iTotalCount; ++i )
					{
						unsigned int iFlags = pWavePopulator->m_bSupportWave ? MVM_CLASS_FLAG_SUPPORT : MVM_CLASS_FLAG_NORMAL;
						if ( pWavePopulator->GetSpawner()->IsMiniBoss( i ) )
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;

						if ( pWavePopulator->GetSpawner()->HasAttribute( CTFBot::AttributeType::ALWAYSCRIT, i ) )
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;

						if ( pWavePopulator->m_bLimitedSupport )
							iFlags |= MVM_CLASS_FLAG_SUPPORT_LIMITED;

						AddClassType( pWavePopulator->GetSpawner()->GetClassIcon( i ), 1, iFlags );
					}
				}
				else
				{
					unsigned int iFlags = pWavePopulator->m_bSupportWave ? MVM_CLASS_FLAG_SUPPORT : MVM_CLASS_FLAG_NORMAL;
					if ( pWavePopulator->GetSpawner()->IsMiniBoss() )
						iFlags |= MVM_CLASS_FLAG_MINIBOSS;

					if ( pWavePopulator->GetSpawner()->HasAttribute( CTFBot::AttributeType::ALWAYSCRIT ) )
						iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;

					if ( pWavePopulator->m_bLimitedSupport )
						iFlags |= MVM_CLASS_FLAG_SUPPORT_LIMITED;

					AddClassType( pWavePopulator->GetSpawner()->GetClassIcon(), pWavePopulator->m_iTotalCount, iFlags );
				}
			}
		}
		else if ( !V_stricmp( pszKey, "Sound" ) )
		{
			m_soundName.sprintf( "%s", pSubKey->GetString() );
		}
		else if ( !V_stricmp( pszKey, "Description" ) )
		{
			m_description.sprintf( "%s", pSubKey->GetString() );
		}
		else if ( !V_stricmp( pszKey, "WaitWhenDone" ) )
		{
			m_flWaitWhenDone = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "Checkpoint" ) )
		{

		}
		else if ( !V_stricmp( pszKey, "StartWaveOutput" ) )
		{
			m_startWaveEvent = ParseEvent( pSubKey );
		}
		else if ( !V_stricmp( pszKey, "DoneOutput" ) )
		{
			m_doneEvent = ParseEvent( pSubKey );
		}
		else if ( !V_stricmp( pszKey, "InitWaveOutput" ) )
		{
			m_initWaveEvent = ParseEvent( pSubKey );
		}
		else
		{
			Warning( "Unknown attribute '%s' in Wave definition.\n", pszKey );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		ActiveWaveUpdate();
	}
	else if ( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS || TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		WaveIntermissionUpdate();
	}

	if ( m_bEveryWaveSpawnDone && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		if ( m_pManager->byte58A )
		{
			if ( m_pManager->ehandle58C && m_pManager->ehandle58C->IsAlive() )
				return;
		}
		WaveCompleteUpdate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::OnPlayerKilled( CTFPlayer *pPlayer )
{
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		m_WaveSpawns[i]->OnPlayerKilled( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWave::HasEventChangeAttributes( const char *pszEventName ) const
{
	bool bHasEventChangeAttributes = false;
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		bHasEventChangeAttributes |= m_WaveSpawns[i]->HasEventChangeAttributes( pszEventName );
	}

	return bHasEventChangeAttributes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::AddClassType( string_t iszClassIconName, int nCount, unsigned int iFlags )
{
	int nIndex = -1;
	FOR_EACH_VEC( m_WaveClassCounts, i )
	{
		WaveClassCount_t const &count = m_WaveClassCounts[i];
		if ( ( count.iszClassIconName == iszClassIconName ) && ( count.iFlags & iFlags ) )
		{
			nIndex = i;
			break;
		}
	}

	if ( nIndex == -1 )
	{
		nIndex = m_WaveClassCounts.AddToTail( {0, iszClassIconName, MVM_CLASS_FLAG_NONE} );
	}

	m_WaveClassCounts[ nIndex ].nClassCount += nCount;
	m_WaveClassCounts[ nIndex ].iFlags |= iFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWaveSpawnPopulator *CWave::FindWaveSpawnPopulator( const char *name )
{
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		CWaveSpawnPopulator *pPopulator = m_WaveSpawns[i];
		if ( !V_stricmp( pPopulator->m_name, name ) )
			return pPopulator;
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::ForceFinish()
{
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		m_WaveSpawns[i]->ForceFinish();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::ForceReset()
{
	m_bStarted = false;
	m_bFiredInitWaveOutput = false;
	m_bEveryWaveSpawnDone = false;
	m_flStartTime = 0;

	m_doneTimer.Invalidate();

	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		m_WaveSpawns[i]->m_iRemainingCurrency = m_WaveSpawns[i]->m_iTotalCurrency;
		m_WaveSpawns[i]->m_iRemainingCount = m_WaveSpawns[i]->m_iTotalCount;
		m_WaveSpawns[i]->m_eState = CWaveSpawnPopulator::PENDING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWave::IsDoneWithNonSupportWaves( void )
{
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		CWaveSpawnPopulator *pPopulator = m_WaveSpawns[i];
		if ( pPopulator->m_bSupportWave && pPopulator->m_eState != CWaveSpawnPopulator::DONE )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::ActiveWaveUpdate( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( !m_bStarted )
	{
		if ( m_pManager->IsInEndlessWaves() && m_flStartTime > gpGlobals->curtime )
			return;

		m_bStarted = true;
		FireEvent( m_startWaveEvent, "StartWaveOutput" );

		if ( m_soundName.Length() > 0 )
			TFGameRules()->BroadcastSound( 255, m_soundName );

		m_pManager->AdjustMinPlayerSpawnTime();
	}

	m_bEveryWaveSpawnDone = true;
	FOR_EACH_VEC( m_WaveSpawns, i )
	{
		CWaveSpawnPopulator *pPopulator = m_WaveSpawns[i];

		bool bWaiting = false;
		if ( !pPopulator->m_szWaitForAllSpawned.IsEmpty() )
		{
			FOR_EACH_VEC( m_WaveSpawns, j )
			{
				CWaveSpawnPopulator *pWaitingPopulator = m_WaveSpawns[j];
				if ( pWaitingPopulator == NULL )
					continue;

				if ( V_stricmp( pWaitingPopulator->m_name, pPopulator->m_szWaitForAllSpawned ) )
					continue;

				if ( pWaitingPopulator->m_eState <= CWaveSpawnPopulator::SPAWNING )
				{
					bWaiting = true;
					break;
				}
			}
		}

		if ( !bWaiting && !pPopulator->m_szWaitForAllDead.IsEmpty() )
		{
			FOR_EACH_VEC( m_WaveSpawns, j )
			{
				CWaveSpawnPopulator *pWaitingPopulator = m_WaveSpawns[j];
				if ( pWaitingPopulator == NULL )
					continue;

				if ( V_stricmp( pWaitingPopulator->m_name, pPopulator->m_szWaitForAllSpawned ) )
					continue;

				if ( pWaitingPopulator->m_eState != CWaveSpawnPopulator::DONE )
				{
					bWaiting = true;
					break;
				}
			}
		}

		if ( bWaiting )
			continue;

		pPopulator->Update();

		m_bEveryWaveSpawnDone &= ( pPopulator->m_eState == CWaveSpawnPopulator::DONE );
	}

	if ( IsDoneWithNonSupportWaves() )
	{
		FOR_EACH_VEC( m_WaveSpawns, i )
		{
			m_WaveSpawns[i]->OnNonSupportWavesDone();
		}

		for ( int i = 0; i <= gpGlobals->maxClients; ++i )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer || !pPlayer->IsAlive() )
				continue;

			if ( pPlayer->GetTeamNumber() != TF_TEAM_MVM_BOTS )
				continue;

			pPlayer->CommitSuicide( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::WaveCompleteUpdate( void )
{
	FireEvent( m_doneEvent, "DoneOutput" );

	bool bAdvancedPopfile = ( g_pPopulationManager ? g_pPopulationManager->IsAdvanced() : false );

	IGameEvent *event = gameeventmanager->CreateEvent( "mvm_wave_complete" );
	if ( event )
	{
		event->SetBool( "advanced", bAdvancedPopfile );
		gameeventmanager->FireEvent( event );
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && bAdvancedPopfile )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( pMaster && ( pMaster->GetNumPoints() > 0 ) )
		{
			if ( pMaster->GetNumPointsOwnedByTeam( TF_TEAM_MVM_BOTS ) == pMaster->GetNumPoints() )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_adv_wave_complete_no_gates" );
				if ( event )
				{
					event->SetInt( "index", m_pManager->m_nCurrentWaveIndex );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}

	if ( ( m_pManager->m_nCurrentWaveIndex + 1 ) >= m_pManager->m_Waves.Count() && !m_pManager->IsInEndlessWaves() )
	{
		m_pManager->MvMVictory();

		if ( TFGameRules() )
		{
			/*if ( GTFGCClientSystem()->dword3B8 && GTFGCClientSystem()->dword3B8->dword10 == 1 )
				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Manned_Up_Wave_End" );
			else*/
				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Final_Wave_End" );

			TFGameRules()->BroadcastSound( 255, "music.mvm_end_last_wave" );
		}

		event = gameeventmanager->CreateEvent( "mvm_mission_complete" );
		if ( event )
		{
			event->SetString( "mission", m_pManager->GetPopulationFilename() );
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		if ( TFGameRules() )
		{
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Wave_End" );

			if ( m_nNumTanksSpawned >= 1 )
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_tank_wave" );
			else if ( ( m_pManager->m_nCurrentWaveIndex + 1 ) >= ( m_pManager->m_Waves.Count() / 2 ) )
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_mid_wave" );
			else 
				TFGameRules()->BroadcastSound( 255, "music.mvm_end_wave" );
		}
	}

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMAnnouncement" );
		WRITE_CHAR( MVM_ANNOUNCEMENT_WAVE_COMPLETE );
		WRITE_CHAR( m_pManager->m_nCurrentWaveIndex );
	MessageEnd();

	// Why does this care about the resource?
	if ( TFObjectiveResource() )
	{
		CUtlVector<CTFPlayer *> players;
		CollectPlayers( &players, TF_TEAM_MVM_PLAYERS );

		FOR_EACH_VEC( players, i )
		{
			if ( !players[i]->IsAlive() )
				players[i]->ForceRespawn();

			players[i]->m_nAccumulatedSentryGunDamageDealt = 0;
			players[i]->m_nAccumulatedSentryGunKillCount = 0;
		}
	}

	m_pManager->WaveEnd( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWave::WaveIntermissionUpdate( void )
{
	if ( !m_bFiredInitWaveOutput )
	{
		FireEvent( m_initWaveEvent, "InitWaveOutput" );

		m_bFiredInitWaveOutput = true;
	}

	if ( m_upgradeAlertTimer.HasStarted() && m_upgradeAlertTimer.IsElapsed() )
	{
		if ( ( m_bCheckBonusCreditsMin || m_bCheckBonusCreditsMax ) && gpGlobals->curtime > m_flBonusCreditsTime )
		{
			int nWaveNum = m_pManager->m_nCurrentWaveIndex - 1;
			int nDropped = MannVsMachineStats_GetDroppedCredits( nWaveNum );
			int nAcquired = MannVsMachineStats_GetAcquiredCredits( nWaveNum, false );
			float flRatioCollected = clamp( ( (float)nAcquired / (float)nDropped ), 0.1f, 1.f );

			float flMinBonus = tf_mvm_currency_bonus_ratio_min.GetFloat();
			float flMaxBonus = tf_mvm_currency_bonus_ratio_max.GetFloat();

			if ( m_bCheckBonusCreditsMax && nDropped > 0 && flRatioCollected >= flMaxBonus )
			{
				int nAmount = TFGameRules()->CalculateCurrencyAmount_ByType( CURRENCY_WAVE_COLLECTION_BONUS );
				TFGameRules()->DistributeCurrencyAmount( (float)nAmount * 0.5f, NULL, true, false, true );

				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bonus" );
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_creditbonus_wave" );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}

				m_bCheckBonusCreditsMax = false;
				m_upgradeAlertTimer.Reset();
			}

			if ( m_bCheckBonusCreditsMin && nDropped > 0 && flRatioCollected >= fminf( flMinBonus, flMaxBonus ) )
			{
				int nAmount = TFGameRules()->CalculateCurrencyAmount_ByType( CURRENCY_WAVE_COLLECTION_BONUS );
				TFGameRules()->DistributeCurrencyAmount( (float)nAmount * 0.5f, NULL, true, false, true );

				TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bonus" );
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_creditbonus_wave" );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}

				m_bCheckBonusCreditsMin = false;
			}

			m_flBonusCreditsTime = gpGlobals->curtime + 0.25f;
		}
	}

	if ( m_doneTimer.HasStarted() && m_doneTimer.IsElapsed() )
	{
		m_doneTimer.Invalidate();

		m_pManager->StartCurrentWave();
	}
}
