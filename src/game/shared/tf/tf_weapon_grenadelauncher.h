//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_GRENADELAUNCHER_H
#define TF_WEAPON_GRENADELAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weaponbase_grenadeproj.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeLauncher C_TFGrenadeLauncher
#endif

#define TF_GRENADE_LAUNCHER_XBOX_CLIP 6

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeLauncher : public CTFWeaponBaseGun, public ITFChargeUpWeapon
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFGrenadeLauncher();
	~CTFGrenadeLauncher();

	virtual void	Spawn( void );
	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_GRENADELAUNCHER; }
	virtual void	SecondaryAttack();

	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Deploy( void );
	virtual void	WeaponReset(void);
	virtual void	PrimaryAttack( void );
	virtual void	WeaponIdle( void );
	virtual float	GetProjectileSpeed( void );

	virtual bool	Reload( void );

	virtual int GetMaxClip1( void ) const;
	virtual int GetDefaultClip1( void ) const;

	virtual void SwitchBodyGroups( void );

	int GetDetonateMode( void ) const;

	// Mortar.
	bool IsMortar(void) const;
	int  MortarTime(void);
	
	// ITFChargeUpWeapon
	// These are inverted compared to the regular to compensate for HUD.
	virtual float	GetChargeBeginTime(void) { return m_flChargeBeginTime + MortarTime(); }
	virtual float	GetChargeMaxTime( void ) { return m_flChargeBeginTime; }
	
	float	m_flChargeBeginTime;

#ifdef CLIENT_DLL
	void				ToggleCannonFuse();
	CNewParticleEffect	*m_pCannonFuse;
#endif

	// Donk table.
	struct Donk_t
	{
		CHandle <CBaseEntity> m_hDonk;
		float m_flDonkTime;
	};

public:

	CBaseEntity *FireProjectileInternal( CTFPlayer *pPlayer );
	void LaunchGrenade( void );

private:

	CTFGrenadeLauncher( const CTFGrenadeLauncher & ) {}
};

// Cannon.

#if defined CLIENT_DLL
#define CTFGrenadeLauncher_Cannon C_TFGrenadeLauncher_Cannon
#endif

class CTFGrenadeLauncher_Cannon : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS( CTFGrenadeLauncher_Cannon, CTFGrenadeLauncher )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual int GetWeaponID( void ) const { return TF_WEAPON_CANNON; }
};


// TF2 CUT Grenades weaponbased.

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeFrag C_TFGrenadeFrag
#endif

class CTFGrenadeFrag : public CTFGrenadeLauncher
{
public:



	DECLARE_CLASS(CTFGrenadeFrag, CTFGrenadeLauncher)
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();



	// Server specific.

	CTFGrenadeFrag();
	~CTFGrenadeFrag();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_NORMAL; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeFrag(const CTFGrenadeFrag&) {}
};


// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeCaltrop C_TFGrenadeCaltrop
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeCaltrop : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeCaltrop, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeCaltrop();
	~CTFGrenadeCaltrop();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_CALTROP; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeCaltrop(const CTFGrenadeCaltrop&) {}
};
// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeConcussion C_TFGrenadeConcussion
#endif


class CTFGrenadeConcussion : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeConcussion, CTFGrenadeLauncher);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeConcussion();
	~CTFGrenadeConcussion();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_CONCUSSION; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeConcussion(const CTFGrenadeConcussion&) {}
};

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeEmp C_TFGrenadeEmp
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeEmp : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeEmp, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.


	CTFGrenadeEmp();
	~CTFGrenadeEmp();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_EMP; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeEmp(const CTFGrenadeEmp&) {}
};

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeSmokeBomb C_TFGrenadeSmokeBomb
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeSmokeBomb : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeSmokeBomb, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeSmokeBomb();
	~CTFGrenadeSmokeBomb();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_SMOKE_BOMB; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeSmokeBomb(const CTFGrenadeSmokeBomb&) {}
};
// Client specific.
#ifdef CLIENT_DLL
#define CTFHealGrenade C_TFHealGrenade
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFHealGrenade : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFHealGrenade, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFHealGrenade();
	~CTFHealGrenade();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_HEAL; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFHealGrenade(const CTFHealGrenade&) {}
};

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeMirv C_TFGrenadeMirv
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeMirv : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeMirv, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeMirv();
	~CTFGrenadeMirv();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_MIRV; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeMirv(const CTFGrenadeMirv&) {}
};

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeNail C_TFGrenadeNail
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeNail : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeNail, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeNail();
	~CTFGrenadeNail();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_NAIL; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeNail(const CTFGrenadeNail&) {}
};

// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeNapalm C_TFGrenadeNapalm
#endif

//#define TF_GRENADE_LAUNCHER_XBOX_CLIP 4

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
class CTFGrenadeNapalm : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeNapalm, CTFGrenadeLauncher);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeNapalm();
	~CTFGrenadeNapalm();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_NAPALM; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeNapalm(const CTFGrenadeNapalm&) {}
};

//=============================================================================
//
// TF Weapon Grenade Launcher.
//
// Client specific.
#ifdef CLIENT_DLL
#define CTFGrenadeGas C_TFGrenadeGas
#endif

class CTFGrenadeGas : public CTFGrenadeLauncher
{
public:

	DECLARE_CLASS(CTFGrenadeGas, CTFGrenadeLauncher);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	// Server specific.

	CTFGrenadeGas();
	~CTFGrenadeGas();

	virtual void	Spawn(void);
	virtual int		GetWeaponID(void) const { return TF_WEAPON_GRENADE_GAS; }
	virtual void	SecondaryAttack();

	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual bool	Deploy(void);
	virtual void	PrimaryAttack(void);
	virtual void	WeaponIdle(void);
	virtual float	GetProjectileSpeed(void);

	virtual bool	Reload(void);

	virtual int GetMaxClip1(void) const;
	virtual int GetDefaultClip1(void) const;

public:

	void LaunchGrenade(void);

private:

	CTFGrenadeGas(const CTFGrenadeGas&) {}
};

#endif // TF_WEAPON_GRENADELAUNCHER_H