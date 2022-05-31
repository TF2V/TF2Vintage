//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_population_manager.h"
#include "tf_populators.h"
#include "tf_populator_spawners.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_upgrades_shared.h"
#include "tf_powerup_bottle.h"
#include "map_entities/tf_upgrades.h"

extern ConVar tf_mvm_respec_limit;
extern ConVar tf_mvm_respec_credit_goal;
extern ConVar tf_mvm_buybacks_method;
extern ConVar tf_mvm_buybacks_per_wave;

ConVar tf_mvm_default_sentry_buster_damage_dealt_threshold( "tf_mvm_default_sentry_buster_damage_dealt_threshold", "3000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_mvm_default_sentry_buster_kill_threshold( "tf_mvm_default_sentry_buster_kill_threshold", "15", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_mvm_endless_force_on( "tf_mvm_endless_force_on", "0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Force MvM Endless mode on" );
ConVar tf_mvm_endless_wait_time( "tf_mvm_endless_wait_time", "5.0f", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_mvm_endless_bomb_reset( "tf_mvm_endless_bomb_reset", "5", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Number of Waves to Complete before bomb reset" );
ConVar tf_mvm_endless_bot_cash( "tf_mvm_endless_bot_cash", "120", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "In Endless, number of credits bots get per wave" );
ConVar tf_mvm_endless_tank_boost( "tf_mvm_endless_tank_boost", "0.2", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "In Endless, amount of extra health for the tank per wave" );

ConVar tf_populator_debug( "tf_populator_debug", "0", FCVAR_CHEAT );
ConVar tf_populator_active_buffer_range( "tf_populator_active_buffer_range", "3000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Populate the world this far ahead of lead raider, and this far behind last raider" );
ConVar tf_populator_health_multiplier( "tf_populator_health_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_populator_damage_multiplier( "tf_populator_damage_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT );

ConVar tf_mvm_disconnect_on_victory( "tf_mvm_disconnect_on_victory", "0", FCVAR_REPLICATED, "Enable to Disconnect Players after completing MvM" );
ConVar tf_mvm_victory_reset_time( "tf_mvm_victory_reset_time", "60.0", FCVAR_REPLICATED, "Seconds to wait after MvM victory before cycling to the next mission.  (Only used if tf_mvm_disconnect_on_victory is false.)" );
ConVar tf_mvm_victory_disconnect_time( "tf_mvm_victory_disconnect_time", "180.0", FCVAR_REPLICATED, "Seconds to wait after MvM victory before kicking players.  (Only used if tf_mvm_disconnect_on_victory is true.)" );

void MvMMinibossScaleChangedCallBack( IConVar *pVar, const char *pOldString, float flOldValue );
ConVar tf_mvm_miniboss_scale( "tf_mvm_miniboss_scale", "1.75", FCVAR_REPLICATED | FCVAR_CHEAT, 
							  "Full body scale for minibosses.", MvMMinibossScaleChangedCallBack );

void MvMMissionCycleFileChangedCallback( IConVar *var, const char *pOldString, float flOldValue );
ConVar tf_mvm_missioncyclefile( "tf_mvm_missioncyclefile", "tf_mvm_missioncycle.res", FCVAR_NONE, 
								"Name of the .res file used to cycle mvm misisons", MvMMissionCycleFileChangedCallback );

void MvMSkillChangedCallback( IConVar *pVar, const char *pOldString, float flOldValue );
ConVar tf_mvm_skill( "tf_mvm_skill", "3", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT, 
					 "Sets the challenge level of the invading bot army. 1 = easiest, 3 = normal, 5 = hardest", 
					 true, 1, true, 5, MvMSkillChangedCallback );

void MvMMinibossScaleChangedCallBack( IConVar *pVar, const char *pOldString, float flOldValue )
{
	ConVarRef cvar( pVar );
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CTFBot *pBot = ToTFBot( UTIL_PlayerByIndex( i ) );
		if ( pBot && pBot->IsMiniBoss() )
		{
			pBot->SetModelScale( cvar.GetFloat(), 1.0f );
		}
	}
}

void MvMMissionCycleFileChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->LoadMissionCycleFile();
	}
}

void MvMSkillChangedCallback( IConVar *pVar, const char *pOldString, float flOldValue )
{
	ConVarRef cvar( pVar );
	float flHealth = 1.0f;	// Health modifier for bots
	float flDamage = 1.0f;	// Damage modifier in bot vs player

	switch ( cvar.GetInt() )
	{
		case 1:
		{
			flHealth = 0.75f;
			flDamage = 0.75f;
			break;
		}
		case 2:
		{
			flHealth = 0.9f;
			flDamage = 0.9f;
			break;
		}
		// "Normal"
		case 3:
		{
			flHealth = 1.0f;
			flDamage = 1.0f;
			break;
		}
		case 4:
		{
			flHealth = 1.10f;
			flDamage = 1.10f;
			break;
		}
		case 5:
		{
			flHealth = 1.25f;
			flDamage = 1.25f;
			break;
		}
	}

	tf_populator_health_multiplier.SetValue( flHealth );
	tf_populator_damage_multiplier.SetValue( flDamage );
}


//-----------------------------------------------------------------------------
static bool HaveMap( char const *pszMap )
{
	char szMap[64];
	V_strcpy_safe( szMap, pszMap );

	IVEngineServer::eFindMapResult result = engine->FindMap( szMap, sizeof( szMap ) );
	if ( result == IVEngineServer::eFindMap_Found || result == IVEngineServer::eFindMap_PossiblyAvailable )
		return true;

	return false;
}

CPopulationManager *g_pPopulationManager = NULL;


BEGIN_DATADESC( CPopulationManager )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_populator, CPopulationManager );

PRECACHE_REGISTER( info_populator );


CUtlVector<CPopulationManager::CheckpointSnapshotInfo *> CPopulationManager::sm_checkpointSnapshots;
int CPopulationManager::m_nCheckpointWaveIndex;
int CPopulationManager::m_nNumConsecutiveWipes;


