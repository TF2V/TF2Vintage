#ifndef BASESCRIPTEDWEAPON_H
#define BASESCRIPTEDWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "vscript/ivscript.h"


#if defined( CLIENT_DLL )
#define CBaseScriptedWeapon C_BaseScriptedWeapon
#endif

// Pulled from scripts identically to parsing a weapon file,
// unlike parsing though, defaults aren't provided
struct ScriptWeaponInfo_t : public FileWeaponInfo_t
{
	DECLARE_STRUCT_SCRIPTDESC();
};
class CScriptedWeaponData
{
	DECLARE_CLASS_NOBASE( CScriptedWeaponData )
public:
	DECLARE_EMBEDDED_NETWORKVAR();

	bool BInit( CScriptedWeaponScope &scope, FileWeaponInfo_t *pWeaponInfo );
	operator FileWeaponInfo_t const &( ) const { Assert( m_pWeaponInfo ); return *m_pWeaponInfo; }
	FileWeaponInfo_t *operator->() { Assert( m_pWeaponInfo ); return m_pWeaponInfo; }

private:
	ScriptWeaponInfo_t *m_pWeaponInfo;
};

class CBaseScriptedWeapon : public CBaseCombatWeapon
{
	DECLARE_CLASS( CBaseScriptedWeapon, CBaseCombatWeapon );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ENT_SCRIPTDESC();

	CBaseScriptedWeapon();
	~CBaseScriptedWeapon();

	virtual void		Precache();
	virtual void		Spawn();

#if defined( GAME_DLL )
	void				HandleAnimEvent( animevent_t *pEvent ) OVERRIDE;
#endif

	virtual void		PrimaryAttack();
	virtual void		SecondaryAttack();

	virtual bool		CanHolster( void );
	virtual bool		CanDeploy( void );
	virtual bool		Deploy( void );
	virtual bool		Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	bool				ScriptHolster( HSCRIPT pSwitchingTo );
	virtual void		OnActiveStateChanged( int iOldState );
	virtual void		Equip( CBaseCombatCharacter *pOwner );
	void				ScriptEquip( HSCRIPT pOwner );
	virtual void		Detach();

private:
	CNetworkVarEmbedded( CScriptedWeaponData, m_WeaponData );
	CNetworkVar( int, m_nWeaponDataChanged );
#if defined(CLIENT_DLL)
	int m_nWeaponDataChangedOld;
#endif
};

#endif
