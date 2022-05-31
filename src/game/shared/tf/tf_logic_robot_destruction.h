//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_LOGIC_ROBOT_DESTRUCTION_H
#define TF_LOGIC_ROBOT_DESTRUCTION_H

#ifdef GAME_DLL
#include "triggers.h"
#include "GameEventListener.h"
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

#include "tf_shareddefs.h"

#if defined(CLIENT_DLL)
#define CTFRobotDestruction_RobotSpawn	C_TFRobotDestruction_RobotSpawn
#define CTFRobotDestruction_RobotGroup	C_TFRobotDestruction_RobotGroup
#define CTFRobotDestructionLogic		C_TFRobotDestructionLogic
#endif

class CTFRobotDestruction_Robot;
class CTFRobotDestruction_RobotGroup;
class CTFRobotDestructionLogic;

typedef struct RobotData_s
{
	RobotData_s( char const *pszModelName, char const *pszDamagedModelName, char const *pszHurtSound, char const *pszDeathSound, char const *pszCollideSound, char const *pszIdleSound, float flHealthBarOffset )
		: m_pszModelName(pszModelName), m_pszDamagedModelName(pszDamagedModelName), m_pszHurtSound(pszHurtSound), 
		m_pszDeathSound(pszDeathSound), m_pszCollideSound(pszCollideSound), m_pszIdleSound(pszIdleSound), m_flHealthBarOffset(flHealthBarOffset) {}
	void Precache( void );

	char const *m_pszModelName;
	char const *m_pszDamagedModelName;
	char const *m_pszHurtSound;
	char const *m_pszDeathSound;
	char const *m_pszCollideSound;
	char const *m_pszIdleSound;
	float m_flHealthBarOffset;
} RobotData_t;

enum ERobotType
{
	ROBOT_TYPE_SMALL,
	ROBOT_TYPE_MEDIUM,
	ROBOT_TYPE_LARGE,

	NUM_ROBOT_TYPES,
};
extern RobotData_t *g_RobotData[NUM_ROBOT_TYPES];

enum ERobotState
{
	ROBOT_STATE_INACTIVE = 0,
	ROBOT_STATE_ACTIVE,
	ROBOT_STATE_DEAD,
	ROBOT_STATE_SHIELDED,

	NUM_ROBOT_STATES
};

#if defined(GAME_DLL)
typedef struct RobotSpawnData_s
{
	RobotSpawnData_s &operator=( const RobotSpawnData_s &rhs )
	{
		m_eType = rhs.m_eType;
		m_nRobotHealth = rhs.m_nRobotHealth;
		m_nPoints = rhs.m_nPoints;
		m_nNumGibs = rhs.m_nNumGibs;
		m_pszPathName = rhs.m_pszPathName;
		m_pszGroupName = rhs.m_pszGroupName;

		return *this;
	}

	ERobotType m_eType;
	int m_nRobotHealth;
	int m_nPoints;
	int m_nNumGibs;
	const char *m_pszPathName;
	const char *m_pszGroupName;
} RobotSpawnData_t;

typedef struct RateLimitedSound_s
{
	RateLimitedSound_s( float flDelay )
		: m_flDelay( flDelay ), m_NextAvailableTime( DefLessFunc( CBaseEntity const * ) ) {}
	float m_flDelay;
	CUtlMap<CBaseEntity const *, float> m_NextAvailableTime;
} RateLimitedSound_t;

class CRobotDestructionVaultTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CRobotDestructionVaultTrigger, CBaseTrigger );
	DECLARE_DATADESC();

public:
	CRobotDestructionVaultTrigger();
	virtual void	Precache();
	virtual void	Spawn();

	virtual bool	PassesTriggerFilters( CBaseEntity *pOther );
	virtual void	StartTouch( CBaseEntity *pOther );
	virtual void	EndTouch( CBaseEntity *pOther );

	int				StealPoints( CTFPlayer *pPlayer );
	void			StealPointsThink();

private:
	COutputEvent m_OnPointsStolen;
	COutputEvent m_OnPointsStartStealing;
	COutputEvent m_OnPointsEndStealing;

	bool m_bStealing;
};
#endif

