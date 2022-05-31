#include "cbase.h"
#include "fmtstr.h"
#include "utlbuffer.h"
#include "tier0/vprof.h"
#include "functorutils.h"
#include "triggers.h"
#include "filters.h"

#include "tf_nav_mesh.h"

#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "func_capture_zone.h"

#include "NextBotUtil.h"

ConVar tf_show_in_combat_areas( "tf_show_in_combat_areas", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_mesh_decoration( "tf_show_mesh_decoration", "0", FCVAR_CHEAT, "Highlight special areas", true, 0.0f, true, 1.0f );
ConVar tf_show_mesh_decoration_manual( "tf_show_mesh_decoration_manual", "0", FCVAR_CHEAT, "Highlight special areas marked by hand", true, 0.0f, true, 1.0f );
ConVar tf_show_enemy_invasion_areas( "tf_show_enemy_invasion_areas", "0", FCVAR_CHEAT, "Highlight areas where the enemy team enters the visible environment of the local player", true, 0.0f, true, 1.0f );
ConVar tf_show_blocked_areas( "tf_show_blocked_areas", "0", FCVAR_CHEAT, "Highlight areas that are considered blocked for TF-specific reasons", true, 0.0f, true, 1.0f );
ConVar tf_show_sentry_danger( "tf_show_sentry_danger", "0", FCVAR_CHEAT, "Show sentry danger areas. 1:Use m_sentryAreas. 2:Check all nav areas.", true, 0.0f, true, 2.0f );
ConVar tf_show_incursion_flow( "tf_show_incursion_flow", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_incursion_flow_range( "tf_show_incursion_flow_range", "150", FCVAR_CHEAT, "" );
ConVar tf_show_incursion_flow_gradient( "tf_show_incursion_flow_gradient", "0", FCVAR_CHEAT, "1 = red, 2 = blue", true, 0.0f, true, 2.0f );
ConVar tf_show_bomb_drop_areas( "tf_show_bomb_drop_areas", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_control_points( "tf_show_control_points", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );
ConVar tf_show_actor_potential_visibility( "tf_show_actor_potential_visibility", "0", FCVAR_CHEAT, "", true, 0.0f, true, 1.0f );

ConVar tf_bot_min_setup_gate_defend_range( "tf_bot_min_setup_gate_defend_range", "750", FCVAR_CHEAT, "How close from the setup gate(s) defending bots can take up positions. Areas closer than this will be in cover to ambush." );
ConVar tf_bot_max_setup_gate_defend_range( "tf_bot_max_setup_gate_defend_range", "2000", FCVAR_CHEAT, "How far from the setup gate(s) defending bots can take up positions" );

ConVar tf_select_ambush_areas_radius( "tf_select_ambush_areas_radius", "750", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_close_range( "tf_select_ambush_areas_close_range", "300", FCVAR_CHEAT );
ConVar tf_select_ambush_areas_max_enemy_exposure_area( "tf_select_ambush_areas_max_enemy_exposure_area", "500000", FCVAR_CHEAT );

#define TF_PERSISTENT_ATTRIBUTES    (TF_NAV_BLUE_SETUP_GATE|TF_NAV_RED_SETUP_GATE|TF_NAV_BLOCKED_AFTER_POINT_CAPTURE|TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE|TF_NAV_BLUE_ONE_WAY_DOOR|TF_NAV_RED_ONE_WAY_DOOR|TF_NAV_WITH_SECOND_POINT|TF_NAV_WITH_THIRD_POINT|TF_NAV_WITH_FOURTH_POINT|TF_NAV_WITH_FIFTH_POINT|TF_NAV_SNIPER_SPOT|TF_NAV_SENTRY_SPOT|TF_NAV_NO_SPAWNING|TF_NAV_RESCUE_CLOSET|TF_NAV_DOOR_NEVER_BLOCKS|TF_NAV_DOOR_ALWAYS_BLOCKS|TF_NAV_UNBLOCKABLE)


void TestAndBlockOverlappingAreas( CBaseEntity *pBlocker )
{
	NextBotTraceFilterIgnoreActors filter( pBlocker, COLLISION_GROUP_NONE );

	Extent blockerExtent;
	blockerExtent.Init( pBlocker );

	CUtlVector<CNavArea *> potentiallyBlockedAreas;
	TheNavMesh->CollectAreasOverlappingExtent( blockerExtent, &potentiallyBlockedAreas );

	FOR_EACH_VEC( potentiallyBlockedAreas, i )
	{
		CNavArea *area = potentiallyBlockedAreas[i];

		Vector const nwCorner = area->GetCorner( NORTH_WEST );
		Vector const neCorner = area->GetCorner( NORTH_EAST );
		Vector const swCorner = area->GetCorner( SOUTH_WEST );
		Vector const seCorner = area->GetCorner( SOUTH_EAST );

		Vector vecMaxs(0, 0, HalfHumanHeight), vecMins(0, 0, StepHeight), vecTest;
		if ( fabs( nwCorner.z - neCorner.z ) >= 1.0f )
		{
			if ( fabs( seCorner.z - swCorner.z ) >= 1.0f )
			{
				vecTest = seCorner;
				vecMaxs.x = 1.0f;
				vecMaxs.y = 1.0f;
			}
			else
			{
				vecTest = neCorner;
				vecMaxs.x = 0.0f;
				vecMaxs.y = area->GetSizeY();
			}
		}
		else
		{
			vecTest = swCorner;
			vecMaxs.x = area->GetSizeX();
			vecMaxs.y = 0.0f;
		}

		Vector const vecStart = (nwCorner.z >= vecTest.z) ? nwCorner : vecTest;
		Vector const vecEnd = (nwCorner.z >= vecTest.z) ? vecTest : nwCorner;

		trace_t tr;
		UTIL_TraceHull( vecStart, vecEnd, vecMins, vecMaxs, MASK_PLAYERSOLID, &filter, &tr );

		if ( tr.DidHit() )
		{
			if ( tr.m_pEnt && !tr.m_pEnt->ShouldBlockNav() )
				continue;

			area->MarkAsBlocked( TEAM_ANY, pBlocker );
		}
	}
}


class CollectAndLabelSpawnRooms
{
public:
	CollectAndLabelSpawnRooms( CFuncRespawnRoom *respawnRoom, int teamNum, CUtlVector<CTFNavArea *> *vector )
		: m_vector( vector )
	{
		m_respawn = respawnRoom;
		m_team = teamNum;
	};

	inline bool operator()( CNavArea *a )
	{
		CTFNavArea *area = (CTFNavArea *)a;
		if ( m_respawn == nullptr )
			return true;

		const int nIndex = m_vector->Find( area );
		if ( nIndex != m_vector->InvalidIndex() )
			return true;

		Vector const center = area->GetCenter() + Vector( 0, 0, StepHeight );
		Vector const nwCorner = area->GetCorner( NORTH_WEST ) + Vector( 0, 0, StepHeight );
		Vector const neCorner = area->GetCorner( NORTH_EAST ) + Vector( 0, 0, StepHeight );
		Vector const swCorner = area->GetCorner( SOUTH_WEST ) + Vector( 0, 0, StepHeight );
		Vector const seCorner = area->GetCorner( SOUTH_EAST ) + Vector( 0, 0, StepHeight );

		if ( m_respawn->PointIsWithin( center ) ||
			 m_respawn->PointIsWithin( nwCorner ) ||
			 m_respawn->PointIsWithin( neCorner ) ||
			 m_respawn->PointIsWithin( swCorner ) ||
			 m_respawn->PointIsWithin( seCorner ) )
		{
			area->AddTFAttributes( m_team == TF_TEAM_RED ? TF_NAV_RED_SPAWN_ROOM : TF_NAV_BLUE_SPAWN_ROOM );
			m_vector->AddToTail( area );
		}

		return true;
	}

private:
	CFuncRespawnRoom *m_respawn;
	int m_team;
	CUtlVector<CTFNavArea *> *m_vector;
};


class ScanSelectAmbushAreas
{
public:
	ScanSelectAmbushAreas(int iTeam, float fIncursion, CTFNavArea *area, CUtlVector<CTFNavArea *> *areas )
		: m_iTeam( iTeam ), m_vector( areas )
	{
		m_flIncursionDistance = fIncursion + area->GetIncursionDistance( iTeam );
	}

	inline bool operator()( CNavArea *a )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( a );
		if ( area->GetParent() && area->GetParent()->IsContiguous( area ) )
		{
			if ( area->GetIncursionDistance( m_iTeam ) <= m_flIncursionDistance )
			{
				NavAreaCollector collector;
				area->ForAllPotentiallyVisibleAreas( collector );

				float flDistance = 0.0f;
				for ( int i=0; i < collector.m_area.Count(); ++i )
				{
					CTFNavArea *other = static_cast<CTFNavArea *>( collector.m_area[i] );

					if ( area->GetIncursionDistance( m_iTeam ) > other->GetIncursionDistance( m_iTeam ) )
						flDistance += other->GetSizeX() * other->GetSizeY();

					if ( flDistance <= tf_select_ambush_areas_max_enemy_exposure_area.GetFloat() )
					{
						if ( area->GetIncursionDistance( m_iTeam ) > other->GetIncursionDistance( m_iTeam ) && 
							( area->GetCenter() - other->GetCenter() ).LengthSqr() <= Square( tf_select_ambush_areas_close_range.GetFloat() ) )
							continue;

						m_vector->AddToTail( area );
					}
					else
					{
						return false;
					}
				}

				return true;
			}
		}

		return false;
	}

private:
	int m_iTeam;
	float m_flIncursionDistance;
	CUtlVector<CTFNavArea *> *m_vector;
};


class DrawIncursionFlow
{
public:
	inline bool operator()( CNavArea *a )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( a );
		int nTeam = tf_show_incursion_flow.GetInt() + 1;

		for ( int dir = 0; dir < NUM_DIRECTIONS; dir++ )
		{
			for ( int i=0; i < area->GetAdjacentCount( (NavDirType)dir ); ++i )
			{
				CTFNavArea *adjArea = static_cast<CTFNavArea *>( area->GetAdjacentArea( (NavDirType)dir, i ) );
				if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > 45.f )
					continue;

				if ( adjArea->GetIncursionDistance( nTeam ) > area->GetIncursionDistance( nTeam ) )
				{
					float flTimeMod = fmodf( adjArea->GetIncursionDistance( nTeam ) - ( gpGlobals->curtime * 0.3333f * 2500.f ), 2500.f ) * 2 / 2500.f;
					if ( flTimeMod > 1.0f )
						flTimeMod -= 2.0f;

					int r = (nTeam == TF_TEAM_RED) ? 255 * flTimeMod : 0;
					int b = (nTeam == TF_TEAM_BLUE) ? 255 * flTimeMod : 0;
					NDebugOverlay::HorzArrow( area->GetCenter(), adjArea->GetCenter(), 5.0f, r, 0, b, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}

		return true;
	}
};


CTFNavMesh::CTFNavMesh()
{
	for ( int i=0; i < MAX_CONTROL_POINTS; ++i )
	{
		m_CPAreas[i].RemoveAll();
		m_CPArea[i] = NULL;
	}

	ListenForGameEvent( "teamplay_setup_finished" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_point_unlocked" );
	ListenForGameEvent( "player_builtobject" );
	ListenForGameEvent( "player_dropobject" );
	ListenForGameEvent( "player_carryobject" );
	ListenForGameEvent( "object_detonated" );
	ListenForGameEvent( "object_destroyed" );
}

CTFNavMesh::~CTFNavMesh()
{
}

void CTFNavMesh::FireGameEvent( IGameEvent *event )
{
	CUtlString string( event->GetName() );
	CNavMesh::FireGameEvent( event );

	if ( string == "teamplay_point_captured" )
	{
		m_pointChangedIdx = event->GetInt( "cp" );
		m_pointState = CP_STATE_OWNERSHIP_CHANGED;
		m_recomputeTimer.Start( 2.0f );
	}

	if ( string == "teamplay_setup_finished" )
	{
		m_pointChangedIdx = 0;
		m_pointState = CP_STATE_RESET;
		m_recomputeTimer.Start( 2.0f );
	}

	if ( string == "teamplay_point_unlocked" )
	{
		m_pointChangedIdx = event->GetInt( "cp" );
		m_pointState = CP_STATE_AWAITING_CAPTURE;
		m_recomputeTimer.Start( 2.0f );
	}

	if ( string == "player_builtobject" || string == "player_carryobject" || string == "player_dropobject" ||
		 string == "object_detonated" || string == "object_destroyed" )
	{
		int iObjectType = event->IsEmpty( "objecttype" ) ? event->GetInt( "object" ) : event->GetInt( "objecttype" );
		if ( iObjectType == OBJ_SENTRYGUN )
		{
			if ( tf_show_sentry_danger.GetInt() )
				DevMsg( "%s: Got sentrygun %s event\n", __FUNCTION__, string.Get() );

			OnObjectChanged();
		}
	}
}

CNavArea *CTFNavMesh::CreateArea() const
{
	return new CTFNavArea;
}

void CTFNavMesh::Update()
{
	CNavMesh::Update();
	if ( !TheNavAreas.IsEmpty() )
	{
		UpdateDebugDisplay();
		if ( TheNextBots().GetNextBotCount() > 0 )
		{
			if ( !m_lastNPCCount )
			{
				m_recomputeTimer.Start( 2.0f );
			}

			if ( m_recomputeTimer.HasStarted() && m_recomputeTimer.IsElapsed() )
			{
				m_recomputeTimer.Invalidate();
				RecomputeInternalData();
			}
		}
		m_lastNPCCount = TheNextBots().GetNextBotCount();
	}
}

bool CTFNavMesh::IsAuthoritative() const
{
	return true;
}

unsigned int CTFNavMesh::GetSubVersionNumber() const
{
	return 2;
}

void CTFNavMesh::SaveCustomData( CUtlBuffer &fileBuffer ) const
{

}

void CTFNavMesh::LoadCustomData( CUtlBuffer &fileBuffer, unsigned int subVersion )
{

}

void CTFNavMesh::OnServerActivate()
{
	CNavMesh::OnServerActivate();
	ResetMeshAttributes( true );

	m_sentryAreas.RemoveAll();
	m_spawnAreasTeam1.RemoveAll();
	m_spawnAreasTeam2.RemoveAll();
	m_spawnExitsTeam1.RemoveAll();
	m_spawnExitsTeam2.RemoveAll();

	for ( int i=0; i < MAX_CONTROL_POINTS; ++i )
		m_CPAreas[i].RemoveAll();
}

void CTFNavMesh::OnRoundRestart()
{
	CNavMesh::OnRoundRestart();
	ResetMeshAttributes( true );
	TheNextBots().OnRoundRestart();
}

unsigned int CTFNavMesh::GetGenerationTraceMask() const
{
	return MASK_PLAYERSOLID_BRUSHONLY;
}

void CTFNavMesh::PostCustomAnalysis()
{

}

void CTFNavMesh::BeginCustomAnalysis( bool bIncremental )
{

}

void CTFNavMesh::EndCustomAnalysis()
{

}

void CTFNavMesh::CollectAmbushAreas( CUtlVector<CTFNavArea *> *areas, CTFNavArea *startArea, int teamNum, float fMaxDist, float fIncursionDiff ) const
{
	ScanSelectAmbushAreas functor( teamNum, fIncursionDiff, startArea, areas );
	SearchSurroundingAreas( startArea, startArea->GetCenter(), functor, fMaxDist );
}

void CTFNavMesh::CollectBuiltObjects( CUtlVector<CBaseObject *> *objects, int teamNum )
{
	objects->RemoveAll();

	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( obj && ( teamNum == TEAM_ANY || obj->GetTeamNumber() == teamNum ) )
			objects->AddToTail( obj );
	}
}

// this function makes 0 sense
void CTFNavMesh::CollectSpawnRoomThresholdAreas( CUtlVector<CTFNavArea*> *areas, int teamNum ) const
{
	const CUtlVector<CTFNavArea *> &spawnExits = GetSpawnRoomExitsForTeam( teamNum );
	FOR_EACH_VEC( spawnExits, i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			
		CTFNavArea *exitArea = NULL;
		float flMaxAreaSize = 0.0f;

		for( int dir=0; dir<NUM_DIRECTIONS; ++dir )
		{
			for ( int j=0; j<area->GetAdjacentCount( (NavDirType)dir ); ++j )
			{
				CTFNavArea *adjArea = static_cast<CTFNavArea *>( area->GetAdjacentArea( (NavDirType)dir, j ) );

				if ( !adjArea->HasTFAttributes( TF_NAV_RED_SPAWN_ROOM|TF_NAV_BLUE_SPAWN_ROOM|TF_NAV_SPAWN_ROOM_EXIT ) )
				{
					float size = adjArea->GetSizeX() * adjArea->GetSizeY();
					if ( size > flMaxAreaSize )
					{
						exitArea = adjArea;
						flMaxAreaSize = size;
					}
				}
			}
		}

		if ( exitArea )
		{
			areas->AddToTail( exitArea );
		}
	}
}

bool CTFNavMesh::IsSentryGunHere( CTFNavArea *area ) const
{
	FOR_EACH_VEC( IBaseObjectAutoList::AutoList(), i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( obj == nullptr || obj->GetType() != OBJ_SENTRYGUN )
			continue;

		obj->UpdateLastKnownArea();
		if ( obj->GetLastKnownArea() == area )
			return true;
	}

	return false;
}

void CTFNavMesh::OnBlockedAreasChanged()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( TheNextBots().GetNextBotCount() > 0 )
		m_recomputeTimer.Start( 2.0f );
}

void CTFNavMesh::CollectAndMarkSpawnRoomExits( CTFNavArea *area, CUtlVector<CTFNavArea *> *areas )
{
	const int nIndex = areas->Find( area );
	if ( nIndex != areas->InvalidIndex() )
		return;

	for ( int dir=0; dir < NUM_DIRECTIONS; dir++ )
	{
		for ( int i=0; i<area->GetAdjacentCount( (NavDirType)dir ); ++i )
		{
			CTFNavArea *other = static_cast<CTFNavArea *>( area->GetAdjacentArea( (NavDirType)dir, i ) );
			if ( !other->HasTFAttributes( TF_NAV_RED_SPAWN_ROOM|TF_NAV_BLUE_SPAWN_ROOM ) )
			{
				area->AddTFAttributes( TF_NAV_SPAWN_ROOM_EXIT );
				areas->AddToTail( area );

				return;
			}
		}
	}
}

void CTFNavMesh::CollectControlPointAreas()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for ( int i=0; i < MAX_CONTROL_POINTS; ++i )
		m_CPAreas[i].RemoveAll();

	if ( !g_hControlPointMasters.IsEmpty() )
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters[0].Get();
		if ( pMaster )
		{
			FOR_EACH_VEC( ITriggerAreaCaptureAutoList::AutoList(), i )
			{
				CTriggerAreaCapture *pCaptureArea = static_cast<CTriggerAreaCapture *>( ITriggerAreaCaptureAutoList::AutoList()[i] );

				CTeamControlPoint *pPoint = pCaptureArea->GetControlPoint();
				if ( pPoint )
				{
					Extent captureExtent;
					captureExtent.Init( pCaptureArea );

					captureExtent.lo -= 35.5f;
					captureExtent.hi += 35.5f;

					int iIndex = pPoint->GetPointIndex();

					CollectAreasOverlappingExtent( captureExtent, &m_CPAreas[iIndex] );

					float flMinDist = FLT_MAX;
					Vector vOrigin = pCaptureArea->WorldSpaceCenter();
					FOR_EACH_VEC( m_CPAreas[iIndex], j )
					{
						Vector vCenter = m_CPAreas[iIndex][j]->GetCenter();
						float flDistance = ( vCenter - vOrigin ).AsVector2D().LengthSqr();
						if ( flMinDist > flDistance )
						{
							flMinDist = flDistance;
							m_CPArea[iIndex] = m_CPAreas[iIndex][j];
						}
					}
				}
			}
		}
	}
}

void CTFNavMesh::CollectAreasWithinBombTravelRange( CUtlVector<CTFNavArea *> *areas, float minTravel, float maxTravel ) const
{
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		float travelDistance = area->GetBombTargetDistance();
		if ( travelDistance >= minTravel && travelDistance <= maxTravel )
		{
			areas->AddToTail( area );
		}
	}
}

