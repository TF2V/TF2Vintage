//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_ESCORT_H
#define TF_BOT_ESCORT_H

#include "NextBotBehavior.h"

class CTFBotEscort : public Action<CTFBot>
{
public:
	CTFBotEscort( CBaseCombatCharacter *who );
	virtual ~CTFBotEscort();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *priorAction ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual EventDesiredResult<CTFBot> OnMoveToSuccess( CTFBot *me, const Path *path ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck( CTFBot *me ) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnCommandApproach( CTFBot *me, const Vector &v1, float f1 ) OVERRIDE;

	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;

	CBaseCombatCharacter *GetWho() const;
	void SetWho( CBaseCombatCharacter *who );

private:
	CBaseCombatCharacter *m_hWho;
	PathFollower m_PathFollower;
	// 480c CountdownTimer
	CountdownTimer m_ctRecomputePath;
};

#endif