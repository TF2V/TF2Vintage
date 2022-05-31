//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_BOT_MVM_ENGINEER_IDLE_H
#define TF_BOT_MVM_ENGINEER_IDLE_H

#include "NextBotBehavior.h"
#include "map_entities/tf_hint_engineer_nest.h"

class CTFBotMvMEngineerIdle : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerIdle();
	virtual ~CTFBotMvMEngineerIdle();

	virtual const char *GetName() const OVERRIDE;

	virtual ActionResult<CTFBot> OnStart( CTFBot *me, Action<CTFBot> *action ) OVERRIDE;
	virtual ActionResult<CTFBot> Update( CTFBot *me, float dt ) OVERRIDE;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const OVERRIDE;
	virtual QueryResultType ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const OVERRIDE;

private:
	bool ShouldAdvanceNestSpot( CTFBot *me );
	void TakeOverStaleNest( CBaseTFBotHintEntity *hint, CTFBot *me );
	void TryToDetonateStaleNest();

	PathFollower m_PathFollower;
	CountdownTimer m_ctRecomputePath;
	CountdownTimer m_ctSentrySafe;
	CountdownTimer m_ctSentryCooldown;
	CountdownTimer m_ctTeleCooldown;
	CountdownTimer m_ctFindNestHint;
	CountdownTimer m_ctAdvanceNestSpot;
	int m_nTeleportAttempts;
	bool m_bTeleportedToHint;
	CHandle<CTFBotHintTeleporterExit> m_hHintTele;
	CHandle<CTFBotHintSentrygun> m_hHintSentry;
	CHandle<CTFBotHintEngineerNest> m_hHintNest;
	bool m_bTriedToDetonateStaleNest;
};


class CTFBotMvMEngineerHintFinder
{
public:
	static bool FindHint( bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint = nullptr );
};


struct BombInfo_t
{
	Vector closest_pos;
	float hatch_dist_fwd;
	float hatch_dist_back;
};


CTFBotHintEngineerNest *SelectOutOfRangeNest( const CUtlVector<CTFBotHintEngineerNest *> &nests );
bool GetBombInfo( BombInfo_t *info );

#endif