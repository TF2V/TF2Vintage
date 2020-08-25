//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Base Gun 
//
//=============================================================================

#ifndef TF_WEAPONBASE_GUN_H
#define TF_WEAPONBASE_GUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "tf_weapon_grenade_mirv.h"
#include "tf_weapon_grenade_heal.h"
#include "tf_weapon_grenade_caltrop.h"
#include "tf_weapon_grenade_smoke_bomb.h"
#include "tf_weapon_grenade_normal.h"
#include "tf_weapon_grenade_concussion.h"
#include "tf_weapon_grenade_emp.h"
#include "tf_weapon_grenade_nail.h"
#include "tf_weapon_grenade_napalm.h"
#include "tf_weapon_grenade_gas.h"
#include "tf_weapon_grenade_normal.h"

#if defined( CLIENT_DLL )
#define CTFWeaponBaseGun C_TFWeaponBaseGun
#endif

#define ZOOM_CONTEXT		"ZoomContext"
#define ZOOM_REZOOM_TIME	1.4f

//=============================================================================
//
// Weapon Base Melee Gun
//
class CTFWeaponBaseGun : public CTFWeaponBase
{
public:

	DECLARE_CLASS( CTFWeaponBaseGun, CTFWeaponBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined( CLIENT_DLL ) 
	DECLARE_DATADESC();
#endif

	CTFWeaponBaseGun();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	// Derived classes call this to fire a bullet.
	//bool TFBaseGunFire( void );

	virtual void DoFireEffects();

	void ToggleZoom( void );

	virtual CBaseEntity *FireProjectile( CTFPlayer *pPlayer );
	virtual void		GetProjectileFireSetup( CTFPlayer *pPlayer, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammates = true, bool bUseHitboxes = false );
	void				GetProjectileReflectSetup( CTFPlayer *pPlayer, const Vector &vecPos, Vector *vecDeflect, bool bHitTeammates = true, bool bUseHitboxes = false );

	virtual void FireBullet( CTFPlayer *pPlayer );
	CBaseEntity *FireRocket( CTFPlayer *pPlayer );
	CBaseEntity *FireEnergyBall( CTFPlayer *pPlayer, bool bCharged = false );
	CBaseEntity *FireEnergyRing( CTFPlayer *pPlayer );
	CBaseEntity *FireFireBall( CTFPlayer *pPlayer );
	CBaseEntity *FireEnergyOrb(CTFPlayer *pPlayer);
	CBaseEntity *FireNail( CTFPlayer *pPlayer, int iSpecificNail );
	CBaseEntity *FirePipeBomb( CTFPlayer *pPlayer, int iRemoteDetonate );
	CBaseEntity *FireFlare( CTFPlayer *pPlayer );
	CBaseEntity *FireArrow( CTFPlayer *pPlayer, int iType );
	CBaseEntity *FireJar( CTFPlayer *pPlayer, int iType );
	//CBaseEntity *FireGrenade( CTFPlayer *pPlayer );
	CBaseEntity* FireMirv(CTFPlayer* pPlayer);
	CBaseEntity* FireHeal(CTFPlayer* pPlayer);
	CBaseEntity* FireCaltrop(CTFPlayer* pPlayer);
	CBaseEntity* FireSmokeBomb(CTFPlayer* pPlayer);
	CBaseEntity* FireGas(CTFPlayer* pPlayer);
	CBaseEntity* FireNormal(CTFPlayer* pPlayer);
	CBaseEntity* FireConcussion(CTFPlayer* pPlayer);
	CBaseEntity* FireEmp(CTFPlayer* pPlayer);
	CBaseEntity* FireNailGrenade(CTFPlayer* pPlayer);
	CBaseEntity* FireNapalm(CTFPlayer* pPlayer);

	virtual float GetWeaponSpread( void );
	virtual float GetProjectileSpeed( void );
	virtual float GetProjectileGravity( void );
	virtual bool  IsFlameArrow( void );

	void UpdatePunchAngles( CTFPlayer *pPlayer );
	virtual float GetProjectileDamage( void );


	virtual void ZoomIn( void );
	virtual void ZoomOut( void );
	void ZoomOutIn( void );

	virtual void PlayWeaponShootSound( void );

	virtual int GetAmmoPerShot( void ) const;

	virtual void RemoveAmmo( CTFPlayer *pPlayer );
	
	virtual void AddDoubleDonk(CBaseEntity* pVictim );
	virtual bool IsDoubleDonk(CBaseEntity* pVictim );
	
	CUtlVector<CBaseEntity*> hDonkedPlayers;
	CUtlVector<float> hDonkedTimeLimit;

private:

	CTFWeaponBaseGun( const CTFWeaponBaseGun & );

};

#endif // TF_WEAPONBASE_GUN_H