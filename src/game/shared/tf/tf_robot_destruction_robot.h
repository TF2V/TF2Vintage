//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ROBOT_DESTRUCTION_ROBOT_H
#define ROBOT_DESTRUCTION_ROBOT_H

#ifdef GAME_DLL
	#include "tf_shareddefs.h"
	#include "pathtrack.h"
	#include "NextBotGroundLocomotion.h"
	#include "NextBotBehavior.h"
	#include "NextBot.h"
	#include "tf_obj_dispenser.h"
#else
	#include "c_obj_dispenser.h"
	#include "NextBot/c_NextBot.h"
#endif

#include "props_shared.h"
#include "tf_logic_robot_destruction.h"

#ifdef CLIENT_DLL
#define NextBotCombatCharacter C_NextBotCombatCharacter
#define CTFRobotDestruction_Robot C_TFRobotDestruction_Robot

#define CObjectDispenser C_ObjectDispenser
#define CRobotDispenser C_RobotDispenser

#define CRobotBody C_RobotBody
#endif

class CTFRobotDestruction_Robot;

#ifdef GAME_DLL

class CRobotBehavior : public Action<CTFRobotDestruction_Robot>
{
public:
	virtual const char *GetName( void ) const { return "RobotBehavior"; }

	virtual Action<CTFRobotDestruction_Robot> *InitialContainedAction( CTFRobotDestruction_Robot *me );

	virtual ActionResult<CTFRobotDestruction_Robot> OnStart( CTFRobotDestruction_Robot *me, Action<CTFRobotDestruction_Robot> *priorAction );
	virtual ActionResult<CTFRobotDestruction_Robot> Update( CTFRobotDestruction_Robot *me, float dt );

	virtual QueryResultType	IsPositionAllowed( const INextBot *me, const Vector &pos ) const;

	virtual EventDesiredResult<CTFRobotDestruction_Robot> OnInjured( CTFRobotDestruction_Robot *me, const CTakeDamageInfo &info );
	EventDesiredResult<CTFRobotDestruction_Robot> OnContact( CTFRobotDestruction_Robot *me, CBaseEntity *pOther, CGameTrace *result = NULL );

private:
	CountdownTimer m_speakTimer;
	CountdownTimer m_idleTimer;
};


class CRobotLocomotion : public NextBotGroundLocomotion
{
public:
	CRobotLocomotion( INextBot *bot );
	virtual ~CRobotLocomotion() { }

	virtual float GetGroundSpeed( void ) const OVERRIDE;
	virtual float GetRunSpeed( void ) const OVERRIDE;								// get maximum running speed
	virtual float GetStepHeight( void ) const OVERRIDE { return 24.f; }				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const OVERRIDE { return 40.f; }			// return maximum height of a jump
	virtual float GetMaxYawRate( void ) const OVERRIDE { return 200.f; }				// return max rate of yaw rotation

	virtual bool ShouldCollideWith( const CBaseEntity *object ) const OVERRIDE;
};

class CRobotBody : public IBody
{
public:
	CRobotBody( INextBot *actor );
	virtual ~CRobotBody() { };

	virtual void Update( void ) OVERRIDE;

	virtual unsigned int GetSolidMask( void ) const OVERRIDE { return MASK_NPCSOLID|CONTENTS_PLAYERCLIP; }

	virtual Activity GetActivity( void ) const OVERRIDE { return m_Activity; }
	virtual bool StartActivity( Activity act, unsigned int flags = 0 ) OVERRIDE;
	virtual bool IsActivity( Activity act ) const OVERRIDE { return m_Activity == act; }

	void Impulse( Vector const& vecImpulse );

private:
	void Approach( Vector& vecIn, Vector const& vecTarget, float flRate );

	Activity m_Activity;

	int m_iMoveX;
	int m_iMoveY;

	Vector m_vecPrevOrigin;
	Vector m_vecLean;
	Vector m_vecImpulse;
};
#else

class C_RobotBody
{
public:
	C_RobotBody( CBaseCombatCharacter *actor );

	void Update( void );
	void Impulse( Vector const& vecImpulse );

private:
	void Approach( Vector& vecIn, Vector const& vecTarget, float flRate );

	CBaseCombatCharacter *m_Actor;

	int m_iMoveX;
	int m_iMoveY;

	Vector m_vecPrevOrigin;
	Vector m_vecLean;
	Vector m_vecImpulse;
};
#endif


class CRobotDispenser : public CObjectDispenser
{
	DECLARE_CLASS( CRobotDispenser, CObjectDispenser )
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

#ifdef GAME_DLL
	CRobotDispenser();

	virtual void Spawn( void );
	virtual void OnGoActive( void );
	virtual void GetControlPanelInfo( int nPanelIndex, const char *&pPanelName ) OVERRIDE { }
	virtual void SetModel( const char *pModel );
	virtual float GetDispenserRadius( void ) { return 128; }
	virtual float GetHealRate() { return 5.f; }

	virtual int DispenseMetal( CTFPlayer* ) { return 0; }
	virtual bool DispenseAmmo( CTFPlayer* ) { return false; }

	virtual void PlayActiveSound() OVERRIDE { }
#endif
};


