#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#else
#include "tf_player.h"
#endif

#include "tf_wearable_demoshield.h"
#include "tf_weaponbase_decapitation.h"


CTFDecapitationMeleeWeaponBase::CTFDecapitationMeleeWeaponBase()
{
	m_hEquippedShield = nullptr;
}

CTFDecapitationMeleeWeaponBase::~CTFDecapitationMeleeWeaponBase()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFDecapitationMeleeWeaponBase::TranslateViewmodelHandActivity( int iActivity )
{
	int iTranslation = iActivity;
	if( !GetTFPlayerOwner() )
		return BaseClass::TranslateViewmodelHandActivity( iActivity );

	if( GetItem()->GetAnimationSlot() == TF_WPN_TYPE_MELEE_ALLCLASS )
		return BaseClass::TranslateViewmodelHandActivity( iActivity );

	if ( GetTFPlayerOwner()->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		switch (iActivity)
		{
			case ACT_VM_DRAW:
				iTranslation = ACT_VM_DRAW_SPECIAL;
				break;
			case ACT_VM_HOLSTER:
				iTranslation = ACT_VM_HOLSTER_SPECIAL;
				break;
			case ACT_VM_IDLE:
				iTranslation = ACT_VM_IDLE_SPECIAL;
				break;
			case ACT_VM_PULLBACK:
				iTranslation = ACT_VM_PULLBACK_SPECIAL;
				break;
			case ACT_VM_PRIMARYATTACK:
			case ACT_VM_SECONDARYATTACK:
				iTranslation = ACT_VM_PRIMARYATTACK_SPECIAL;
				break;
			case ACT_VM_HITCENTER:
				iTranslation = ACT_VM_HITCENTER_SPECIAL;
				break;
			case ACT_VM_SWINGHARD:
				iTranslation = ACT_VM_SWINGHARD_SPECIAL;
				break;
			case ACT_VM_IDLE_TO_LOWERED:
				iTranslation = ACT_VM_IDLE_TO_LOWERED_SPECIAL;
				break;
			case ACT_VM_IDLE_LOWERED:
				iTranslation = ACT_VM_IDLE_LOWERED_SPECIAL;
				break;
			case ACT_VM_LOWERED_TO_IDLE:
				iTranslation = ACT_VM_LOWERED_TO_IDLE_SPECIAL;
				break;
			default:
				return BaseClass::TranslateViewmodelHandActivity( iActivity );
		}
	}

	return BaseClass::TranslateViewmodelHandActivity( iTranslation );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFDecapitationMeleeWeaponBase::GetMeleeDamage( CBaseEntity *pTarget, int &iDamageType, int &iCustomDamage )
{
	float res = BaseClass::GetMeleeDamage( pTarget, iDamageType, iCustomDamage );
	iCustomDamage = TF_DMG_CUSTOM_DECAPITATION;
	return res;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDecapitationMeleeWeaponBase::Holster( CBaseCombatWeapon *pSwitchTo )
{
	if (BaseClass::Holster( pSwitchTo ))
	{
		StopListeningForAllEvents();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::SetupGameEventListeners( void )
{
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDecapitationMeleeWeaponBase::FireGameEvent( IGameEvent *event )
{
	if ( !FStrEq( event->GetName(), "player_death" ) )
		return;

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( event->GetInt( "attacker" ) != pOwner->GetUserID() )
		return;

	/*if ( event->GetInt( "weaponid" ) != TF_WEAPON_SWORD && event->GetInt( "customkill" ) != TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING )
		return;*/

	if ( !CanDecapitate() )
		return;

	CTFPlayer *pVictim = ToTFPlayer( UTIL_PlayerByUserId( event->GetInt( "userid" ) ) );
	Assert( pVictim );

	OnDecapitation( pVictim );
}

#endif