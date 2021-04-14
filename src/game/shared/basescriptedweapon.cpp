#include "cbase.h"
#include "npcevent.h"
#include "ammodef.h"
#include "weapon_parse.h"
#include "basescriptedweapon.h"

typedef struct
{
	const char *m_pFlagName;
	int m_iFlagValue;
} itemFlags_t;

BEGIN_NETWORK_TABLE_NOBASE( CScriptedWeaponData, DT_ScriptedWeaponData )
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( BaseScriptedWeapon, DT_BaseScriptedWeapon )

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CBaseScriptedWeapon )
END_PREDICTION_DATA()
#endif

BEGIN_NETWORK_TABLE( CBaseScriptedWeapon, DT_BaseScriptedWeapon )
#if defined(CLIENT_DLL)
	RecvPropDataTable( RECVINFO_DT( m_WeaponData ), 0, &REFERENCE_RECV_TABLE( DT_ScriptedWeaponData ) ),
	RecvPropInt( RECVINFO( m_nWeaponDataChanged ) ),
#else
	SendPropDataTable( SENDINFO_DT( m_WeaponData ), &REFERENCE_SEND_TABLE( DT_ScriptedWeaponData ) ),
	SendPropInt( SENDINFO( m_nWeaponDataChanged ), 2 ),
#endif
END_NETWORK_TABLE()

BEGIN_ENT_SCRIPTDESC( CBaseScriptedWeapon, CBaseCombatWeapon, "The base for all custom scripted weapons" )
	DEFINE_SCRIPTFUNC( IsWeaponVisible, "" )
