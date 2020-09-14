//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:			The "weapon" used to build objects
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "entitylist.h"
#include "in_buttons.h"
#include "tf_obj.h"
#include "sendproxy.h"
#include "tf_weapon_builder.h"
#include "vguiscreen.h"
#include "tf_gamerules.h"
#include "tf_obj_teleporter.h"

extern ConVar tf2_object_hard_limits;
extern ConVar tf_fastbuild;

EXTERN_SEND_TABLE(DT_BaseCombatWeapon)

BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBuilder, DT_BuilderLocalData )
	SendPropInt( SENDINFO( m_iObjectType ), BUILDER_OBJECT_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hObjectBeingBuilt ) ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST(CTFWeaponBuilder, DT_TFWeaponBuilder)
	SendPropInt( SENDINFO( m_iBuildState ), 4, SPROP_UNSIGNED ),
	SendPropDataTable( "BuilderLocalData", 0, &REFERENCE_SEND_TABLE( DT_BuilderLocalData ), SendProxy_SendLocalWeaponDataTable ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_builder, CTFWeaponBuilder );
PRECACHE_WEAPON_REGISTER( tf_weapon_builder );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder::CTFWeaponBuilder()
{
	m_iObjectType.Set( BUILDER_INVALID_OBJECT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder::~CTFWeaponBuilder()
{
	StopPlacement();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetSubType( int iSubType )
{
	m_iObjectType = iSubType;

	BaseClass::SetSubType( iSubType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetObjectMode( int iObjectMode )
{
	m_iObjectMode = iObjectMode;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::Precache( void )
{
	BaseClass::Precache();

	// Precache all the viewmodels we could possibly be building
	for ( int iObj=0; iObj < OBJ_LAST; iObj++ )
	{
		const CObjectInfo *pInfo = GetObjectInfo( iObj );

		if ( pInfo )
		{
			if ( pInfo->m_pViewModel )
			{
				PrecacheModel( pInfo->m_pViewModel );
			}

			if ( pInfo->m_pPlayerModel )
			{
				PrecacheModel( pInfo->m_pPlayerModel );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::CanDeploy( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if (!pPlayer)
		return false;

	if ( pPlayer->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
	{
		return false;
	}

	return BaseClass::CanDeploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		SetCurrentState( BS_PLACING );
		StartPlacement(); 
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.35f;
		m_flNextSecondaryAttack = gpGlobals->curtime;		// asap

		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if (!pPlayer)
			return false;

		pPlayer->SetNextAttack( gpGlobals->curtime );

		m_iViewModelIndex = modelinfo->GetModelIndex( GetViewModel(0) );
		m_iWorldModelIndex = modelinfo->GetModelIndex( GetWorldModel() );

		m_flNextDenySound = 0;

		// Set off the hint here, because we don't know until now if our building
		// is rotate-able or not.
		if ( m_hObjectBeingBuilt && !m_hObjectBeingBuilt->MustBeBuiltOnAttachmentPoint() )
		{
			// set the alt-fire hint so it gets removed when we holster
			m_iAltFireHint = HINT_ALTFIRE_ROTATE_BUILDING;
			pPlayer->StartHintTimer( m_iAltFireHint );
		}

		if ( m_hObjectBeingBuilt && m_hObjectBeingBuilt->IsBeingCarried() )
		{
			// We just pressed attack2, don't immediately rotate it.
			m_bInAttack2 = true;
		}
	}

	return bDeploy;
}

Activity CTFWeaponBuilder::GetDrawActivity( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );

	// Use the one handed sapper deploy if we're invisible.
	if ( pOwner && GetType() == OBJ_ATTACHMENT_SAPPER && pOwner->m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		return ACT_VM_DRAW_DEPLOYED;
	}
	else
	{
		return BaseClass::GetDrawActivity();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::CanHolster( void ) const
{
	// If player is hauling a building he can't switch away without dropping it.
	CTFPlayer *pOwner = GetTFPlayerOwner();

	if ( pOwner && pOwner->m_Shared.IsCarryingObject() )
	{
		return false;
	}

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: Stop placement when holstering
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false; 

	if ( pOwner->m_Shared.IsCarryingObject() )
		return false;

	if ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID )
	{
		SetCurrentState( BS_IDLE );
	}

	StopPlacement();

	// Make sure hauling status is cleared.
	pOwner->m_Shared.SetCarriedObject( NULL );

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// If we're building, and our team has lost, stop placing the object
	if ( m_hObjectBeingBuilt.Get() && 
		TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		pOwner->GetTeamNumber() != TFGameRules()->GetWinningTeam() )
	{
		StopPlacement();
		return;
	}

	// Check that I still have enough resources to build this item
	if ( pOwner->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
	{
		SwitchOwnersWeaponToLast();
	}

	if (( pOwner->m_nButtons & IN_ATTACK ) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
		{
			SecondaryAttack();
		}
	}
	else
	{
		m_bInAttack2 = false;
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: Start placing or building the currently selected object
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	// Necessary so that we get the latest building position for the test, otherwise
	// we are one frame behind.
	UpdatePlacementState();

	// What state should we move to?
	switch( m_iBuildState )
	{
	case BS_IDLE:
		{
			// Idle state starts selection
			SetCurrentState( BS_SELECTING );
		}
		break;

	case BS_SELECTING:
		{
			// Do nothing, client handles selection
			return;
		}
		break;

	case BS_PLACING:
		{
			if ( m_hObjectBeingBuilt )
			{
				int iFlags = m_hObjectBeingBuilt->GetObjectFlags();

				// Tricky, because this can re-calc the object position and change whether its a valid 
				// pos or not. Best not to do this only in debug, but we can be pretty sure that this
				// will give the same result as was calculated in UpdatePlacementState() above.
				Assert( IsValidPlacement() );

				// If we're placing an attachment, like a sapper, play a placement animation on the owner
				if ( m_hObjectBeingBuilt->MustBeBuiltOnAttachmentPoint() )
				{
					pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
				}

				// Need to save this for later since StartBuilding will clear m_hObjectBeingBuilt.
				CBaseObject *pParentObject = m_hObjectBeingBuilt->GetParentObject();

				if ( pParentObject && m_iObjectType == OBJ_ATTACHMENT_SAPPER )
				{
					m_vLastSapPos = pParentObject->GetAbsOrigin();
					m_hLastSappedBuilding = pParentObject;
				}

				StartBuilding();

				if ( GetType() == OBJ_ATTACHMENT_SAPPER )
				{
					pOwner->OnSapperPlaced( pParentObject );

					// Attaching a sapper to a teleporter automatically saps another end.
					CObjectTeleporter *pTeleporter = dynamic_cast<CObjectTeleporter *>( pParentObject );
					if ( pTeleporter )
					{
						CObjectTeleporter *pMatch = pTeleporter->GetMatchingTeleporter();

						// If the other end is not already sapped then place a sapper on it.
						if ( pMatch && !pMatch->IsPlacing() && !pMatch->HasSapper() )
						{
							SetCurrentState( BS_PLACING );
							StartPlacement();
							if ( m_hObjectBeingBuilt.Get() )
							{
								m_hObjectBeingBuilt->UpdateAttachmentPlacement( pMatch );
								StartBuilding();
							}
						}
					}
				}

				// Should we switch away?
				if ( iFlags & OF_ALLOW_REPEAT_PLACEMENT )
				{
					// Start placing another
					SetCurrentState( BS_PLACING );
					StartPlacement(); 
				}
				else
				{
					SwitchOwnersWeaponToLast();
				}
			}
		}
		break;

	case BS_PLACING_INVALID:
		{
			if ( m_flNextDenySound < gpGlobals->curtime )
			{
				CSingleUserRecipientFilter filter( pOwner );
				EmitSound( filter, entindex(), "Player.DenyWeaponSelection" );

				m_flNextDenySound = gpGlobals->curtime + 0.5;
			}
		}
		break;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
}

void CTFWeaponBuilder::SecondaryAttack( void )
{
	if ( m_bInAttack2 )
		return;

	// require a re-press
	m_bInAttack2 = true;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pOwner->DoClassSpecialSkill() )
	{
		// intentionally blank
	}
	else if ( m_iBuildState == BS_PLACING )
	{
		if ( m_hObjectBeingBuilt )
		{
			pOwner->StopHintTimer( HINT_ALTFIRE_ROTATE_BUILDING );
			m_hObjectBeingBuilt->RotateBuildAngles();
		}
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}

//-----------------------------------------------------------------------------
// Purpose: Set the builder to the specified state
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetCurrentState( int iState )
{
	m_iBuildState = iState;
}

//-----------------------------------------------------------------------------
// Purpose: Set the owner's weapon and last weapon appropriately when we need to
//			switch away from the builder weapon.  
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SwitchOwnersWeaponToLast()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// for engineer, switch to wrench and set last weapon appropriately
	if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// Switch to wrench if possible. if not, then best weapon
		CBaseCombatWeapon *pWpn = pOwner->Weapon_GetSlot( 2 );

		// Don't store last weapon when we autoswitch off builder
		CBaseCombatWeapon *pLastWpn = pOwner->GetLastWeapon();

		if ( pWpn )
		{
			pOwner->Weapon_Switch( pWpn );
		}
		else
		{
			pOwner->SwitchToNextBestWeapon( NULL );
		}

		if ( pWpn == pLastWpn )
		{
			// We had the wrench out before we started building. Go ahead and set out last
			// weapon to our primary weapon.
			pWpn = pOwner->Weapon_GetSlot( 0 );
			pOwner->Weapon_SetLast( pWpn );
		}
		else
		{
			pOwner->Weapon_SetLast( pLastWpn );
		}
	}
	else
	{
		// for all other classes, just switch to last weapon used
		pOwner->Weapon_Switch( pOwner->GetLastWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates the building postion and checks the new postion
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::UpdatePlacementState( void )
{
	// This updates the building position
	bool bValidPos = IsValidPlacement();

	// If we're in placement mode, update the placement model
	switch( m_iBuildState )
	{
	case BS_PLACING:
	case BS_PLACING_INVALID:
		{
			if ( bValidPos )
			{
				SetCurrentState( BS_PLACING );
			}
			else
			{
				SetCurrentState( BS_PLACING_INVALID );
			}
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Idle updates the position of the build placement model
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WeaponIdle( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start placing the object
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartPlacement( void )
{
	StopPlacement();

	if ( GetOwner() && ToTFPlayer( GetOwner() )->m_Shared.GetCarriedObject() )
	{
		m_hObjectBeingBuilt = ToTFPlayer( GetOwner() )->m_Shared.GetCarriedObject();
		m_hObjectBeingBuilt->StartPlacement( ToTFPlayer( GetOwner() ) );
		return;
	}

	// Create the slab
	m_hObjectBeingBuilt = (CBaseObject*)CreateEntityByName( GetObjectInfo( m_iObjectType )->m_pClassName );
	if ( m_hObjectBeingBuilt )
	{
		m_hObjectBeingBuilt->SetObjectMode( m_iObjectMode );
		m_hObjectBeingBuilt->SetBuilder( ToTFPlayer( GetOwner() ) );
		m_hObjectBeingBuilt->Spawn();
		m_hObjectBeingBuilt->StartPlacement( ToTFPlayer( GetOwner() ) );

		// Stomp this here in the same frame we make the object, so prevent clientside warnings that it's under attack
		m_hObjectBeingBuilt->m_iHealth = OBJECT_CONSTRUCTION_STARTINGHEALTH;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StopPlacement( void )
{
	if ( m_hObjectBeingBuilt )
	{
		m_hObjectBeingBuilt->StopPlacement();
		m_hObjectBeingBuilt = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WeaponReset( void )
{
	BaseClass::WeaponReset();

	StopPlacement();
}


//-----------------------------------------------------------------------------
// Purpose: Move the placement model to the current position. Return false if it's an invalid position
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::IsValidPlacement( void )
{
	if ( !m_hObjectBeingBuilt )
		return false;

	CBaseObject *pObj = m_hObjectBeingBuilt.Get();

	pObj->UpdatePlacement();

	return m_hObjectBeingBuilt->IsValidPlacement();
}

//-----------------------------------------------------------------------------
// Purpose: Player holding this weapon has started building something
// Assumes we are in a valid build position
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartBuilding( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	CBaseObject *pObj = m_hObjectBeingBuilt.Get();

	if ( pPlayer && pPlayer->m_Shared.IsCarryingObject() )
	{
		Assert( pObj );

		pObj->DropCarriedObject( pPlayer );
	}

	Assert( pObj );

	pObj->StartBuilding( GetOwner() );

	int nMiniSentry = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nMiniSentry, wrench_builds_minisentry );
	if ( nMiniSentry == 1 )
	{
		if ( GetType() == OBJ_SENTRYGUN )
		{
			pObj->MakeMiniBuilding();
		}
	}

	m_hObjectBeingBuilt = NULL;

	if ( pPlayer )
	{
		pPlayer->RemoveInvisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::HasAmmo( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	int iCost = pOwner->ModCalculateObjectCost(m_iObjectType, pOwner->HasGunslinger());
	return ( pOwner->GetBuildResources() >= iCost );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBuilder::GetSlot( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBuilder::GetPosition( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetPrintName( void ) const
{
	if ( GetObjectInfo( m_iObjectType )->m_AltModes.Count() > 0 )
		return GetObjectInfo( m_iObjectType )->m_AltModes.Element( m_iObjectMode * 3 + 0 );

	return GetObjectInfo( m_iObjectType )->m_pStatusName;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetViewModel( int iViewModel ) const
{
	if ( GetPlayerOwner() == NULL )
	{
		return BaseClass::GetViewModel();
	}

	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		return DetermineViewModelType( GetObjectInfo(m_iObjectType)->m_pViewModel );
	}

	return BaseClass::GetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetWorldModel( void ) const
{
	if ( GetPlayerOwner() == NULL )
	{
		return BaseClass::GetWorldModel();
	}

	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		return GetObjectInfo( m_iObjectType )->m_pPlayerModel;
	}

	return BaseClass::GetWorldModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::AllowsAutoSwitchTo( void ) const
{
	// ask the object we're building
	return GetObjectInfo( m_iObjectType )->m_bAutoSwitchTo;
}


IMPLEMENT_SERVERCLASS_ST( CTFWeaponSapper, DT_TFWeaponSapper )
	SendPropFloat( SENDINFO( m_flWheatleyTalkingUntil ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_sapper, CTFWeaponSapper );
PRECACHE_WEAPON_REGISTER( tf_weapon_sapper );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponSapper::CTFWeaponSapper()
{
	WheatleyReset( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponSapper::GetViewModel( int iViewModel ) const
{
	// Skip over Builder's version
	return CTFWeaponBase::GetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponSapper::GetWorldModel( void ) const	
{
	// Skip over Builder's version
	return CTFWeaponBase::GetWorldModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponSapper::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false; 

	if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER && IsWheatleySapper() )
	{
		pOwner->ResetSappingState();

		if ( pOwner->m_Shared.GetState() == TF_STATE_DYING )
		{
			if ( !RandomInt( 0, 4 ) )
				WheatleyEmitSound( "PSap.DeathLong", true );
			else
				WheatleyEmitSound( "PSap.Death", true );
		}
		else
		{
			float flSoundDuration;
			if ( ( gpGlobals->curtime - m_flWheatleyLastDeploy ) < 1.5 && ( gpGlobals->curtime - m_flWheatleyLastDeploy ) > -1.0 )
				flSoundDuration = WheatleyEmitSound( "PSap.HolsterFast");
			else
				flSoundDuration = WheatleyEmitSound( "PSap.Holster");

			m_flWheatleyLastHolster = gpGlobals->curtime + flSoundDuration;
		}
	}

	m_flWheatleyIdleTime = -1.0f;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponSapper::WeaponReset( void )
{
	if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER )
	{
		if ( IsWheatleySapper() )
		{
			CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
			if ( pOwner )
				pOwner->ResetSappingState();

			WheatleyReset();
		}
	}

	BaseClass::WeaponReset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponSapper::WeaponIdle( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	WheatleySapperIdle( pOwner );

	BaseClass::WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: Holding Wheatley?
//-----------------------------------------------------------------------------
bool CTFWeaponSapper::IsWheatleySapper( void )
{
	int nVoicePak = 0;
	CALL_ATTRIB_HOOK_INT( nVoicePak, sapper_voice_pak );

	return nVoicePak == 1;
}

//-----------------------------------------------------------------------------
// Purpose: TODO: Enum for this
//-----------------------------------------------------------------------------
void CTFWeaponSapper::SetWheatleyState( int iNewState )
{
	m_iWheatleyState = iNewState;
}

//-----------------------------------------------------------------------------
// Purpose: Special Item Reset
//-----------------------------------------------------------------------------
void CTFWeaponSapper::WheatleyReset( bool bResetIntro )
{
	if ( IsWheatleySapper() )
		WheatleyEmitSound( "PSap.null" );

	if ( bResetIntro )
		m_bWheatleyIntroPlayed = false;

	m_iWheatleyState = 0;
	m_flWheatleyTalkingUntil = 0;
	m_flWheatleyLastDamage = 0;
	m_iNextWheatleyVoiceLine = 0;
	m_flWheatleyIdleTime = -1.0f;
	m_flWheatleyLastDeploy = 0;
	m_flWheatleyLastHolster = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponSapper::WheatleyDamage( void )
{
	if ( ( gpGlobals->curtime - m_flWheatleyLastDamage ) > 10.0)
	{
		if ( RandomInt(0,2) == 0 )
		{
			CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
			if ( pOwner )
				pOwner->ClearSappingState();

			SetWheatleyState( 0 );
			m_flWheatleyLastDamage = gpGlobals->curtime;
			WheatleyEmitSound( "PSap.Damage" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFWeaponSapper::WheatleyEmitSound( const char *pSound, bool bEmitToAll, bool bNoRepeat )
{
	CSoundParameters params;
	if ( !GetParametersForSound( pSound, params, NULL ) )
		return 0;

	//if ( bNoRepeat && )

	if ( V_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle38.mp3") == 0 )
	{
		SetWheatleyState( 6 );
		m_iNextWheatleyVoiceLine = 0;
	}
	else if ( V_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle41.mp3") == 0 )
	{
		SetWheatleyState( 7 );
		m_iNextWheatleyVoiceLine = 0;
	}
	else if ( V_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle35.mp3") == 0 )
	{
		SetWheatleyState( 5 );
		m_iNextWheatleyVoiceLine = 0;
	}

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( bEmitToAll )
	{
		CBroadcastNonOwnerRecipientFilter filter( pOwner );
		EmitSound( filter, entindex(), pSound, &m_vLastSapPos );
	}

	CSingleUserRecipientFilter filter( pOwner );
	EmitSound( filter, entindex(), params );

	m_flWheatleyTalkingUntil = gpGlobals->curtime + 3.0f;

	return 3.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Random sound handling
//-----------------------------------------------------------------------------
void CTFWeaponSapper::WheatleySapperIdle( CTFPlayer *pOwner )
{
	// This is a doozy
	if ( !pOwner || m_iObjectType != OBJ_ATTACHMENT_SAPPER || !IsWheatleySapper() )
		return;
	
	if( m_flWheatleyIdleTime < 0.0f )
	{			
		pOwner->ResetSappingState();

		float flDuration;
		if ( ( gpGlobals->curtime - m_flWheatleyLastHolster ) < 2.0f && ( gpGlobals->curtime - m_flWheatleyLastHolster ) >= -1.0f )
			flDuration = WheatleyEmitSound( "Psap.DeployAgain" );
		else
			flDuration = WheatleyEmitSound( m_bWheatleyIntroPlayed ? "Psap.Deploy" : "Psap.DeployIntro" );

		m_flWheatleyLastDeploy = gpGlobals->curtime + flDuration;

		if ( !m_bWheatleyIntroPlayed && !RandomInt( 0, 2 ) )
		{
			SetWheatleyState( 8 );
			m_bWheatleyIntroPlayed = true;

			m_iNextWheatleyVoiceLine = 0;
			m_flWheatleyIdleTime = gpGlobals->curtime + flDuration + 3.0f;
		}
		else
		{
			SetWheatleyState( 0 );
			m_bWheatleyIntroPlayed = true;

			m_flWheatleyIdleTime = gpGlobals->curtime + flDuration + GetWheatleyIdleWait();
		}

		return;
	}
	
	if ( pOwner->GetSappingState() == SAPPING_NONE )
	{
		if ( m_iWheatleyState == 8 && m_flWheatleyIdleTime < gpGlobals->curtime )
		{
			if ( !IsWheatleyTalking() )
			{
				char szVoiceLine[128];
				szVoiceLine[0] = 0;

				if ( m_iNextWheatleyVoiceLine <= 3 )
				{
					V_sprintf_safe( szVoiceLine, "PSap.IdleIntro0%i", ++m_iNextWheatleyVoiceLine );

					float flSoundDuration = WheatleyEmitSound( szVoiceLine );
					if ( m_iNextWheatleyVoiceLine == 4 )
						m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait() + flSoundDuration;
					else
						m_flWheatleyIdleTime = gpGlobals->curtime + 1.0f + flSoundDuration;
				}
				else
				{
					SetWheatleyState( 0 );
					m_iNextWheatleyVoiceLine = 0;
				}
			}
		}
		else if ( m_flWheatleyIdleTime < gpGlobals->curtime )
		{
			bool bEmitToAll = false;
			bool bNoRepeats = false;
			CUtlString szVoiceLine;

			if ( m_iWheatleyState == 3 )
			{
				bEmitToAll = true;

				if ( IsWheatleyTalking() )
					szVoiceLine = "PSap.HackedLoud";
				else
					szVoiceLine = "PSap.Hacked";

				if ( !RandomInt( 0, 3 ) )
				{
					SetWheatleyState( 4 );
					m_flWheatleyIdleTime = gpGlobals->curtime + 1.3f;
				}
				else
				{
					SetWheatleyState( 0 );
					m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
				}
			}
			else if ( m_iWheatleyState == 2 )
			{
				bEmitToAll = true;
				SetWheatleyState( 0 );
				szVoiceLine = "PSap.HackingPW";
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( m_iWheatleyState == 1 )
			{
				bEmitToAll = true;
				SetWheatleyState( 0 );
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();

				if ( !RandomInt( 0, 2 ) )
					szVoiceLine = "PSap.HackingShort";
				else
					szVoiceLine = "PSap.Hacking";
			}
			else if ( m_iWheatleyState == 4 )
			{
				bEmitToAll = true;
				SetWheatleyState( 0 );
				szVoiceLine = "PSap.HackedFollowup";
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( IsWheatleyTalking() )
			{
				m_flWheatleyIdleTime = gpGlobals->curtime + 5.0f;
			}
			else if ( m_iWheatleyState == 7 )
			{
				if ( m_iNextWheatleyVoiceLine == 0 )
				{
					szVoiceLine = "PSap.IdleHack02";
					m_iNextWheatleyVoiceLine++;
				}
				else
				{
					SetWheatleyState( 0 );
					m_iNextWheatleyVoiceLine = 0;
				}

				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( m_iWheatleyState == 5 )
			{
				switch ( m_iNextWheatleyVoiceLine )
				{
					case 0:
						szVoiceLine = "PSap.IdleKnife02";
						m_iNextWheatleyVoiceLine++;
						m_flWheatleyIdleTime = gpGlobals->curtime + 0.3f;
						break;
					case 1:
						szVoiceLine = "PSap.IdleKnife03";
						m_iNextWheatleyVoiceLine++;
						m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
						break;
					default:
						SetWheatleyState( 0 );
						m_iNextWheatleyVoiceLine = 0;
						m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
						break;
				}
			}
			else if ( m_iWheatleyState == 6 )
			{
				if ( m_iNextWheatleyVoiceLine == 0 )
				{
					szVoiceLine = "PSap.IdleHarmless02";
					m_iNextWheatleyVoiceLine++;
				}
				else
				{
					SetWheatleyState( 0 );
					m_iNextWheatleyVoiceLine = 0;
				}

				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( pOwner->m_Shared.IsStealthed() )
			{
				if ( !RandomInt( 0, 1 ) )
					szVoiceLine = "PSap.Sneak";

				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( m_iWheatleyState == 0 )
			{
				bNoRepeats = true;
				szVoiceLine = "PSap.Idle";
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}

			if ( szVoiceLine == NULL )
			{
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
				return;
			}

			float flDuration = WheatleyEmitSound( szVoiceLine, bEmitToAll, bNoRepeats );
			m_flWheatleyIdleTime += flDuration;
		}

		return;
	}

	CUtlString szVoiceLine;
	if ( pOwner->GetSappingState() == SAPPING_PLACED )
	{
		if ( !RandomInt( 0, 1 ) )
		{
			if ( !RandomInt( 0, 3 ) )
			{
				szVoiceLine = "PSap.AttachedPW";
				SetWheatleyState( 1 );
			}
			else
			{
				szVoiceLine = "PSap.Attached";
				SetWheatleyState( 2 );
			}

			m_flWheatleyIdleTime = gpGlobals->curtime + 0.2f;
		}
		else
		{
			szVoiceLine = "PSap.Hacking";
			SetWheatleyState( 0 );
			m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
		}
	}
	else if ( pOwner->GetSappingState() == SAPPING_DONE )
	{
		if ( IsWheatleyTalking() )
		{
			if ( m_hLastSappedBuilding && m_hLastSappedBuilding->IsAlive() )
				szVoiceLine = "PSap.Death";
			else
				szVoiceLine = "PSap.HackedLoud";

			if ( !RandomInt( 0, 3 ) )
			{
				SetWheatleyState( 4 );
				m_flWheatleyIdleTime = gpGlobals->curtime + 1.3f;
			}
			else
			{
				m_flWheatleyIdleTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
		}
		else
		{
			SetWheatleyState( 3 );
			m_flWheatleyIdleTime = gpGlobals->curtime + 0.5f;
		}
	}

	pOwner->ClearSappingState();

	if ( szVoiceLine == NULL )
		return;

	float flSoundDuration = WheatleyEmitSound( szVoiceLine, true );
	m_flWheatleyIdleTime += flSoundDuration;
}

#ifdef _DEBUG

CON_COMMAND( tf_wheatley_speak, "For testing; Force a line out of the Ap-Sap" )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_GetLocalPlayer() );
	if ( !pPlayer )
		return;

	CTFWeaponSapper *pSapper = dynamic_cast<CTFWeaponSapper *>( pPlayer->Weapon_GetWeaponByType( TF_WPN_TYPE_BUILDING ) );
	if ( pSapper && pSapper->IsWheatleySapper() )
		pSapper->WheatleyEmitSound( "PSap.Hacked" );
}

#endif
