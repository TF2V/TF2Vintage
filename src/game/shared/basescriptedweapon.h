#ifndef BASESCRIPTEDWEAPON_H
#define BASESCRIPTEDWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "basecombatweapon_shared.h"
#include "vscript/ivscript.h"


#if defined( CLIENT_DLL )
#define CBaseScriptedWeapon C_BaseScriptedWeapon
#endif

#define ScriptGetStructMember(pStruct, hScope, memberName, scriptName)		do { ScriptVariant_t value; g_pScriptVM->GetValue( hScope, scriptName, &value ); value.AssignTo( &pStruct->##memberName ); g_pScriptVM->ReleaseValue( value ); } while ( 0 )


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

	template<typename T, typename D>
	void GetStructData( T *pStruct, ptrdiff_t nOffs, D data )
	{
		*(D *)( (intptr_t)pStruct + nOffs ) = (D)data;
	}

	template<typename T, typename D>
	void GetStructData( T *pStruct, ptrdiff_t nOffs, D data, size_t unSize )
	{
		V_memcpy( (void *)( (intptr_t)pStruct + nOffs ), data, unSize );
	}

	template<typename T>
	void GetStruct( T *pStruct, HSCRIPT hScope )
	{
		ScriptStructDesc_t *pDesc = pStruct->GetScriptDesc();

		FOR_EACH_VEC(pDesc->m_MemberBindings, idx)
		{
			ScriptStructMemberBinding_t const &binding = pDesc->m_MemberBindings[idx];

			ScriptVariant_t res;
			if( GetVM()->GetValue( hScope, binding.m_pszScriptName, &res ) )
			{
				switch ( binding.m_nMemberType )
				{
					case FIELD_VECTOR:
					{
						Vector newVec( res.m_pVector->x, res.m_pVector->y, res.m_pVector->z );
						GetStructData( pStruct, binding.m_unMemberOffs, newVec );
						break;
					}
					case FIELD_CSTRING:
					{
						string_t iNewString = AllocPooledString( res.m_pszString );
						GetStructData( pStruct, binding.m_unMemberOffs, STRING( iNewString ), binding.m_unMemberSize );
						break;
					}
					case FIELD_BOOLEAN:
					{
						GetStructData( pStruct, binding.m_unMemberOffs, res.m_bool );
						break;
					}
					case FIELD_INTEGER:
					{
						GetStructData( pStruct, binding.m_unMemberOffs, res.m_int );
						break;
					}
					case FIELD_FLOAT:
					{
						GetStructData( pStruct, binding.m_unMemberOffs, res.m_float );
						break;
					}
					default:
					{
						DevWarning( "Unsupported data type (%s) hit building struct data\n", ScriptFieldTypeName( binding.m_nMemberType ) );
						break;
					}
				}

				GetVM()->ReleaseValue( res );
			}
		}
	}

private:
	CScriptArgArray m_vecPushedArgs;
	CScriptFuncMap m_FuncMap;
};

// Pulled from scripts identically to parsing a weapon file
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

#if defined(CLIENT_DLL)
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif

private:
#if defined( GAME_DLL )
	CScriptedWeaponScope m_ScriptScope;
#endif
	CNetworkVarEmbedded( CScriptedWeaponData, m_WeaponData );
	CNetworkVar( int, m_nWeaponDataChanged );
#if defined(CLIENT_DLL)
	int m_nWeaponDataChangedOld;
#endif
};

#endif