#if defined( GAME_DLL )
	DEFINE_SCRIPTFUNC( SetWeaponVisible, "" )
	DEFINE_SCRIPTFUNC( PrimaryAttack, "" )
	DEFINE_SCRIPTFUNC( SecondaryAttack, "" )
	DEFINE_SCRIPTFUNC( Deploy, "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptHolster, "Holster", "" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptEquip, "Equip", "" )
#endif
END_SCRIPTDESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseScriptedWeapon::CBaseScriptedWeapon()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseScriptedWeapon::~CBaseScriptedWeapon()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Precache()
{
#if defined( USES_ECON_ITEMS )
	// If were we Econ generated then setup our weapon name using it,
	// else rely on the mapper to name their entities correctly
	CEconItemDefinition *pItemDef = GetItem()->GetStaticData();
	if ( pItemDef )
	{
		if ( pItemDef->GetVScriptName() )
			SetClassname( pItemDef->GetVScriptName() );
	}
#endif

#if defined( GAME_DLL )
	// Setup our script ID
	ValidateScriptScope();

	m_iszVScripts = AllocPooledString( GetClassname() );

	m_ScriptScope.Init( STRING( m_iszScriptId ) );
	VScriptRunScript( STRING( m_iszVScripts ), m_ScriptScope, true );
#endif

	// ReadWeaponDataFromFileForSlot *will* fail due to the client not having the file
	// but it will still create the handle, so we'll use that to manually populate
	// a weapon info for this scripted classname
	WEAPON_FILE_INFO_HANDLE hHandle;
	ReadWeaponDataFromFileForSlot( filesystem, GetClassname(), &hHandle, GetEncryptionKey() );
	Assert( hHandle && hHandle != 0xFFFF );

#if defined( GAME_DLL )
	m_WeaponData.BInit( m_ScriptScope, GetFileWeaponInfoFromHandle( hHandle ) );
	m_nWeaponDataChanged = ( m_nWeaponDataChanged + 1 ) & ( ( 1 << 2 ) - 1 );

	BaseClass::Precache();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Spawn()
{
#if defined( USES_ECON_ITEMS )
	InitializeAttributes();
#endif

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::PrimaryAttack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::SecondaryAttack()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanDeploy( void )
{
	bool bResult = true;
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Deploy( void )
{
	bool bResult = BaseClass::Deploy();
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanHolster( void )
{
	bool bResult = true;
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	bool bResult = BaseClass::Holster( pSwitchingTo );
	return bResult;
}

bool CBaseScriptedWeapon::ScriptHolster( HSCRIPT pSwitchingTo )
{
	return Holster( (CBaseCombatWeapon *)ToEnt( pSwitchingTo ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnActiveStateChanged( int iOldState )
{
	BaseClass::OnActiveStateChanged( iOldState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

#if defined( GAME_DLL )
	m_ScriptScope.SetValue( "owner", ToHScript( pOwner ) );
#endif
}

void CBaseScriptedWeapon::ScriptEquip( HSCRIPT pOwner )
{
	Equip( (CBaseCombatCharacter *)ToEnt( pOwner ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Detach()
{
	BaseClass::Detach();

#if defined( GAME_DLL )
	m_ScriptScope.SetValue( "owner", INVALID_HSCRIPT );
#endif
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );
}
#else
void CBaseScriptedWeapon::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	m_nWeaponDataChangedOld = m_nWeaponDataChanged;
}

void CBaseScriptedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_nWeaponDataChanged != m_nWeaponDataChangedOld )
	{
		WEAPON_FILE_INFO_HANDLE hHandle = LookupWeaponInfoSlot( GetClassname() );
		Assert( hHandle && hHandle != 0xFFFF );

		*GetFileWeaponInfoFromHandle( hHandle ) = m_WeaponData;

		// setup our data with a late precache
		BaseClass::Precache();
	}
}
#endif


IScriptVM *CScriptedWeaponScopeBase::m_pVM = NULL;

CScriptedWeaponScope::CScriptedWeaponScope()
{
	m_FuncMap.SetLessFunc( StringLessThan );
}

CScriptedWeaponScope::~CScriptedWeaponScope()
{
	FOR_EACH_VEC_BACK( m_vecPushedArgs, i )
	{
		m_vecPushedArgs[i].Free();
	}
	m_vecPushedArgs.Purge();

	Term();
}


BEGIN_STRUCT_SCRIPTDESC( ScriptWeaponInfo_t, "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szPrintName, "printname" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szViewModel, "viewmodel" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szWorldModel, "playermodel" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAnimationPrefix, "anim_prefix" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iSlot, "bucket" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iPosition, "bucket_position" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iMaxClip1, "clip_size" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iMaxClip2, "clip_size2" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iWeight, "weight" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iRumbleEffect, "rumble" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iFlags, "item_flags" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bShowUsageHint, "showusagehint" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bAutoSwitchTo, "autoswitchto" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bAutoSwitchFrom, "autoswitchfrom" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bAllowFlipping, "AllowFlipping" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bBuiltRightHanded, "BuiltRightHanded" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bMeleeWeapon, "MeleeWeapon" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAmmo1, "primary_ammo" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAmmo2, "secondary_ammo" )
END_STRUCT_SCRIPTDESC()

struct ScriptShootSound_t
{
	DECLARE_STRUCT_SCRIPTDESC();

	ScriptShootSound_t() { V_memset( aShootSounds, 0, sizeof( aShootSounds ) ); }
	char aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];
};
BEGIN_STRUCT_SCRIPTDESC( ScriptShootSound_t, "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[0], "empty" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[1], "single_shot" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[2], "single_shot_npc" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[3], "double_shot" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[4], "double_shot_npc" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[5], "burst" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[6], "reload" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[7], "reload_npc" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[8], "melee_miss" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[9], "melee_hit" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[10], "melee_hit_world" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[11], "special1" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[12], "special2" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[13], "special3" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[14], "taunt" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[15], "deploy" )
END_STRUCT_SCRIPTDESC()

bool CScriptedWeaponData::BInit( CScriptedWeaponScope &scope, FileWeaponInfo_t *pWeaponInfo )
{
	if( !scope.IsInitialized() )
		return false;

	m_pWeaponInfo = (ScriptWeaponInfo_t *)pWeaponInfo;

	ScriptVariant_t info;
	scope.GetValue( "WeaponData", &info );

	if ( info.m_hScript == NULL || info.m_hScript == INVALID_HSCRIPT )
	{
		ConColorMsg( COLOR_RED, "No WeaponData table defined in weapon script, weapon won't function properly!\n" );
		return false;
	}

	scope.GetStruct( m_pWeaponInfo, info.m_hScript );
	m_pWeaponInfo->bParsedScript = true;

	if ( Q_strcmp( "None", m_pWeaponInfo->szAmmo1 ) == 0 )
		m_pWeaponInfo->szAmmo1[0] = '\0';
	if ( Q_strcmp( "None", m_pWeaponInfo->szAmmo2 ) == 0 )
		m_pWeaponInfo->szAmmo2[0] = '\0';

	m_pWeaponInfo->iAmmoType = GetAmmoDef()->Index( m_pWeaponInfo->szAmmo1 );
	m_pWeaponInfo->iAmmo2Type = GetAmmoDef()->Index( m_pWeaponInfo->szAmmo2 );

	ScriptVariant_t sounds;
	if( scope.GetVM()->GetValue( info, "SoundData", &sounds ) )
	{
		ScriptShootSound_t weaponSounds;
		scope.GetStruct( &weaponSounds, sounds.m_hScript );

		V_memcpy( m_pWeaponInfo->aShootSounds, weaponSounds.aShootSounds, NUM_SHOOT_SOUND_TYPES*MAX_WEAPON_STRING );

		scope.ReleaseValue( sounds );
	}

	ScriptVariant_t flags;
	if ( scope.GetVM()->GetValue( info, "item_flags", &flags ) )
	{
		if ( flags.m_type == FIELD_CSTRING )
		{
			CUtlStringList bits;
			V_SplitString( flags.m_pszString, "|", bits );

			extern itemFlags_t g_ItemFlags[8];

			for( int i=0; i<bits.Count(); ++i )
			{
				for ( int j=0; j<ARRAYSIZE( g_ItemFlags ); ++j )
				{
					if ( Q_strcmp( bits[i], g_ItemFlags[j].m_pFlagName ) == 0 )
						m_pWeaponInfo->iFlags |= g_ItemFlags[j].m_iFlagValue;
				}
			}
		}

		scope.ReleaseValue( flags );
	}

	scope.ReleaseValue( info );

	NetworkStateChanged();

	return true;
}
