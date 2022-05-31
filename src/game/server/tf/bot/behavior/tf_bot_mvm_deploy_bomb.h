//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MISSION_DESTROY_SENTRIES_H
#define TF_BOT_MISSION_DESTROY_SENTRIES_H

#include "NextBotBehavior.h"

class CTFBotMvMDeployBomb : public Action<CTFBot>
{
public:
	CTFBotMvMDeployBomb();
	virtual ~CTFBotMvMDeployBomb();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnContact( CTFBot *me, CBaseEntity *who, CGameTrace *trace ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	CountdownTimer m_ctDelay;
	Vector m_vecStand;
};

#endif