//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_gamerules.h"
#include "steam/steamclientpublic.h"

#ifdef GAME_DLL
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
	#include "hud_macros.h"
#endif

CBasePlayer *GetPlayerBySteamID( const CSteamID &targetID )
{
	CSteamID steamID;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer || !pPlayer->GetSteamID( &steamID ) )
			continue;

		if ( steamID == targetID )
			return pPlayer;
	}
	return NULL;
}

static CMannVsMachineStats *g_pMVMStats = NULL;

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMLocalPlayerUpgradesClear( bf_read &msg )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->ClearLocalPlayerUpgrades();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMLocalPlayerUpgradesValue( bf_read &msg )
{
	uint8 nClass = msg.ReadByte();
	uint16 nItemDef = msg.ReadWord();

	if ( g_pMVMStats )
	{
		g_pMVMStats->AddLocalPlayerUpgrade( nClass, nItemDef );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMLocalPlayerWaveSpendingValue( bf_read &msg )
{
	uint64 steamId;
	if ( !msg.ReadBytes( &steamId, sizeof( steamId ) ) )
	{
		DevMsg( " Unable to Get Player SteamID from __MsgFunc_MVMLocalPlayerWaveSpendingValue() " );
		return;
	}

	uint8 nWave = msg.ReadByte();
	uint8 nEvent = msg.ReadByte();
	uint16 nCost = msg.ReadWord();

	if ( g_pMVMStats )
	{
		CPlayerWaveSpendingStats *pStats = g_pMVMStats->GetSpending( nWave, steamId );
		if ( pStats == NULL )
			return;

		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerBySteamID( steamId ) );

		switch ( nEvent )
		{
			case MVMEvent_BoughtInstantRespawn:
			{
				pStats->nCreditsSpentOnBuyBacks += nCost;
				if ( pPlayer == C_TFPlayer::GetLocalTFPlayer() )
					g_pMVMStats->SW_ReportClientBuyBackPurchase( nWave, nCost );

				break;
			}
			case MVMEvent_BoughtBottle:
			{
				pStats->nCreditsSpentOnBottles += nCost;
				break;
			}
			case MVMEvent_BoughtUpgrade:
			{
				pStats->nCreditsSpentOnUpgrades += nCost;
				break;
			}
			case MVMEvent_ActiveUpgrades:
			{
				g_pMVMStats->SetPlayerActiveUpgradeCosts( steamId, nCost );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMPlayerEvent( bf_read &msg )
{
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMPlayerUpgradedEvent( bf_read &msg )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMResetPlayerStats( bf_read &msg )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( msg.ReadByte() ) );
	if ( pPlayer == NULL )
		return;

	if ( g_pMVMStats )
	{
		g_pMVMStats->ResetPlayerEvents( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMResetPlayerUpgradeSpending( bf_read &msg )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( msg.ReadByte() ) );
	if ( pPlayer == NULL )
		return;

	if ( g_pMVMStats )
	{
		g_pMVMStats->ResetUpgradeSpending( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMResetPlayerWaveSpendingStats( bf_read &msg )
{
	uint8 nWave = msg.ReadByte();

	if ( g_pMVMStats )
	{
		g_pMVMStats->ClearCurrentPlayerWaveSpendingStats( nWave );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMStatsReset( bf_read &msg )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->ResetStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void __MsgFunc_MVMWaveChange( bf_read &msg )
{
	uint16 nWave = msg.ReadWord();
	CMannVsMachinePlayerStats stats ={
		msg.ReadUBitLong( sizeof( unsigned char ) << 3 ),
		msg.ReadUBitLong( sizeof( long ) << 3 ),
		msg.ReadUBitLong( sizeof( long ) << 3 ),
		msg.ReadUBitLong( sizeof( long ) << 3 )
	};

	if ( g_pMVMStats )
	{
		g_pMVMStats->SW_ReportClientWaveSummary( nWave, stats );
	}
}
#endif


BEGIN_NETWORK_TABLE_NOBASE( CMannVsMachineWaveStats, DT_CMannVsMachineWaveStats )
#if defined( GAME_DLL )
	SendPropInt( SENDINFO( nCreditsDropped ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nCreditsAcquired ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nCreditsBonus ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nPlayerDeaths ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( nBuyBacks ), 8, SPROP_UNSIGNED ),
#else
	RecvPropInt( RECVINFO( nCreditsDropped ) ),
	RecvPropInt( RECVINFO( nCreditsAcquired ) ),
	RecvPropInt( RECVINFO( nCreditsBonus ) ),
	RecvPropInt( RECVINFO( nPlayerDeaths ) ),
	RecvPropInt( RECVINFO( nBuyBacks ) ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( MannVsMachineStats, DT_MannVsMachineStats )

BEGIN_NETWORK_TABLE( CMannVsMachineStats, DT_MannVsMachineStats )
#if defined ( GAME_DLL )
	SendPropInt( SENDINFO( m_iCurrentWaveIdx ), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iServerWaveID ), 8, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( m_runningTotalWaveStats ), &REFERENCE_SEND_TABLE( DT_CMannVsMachineWaveStats ) ),
	SendPropDataTable( SENDINFO_DT( m_previousWaveStats ), &REFERENCE_SEND_TABLE( DT_CMannVsMachineWaveStats ) ),
	SendPropDataTable( SENDINFO_DT( m_currentWaveStats ), &REFERENCE_SEND_TABLE( DT_CMannVsMachineWaveStats ) ),
	SendPropInt( SENDINFO( m_iCurrencyCollectedForRespec ), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO( m_nRespecsAwardedInWave ), 8, SPROP_UNSIGNED ),
#else
	RecvPropInt( RECVINFO( m_iCurrentWaveIdx ) ),
	RecvPropInt( RECVINFO( m_iServerWaveID ) ),
	RecvPropDataTable( RECVINFO_DT( m_runningTotalWaveStats ), 0, &REFERENCE_RECV_TABLE( DT_CMannVsMachineWaveStats ) ),
	RecvPropDataTable( RECVINFO_DT( m_previousWaveStats ), 0, &REFERENCE_RECV_TABLE( DT_CMannVsMachineWaveStats ) ),
	RecvPropDataTable( RECVINFO_DT( m_currentWaveStats ), 0, &REFERENCE_RECV_TABLE( DT_CMannVsMachineWaveStats ) ),
	RecvPropInt( RECVINFO( m_iCurrencyCollectedForRespec ) ),
	RecvPropInt( RECVINFO( m_nRespecsAwardedInWave ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_mann_vs_machine_stats, CMannVsMachineStats );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineStats::CMannVsMachineStats() :
	m_currentWaveSpendingStats( DefLessFunc( uint64 ) ),
	m_previousWaveSpendingStats( DefLessFunc( uint64 ) ),
	m_totalWaveSpendingStats( DefLessFunc( uint64 ) )
{
	Assert( g_pMVMStats == NULL );
	g_pMVMStats = this;

#ifdef GAME_DLL
	SetCurrentWave( 0 );

	Q_memset( m_playerStats, 0, sizeof( m_playerStats ) );
#else
	m_iCurrentWaveIdx = 0;
#endif

	m_iCurrencyCollectedForRespec = 0;
	m_nRespecsAwardedInWave = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineStats::~CMannVsMachineStats()
{
	Assert( g_pMVMStats == this );
	g_pMVMStats = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ClearCurrentPlayerWaveSpendingStats( int nWave )
{
	if ( nWave != (int)m_iCurrentWaveIdx )
	{
		m_previousWaveSpendingStats.RemoveAll();

		FOR_EACH_MAP_FAST( m_currentWaveSpendingStats, i )
		{
			m_previousWaveSpendingStats.Insert( m_currentWaveSpendingStats.Key( i ), m_currentWaveSpendingStats.Element( i ) );

			int nIndex = m_totalWaveSpendingStats.Find( m_currentWaveSpendingStats.Key( i ) );
			if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
			{
				m_previousWaveSpendingStats[ nIndex ].nCreditsSpentOnBottles += m_currentWaveSpendingStats[i].nCreditsSpentOnBottles;
				m_previousWaveSpendingStats[ nIndex ].nCreditsSpentOnBuyBacks += m_currentWaveSpendingStats[i].nCreditsSpentOnBuyBacks;
				m_previousWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades += m_currentWaveSpendingStats[i].nCreditsSpentOnUpgrades;
			}
			else
			{
				nIndex = m_totalWaveSpendingStats.Insert( m_currentWaveSpendingStats.Key( i ) );
				m_totalWaveSpendingStats[ nIndex ] = m_currentWaveSpendingStats[i];
			}
		}
	}

	m_currentWaveSpendingStats.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetAcquiredCredits( int nWave, bool bIncludeBonus )
{
	CMannVsMachineWaveStats stats = GetWaveStats( nWave );

	if ( bIncludeBonus )
		return stats.nCreditsAcquired + stats.nCreditsBonus;

	return stats.nCreditsAcquired;;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetBonusCredits( int nWave )
{
	return GetWaveStats( nWave ).nCreditsBonus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetBottleSpending( CTFPlayer *pPlayer )
{
	int nSpending = 0;

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		int nIndex = m_currentWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_currentWaveSpendingStats.InvalidIndex() )
			nSpending += m_currentWaveSpendingStats[ nIndex ].nCreditsSpentOnBottles;

		nIndex = m_totalWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
			nSpending += m_totalWaveSpendingStats[ nIndex ].nCreditsSpentOnBottles;
	}
	else
	{
		FOR_EACH_MAP_FAST( m_currentWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_currentWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_currentWaveSpendingStats[i].nCreditsSpentOnBottles;
		}

		FOR_EACH_MAP_FAST( m_totalWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_totalWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_totalWaveSpendingStats[i].nCreditsSpentOnBottles;
		}
	}

	return nSpending;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetBuyBackSpending( CTFPlayer *pPlayer )
{
	int nSpending = 0;

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		int nIndex = m_currentWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_currentWaveSpendingStats.InvalidIndex() )
			nSpending += m_currentWaveSpendingStats[ nIndex ].nCreditsSpentOnBuyBacks;

		nIndex = m_totalWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
			nSpending += m_totalWaveSpendingStats[ nIndex ].nCreditsSpentOnBuyBacks;
	}
	else
	{
		FOR_EACH_MAP_FAST( m_currentWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_currentWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_currentWaveSpendingStats[i].nCreditsSpentOnBuyBacks;
		}

		FOR_EACH_MAP_FAST( m_totalWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_totalWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_totalWaveSpendingStats[i].nCreditsSpentOnBuyBacks;
		}
	}

	return nSpending;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetDroppedCredits( int nWave )
{
	return GetWaveStats( nWave ).nCreditsDropped;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 CMannVsMachineStats::GetMissedCredits( int nWave )
{
	CMannVsMachineWaveStats stats = GetWaveStats( nWave );
	return stats.nCreditsDropped - stats.nCreditsAcquired;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerWaveSpendingStats *CMannVsMachineStats::GetSpending( int nWave, uint64 steamId )
{
	if ( nWave == (int)m_iCurrentWaveIdx )
	{
		int nIndex = m_currentWaveSpendingStats.Find( steamId );
		if ( nIndex != m_currentWaveSpendingStats.InvalidIndex() )
		{
			return &m_currentWaveSpendingStats[nIndex];
		}
		else
		{
			nIndex = m_currentWaveSpendingStats.Insert( steamId );
			return &m_currentWaveSpendingStats[ nIndex ];
		}
	}
	else if ( nWave == (int)( m_iCurrentWaveIdx - 1 ) )
	{
		int nIndex = m_previousWaveSpendingStats.Find( steamId );
		if ( nIndex != m_previousWaveSpendingStats.InvalidIndex() )
		{
			return &m_previousWaveSpendingStats[ nIndex ];
		}
		else
		{
			nIndex = m_previousWaveSpendingStats.Insert( steamId );
			return &m_previousWaveSpendingStats[ nIndex ];
		}
	}
	else if ( nWave == -1 )
	{
		int nIndex = m_totalWaveSpendingStats.Find( steamId );
		if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
		{
			return &m_totalWaveSpendingStats[ nIndex ];
		}
		else
		{
			nIndex = m_totalWaveSpendingStats.Insert( steamId );
			return &m_totalWaveSpendingStats[ nIndex ];
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetUpgradeSpending( CTFPlayer *pPlayer )
{
	int nSpending = 0;

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		int nIndex = m_currentWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_currentWaveSpendingStats.InvalidIndex() )
			nSpending += m_currentWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades;

		nIndex = m_totalWaveSpendingStats.Find( steamID.ConvertToUint64() );
		if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
			nSpending += m_totalWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades;
	}
	else
	{
		FOR_EACH_MAP_FAST( m_currentWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_currentWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_currentWaveSpendingStats[i].nCreditsSpentOnUpgrades;
		}

		FOR_EACH_MAP_FAST( m_totalWaveSpendingStats, i )
		{
			CBasePlayer *pPlayer = GetPlayerBySteamID( CSteamID( m_totalWaveSpendingStats.Key( i ) ) );
			if ( pPlayer == NULL )
				continue;

			nSpending += m_totalWaveSpendingStats[i].nCreditsSpentOnUpgrades;
		}
	}

	return nSpending;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineWaveStats CMannVsMachineStats::GetWaveStats( int nWave )
{
	CMannVsMachineWaveStats waveStats;
	if ( nWave == (int)m_iCurrentWaveIdx )
	{
		waveStats = m_currentWaveStats;
	}
	else if ( nWave >= 0 && nWave == (int)( m_iCurrentWaveIdx - 1 ) )
	{
		waveStats = m_previousWaveStats;
	}
	else if ( nWave == -1 )
	{
		waveStats = m_runningTotalWaveStats;
		
		waveStats.nCreditsDropped += m_previousWaveStats.nCreditsDropped;
		waveStats.nCreditsAcquired += m_previousWaveStats.nCreditsAcquired;
		waveStats.nCreditsBonus += m_previousWaveStats.nCreditsBonus;
		waveStats.nPlayerDeaths += m_previousWaveStats.nPlayerDeaths;
		waveStats.nBuyBacks += m_previousWaveStats.nBuyBacks;
		waveStats.nAttempts += m_previousWaveStats.nAttempts;

		waveStats.nCreditsDropped += m_currentWaveStats.nCreditsDropped;
		waveStats.nCreditsAcquired += m_currentWaveStats.nCreditsAcquired;
		waveStats.nCreditsBonus += m_currentWaveStats.nCreditsBonus;
		waveStats.nPlayerDeaths += m_currentWaveStats.nPlayerDeaths;
		waveStats.nBuyBacks += m_currentWaveStats.nBuyBacks;
		waveStats.nAttempts += m_currentWaveStats.nAttempts;
	}

	return waveStats;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::OnStatsChanged( void )
{
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetPlayerEvents( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsBot() )
		return;

#ifdef GAME_DLL
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMResetPlayerStats" );
		WRITE_BYTE( pPlayer->entindex() );
	MessageEnd();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetStats( void )
{
#ifdef GAME_DLL
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMStatsReset" );
	MessageEnd();

	SetCurrentWave( 0 );

	m_runningTotalWaveStats.nCreditsDropped = 0;
	m_runningTotalWaveStats.nCreditsAcquired = 0;
	m_runningTotalWaveStats.nCreditsBonus = 0;
	m_runningTotalWaveStats.nPlayerDeaths = 0;
	m_runningTotalWaveStats.nBuyBacks = 0;
	m_runningTotalWaveStats.nAttempts = 0;

	m_previousWaveStats.nCreditsDropped = 0;
	m_previousWaveStats.nCreditsAcquired = 0;
	m_previousWaveStats.nCreditsBonus = 0;
	m_previousWaveStats.nPlayerDeaths = 0;
	m_previousWaveStats.nBuyBacks = 0;
	m_previousWaveStats.nAttempts = 0;

	m_currentWaveStats.nCreditsDropped = 0;
	m_currentWaveStats.nCreditsAcquired = 0;
	m_currentWaveStats.nCreditsBonus = 0;
	m_currentWaveStats.nPlayerDeaths = 0;
	m_currentWaveStats.nBuyBacks = 0;
	m_currentWaveStats.nAttempts = 0;
#else
	m_LocalPlayerUpgrades.Purge();
#endif

	m_currentWaveSpendingStats.RemoveAll();
	m_previousWaveSpendingStats.RemoveAll();
	m_totalWaveSpendingStats.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetUpgradeSpending( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsBot() )
		return;

	CSteamID steamId;
	if ( pPlayer->GetSteamID( &steamId ) )
	{
		uint64 ullSteamID = steamId.ConvertToUint64();

		int nIndex = m_currentWaveSpendingStats.Find( ullSteamID );
		if ( nIndex != m_currentWaveSpendingStats.InvalidIndex() )
			m_currentWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades = 0;

		nIndex = m_previousWaveSpendingStats.Find( ullSteamID );
		if ( nIndex != m_previousWaveSpendingStats.InvalidIndex() )
			m_previousWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades = 0;

		nIndex = m_totalWaveSpendingStats.Find( ullSteamID );
		if ( nIndex != m_totalWaveSpendingStats.InvalidIndex() )
			m_totalWaveSpendingStats[ nIndex ].nCreditsSpentOnUpgrades = 0;
	}

#ifdef GAME_DLL
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMResetPlayerUpgradeSpending" );
		WRITE_BYTE( pPlayer->entindex() );
	MessageEnd();
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::NotifyPlayerActiveUpgradeCosts( CTFPlayer *pPlayer, int nCosts )
{
	NotifyTargetPlayerEvent( pPlayer, 0, MVMEvent_ActiveUpgrades, nCosts );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::NotifyPlayerEvent( CTFPlayer *pPlayer, uint32 nWave, eMannVsMachineEvent eEvent, int, int )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::NotifyTargetPlayerEvent( CTFPlayer *pPlayer, uint32 nWave, eMannVsMachineEvent eEvent, int nCost )
{
	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMLocalPlayerWaveSpendingValue" );
		WRITE_BITS( (byte *)pPlayer->GetSteamIDAsUInt64(), 64 );
		WRITE_BYTE( nWave );
		WRITE_BYTE( eEvent );
		WRITE_WORD( nCost );
	MessageEnd();

	CSteamID steamID;
	if ( !pPlayer->GetSteamID( &steamID ) )
		return;

	CPlayerWaveSpendingStats *pStats = GetSpending( nWave, steamID.ConvertToUint64() );
	if ( pStats )
	{
		switch ( eEvent )
		{
			case MVMEvent_BoughtInstantRespawn:
			{
				pStats->nCreditsSpentOnBuyBacks += nCost;
				break;
			}
			case MVMEvent_BoughtBottle:
			{
				pStats->nCreditsSpentOnBottles += nCost;
				break;
			}
			case MVMEvent_BoughtUpgrade:
			{
				pStats->nCreditsSpentOnUpgrades += nCost;
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_BoughtInstantRespawn( CTFPlayer *pPlayer, int nCost )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	m_currentWaveStats.nBuyBacks++;
	OnStatsChanged();

	NotifyTargetPlayerEvent( pPlayer, m_iCurrentWaveIdx, MVMEvent_BoughtInstantRespawn, nCost );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealthDakageToBots( CTFPlayer *pPlayer, int nDamage )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	m_playerStats[ pPlayer->entindex() ].nBotDamage += nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealtDamageToGiants( CTFPlayer *pPlayer, int nDamage )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	m_playerStats[ pPlayer->entindex() ].nGiantDamage += nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_DealtDamageToTanks( CTFPlayer *pPlayer, int nDamage )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	m_playerStats[ pPlayer->entindex() ].nTankDamage += nDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_Died( CTFPlayer *pPlayer )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	m_playerStats[ pPlayer->entindex() ].nDeaths += 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_PickedUpCredits( CTFPlayer *pPlayer, uint32 nWave, int nAmount )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	OnStatsChanged();

	NotifyPlayerEvent( pPlayer, nWave, MVMEvent_PickedUpCredits, nAmount, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_PointsChanged( CTFPlayer *pPlayer, int nPoints )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::PlayerEvent_Upgraded( CTFPlayer *pPlayer, uint16 i1, uint16 i2, uint8 i3, int16 nCost, bool bIsBottle )
{
	if ( pPlayer->IsBot() )
		return;

	if ( TFGameRules() && !TFGameRules()->IsPVEModeActive() )
		return;

	OnStatsChanged();

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMPlayerUpgradedEvent" );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_BYTE( m_iCurrentWaveIdx );
		WRITE_WORD( i1 );
		WRITE_WORD( i2 );
		WRITE_BYTE( i3 );
		WRITE_WORD( nCost );
	MessageEnd();

	if ( bIsBottle )
		NotifyTargetPlayerEvent( pPlayer, m_iCurrentWaveIdx, MVMEvent_BoughtBottle, nCost );
	else
		NotifyTargetPlayerEvent( pPlayer, m_iCurrentWaveIdx, MVMEvent_BoughtUpgrade, nCost );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ResetWaveStats( void )
{
	m_currentWaveStats.nCreditsDropped = 0;
	m_currentWaveStats.nCreditsAcquired = 0;
	m_currentWaveStats.nCreditsBonus = 0;
	m_currentWaveStats.nPlayerDeaths = 0;
	m_currentWaveStats.nBuyBacks = 0;
	m_currentWaveStats.nAttempts = 0;
	m_nRespecsAwardedInWave = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_AcquiredCredits( uint32 nWave, int nAmount, bool bBonus )
{
	if ( nWave == m_iCurrentWaveIdx )
	{
		if ( bBonus )
			m_currentWaveStats.nCreditsBonus += nAmount;
		else
			m_currentWaveStats.nCreditsAcquired += nAmount;
	}
	else if ( nWave == ( m_iCurrentWaveIdx - 1 ) )
	{
		if ( bBonus )
			m_previousWaveStats.nCreditsBonus += nAmount;
		else
			m_previousWaveStats.nCreditsAcquired += nAmount;
	}

	OnStatsChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_CreditsDropped( uint32 nWave, int nAmount )
{
	if ( nWave == m_iCurrentWaveIdx )
		m_currentWaveStats.nCreditsDropped += nAmount;
	else if ( nWave == ( m_iCurrentWaveIdx - 1 ) )
		m_previousWaveStats.nCreditsDropped += nAmount;

	OnStatsChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_WaveEnd( bool bSuccess )
{
	ConVarRef sv_cheats( "sv_cheats" );
	if ( !bSuccess && ( !sv_cheats.IsValid() || !sv_cheats.GetBool() ) )
	{
		m_currentWaveStats.nAttempts++;

		SW_ReportWaveSummary( m_iCurrentWaveIdx, false );

		ResetWaveStats();
		OnStatsChanged();

		CReliableBroadcastRecipientFilter filter;
		UserMessageBegin( filter, "MVMWaveFailed" );
		MessageEnd();
	}

	CUtlVector<CTFPlayer *> humans;
	CollectPlayers( &humans, TF_TEAM_MVM_PLAYERS );
	FOR_EACH_VEC( humans, i )
	{
		CTFPlayer *pPlayer = humans[i];
		CMannVsMachinePlayerStats &stats = m_playerStats[ pPlayer->entindex() ];

		CSingleUserReliableRecipientFilter filter( pPlayer );
		UserMessageBegin( filter, "MVMWaveChange" );
			WRITE_WORD( m_iServerWaveID );
			WRITE_BYTE( stats.nDeaths );
			WRITE_LONG( stats.nBotDamage );
			WRITE_LONG( stats.nGiantDamage );
			WRITE_LONG( stats.nTankDamage );
		MessageEnd();
	}

	m_iServerWaveID++;

	Q_memset( m_playerStats, 0, sizeof( m_playerStats ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundEvent_WaveStart( void )
{
	if ( m_iCurrentWaveIdx > 0 && m_currentWaveStats.nAttempts == 0 )
		SW_ReportWaveSummary( m_iCurrentWaveIdx - 1, true );

	ResetWaveStats();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::RoundOver( bool bSuccess )
{
	m_iServerWaveID++;

	SW_ReportWaveSummary( m_iCurrentWaveIdx, bSuccess );

	m_iServerWaveID--;
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportWaveSummary( int nWave, bool bSuccess )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SendUpgradesToPlayer( CTFPlayer *pPlayer, CUtlVector<CUpgradeInfo> const *pUpgrades )
{
	CSingleUserRecipientFilter filter( pPlayer );
	if ( pUpgrades == NULL )
	{
		UserMessageBegin( filter, "MVMLocalPlayerUpgradesClear" );
			WRITE_BYTE( 0 );
		MessageEnd();

		return;
	}

	UserMessageBegin( filter, "MVMLocalPlayerUpgradesClear" );
		WRITE_BYTE( 0 );
	MessageEnd();

	FOR_EACH_VEC( *pUpgrades, i )
	{
		CUpgradeInfo const &info = pUpgrades->Element( i );

		UserMessageBegin( filter, "MVMLocalPlayerUpgradesValue" );
			WRITE_BYTE( info.m_iPlayerClass );
			WRITE_WORD( info.m_nItemDefIndex );
			WRITE_BYTE( info.m_iUpgrade );
			WRITE_WORD( info.m_nCost );
		MessageEnd();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SetCurrentWave( uint32 nWave )
{
	if ( nWave > m_iCurrentWaveIdx )
	{
		m_runningTotalWaveStats.nCreditsDropped += m_previousWaveStats.nCreditsDropped;
		m_runningTotalWaveStats.nCreditsAcquired += m_previousWaveStats.nCreditsAcquired;
		m_runningTotalWaveStats.nCreditsBonus += m_previousWaveStats.nCreditsBonus;
		m_runningTotalWaveStats.nPlayerDeaths += m_previousWaveStats.nPlayerDeaths;
		m_runningTotalWaveStats.nBuyBacks += m_previousWaveStats.nBuyBacks;
		m_runningTotalWaveStats.nAttempts += m_previousWaveStats.nAttempts;

		m_previousWaveStats = m_currentWaveStats;
	}

	if ( nWave == 0 )
	{
		m_runningTotalWaveStats.nCreditsDropped = 0;
		m_runningTotalWaveStats.nCreditsAcquired = 0;
		m_runningTotalWaveStats.nCreditsBonus = 0;
		m_runningTotalWaveStats.nPlayerDeaths = 0;
		m_runningTotalWaveStats.nBuyBacks = 0;
		m_runningTotalWaveStats.nAttempts = 0;

		m_previousWaveStats.nCreditsDropped = 0;
		m_previousWaveStats.nCreditsAcquired = 0;
		m_previousWaveStats.nCreditsBonus = 0;
		m_previousWaveStats.nPlayerDeaths = 0;
		m_previousWaveStats.nBuyBacks = 0;
		m_previousWaveStats.nAttempts = 0;

		m_currentWaveStats.nCreditsDropped = 0;
		m_currentWaveStats.nCreditsAcquired = 0;
		m_currentWaveStats.nCreditsBonus = 0;
		m_currentWaveStats.nPlayerDeaths = 0;
		m_currentWaveStats.nBuyBacks = 0;
		m_currentWaveStats.nAttempts = 0;
	}

	m_iCurrentWaveIdx = nWave;

	CReliableBroadcastRecipientFilter filter;
	UserMessageBegin( filter, "MVMResetPlayerWaveSpendingStats" );
		WRITE_BYTE( m_iCurrentWaveIdx );
	MessageEnd();

	ResetWaveStats();

	OnStatsChanged();
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::OnDataChanged( DataUpdateType_t updateType )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::AddLocalPlayerUpgrade( int iClass, item_def_index_t iItemIndex )
{
	m_LocalPlayerUpgrades.AddToTail( {iClass, iItemIndex, 0, 0} );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::ClearLocalPlayerUpgrades( void )
{
	m_LocalPlayerUpgrades.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerBottleSpending( int nWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( nWave );
	return pStats ? pStats->nCreditsSpentOnBottles : 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerBuyBackSpending( int nWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( nWave );
	return pStats ? pStats->nCreditsSpentOnBuyBacks : 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetLocalPlayerUpgradeSpending( int nWave )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer == NULL )
		return 0;

	CPlayerWaveSpendingStats *pStats = GetLocalSpending( nWave );
	return pStats ? pStats->nCreditsSpentOnUpgrades : 0;
}

CPlayerWaveSpendingStats *CMannVsMachineStats::GetLocalSpending( int nWave )
{
	CSteamID steamId;
	if ( C_BasePlayer::GetLocalPlayer()->GetSteamID( &steamId ) )
	{
		return GetSpending( nWave, steamId.ConvertToUint64() );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMannVsMachineStats::GetPlayerActiveUpgradeCosts( uint64 steamId )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SetPlayerActiveUpgradeCosts( uint64 steamId, int nSpending )
{
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientBuyBackPurchase( uint8 nWave, uint16 nCost )
{
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientUpgradePurchase( uint8 nWave, uint16 i1, uint16 i2, uint8 i3, int16 nCost )
{
}

//-----------------------------------------------------------------------------
// Purpose: Unused
//-----------------------------------------------------------------------------
void CMannVsMachineStats::SW_ReportClientWaveSummary( uint16 nServerWaveID, CMannVsMachinePlayerStats stats )
{
}

#endif


int MannVsMachineStats_GetAcquiredCredits( int nWave, bool b1 )
{
	return g_pMVMStats ? g_pMVMStats->GetAcquiredCredits( nWave, b1 ) : 0;
}

int MannVsMachineStats_GetCurrentWave( void )
{
	return g_pMVMStats ? g_pMVMStats->GetCurrentWave() : 0;
}

int MannVsMachineStats_GetDroppedCredits( int nWave )
{
	return g_pMVMStats ? g_pMVMStats->GetDroppedCredits( nWave ) : 0;
}

CMannVsMachineStats *MannVsMachineStats_GetInstance( void )
{
	return g_pMVMStats;
}

int MannVsMachineStats_GetMissedCredits( int nWave )
{
	return g_pMVMStats ? g_pMVMStats->GetMissedCredits( nWave ) : 0;
}

void MannVsMachineStats_Init( void )
{
#ifdef GAME_DLL
	CBaseEntity::Create( "tf_mann_vs_machine_stats", vec3_origin, vec3_angle );
#else
	HOOK_MESSAGE( MVMPlayerEvent );
	HOOK_MESSAGE( MVMResetPlayerStats );
	HOOK_MESSAGE( MVMStatsReset );
	HOOK_MESSAGE( MVMPlayerUpgradedEvent );
	HOOK_MESSAGE( MVMLocalPlayerUpgradesClear );
	HOOK_MESSAGE( MVMLocalPlayerUpgradesValue );
	HOOK_MESSAGE( MVMResetPlayerWaveSpendingStats );
	HOOK_MESSAGE( MVMLocalPlayerWaveSpendingValue );
	HOOK_MESSAGE( MVMWaveChange );
	HOOK_MESSAGE( MVMResetPlayerUpgradeSpending );
#endif
}

#ifdef GAME_DLL
void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn( CTFPlayer *pPlayer, int nCost )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_BoughtInstantRespawn( pPlayer, nCost );
	}
}

void MannVsMachineStats_PlayerEvent_Died( CTFPlayer *pPlayer )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_Died( pPlayer );
	}
}

void MannVsMachineStats_PlayerEvent_PickedUpCredits( CTFPlayer *pPlayer, uint32 nWave, int nAmount )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_PickedUpCredits( pPlayer, nWave, nAmount );
	}
}

void MannVsMachineStats_PlayerEvent_PointsChanged( CTFPlayer *pPlayer, int nPoints )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_PointsChanged( pPlayer, nPoints );
	}
}

void MannVsMachineStats_PlayerEvent_Upgraded( CTFPlayer *pPlayer, uint16 i1, uint16 i2, uint16 i3, int16 i4, bool b1 )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->PlayerEvent_Upgraded( pPlayer, i1, i2, i3, i4, b1 );
	}
}

void MannVsMachineStats_RoundEvent_CreditsDropped( uint32 nWave, int nAmount )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->RoundEvent_CreditsDropped( nWave, nAmount );
	}
}

void MannVsMachineStats_SetPopulationFile( char const *pszPopfile )
{
	if ( g_pMVMStats )
	{
		g_pMVMStats->SetPopFile( pszPopfile );
	}
}

#endif