void CTFNavMesh::ComputeBlockedAreas()
{
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CNavArea *area = TheNavAreas[i];
		area->UnblockArea();
	}

	CBaseEntity *pBrush = NULL;
	while ( ( pBrush = gEntList.FindEntityByClassname( pBrush, "func_brush" ) ) != NULL )
	{
		if ( pBrush->IsSolid() )
			TestAndBlockOverlappingAreas( pBrush );
	}

	CBaseToggle *pDoor = NULL;
	while ( ( pDoor = (CBaseToggle *)gEntList.FindEntityByClassname( pDoor, "func_door*" ) ) != NULL )
	{
		Extent doorExent, triggerExtent;
		doorExent.Init( pDoor );

		bool bDoorClosed = pDoor->m_toggle_state == TS_AT_BOTTOM || pDoor->m_toggle_state == TS_GOING_DOWN;

		int iBlockedTeam = TEAM_UNASSIGNED;
		bool bFiltered = false;

		CBaseTrigger *pTrigger = NULL;
		while ( ( pTrigger = (CBaseTrigger *)gEntList.FindEntityByClassname( pTrigger, "trigger_multiple" ) ) != NULL )
		{
			triggerExtent.Init( pTrigger );

			if ( doorExent.IsOverlapping( triggerExtent ) && !pTrigger->m_bDisabled )
			{
				CBaseFilter *pFilter = pTrigger->m_hFilter;
				if ( pFilter && FClassnameIs( pFilter, "filter_activator_tfteam" ) )
					iBlockedTeam = pFilter->GetTeamNumber();
				
				bFiltered = true;
			}
		}

		CUtlVector<CTFNavArea *> potentiallyBlockedAreas;
		CollectAreasOverlappingExtent( doorExent, &potentiallyBlockedAreas );

		int iNavTeam = TEAM_ANY;
		if ( iBlockedTeam > TEAM_UNASSIGNED )
			iNavTeam = ( iBlockedTeam == TF_TEAM_RED ) ? TF_TEAM_BLUE : TF_TEAM_RED;

		for ( int i=0; i<potentiallyBlockedAreas.Count(); ++i )
		{
			CTFNavArea *area = potentiallyBlockedAreas[i];

			bool bDoorBlocks = false;
			if ( area->HasTFAttributes( TF_NAV_DOOR_ALWAYS_BLOCKS ) )
				bDoorBlocks = bDoorClosed;
			else
				bDoorBlocks = ( !bFiltered && bDoorClosed ) || iNavTeam > TEAM_UNASSIGNED;

			if ( bDoorBlocks )
			{
				if ( !area->HasTFAttributes( TF_NAV_DOOR_NEVER_BLOCKS ) )
					area->MarkAsBlocked( iNavTeam, pDoor );
			}
			else
			{
				area->UnblockArea( iNavTeam );
			}
		}
	}
}

