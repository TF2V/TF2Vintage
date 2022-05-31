//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_MANN_VS_MACHINE_STATS_H
#define TF_MANN_VS_MACHINE_STATS_H

#include "tf_player_shared.h"

#if defined(CLIENT_DLL)
#define CTFPlayer C_TFPlayer
#define CMannVsMachineStats C_MannVsMachineStats
#define CMannVsMachineWaveStats C_MannVsMachineWaveStats
#endif

enum eMannVsMachineEvent
{
	// 0 .. 1 ??
	MVMEvent_PickedUpCredits = 2,
	MVMEvent_BoughtInstantRespawn,
	MVMEvent_BoughtBottle,
	MVMEvent_BoughtUpgrade,
	MVMEvent_ActiveUpgrades
};

struct CMannVsMachineWaveStats
{
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CMannVsMachineWaveStats );

	CNetworkVar( uint32, nCreditsDropped );
	CNetworkVar( uint32, nCreditsAcquired );
	CNetworkVar( uint32, nCreditsBonus );
	CNetworkVar( uint32, nPlayerDeaths );
	CNetworkVar( uint32, nBuyBacks );
	CNetworkVar( uint32, nAttempts );
};

struct CMannVsMachinePlayerStats
{
	uint32 nDeaths;
	uint32 nBotDamage;
	uint32 nGiantDamage;
	uint32 nTankDamage;
};

struct CPlayerWaveSpendingStats
{
	uint32 nCreditsSpentOnBuyBacks;
	uint32 nCreditsSpentOnBottles;
	uint32 nCreditsSpentOnUpgrades;
};

int MannVsMachineStats_GetAcquiredCredits( int nWave = -1, bool = true );
int MannVsMachineStats_GetCurrentWave( void );
int MannVsMachineStats_GetDroppedCredits( int nWave = -1 );
int MannVsMachineStats_GetMissedCredits( int nWave = -1 );
void MannVsMachineStats_Init( void );

#if defined(GAME_DLL)
void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn( CTFPlayer *pPlayer, int nCost );
void MannVsMachineStats_PlayerEvent_Died( CTFPlayer *pPlayer );
void MannVsMachineStats_PlayerEvent_PickedUpCredits( CTFPlayer *pPlayer, uint32 nWave, int nAmount );
void MannVsMachineStats_PlayerEvent_PointsChanged( CTFPlayer *pPlayer, int nPoints );
void MannVsMachineStats_PlayerEvent_Upgraded( CTFPlayer *pPlayer, uint16, uint16, uint16, int16, bool );
void MannVsMachineStats_RoundEvent_CreditsDropped( uint32 nWave, int nAmount );
void MannVsMachineStats_SetPopulationFile( char const *pszPopfile );
#endif


// This is an entity... why?

class CMannVsMachineStats : public CBaseEntity
{
	DECLARE_CLASS( CMannVsMachineStats, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();

	CMannVsMachineStats();
	virtual ~CMannVsMachineStats();

	virtual	int ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	void ClearCurrentPlayerWaveSpendingStats( int nWave );
	uint32 GetAcquiredCredits( int nWave, bool = true );
	uint32 GetBonusCredits( int nWave );
	int GetBottleSpending( CTFPlayer *pPlayer = NULL );
	int GetBuyBackSpending( CTFPlayer *pPlayer = NULL );
	uint32 GetDroppedCredits( int nWave );
	uint32 GetMissedCredits( int nWave );
	CPlayerWaveSpendingStats *GetSpending( int nWave, uint64 steamId );
	int GetUpgradeSpending( CTFPlayer *pPlayer = NULL );
	CMannVsMachineWaveStats GetWaveStats( int nWave );
	void OnStatsChanged( void );
	void ResetPlayerEvents( CTFPlayer *pPlayer );
	void ResetStats( void );
	void ResetUpgradeSpending( CTFPlayer *pPlayer );

	int GetCurrentWave( void ) const { return m_iCurrentWaveIdx; }
	uint32 GetAcquiredCreditsForRespec( void ) { return m_iCurrencyCollectedForRespec; }
	uint16 GetNumRespecsEarnedInWave( void ) { return m_nRespecsAwardedInWave; }

#if defined(GAME_DLL)
	virtual int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }

	void NotifyPlayerActiveUpgradeCosts( CTFPlayer *pPlayer, int nCosts );
	void NotifyPlayerEvent( CTFPlayer *pPlayer, uint32 nWave, eMannVsMachineEvent eEvent, int, int );
	void NotifyTargetPlayerEvent( CTFPlayer *pPlayer, uint32 nWave, eMannVsMachineEvent eEvent, int nCost );
	void PlayerEvent_BoughtInstantRespawn( CTFPlayer *pPlayer, int nCost );
	void PlayerEvent_DealthDakageToBots( CTFPlayer *pPlayer, int nDamage );
	void PlayerEvent_DealtDamageToGiants( CTFPlayer *pPlayer, int nDamage );
	void PlayerEvent_DealtDamageToTanks( CTFPlayer *pPlayer, int nDamage );
	void PlayerEvent_Died( CTFPlayer *pPlayer );
	void PlayerEvent_PickedUpCredits( CTFPlayer *pPlayer, uint32 nWave, int nAmount );
	void PlayerEvent_PointsChanged( CTFPlayer *pPlayer, int nPoints );
	void PlayerEvent_Upgraded( CTFPlayer *pPlayer, uint16, uint16, uint8, int16 nCost, bool bIsBottle );
	void ResetWaveStats( void );
	void RoundEvent_AcquiredCredits( uint32 nWave, int nAmount, bool bBonus = true );
	void RoundEvent_CreditsDropped( uint32 nWave, int nAmount );
	void RoundEvent_WaveEnd( bool bSuccess );
	void RoundEvent_WaveStart( void );
	void RoundOver( bool bSuccess );
	void SendUpgradesToPlayer( CTFPlayer *pPlayer, CUtlVector<CUpgradeInfo> const *upgrades );

	void SetCurrentWave( uint32 nWave );
	void SetAcquiredCreditsForRespec( uint32 nCredits ) { m_iCurrencyCollectedForRespec = nCredits; }
	void SetNumRespecsEarnedInWave( uint16 nRespecs ) { m_nRespecsAwardedInWave = nRespecs; }
	void SetPopFile( const char *pszPopfile ) { V_strcpy_safe( m_szPopFile, pszPopfile ); }

	void SW_ReportWaveSummary( int nWave, bool bSuccess );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );

	void AddLocalPlayerUpgrade( int iClass, uint16 iItemIndex );
	void ClearLocalPlayerUpgrades( void );
	int GetLocalPlayerBottleSpending( int nWave );
	int GetLocalPlayerBuyBackSpending( int nWave );
	int GetLocalPlayerUpgradeSpending( int nWave );
	CPlayerWaveSpendingStats *GetLocalSpending( int nWave );
	int GetPlayerActiveUpgradeCosts( uint64 steamId );
	void SetPlayerActiveUpgradeCosts( uint64 steamId, int nCosts );

	void SW_ReportClientBuyBackPurchase( uint8 nWave, uint16 nCost );
	void SW_ReportClientUpgradePurchase( uint8 nWave, uint16 i1, uint16 i2, uint8 i3, int16 nCost );
	void SW_ReportClientWaveSummary( uint16 nServerWaveID, CMannVsMachinePlayerStats stats );
#endif

private:
#if defined(GAME_DLL)
	CMannVsMachinePlayerStats m_playerStats[MAX_PLAYERS + 1];

	char m_szPopFile[MAX_PATH];
#endif

	CMannVsMachineWaveStats m_runningTotalWaveStats;
	CMannVsMachineWaveStats m_previousWaveStats;
	CMannVsMachineWaveStats m_currentWaveStats;
	CNetworkVar( uint32, m_iCurrentWaveIdx );
	CNetworkVar( uint32, m_iServerWaveID );
	CNetworkVar( uint32, m_iCurrencyCollectedForRespec );
	CNetworkVar( uint16, m_nRespecsAwardedInWave );

	CUtlMap<uint64, CPlayerWaveSpendingStats> m_currentWaveSpendingStats;
	CUtlMap<uint64, CPlayerWaveSpendingStats> m_previousWaveSpendingStats;
	CUtlMap<uint64, CPlayerWaveSpendingStats> m_totalWaveSpendingStats;

#if defined(CLIENT_DLL)
	CUtlVector<CUpgradeInfo> m_LocalPlayerUpgrades;
#endif
};

CMannVsMachineStats *MannVsMachineStats_GetInstance( void );
#endif