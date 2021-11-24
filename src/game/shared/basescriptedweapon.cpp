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

IMPLEMENT_NETWORKCLASS_ALIASED( BaseScriptedWeapon, DT_BaseScriptedWeapon )

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CBaseScriptedWeapon )
END_PREDICTION_DATA()
#endif

BEGIN_NETWORK_TABLE( CBaseScriptedWeapon, DT_BaseScriptedWeapon )
#if defined(CLIENT_DLL)

#else

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
	BaseClass::Precache();

#if defined( GAME_DLL )
	// Setup our script ID
	ValidateScriptScope();

	m_iszVScripts = AllocPooledString( GetWeaponScriptName() );

	m_ScriptScope.Init( STRING( m_iszScriptId ) );
	VScriptRunScript( STRING( m_iszVScripts ), m_ScriptScope, true );
#endif

	m_ScriptScope.CallFunc( "Precache");
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

	m_ScriptScope.CallFunc( "Spawn" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::PrimaryAttack()
{
	m_ScriptScope.CallFunc( "PrimaryAttack" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::SecondaryAttack()
{
	m_ScriptScope.CallFunc( "SecondaryAttack" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanDeploy( void )
{
	bool bResult = true;

	m_ScriptScope.CallFunc( "CanDeploy", &bResult);

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Deploy( void )
{
	bool bResult = BaseClass::Deploy();

	m_ScriptScope.CallFunc( "Deploy", &bResult);

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::CanHolster( void )
{
	bool bResult = true;

	m_ScriptScope.CallFunc( "CanHolster", &bResult);

	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseScriptedWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	bool bResult = BaseClass::Holster( pSwitchingTo );

	m_ScriptScope.PushArg( ToHScript( pSwitchingTo ) );
	m_ScriptScope.CallFunc( "Holster", &bResult );

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

	m_ScriptScope.PushArg( iOldState );
	m_ScriptScope.CallFunc( "OnActiveStateChanged" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_ScriptScope.SetValue( "owner", ToHScript( pOwner ) );

	m_ScriptScope.CallFunc( "Equip" );
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

	m_ScriptScope.CallFunc( "Detach" );

	m_ScriptScope.SetValue( "owner", INVALID_HSCRIPT );
}

char const *CBaseScriptedWeapon::GetWeaponScriptName()
{
#if defined( USES_ECON_ITEMS )
	// If were we Econ generated then setup our weapon name using it,
	// else rely on the mapper to name their entities correctly
	CEconItemDefinition *pItemDef = GetItem()->GetStaticData();
	if ( pItemDef )
	{
		if ( pItemDef->GetVScriptName() )
			return pItemDef->GetVScriptName();
	}
#endif

	return GetClassname();
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::HandleAnimEvent( animevent_t *pEvent )
{
	BaseClass::HandleAnimEvent( pEvent );

	m_ScriptScope.PushArg( pEvent->event, 
						   pEvent->type, 
						   pEvent->options, 
						   pEvent->eventtime, 
						   ToHScript( pEvent->pSource ) );
	m_ScriptScope.CallFunc( "HandleAnimEvent");
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseScriptedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
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
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szPrintName, "printname", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szViewModel, "viewmodel", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szWorldModel, "playermodel", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAnimationPrefix, "anim_prefix", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iSlot, "bucket", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iPosition, "bucket_position", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iMaxClip1, "clip_size", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iMaxClip2, "clip_size2", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iWeight, "weight", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iRumbleEffect, "rumble", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_INTEGER, iFlags, "item_flags", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bShowUsageHint, "showusagehint", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bAutoSwitchTo, "autoswitchto", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, bAutoSwitchFrom, "autoswitchfrom", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bAllowFlipping, "AllowFlipping", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bBuiltRightHanded, "BuiltRightHanded", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_BOOLEAN, m_bMeleeWeapon, "MeleeWeapon", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAmmo1, "primary_ammo", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, szAmmo2, "secondary_ammo", "" )
END_STRUCT_SCRIPTDESC()


BEGIN_STRUCT_SCRIPTDESC( ScriptShootSound_t, "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[0], "empty", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[1], "single_shot", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[2], "single_shot_npc", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[3], "double_shot", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[4], "double_shot_npc", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[5], "burst", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[6], "reload", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[7], "reload_npc", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[8], "melee_miss", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[9], "melee_hit", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[10], "melee_hit_world", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[11], "special1", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[12], "special2", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[13], "special3", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[14], "taunt", "" )
	DEFINE_STRUCT_MEMBER_NAMED( FIELD_CSTRING, aShootSounds[15], "deploy", "" )
END_STRUCT_SCRIPTDESC()