class CTFRobotDestruction_RobotSpawn : public CBaseEntity
{
	DECLARE_CLASS( CTFRobotDestruction_RobotSpawn, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFRobotDestruction_RobotSpawn();

	virtual void	Activate( void );
	virtual void	Precache( void );
	virtual void	Spawn( void );
#if defined(GAME_DLL)
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void			ClearRobot( void );
	void			OnRobotKilled( void );
	void			SpawnRobot( void );

	void			InputSpawnRobot( inputdata_t &inputdata );

	CTFRobotDestruction_Robot *GetRobot( void ) const { return m_hRobot.Get(); }
	void			SetRobotGroup( CTFRobotDestruction_RobotGroup *pGroup ) { m_hRobotGroup.Set( pGroup ); }

private:
	CHandle<CTFRobotDestruction_Robot> m_hRobot;
	CHandle<CTFRobotDestruction_RobotGroup> m_hRobotGroup;
	RobotSpawnData_t m_spawnData;
	COutputEvent m_OnRobotKilled;
#endif
};

DECLARE_AUTO_LIST( IRobotDestructionGroupAutoList );
class CTFRobotDestruction_RobotGroup : public CBaseEntity, public IRobotDestructionGroupAutoList
{
#if defined(GAME_DLL)
	static float sm_flNextAllowedAttackAlertTime[TF_TEAM_COUNT];
#endif
	DECLARE_CLASS( CTFRobotDestruction_RobotGroup, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFRobotDestruction_RobotGroup();
	virtual ~CTFRobotDestruction_RobotGroup();

#if defined(GAME_DLL)
	virtual void	Activate( void );
	virtual void	Spawn( void );
	virtual int		UpdateTransmitState( void );

	float			GetTeamRespawnScale() const { return m_flTeamRespawnReductionScale; }

	void			AddToGroup( CTFRobotDestruction_RobotSpawn *pSpawn );
	void			DisableUberForGroup( void );
	void			EnableUberForGroup( void );
	int				GetNumAliveBots( void );
	void			OnRobotAttacked( void );
	void			OnRobotKilled( void );
	void			OnRobotSpawned( void );
	void			RemoveFromGroup( CTFRobotDestruction_RobotSpawn *pSpawn );
	void			RespawnCountdownFinish( void );
	void			RespawnRobots( void );
	void			StartRespawnTimerIfNeeded( CTFRobotDestruction_RobotGroup *pGroup );
	void			StopRespawnTimer( void );
	void			UpdateState( void );
#else
	virtual int		GetTeamNumber( void ) const OVERRIDE { return m_iTeamNum; }
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	virtual void	SetDormant( bool bDormant );
#endif

private:
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iTeamNum );
#ifdef GAME_DLL
	CUtlVector< CHandle<CTFRobotDestruction_RobotSpawn> > m_vecSpawns;

	int m_nTeamNumber;
	float m_flRespawnTime;
	string_t m_iszHudIcon;
	float m_flTeamRespawnReductionScale;

