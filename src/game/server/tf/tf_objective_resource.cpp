//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_objective_resource.h"
#include "shareddefs.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern void SendProxy_String_tToString( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFObjectiveResource, DT_TFObjectiveResource )
	SendPropInt( SENDINFO( m_nMannVsMachineMaxWaveCount ), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMannVsMachineWaveCount ), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMannVsMachineWaveEnemyCount ), 16, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMvMWorldMoney ), 16, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flMannVsMachineNextWaveTime ) ),
	SendPropBool( SENDINFO( m_bMannVsMachineBetweenWaves ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassCounts ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassCounts ), 16 ) ),
	SendPropArray( SendPropString( SENDINFO_ARRAY( m_iszMannVsMachineWaveClassNames ), 0, SendProxy_String_tToString ), m_iszMannVsMachineWaveClassNames ),
	SendPropArray3( SENDINFO_ARRAY3( m_nMannVsMachineWaveClassFlags ), SendPropInt( SENDINFO_ARRAY( m_nMannVsMachineWaveClassFlags ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bMannVsMachineWaveClassActive ), SendPropBool( SENDINFO_ARRAY( m_bMannVsMachineWaveClassActive ) ) ),
	SendPropInt( SENDINFO( m_nFlagCarrierUpgradeLevel ), 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flMvMBaseBombUpgradeTime ) ),
	SendPropFloat( SENDINFO( m_flMvMNextBombUpgradeTime ) ),
	SendPropStringT( SENDINFO( m_iszMvMPopfileName ) ),
	SendPropInt( SENDINFO( m_iChallengeIndex ), 16 ),
	SendPropInt( SENDINFO( m_nMvMEventPopfileType ), 4, SPROP_UNSIGNED ),
END_SEND_TABLE()


BEGIN_DATADESC( CTFObjectiveResource )
	DEFINE_FIELD( m_nMannVsMachineMaxWaveCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMannVsMachineWaveCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMannVsMachineWaveEnemyCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMvMWorldMoney, FIELD_INTEGER ),
	DEFINE_FIELD( m_flMannVsMachineNextWaveTime, FIELD_TIME ),
	DEFINE_FIELD( m_bMannVsMachineBetweenWaves, FIELD_BOOLEAN ),

	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassCounts, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_iszMannVsMachineWaveClassNames, FIELD_STRING ),
	DEFINE_AUTO_ARRAY( m_nMannVsMachineWaveClassFlags, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_bMannVsMachineWaveClassActive, FIELD_BOOLEAN ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( tf_objective_resource, CTFObjectiveResource );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFObjectiveResource::CTFObjectiveResource()
{
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
	m_iszMvMPopfileName = MAKE_STRING( "" );
	m_nMvMEventPopfileType.Set( MVM_EVENT_POPFILE_NONE );

	for ( int i = 0; i < m_nMannVsMachineWaveClassCounts.Count(); ++i )
	{
		m_nMannVsMachineWaveClassCounts.Set( i, 0 );
	}

	for ( int i = 0; i < m_nMannVsMachineWaveClassFlags.Count(); ++i )
	{
		m_nMannVsMachineWaveClassFlags.Set( i, MVM_CLASS_FLAG_NONE );
	}

	for ( int i = 0; i < m_iszMannVsMachineWaveClassNames.Count(); ++i )
	{
		m_iszMannVsMachineWaveClassNames.Set( i, NULL_STRING );
	}

	for ( int i = 0; i < m_bMannVsMachineWaveClassActive.Count(); ++i )
	{
		m_bMannVsMachineWaveClassActive.Set( i, false );
	}

	m_iszTeleporter = AllocPooledString( "teleporter" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFObjectiveResource::~CTFObjectiveResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::IncrementMannVsMachineWaveClassCount( string_t iszClassIconName, unsigned int iFlags )
{
	for ( int i = 0; i < m_nMannVsMachineWaveClassCounts.Count(); ++i )
	{
		if ( m_iszMannVsMachineWaveClassNames[ i ] == iszClassIconName && ( m_nMannVsMachineWaveClassFlags[ i ] & iFlags ) )
		{
			int nCurCount = m_nMannVsMachineWaveClassCounts[i];
			m_nMannVsMachineWaveClassCounts.Set( i, nCurCount + 1 );

			if ( m_nMannVsMachineWaveClassCounts[ i ] <= 0 )
			{
				m_nMannVsMachineWaveClassCounts.Set( i, 1 );
			}

			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::DecrementMannVsMachineWaveClassCount( string_t iszClassIconName, unsigned int iFlags )
{
	for ( int i = 0; i < m_nMannVsMachineWaveClassCounts.Count(); ++i )
	{
		if ( m_iszMannVsMachineWaveClassNames[i] == iszClassIconName && ( m_nMannVsMachineWaveClassFlags[ i ] & iFlags ) )
		{
			int nCurCount = m_nMannVsMachineWaveClassCounts[i];
			m_nMannVsMachineWaveClassCounts.Set( i, nCurCount - 1 );

			if ( m_nMannVsMachineWaveClassCounts[i] < 0 )
			{
				m_nMannVsMachineWaveClassCounts.Set( i, 0 );
			}

			if ( m_nMannVsMachineWaveClassCounts[i] == 0 )
			{
				SetMannVsMachineWaveClassActive( iszClassIconName, false );
			}

			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::SetMannVsMachineWaveClassActive( string_t iszClassIconName, bool bActive )
{
	for ( int i = 0; i < m_iszMannVsMachineWaveClassNames.Count(); ++i )
	{
		if ( m_iszMannVsMachineWaveClassNames[ i ] == iszClassIconName )
		{
			m_bMannVsMachineWaveClassActive.Set( i, bActive );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::ClearMannVsMachineWaveClassFlags( void )
{
	for ( int i = 0; i < m_nMannVsMachineWaveClassFlags.Count(); ++i )
	{
		m_nMannVsMachineWaveClassFlags.Set( i, MVM_CLASS_FLAG_NONE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFObjectiveResource::AddMannVsMachineWaveClassFlags( int nIndex, unsigned int iFlags )
{
	unsigned int iOldFlags = m_nMannVsMachineWaveClassFlags[ nIndex ];
	m_nMannVsMachineWaveClassFlags.Set( nIndex, iOldFlags | iFlags );
}
