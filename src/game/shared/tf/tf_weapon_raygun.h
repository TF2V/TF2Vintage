//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_RAYGUN_H
#define TF_WEAPON_RAYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_rocketlauncher.h"

#if defined( CLIENT_DLL )
#define CTFRaygun C_TFRaygun
#define CTFDRGPomson C_TFDRGPomson
#endif

//=============================================================================
//
// Raygun Class.
//
class CTFRaygun : public CTFRocketLauncher
{
public:

	DECLARE_CLASS( CTFRaygun, CTFRocketLauncher );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFRaygun();

	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_RAYGUN; }
	virtual float	GetProjectileSpeed( void )			{ return 1200.0f; }
	virtual float	GetProjectileGravity( void )		{ return 0.0f; }
	virtual void	PrimaryAttack();
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	ItemPostFrame( void );

	bool			IsViewModelFlipped( void ) OVERRIDE { return !BaseClass::IsViewModelFlipped(); }
	virtual const char *GetMuzzleFlashParticleEffect( void ) { return "drg_bison_muzzleflash"; }
	
	virtual bool		HasChargeBar( void )				{ return true; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_BISON"; }

	bool			IsEnergyWeapon( void ) const OVERRIDE { return true; }
	virtual float	Energy_GetShotCost( void ) const;
	virtual float	Energy_GetRechargeCost( void ) const { return 5.f; }

#ifdef CLIENT_DLL
	virtual void		DispatchMuzzleFlash( const char* effectName, C_BaseEntity* pAttachEnt );
	virtual bool		ShouldPlayClientReloadSound() { return true; }
	void				ClientEffectsThink( void );
	virtual const char *GetIdleParticleEffect( void ) { return "drg_bison_idle"; }
#endif

private:

	CTFRaygun( const CTFRaygun & ) {}
};

//=============================================================================
//
// Pomson Class.
//
class CTFDRGPomson : public CTFRaygun
{
public:

	DECLARE_CLASS( CTFDRGPomson, CTFRaygun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFDRGPomson();

	virtual void	Precache( void );

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_DRG_POMSON; }
	virtual const char* GetEffectLabelText( void )			{ return "#TF_POMSON_HUD"; }

	virtual const char *GetMuzzleFlashParticleEffect( void )	{ return "drg_pomson_muzzleflash"; }
	virtual const char *GetIdleParticleEffect( void )			{ return "drg_pomson_idle"; }

	void GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bUseHitboxes = true ) OVERRIDE;

private:

	CTFDRGPomson( const CTFDRGPomson & ) {}
};


#endif // TF_WEAPON_RAYGUN_H