class CTFRobotDestruction_Robot : public NextBotCombatCharacter
#ifdef CLIENT_DLL
	, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFRobotDestruction_Robot, NextBotCombatCharacter )
public:
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFRobotDestruction_Robot( void );
	virtual ~CTFRobotDestruction_Robot( void );

	virtual void Precache( void );

	virtual void Spawn( void );
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

#ifdef GAME_DLL
	DECLARE_INTENTION_INTERFACE( CTFRobotDestruction_Robot );
	virtual CRobotLocomotion	*GetLocomotionInterface( void ) const OVERRIDE	{ return m_loco; }
	virtual CRobotBody			*GetBodyInterface( void ) const	OVERRIDE		{ return m_body; }

	virtual void	HandleAnimEvent( animevent_t *pEvent );

	virtual bool	IsRemovedOnReset( void ) const { return false; }
	virtual void	UpdateOnRemove( void );

	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	void			InputStopAndUseComputer( inputdata_t &inputdata );

	void			RepairSelfThink( void );
	void			SelfDestructThink( void );

	void			EnableUber( void );
	void			DisableUber( void );

	void			SetPanicking( bool bSet )	{ m_bPanicking = bSet; }
	bool			IsPanicking( void ) const	{ return m_bPanicking; }

	CPathTrack*		GetGoalPath( void ) const	{ return m_hGoalPath; }
	void			ArriveAtPath( void );

	void			SetRobotGroup( CTFRobotDestruction_RobotGroup const *pGroup )	{ m_hRobotGroup = pGroup; }
	void			SetRobotSpawn( CTFRobotDestruction_RobotSpawn const *pSpawn )	{ m_hRobotSpawn = pSpawn; }
	void			SetSpawnData( RobotSpawnData_t const &data )					{ m_spawnData = data; }
	
	int				GetType( void ) const		{ return m_eType; }
	void			SetType( ERobotType eType ) { m_eType = eType; }

	bool			IsShielded( void ) const	{ return m_bShielded; }
	void			SetShielded( bool bSet )	{ m_bShielded = bSet; }

private:
	void			PlayDeathEffects( void );
	void			ModifyDamage( CTakeDamageInfo *info ) const;
	void			SpewBars( int nNumToSpew );
	void			SpewBarsThink( void );
	void			SpewGibs( void );
#else
	virtual int		GetHealth( void ) const OVERRIDE { return m_iHealth; }
	virtual int		GetMaxHealth( void ) const OVERRIDE { return m_iMaxHealth; }
	virtual float	GetHealthBarHeightOffset( void ) const OVERRIDE;
	virtual bool	IsHealthBarVisible( void ) const OVERRIDE { return true; }

	virtual void	FireGameEvent( IGameEvent *event );

	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual CStudioHdr* OnNewModel();
	void			UpdateDamagedEffects( void );

	virtual void	FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void	UpdateClientSideAnimation( void );
#endif

private:
	int			m_iHealth;
	int			m_iMaxHealth;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );

	CNetworkVar( bool,	m_bShielded );
	CNetworkVar( int,	m_eType );

	CUtlVector<breakmodel_t> m_aGibs;
	CUtlVector<breakmodel_t> m_aSpawnProps;

#ifdef GAME_DLL

	CRobotLocomotion *m_loco;
	CRobotBody		*m_body;

	CHandle<CRobotDispenser> m_hDispenser;
	CHandle<CPathTrack>		m_hGoalPath;

	CHandle<CTFRobotDestruction_RobotGroup> m_hRobotGroup;
	CHandle<CTFRobotDestruction_RobotSpawn> m_hRobotSpawn;
	RobotSpawnData_t m_spawnData;

	int m_nBarsSpewed;
	bool m_bPanicking;
#else

	HPARTICLEFFECT	m_hDamagedEffect;
	C_RobotBody*	m_body;
#endif
};

#ifdef GAME_DLL
class CRobotPathCost : public IPathCost
{
public:
	CRobotPathCost( CTFRobotDestruction_Robot *me )
	{
		m_Actor = me;
	}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const OVERRIDE
	{
		if ( fromArea == nullptr )
		{
			// first area in path; zero cost
			return 0.0f;
		}

		if ( !m_Actor->GetLocomotionInterface()->IsAreaTraversable( area ) )
		{
			// dead end
			return -1.0f;
		}

		if ( ladder != nullptr )
			length = ladder->m_length;
		else if ( length <= 0.0f )
			length = ( area->GetCenter() - fromArea->GetCenter() ).Length();

		const float dz = fromArea->ComputeAdjacentConnectionHeightChange( area );
		if ( dz >= m_Actor->GetLocomotionInterface()->GetStepHeight() )
		{
			if ( dz >= m_Actor->GetLocomotionInterface()->GetMaxJumpHeight() )
				return -1.0f;

			// we won't actually get here according to the locomotor
			length *= 5;
		}
		else
		{
			if ( dz < -m_Actor->GetLocomotionInterface()->GetDeathDropHeight() )
				return -1.0f;
		}

		return length + fromArea->GetCostSoFar();
	}

private:
	CTFRobotDestruction_Robot *m_Actor;
};
#endif
#endif // ROBOT_DESTRUCTION_ROBOT_H
