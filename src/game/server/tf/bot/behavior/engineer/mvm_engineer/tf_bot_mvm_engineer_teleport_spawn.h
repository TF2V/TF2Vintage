//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MVM_ENGINEER_TELEPORT_SPAWN_H
#define TF_BOT_MVM_ENGINEER_TELEPORT_SPAWN_H

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_entity.h"


class CTFBotMvMEngineerTeleportSpawn : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerTeleportSpawn( CBaseTFBotHintEntity *hint, bool non_silent );
	virtual ~CTFBotMvMEngineerTeleportSpawn();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_ctPushAway;
	CHandle<CBaseTFBotHintEntity> m_hintEntity;
	bool m_bNonSilent;
};

#endif