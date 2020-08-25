//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_WeaponGrenadeLauncher )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_WeaponGrenadeLauncher )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CTFGrenadeLauncher)
DEFINE_FIELD(m_flChargeBeginTime, FIELD_FLOAT)
END_PREDICTION_DATA()
#else
BEGIN_PREDICTION_DATA(CTFGrenadeLauncher)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );

CREATE_SIMPLE_WEAPON_TABLE( TFGrenadeLauncher_Cannon, tf_weapon_cannon )

//=============================================================================

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFGrenadeLauncher )
END_DATADESC()
#endif

#define TF_GRENADE_LAUNCER_MIN_VEL 1200
#define TF_GRENADES_SWITCHGROUP 2 
#define TF_GRENADE_BARREL_SPIN 0.25 // barrel increments by one quarter for each pill

extern ConVar tf2v_console_grenadelauncher_magazine;

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;
	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
#ifdef CLIENT_DLL
	if (m_pCannonFuse)
	ToggleCannonFuse();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_flChargeBeginTime = 0;
	StopSound("Weapon_LooseCannon.Charge");

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponReset(void)
{
	BaseClass::WeaponReset();

	m_flChargeBeginTime = 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetMaxClip1( void ) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetMaxClip1() but with the base set to TF_GRENADE_LAUNCHER_XBOX_CLIP.
	// We need to do it this way in order to consider attributes.
	if ( tf2v_console_grenadelauncher_magazine.GetBool() )
	{
		
		int iMaxClip = TF_GRENADE_LAUNCHER_XBOX_CLIP;
		if ( iMaxClip < 0 )
			return iMaxClip;

		CALL_ATTRIB_HOOK_FLOAT( iMaxClip, mult_clipsize );
		if ( iMaxClip < 0 )
			return iMaxClip;

		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner == NULL )
			return iMaxClip;

		int nClipSizePerKill = 0;
		CALL_ATTRIB_HOOK_INT( nClipSizePerKill, clipsize_increase_on_kill );

		iMaxClip += Min( nClipSizePerKill, pOwner->m_Shared.GetStrikeCount() );

		return iMaxClip;

	}

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetDefaultClip1( void ) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetDefaultClip1() is just checking GetMaxClip1(), nothing fancy to do here.
	if ( tf2v_console_grenadelauncher_magazine.GetBool() )
		return GetMaxClip1();
	 
	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		m_flChargeBeginTime = 0;
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if (IsMortar())
	{
		if (m_flChargeBeginTime <= 0)
		{
			// save that we had the attack button down
			m_flChargeBeginTime = gpGlobals->curtime;
			// Turn on the cannon fuse.
#ifdef CLIENT_DLL
			if (!m_pCannonFuse)
			ToggleCannonFuse();
#endif
			EmitSound("Weapon_LooseCannon.Charge");

			SendWeaponAnim(ACT_VM_PULLBACK);

		}
		else
		{
			// Check how long we've been holding down the charge.
			float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;

			// Held too long, blow yourself up dummy!
			if (flTotalChargeTime >= MortarTime())
			{
				Overload();
				// Reset charging time.
				m_flChargeBeginTime = 0;
#ifdef CLIENT_DLL
				if (m_pCannonFuse)
					ToggleCannonFuse();
#endif
				StopSound("Weapon_LooseCannon.Charge");

				// Set next attack times.
				float flDelay = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;
				CALL_ATTRIB_HOOK_FLOAT(flDelay, mult_postfiredelay);
				m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;
			}
		}
	}
	else
		LaunchGrenade();

	SwitchBodyGroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponIdle( void )
{
	if (IsMortar() && ( m_flChargeBeginTime > 0 && m_iClip1 > 0) )
	{

#ifdef CLIENT_DLL
		if (m_pCannonFuse)
			ToggleCannonFuse();
#endif
		StopSound("Weapon_LooseCannon.Charge");

		LaunchGrenade();

	}
	else
		BaseClass::WeaponIdle();
}

CBaseEntity *CTFGrenadeLauncher::FireProjectileInternal( CTFPlayer *pPlayer )
{
	CTFWeaponBaseGrenadeProj *pGrenade = (CTFWeaponBaseGrenadeProj *)FireProjectile( pPlayer );
	if ( pGrenade )
	{
#ifdef GAME_DLL
		// Mortar weapons have a custom detonator time.
		if (IsMortar())
			pGrenade->SetDetonateTimerLength(gpGlobals->curtime - m_flChargeBeginTime );

		/*if ( GetDetonateMode() == TF_GL_MODE_FIZZLE )
			pGrenade->m_bFizzle = true;*/
#endif
	}
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	FireProjectileInternal(pPlayer);

#if 0
	CBaseEntity *pPipeBomb = 

	if (pPipeBomb)
	{
#ifdef GAME_DLL
		// If we've gone over the max pipebomb count, detonate the oldest
		if (m_Pipebombs.Count() >= TF_WEAPON_PIPEBOMB_COUNT)
		{
			CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[0];
			if (pTemp)
			{
				pTemp->SetTimer(gpGlobals->curtime); // explode NOW
			}

			m_Pipebombs.Remove(0);
		}

		CTFGrenadePipebombProjectile *pPipebomb = (CTFGrenadePipebombProjectile*)pProjectile;
		pPipebomb->SetLauncher(this);

		PipebombHandle hHandle;
		hHandle = pPipebomb;
		m_Pipebombs.AddToTail(hHandle);

		m_iPipebombCount = m_Pipebombs.Count();
#endif
	}
#endif

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}

