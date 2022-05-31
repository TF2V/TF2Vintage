//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Bison Projectile.
// baseclass is originally CTFBaseProjectile.
//=============================================================================//
#ifndef TF_PROJECTILE_ENERGY_RING_H
#define TF_PROJECTILE_ENERGY_RING_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_projectile_base.h"
#ifdef GAME_DLL
#include "iscorer.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFProjectile_EnergyRing C_TFProjectile_EnergyRing
#endif

class CTFProjectile_EnergyRing : public CTFBaseProjectile
{
public:
	DECLARE_CLASS( CTFProjectile_EnergyRing, CTFBaseProjectile );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CTFProjectile_EnergyRing();
	~CTFProjectile_EnergyRing();

	static CTFProjectile_EnergyRing *Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL );
	virtual void	Spawn();
	virtual void	Precache();

	virtual int		GetWeaponID( void ) const	{ return UsePenetratingBeam() ? TF_WEAPON_RAYGUN : TF_WEAPON_DRG_POMSON; }

	virtual bool	IsDeflectable() { return false; }

#ifdef GAME_DLL
	virtual void	ProjectileTouch( CBaseEntity *pOther );
#else
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	CreateTrails( void );
	virtual void	CreateLightEffects( void );
#endif

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void )			{ return 0.0f; }
	virtual float GetDamage( void );

private:
	bool UsePenetratingBeam() const;
	char const *GetTrailParticleName() const;
	void PlayImpactEffects( Vector const& vecPos, bool bHitFlesh );

#ifdef GAME_DLL
	typedef struct PenTargets_s
	{
		EHANDLE hEntity;
		float flLastHitTime;
	} PenTargets_t;
	CUtlVector<PenTargets_t> m_aHitEnemies;
#else
	CNewParticleEffect	*m_pRing;
#endif

};

#endif //TF_PROJECTILE_ENERGY_RING_H