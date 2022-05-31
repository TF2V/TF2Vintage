//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife Class
//
//=============================================================================
#ifndef TF_WEAPON_KNIFE_H
#define TF_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFKnife C_TFKnife
#endif

//=============================================================================
//
// Knife class.
//
class CTFKnife : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFKnife, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFKnife();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_KNIFE; }

	virtual bool		Deploy( void );
	virtual void		ItemPostFrame( void );
	virtual void		PrimaryAttack( void );
	virtual void		WeaponIdle(void);

	virtual float		GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage );

	virtual void		SendPlayerAnimEvent( CTFPlayer *pPlayer );

	bool				IsBehindAndFacingTarget( CBaseEntity *pTarget );
	bool				IsBehindTarget(CBaseEntity *pTarget);

	virtual bool		CalcIsAttackCriticalHelper( void );

	virtual void		DoViewModelAnimation( void );
	virtual bool		SendWeaponAnim( int iActivity );

	void				BackstabVMThink( void );
	void				DisguiseOnKill( void );
	void				BackstabBlocked(void);
	
	bool 				CanHolster( void );
	bool 				CanDeploy( void );	
	
	bool				CanExtinguish( void );
	void				Extinguish( void );
	
	virtual bool		HasChargeBar(void);
	virtual const char* GetEffectLabelText( void )			{ return "#TF_KNIFE"; }
	virtual float	GetEffectBarProgress( void );

private:
	CHandle<CTFPlayer> m_hBackstabVictim;
	CNetworkVar( bool, m_bReadyToBackstab );
	CNetworkVar( float, m_flKnifeRegenTime );
	CNetworkVar( bool, m_bForcedSwap );
	CNetworkVar( float, m_flSwapBlocked);
	CNetworkVar( bool, m_bDelayedStab );
	
	CTFKnife( const CTFKnife & ) {}
};

#endif // TF_WEAPON_KNIFE_H