void CTFNavMesh::ComputeIncursionDistances()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		for ( int i=0; i < TF_TEAM_COUNT; ++i )
			area->SetIncursionDistance( i, -1.0f );
	}

	bool bFoundRedSpawn = false;
	bool bFoundBluSpawn = false;

	for ( int i=0; i<IFuncRespawnRoomAutoList::AutoList().Count(); ++i )
	{
		CFuncRespawnRoom *pRespawnRoom = static_cast<CFuncRespawnRoom *>( IFuncRespawnRoomAutoList::AutoList()[i] );
		if ( pRespawnRoom->GetActive() && !pRespawnRoom->m_bDisabled )
		{
			for ( int j=0; j<ITFTeamSpawnAutoList::AutoList().Count(); ++j )
			{
				CTFTeamSpawn *pTeamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[j] );
				int iSpawnTeam = pTeamSpawn->GetTeamNumber();

				// Has anyone spawned here yet?
				if ( !pTeamSpawn->IsTriggered( NULL ) || pTeamSpawn->IsDisabled() )
					continue;

				// Have we already found a spawn point for RED?
				if ( iSpawnTeam == TF_TEAM_RED && bFoundRedSpawn )
					continue;

				// Have we already found a spawn point for BLU?
				if ( iSpawnTeam == TF_TEAM_BLUE && bFoundBluSpawn )
					continue;

				// Is it even located in a spawn room? !BUG! this breaks on Arena
				if ( !pRespawnRoom->PointIsWithin( pTeamSpawn->GetAbsOrigin() ) )
					continue;

				CTFNavArea *pArea = static_cast<CTFNavArea *>( GetNearestNavArea( pTeamSpawn, GETNAVAREA_ALLOW_BLOCKED_AREAS|GETNAVAREA_CHECK_GROUND ) );
				if ( pArea )
				{
					ComputeIncursionDistances( pArea, iSpawnTeam );
					if ( iSpawnTeam == TF_TEAM_RED )
						bFoundRedSpawn = true;
					else
						bFoundBluSpawn = true;
				}
			}
		}
	}

	if ( !bFoundRedSpawn )
	{
		Warning(
			"Can't compute incursion distances from the Red spawn room(s). Bots will perform poorly. This is caused by either a"
			" missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}

	if ( !bFoundBluSpawn )
	{
		Warning(
			"Can't compute incursion distances from the Blue spawn room(s). Bots will perform poorly. This is caused by either "
			"a missing func_respawnroom, or missing info_player_teamspawn entities within the func_respawnroom.\n" );
	}

	// Update RED incursion distance based on BLU's distance
	/*float flMaxDistance = 0.0f;
	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		flMaxDistance = Max( flMaxDistance, area->GetIncursionDistance( TF_TEAM_BLUE ) );
	}

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		float flIncursionDist = area->GetIncursionDistance( TF_TEAM_BLUE );
		if ( flIncursionDist >= 0.0f )
			area->SetIncursionDistance( TF_TEAM_RED, ( flMaxDistance - flIncursionDist ) );
	}*/
}

void CTFNavMesh::ComputeIncursionDistances( CTFNavArea *startArea, int teamNum )
{
	VPROF_SCOPE_BEGIN( __FUNCTION__ );
	Assert( teamNum >= 0 && teamNum <= 3 );
	
	if ( startArea )
	{
		startArea->SetIncursionDistance( teamNum, 0.0f );

		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVectorFixedGrowable<const NavConnect *, 64u> adjCons;
		while ( !CNavArea::IsOpenListEmpty() )
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( CNavArea::PopOpenList() );

			adjCons.RemoveAll();

			if ( !TFGameRules()->IsMannVsMachineMode() && !area->HasTFAttributes( TF_NAV_RED_SETUP_GATE|TF_NAV_BLUE_SETUP_GATE|TF_NAV_SPAWN_ROOM_EXIT ) && area->IsBlocked( teamNum ) )
				continue;

			for ( int dir = 0; dir < NUM_DIRECTIONS; ++dir )
			{
				for ( int i=0; i < area->GetAdjacentCount( (NavDirType)dir ); ++i )
				{
					const NavConnect *connection = &( *area->GetAdjacentAreas( (NavDirType)dir ) )[i];
					adjCons.AddToTail( connection );
				}
			}

			if ( adjCons.IsEmpty() )
				continue;

			FOR_EACH_VEC( adjCons, i )
			{
				const NavConnect *connect = adjCons[i];
				CTFNavArea *adjArea = static_cast<CTFNavArea *>( connect->area );

				if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > 45.f )
					continue;

				float flIncursionDist = area->GetIncursionDistance( teamNum ) + connect->length;
				if ( adjArea->GetIncursionDistance( teamNum ) < 0.0f || flIncursionDist < adjArea->GetIncursionDistance( teamNum ) )
				{
					adjArea->SetIncursionDistance( teamNum, flIncursionDist );
					adjArea->Mark();
					adjArea->SetParent( area );

					if ( !adjArea->IsOpen() )
						adjArea->AddToOpenListTail();
				}
			}
		}
	}

	VPROF_SCOPE_END();
}

