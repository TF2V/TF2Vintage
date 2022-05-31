//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ESCORT_SQUAD_LEADER_H
#define TF_BOT_ESCORT_SQUAD_LEADER_H

#include "NextBotBehavior.h"
#include "../tf_bot_melee_attack.h"

class CTFBotEscortSquadLeader : public Action<CTFBot>
{
public:
	CTFBotEscortSquadLeader( Action<CTFBot> *done_action );
	virtual ~CTFBotEscortSquadLeader();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;
	virtual void OnEnd( CTFBot *me, Action<CTFBot> *newAction ) OVERRIDE;

private:
	Action<CTFBot> *m_DoneAction;
	CTFBotMeleeAttack m_MeleeAttack;
	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	Vector m_vecLeaderGoalDirection;
};


// sizeof: 0x40
class CTFBotWaitForOutOfPositionSquadMember : public Action<CTFBot>
{
public:
	CTFBotWaitForOutOfPositionSquadMember();
	virtual ~CTFBotWaitForOutOfPositionSquadMember();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

private:
	CountdownTimer m_ctTimeout;
};

#endif