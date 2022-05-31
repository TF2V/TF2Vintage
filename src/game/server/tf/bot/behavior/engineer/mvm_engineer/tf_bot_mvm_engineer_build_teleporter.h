//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MVM_ENGINEER_BUILD_TELEPORTER_H
#define TF_BOT_MVM_ENGINEER_BUILD_TELEPORTER_H

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_teleexit.h"

class CTFBotMvMEngineerBuildTeleportExit : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerBuildTeleportExit( CTFBotHintTeleporterExit *hint );
	virtual ~CTFBotMvMEngineerBuildTeleportExit();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

private:
	CHandle<CTFBotHintTeleporterExit> m_hintEntity;
	CountdownTimer m_ctPushAway;
	CountdownTimer m_ctRecomputePath;
	PathFollower m_PathFollower;
};

#endif