void CTFNavMesh::ComputeInvasionAreas()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		if ( area ) area->ComputeInvasionAreaVectors();
	}
}

void CTFNavMesh::DecorateMesh()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_spawnAreasTeam1.RemoveAll();
	m_spawnAreasTeam2.RemoveAll();
	m_spawnExitsTeam1.RemoveAll();
	m_spawnExitsTeam2.RemoveAll();

	FOR_EACH_VEC( IFuncRespawnRoomAutoList::AutoList(), i )
	{
		CFuncRespawnRoom *pRespawnRoom = static_cast<CFuncRespawnRoom *>( IFuncRespawnRoomAutoList::AutoList()[i] );
		if ( pRespawnRoom->GetActive() && !pRespawnRoom->m_bDisabled )
		{
			FOR_EACH_VEC( ITFTeamSpawnAutoList::AutoList(), j )
			{
				CTFTeamSpawn *pTeamSpawn = static_cast<CTFTeamSpawn *>( ITFTeamSpawnAutoList::AutoList()[j] );
				// Has anyone spawned here yet?
				if ( !pTeamSpawn->IsTriggered( NULL ) || pTeamSpawn->IsDisabled() )
					continue;
				// Is it even located in a spawn room? !BUG! this breaks on Arena
				if ( !pRespawnRoom->PointIsWithin( pTeamSpawn->GetAbsOrigin() ) )
					continue;
				
				Extent ext;
				ext.Init( pRespawnRoom );

				if ( pTeamSpawn->GetTeamNumber() == TF_TEAM_RED )
				{
					CollectAndLabelSpawnRooms func( pRespawnRoom, TF_TEAM_RED, &m_spawnAreasTeam1 );
					ForAllAreasOverlappingExtent( func, ext );
				}
				else
				{
					CollectAndLabelSpawnRooms func( pRespawnRoom, TF_TEAM_BLUE, &m_spawnAreasTeam2 );
					ForAllAreasOverlappingExtent( func, ext );
				}
			}
		}
	}

	FOR_EACH_VEC( m_spawnAreasTeam1, i )
	{
		CollectAndMarkSpawnRoomExits( m_spawnAreasTeam1[i], &m_spawnExitsTeam1 );
	}

	FOR_EACH_VEC( m_spawnAreasTeam2, i )
	{
		CollectAndMarkSpawnRoomExits( m_spawnAreasTeam2[i], &m_spawnExitsTeam2 );
	}

	for ( CBaseEntity *pEnt = gEntList.FirstEnt(); pEnt; pEnt = gEntList.NextEnt( pEnt ) )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( GetNearestNavArea( pEnt->GetAbsOrigin() ) );
		if ( area )
		{
			if ( FClassnameIs( pEnt, "item_ammopack*" ) )
				area->AddTFAttributes( TF_NAV_AMMO );
			else if ( FClassnameIs( pEnt, "item_healthkit*" ) )
				area->AddTFAttributes( TF_NAV_HEALTH );
		}
	}

	for ( int i=0; i<MAX_CONTROL_POINTS; ++i )
	{
		for ( int j=0; j<m_CPAreas[i].Count(); ++j )
		{
			m_CPAreas[i][j]->AddTFAttributes( TF_NAV_CONTROL_POINT );
		}
	}
}

