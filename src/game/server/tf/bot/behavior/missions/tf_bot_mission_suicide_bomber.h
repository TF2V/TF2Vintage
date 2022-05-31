//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MISSION_SUICIDE_BOMBER_H
#define TF_BOT_MISSION_SUICIDE_BOMBER_H

#include "NextBotBehavior.h"

class CTFBotMissionSuicideBomber : public Action<CTFBot>
{
public:
	CTFBotMissionSuicideBomber();
	virtual ~CTFBotMissionSuicideBomber();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnKilled( CTFBot *me, const CTakeDamageInfo &info ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	void StartDetonate( CTFBot *me, bool reached_goal, bool lost_all_health );
	void Detonate( CTFBot *me );

	CHandle<CBaseEntity> m_hTarget;   // +0x0034
	Vector m_vecTargetPos;            // +0x0038
	PathFollower m_PathFollower;      // +0x0044
	CountdownTimer m_ctRecomputePath; // +0x4818, fires every 0.5-1.0 seconds to recompute path
	CountdownTimer m_ctPlaySound;     // +0x4824, fires every 4 seconds to play sound "MVM.SentryBusterIntro"
	CountdownTimer m_ctDetonation;    // +0x4830, started when detonation sequence begins (duration: 2.0 seconds)
	bool m_bDetonating;               // +0x483c
	bool m_bDetReachedGoal;           // +0x483d
	bool m_bDetLostAllHealth;         // +0x483e
	int m_nConsecutivePathFailures;   // +0x4840
	Vector m_vecDetonatePos;          // +0x4844
};

#endif