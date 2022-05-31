//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_raygun.h"
#include "decals.h"
#include "tf_fx_shared.h"

// Client specific.
#if defined( CLIENT_DLL )
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif


//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFRaygun, tf_weapon_raygun )
CREATE_SIMPLE_WEAPON_TABLE( TFDRGPomson, tf_weapon_drg_pomson )

//=============================================================================
//
// Weapon Raygun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFRaygun::CTFRaygun()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRaygun::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_bison_impact" );
	PrecacheParticleSystem( "drg_bison_idle" );
	PrecacheParticleSystem( "drg_bison_muzzleflash" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFRaygun::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	if ( !Energy_HasEnergy() )
		return;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFRaygun::Energy_GetShotCost( void ) const
{
	int nNoDrain = 0;
	CALL_ATTRIB_HOOK_INT( nNoDrain, energy_weapon_no_drain );
	if ( nNoDrain != 0 )
		return 0.0f;

	return 5.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFRaygun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	GetViewmodelAddon()->ParticleProp()->StopParticlesNamed( GetIdleParticleEffect(), true );
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFRaygun::Deploy( void )
{
#ifdef CLIENT_DLL
	SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "RG_EFFECTS_THINK" );
#endif

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef CLIENT_DLL
	if ( WeaponState() == WEAPON_IS_ACTIVE )
	{
		if ( GetIndexForThinkContext( "RG_EFFECTS_THINK" ) == NO_THINK_CONTEXT )
			SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + rand() % 5, "RG_EFFECTS_THINK" );
	}
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt )
{
	DispatchParticleEffect( effectName, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", GetEnergyWeaponColor( false ), GetEnergyWeaponColor( true ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRaygun::ClientEffectsThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsLocalPlayer() )
		return;

	if ( !pPlayer->GetViewModel() )
		return;

	if ( WeaponState() != WEAPON_IS_ACTIVE )
		return;

	const int nRandomTime = 2 + rand() % 5;
	SetContextThink( &CTFRaygun::ClientEffectsThink, gpGlobals->curtime + nRandomTime, "RG_EFFECTS_THINK" );

	CNewParticleEffect *pEffect = GetViewmodelAddon()->ParticleProp()->Create( GetIdleParticleEffect(), PATTACH_POINT_FOLLOW, "muzzle" );
	if ( pEffect )
	{
		pEffect->SetControlPoint( CUSTOM_COLOR_CP1, GetEnergyWeaponColor( false ) );
		pEffect->SetControlPoint( CUSTOM_COLOR_CP2, GetEnergyWeaponColor( true ) );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFDRGPomson::CTFDRGPomson()
{
	m_bReloadsSingly = true;
}

void CTFDRGPomson::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem( "drg_pomson_idle" );
	PrecacheParticleSystem( "drg_pomson_impact_drain" );
	PrecacheParticleSystem( "drg_pomson_projectile" );
	PrecacheParticleSystem( "drg_pomson_muzzleflash" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDRGPomson::GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates, bool bUseHitboxes )
{
	BaseClass::GetProjectileFireSetup( pPlayer, vecOffset, vecSrc, angForward, bHitTeammates, bUseHitboxes );

	// adjust to line up with the weapon muzzle
	vecSrc->z -= 13.0f;
}