void CTFNavMesh::ComputeBombTargetDistance()
{
	if ( !TFGameRules()->IsMannVsMachineMode() )
		return;
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CCaptureZone *zone = NULL;
	FOR_EACH_VEC( ICaptureZoneAutoList::AutoList(), i )
	{
		zone = static_cast<CCaptureZone *>( ICaptureZoneAutoList::AutoList()[i] );
		if ( zone->GetTeamNumber() == TF_TEAM_MVM_BOTS )
		{
			break;
		}
	}

	if ( zone )
	{
		CTFNavArea *startArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( zone->WorldSpaceCenter(), false, 500.0f, true );
		if ( startArea )
		{
			FOR_EACH_VEC( TheNavAreas, it )
			{
				CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[it] );
				area->SetBombTargetDistance( -1.0f );
			}

			startArea->SetBombTargetDistance( 0.0f );

			CNavArea::ClearSearchLists();

			startArea->AddToOpenList();
			startArea->SetParent( NULL );
			startArea->Mark();

			CUtlVectorFixedGrowable<const NavConnect *, 64u> adjCons;
			while ( !CNavArea::IsOpenListEmpty() )
			{
				CTFNavArea *area = static_cast<CTFNavArea *>( CNavArea::PopOpenList() );

				adjCons.RemoveAll();

				for ( int dir = 0; dir < NUM_DIRECTIONS; ++dir )
				{
					for ( int i=0; i < area->GetAdjacentCount( (NavDirType)dir ); ++i )
					{
						const NavConnect *connection = &( *area->GetAdjacentAreas( (NavDirType)dir ) )[i];
						adjCons.AddToTail( connection );
					}
				}

				if ( adjCons.IsEmpty() )
					continue;

				FOR_EACH_VEC( adjCons, i )
				{
					const NavConnect *connect = adjCons[i];
					CTFNavArea *adjArea = static_cast<CTFNavArea *>( connect->area );

					if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > 45.f )
						continue;

					float flDistanceToHatch = area->GetBombTargetDistance() + connect->length;
					if ( adjArea->GetBombTargetDistance() < 0.0f || flDistanceToHatch < adjArea->GetBombTargetDistance() )
					{
						adjArea->SetBombTargetDistance( flDistanceToHatch );
						adjArea->Mark();
						adjArea->SetParent( area );

						if ( !adjArea->IsOpen() )
							adjArea->AddToOpenListTail();
					}
					else
					{
						float flShortcutToHatch = adjArea->GetBombTargetDistance() + connect->length;
						if ( flShortcutToHatch < area->GetBombTargetDistance() )
						{
							area->SetBombTargetDistance( flShortcutToHatch );
							area->Mark();
							area->SetParent( adjArea );

							if ( !area->IsOpen() )
								area->AddToOpenListTail();
						}
					}
				}
			}
		}
		else
		{
			Warning( "No nav area for bomb delivery zone." );
		}
	}
	else
	{
		Warning( "Can't find bomb delivery zone." );
	}
}

