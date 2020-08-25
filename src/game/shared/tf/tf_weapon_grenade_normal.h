//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Normal Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_NORMAL_H
#define TF_WEAPON_GRENADE_NORMAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"

//=============================================================================
//
// TF Normal Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeNormalProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeNormalProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_NORMAL; }

	// Creation.
	static CTFGrenadeNormalProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                                       const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
};

#endif

#endif // TF_WEAPON_GRENADE_NORMAL_H
