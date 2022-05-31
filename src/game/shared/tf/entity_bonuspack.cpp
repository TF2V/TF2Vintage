//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "entity_bonuspack.h"
#include "tf_logic_robot_destruction.h"

#ifdef GAME_DLL
#include "particle_parse.h"
#include "tf_fx.h"
#include "tf_player.h"
#endif

ConVar tf_bonuspack_score( "tf_bonuspack_score", "1", FCVAR_REPLICATED|FCVAR_DEVELOPMENTONLY );

IMPLEMENT_NETWORKCLASS_ALIASED( BonusPack, DT_CBonusPack )

BEGIN_NETWORK_TABLE( CBonusPack, DT_CBonusPack )
END_NETWORK_TABLE()

BEGIN_DATADESC( CBonusPack )
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_bonuspack, CBonusPack );

IMPLEMENT_AUTO_LIST( IBonusPackAutoList );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBonusPack::CBonusPack()
{
#ifdef GAME_DLL
	m_nBlinkState = 0;
#else
	SetCycle( RandomFloat() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::Spawn( void )
{
	Precache();
#ifdef GAME_DLL
	// We are skipping over our base class
	CTFPowerup::Spawn();

	const char *pszParticleName = GetTeamNumber() == TF_TEAM_RED ? "powercore_alert_blue" : "powercore_alert_red";
	DispatchParticleEffect( pszParticleName, PATTACH_POINT_FOLLOW, this, "particle_spawn" );

	SetModel( GetPowerupModel() );
	CollisionProp()->UseTriggerBounds( true, 64 );
	SetRenderMode( kRenderTransAlpha );

	m_flAllowPickupAt = gpGlobals->curtime + 0.5f;

	m_flRemoveAt = gpGlobals->curtime + 25.0f;
	SetContextThink( &CBonusPack::BlinkThink, m_flRemoveAt - 5.0f, "blink_think" );
	SetContextThink( &CBonusPack::SUB_Remove, m_flRemoveAt, "RemoveThink" );
#else
	CBaseAnimating::Spawn();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::Precache( void )
{
	BaseClass::Precache();

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheTeamParticles( "powercore_embers_%s" );

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBonusPack::BlinkThink()
{
	float flTimeToKill = m_flRemoveAt - gpGlobals->curtime;
	float flNextBlink = RemapValClamped( flTimeToKill, 5.0f, 0.0f, 0.5f, 0.1f );
	SetContextThink( &CBonusPack::BlinkThink, gpGlobals->curtime + flNextBlink, "blink_think" );

	++m_nBlinkState;
	if ( m_nBlinkState % 2 == 0 )
	{
		SetRenderColorA( 25 );
	}
	else
	{
		SetRenderColorA( 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusPack::MyTouch( CBasePlayer *pPlayer )
{
	if ( ValidTouch( pPlayer ) && gpGlobals->curtime >= m_flAllowPickupAt )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return true;

		const Vector &vecOrigin = GetAbsOrigin() + Vector( 0, 0, 5.0f );
		CPVSFilter filter( vecOrigin );
		const char *pszParticleName = ConstructTeamParticle( "powercore_embers_%s", pPlayer->GetTeamNumber() );
		TE_TFParticleEffect( filter, 0.15f, pszParticleName, vecOrigin, vec3_angle );

		int nHealthToAdd = clamp( 5, 0, pTFPlayer->m_Shared.GetMaxBuffedHealth() - pPlayer->GetHealth() );
		pPlayer->TakeHealth( nHealthToAdd, DMG_GENERIC | DMG_IGNORE_MAXHEALTH );

		for ( int i=0; i < TF_AMMO_COUNT; i++ )
		{
			pPlayer->GiveAmmo( 5, i );
		}

		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
 		{
 			CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( GetTeamNumber(), 
																			   tf_bonuspack_score.GetInt(), 
																			   SCORE_CORES_COLLECTED, 
																			   ToTFPlayer( pPlayer ) );
 		}

		pPlayer->SetLastObjectiveTime( gpGlobals->curtime );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBonusPack::ValidTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer->GetTeamNumber() != GetTeamNumber() )
		return false;

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED ) || pTFPlayer->m_Shared.GetPercentInvisible() > 0.25f )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pTFPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) || pTFPlayer->m_Shared.InCond( TF_COND_PASSTIME_INTERCEPTION ) )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
		return false;

	if ( pTFPlayer->m_Shared.IsInvulnerable() )
		return false;

	return BaseClass::ValidTouch( pPlayer );
}
#endif