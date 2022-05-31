//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_H
#define TF_BOT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBot/Player/NextBotPlayer.h"
#include "tier1/utlstack.h"
#include "tf_obj.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_path_follower.h"

class CTeamControlPoint;
class CCaptureFlag;
class CCaptureZone;
class CTFBotSquad;
class CSoundPatch;

class CClosestTFPlayer
{
public:
	CClosestTFPlayer( Vector start )
		: m_vecOrigin( start )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
		m_iTeam = TEAM_ANY;
	}

	CClosestTFPlayer( Vector start, int teamNum )
		: m_vecOrigin( start ), m_iTeam( teamNum )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
	}

	inline bool operator()( CBasePlayer *player )
	{
		if ( ( player->GetTeamNumber() == TF_TEAM_RED || player->GetTeamNumber() == TF_TEAM_BLUE )
			 && ( m_iTeam == TEAM_ANY || player->GetTeamNumber() == m_iTeam ) )
		{
			float flDistance = ( m_vecOrigin - player->GetAbsOrigin() ).LengthSqr();
			if ( flDistance < m_flMinDist )
			{
				m_flMinDist = flDistance;
				m_pPlayer = (CTFPlayer *)player;
			}
		}

		return true;
	}

	Vector m_vecOrigin;
	float m_flMinDist;
	CTFPlayer *m_pPlayer;
	int m_iTeam;
};


#define TF_BOT_TYPE		1337

class CTFBot : public NextBotPlayer<CTFPlayer>, public CGameEventListener
{
	DECLARE_CLASS( CTFBot, NextBotPlayer<CTFPlayer> )
public:

	static CBasePlayer *AllocatePlayerEntity( edict_t *edict, const char *playerName );

	CTFBot( CTFPlayer *player=nullptr );
	virtual ~CTFBot();

	DECLARE_INTENTION_INTERFACE( CTFBot )
	virtual ILocomotion *GetLocomotionInterface( void ) const OVERRIDE { return m_locomotor; }
	virtual IBody *GetBodyInterface( void ) const OVERRIDE { return m_body; }
	virtual IVision *GetVisionInterface( void ) const OVERRIDE { return m_vision; }
	virtual CBaseCombatCharacter *GetEntity( void ) const;
	virtual bool	IsDormantWhenDead( void ) const { return false; }
	virtual bool	IsDebugFilterMatch( const char *name ) const;

