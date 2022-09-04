#ifndef TF_BOT_SNIPER_LURK_H
#define TF_BOT_SNIPER_LURK_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotBehavior.h"

class CTFBotSniperLurk : public Action<CTFBot>
{
public:
	CTFBotSniperLurk();
	virtual ~CTFBotSniperLurk();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *actor, Action<CTFBot> *newAction ) OVERRIDE;
	virtual ActionResult<CTFBot> OnSuspend( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;
	virtual ActionResult<CTFBot> OnResume( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;

	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

	virtual const CKnownEntity *SelectMoreDangerousThreat( const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const OVERRIDE;

private:
	bool FindHint( CTFBot *actor );
	bool FindNewHome( CTFBot *actor );

	CountdownTimer m_patienceDuration;
	CountdownTimer m_recomputePathTimer;
	PathFollower m_PathFollower;
	int m_nAttempts;
	Vector m_vecHome;
	bool m_bHasHome;
	bool m_bNearHome;
	CountdownTimer m_findHomeTimer;
	bool m_bOpportunistic;
	CUtlVector< CHandle<CTFBotHint> > m_Hints;
	CHandle<CTFBotHint> m_hHint;
};

#endif
