//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates objective data
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "clientmode_tf.h"
#include "tf_gamerules.h"
#include "c_tf_objective_resource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT( C_TFObjectiveResource, DT_TFObjectiveResource, CTFObjectiveResource )
	RecvPropInt( RECVINFO( m_nMannVsMachineMaxWaveCount ) ),
	RecvPropInt( RECVINFO( m_nMannVsMachineWaveCount ) ),
	RecvPropInt( RECVINFO( m_nMannVsMachineWaveEnemyCount ) ),
	RecvPropInt( RECVINFO( m_nMvMWorldMoney ) ),
	RecvPropFloat( RECVINFO( m_flMannVsMachineNextWaveTime ) ),
	RecvPropBool( RECVINFO( m_bMannVsMachineBetweenWaves ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassCounts ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassCounts[0] ) ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_iszMannVsMachineWaveClassNames[0] ) ), m_iszMannVsMachineWaveClassNames ),
	RecvPropArray3( RECVINFO_ARRAY( m_nMannVsMachineWaveClassFlags ), RecvPropInt( RECVINFO( m_nMannVsMachineWaveClassFlags[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bMannVsMachineWaveClassActive ), RecvPropBool( RECVINFO( m_bMannVsMachineWaveClassActive[0] ) ) ),
	RecvPropInt( RECVINFO( m_nFlagCarrierUpgradeLevel ) ),
	RecvPropFloat( RECVINFO( m_flMvMBaseBombUpgradeTime ) ),
	RecvPropFloat( RECVINFO( m_flMvMNextBombUpgradeTime ) ),
	RecvPropString( RECVINFO( m_iszMvMPopfileName ) ),
	RecvPropInt( RECVINFO( m_iChallengeIndex ) ),
	RecvPropInt( RECVINFO( m_nMvMEventPopfileType ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::C_TFObjectiveResource()
{
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_blu_up" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red" );
	PrecacheMaterial( "sprites/obj_icons/icon_obj_cap_red_up" );
	PrecacheMaterial( "VGUI/flagtime_empty" );
	PrecacheMaterial( "VGUI/flagtime_full" );

	m_nMannVsMachineMaxWaveCount = 0;
	m_nMannVsMachineWaveCount = 0;
	m_nMannVsMachineWaveEnemyCount = 0;
	m_nMvMWorldMoney = 0;
	m_flMannVsMachineNextWaveTime = 0;
	m_bMannVsMachineBetweenWaves = false;
	m_nFlagCarrierUpgradeLevel = 0;
	m_flMvMBaseBombUpgradeTime = 0;
	m_flMvMNextBombUpgradeTime = 0;
	m_iChallengeIndex = -1;
	m_iszMvMPopfileName[0] = '\0';
	m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;

	for ( int i = 0 ; i < ARRAYSIZE( m_nMannVsMachineWaveClassCounts ); ++i )
	{
		m_nMannVsMachineWaveClassCounts[ i ] = 0;
	}

	for ( int i = 0 ; i < ARRAYSIZE( m_nMannVsMachineWaveClassFlags ); ++i )
	{
		m_nMannVsMachineWaveClassFlags[ i ] = MVM_CLASS_FLAG_NONE;
	}

	for ( int i = 0 ; i < ARRAYSIZE( m_iszMannVsMachineWaveClassNames ); ++i )
	{
		m_iszMannVsMachineWaveClassNames[ i ][0] = NULL_STRING;
	}

	for ( int i = 0 ; i < ARRAYSIZE( m_bMannVsMachineWaveClassActive ); ++i )
	{
		m_bMannVsMachineWaveClassActive[ i ] = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFObjectiveResource::~C_TFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPCappingSwipe( int index, int iCappingTeam )
{
	Assert( index < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	switch ( iCappingTeam )
	{
		case TF_TEAM_RED:
			return "sprites/obj_icons/icon_obj_cap_red";
		case TF_TEAM_BLUE:
			return "sprites/obj_icons/icon_obj_cap_blu";
		case TF_TEAM_GREEN:
			return "sprites/obj_icons/icon_obj_cap_grn";
		case TF_TEAM_YELLOW:
			return "sprites/obj_icons/icon_obj_cap_ylw";
	}


	return "sprites/obj_icons/icon_obj_cap_blu";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarFG( int index, int iOwningTeam )
{
	Assert( index < m_iNumControlPoints );

	switch ( iOwningTeam )
	{
		case TF_TEAM_RED:
			return "progress_bar_red";
		case TF_TEAM_BLUE:
			return "progress_bar_blu";
		case TF_TEAM_GREEN:
			return "progress_bar_grn";
		case TF_TEAM_YELLOW:
			return "progress_bar_ylw";
	}
	return "progress_bar";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFObjectiveResource::GetGameSpecificCPBarBG( int index, int iCappingTeam )
{
	Assert( index < m_iNumControlPoints );
	Assert( iCappingTeam != TEAM_UNASSIGNED );

	switch ( iCappingTeam )
	{
		case TF_TEAM_RED:
			return "progress_bar_red";
		case TF_TEAM_BLUE:
			return "progress_bar_blu";
		case TF_TEAM_GREEN:
			return "progress_bar_grn";
		case TF_TEAM_YELLOW:
			return "progress_bar_ylw";
	}

	return "progress_bar";
}

void C_TFObjectiveResource::SetCappingTeam( int index, int team )
{
	//Display warning that someone is capping our point.
	//Only do this at the start of a cap and if WE own the point.
	//Also don't warn on a point that will do a "Last Point cap" warning.
	//Don't play the alert for payload
	if ( !TFGameRules()->IsInEscortMode() && GetNumControlPoints() > 0 && GetCapWarningLevel( index ) != index && GetCPCapPercentage( index ) == 0.0f && team != TEAM_UNASSIGNED && GetOwningTeam( index ) != TEAM_UNASSIGNED )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer )
		{
			int iLocalTeam = pLocalPlayer->GetTeamNumber();

			if ( iLocalTeam != team )
			{
				CLocalPlayerFilter filter;
				C_BaseEntity::EmitSound( filter, -1, "Announcer.ControlPointContested" );
			}
		}
	}

	BaseClass::SetCappingTeam( index, team );
}