float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	float flVelocity = TF_GRENADE_LAUNCER_MIN_VEL;

	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	return flVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SecondaryAttack( void )
{
	BaseClass::SecondaryAttack();
}

bool CTFGrenadeLauncher::Reload( void )
{
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: Change model state to reflect available pills in launcher
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SwitchBodyGroups( void )
{
	if ( GetNumBodyGroups() < TF_GRENADES_SWITCHGROUP )
		return; 

    int iState = 4;

    iState = m_iClip1;

    SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
	SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );

    CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );

    if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
    {
		CBaseViewModel *vm = pTFPlayer->GetViewModel( m_nViewModelIndex );
        if ( vm )
        {
            vm->SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
			vm->SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );
        }
    }
}

int CTFGrenadeLauncher::GetDetonateMode( void ) const
{
	int nDetonateMode = 0;
	CALL_ATTRIB_HOOK_INT( nDetonateMode, set_detonate_mode );
	return nDetonateMode;
}


bool CTFGrenadeLauncher::IsMortar(void) const
{
	int nMortarMode = 0;
	CALL_ATTRIB_HOOK_INT(nMortarMode, grenade_launcher_mortar_mode);
	return ( nMortarMode != 0 );
}

int CTFGrenadeLauncher::MortarTime(void)
{
	int nMortarMode = 0;
	CALL_ATTRIB_HOOK_INT(nMortarMode, grenade_launcher_mortar_mode);
	return (nMortarMode);
}

#ifdef CLIENT_DLL
void CTFGrenadeLauncher::ToggleCannonFuse()
{
	if (!m_pCannonFuse)
	{
		m_pCannonFuse = ParticleProp()->Create("loose_cannon_sparks", PATTACH_POINT_FOLLOW, "cannon_fuse");
	}
	else if (m_pCannonFuse)
	{
		ParticleProp()->StopEmission(m_pCannonFuse);
		m_pCannonFuse = NULL;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher_Cannon::Precache(void)
{
	PrecacheScriptSound("Weapon_LooseCannon.Charge");

	BaseClass::Precache();
}

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeFrag, DT_WeaponGrenadeFrag)

BEGIN_NETWORK_TABLE(CTFGrenadeFrag, DT_WeaponGrenadeFrag)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeFrag)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_frag, CTFGrenadeFrag);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_frag);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeFrag::CTFGrenadeFrag()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeFrag::~CTFGrenadeFrag()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeFrag::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeFrag::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeFrag::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeFrag::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeFrag::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeFrag::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeFrag::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeFrag::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeFrag::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeFrag::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeFrag::Reload(void)
{
	return BaseClass::Reload();
}

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeCaltrop, DT_WeaponGrenadeCaltrop)

BEGIN_NETWORK_TABLE(CTFGrenadeCaltrop, DT_WeaponGrenadeCaltrop)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeCaltrop)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_caltrop, CTFGrenadeCaltrop);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_caltrop);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeCaltrop::CTFGrenadeCaltrop()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeCaltrop::~CTFGrenadeCaltrop()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeCaltrop::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeCaltrop::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeCaltrop::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeCaltrop::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeCaltrop::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeCaltrop::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeCaltrop::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeCaltrop::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeCaltrop::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeCaltrop::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeCaltrop::Reload(void)
{
	return BaseClass::Reload();
}

//Grenade CONCUSSION

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeConcussion, DT_WeaponGrenadeConcussion)

BEGIN_NETWORK_TABLE(CTFGrenadeConcussion, DT_WeaponGrenadeConcussion)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeConcussion)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_concussion, CTFGrenadeConcussion);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_concussion);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeConcussion::CTFGrenadeConcussion()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeConcussion::~CTFGrenadeConcussion()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeConcussion::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeConcussion::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeConcussion::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeConcussion::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeConcussion::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeConcussion::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeConcussion::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeConcussion::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeConcussion::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeConcussion::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeConcussion::Reload(void)
{
	return BaseClass::Reload();
}

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeEmp, DT_WeaponGrenadeEmp)

BEGIN_NETWORK_TABLE(CTFGrenadeEmp, DT_WeaponGrenadeEmp)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeEmp)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_emp, CTFGrenadeEmp);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_emp);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeEmp::CTFGrenadeEmp()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeEmp::~CTFGrenadeEmp()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeEmp::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeEmp::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeEmp::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeEmp::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeEmp::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeEmp::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeEmp::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeEmp::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeEmp::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeEmp::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeEmp::Reload(void)
{
	return BaseClass::Reload();
}

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeSmokeBomb, DT_WeaponGrenadeSmokeBomb)