	COutputEvent m_OnRobotsRespawn;
	COutputEvent m_OnAllRobotsDead;
#endif
public:
	CNetworkString( m_pszHudIcon, MAX_PATH );
	CNetworkVar( int, m_nGroupNumber );
	CNetworkVar( int, m_nState );
	CNetworkVar( float, m_flRespawnStartTime );
	CNetworkVar( float, m_flRespawnEndTime );
	CNetworkVar( float, m_flLastAttackedTime );
};


class CTFRobotDestructionLogic : public CBaseEntity
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
	static CTFRobotDestructionLogic *sm_CTFRobotDestructionLogic;
	DECLARE_CLASS( CTFRobotDestructionLogic, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	enum
	{
		TYPE_ROBOT_DESTRUCTION,
		TYPE_PLAYER_DESTRUCTION,
	};

	static CTFRobotDestructionLogic *GetRobotDestructionLogic();

	CTFRobotDestructionLogic();
	virtual ~CTFRobotDestructionLogic();

	virtual void	Precache( void );
	virtual void	Spawn( void );

	float			GetFinaleLength() const { return m_flFinaleLength; }
	float			GetFinaleWinTime( int nTeam ) const;
	int				GetMaxPoints( void ) const { return m_nMaxPoints; }
	float			GetRespawnScaleForTeam( int nTeam ) const;
	int				GetScore( int nTeam ) const;
	int				GetTargetScore( int nTeam ) const;

#if defined(GAME_DLL)
	virtual void	Activate( void );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual int		UpdateTransmitState( void );

	void			AddRobotGroup( CTFRobotDestruction_RobotGroup *pGroup );
	void			ApproachTargetScoresThink( void );
	int				ApproachTeamTargetScore( int nTeam, int nTargetScore, int nScore );
	void			BlueTeamWin( void );
	void			FlagCreated( int nTeam );
	void			FlagDestroyed( int nTeam );
	CTFRobotDestruction_Robot *IterateRobots( CTFRobotDestruction_Robot *pIter );
	void			ManageGameState( void );
	void			PlaySoundInPlayersEars( CTFPlayer *pSpeaker, EmitSound_t const &params );
	void			PlaySoundInfoForScoreEvent( CTFPlayer *pSpeaker, bool bEnemyScore, int nScore, int nTeam, ERDScoreMethod eEvent = SCORE_UNDEFINED );
	void			RedTeamWin( void );
	void			RobotAttacked( CTFRobotDestruction_Robot *pRobot );
	void			RobotCreated( CTFRobotDestruction_Robot *pRobot );
	void			RobotRemoved( CTFRobotDestruction_Robot *pRobot );
	void			SetMaxPoints( int nPoints ) { m_nMaxPoints = nPoints; }
	void			ScorePoints( int nTeam, int nPoints, ERDScoreMethod eEvent, CTFPlayer *pScorer );

	void			InputRoundActivate( inputdata_t &inputdata );

	virtual int		GetHealDistance( void ) { return 64; }
#else
	virtual void ClientThink();
	virtual void OnDataChanged( DataUpdateType_t type );

	const char *GetResFile() const { return STRING( m_szResFile ); }
#endif

	virtual int GetType() const { return TYPE_ROBOT_DESTRUCTION; }

	virtual void	SetCountdownEndTime( float flTime ) { m_flCountdownEndTime = flTime; }
	virtual float	GetCountdownEndTime() { return m_flCountdownEndTime; }
	virtual CTFPlayer *GetTeamLeader( int iTeam ) const { return NULL; }
	virtual string_t GetCountdownImage( void ) { return NULL_STRING; }
	virtual bool	IsUsingCustomCountdownImage( void ) { return false; }
#if defined(GAME_DLL)
	virtual void	OnRedScoreChanged() {}
	virtual void	OnBlueScoreChanged() {}
	virtual void	TeamWin( int nTeam );
#endif

	// Shared info with CTFPlayerDestructionLogic by using protected
protected:
#if defined(GAME_DLL)
	CUtlVector<CTFRobotDestruction_Robot *> m_vecRobots;
	CUtlVector<CTFRobotDestruction_RobotGroup *> m_vecRobotGroups;
	int m_nNumFlags[TF_TEAM_COUNT];
	float m_flLoserRespawnBonusPerBot;
	float m_flRobotScoreInterval;
	string_t m_iszResFile;

	CUtlMap<char const *, RateLimitedSound_t *> m_RateLimitedSounds;

	COutputEvent m_OnRedHitZeroPoints;
	COutputEvent m_OnRedHasPoints;
	COutputEvent m_OnRedFinalePeriodEnd;
	COutputEvent m_OnBlueHitZeroPoints;
	COutputEvent m_OnBlueHasPoints;
	COutputEvent m_OnBlueFinalePeriodEnd;
	COutputEvent m_OnRedFirstFlagStolen;
	COutputEvent m_OnRedFlagStolen;
	COutputEvent m_OnRedLastFlagReturned;
	COutputEvent m_OnBlueFirstFlagStolen;
	COutputEvent m_OnBlueFlagStolen;
	COutputEvent m_OnBlueLastFlagReturned;
	COutputEvent m_OnBlueLeaveMaxPoints;
	COutputEvent m_OnRedLeaveMaxPoints;
	COutputEvent m_OnBlueHitMaxPoints;
	COutputEvent m_OnRedHitMaxPoints;
#endif
	CNetworkVar( int, m_nMaxPoints );
	CNetworkVar( float, m_flFinaleLength );
	CNetworkVar( float, m_flBlueFinaleEndTime );
	CNetworkVar( float, m_flRedFinaleEndTime );
	CNetworkVar( int, m_nBlueScore );
	CNetworkVar( int, m_nRedScore );
	CNetworkVar( int, m_nBlueTargetPoints );
	CNetworkVar( int, m_nRedTargetPoints );
	CNetworkVar( float, m_flBlueTeamRespawnScale );
	CNetworkVar( float, m_flRedTeamRespawnScale );
	CNetworkString( m_szResFile, MAX_PATH );
	CNetworkArray( int, m_eWinningMethod, TF_TEAM_COUNT );
	CNetworkVar( float, m_flCountdownEndTime );
};

#endif