void CTFNavMesh::ComputeLegalBombDropAreas()
{
	if ( !TFGameRules()->IsMannVsMachineMode() )
		return;
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CTFNavArea *startArea = NULL;
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		area->RemoveTFAttributes( TF_NAV_BOMB_DROP );

		if ( area->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM ) )
			startArea = area;
	}

	if ( startArea )
	{
		CNavArea::ClearSearchLists();

		startArea->AddToOpenList();
		startArea->SetParent( NULL );
		startArea->Mark();

		CUtlVectorFixedGrowable<const NavConnect *, 64u> adjCons;
		while ( !CNavArea::IsOpenListEmpty() )
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( CNavArea::PopOpenList() );

			adjCons.RemoveAll();

			for ( int dir = 0; dir < NUM_DIRECTIONS; ++dir )
			{
				for ( int i=0; i < area->GetAdjacentCount( (NavDirType)dir ); ++i )
				{
					const NavConnect *connection = &( *area->GetAdjacentAreas( (NavDirType)dir ) )[i];
					adjCons.AddToTail( connection );
				}
			}

			if ( adjCons.IsEmpty() )
				continue;

			FOR_EACH_VEC( adjCons, i )
			{
				const NavConnect *connect = adjCons[i];
				CTFNavArea *adjArea = static_cast<CTFNavArea *>( connect->area );

				if ( area->ComputeAdjacentConnectionHeightChange( adjArea ) > StepHeight )
					continue;

				if ( !adjArea->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM|TF_NAV_RED_SPAWN_ROOM ) )
					adjArea->AddTFAttributes( TF_NAV_BOMB_DROP );
				
				adjArea->Mark();
				adjArea->SetParent( area );

				if ( !adjArea->IsOpen() )
					adjArea->AddToOpenListTail();
			}
		}
	}
	else
	{
		Warning( "Can't find blue spawn room nav areas. No legal bomb drop areas are marked" );
	}
}