BEGIN_NETWORK_TABLE(CTFGrenadeSmokeBomb, DT_WeaponGrenadeSmokeBomb)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeSmokeBomb)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_smoke_bomb, CTFGrenadeSmokeBomb);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_smoke_bomb);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeSmokeBomb::CTFGrenadeSmokeBomb()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeSmokeBomb::~CTFGrenadeSmokeBomb()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBomb::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeSmokeBomb::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeSmokeBomb::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeSmokeBomb::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeSmokeBomb::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBomb::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBomb::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBomb::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeSmokeBomb::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeSmokeBomb::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeSmokeBomb::Reload(void)
{
	return BaseClass::Reload();
}

#define GRENADE_HEAL_TIMER	1.0f // seconds

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFHealGrenade, DT_WeaponHealGrenade)

BEGIN_NETWORK_TABLE(CTFHealGrenade, DT_WeaponHealGrenade)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFHealGrenade)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_heal_grenade, CTFHealGrenade);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_heal_grenade);

#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFHealGrenade::CTFHealGrenade()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFHealGrenade::~CTFHealGrenade()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealGrenade::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFHealGrenade::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFHealGrenade::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFHealGrenade::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFHealGrenade::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealGrenade::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealGrenade::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHealGrenade::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFHealGrenade::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFHealGrenade::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFHealGrenade::Reload(void)
{
	return BaseClass::Reload();
}

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeMirv, DT_WeaponGrenadeMirv)

BEGIN_NETWORK_TABLE(CTFGrenadeMirv, DT_WeaponGrenadeMirv)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeMirv)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_mirv, CTFGrenadeMirv);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_mirv);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeMirv::CTFGrenadeMirv()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeMirv::~CTFGrenadeMirv()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeMirv::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeMirv::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeMirv::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeMirv::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeMirv::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeMirv::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeMirv::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeMirv::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeMirv::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeMirv::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeMirv::Reload(void)
{
	return BaseClass::Reload();
}

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeNail, DT_WeaponGrenadeNail)

BEGIN_NETWORK_TABLE(CTFGrenadeNail, DT_WeaponGrenadeNail)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeNail)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_nail, CTFGrenadeNail);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_nail);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeNail::CTFGrenadeNail()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeNail::~CTFGrenadeNail()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNail::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeNail::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeNail::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeNail::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeNail::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNail::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNail::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNail::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeNail::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeNail::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeNail::Reload(void)
{
	return BaseClass::Reload();
}

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeNapalm, DT_WeaponGrenadeNapalm)

BEGIN_NETWORK_TABLE(CTFGrenadeNapalm, DT_WeaponGrenadeNapalm)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeNapalm)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_napalm, CTFGrenadeNapalm);
PRECACHE_WEAPON_REGISTER(tf_weaponbased_grenade_napalm);


#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeNapalm::CTFGrenadeNapalm()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeNapalm::~CTFGrenadeNapalm()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNapalm::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeNapalm::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeNapalm::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeNapalm::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeNapalm::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNapalm::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNapalm::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeNapalm::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeNapalm::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeNapalm::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeNapalm::Reload(void)
{
	return BaseClass::Reload();
}

//=============================================================================
//
// Weapon Grenade Launcher tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED(TFGrenadeGas, DT_WeaponGrenadeGas)

BEGIN_NETWORK_TABLE(CTFGrenadeGas, DT_WeaponGrenadeGas)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFGrenadeGas)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weaponbased_grenade_gas, CTFGrenadeGas);
PRECACHE_WEAPON_REGISTER(tf_weapon_grenade_gas);



#define TF_GRENADE_LAUNCER_MIN_VEL 1200

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeGas::CTFGrenadeGas()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeGas::~CTFGrenadeGas()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeGas::Spawn(void)
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeGas::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeGas::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeGas::GetMaxClip1(void) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeGas::GetDefaultClip1(void) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeGas::PrimaryAttack(void)
{
	// Check for ammunition.
	if (m_iClip1 <= 0 && m_iClip1 != -1)
		return;

	// Are we capable of firing again?
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return;

	if (!CanAttack())
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	LaunchGrenade();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeGas::WeaponIdle(void)
{
	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeGas::LaunchGrenade(void)
{
	// Get the player owning the weapon.
	CTFPlayer* pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	CalcIsAttackCritical();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireProjectile(pPlayer);

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon(pPlayer, IsCurrentAttackACrit());
#endif

	// Set next attack times.
	m_flNextPrimaryAttack = gpGlobals->curtime + m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_flTimeFireDelay;

	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

	// Check the reload mode and behave appropriately.
	if (m_bReloadsSingly)
	{
		m_iReloadMode.Set(TF_RELOAD_START);
	}
}

float CTFGrenadeGas::GetProjectileSpeed(void)
{
	return TF_GRENADE_LAUNCER_MIN_VEL;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeGas::SecondaryAttack(void)
{
#ifdef GAME_DLL

	if (!CanAttack())
		return;

	CTFPlayer* pOwner = ToTFPlayer(GetOwner());
	pOwner->DoClassSpecialSkill();

#endif
}

bool CTFGrenadeGas::Reload(void)
{
	return BaseClass::Reload();
}


