//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_TANK_BOSS_H
#define TF_TANK_BOSS_H


#include "NextBotBodyInterface.h"
#include "tf_base_boss.h"
#include "pathtrack.h"
#include "tf_populators.h"

class INextBot;


class CTFTankBossBody : public IBody
{
public:
	CTFTankBossBody( INextBot *bot );
	virtual ~CTFTankBossBody() { }

	virtual void Update( void );
	virtual unsigned int GetSolidMask( void ) const;

	bool StartSequence( const char *name );
	void SetSkin( int nSkin );
};

class CTFTankBoss : public CTFBaseBoss
{
	static float sm_flLastTankAlert;
public:
	DECLARE_CLASS( CTFTankBoss, CTFBaseBoss );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFTankBoss();
	virtual ~CTFTankBoss();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void SetSkin( int nSkin );
	virtual void UpdateOnRemove( void );
	virtual void UpdateCollisionBounds( void );

	virtual CTFTankBossBody *GetBodyInterface( void ) const OVERRIDE { return m_body; }

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &rawInfo );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual int GetCurrencyValue( void );

	void TankBossThink( void );

	void InputDestroyIfAtCapturePoint( inputdata_t &inputdata );
	void InputAddCaptureDestroyPostfix( inputdata_t &inputdata );

	void SetStartingPathTrackNode( char *pszName );

	inline void SetOnKilledEvent( EventInfo *eventInfo )
	{
		if ( eventInfo )
		{
			m_onKilledEventInfo.m_action = eventInfo->m_action;
			m_onKilledEventInfo.m_target = eventInfo->m_target;
		}
	}

	inline void SetOnBombDroppedEvent( EventInfo *eventInfo )
	{
		if ( eventInfo )
		{
			m_onBombDroppedEventInfo.m_action = eventInfo->m_action;
			m_onBombDroppedEventInfo.m_target = eventInfo->m_target;
		}
	}

	inline void SetWaveSpawnPopulator( CWaveSpawnPopulator *pWave ) { m_pWaveSpawnPopulator = pWave; }

protected:
	virtual void ModifyDamage( CTakeDamageInfo *info ) OVERRIDE;

private:
	void Explode( void );
	void FirePopFileEvent( EventInfo *eventInfo );
	void UpdatePingSound( void );
	float m_flLastPingTime;

	CTFTankBossBody *m_body;

	int m_nLastHealth;
	int m_iDamageModelIndex;
	int m_nDeathAnimPick;
	char m_szDeathPostfix[8];
	bool m_bKilledByPlayers;
	bool m_bDroppingBomb;
	float m_flDroppingStart;
	int m_iExhaustAttachment;
	bool m_bSmoking;

	EventInfo m_onKilledEventInfo;
	EventInfo m_onBombDroppedEventInfo;

	CHandle<CPathTrack> m_hStartNode;
	CHandle<CPathTrack> m_hEndNode;
	CHandle<CPathTrack> m_hGoalNode;
	int m_nNodeNumber;

	CUtlVector< float > m_CumulativeDistances;
	float m_flTotalDistance;
	bool m_bPlayedNearAlert;
	bool m_bPlayedHalfwayAlert;

	CHandle<CBaseAnimating> m_hBomb;
	CHandle<CBaseAnimating> m_hLeftTrack;
	CHandle<CBaseAnimating> m_hRightTrack;

	Vector m_vecRightTrackPrevPos;
	Vector m_vecLeftTrackPrevPos;

	CountdownTimer m_rumbleTimer;
	CountdownTimer m_crushTimer;

	CWaveSpawnPopulator *m_pWaveSpawnPopulator;

	Vector m_vecCollisionMins;
	Vector m_vecCollisionMaxs;
};

inline void CTFTankBoss::SetSkin( int nSkin )
{
	if ( m_body )
		m_body->SetSkin( nSkin );
}

#endif