void CTFNavMesh::OnObjectChanged()
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	ResetMeshAttributes( false );

	CUtlVector<CBaseObject *> sentries;
	for ( int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( obj && obj->ObjectType() == OBJ_SENTRYGUN && !obj->IsDying() && !obj->IsBeingCarried() )
		{
			sentries.AddToTail( obj );
		}
	}

	if ( !sentries.IsEmpty() )
	{
		FOR_EACH_VEC( TheNavAreas, i )
		{
			CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
			FOR_EACH_VEC( sentries, j )
			{
				CBaseObject *obj = sentries[j];
				Vector sentryPos = obj->GetAbsOrigin();
				int team = obj->GetTeamNumber();

				Vector areaPos( 0.0f );
				area->GetClosestPointOnArea( sentryPos, &areaPos );

				if ( areaPos.DistToSqr( sentryPos ) < Square( SENTRYGUN_BASE_RANGE ) &&
					 area->IsPartiallyVisible( obj->GetAbsOrigin() + Vector(0, 0, 30.f), obj ) )
				{
					if ( !area->HasAttributes( TF_NAV_BLUE_SENTRY|TF_NAV_RED_SENTRY ) )
						m_sentryAreas.AddToTail( area );

					area->AddTFAttributes( team == TF_TEAM_RED ? TF_NAV_RED_SENTRY : TF_NAV_BLUE_SENTRY );
				}
			}
		}
	}

	if ( tf_show_sentry_danger.GetInt() )
		DevMsg( "%s: sentries:%d areas count:%d\n", __FUNCTION__, sentries.Count(), m_sentryAreas.Count() );
}

// TODO: Why do they recompute so much so often?
void CTFNavMesh::RecomputeInternalData()
{
	RemoveAllMeshDecoration();
	DecorateMesh();
	CollectControlPointAreas();
	ComputeBlockedAreas();
	ComputeIncursionDistances();
	ComputeInvasionAreas();
	ComputeLegalBombDropAreas();
	ComputeBombTargetDistance();

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		if ( m_pointState <= CP_STATE_RESET )
		{
			if ( area->HasTFAttributes( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
				area->AddTFAttributes( TF_NAV_BLOCKED );
		}

		if ( m_pointState == CP_STATE_OWNERSHIP_CHANGED )
		{
			if ( area->HasTFAttributes( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
			{
				if ( area->HasTFAttributes( TF_NAV_WITH_SECOND_POINT ) && m_pointChangedIdx > 0 )
					area->RemoveTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_THIRD_POINT ) && m_pointChangedIdx > 1 )
					area->RemoveTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_FOURTH_POINT ) && m_pointChangedIdx > 2 )
					area->RemoveTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_FIFTH_POINT ) && m_pointChangedIdx > 3 )
					area->RemoveTFAttributes( TF_NAV_BLOCKED );
			}
			else if ( area->HasTFAttributes( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE ) )
			{
				if ( area->HasTFAttributes( TF_NAV_WITH_SECOND_POINT ) && m_pointChangedIdx > 0 )
					area->AddTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_THIRD_POINT ) && m_pointChangedIdx > 1 )
					area->AddTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_FOURTH_POINT ) && m_pointChangedIdx > 2 )
					area->AddTFAttributes( TF_NAV_BLOCKED );
				else if ( area->HasTFAttributes( TF_NAV_WITH_FIFTH_POINT ) && m_pointChangedIdx > 3 )
					area->AddTFAttributes( TF_NAV_BLOCKED );
			}
		}
	}

	m_recomputeTimer.Invalidate();
}

void CTFNavMesh::RemoveAllMeshDecoration()
{
	for ( int i=0; i < TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		area->RemoveTFAttributes( ~TF_PERSISTENT_ATTRIBUTES );
	}

	m_sentryAreas.RemoveAll();
	OnObjectChanged();
}

void CTFNavMesh::ResetMeshAttributes( bool fullReset )
{
	for ( int i=0; i<m_sentryAreas.Count(); ++i )
		m_sentryAreas[i]->RemoveTFAttributes( TF_NAV_BLUE_SENTRY|TF_NAV_RED_SENTRY );

	m_sentryAreas.RemoveAll();

	if ( fullReset )
	{
		m_recomputeTimer.Start( 2.0f );
		m_pointState = 0;
		m_pointChangedIdx = 0;
	}
}

