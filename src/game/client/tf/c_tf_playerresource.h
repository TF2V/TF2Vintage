//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_PLAYERRESOURCE_H
#define C_TF_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "c_playerresource.h"

class C_TF_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS( C_TF_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_TF_PlayerResource();
	virtual ~C_TF_PlayerResource();

	int	GetTotalScore( int iIndex ) { return GetArrayValue( iIndex, m_iTotalScore, 0 ); }
	int GetDominations( int iIndex ) { return GetArrayValue( iIndex, m_iDomination, 0 ); }
	int GetMaxHealth( int iIndex )   { return GetArrayValue( iIndex, m_iMaxHealth, TF_HEALTH_UNDEFINED ); }
	int GetMaxHealthForBuffing( int iIndex ) { return GetArrayValue( iIndex, m_iMaxBuffedHealth, TF_HEALTH_UNDEFINED ); }
	int GetPlayerClass( int iIndex ) { return GetArrayValue( iIndex, m_iPlayerClass, TF_CLASS_UNDEFINED ); }
	Color GetPlayerColor(int iIndex);
	int GetKillstreak(int iIndex) { return GetArrayValue(iIndex, m_iKillstreak, 0); }
	bool GetArenaSpectator( int iIndex ) { return m_bArenaSpectator[iIndex]; }

	int GetCountForPlayerClass( int iTeam, int iClass, bool bExcludeLocalPlayer = false );
	
protected:
	int GetArrayValue( int iIndex, int *pArray, int defaultVal );

	int		m_iDomination[MAX_PLAYERS+1];
	int		m_iTotalScore[MAX_PLAYERS+1];
	int		m_iMaxHealth[MAX_PLAYERS+1];
	int		m_iMaxBuffedHealth[MAX_PLAYERS+1];
	int		m_iPlayerClass[MAX_PLAYERS+1];
	int		m_iKillstreak[MAX_PLAYERS+1];
	Vector	m_iColors[MAX_PLAYERS+1];
	bool    m_bArenaSpectator[MAX_PLAYERS+1];
	int		m_iChargeLevel[MAX_PLAYERS+1];
	int		m_iDamage[MAX_PLAYERS+1];
	int		m_iDamageAssist[MAX_PLAYERS+1];
	int		m_iDamageBoss[MAX_PLAYERS+1];
	int		m_iHealing[MAX_PLAYERS+1];
	int		m_iHealingAssist[MAX_PLAYERS+1];
	int		m_iDamageBlocked[MAX_PLAYERS+1];
	int		m_iCurrencyCollected[MAX_PLAYERS+1];
	int		m_iBonusPoints[MAX_PLAYERS+1];
};

inline C_TF_PlayerResource *TFPlayerResource()
{
	Assert( dynamic_cast<C_TF_PlayerResource *>( g_PR ) );
	return static_cast<C_TF_PlayerResource *>( g_PR );
}

#endif // C_TF_PLAYERRESOURCE_H
