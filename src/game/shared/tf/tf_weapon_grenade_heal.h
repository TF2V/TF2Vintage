//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: TF Heal Grenade.
//
//=============================================================================//
#ifndef TF_WEAPON_GRENADE_HEAL_H
#define TF_WEAPON_GRENADE_HEAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_grenade.h"
#include "tf_weaponbase_grenadeproj.h"



//=============================================================================
//
// TF Heal Grenade Projectile (Server specific.)
//
#ifdef GAME_DLL

class CTFGrenadeHealProjectile : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFGrenadeHealProjectile, CTFWeaponBaseGrenadeProj );

	// Unique identifier.
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_HEAL; }

	// Creation.
	static CTFGrenadeHealProjectile *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );

	// Overrides.
	virtual void	Spawn();
	virtual void	Precache();
	virtual void	BounceSound( void );
	virtual void	Detonate();
	void			DetonateThink( void );

	DECLARE_DATADESC();

private:

	bool			m_bPlayedLeadIn;
};

#endif

#endif // TF_WEAPON_GRENADE_HEAL_H