static int s_iLastKnownMissionCategory = 1;
static int s_iLastKnownMission = 1;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::CPopulationManager()
	: m_mapRespecs( DefLessFunc( uint64 ) ), m_mapBuyBackCredits( DefLessFunc( uint64 ) )
{
	Assert( g_pPopulationManager == NULL );
	g_pPopulationManager = this;

	m_pMVMStats = MannVsMachineStats_GetInstance();

	m_iSentryBusterDamageDealtThreshold = tf_mvm_default_sentry_buster_damage_dealt_threshold.GetInt();
	m_iSentryBusterKillThreshold = tf_mvm_default_sentry_buster_kill_threshold.GetInt();

	m_nCurrentWaveIndex = 0;
	m_nStartingCurrency = 0;
	m_nNumConsecutiveWipes = 0;
	m_nRespawnWaveTime = 10;
	m_szPopfileFull[0] = '\0';
	m_szPopfileShort[0] = '\0';
	m_bIsInitialized = false;
	m_bIsAdvanced = false;
	m_bAllocatedBots = false;
	m_bFixedRespawnWaveTime = false;

	m_bCanBotsAttackWhileInSpawnRoom = true;

	SetThink( &CPopulationManager::Update );
	SetNextThink( gpGlobals->curtime );

	ListenForGameEvent( "pve_win_panel" );

	m_random.SetSeed( 0 );
	for ( int i = 0; i < 27; ++i )
	{
		int nRandom = m_random.RandomInt( 0, INT_MAX );
		m_RandomSeeds.AddToTail( nRandom );
	}

	EndlessParseBotUpgrades();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::~CPopulationManager()
{
	Assert( g_pPopulationManager == this );
	g_pPopulationManager = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::FireGameEvent( IGameEvent *event )
{
	const char *pszName = event->GetName();

	if ( FStrEq( pszName, "pve_win_panel" ) )
	{
		MarkAllCurrentPlayersSafeToLeave();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::Initialize( void )
{
	if ( ( TheNavMesh == NULL ) || ( TheNavMesh->GetNavAreaCount() <= 0 ) )
	{
		Warning( "No Nav Mesh CPopulationManager::Initialize for %s", m_szPopfileFull );
		return false;
	}

	Reset();

	if ( !Parse() )
	{
		Warning( "Parse Failed in CPopulationManager::Initialize for %s", m_szPopfileFull );
		return false;
	}

	if ( TFGameRules()->State_Get() == GR_STATE_PREGAME )
	{
		ClearCheckpoint();
		m_nCurrentWaveIndex = 0;
		m_nNumConsecutiveWipes = 0;
		m_pMVMStats->SetCurrentWave( m_nCurrentWaveIndex );
	}
	else
	{
		RestoreCheckpoint();

		if ( m_bIsInitialized && ( m_nCurrentWaveIndex > 0 || m_nNumConsecutiveWipes > 1 ) )
			m_pMVMStats->RoundEvent_WaveEnd( false );

		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_wave_failed" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}

	if ( IsInEndlessWaves() )
		EndlessRollEscalation();

	UpdateObjectiveResource();

	DebugWaveStats();

	if ( tf_mvm_respec_limit.GetBool() )
	{
		const int nAmount = tf_mvm_respec_limit.GetInt() + 1;
		tf_mvm_respec_credit_goal.SetValue( GetTotalPopFileCurrency() / nAmount );
	}

	PostInitialize();
	m_bIsInitialized = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "music.mvm_end_wave" );
	PrecacheScriptSound( "music.mvm_end_tank_wave" );
	PrecacheScriptSound( "music.mvm_end_mid_wave" );
	PrecacheScriptSound( "music.mvm_end_last_wave" );
	PrecacheScriptSound( "MVM.PlayerUpgraded" );
	PrecacheScriptSound( "MVM.PlayerBoughtIn" );
	PrecacheScriptSound( "MVM.PlayerUsedPowerup" );
	PrecacheScriptSound( "MVM.PlayerDied" );
	PrecacheScriptSound( "MVM.PlayerDiedScout" );
	PrecacheScriptSound( "MVM.PlayerDiedSniper" );
	PrecacheScriptSound( "MVM.PlayerDiedSoldier" );
	PrecacheScriptSound( "MVM.PlayerDiedDemoman" );
	PrecacheScriptSound( "MVM.PlayerDiedMedic" );
	PrecacheScriptSound( "MVM.PlayerDiedHeavy" );
	PrecacheScriptSound( "MVM.PlayerDiedPyro" );
	PrecacheScriptSound( "MVM.PlayerDiedSpy" );
	PrecacheScriptSound( "MVM.PlayerDiedEngineer" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::Reset( void )
{
	m_iSentryBusterDamageDealtThreshold = tf_mvm_default_sentry_buster_damage_dealt_threshold.GetInt();
	m_iSentryBusterKillThreshold = tf_mvm_default_sentry_buster_kill_threshold.GetInt();

	m_nStartingCurrency = 0;
	m_nRespawnWaveTime = 10;
	m_nRespecsAwarded = 0;
	m_nRespecsAwardedInWave = 0;

	m_bSpawningPaused = false;
	m_bIsAdvanced = false;
	m_bCanBotsAttackWhileInSpawnRoom = true;

	m_szDefaultEventChangeAttributesName = "Default";

	cutlvector6BC.Purge();

	if ( !m_bWaveJumping )
		m_nCurrentWaveIndex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::Spawn( void )
{
	BaseClass::Spawn();

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::AddPlayerCurrencySpent( CTFPlayer *pPlayer, int nSpent )
{
	PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pPlayer );
	if ( pHistory )
		pHistory->m_nCurrencySpent += nSpent;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::AddRespecToPlayer( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();

		uint16 nIndex = m_mapRespecs.Find( ulSteamId );
		if ( nIndex == m_mapRespecs.InvalidIndex() )
			nIndex = m_mapRespecs.Insert( ulSteamId );

		m_mapRespecs[ nIndex ] += 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::AdjustMinPlayerSpawnTime( void )
{
	float flRespawnWaveTime = fminf( 2 * ( m_nCurrentWaveIndex + 1 ), m_nRespawnWaveTime );

	if ( IsInEndlessWaves() )
		flRespawnWaveTime = (float)( m_nCurrentWaveIndex + 1 ) / 3;
	else if ( m_bFixedRespawnWaveTime )
		flRespawnWaveTime = m_nRespawnWaveTime;

	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_MVM_PLAYERS, flRespawnWaveTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::AllocateBots( void )
{
	if ( m_bAllocatedBots )
		return;

	CUtlVector<CTFPlayer *> bots;
	int nNumBots = CollectMvMBots( &bots );

	if ( nNumBots > 0 )
	{
		Warning( "%d bots were already allocated some how before "
				 "CPopulationManager::AllocateBots was called\n", nNumBots );
	}

	for ( int i = nNumBots; i < k_nMvMBotTeamSize; ++i )
	{
		CTFBot *pBot = NextBotCreatePlayerBot<CTFBot>( "TFBot", false );
		if ( pBot ) pBot->ChangeTeam( TEAM_SPECTATOR, false, true );
	}

	m_bAllocatedBots = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::ClearCheckpoint( void )
{
	if( tf_populator_debug.GetBool() )
		DevMsg( "%3.2f: CHECKPOINT_CLEAR\n", gpGlobals->curtime );

	m_nNumConsecutiveWipes = 0;

	sm_checkpointSnapshots.PurgeAndDeleteElements();

	CUtlVector<CTFPlayer *> humans;
	CollectHumanPlayers( &humans, TF_TEAM_MVM_PLAYERS );
	FOR_EACH_VEC( humans, i )
	{
		humans[i]->ClearUpgradeHistory();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPopulationManager::CollectMvMBots( CUtlVector<CTFPlayer *> *pBotsOut )
{
	pBotsOut->RemoveAll();
	for ( int i = 0; i < gpGlobals->maxClients; ++i )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer == NULL || FNullEnt( pPlayer->edict() ) || !pPlayer->IsPlayer() )
			continue;

		if ( !pPlayer->IsConnected() || !pPlayer->IsBot() )
			continue;

		if ( pPlayer->GetTeamNumber() != TF_TEAM_MVM_BOTS )
			continue;

		pBotsOut->AddToTail( (CTFPlayer *)pPlayer );
	}

	return pBotsOut->Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::CycleMission( void )
{
	bool bHaveMission = true;
	if ( m_pMvMMapCycle == NULL )
		bHaveMission = LoadMissionCycleFile();

	if ( !bHaveMission )
	{
		LoadLastKnownMission();
		return;
	}

	const char *pszCurrentMap = STRING( gpGlobals->mapname );
	char szCurrentPopfile[MAX_PATH]{};
	V_FileBase( m_szPopfileFull, szCurrentPopfile, sizeof( szCurrentPopfile ) );

	int iNumCatagories = m_pMvMMapCycle->GetInt( "catagories" );
	for ( int i = 1; i < iNumCatagories; ++i )
	{
		KeyValues *pCatagory = m_pMvMMapCycle->FindKey( UTIL_VarArgs( "%d", i ) );

		if ( pCatagory )
		{
			int iNumMissions = pCatagory->GetInt( "count" );
			for ( int j = 1; j < iNumMissions; ++j )
			{
				KeyValues *pMission = pCatagory->FindKey( UTIL_VarArgs( "%d", j ) );

				if ( pMission )
				{
					const char *pszMap = pMission->GetString( "map" );
					const char *pszPopfile = pMission->GetString( "popfile" );

					if ( !V_strcmp( pszMap, pszCurrentMap ) && !V_strcmp( pszPopfile, szCurrentPopfile ) )
					{
						int nNextMission = ( j + 1 ) % iNumMissions;
						KeyValues *pNextMission = pCatagory->FindKey( UTIL_VarArgs( "%d", nNextMission ) );

						if ( LoadMvMMission( pNextMission ) )
						{
							s_iLastKnownMission = nNextMission;
							s_iLastKnownMissionCategory = i;

							return;
						}
					}
				}
			}
		}
	}

	LoadLastKnownMission();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::DebugWaveStats( void )
{
	if ( !m_Waves.IsEmpty() )
	{
		int nTotalPopCurrency = GetTotalPopFileCurrency();
		int nTotalWaves = m_Waves.Count();

		DevMsg( "---\n" );
		DevMsg( "Credits: %d\n", nTotalPopCurrency );
		DevMsg( "Waves: %d ( %3.2f credits per wave )\n", nTotalWaves, (float)nTotalPopCurrency / nTotalWaves );
		DevMsg( "---\n" );
	}

	if ( !m_ActiveBotUpgrades.IsEmpty() )
	{
		DevMsg( "*** Endless Bot Upgrades - %.0f Cash *** \n", m_nCurrentWaveIndex * tf_mvm_endless_bot_cash.GetFloat() );
		FOR_EACH_VEC( m_ActiveBotUpgrades, i )
		{
			DevMsg( "   - %s %.2f\n", m_ActiveBotUpgrades[i].m_szAttrib, m_ActiveBotUpgrades[i].m_flValue );
		}

		char msg[255];
		V_strcpy_safe( msg, "***  Bot Upgrades\n" );
		FOR_EACH_VEC( m_ActiveBotUpgrades, i )
		{
			char line[255];
			V_sprintf_safe( line, "-%s %.1f\n", m_ActiveBotUpgrades[i].m_szAttrib, m_ActiveBotUpgrades[i].m_flValue );
			V_strcat_safe( msg, line );
		}

		UTIL_CenterPrintAll( msg );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, msg );
	}

	DevMsg( "Popfile: %s\n", GetPopulationFilename() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::EndlessFlagHasReset( void )
{
	m_bShouldResetFlag = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::EndlessParseBotUpgrades( void )
{
	m_BotUpgrades.RemoveAll();

	KeyValuesAD pKV( "Upgrades" );
	if ( !pKV->LoadFromFile( filesystem, "scripts/items/mvm_botupgrades.txt", "MOD" ) )
	{
		Warning( "Can't open scripts/items/mvm_botupgrades.txt\n" );
		return;
	}

	FOR_EACH_SUBKEY( pKV, pSubKey )
	{
		const char *pszAttrib = pSubKey->GetString( "attribute" );
		bool bIsBotAttr = pSubKey->GetBool( "IsBotAttr" );
		bool bIsSkillAttr = pSubKey->GetBool( "IsSkillAttr" );
		float flValue = pSubKey->GetFloat( "value" );
		float flMax = pSubKey->GetFloat( "max" );
		int nCost = pSubKey->GetInt( "cost", 100 );
		int nWeight = pSubKey->GetInt( "weight", 1 );

		attrib_def_index_t iAttrIndex = 0;
		if ( !bIsBotAttr && !bIsSkillAttr )
		{
			CEconAttributeDefinition const *pAttr = GetItemSchema()->GetAttributeDefinitionByName( pszAttrib );
			if ( pAttr == NULL )
			{
				Warning( "Unable to Find Attribute %s when parsing EndlessParseBotUpgrades.\n", pszAttrib );
				return;
			}

			iAttrIndex = pAttr->index;
		}

		int nIndex = m_BotUpgrades.AddToTail();

		// ???
		for ( int i =0; i < nWeight; ++i )
		{
			CMvMBotUpgrade *pUpgrade = &m_BotUpgrades[ nIndex ];
			V_strncpy( pUpgrade->m_szAttrib, pszAttrib, sizeof( pUpgrade->m_szAttrib ) );
			pUpgrade->m_flValue = flValue;
			pUpgrade->m_flMax = flMax;
			pUpgrade->m_nCost = nCost;
			pUpgrade->m_bIsBotAttr = bIsBotAttr;
			pUpgrade->m_bIsSkillAttr = bIsSkillAttr;
			pUpgrade->m_iAttribIndex = iAttrIndex;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::EndlessRollEscalation( void )
{
	m_ActiveBotUpgrades.Purge();

	int nCurrency = tf_mvm_endless_bot_cash.GetFloat() * m_nCheckpointWaveIndex;

	CUtlVector<CMvMBotUpgrade> availableUpgrades;
	FOR_EACH_VEC( m_BotUpgrades, i )
	{
		if ( m_BotUpgrades[i].m_nCost <= nCurrency )
			availableUpgrades.AddToTail( m_BotUpgrades[i] );
	}

	CUniformRandomStream random;
	random.SetSeed( m_RandomSeeds[ m_nCurrentWaveIndex % m_RandomSeeds.Count() ] );
	while ( nCurrency >= 100 && !availableUpgrades.IsEmpty() )
	{
		int nRandIndex = random.RandomInt( 0, availableUpgrades.Count() - 1 );
		CMvMBotUpgrade upgrade = availableUpgrades[ nRandIndex ];

		if ( m_ActiveBotUpgrades.IsEmpty() )
		{
			m_ActiveBotUpgrades.AddToTail( upgrade );

			nCurrency -= upgrade.m_nCost;

			if ( ( upgrade.m_flMax > 0 && upgrade.m_flMax <= upgrade.m_flValue ) ||
				 ( upgrade.m_flMax < 0 && upgrade.m_flMax >= upgrade.m_flValue ) )
			{
				availableUpgrades.FastRemove( nRandIndex );
			}
		}
		else
		{
			int nFoundIndex = -1;
			FOR_EACH_VEC( m_ActiveBotUpgrades, i )
			{
				if ( !V_strcmp( upgrade.m_szAttrib, m_ActiveBotUpgrades[i].m_szAttrib ) )
				{
					m_ActiveBotUpgrades[i].m_flValue += upgrade.m_flValue;
					float flValue = m_ActiveBotUpgrades[i].m_flValue;
					
					nCurrency -= upgrade.m_nCost;

					if ( ( upgrade.m_flMax > 0 && upgrade.m_flMax <= flValue ) ||
						 ( upgrade.m_flMax < 0 && upgrade.m_flMax >= flValue ) )
					{
						availableUpgrades.FastRemove( nRandIndex );
					}

					nFoundIndex = i;
					break;
				}
			}

			if ( !m_ActiveBotUpgrades.IsValidIndex( nFoundIndex ) )
			{
				m_ActiveBotUpgrades.AddToTail( upgrade );

				nCurrency -= upgrade.m_nCost;

				if ( ( upgrade.m_flMax > 0 && upgrade.m_flMax <= upgrade.m_flValue ) ||
					 ( upgrade.m_flMax < 0 && upgrade.m_flMax >= upgrade.m_flValue ) )
				{
					availableUpgrades.FastRemove( nRandIndex );
				}
			}
		}

		if ( nCurrency > 0 )
		{
			FOR_EACH_VEC_BACK( availableUpgrades, i )
			{
				if ( availableUpgrades[i].m_nCost > nCurrency )
					availableUpgrades.FastRemove( i );
			}
		}
	}

	char szMsg[255];
	V_strcpy_safe( szMsg, "*** Bot Upgrades\n" );
	FOR_EACH_VEC( m_ActiveBotUpgrades, i )
	{
		char szAttrib[sizeof( m_ActiveBotUpgrades[i].m_szAttrib ) + 20];
		V_sprintf_safe( szAttrib, "-%s %.1f", m_ActiveBotUpgrades[i].m_szAttrib, m_ActiveBotUpgrades[i].m_flValue );
		V_strcat_safe( szMsg, szAttrib );
	}

	UTIL_ClientPrintAll( HUD_PRINTCENTER, szMsg );
	UTIL_ClientPrintAll( HUD_PRINTCONSOLE, szMsg );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::EndlessSetAttributesForBot( CTFBot *pBot )
{
	FOR_EACH_VEC( m_ActiveBotUpgrades, i )
	{
		CMvMBotUpgrade upgrade = m_ActiveBotUpgrades[i];
		if ( upgrade.m_bIsBotAttr )
		{
			pBot->SetAttribute( (CTFBot::AttributeType)(int)upgrade.m_flValue );
		}
		else if ( upgrade.m_bIsSkillAttr )
		{
			pBot->SetDifficulty( (CTFBot::DifficultyType)(int)upgrade.m_flValue );
		}
		else
		{
			CEconAttributeDefinition const *pAttr = GetItemSchema()->GetAttributeDefinition( upgrade.m_iAttribIndex );
			if ( pAttr )
			{
				float flValue = upgrade.m_flValue;
				if ( pAttr->description_format == ATTRIB_FORMAT_PERCENTAGE ||
						 pAttr->description_format == ATTRIB_FORMAT_INVERTED_PERCENTAGE )
				{
						flValue += 1.0f;
				}

				pBot->GetAttributeList()->SetRuntimeAttributeValue( pAttr, flValue );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::EndlessShouldResetFlag( void )
{
	return m_bShouldResetFlag;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::CheckpointSnapshotInfo *CPopulationManager::FindCheckpointSnapshot( CSteamID steamID ) const
{
	FOR_EACH_VEC( sm_checkpointSnapshots, i )
	{
		if ( sm_checkpointSnapshots[i]->m_steamID == steamID )
			return sm_checkpointSnapshots[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::CheckpointSnapshotInfo *CPopulationManager::FindCheckpointSnapshot( CTFPlayer *pPlayer ) const
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
		return FindCheckpointSnapshot( steamID );

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::FindDefaultPopulationFileShortNames( CUtlVector<CUtlString> &outNames )
{
	char szBaseName[_MAX_PATH];
	V_snprintf( szBaseName, sizeof( szBaseName ), "scripts/population/%s*.pop", STRING( gpGlobals->mapname ) );

	FileFindHandle_t popHandle;
	const char *pPopFileName = filesystem->FindFirstEx( szBaseName, "GAME", &popHandle );
	while ( pPopFileName && pPopFileName[0] != '\0' )
	{
		// Skip it if it's a directory or is the folder info
		if ( filesystem->FindIsDirectory( popHandle ) )
		{
			pPopFileName = filesystem->FindNext( popHandle );
			continue;
		}

		const char *pchPopPostfix = StringAfterPrefix( pPopFileName, STRING( gpGlobals->mapname ) );
		if ( pchPopPostfix )
		{
			char szShortName[_MAX_PATH];
			V_strncpy( szShortName, ( ( pchPopPostfix[0] == '_' ) ? ( pchPopPostfix + 1 ) : "normal" ), sizeof( szShortName ) ); // skip the '_'
			V_StripExtension( szShortName, szShortName, sizeof( szShortName ) );

			if( outNames.Find(szShortName) == outNames.InvalidIndex() )
				outNames.AddToTail( szShortName );
		}

		pPopFileName = filesystem->FindNext( popHandle );
	}
	filesystem->FindClose( popHandle );

	pPopFileName = filesystem->FindFirstEx( "scripts/population/*.pop", "BSP", &popHandle );
	while ( pPopFileName && pPopFileName[0] != '\0' )
	{
		// Skip it if it's a directory or is the folder info
		if ( filesystem->FindIsDirectory( popHandle ) )
		{
			pPopFileName = filesystem->FindNext( popHandle );
			continue;
		}

		char szShortName[_MAX_PATH];
		V_strncpy( szShortName, pPopFileName, sizeof( szShortName ) );
		V_StripExtension( szShortName, szShortName, sizeof( szShortName ) );

		if ( !V_stricmp( szShortName, STRING( gpGlobals->mapname ) ) )
			V_strcpy_safe( szShortName, "normal" );

		if ( outNames.Find( szShortName ) == outNames.InvalidIndex() )
			outNames.AddToTail( szShortName );

		pPopFileName = filesystem->FindNext( popHandle );
	}
	filesystem->FindClose( popHandle );

	int nDefaultIndex = outNames.Find( "normal" );
	if ( nDefaultIndex != outNames.InvalidIndex() && nDefaultIndex != 0 )
	{
		outNames.Remove( nDefaultIndex );
		outNames.AddToHead( "normal" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::PlayerUpgradeHistory *CPopulationManager::FindOrAddPlayerUpgradeHistory( CSteamID steamID ) const
{
	FOR_EACH_VEC( m_PlayerUpgrades, i )
	{
		if ( m_PlayerUpgrades[i]->m_steamID == steamID )
			return m_PlayerUpgrades[i];
	}

	PlayerUpgradeHistory *pHistory = new PlayerUpgradeHistory;
	pHistory->m_steamID = steamID;
	pHistory->m_nCurrencySpent = 0;
	m_PlayerUpgrades.AddToTail( pHistory );

	return pHistory;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPopulationManager::PlayerUpgradeHistory *CPopulationManager::FindOrAddPlayerUpgradeHistory( CTFPlayer *pPlayer ) const
{
	CSteamID steamID;
	if ( !pPlayer->GetSteamID( &steamID ) )
	{
		Log( "MvM : Unable to Find SteamID for player %s, unable to locate their upgrade history!", pPlayer->GetPlayerName() );
		return nullptr;
	}

	return FindOrAddPlayerUpgradeHistory( steamID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::FindPopulationFileByShortName( char const *pszName, CUtlString *outFullName )
{
	char szFullPath[MAX_PATH]{0};
	V_sprintf_safe( szFullPath, "scripts/population/%s.pop", pszName );

	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		*outFullName = szFullPath;
		return true;
	}

	V_sprintf_safe( szFullPath, "scripts/population/%s_%s.pop", STRING( gpGlobals->mapname ), pszName );
	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		*outFullName = szFullPath;
		return true;
	}

	V_sprintf_safe( szFullPath, "scripts/population/%s.pop", STRING( gpGlobals->mapname ) );
	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		*outFullName = szFullPath;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::ForgetOtherBottleUpgrades( CTFPlayer *pPlayer, CEconItemView *pItem, int nUpgradeKept )
{
	PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pPlayer );
	int iClass = pPlayer->GetPlayerClass()->GetClassIndex();

	FOR_EACH_VEC( pHistory->m_Upgrades, i )
	{
		CUpgradeInfo &info = pHistory->m_Upgrades[i];
		if ( info.m_iPlayerClass != iClass )
			continue;

		if ( info.m_nItemDefIndex != pItem->GetItemDefIndex() )
			continue;

		if ( info.m_iUpgrade == nUpgradeKept )
			continue;

		pHistory->m_Upgrades.FastRemove( i-- );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::GameRulesThink( void )
{
	// Some MM logic goes here
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWave *CPopulationManager::GetCurrentWave( void )
{
	if ( !m_bIsInitialized || m_Waves.Count() == 0 )
		return nullptr;

	if ( IsInEndlessWaves() )
	{
		return m_Waves[ m_nCurrentWaveIndex % m_Waves.Count() ];
	}
	else if ( m_nCurrentWaveIndex < m_Waves.Count() )
	{
		return m_Waves[ m_nCurrentWaveIndex ];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPopulationManager::GetDamageMultiplier( void ) const
{
	return tf_populator_damage_multiplier.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPopulationManager::GetHealthMultiplier( bool bTankMultiplier ) const
{
	if ( bTankMultiplier && IsInEndlessWaves() )
		return tf_populator_health_multiplier.GetFloat() + m_nCurrentWaveIndex * tf_mvm_endless_tank_boost.GetFloat();

	return tf_populator_health_multiplier.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPopulationManager::GetNumBuybackCreditsForPlayer( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();
		int nIndex = m_mapBuyBackCredits.Find( ulSteamId );

		if ( nIndex != m_mapBuyBackCredits.InvalidIndex() )
			return m_mapBuyBackCredits[ nIndex ];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPopulationManager::GetNumRespecsAvailableForPlayer( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();
		int nIndex = m_mapRespecs.Find( ulSteamId );

		if ( nIndex != m_mapRespecs.InvalidIndex() )
			return m_mapRespecs[ nIndex ];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPopulationManager::GetPlayerCurrencySpent( CTFPlayer *pPlayer )
{
	PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pPlayer );
	if ( pHistory )
		return pHistory->m_nCurrencySpent;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CUtlVector<CUpgradeInfo> *CPopulationManager::GetPlayerUpgradeHistory( CTFPlayer *pPlayer )
{
	PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pPlayer );
	if ( pHistory )
		return &pHistory->m_Upgrades;

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CPopulationManager::GetPopulationFilename( void ) const
{
	return m_szPopfileFull;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CPopulationManager::GetPopulationFilenameShort( void ) const
{
	return m_szPopfileShort;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::GetSentryBusterDamageAndKillThreshold( int &nNumDamage, int &nNumKills )
{
	int nNumSentryGuns = 0;
	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *pObject = assert_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( !pObject || pObject->GetType() != OBJ_SENTRYGUN || pObject->IsDisposableBuilding() )
			continue;

		if ( pObject->GetTeamNumber() == TF_TEAM_MVM_PLAYERS )
			nNumSentryGuns++;
	}

	float flScale = RemapValClamped( nNumSentryGuns, 1.0, 6.0, 1.0, 0.5 );
	if ( nNumSentryGuns <= 1 )
	{
		nNumDamage = m_iSentryBusterDamageDealtThreshold;
		nNumKills = m_iSentryBusterKillThreshold;
	}
	else
	{
		nNumDamage = m_iSentryBusterDamageDealtThreshold * flScale;
		nNumKills = m_iSentryBusterKillThreshold * flScale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPopulationManager::GetTotalPopFileCurrency( void )
{
	int nTotalCurrency = 0;
	FOR_EACH_VEC( m_Waves, i )
	{
		nTotalCurrency += m_Waves[i]->GetTotalCurrency();
	}

	return nTotalCurrency;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::HasEventChangeAttributes( char const *pszEventName )
{
	FOR_EACH_VEC( m_Waves, i )
	{
		if ( m_Waves[i]->HasEventChangeAttributes( pszEventName ) )
			return true;
	}

	FOR_EACH_VEC( m_Populators, i )
	{
		if ( m_Populators[i]->HasEventChangeAttributes( pszEventName ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::IsInEndlessWaves( void ) const
{
	return ( m_bIsEndless || tf_mvm_endless_force_on.GetBool() ) && !m_Waves.IsEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::IsPlayerBeingTrackedForBuybacks( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();
		uint16 nIndex = m_mapBuyBackCredits.Find( ulSteamId );

		return nIndex != m_mapBuyBackCredits.InvalidIndex();
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::IsValidMvMMap( char const *pszMapName )
{
	if ( m_pMvMMapCycle == NULL )
		LoadMissionCycleFile();

	if ( !pszMapName || !pszMapName[0] )
		return false;

	int nCatagories = m_pMvMMapCycle->GetInt( "catagories" );
	for ( int i = 1; i <= nCatagories; ++i )
	{
		KeyValues *pCatagory = m_pMvMMapCycle->FindKey( UTIL_VarArgs( "%d", i ) );
		if ( pCatagory )
		{
			int nCount = pCatagory->GetInt( "count" );
			for ( int j = 1; j <= nCount; ++j )
			{
				KeyValues *pMission = pCatagory->FindKey( UTIL_VarArgs( "%d", j ) );
				if ( pMission )
				{
					if ( !V_strcmp( pszMapName, pMission->GetString( "map" ) ) )
						return HaveMap( pszMapName );
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::JumpToWave( int nWave, float f1 )
{
	if ( IsInEndlessWaves() && nWave >= m_Waves.Count() )
	{
		if ( !m_Waves.IsEmpty() )
			Warning( "Invalid wave number %d\n", nWave );

		return;
	}

	CWave *pWave = GetCurrentWave();
	if ( pWave ) pWave->ForceFinish();

	m_bWaveJumping = true;
	m_nCurrentWaveIndex = nWave;

	if ( f1 != -1.0f )
	{
		ClearCheckpoint();
		Initialize();
			
		m_pMVMStats->ResetStats();

		for ( m_nCurrentWaveIndex = 0; m_nCurrentWaveIndex < nWave; ++m_nCurrentWaveIndex )
		{
			pWave = GetCurrentWave();
			if ( pWave )
			{
				int nCurrency = pWave->GetTotalCurrency();
				m_pMVMStats->SetCurrentWave( m_nCurrentWaveIndex );

				if ( m_nCurrentWaveIndex < nWave )
				{
					m_pMVMStats->RoundEvent_CreditsDropped( m_nCurrentWaveIndex, nCurrency );
					m_pMVMStats->RoundEvent_AcquiredCredits( m_nCurrentWaveIndex, nCurrency * f1, false );
				}
			}
		}
	}

	m_nCurrentWaveIndex = nWave;

	pWave = GetCurrentWave();
	if ( pWave ) pWave->ForceReset();

	m_pMVMStats->SetCurrentWave( m_nCurrentWaveIndex );

	if ( IsInEndlessWaves() )
		EndlessRollEscalation();

	UpdateObjectiveResource();
	SetCheckpoint( -1 );
	ResetRespecPoints();

	TFGameRules()->SetAllowBetweenRounds( true );
	TFGameRules()->State_Transition( GR_STATE_PREROUND );
	TFGameRules()->PlayerReadyStatus_ResetState();

	TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );

	RestorePlayerCurrency();
	m_bWaveJumping = false;

	CTF_GameStats.ResetRoundStats();

	IGameEvent *event = gameeventmanager->CreateEvent( "mvm_reset_stats" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::LoadLastKnownMission( void )
{
	if ( !m_pMvMMapCycle && !LoadMissionCycleFile() )
	{
		ResetMap();
		return;
	}

	KeyValues *pCatagory = m_pMvMMapCycle->FindKey( UTIL_VarArgs( "%d", s_iLastKnownMissionCategory ) );
	if ( pCatagory )
	{
		KeyValues *pMission = pCatagory->FindKey( UTIL_VarArgs( "%d", s_iLastKnownMission ) );
		if ( pMission )
		{
			if ( LoadMvMMission( pMission ) )
				return;
		}
	}

	pCatagory = m_pMvMMapCycle->FindKey( UTIL_VarArgs( "%d", 1 ) );
	if ( pCatagory )
	{
		KeyValues *pMission = pCatagory->FindKey( UTIL_VarArgs( "%d" ), 1 );
		if ( pMission )
		{
			if ( !LoadMvMMission( pMission ) )
			{
				ResetMap();
				return;
			}
		}
	}

	ResetMap();
	
	s_iLastKnownMission = 1;
	s_iLastKnownMissionCategory = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::LoadMissionCycleFile( void )
{
	if ( m_pMvMMapCycle )
		m_pMvMMapCycle->deleteThis();

	char const *pszMissionCycle = tf_mvm_missioncyclefile.GetString();
	m_pMvMMapCycle = new KeyValues( pszMissionCycle );
	return m_pMvMMapCycle->LoadFromFile( g_pFullFileSystem, pszMissionCycle, "MOD" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::LoadMvMMission( KeyValues *pMissionKV )
{
	if ( pMissionKV == NULL )
		return false;

	char const *pszMap = pMissionKV->GetString( "map" );
	char const *pszPopFile = pMissionKV->GetString( "popfile" );
	if ( !pszMap || !pszPopFile )
		return false;

	char szPopFile[MAX_PATH];
	V_sprintf_safe( szPopFile, "scripts/population/%s.pop", pszPopFile );
	if ( !g_pFullFileSystem->FileExists( szPopFile, "MOD" ) )
		return false;

	if ( !HaveMap( pszMap ) )
		return false;

	engine->ChangeLevel( pszMap, NULL );
	TFGameRules()->SetNextMvMPopfile( pszPopFile );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::MarkAllCurrentPlayersSafeToLeave( void )
{
	// MM stuff
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::MvMVictory( void )
{
	for ( int i = 0; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
			continue;

		pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_BONUS_TIME, 10.0f );
	}

	TFGameRules()->BroadcastSound( 255, "Game.YourTeamWon" );

	m_pMVMStats->RoundOver( true );

	bool bDisconnect = tf_mvm_disconnect_on_victory.GetBool();
	float flActionTime = bDisconnect ? tf_mvm_victory_disconnect_time.GetFloat() : tf_mvm_victory_reset_time.GetFloat();

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMVictory" );
		WRITE_BYTE( bDisconnect );
		WRITE_BYTE( RoundFloatToByte( flActionTime ) );
	MessageEnd();

	// MM stuff
	// GTFGCClientSystem()->SendMvMVictoryResult();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::OnCurrencyCollected( int nCurrency, bool bWasDropped, bool bBonus )
{
	bool bBetweenWaves = TFObjectiveResource()->GetMannVsMachineIsBetweenWaves();
	int nCurrentWave = bBetweenWaves ? m_nCurrentWaveIndex - 1 : m_nCurrentWaveIndex;

	if ( bWasDropped )
		m_pMVMStats->RoundEvent_CreditsDropped( nCurrentWave, nCurrency );

	m_pMVMStats->RoundEvent_AcquiredCredits( nCurrentWave, nCurrency, bBonus );

	int const nRespecLimit = tf_mvm_respec_limit.GetInt();
	if ( nRespecLimit != 0 )
	{
		if ( m_nRespecsAwarded < nRespecLimit )
		{
			m_nCurrencyForRespec += nCurrency;

			int nRespecCreditGoal = tf_mvm_respec_credit_goal.GetFloat();
			while ( m_nCurrencyForRespec > nRespecCreditGoal )
			{
				m_nRespecsAwardedInWave++;
				m_nRespecsAwarded++;

				CUtlVector<CTFPlayer *> humans;
				CollectPlayers( &humans, TF_TEAM_MVM_PLAYERS );
				FOR_EACH_VEC( humans, i )
				{
					AddRespecToPlayer( humans[i] );
				}

				if ( m_nRespecsAwarded < nRespecLimit )
					m_nCurrencyForRespec -= nRespecCreditGoal;
				else
					m_nCurrencyForRespec = nRespecCreditGoal;
			}

			if ( m_pMVMStats )
			{
				m_pMVMStats->SetNumRespecsEarnedInWave( m_nRespecsAwardedInWave );
				m_pMVMStats->SetAcquiredCreditsForRespec( m_nCurrencyForRespec );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
void CPopulationManager::OnCurrencyPackFade( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::OnPlayerKilled( CTFPlayer *pPlayer )
{
	FOR_EACH_VEC( m_Populators, i )
	{
		m_Populators[i]->OnPlayerKilled( pPlayer );
	}

	CWave *pWave = GetCurrentWave();
	if ( pWave ) pWave->OnPlayerKilled( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPopulationManager::Parse( void )
{
	if ( m_szPopfileFull[0] == '\0' )
	{
		Warning( "No population file specified.\n" );
		return false;
	}

	KeyValuesAD pKV( "Population" );
	if ( !pKV->LoadFromFile( filesystem, m_szPopfileFull, "GAME" ) )
	{
		Warning( "Can't open %s.\n", m_szPopfileFull );
		pKV->deleteThis();
		return false;
	}

	m_Populators.PurgeAndDeleteElements();
	m_Waves.RemoveAll();

	if ( m_pTemplates )
	{
		m_pTemplates->deleteThis();
		m_pTemplates = NULL;
	}

	KeyValues *pKVTemplates = pKV->FindKey( "Templates" );
	if ( pKVTemplates )
	{
		m_pTemplates = pKVTemplates->MakeCopy();
	}

	FOR_EACH_SUBKEY( pKV, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "StartingCurrency" ) )
		{
			m_nStartingCurrency = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "RespawnBaveTime" ) )
		{
			m_nRespawnWaveTime = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "EventPopFile" ) )
		{
			if ( !V_stricmp( pSubKey->GetString(), "Halloween" ) )
			{
				m_nMvMEventPopfileType = MVM_EVENT_POPFILE_HALLOWEEN;
			}
			else
			{
				m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;
			}
		}
		else if ( !V_stricmp( pszKey, "FixedRespawnWaveTime" ) )
		{
			m_bFixedRespawnWaveTime = true;
		}
		else if ( !V_stricmp( pszKey, "AddSentryBusterWhenDamageDealtExceeds" ) )
		{
			m_iSentryBusterDamageDealtThreshold = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "AddSentryBusterWhenKillCountExceeds" ) )
		{
			m_iSentryBusterKillThreshold = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "CanBotsAttackInSpawnRoom" ) )
		{
			// Why is "no" and "false" taken instead of pSubKey->GetBool ??
			if ( !V_stricmp( pSubKey->GetString(), "no" ) || !V_stricmp( pSubKey->GetString(), "false" ) )
			{
				m_bCanBotsAttackWhileInSpawnRoom = false;
			}
			else
			{
				m_bCanBotsAttackWhileInSpawnRoom = true;
			}
		}
		else if ( !V_stricmp( pszKey, "RandomPlacement" ) )
		{
			CRandomPlacementPopulator *pPopulator = new CRandomPlacementPopulator( this );
			if ( !pPopulator->Parse( pSubKey ) )
			{
				Warning( "Error reading RandomPlacement definition\n" );
				return false;
			}

			m_Populators.AddToTail( pPopulator );
		}
		else if ( !V_stricmp( pszKey, "PeriodicSpawn" ) )
		{
			CPeriodicSpawnPopulator *pPopulator = new CPeriodicSpawnPopulator( this );
			if ( !pPopulator->Parse( pSubKey ) )
			{
				Warning( "Error reading PeriodicSpawn definition\n" );
				return false;
			}

			m_Populators.AddToTail( pPopulator );
		}
		else if ( !V_stricmp( pszKey, "Wave" ) )
		{
			CWave *pWave = new CWave( this );
			if ( !pWave->Parse( pSubKey ) )
			{
				Warning( "Error reading Wave definition\n" );
				return false;
			}

			m_Waves.AddToTail( pWave );
		}
		else if ( !V_stricmp( pszKey, "Mission" ) )
		{
			CMissionPopulator *pPopulator = new CMissionPopulator( this );
			if ( !pPopulator->Parse( pSubKey ) )
			{
				Warning( "Error reading RandomPlacement definition\n" );
				return false;
			}

			m_Populators.AddToTail( pPopulator );
		}
		else if ( !V_stricmp( pszKey, "Templates" ) )
		{

		}
		else if ( !V_stricmp( pszKey, "Advanced" ) )
		{
			m_bIsAdvanced = true;
		}
		else if ( !V_stricmp( pszKey, "IsEndless" ) )
		{
			m_bIsEndless = true;
		}
		else
		{
			Warning( "Invalid populator '%s'\n", pszKey );
			return false;
		}
	}

	FOR_EACH_VEC( m_Populators, i )
	{
		CMissionPopulator *pMission = dynamic_cast<CMissionPopulator *>( m_Populators[i] );
		if ( pMission )
		{
			if ( pMission->GetSpawner() && !pMission->GetSpawner()->IsVarious() )
			{
				for ( int i = pMission->m_nStartWave; i < pMission->m_nEndWave; ++i )
				{
					if ( m_Waves.IsValidIndex( i ) )
					{
						CWave *pWave = m_Waves[i];

						unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
						if ( pMission->GetSpawner()->IsMiniBoss() )
						{
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;
						}
						if ( pMission->GetSpawner()->HasAttribute( CTFBot::AttributeType::ALWAYSCRIT ) )
						{
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
						}
						pWave->AddClassType( pMission->GetSpawner()->GetClassIcon(), 0, iFlags );
					}
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::PauseSpawning( void )
{
	DevMsg( "Wave paused\n" );
	m_bSpawningPaused = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::PlayerDoneViewingLoot( CTFPlayer const *pPlayer )
{
	if ( cutlvector6BC.Find( pPlayer ) != cutlvector6BC.InvalidIndex() )
		return;

	CUtlVector<CTFPlayer const *> humans;
	CollectPlayers( &humans, TF_TEAM_MVM_PLAYERS );
	if ( humans.Find( pPlayer ) == humans.InvalidIndex() )
		return;

	cutlvector6BC.AddToTail( pPlayer );

	float flTimeRemaining = m_flTimeToRestart - gpGlobals->curtime;
	if ( flTimeRemaining > 15.0f )
	{
		float flTimeDeduction = ( /*tf_mm_trusted.GetBool() ||*/ tf_mvm_disconnect_on_victory.GetBool() ) ?
								tf_mvm_victory_disconnect_time.GetFloat() 
								:
								tf_mvm_victory_reset_time.GetFloat();

		flTimeDeduction = flTimeDeduction * 0.8 / humans.Count();
		flTimeRemaining = fmaxf( 15.0, flTimeRemaining - flTimeDeduction );

		m_flTimeToRestart = gpGlobals->curtime + flTimeRemaining;

		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "MVMServerKickTimeUpdate" );
			WRITE_BYTE( RoundFloatToByte( flTimeRemaining ) );
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::PostInitialize( void )
{
	if ( TheNavMesh->GetNavAreaCount() <= 0 )
	{
		Warning( "Cannot populate - no Navigation Mesh exists.\n" );
		return;
	}

	FOR_EACH_VEC( m_Populators, i )
	{
		m_Populators[i]->PostInitialize();
	}

	FOR_EACH_VEC( m_Waves, i )
	{
		m_Waves[i]->PostInitialize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RemoveBuybackCreditFromPlayer( CTFPlayer *pPlayer )
{
	if ( !tf_mvm_buybacks_method.GetInt() )
		return;

	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();

		int nIndex = m_mapBuyBackCredits.Find( ulSteamId );
		if ( nIndex != m_mapBuyBackCredits.InvalidIndex() )
			m_mapBuyBackCredits[ nIndex ] -= 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RemovePlayerAndItemUpgradesFromHistory( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( !pPlayer->GetSteamID( &steamID ) )
		return;

	FOR_EACH_VEC_BACK( sm_checkpointSnapshots, i )
	{
		CheckpointSnapshotInfo *pInfo = sm_checkpointSnapshots[i];
		if ( pInfo->m_steamID == steamID )
		{
			FOR_EACH_VEC_BACK( pInfo->m_Upgrades, j )
			{
				int const iUpgrade = pInfo->m_Upgrades[j].m_iUpgrade;
				CMannVsMachineUpgrades *pUpgrade = &( g_MannVsMachineUpgrades.GetUpgradeVector()[ iUpgrade ] );

				if ( pUpgrade )
				{
					if ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ITEM || pUpgrade->nUIGroup == UIGROUP_UPGRADE_PLAYER )
					{
						pInfo->m_nCurrencySpent -= pUpgrade->nCost;
						pInfo->m_Upgrades.Remove( j );
					}
				}
			}
		}
	}

	FOR_EACH_VEC_BACK( m_PlayerUpgrades, i )
	{
		PlayerUpgradeHistory *pHistory = m_PlayerUpgrades[i];
		if ( pHistory->m_steamID == steamID )
		{
			FOR_EACH_VEC_BACK( pHistory->m_Upgrades, j )
			{
				int const iUpgrade = pHistory->m_Upgrades[j].m_iUpgrade;
				CMannVsMachineUpgrades *pUpgrade = &( g_MannVsMachineUpgrades.GetUpgradeVector()[ iUpgrade ] );

				if ( pUpgrade )
				{
					if ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ITEM || pUpgrade->nUIGroup == UIGROUP_UPGRADE_PLAYER )
					{
						pHistory->m_nCurrencySpent -= pUpgrade->nCost;
						pHistory->m_Upgrades.Remove( j );
					}
				}
			}
		}
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && m_pMVMStats )
	{
		int nTotalAcquiredCurrency = m_pMVMStats->GetAcquiredCredits( -1 ) + GetStartingCurrency();
		pPlayer->SetCurrency( nTotalAcquiredCurrency - GetPlayerCurrencySpent( pPlayer ) );

		// Reset the stat that tracks upgrade purchases
		m_pMVMStats->ResetUpgradeSpending( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RemoveRespecFromPlayer( CTFPlayer *pPlayer )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();

		int nIndex = m_mapRespecs.Find( ulSteamId );
		if ( nIndex != m_mapRespecs.InvalidIndex() )
			m_mapRespecs[ nIndex ] -= 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::ResetMap( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer || FNullEnt( pPlayer->edict() ) )
			continue;

		if ( pPlayer->GetTeamNumber() != TF_TEAM_MVM_PLAYERS )
			continue;

		pPlayer->ResetScores();
	}

	m_pMVMStats->ResetStats();

	ResetRespecPoints();
	ClearCheckpoint();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer || FNullEnt( pPlayer->edict() ) )
			continue;

		if ( pPlayer->IsBot() )
			continue;

		pPlayer->m_RefundableUpgrades.RemoveAll();
	}

	JumpToWave( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::ResetRespecPoints( void )
{
	m_mapRespecs.RemoveAll();
	m_nRespecsAwarded = 0;
	m_nCurrencyForRespec = 0;
	m_nRespecsAwardedInWave = 0;

	if ( tf_mvm_respec_limit.GetBool() )
	{
		if ( m_pMVMStats )
		{
			m_pMVMStats->SetNumRespecsEarnedInWave( m_nRespecsAwardedInWave );
			m_pMVMStats->SetAcquiredCreditsForRespec( m_nCurrencyForRespec );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RestoreCheckpoint( void )
{
	m_bRestoringCheckpoint = true;

	if ( !IsInEndlessWaves() )
		m_nCurrentWaveIndex = m_nCheckpointWaveIndex;

	m_PlayerUpgrades.PurgeAndDeleteElements();
	FOR_EACH_VEC( sm_checkpointSnapshots, i )
	{
		CheckpointSnapshotInfo *pInfo = sm_checkpointSnapshots[i];

		PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pInfo->m_steamID );
		pHistory->m_nCurrencySpent = pInfo->m_nCurrencySpent;

		pHistory->m_Upgrades.RemoveAll();
		FOR_EACH_VEC( pInfo->m_Upgrades, j )
		{
			pHistory->m_Upgrades.AddToTail( pInfo->m_Upgrades[j] );
		}
	}

	CUtlVector<CTFPlayer *> humans;
	CollectHumanPlayers( &humans, TF_TEAM_MVM_PLAYERS );
	FOR_EACH_VEC( humans, i )
	{
		CTFPlayer *pPlayer = humans[i];

		pPlayer->m_nAccumulatedSentryGunDamageDealt = 0;
		pPlayer->m_nAccumulatedSentryGunKillCount = 0;

		CEconWearable *pWearable = pPlayer->GetWearableForLoadoutSlot( TF_LOADOUT_SLOT_ACTION );
		if ( pWearable )
		{
			CTFPowerupBottle *pBottle = dynamic_cast<CTFPowerupBottle *>( pWearable );
			if ( pBottle )
				pBottle->Reset();
		}

		SendUpgradesToPlayer( pPlayer );
	}

	m_nNumConsecutiveWipes++;
	m_nRespecsAwardedInWave = 0;

	m_pMVMStats->SetCurrentWave( m_nCurrentWaveIndex );

	UpdateObjectiveResource();

	TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Get_To_Upgrade" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RestoreItemToCheckpoint( CTFPlayer *pPlayer, CEconItemView *pItem )
{
	CheckpointSnapshotInfo *pInfo = FindCheckpointSnapshot( pPlayer );
	if ( pInfo == NULL )
		return;

	if ( pItem == NULL )
		return;

	FOR_EACH_VEC( pInfo->m_Upgrades, i )
	{
		CUpgradeInfo &info = pInfo->m_Upgrades[i];

		if ( info.m_nItemDefIndex != pItem->GetItemDefIndex() )
			continue;

		if ( pPlayer->GetPlayerClass()->GetClassIndex() != info.m_iPlayerClass )
			continue;

		if ( g_hUpgradeEntity->ApplyUpgradeToItem( pPlayer, pItem, info.m_iUpgrade, info.m_nCost ) )
		{
			if ( tf_populator_debug.GetBool() )
			{
				const char *pszUpgradeName = g_hUpgradeEntity->GetUpgradeAttributeName( info.m_iUpgrade );
				DevMsg( "%3.2f: CHECKPOINT_RESTORE_ITEM: Player '%s', item '%s', upgrade '%s'\n",
						gpGlobals->curtime,
						pPlayer->GetPlayerName(),
						pItem->GetStaticData()->GetName(),
						pszUpgradeName ? pszUpgradeName : "<self>" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::RestorePlayerCurrency( void )
{
	int nCurrency = m_pMVMStats->GetAcquiredCredits( -1 ) + GetStartingCurrency();

	CUtlVector<CTFPlayer *> humans;
	CollectHumanPlayers( &humans, TF_TEAM_MVM_PLAYERS );
	FOR_EACH_VEC( humans, i )
	{
		humans[i]->SetCurrency( nCurrency - GetPlayerCurrencySpent( humans[i] ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SendUpgradesToPlayer( CTFPlayer *pPlayer )
{
	CUtlVector<CUpgradeInfo> *upgrades = NULL;

	PlayerUpgradeHistory *pHistory = FindOrAddPlayerUpgradeHistory( pPlayer );
	if ( pHistory )
	{
		upgrades = &pHistory->m_Upgrades;
	}

	m_pMVMStats->SendUpgradesToPlayer( pPlayer, upgrades );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SetBuybackCreditsForPlayer( CTFPlayer *pPlayer, int nCredits )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();

		uint16 nIndex = m_mapBuyBackCredits.Find( ulSteamId );
		if ( nIndex == m_mapBuyBackCredits.InvalidIndex() )
			nIndex = m_mapBuyBackCredits.Insert( ulSteamId );

		m_mapBuyBackCredits[ nIndex ] = nCredits;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SetCheckpoint( int nWave )
{
	if ( !IsInEndlessWaves() )
	{
		if ( nWave < 0 )
		{
			nWave = m_nCurrentWaveIndex;
			if ( nWave < 0 )
			{
				Warning( "Warning: SetCheckpoint() called with invalid wave number %d\n", nWave );
				return;
			}
		}

		if ( m_Waves.Count() <= nWave )
		{
			Warning( "Warning: SetCheckpoint() called with invalid wave number %d\n", nWave );
			return;
		}

		DevMsg( "Checkpoint Saved\n" );

		m_nCheckpointWaveIndex = nWave;
		m_nNumConsecutiveWipes = 0;

		FOR_EACH_VEC( m_PlayerUpgrades, i )
		{
			CheckpointSnapshotInfo *pInfo = FindCheckpointSnapshot( m_PlayerUpgrades[i]->m_steamID );
			if ( pInfo )
			{
				pInfo->m_nCurrencySpent = m_PlayerUpgrades[i]->m_nCurrencySpent;

				pInfo->m_Upgrades.RemoveAll();
				FOR_EACH_VEC( m_PlayerUpgrades[i]->m_Upgrades, k )
				{
					pInfo->m_Upgrades.AddToTail( m_PlayerUpgrades[i]->m_Upgrades[k] );
				}

				return;
			}

			pInfo = new CheckpointSnapshotInfo;
			pInfo->m_steamID = m_PlayerUpgrades[i]->m_steamID;
			pInfo->m_nCurrencySpent = m_PlayerUpgrades[i]->m_nCurrencySpent;
			
			pInfo->m_Upgrades.RemoveAll();
			FOR_EACH_VEC( m_PlayerUpgrades[i]->m_Upgrades, j )
			{
				pInfo->m_Upgrades.AddToTail( m_PlayerUpgrades[i]->m_Upgrades[j] );
			}

			sm_checkpointSnapshots.AddToTail( pInfo );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SetNumRespecsForPlayer( CTFPlayer *pPlayer, int nRespecs )
{
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		uint64 ulSteamId = steamID.ConvertToUint64();

		uint16 nIndex = m_mapRespecs.Find( ulSteamId );
		if ( nIndex == m_mapRespecs.InvalidIndex() )
			nIndex = m_mapRespecs.Insert( ulSteamId );

		m_mapRespecs[ nIndex ] = nRespecs;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SetPopulationFilename( char const *pszFileName )
{
	m_bIsInitialized = false;

	V_strcpy_safe( m_szPopfileFull, pszFileName );
	V_FileBase( m_szPopfileFull, m_szPopfileShort, sizeof( m_szPopfileShort ) );

	MannVsMachineStats_SetPopulationFile( m_szPopfileFull );

	ResetMap();

	if ( TFObjectiveResource() )
	{
		//TFObjectiveResource()->SetMannVsMachineChallengeIndex( GetItemSchema()->FindMvmMissionByName( m_szPopfileFull ) );
		TFObjectiveResource()->SetMvMPopfileName( MAKE_STRING( m_szPopfileFull ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::SetupOnRoundStart( void )
{
	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::ShowNextWaveDescription( void )
{
	UpdateObjectiveResource();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::StartCurrentWave( void )
{
	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetMannVsMachineNextWaveTime( 0 );
		TFObjectiveResource()->SetMannVsMachineBetweenWaves( false );
	}

	UpdateObjectiveResource();
	m_pMVMStats->RoundEvent_WaveStart();

	TFGameRules()->State_Transition( GR_STATE_RND_RUNNING );

	m_nRespecsAwardedInWave = 0;

	FOR_EACH_MAP( m_mapBuyBackCredits, i )
	{
		m_mapBuyBackCredits[i] = tf_mvm_buybacks_per_wave.GetInt();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::UnpauseSpawning( void )
{
	DevMsg( "Wave unpaused\n" );
	m_bSpawningPaused = false;

	FOR_EACH_VEC( m_Populators, i )
	{
		m_Populators[i]->UnpauseSpawning();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::Update( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );
	SetNextThink( gpGlobals->curtime );

	FOR_EACH_VEC( m_Populators, i )
	{
		m_Populators[i]->Update();
	}

	CWave *pWave = GetCurrentWave();
	if ( pWave ) pWave->Update();

	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		if ( m_flTimeToRestart < gpGlobals->curtime )
		{
			if ( tf_mvm_disconnect_on_victory.GetBool() )
			{
				// TODO: Alternative to GC
				/*if ( !TFGameRules()->IsManagedMatchEnded() )
				{
					TFGameRules()->EndManagedMvMMatch( true );
				}*/
			}
			else
			{
				CycleMission();
			}
		}

		if ( tf_mvm_disconnect_on_victory.GetBool() && ( m_flTimeToRestart + 5.0f ) < gpGlobals->curtime )
		{
			Log( "Kicking all players\n" );
			engine->ServerCommand( "kickall #TF_PVE_Disconnect" );
			CycleMission();
		}

		// Logic for achievements
		if ( m_bRestoringCheckpoint && MannVsMachineStats_GetDroppedCredits() && !MannVsMachineStats_GetMissedCredits() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( m_bIsAdvanced ? "mvm_creditbonus_all_advanced" : "mvm_creditbonus_all" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}

			m_bRestoringCheckpoint = false;
		}
	}
	else if ( TFGameRules()->State_Get() == GR_STATE_STARTGAME )
	{
		AllocateBots();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::UpdateObjectiveResource( void )
{
	if ( m_Waves.IsEmpty() || !TFObjectiveResource() )
		return;

	TFObjectiveResource()->SetMannVsMachineEventPopfileType( m_nMvMEventPopfileType );

	if ( IsInEndlessWaves() )
		TFObjectiveResource()->SetMannVsMachineMaxWaveCount( 0 );
	else
		TFObjectiveResource()->SetMannVsMachineMaxWaveCount( m_Waves.Count() );

	TFObjectiveResource()->SetMannVsMachineWaveCount( m_nCurrentWaveIndex + 1 );

	CWave *pWave = GetCurrentWave();
	if ( pWave )
	{
		TFObjectiveResource()->SetMannVsMachineWaveEnemyCount( pWave->m_nTotalEnemyCount );

		TFObjectiveResource()->ClearMannVsMachineWaveClassFlags();

		int nTypes = 0; bool bHasEngineer = false;
		while ( nTypes < MVM_CLASS_TYPES_PER_WAVE_MAX && nTypes < pWave->GetNumClassTypes() )
		{
			bHasEngineer |= FStrEq( pWave->GetClassIconName( nTypes ).ToCStr(), "engineer" );

			TFObjectiveResource()->SetMannVsMachineWaveClassName( nTypes, pWave->GetClassIconName( nTypes ) );
			TFObjectiveResource()->SetMannVsMachineWaveClassCount( nTypes, pWave->GetClassCount( nTypes ) );
			TFObjectiveResource()->AddMannVsMachineWaveClassFlags( nTypes, pWave->GetClassFlags( nTypes ) );

			nTypes++;
		}

		if ( bHasEngineer )
		{
			if ( nTypes < MVM_CLASS_TYPES_PER_WAVE_MAX )
			{
				TFObjectiveResource()->SetMannVsMachineWaveClassName( nTypes, AllocPooledString( "teleporter" ) );
				TFObjectiveResource()->SetMannVsMachineWaveClassCount( nTypes, 0 );
				TFObjectiveResource()->AddMannVsMachineWaveClassFlags( nTypes, MVM_CLASS_FLAG_MISSION );

				nTypes++;
			}
		}

		while ( nTypes++ <= MVM_CLASS_TYPES_PER_WAVE_MAX )
		{
			TFObjectiveResource()->SetMannVsMachineWaveClassCount( nTypes, 0 );
			TFObjectiveResource()->SetMannVsMachineWaveClassName( nTypes, NULL_STRING );
			TFObjectiveResource()->AddMannVsMachineWaveClassFlags( nTypes, MVM_CLASS_FLAG_NONE );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPopulationManager::WaveEnd( bool bComplete )
{
	m_pMVMStats->RoundEvent_WaveEnd( bComplete );

	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	CTF_GameStats.ResetRoundStats();
	MarkAllCurrentPlayersSafeToLeave();

	if ( bComplete )
	{
		if ( byte58A )
		{
			if ( ehandle58C )
			{
				UTIL_Remove( ehandle58C );
				ehandle58C = NULL;
			}

			byte58A = false;
		}
		else
		{
			m_nCurrentWaveIndex++;
		}

		m_pMVMStats->SetCurrentWave( m_nCurrentWaveIndex );

		CWave *pWave = GetCurrentWave();
		if ( pWave )
		{
			SetCheckpoint( -1 );
			UpdateObjectiveResource();

			pWave->m_upgradeAlertTimer.Start( 3.0f );

			if ( IsInEndlessWaves() )
			{
				EndlessRollEscalation();

				pWave->ForceReset();

				float flTime = gpGlobals->curtime + tf_mvm_endless_wait_time.GetFloat();
				if ( m_nCurrentWaveIndex % tf_mvm_endless_bomb_reset.GetInt() == 0 )
				{
					flTime += tf_mvm_endless_wait_time.GetFloat();
					m_bShouldResetFlag = true;
				}

				pWave->m_flStartTime = flTime;
			}
		}

		if ( m_nCurrentWaveIndex >= m_Waves.Count() && !IsInEndlessWaves() )
		{
			if ( /*tf_mm_trusted.GetBool() ||*/ tf_mvm_disconnect_on_victory.GetBool() )
				m_flTimeToRestart = gpGlobals->curtime + tf_mvm_victory_disconnect_time.GetFloat();
			else
				m_flTimeToRestart = gpGlobals->curtime + tf_mvm_victory_reset_time.GetFloat();

			TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
			TFGameRules()->State_Transition( GR_STATE_GAME_OVER );

			return;
		}
	}

	if ( !IsInEndlessWaves() )
	{
		TFGameRules()->State_Transition( GR_STATE_BETWEEN_RNDS );
		TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
	}
}


CON_COMMAND_F( tf_mvm_nextmission, "Load the next mission", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->CycleMission();
	}
}

CON_COMMAND_F( tf_mvm_force_victory, "Force immediate victory.", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->JumpToWave( g_pPopulationManager->m_Waves.Count() - 1 );
		g_pPopulationManager->WaveEnd( true );
		g_pPopulationManager->MvMVictory();
	}
}

CON_COMMAND_F( tf_mvm_checkpoint, "Save a checkpoint snapshot", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->SetCheckpoint( -1 );
	}
}

CON_COMMAND_F( tf_mvm_checkpoint_clear, "Clear the saved checkpoint", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->ClearCheckpoint();
	}
}

CON_COMMAND_F( tf_mvm_jump_to_wave, "Jumps directly to the given Mann Vs Machine wave number", FCVAR_CHEAT )
{
	if ( args.ArgC() <= 1 )
	{
		Msg( "Missing wave number\n" );
		return;
	}

	float f1 = -1.0f;
	if ( args.ArgC() >= 3 )
	{
		f1 = atof( args.Arg( 2 ) );
	}

	// find the population manager
	CPopulationManager *manager = (CPopulationManager *)gEntList.FindEntityByClassname( NULL, "info_populator" );
	if ( !manager )
	{
		Msg( "No Population Manager found in the map\n" );
		return;
	}

	uint32 desiredWave = (uint32)Max( atoi( args.Arg( 1 ) ) - 1, 0 );
	manager->JumpToWave( desiredWave, f1 );
}

CON_COMMAND_F( tf_mvm_debugstats, "Dumpout MvM Data", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( g_pPopulationManager )
	{
		g_pPopulationManager->DebugWaveStats();
	}
}
