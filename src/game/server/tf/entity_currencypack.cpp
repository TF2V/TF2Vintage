//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "triggers.h"
#include "entity_currencypack.h"
#include "particle_parse.h"
#include "tf_gamestats.h"
#include "tf_objective_resource.h"
#include "tf_gamerules.h"
#include "collisionutils.h"
#include "tf_player_shared.h"

IMPLEMENT_SERVERCLASS_ST( CCurrencyPack, DT_CurrencyPack )
	SendPropBool( SENDINFO( m_bDistributed ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( item_currencypack_large, CCurrencyPack );
LINK_ENTITY_TO_CLASS( item_currencypack_medium, CCurrencyPackMedium );
LINK_ENTITY_TO_CLASS( item_currencypack_small, CCurrencyPackSmall );
LINK_ENTITY_TO_CLASS( item_currencypack_custom, CCurrencyPackCustom );

IMPLEMENT_AUTO_LIST( ICurrencyPackAutoList );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCurrencyPack::CCurrencyPack()
{
	m_nAmount = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCurrencyPack::~CCurrencyPack()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCurrencyPack::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "MVM.MoneyPickup" );
	PrecacheScriptSound( "MVM.MoneyVanish" );

	PrecacheParticleSystem( "mvm_cash_embers" );
	PrecacheParticleSystem( "mvm_cash_explosion" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCurrencyPack::Spawn( void )
{
	BaseClass::Spawn();
	
	float flTimeToBlink = 5 - RandomFloat( 0.0, 0.25 );
	SetContextThink( &CCurrencyPack::BlinkThink, gpGlobals->curtime + GetLifeTime() - flTimeToBlink, "CurrencyPackWaitingToBlinkThink" );

	SetCollisionBounds( Vector( -10, -10, -10 ), Vector( 10, 10, 10 ) );
	SetRenderMode( kRenderTransAlpha );

	if ( m_bDistributed )
	{
		DispatchParticleEffect( "mvm_cash_embers_red", PATTACH_ABSORIGIN_FOLLOW, this );
	}
	else
	{
		DispatchParticleEffect( "mvm_cash_embers", PATTACH_ABSORIGIN_FOLLOW, this );
	}

	//m_nWaveNumber = MannVsMachineStats_GetCurrentWave();

	if ( m_nAmount == 0 )
		m_nAmount = TFGameRules()->CalculateCurrencyAmount_ByType( GetPackSize() );

	//MannVsMachineStats_RoundEvent_CreditsDropped( m_nWaveNumber, m_nAmount );

	if ( !m_bDistributed && TFObjectiveResource() )
	{
		TFObjectiveResource()->AddMvMWorldMoney( m_nAmount );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCurrencyPack::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	/*if ( g_pPopulationManager && !m_bTouched )
	{
		if ( !m_bDistributed )
		{
			g_pPopulationManager->OnCurrencyPackFade();
		}

		DispatchParticleEffect( "mvm_cash_explosion", GetAbsOrigin(), GetAbsAngles() );
	}*/

	if ( !m_bDistributed && TFObjectiveResource() )
	{
		TFObjectiveResource()->AddMvMWorldMoney( -m_nAmount );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCurrencyPack::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CCurrencyPack::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCurrencyPack::MyTouch( CBasePlayer *pPlayer )
{
	if ( ValidTouch( pPlayer ) && !m_bTouched )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		if ( pTFPlayer->IsBot() )
			return false;

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
			{
				if ( TFGameRules()->GetWinningTeam() != pTFPlayer->GetTeamNumber() )
					return false;
			}

			// Scouts gain health when grabbing currency packs
			if ( pTFPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
			{
				const int nCurHealth = pTFPlayer->GetHealth();
				const int nMaxHealth = pTFPlayer->GetMaxHealth();
				int nHealth = nCurHealth < nMaxHealth ? 50 : 25;

				const int nHealthCap = nMaxHealth * 4;
				if ( nCurHealth > nHealthCap )
				{
					nHealth = RemapValClamped( nCurHealth, nHealthCap, ( nHealthCap * 1.5f ), 20, 5 );
				}

				pTFPlayer->TakeHealth( nHealth, DMG_IGNORE_MAXHEALTH );
			}

			//MannVsMachineStats_PlayerEvent_PickedUpCredits( pTFTouchPlayer, m_nWaveNumber, m_nAmount );

			IGameEvent *event = gameeventmanager->CreateEvent( "mvm_pickup_currency" );
			if ( event )
			{
				event->SetInt( "player", pTFPlayer->entindex() );
				event->SetInt( "currency", m_nAmount );
				gameeventmanager->FireEvent( event );
			}

			// some achievement here
		}

		CReliableBroadcastRecipientFilter filter;
		EmitSound( filter, entindex(), "MVM.MoneyPickup" );

		if ( !m_bDistributed )
		{
			TFGameRules()->DistributeCurrencyAmount( m_nAmount, pTFPlayer );
			CTF_GameStats.Event_PlayerCollectedCurrency( pTFPlayer, m_nAmount );
		}

		bool bIsHiddenSpy = pTFPlayer->IsPlayerClass( TF_CLASS_SPY ) && pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED );
		bIsHiddenSpy |= pTFPlayer->m_Shared.IsStealthed() && !pTFPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK );

		if ( !bIsHiddenSpy )
		{
			pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_MONEY_PICKUP );
		}

		pTFPlayer->SetLastObjectiveTime( gpGlobals->curtime );

		m_bTouched = true;
	}

	return m_bTouched;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCurrencyPackCustom::GetDefaultPowerupModel( void )
{
	if ( m_nAmount >= 25 )
		return "models/items/currencypack_large.mdl";
	else if ( m_nAmount >= 10 )
		return "models/items/currencypack_medium.mdl";
	else
		return "models/items/currencypack_small.mdl";
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value of a custom pack
//-----------------------------------------------------------------------------
void CCurrencyPack::SetAmount( float nAmount )
{
	m_nAmount = nAmount;
}

//-----------------------------------------------------------------------------
// Purpose: Distribute
//-----------------------------------------------------------------------------
void CCurrencyPack::DistributedBy( CBasePlayer *pMoneyMaker )
{
	TFGameRules()->DistributeCurrencyAmount( m_nAmount );

	if ( pMoneyMaker )
	{
		CTF_GameStats.Event_PlayerCollectedCurrency( pMoneyMaker, m_nAmount );
	}

	m_bDistributed = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCurrencyPack::ComeToRest( void )
{
	BaseClass::ComeToRest();

	if ( /* bool at 0x515 ||*/ m_bDistributed )
		return;

	// No area means we're in an invalid location, dispurse
	if ( TheNavMesh->GetNavArea( GetAbsOrigin() ) == NULL )
	{
		TFGameRules()->DistributeCurrencyAmount( m_nAmount );
		m_bTouched = true;
		UTIL_Remove( this );

		return;
	}

	// See if we've come to rest in a trigger_hurt
	for ( int i = 0; i < ITriggerHurtAutoList::AutoList().Count(); i++ )
	{
		CTriggerHurt *pTrigger = static_cast<CTriggerHurt *>( ITriggerHurtAutoList::AutoList()[i] );
		if ( !pTrigger->m_bDisabled )
		{
			Vector vecMins, vecMaxs;
			pTrigger->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
			if ( IsPointInBox( GetCollideable()->GetCollisionOrigin(), vecMins, vecMaxs ) )
			{
				TFGameRules()->DistributeCurrencyAmount( m_nAmount );

				m_bTouched = true;
				UTIL_Remove( this );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCurrencyPack::BlinkThink( void )
{
	/*if ( bool at 0x515 )
		return;*/

	++m_nBlinkState;
	if ( m_nBlinkState % 2 == 0 )
	{
		SetRenderColorA( 25 );
	}
	else
	{
		SetRenderColorA( 255 );
	}

	SetContextThink( &CCurrencyPack::BlinkThink, gpGlobals->curtime + 0.25f, "CurrencyPackBlinkThink" );
}