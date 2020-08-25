//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Napalm Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_NAPALM_H
#define TF_WEAPON_GRENADE_NAPALM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"



//=============================================================================
//
// TF Napalm Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeNapalmProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeNapalmProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NAPALM; }

	// Creation.
	static CTFGrenadeNapalmProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
};

#endif

#endif // TF_WEAPON_GRENADE_NAPALM_H