void CTFNavMesh::UpdateDebugDisplay() const
{
	if ( engine->IsDedicatedServer() )
		return;
	
	CBasePlayer *host = UTIL_GetListenServerHost();
	if ( !host || !host->IsConnected() )
		return;

	if ( tf_show_enemy_invasion_areas.GetBool() )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( host->GetLastKnownArea() );
		if ( area )
		{
			const CUtlVector<CTFNavArea *> &invasionAreas = area->GetInvasionAreasForTeam( host->GetTeamNumber() );
			FOR_EACH_VEC( invasionAreas, i )
			{
				CTFNavArea *invArea = invasionAreas[i];
				invArea->DrawFilled( 255, 0, 0, 255 );
			}
		}
	}

	if ( tf_show_incursion_flow.GetInt() > 0 || tf_show_incursion_flow_gradient.GetInt() > 0 )
	{
		Vector vecFwd;
		host->EyeVectors( &vecFwd );

		Vector const vecStart = host->EyePosition();

		trace_t tr;
		CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_NONE, WALK_THRU_EVERYTHING );
		UTIL_TraceLine( vecStart, vecStart + vecFwd * 2000.f, MASK_NPCSOLID, &filter, &tr );

		CTFNavArea *area = static_cast<CTFNavArea *>( GetNearestNavArea( tr.endpos, false, 500.f ) );
		if ( area )
		{
			if ( tf_show_incursion_flow.GetInt() > 0 )
			{
				DrawIncursionFlow func;
				SearchSurroundingAreas( area, area->GetCenter(), func, tf_show_incursion_flow_range.GetFloat() );
			}
			else if ( tf_show_incursion_flow_gradient.GetInt() > 0 )
			{
				int nTeam = tf_show_incursion_flow.GetInt() + 1;
				int r = (nTeam == TF_TEAM_RED) ? 255 : 0;
				int b = (nTeam == TF_TEAM_BLUE) ? 255 : 0;

				area->DrawFilled( r, 0, b, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );

				CUtlVector<CTFNavArea *> areas;
				area->CollectNextIncursionAreas( nTeam, &areas );
				FOR_EACH_VEC( areas, i )
				{
					areas[i]->DrawFilled( Min( r + 100, 255 ), 100, Min( b + 100, 255 ), 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}

				area->CollectPriorIncursionAreas( nTeam, &areas );
				FOR_EACH_VEC( areas, i )
				{
					areas[i]->DrawFilled( r / 2, 0, b / 2, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}
			}
		}
	}
	
	FOR_EACH_VEC( TheNavAreas, i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );
		Vector center = area->GetCenter();

		if ( tf_show_in_combat_areas.GetBool() && area->IsInCombat() )
		{
			float flIntensity = area->GetCombatIntensity();
			area->DrawFilled( flIntensity * 255, 0, 0, 255 );
		}

		if ( tf_show_blocked_areas.GetBool() )
		{
			CUtlString string;
			if ( area->IsBlocked( TF_TEAM_RED ) && area->IsBlocked( TF_TEAM_BLUE ) )
				string = "Blocked for All";
			else if ( area->IsBlocked( TF_TEAM_RED ) )
				string = "Blocked for Red";
			else if ( area->IsBlocked( TF_TEAM_BLUE ) )
				string = "Blocked for Blue";

			if ( area == GetSelectedArea() && !string.IsEmpty() )
				NDebugOverlay::Text( center, string, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}

		if ( tf_show_actor_potential_visibility.GetBool() )
		{
			if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_BLUE ) )
			{
				if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_RED ) )
					area->DrawFilled( 255, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				else
					area->DrawFilled( 0, 0, 255, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
			else if ( area->IsPotentiallyVisibleToTeam( TF_TEAM_RED ) )
			{
				area->DrawFilled( 255, 0, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}

		if ( tf_show_bomb_drop_areas.GetBool() )
		{
			if ( area->HasTFAttributes( TF_NAV_BOMB_DROP ) )
				area->DrawFilled( 0, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}

		if ( tf_show_control_points.GetBool() )
		{
			for ( int idx=0; idx < MAX_CONTROL_POINTS; ++idx )
			{
				FOR_EACH_VEC( m_CPAreas[idx], i )
				{
					m_CPAreas[idx][i]->DrawFilled( 255, 150, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
				}

				m_CPArea[idx]->DrawFilled( 255, 255, 0, 255, NDEBUG_PERSIST_TILL_NEXT_SERVER );
			}
		}

		if ( tf_show_mesh_decoration.GetBool() || tf_show_mesh_decoration_manual.GetBool() )
		{
			if ( area == GetSelectedArea() )
			{
				if ( !tf_show_mesh_decoration_manual.GetBool() )
				{
					if ( area->HasTFAttributes( TF_NAV_RESCUE_CLOSET ) )
					{
						NDebugOverlay::Text( center, "Resupply Locker", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						continue;
					}

					if ( area->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM ) )
					{
						if ( !area->HasTFAttributes( TF_NAV_SPAWN_ROOM_EXIT ) )
						{
							NDebugOverlay::Text( center, "Blue Spawn Room", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							continue;
						}

						NDebugOverlay::Text( center, "Blue Spawn Exit", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						continue;
					}

					if ( area->HasTFAttributes( TF_NAV_RED_SPAWN_ROOM ) )
					{
						if ( !area->HasTFAttributes( TF_NAV_SPAWN_ROOM_EXIT ) )
						{
							NDebugOverlay::Text( center, "Red Spawn Room", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							continue;
						}

						NDebugOverlay::Text( center, "Red Spawn Exit", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						continue;
					}

					if ( area->HasTFAttributes( TF_NAV_HEALTH ) || area->HasTFAttributes( TF_NAV_AMMO ) )
					{
						if ( !area->HasTFAttributes( TF_NAV_AMMO ) )
						{
							NDebugOverlay::Text( center, "Health", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							continue;
						}

						if ( !area->HasTFAttributes( TF_NAV_HEALTH ) )
						{
							NDebugOverlay::Text( center, "Ammo", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							continue;
						}

						NDebugOverlay::Text( center, "Health & Ammo", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						continue;
					}

					if ( area->HasTFAttributes( TF_NAV_CONTROL_POINT ) )
					{
						NDebugOverlay::Text( center, "Control Point", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						continue;
					}
				}

				if ( area->HasTFAttributes( TF_NAV_NO_SPAWNING ) )
				{
					NDebugOverlay::Text( center, "No Spawning", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_BLUE_SETUP_GATE ) )
				{
					NDebugOverlay::Text( center, "Blue Setup Gate", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_RED_SETUP_GATE ) )
				{
					NDebugOverlay::Text( center, "Red Setup Gate", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_DOOR_NEVER_BLOCKS ) )
				{
					NDebugOverlay::Text( center, "Door Never Blocks", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_DOOR_ALWAYS_BLOCKS ) )
				{
					NDebugOverlay::Text( center, "Door Always Blocks", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_UNBLOCKABLE ) )
				{
					NDebugOverlay::Text( center, "Unblockable", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_SNIPER_SPOT ) )
				{
					NDebugOverlay::Text( center, "Sniper Spot", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_SENTRY_SPOT ) )
				{
					NDebugOverlay::Text( center, "Sentry Spot", true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE ) )
				{
					CUtlString string( "Blocked Until Second Point Captured" );
					if ( !area->HasAttributes( TF_NAV_WITH_SECOND_POINT ) )
						string = "Blocked Until Third Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_THIRD_POINT ) )
						string = "Blocked Until Fourth Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_FOURTH_POINT ) )
						string = "Blocked Until Fifth Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_FIFTH_POINT ) )
						string = "Blocked Until First Point Captured";

					NDebugOverlay::Text( center, string, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}

				if ( area->HasTFAttributes( TF_NAV_BLOCKED_AFTER_POINT_CAPTURE ) )
				{
					CUtlString string( "Blocked After Second Point Captured" );
					if ( !area->HasAttributes( TF_NAV_WITH_SECOND_POINT ) )
						string = "Blocked After Third Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_THIRD_POINT ) )
						string = "Blocked After Fourth Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_FOURTH_POINT ) )
						string = "Blocked After Fifth Point Captured";
					else if ( !area->HasAttributes( TF_NAV_WITH_FIFTH_POINT ) )
						string = "Blocked After First Point Captured";

					NDebugOverlay::Text( center, string, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
					continue;
				}
			}
		}
	}
}
