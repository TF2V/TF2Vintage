//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_POPULATORS_H
#define TF_POPULATORS_H

#include "tf_populator_spawners.h"
#include "tf_bot.h"

class CPopulationManager;
class CWave;


enum RelativePositionType
{
	UNDEFINED = 0,
	AHEAD,
	BEHIND,
	ANYWHERE
};

struct EventInfo
{
	CFmtStr m_target;
	CFmtStr m_action;
};

enum SpawnLocationResult
{
	SPAWN_LOCATION_NOT_FOUND = 0,
	SPAWN_LOCATION_NORMAL,
	SPAWN_LOCATION_TELEPORTER
};

//-----------------------------------------------------------------------------
// Interface for all populators
class IPopulator
{
public:
	virtual ~IPopulator() { }

	virtual bool Parse( KeyValues *data ) = 0;

	virtual void PostInitialize( void ) { }
	virtual void Update( void ) { }
	virtual void UnpauseSpawning() {}

	virtual void OnPlayerKilled( CTFPlayer *pPlayer ) { }

	virtual bool HasEventChangeAttributes( const char *pszEventName ) const
	{
		if ( GetSpawner() )
		{
			return GetSpawner()->HasEventChangeAttributes( pszEventName );
		}

		return false;
	}

	virtual IPopulationSpawner *GetSpawner( void ) const = 0;
	virtual CPopulationManager *GetManager( void ) const = 0;
};


class CSpawnLocation
{
public:
	CSpawnLocation();

	bool Parse( KeyValues *data );

	SpawnLocationResult FindSpawnLocation( Vector *vSpawnPosition );
	CTFNavArea *SelectSpawnArea( void ) const;

private:
	RelativePositionType m_eRelative;
	CUtlVector< CHandle<CTFTeamSpawn> > m_TeamSpawns;

	int m_nSpawnCount;
	int m_nRandomSeed;
	bool m_bClosestPointOnNav;
};

class CPopulatorInternalSpawnPoint : public CPointEntity
{
	DECLARE_CLASS( CPopulatorInternalSpawnPoint, CPointEntity );
};
extern CHandle< CPopulatorInternalSpawnPoint > g_internalSpawnPoint;


class CMissionPopulator : public IPopulator
{
public:
	CMissionPopulator( CPopulationManager *manager );
	virtual ~CMissionPopulator();

	virtual bool	Parse( KeyValues *data );

	virtual void	Update( void );
	virtual void	UnpauseSpawning( void );

	virtual IPopulationSpawner *GetSpawner( void ) const { return m_pSpawner; }
	virtual CPopulationManager *GetManager( void ) const { return m_pManager; }

	bool			UpdateMission( CTFBot::MissionType mission );
	bool			UpdateMissionDestroySentries( void );

	enum StateType
	{
		NOT_STARTED,
		INITIAL_COOLDOWN,
		RUNNING
	};

	CTFBot::MissionType m_eMission;
	CSpawnLocation m_where;
	StateType m_eState;
	float m_flInitialCooldown;
	float m_flCooldownDuration;
	CountdownTimer m_cooldownTimer;
	CountdownTimer m_checkSentriesTimer;
	int m_nDesiredCount;
	int m_nStartWave;
	int m_nEndWave;

private:
	IPopulationSpawner *m_pSpawner;
	CPopulationManager *m_pManager;
};


class CWaveSpawnPopulator : public IPopulator
{
	static int sm_reservedPlayerSlotCount;
public:
	CWaveSpawnPopulator( CPopulationManager *pManager );
	virtual ~CWaveSpawnPopulator();

	virtual bool	Parse( KeyValues *data );

	virtual void	Update( void );

	virtual void	OnPlayerKilled( CTFPlayer *pPlayer );

	virtual IPopulationSpawner *GetSpawner( void ) const { return m_pSpawner; }
	virtual CPopulationManager *GetManager( void ) const { return m_pManager; }

	void			ForceFinish( void );
	int				GetCurrencyAmountPerDeath( void );
	bool			IsFinishedSpawning( void );

	void			OnNonSupportWavesDone( void );

	enum InternalStateType
	{
		PENDING,
		PRE_SPAWN_DELAY,
		SPAWNING,
		WAIT_FOR_ALL_DEAD,
		DONE
	};
	void			SetState( InternalStateType eState );

