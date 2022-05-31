//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 13 ) ),
	SendPropArray3(SENDINFO_ARRAY3( m_iDomination ), SendPropInt( SENDINFO_ARRAY( m_iDomination ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxBuffedHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxBuffedHealth ), 32, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iColors ), SendPropVector( SENDINFO_ARRAY3( m_iColors ), 12, SPROP_COORD ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iKillstreak ), SendPropInt( SENDINFO_ARRAY( m_iKillstreak ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bArenaSpectator ), SendPropBool( SENDINFO_ARRAY( m_bArenaSpectator ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iChargeLevel ), SendPropInt( SENDINFO_ARRAY( m_iChargeLevel ), 8, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamage ), SendPropInt( SENDINFO_ARRAY( m_iDamage ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageAssist ), SendPropInt( SENDINFO_ARRAY( m_iDamageAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBoss ), SendPropInt( SENDINFO_ARRAY( m_iDamageBoss ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealing ), SendPropInt( SENDINFO_ARRAY( m_iHealing ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iHealingAssist ), SendPropInt( SENDINFO_ARRAY( m_iHealingAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBlocked ), SendPropInt( SENDINFO_ARRAY( m_iDamageBlocked ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iCurrencyCollected ), SendPropInt( SENDINFO_ARRAY( m_iCurrencyCollected ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBonusPoints ), SendPropInt( SENDINFO_ARRAY( m_iBonusPoints ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

CTFPlayerResource::CTFPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	BaseClass::UpdatePlayerData();

	for ( int i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{			
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
			if ( pPlayerStats ) 
			{
				int iTotalScore = CTFGameRules::CalcPlayerScore( &pPlayerStats->statsAccumulated );
				m_iTotalScore.Set( i, iTotalScore );

				m_iDamage.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE] );
				m_iDamageAssist.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_ASSIST] );
				m_iDamageBoss.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BOSS] );
				m_iHealing.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING] );
				m_iHealingAssist.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_HEALING_ASSIST] );
				m_iDamageBlocked.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BLOCKED] );
				m_iCurrencyCollected.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_CURRENCY_COLLECTED] );
				m_iBonusPoints.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_BONUS_POINTS] );
			}

			m_iMaxHealth.Set( i, pPlayer->GetMaxHealth() );
			m_iMaxBuffedHealth.Set( i, pPlayer->GetMaxHealthForBuffing() );

			m_iPlayerClass.Set( i, pPlayer->GetPlayerClass()->GetClassIndex() );

			m_iDomination.Set( i, pPlayer->m_Shared.GetDominationCount() );

			m_bArenaSpectator.Set( i, pPlayer->IsArenaSpectator() );

			m_iKillstreak.Set( i, 
				pPlayer->m_Shared.GetKillstreak( 0 ) + pPlayer->m_Shared.GetKillstreak( 1 ) + pPlayer->m_Shared.GetKillstreak( 2 ) );
		}
	}
}

void CTFPlayerResource::Spawn( void )
{
	for ( int i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iDomination.Set( i, 0 );
		m_iTotalScore.Set( i, 0 );
		m_iDamage.Set( i, 0 );
		m_iDamageAssist.Set( i, 0 );
		m_iDamageBoss.Set( i, 0 );
		m_iHealing.Set( i, 0 );
		m_iHealingAssist.Set( i, 0 );
		m_iDamageBlocked.Set( i, 0 );
		m_iCurrencyCollected.Set( i, 0 );
		m_iBonusPoints.Set( i, 0 );
		m_iMaxHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iMaxBuffedHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iPlayerClass.Set( i, TF_CLASS_UNDEFINED );
		m_iColors.Set( i, Vector(0.0, 0.0, 0.0) );
		m_iKillstreak.Set( i, 0 );
		m_bArenaSpectator.Set( i, false );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTFPlayerResource::GetPlayerColor( int iIndex )
{
	return Color( m_iColors[iIndex].x * 255.0, m_iColors[iIndex].y * 255.0, m_iColors[iIndex].z * 255.0, 255 );
}