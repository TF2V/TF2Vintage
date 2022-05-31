//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MISSION_DESTROY_SENTRIES_H
#define TF_BOT_MISSION_DESTROY_SENTRIES_H

#include "NextBotBehavior.h"

class CObjectSentrygun;

// sizeof: TODO (>=0x38)
class CTFBotMissionDestroySentries : public Action<CTFBot>
{
public:
	CTFBotMissionDestroySentries( CObjectSentrygun *sentry );
	virtual ~CTFBotMissionDestroySentries();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;

private:
	CObjectSentrygun *SelectSentryTarget( CTFBot *actor );

	CHandle<CObjectSentrygun> m_hSentry; // +0x34
};

#endif