	CSpawnLocation m_where;
	int m_iTotalCount;
	int m_iRemainingCount;
	int m_nClassCounts;
	int m_iMaxActive;
	int m_nSpawnCount;
	float m_flWaitBeforeStarting;
	float m_flWaitBetweenSpawns;
	bool m_bWaitBetweenSpawnsAfterDeath;
	CFmtStr m_startWaveWarningSound;
	EventInfo *m_startWaveEvent;
	CFmtStr m_firstSpawnWarningSound;
	EventInfo *m_firstSpawnEvent;
	CFmtStr m_lastSpawnWarningSound;
	EventInfo *m_lastSpawnEvent;
	CFmtStr m_doneWarningSound;
	EventInfo *m_doneEvent;
	int m_iTotalCurrency;
	int m_iRemainingCurrency;
	CUtlString m_name;
	CUtlString m_szWaitForAllSpawned;
	CUtlString m_szWaitForAllDead;
	CountdownTimer m_timer;
	CUtlVector<EHANDLE> m_activeSpawns;
	int m_nNumSpawnedSoFar;
	int m_nReservedPlayerSlots;
	bool m_bSupportWave;
	bool m_bLimitedSupport;
	CWave *m_parentWave;
	InternalStateType m_eState;
	bool m_bRandomSpawn;
	SpawnLocationResult m_spawnLocationResult;
	Vector m_vecSpawnPosition;

private:
	IPopulationSpawner *m_pSpawner;
	CPopulationManager *m_pManager;
};


class CPeriodicSpawnPopulator : public IPopulator
{
public:
	CPeriodicSpawnPopulator( CPopulationManager *manager );
	virtual ~CPeriodicSpawnPopulator();

	virtual bool Parse( KeyValues *data );

	virtual void PostInitialize( void );
	virtual void Update( void );
	virtual void UnpauseSpawning( void );

	virtual IPopulationSpawner *GetSpawner( void ) const { return m_pSpawner; }
	virtual CPopulationManager *GetManager( void ) const { return m_pManager; }

	CSpawnLocation m_where;
	float m_flMinInterval;
	float m_flMaxInterval;
	CountdownTimer m_timer;

private:
	IPopulationSpawner *m_pSpawner;
	CPopulationManager *m_pManager;
};


class CRandomPlacementPopulator : public IPopulator
{
public:
	CRandomPlacementPopulator( CPopulationManager *manager );
	virtual ~CRandomPlacementPopulator();

	virtual bool Parse( KeyValues *data );

	virtual void PostInitialize( void );

	virtual IPopulationSpawner *GetSpawner( void ) const { return m_pSpawner; }
	virtual CPopulationManager *GetManager( void ) const { return m_pManager; }

	int m_iCount;
	float m_flMinSeparation;
	unsigned int m_nNavAreaFilter;

private:
	IPopulationSpawner *m_pSpawner;
	CPopulationManager *m_pManager;
};


struct WaveClassCount_t
{
	int nClassCount;
	string_t iszClassIconName;
	unsigned int iFlags;
};

class CWave : public IPopulator
{
public:
	CWave( CPopulationManager *pManager );
	virtual ~CWave();

	virtual bool Parse( KeyValues *data );
	virtual void Update( void );

	virtual void OnPlayerKilled( CTFPlayer *pPlayer );

	virtual bool HasEventChangeAttributes( const char *pszEventName ) const;

	virtual IPopulationSpawner *GetSpawner( void ) const { return m_pSpawner; }
	virtual CPopulationManager *GetManager( void ) const { return m_pManager; }
	
	void AddClassType( string_t iszClassIconName, int nCount, unsigned int iFlags );
	CWaveSpawnPopulator *FindWaveSpawnPopulator( const char *name );
	void ForceFinish();
	void ForceReset();
	int GetClassCount( int i ) const { return m_WaveClassCounts[i].nClassCount; }
	unsigned int GetClassFlags( int i ) const { return m_WaveClassCounts[i].iFlags; }
	string_t GetClassIconName( int i ) const { return m_WaveClassCounts[i].iszClassIconName; }
	int GetNumClassTypes( void ) const { return m_WaveClassCounts.Count(); }
	int GetTotalCurrency( void ) const { return m_iTotalCurrency; }
	bool IsDoneWithNonSupportWaves( void );

	int m_nTotalEnemyCount;
	int m_nNumTanksSpawned;
	int m_nNumSentryBustersSpawned;
	int m_nNumEngineersTeleportSpawned;
	int m_nNumSentryBustersKilled;

	CountdownTimer m_upgradeAlertTimer;

	float m_flStartTime;

private:
	void ActiveWaveUpdate( void );
	void WaveCompleteUpdate( void );
	void WaveIntermissionUpdate( void );

	IPopulationSpawner *m_pSpawner;
	CPopulationManager *m_pManager;

	CUtlVector<CWaveSpawnPopulator *> m_WaveSpawns;

	bool m_bStarted;
	bool m_bFiredInitWaveOutput;

	CUtlVector<WaveClassCount_t> m_WaveClassCounts;
	int	m_iTotalCurrency;

	EventInfo *m_startWaveEvent;
	EventInfo *m_doneEvent;
	EventInfo *m_initWaveEvent;

	CFmtStr m_description;
	CFmtStr m_soundName;

	float m_flWaitWhenDone;
	CountdownTimer m_doneTimer;

	bool m_bCheckBonusCreditsMin;
	bool m_bCheckBonusCreditsMax;
	float m_flBonusCreditsTime;

	bool m_bPlayedUpgradeAlert;
	bool m_bEveryWaveSpawnDone;
};

#endif