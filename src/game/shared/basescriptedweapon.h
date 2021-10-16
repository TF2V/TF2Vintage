#ifndef BASESCRIPTEDWEAPON_H
#define BASESCRIPTEDWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "basecombatweapon_shared.h"
#include "vscript/ivscript.h"
#include <type_traits>


#if defined( CLIENT_DLL )
#define CBaseScriptedWeapon C_BaseScriptedWeapon
#endif


class CScriptedWeaponScopeBase
{
public:
	static IScriptVM *GetVM( void )
	{
		if ( m_pVM )
		{
			return m_pVM;
		}
		else
		{
			extern IScriptVM *g_pScriptVM;
			return g_pScriptVM;
		}
	}

	static void GetVMOverride( IScriptVM *pVM )
	{
		m_pVM = pVM;
	}
private:
	static IScriptVM *m_pVM;
};
class CScriptedWeaponScope : public CScriptScopeT<CScriptedWeaponScopeBase>
{
	typedef CUtlVectorFixed<ScriptVariant_t, 14> CScriptArgArray;
	typedef CUtlMap<char const *, CScriptFuncHolder> CScriptFuncMap;
public:
	CScriptedWeaponScope();
	~CScriptedWeaponScope();

	template<class Arg>
	void PushArg( Arg const &arg )
	{
		// This is not at all the right method to do this, but pushing a
		// "void" type as a argument is an error anyway, c'est la vie
		COMPILE_TIME_ASSERT( ScriptDeduceType( Arg ) );
		m_vecPushedArgs.AddToTail( arg );
	}
	template<class Arg, typename ...Rest>
	void PushArg( Arg const &arg, Rest const &...args )
	{
		COMPILE_TIME_ASSERT( ScriptDeduceType( Arg ) );
		m_vecPushedArgs.AddToTail( arg );

		PushArg( args... );
	}

	template<typename T=int32>
	ScriptStatus_t CallFunc( char const *pszFuncName, T *pReturn = NULL )
	{
		// See if we used this function previously...
		int nIndex = m_FuncMap.Find( pszFuncName );
		if ( nIndex == m_FuncMap.InvalidIndex() )
		{	// ...and cache it if not
			CScriptFuncHolder holder;
			HSCRIPT hFunction = GetVM()->LookupFunction( pszFuncName, m_hScope );
			if ( hFunction != INVALID_HSCRIPT )
			{
				holder.hFunction = hFunction;
				m_FuncHandles.AddToTail( &holder.hFunction );
				nIndex = m_FuncMap.Insert( pszFuncName, holder );
			}
		}
		
		ScriptVariant_t returnVal;
		ScriptStatus_t result = GetVM()->ExecuteFunction( m_FuncMap[nIndex].hFunction, 
														  m_vecPushedArgs.Base(), 
														  m_vecPushedArgs.Count(), 
														  pReturn ? &returnVal : NULL, 
														  m_hScope, true );
		if ( pReturn && result != SCRIPT_ERROR )
		{
			returnVal.AssignTo( pReturn );
		}

		returnVal.Free();
		m_vecPushedArgs.RemoveAll();

		return result;
	}

private:
	CScriptArgArray m_vecPushedArgs;
	CScriptFuncMap m_FuncMap;
};

// Pulled from scripts identically to parsing a weapon file
struct ScriptWeaponInfo_t : public FileWeaponInfo_t
{
	DECLARE_STRUCT_SCRIPTDESC();
	ScriptWeaponInfo_t()
	{
		Q_strncpy( szPrintName, WEAPON_PRINTNAME_MISSING, MAX_WEAPON_STRING );
		iDefaultClip1 = iMaxClip1 = -1;
		iDefaultClip2 = iMaxClip2 = -1;
		iFlags = ITEM_FLAG_LIMITINWORLD;
		bShowUsageHint = false;
		bAutoSwitchTo = true;
		bAutoSwitchFrom = true;
		m_bBuiltRightHanded = true;
		m_bAllowFlipping = true;
		m_bMeleeWeapon = false;
		Q_strncpy( szAmmo1, "", sizeof( szAmmo1 ) );
		Q_strncpy( szAmmo2, "", sizeof( szAmmo2 ) );
		iRumbleEffect = -1;
	}
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

	char const*			GetWeaponScriptName( void ) OVERRIDE;

#if defined(CLIENT_DLL)
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif

private:
	CScriptedWeaponScope m_ScriptScope;
};

#endif