	virtual void	Spawn( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual void	UpdateOnRemove( void ) OVERRIDE;
	virtual void	FireGameEvent( IGameEvent *event );
	virtual int		GetBotType() const { return TF_BOT_TYPE; }
	virtual int		DrawDebugTextOverlays( void );
	virtual void	PhysicsSimulate( void );
	virtual void	Touch( CBaseEntity *other );
	virtual void	AvoidPlayers( CUserCmd *pCmd );
	virtual bool	IsAllowedToPickUpFlag( void );

	virtual void	PressFireButton( float duration = -1.0f );
	virtual void	PressAltFireButton( float duration = -1.0f );
	virtual void	PressSpecialFireButton( float duration = -1.0f );

	bool			IsCombatWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsQuietWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsHitScanWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsExplosiveProjectileWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsContinuousFireWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsBarrageAndReloadWeapon( CTFWeaponBase *weapon = nullptr ) const;

	void			AddItem( char const *pszItemName );

	float			GetMaxAttackRange( void ) const;
	float			GetDesiredAttackRange( void ) const;

	float			GetDesiredPathLookAheadRange( void ) const;

	bool			ShouldFireCompressionBlast( void );

	CTFNavArea*		FindVantagePoint( float flMaxDist );

	bool			IsLineOfFireClear( CBaseEntity *to );
	bool			IsLineOfFireClear( const Vector& to );
	bool			IsLineOfFireClear( const Vector& from, CBaseEntity *to );
	bool			IsLineOfFireClear( const Vector& from, const Vector& to );
	bool			IsAnyEnemySentryAbleToAttackMe( void ) const;
	bool			IsThreatAimingTowardsMe( CBaseEntity *threat, float dotTolerance = 0.8 ) const;
	bool			IsThreatFiringAtMe( CBaseEntity *threat ) const;
	bool			IsEntityBetweenTargetAndSelf( CBaseEntity *blocker, CBaseEntity *target ) const;

	bool			IsAmmoLow( void ) const;
	bool			IsAmmoFull( void ) const;

	bool			AreAllPointsUncontestedSoFar( void ) const;
	bool			IsNearPoint( CTeamControlPoint *point ) const;
	void			ClearMyControlPoint( void ) { m_hMyControlPoint = nullptr; }
	CTeamControlPoint *GetMyControlPoint( void );
	bool			IsAnyPointBeingCaptured( void ) const;
	bool			IsPointBeingContested( CTeamControlPoint *point ) const;
	float			GetTimeLeftToCapture( void );
	bool			HasPointRecentlyChanged( void ) const;
	CTeamControlPoint *SelectPointToCapture( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectPointToDefend( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectClosestPointByTravelDistance( const CUtlVector<CTeamControlPoint *> &candidates ) const;

	CCaptureZone*	GetFlagCaptureZone( void );
	CCaptureFlag*	GetFlagToFetch( void );
	void			SetFlagTarget( CCaptureFlag *pFlag );
	CCaptureFlag*	GetFlagTarget( void ) const;

	float			TransientlyConsistentRandomValue( float duration, int seed ) const;

	Action<CTFBot> *OpportunisticallyUseWeaponAbilities( void );

	void			DisguiseAsEnemy( void );
	CBaseObject*	GetNearestKnownSappableTarget( void ) const;

	void			SelectReachableObjects( CUtlVector<EHANDLE> const& append, CUtlVector<EHANDLE> *outVector, INextBotFilter const& func, CNavArea *pStartArea, float flMaxRange );
	CTFPlayer *		SelectRandomReachableEnemy( void );

	void			StartIdleSound( void );
	void			StopIdleSound( void );

	void			ModifyMaxHealth( int nNewhealth, bool bSetHealth = true, bool bScaleModel = true );

	bool			CanChangeClass( void );
	const char*		GetNextSpawnClassname( void );

	void			WantsToLookForEnemies( void );
	bool			IsLookingForEnemies( void ) const;
	void			StopLookingForEnemies( void );
	void			UpdateLookingAroundForEnemies( void );
	void			UpdateLookingForIncomingEnemies( bool );

	bool			EquipBestWeaponForThreat( const CKnownEntity *threat );
	bool			EquipLongRangeWeapon( void );

	void			PushRequiredWeapon( CTFWeaponBase *weapon );
	bool			EquipRequiredWeapon( void );
	void			PopRequiredWeapon( void );

	CTFBotSquad*	GetSquad( void ) const { return m_pSquad; }
	bool			IsSquadmate( CTFPlayer *player ) const;
	void			JoinSquad( CTFBotSquad *squad );
	void			LeaveSquad();
	void			SetSquadFormationError( float flError ) { m_flFormationError = flError; }
	void			SetIsInFormation( bool bSet ) { m_bIsInFormation = bSet; }

	struct DelayedNoticeInfo
	{
		CHandle<CBaseEntity> m_hEnt;
		float m_flWhen;
	};
	void			DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay );
	void			UpdateDelayedThreatNotices( void );

	struct SuspectedSpyInfo
	{
		CHandle<CTFPlayer> m_hSpy;
		CUtlVector<int> m_times;

		void Suspect()
		{
			this->m_times.AddToHead( (int)floor( gpGlobals->curtime ) );
		}

		bool IsCurrentlySuspected()
		{
			if ( this->m_times.IsEmpty() )
			{
				return false;
			}

			extern ConVar tf_bot_suspect_spy_forget_cooldown;
			return ( (float)this->m_times.Head() > ( gpGlobals->curtime - tf_bot_suspect_spy_forget_cooldown.GetFloat() ) );
		}

		bool TestForRealizing()
		{
			extern ConVar tf_bot_suspect_spy_touch_interval;
			int nCurTime = (int)floor( gpGlobals->curtime );
			int nMinTime = nCurTime - tf_bot_suspect_spy_touch_interval.GetInt();

			FOR_EACH_VEC_BACK(m_times, i)
			{
				if ( m_times[i] <= nMinTime )
					m_times.Remove( i );
			}

			m_times.AddToHead( nCurTime );

			CUtlVector<bool> checks;
			checks.AddMultipleToTail( tf_bot_suspect_spy_touch_interval.GetInt() );

			FOR_EACH_VEC(m_times, i)
			{
				int idx = nCurTime - m_times[i];
				if ( checks.IsValidIndex( idx ) )
					checks[idx] = true;
			}

			int nIndex = checks.Find( false );
			if ( nIndex != checks.InvalidIndex() )
				return false;

			return true;
		}
	};
	SuspectedSpyInfo *IsSuspectedSpy( CTFPlayer *spy );
	void			SuspectSpy( CTFPlayer *spy );
	void			StopSuspectingSpy( CTFPlayer *spy );
	bool			IsKnownSpy( CTFPlayer *spy ) const;
	void			RealizeSpy( CTFPlayer *spy );
	void			ForgetSpy( CTFPlayer *spy );

	struct SniperSpotInfo
	{
		CTFNavArea *m_pVantageArea;
		Vector m_vecVantage;
		CTFNavArea *m_pForwardArea;
		Vector m_vecForward;
		float m_flRange;
		float m_flIncursionDiff;
	};
	void			AccumulateSniperSpots( void );
	void			SetupSniperSpotAccumulation( void );
	void			ClearSniperSpots( void );
	CUtlVector<SniperSpotInfo> const &GetSniperSpots( void );

	float			GetUberDeployDelayDuration( void ) const;
	float			GetUberHealthThreshold( void ) const;

	void			NoteTargetSentry( EHANDLE hSentry );
	ObjectHandle	GetTargetSentry( void ) const;
	Vector const&	GetLastKnownSentryLocation( void ) const;

	CTFNavArea *m_HomeArea;

	enum class DifficultyType
	{
		EASY   = 0,
		NORMAL = 1,
		HARD   = 2,
		EXPERT = 3,
		MAX
	}
	m_iSkill;
	DifficultyType	GetDifficulty( void ) const;
	void			SetDifficulty( DifficultyType difficulty );
	bool			IsDifficulty( DifficultyType skill ) const;

	enum class AttributeType
	{
		NONE                    = 0,

		REMOVEONDEATH           = (1 << 0),
		AGGRESSIVE              = (1 << 1),
		DONTLOOKAROUND			= (1 << 2),
		SUPPRESSFIRE            = (1 << 3),
		DISABLEDODGE            = (1 << 4),
		BECOMESPECTATORONDEATH  = (1 << 5),
		QUOTAMANAGED			= (1 << 6),
		RETAINBUILDINGS         = (1 << 7),
		SPAWNWITHFULLCHARGE     = (1 << 8),
		ALWAYSCRIT              = (1 << 9),
		IGNOREENEMIES           = (1 << 10),
		HOLDFIREUNTILFULLRELOAD = (1 << 11),
		PRIORITIZEDEFENSE		= (1 << 12),
		ALWAYSFIREWEAPON        = (1 << 13),
		TELEPORTTOHINT          = (1 << 14),
		MINIBOSS                = (1 << 15),
		USEBOSSHEALTHBAR        = (1 << 16),
		IGNOREFLAG              = (1 << 17),
		AUTOJUMP                = (1 << 18),
		AIRCHARGEONLY           = (1 << 19),
		VACCINATORBULLETS       = (1 << 20),
		VACCINATORBLAST         = (1 << 21),
		VACCINATORFIRE          = (1 << 22),
		BULLETIMMUNE            = (1 << 23),
		BLASTIMMUNE             = (1 << 24),
		FIREIMMUNE              = (1 << 25),
	}
	m_nBotAttributes;
	void			SetAttribute( AttributeType attribute );
	bool			HasAttribute( AttributeType attribute ) const;
	void			ClearAttribute( AttributeType attribute );
	void			ClearAllAttributes( void );

	enum class WeaponRestrictionType
	{
		UNRESTRICTED	= 0,

		MELEEONLY		= ( 1 << 0 ),
		PRIMARYONLY		= ( 1 << 1 ),
		SECONDARYONLY	= ( 1 << 2 )
	}
	m_nWeaponRestrictions;
	void			SetWeaponRestriction( WeaponRestrictionType restrictionFlags );
	bool			HasWeaponRestriction( WeaponRestrictionType restrictionFlags ) const;
	bool			IsWeaponRestricted( CTFWeaponBase *weapon ) const;
	void			ClearWeaponRestrictions( void );

	enum class MissionType
	{
		NONE = 0,
		SEEK_AND_DESTROY,
		DESTROY_SENTRIES,
		SNIPER,
		SPY,
		ENGINEER,
		REPROGRAMMED
	}
	m_eMission;
	void			SetMission( MissionType mission, bool bResetBehavior = true );
	void			SetPrevMission( MissionType mission );
	MissionType		GetMission( void ) const;
	MissionType		GetPrevMission( void ) const;
	bool			HasMission( MissionType mission ) const;
	bool			IsOnAnyMission( void ) const;
	void			SetMissionTarget( CBaseEntity *target );
	CBaseEntity		*GetMissionTarget( void ) const;
	void			SetMissionString( CUtlString string );
	CUtlString		*GetMissionString( void );

	bool			IsMiniBoss( void ) const;
	void			SetIsMiniBoss( bool bSet );

	bool			ShouldUseBossHealthBar( void ) const;
	void			SetUseBossHealthBar( bool bSet );

	void			SetAutoJumpIntervals( float flMin, float flMax );

	void			SetBehaviorFlag( unsigned int flags );
	void			ClearBehaviorFlag( unsigned int flags );
	bool			IsBehaviorFlagSet( unsigned int flags ) const;

	void			ClearTags( void );
	void			AddTag( char const *tag );
	void			RemoveTag( char const *tag );
	bool			HasTag( char const *tag );

	struct EventChangeAttributes_t
	{
		struct item_attributes_t
		{
			CUtlString m_strItemName;
			CCopyableUtlVector<static_attrib_t> m_Attrs;
		};


		EventChangeAttributes_t( const char *name = "default" )
		{
			this->Reset( name );
		}

		void Reset( const char *name = "default" )
		{
			this->m_strName = name;

			this->m_iSkill          = DifficultyType::EASY;
			this->m_nWeaponRestrict = WeaponRestrictionType::UNRESTRICTED;
			this->m_nMission		= MissionType::NONE;
			this->m_prevMission		= MissionType::NONE;
			this->m_nBotAttrs       = AttributeType::NONE;
			this->m_flVisionRange   = -1.0f;

			this->m_ItemNames.RemoveAll();
			this->m_ItemAttrs.RemoveAll();
			this->m_CharAttrs.RemoveAll();
			this->m_Tags.RemoveAll();
		}

		CUtlString m_strName;
		DifficultyType m_iSkill;
		WeaponRestrictionType m_nWeaponRestrict;
		MissionType m_nMission;
		MissionType m_prevMission;
		AttributeType m_nBotAttrs;
		float m_flVisionRange;
		CUtlStringList m_ItemNames;
		CUtlVector<item_attributes_t> m_ItemAttrs;
		CUtlVector<static_attrib_t> m_CharAttrs;
		CUtlStringList m_Tags;
	};
	void ClearEventChangeAttributes();
	void AddEventChangeAttributes( const EventChangeAttributes_t *newEvent );
	const EventChangeAttributes_t *GetEventChangeAttributes( const char *pszEventName ) const;
	void OnEventChangeAttributes( const CTFBot::EventChangeAttributes_t *pEvent );

	float m_flModelScaleOverride;
	CUtlStringList m_TeleportWhere;
	

private:
	void ManageRandomWeapons( void );

	bool m_bWantsToChangeClass;

	bool m_bLookingAroundForEnemies;
	CountdownTimer m_lookForEnemiesTimer;

	CTFPlayer *m_controlling;

	ILocomotion *m_locomotor;
	IBody *m_body;
	IVision *m_vision;

	friend class CTFBotSquad;
	CTFBotSquad *m_pSquad;
	float m_flFormationError;
	bool m_bIsInFormation;

	unsigned int m_behaviorFlags;
	CUtlVector< CFmtStr > m_tags;
	bool m_bIsMiniBoss;
	bool m_bUseHealthBar;

	float m_flAutoJumpMin;
	float m_flAutoJumpMax;
	CountdownTimer m_autoJumpTimer;

	MissionType m_prevMission;
	CHandle< CBaseEntity > m_hMissionTarget;
	CUtlString m_szMission;

	CHandle<CBaseObject> m_hTargetSentry;
	Vector m_vecLastNoticedSentry;

	CHandle<CTeamControlPoint> m_hMyControlPoint;
	CountdownTimer m_myCPValidDuration;
	CountdownTimer m_cpChangedTimer;

	CHandle<CCaptureZone> m_hMyCaptureZone;
	CHandle<CCaptureFlag> m_hMyCaptureFlag;

	CountdownTimer m_useWeaponAbilityTimer;

	CUtlStack< CHandle<CTFWeaponBase> > m_requiredEquipStack;

	CUtlVector<DelayedNoticeInfo> m_delayedThreatNotices;

	CUtlVectorAutoPurge<SuspectedSpyInfo *> m_suspectedSpies;
	CUtlVector< CHandle<CTFPlayer> > m_knownSpies;

	CUtlVector<SniperSpotInfo> m_sniperSpots;
	CUtlVector<CTFNavArea *> m_sniperStandAreas;
	CUtlVector<CTFNavArea *> m_sniperLookAreas;
	CBaseEntity *m_sniperGoalEnt;
	Vector m_sniperGoal;
	CountdownTimer m_sniperSpotTimer;

	CSoundPatch *m_pIdleSound;

	CUtlVector<const EventChangeAttributes_t *> m_EventChangeAttributes;
};

class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost( CTFBot *actor, RouteType routeType = DEFAULT_ROUTE );
	virtual ~CTFBotPathCost() { }

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const OVERRIDE;

private:
	CTFBot *m_Actor;
	RouteType m_iRouteType;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};

DEFINE_ENUM_BITWISE_OPERATORS( CTFBot::AttributeType )
DEFINE_ENUM_BITWISE_OPERATORS( CTFBot::WeaponRestrictionType )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CTFBot::DifficultyType CTFBot::GetDifficulty( void ) const
{
	return m_iSkill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetDifficulty( CTFBot::DifficultyType difficulty )
{
	m_iSkill = difficulty;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::IsDifficulty( CTFBot::DifficultyType skill ) const
{
	return m_iSkill == skill;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetWeaponRestriction( CTFBot::WeaponRestrictionType restrictionFlags )
{
	m_nWeaponRestrictions |= restrictionFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::HasWeaponRestriction( CTFBot::WeaponRestrictionType restrictionFlags ) const
{
	return (m_nWeaponRestrictions & restrictionFlags) != (CTFBot::WeaponRestrictionType)0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::ClearWeaponRestrictions( void )
{
	m_nWeaponRestrictions = CTFBot::WeaponRestrictionType::UNRESTRICTED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetAttribute( CTFBot::AttributeType attribute )
{
	m_nBotAttributes |= attribute;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::ClearAttribute( CTFBot::AttributeType attribute )
{
	m_nBotAttributes &= ~attribute;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::HasAttribute( CTFBot::AttributeType attribute ) const
{
	return (m_nBotAttributes & attribute) != (CTFBot::AttributeType)0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::ClearAllAttributes()
{
	m_nBotAttributes = CTFBot::AttributeType::NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::ClearTags( void )
{
	m_tags.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::AddTag( const char *tag )
{
	if ( !HasTag( tag ) )
	{
		m_tags.AddToTail( CFmtStr( "%s", tag ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::RemoveTag( const char *tag )
{
	for ( int i=0; i < m_tags.Count(); ++i )
	{
		if ( FStrEq( tag, m_tags[i] ) )
		{
			m_tags.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::HasTag( const char *tag )
{
	for ( int i=0; i < m_tags.Count(); ++i )
	{
		if ( FStrEq( tag, m_tags[i] ) )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CCaptureFlag *CTFBot::GetFlagTarget( void ) const
{
	return m_hMyCaptureFlag;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::WantsToLookForEnemies( void )
{
	m_bLookingAroundForEnemies = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::IsLookingForEnemies( void ) const
{
	return m_bLookingAroundForEnemies;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::StopLookingForEnemies( void )
{
	m_bLookingAroundForEnemies = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::PushRequiredWeapon( CTFWeaponBase *pWeapon )
{
	m_requiredEquipStack.Push( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::PopRequiredWeapon( void )
{
	m_requiredEquipStack.Pop();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::EquipRequiredWeapon( void )
{
	if ( m_requiredEquipStack.Count() <= 0 )
		return false;
	return Weapon_Switch( m_requiredEquipStack.Top() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::NoteTargetSentry( EHANDLE hSentry )
{
	m_hTargetSentry = hSentry;
	m_vecLastNoticedSentry = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline ObjectHandle CTFBot::GetTargetSentry( void ) const
{
	return m_hTargetSentry;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline Vector const &CTFBot::GetLastKnownSentryLocation( void ) const
{
	return m_vecLastNoticedSentry;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CUtlVector<CTFBot::SniperSpotInfo> const &CTFBot::GetSniperSpots( void )
{
	return m_sniperSpots;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::HasPointRecentlyChanged( void ) const
{
	return m_cpChangedTimer.HasStarted() && !m_cpChangedTimer.IsElapsed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetMissionString( CUtlString string )
{
	m_szMission = string;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CUtlString *CTFBot::GetMissionString( void )
{
	return &m_szMission;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetMissionTarget( CBaseEntity *target )
{
	m_hMissionTarget = target;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CBaseEntity *CTFBot::GetMissionTarget( void ) const
{
	return m_hMissionTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CTFBot::MissionType CTFBot::GetMission( void ) const
{
	return m_eMission;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetPrevMission( MissionType mission )
{
	m_prevMission = mission;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CTFBot::MissionType CTFBot::GetPrevMission( void ) const
{
	return m_prevMission;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::HasMission( MissionType mission ) const
{
	return m_eMission == mission;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::IsOnAnyMission( void ) const
{
	return m_eMission != MissionType::NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::IsMiniBoss( void ) const
{
	return m_bIsMiniBoss;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetIsMiniBoss( bool bSet )
{
	m_bIsMiniBoss = bSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::ShouldUseBossHealthBar( void ) const
{
	return m_bUseHealthBar;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetUseBossHealthBar( bool bSet )
{
	m_bUseHealthBar = bSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetAutoJumpIntervals( float flMin, float flMax )
{
	m_flAutoJumpMin = flMin;
	m_flAutoJumpMax = flMax;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::SetBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags |= flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CTFBot::ClearBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags &= ~flags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CTFBot::IsBehaviorFlagSet( unsigned int flags ) const
{
	return ( m_behaviorFlags & flags ) ? true : false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CTFBot *ToTFBot( CBaseEntity *ent )
{
	CTFPlayer *player = ToTFPlayer( ent );
	if ( player == nullptr )
		return NULL;

	if ( !player->IsBotOfType( TF_BOT_TYPE ) )
		return NULL;

	Assert( dynamic_cast<CTFBot *>( ent ) );
	return static_cast<CTFBot *>( ent );
}


class CTFBotItemSchema : public CAutoGameSystem
{
	DECLARE_CLASS_GAMEROOT( CTFBotItemSchema, CAutoGameSystem );
public:
	CTFBotItemSchema( char const *name )
		: CAutoGameSystem( name )
	{
		m_pSchema = NULL;
	}

	virtual void PostInit();
	virtual void Shutdown();

	virtual void LevelInitPreEntity()		{ PostInit(); }
	virtual void LevelShutdownPostEntity()  { Shutdown(); }

	float		 GetItemChance( char const *pszItemDefIndex, char const *pszChanceName, char const *pszClassName = NULL );
	float		 GetItemSetChance( char const *pszItemSetName );

	KeyValues *operator->() { return m_pSchema; }
private:
	KeyValues *m_pSchema;
};

extern CTFBotItemSchema &TFBotItemSchema( void );

#endif
