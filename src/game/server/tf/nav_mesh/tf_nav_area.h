#ifndef __TF_NAV_AREA_H__
#define __TF_NAV_AREA_H__

#include "nav_area.h"

enum TFNavAttributeType
{
	TF_NAV_BLOCKED                     = 0x00000001,

	TF_NAV_RED_SPAWN_ROOM              = 0x00000002,
	TF_NAV_BLUE_SPAWN_ROOM             = 0x00000004,
	TF_NAV_SPAWN_ROOM_EXIT             = 0x00000008,

	TF_NAV_AMMO                        = 0x00000010,
	TF_NAV_HEALTH                      = 0x00000020,

	TF_NAV_CONTROL_POINT               = 0x00000040,

	TF_NAV_BLUE_SENTRY                 = 0x00000080,
	TF_NAV_RED_SENTRY                  = 0x00000100,

	/* bit  9: unused */
	/* bit 10: unused */

	TF_NAV_BLUE_SETUP_GATE             = 0x00000800,
	TF_NAV_RED_SETUP_GATE              = 0x00001000,

	TF_NAV_BLOCKED_AFTER_POINT_CAPTURE = 0x00002000,
	TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE = 0x00004000,

	TF_NAV_BLUE_ONE_WAY_DOOR           = 0x00008000,
	TF_NAV_RED_ONE_WAY_DOOR            = 0x00010000,

	TF_NAV_WITH_SECOND_POINT           = 0x00020000,
	TF_NAV_WITH_THIRD_POINT            = 0x00040000,
	TF_NAV_WITH_FOURTH_POINT           = 0x00080000,
	TF_NAV_WITH_FIFTH_POINT            = 0x00100000,

	TF_NAV_SNIPER_SPOT                 = 0x00200000,
	TF_NAV_SENTRY_SPOT                 = 0x00400000,

	/* bit 23: unused */
	/* bit 24: unused */

	TF_NAV_NO_SPAWNING                 = 0x02000000,
	TF_NAV_RESCUE_CLOSET               = 0x04000000,
	TF_NAV_BOMB_DROP                   = 0x08000000,
	TF_NAV_DOOR_NEVER_BLOCKS           = 0x10000000,
	TF_NAV_DOOR_ALWAYS_BLOCKS          = 0x20000000,
	TF_NAV_UNBLOCKABLE                 = 0x40000000,

	/* bit 31: unused */
};
DEFINE_ENUM_BITWISE_OPERATORS( TFNavAttributeType );


class CTFNavArea : public CNavArea
{
public:
	CTFNavArea();
	virtual ~CTFNavArea();

	virtual void OnServerActivate() OVERRIDE;
	virtual void OnRoundRestart() OVERRIDE;

	virtual void Save( CUtlBuffer &fileBuffer, unsigned int version ) const OVERRIDE;
	virtual NavErrorType Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion ) OVERRIDE;

	virtual void UpdateBlocked( bool force = false, int teamID = TEAM_ANY ) OVERRIDE;
	virtual bool IsBlocked( int teamID, bool ignoreNavBlockers = false ) const OVERRIDE;

	virtual void Draw() const OVERRIDE;

	virtual void CustomAnalysis( bool isIncremental = false ) OVERRIDE;

	virtual bool IsPotentiallyVisibleToTeam( int iTeamNum ) const OVERRIDE
	{
		Assert( iTeamNum > -1 && iTeamNum < TF_TEAM_COUNT );
		return !m_PVActors[ iTeamNum ].IsEmpty();
	}

	void CollectNextIncursionAreas( int iTeamNum, CUtlVector<CTFNavArea *> *areas );
	void CollectPriorIncursionAreas( int iTeamNum, CUtlVector<CTFNavArea *> *areas );
	CTFNavArea *GetNextIncursionArea( int iTeamNum ) const;

	void ComputeInvasionAreaVectors();
	bool IsAwayFromInvasionAreas( int iTeamNum, float radius ) const;
	const CUtlVector<CTFNavArea *> &GetInvasionAreasForTeam( int iTeamNum ) const
	{
		Assert( iTeamNum > -1 && iTeamNum < TF_TEAM_COUNT );
		return m_InvasionAreas[ iTeamNum ];
	}

	void AddPotentiallyVisibleActor( CBaseCombatCharacter *actor );
	void RemovePotentiallyVisibleActor( CBaseCombatCharacter *actor )
	{
		CHandle<CBaseCombatCharacter> hActor( actor );
		for( int i=0; i<TF_TEAM_COUNT; ++i )
			m_PVActors[i].FindAndFastRemove( hActor );
	}

	float GetCombatIntensity() const;
	bool IsInCombat() const;
	void OnCombat();

	static void ResetTFMarker()
	{
		m_masterTFMark = 1;
	}
	static void MakeNewTFMarker()
	{
		++m_masterTFMark;
	}
	bool IsTFMarked() const
	{
		return m_TFMarker == m_masterTFMark;
	}
	void TFMark()
	{
		m_TFMarker = m_masterTFMark;
	}

	bool IsValidForWanderingPopulation() const
	{
		return ( m_nAttributes & ( TF_NAV_BLOCKED|TF_NAV_RESCUE_CLOSET|TF_NAV_BLUE_SPAWN_ROOM|TF_NAV_RED_SPAWN_ROOM|TF_NAV_NO_SPAWNING ) ) == 0;
	}

	void SetIncursionDistance( int iTeamNum, float distance )
	{
		Assert( iTeamNum > -1 && iTeamNum < TF_TEAM_COUNT );
		m_aIncursionDistances[ iTeamNum ] = distance;
	}
	float GetIncursionDistance( int iTeamNum ) const
	{
		Assert( iTeamNum > -1 && iTeamNum < TF_TEAM_COUNT );
		return m_aIncursionDistances[ iTeamNum ];
	}

	void AddTFAttributes( int bits ) {
		m_nAttributes |= bits;
	}
	int GetTFAttributes( void ) const {
		return m_nAttributes;
	}
	bool HasTFAttributes( int bits ) const {
		return ( m_nAttributes & bits ) != 0;
	}
	void RemoveTFAttributes( int bits ) {
		m_nAttributes &= ~bits;
	}

	void SetBombTargetDistance( float distance ) {
		m_flBombTargetDistance = distance;
	}
	float GetBombTargetDistance( void ) const {
		return m_flBombTargetDistance;
	}

	int GetSearchMarker( void ) const {
		return m_TFSearchMarker;
	}
	void SetSearchMarker( int mark ) {
		m_TFSearchMarker = mark;
	}

private:
	float m_aIncursionDistances[ TF_TEAM_COUNT ];
	CUtlVector<CTFNavArea *> m_InvasionAreas[ TF_TEAM_COUNT ];
	int m_TFSearchMarker;

	int m_nAttributes;

	CUtlVector< CHandle<CBaseCombatCharacter> > m_PVActors[ TF_TEAM_COUNT ];

	float m_fCombatIntensity;
	IntervalTimer m_combatTimer;

	float m_flBombTargetDistance;

	static int m_masterTFMark;
	int m_TFMarker;
};

#endif