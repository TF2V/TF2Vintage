//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MVM_ENGINEER_BUILD_SENTRYGUN_H
#define TF_BOT_MVM_ENGINEER_BUILD_SENTRYGUN_H

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_sentrygun.h"

class CObjectSentrygun;

class CTFBotMvMEngineerBuildSentryGun : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerBuildSentryGun( CTFBotHintSentrygun *hint );
	virtual ~CTFBotMvMEngineerBuildSentryGun();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;

private:
	CHandle<CTFBotHintSentrygun> m_hintEntity;
	CHandle<CObjectSentrygun> m_hSentry;
	CountdownTimer m_ctPushAway;
	CountdownTimer m_ctRecomputePath;
	PathFollower m_PathFollower;
};

#endif