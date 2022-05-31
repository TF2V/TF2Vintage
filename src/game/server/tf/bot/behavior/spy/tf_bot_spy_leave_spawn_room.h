//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef	TF_BOT_SPY_LEAVE_SPAWN_ROOM_H
#define TF_BOT_SPY_LEAVE_SPAWN_ROOM_H

#include "NextBotBehavior.h"

// sizeof: 0x44
class CTFBotSpyLeaveSpawnRoom : public Action<CTFBot>
{
public:
	CTFBotSpyLeaveSpawnRoom();
	virtual ~CTFBotSpyLeaveSpawnRoom();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	CountdownTimer m_ctTeleport; // +0x34
	int m_nDistance;             // +0x40
};


bool TeleportNearVictim( CTFBot *spy, CTFPlayer *victim, int i1 );

#endif