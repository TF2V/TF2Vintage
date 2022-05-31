//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ROBOT_DESTRUCTION_ROBOT_BEHAVIORS_H
#define ROBOT_DESTRUCTION_ROBOT_BEHAVIORS_H

#include "NextBotBehavior.h"
#include "Path/NextBotPathFollow.h"
class CTFRobotDestruction_Robot;

class CRobotPatrol : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "Patrol"; }

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );
	virtual ActionResult<CTFRobotDestruction_Robot> OnResume( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *interruptingAction );

private:
	PathFollower m_PathFollower;
};


class CRobotSpawn : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "Spawn"; }

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );
	virtual void OnEnd( CTFRobotDestruction_Robot *me, Action< CTFRobotDestruction_Robot > *nextAction );

	EventDesiredResult<CTFRobotDestruction_Robot> OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );
};


class CRobotMaterialize : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "Materialize"; }

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );
};


class CRobotEnterPanic : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "Enter Panic"; }

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );

	EventDesiredResult<CTFRobotDestruction_Robot> OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );
};


class CRobotPanic : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );
	virtual void OnEnd( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *nextAction );

	virtual EventDesiredResult<CTFRobotDestruction_Robot> OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const { return "Panic"; }

private:
	CountdownTimer m_speakTimer;
	CountdownTimer m_attackedTimer;
	CountdownTimer m_spinTimer;
	bool m_bSpinRight;
};


class CRobotLeavePanic : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "Leave Panic"; }

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );

	EventDesiredResult<CTFRobotDestruction_Robot> OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );
};

#endif