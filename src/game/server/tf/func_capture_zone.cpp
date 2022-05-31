//======= Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: CTF Flag Capture Zone.
//
//=============================================================================//
#include "cbase.h"
#include "func_capture_zone.h"
#include "tf_player.h"
#include "tf_item.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"

//=============================================================================
//
// CTF Flag Capture Zone tables.
//

BEGIN_DATADESC( CCaptureZone )

	// Keyfields.
	DEFINE_KEYFIELD( m_nCapturePoint, FIELD_INTEGER, "CapturePoint" ),

	// Functions.
	DEFINE_FUNCTION( CCaptureZoneShim::Touch ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_OnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_OnCapTeam2, "OnCapTeam2" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_capturezone, CCaptureZone );


IMPLEMENT_SERVERCLASS_ST( CCaptureZone, DT_CaptureZone )
	SendPropInt( SENDINFO( m_bDisabled ) ),
END_SEND_TABLE()


IMPLEMENT_AUTO_LIST( ICaptureZoneAutoList );


extern ConVar tf2v_assault_ctf_rules;

//=============================================================================
//
// CTF Flag Capture Zone functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::Spawn()
{
	InitTrigger();
	SetTouch( &CCaptureZoneShim::Touch );

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}

	m_flNextTouchingEnemyZoneWarning = -1;
	AddSpawnFlags( SF_TRIGGER_ALLOW_ALL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::CaptureTouch( CBaseEntity *pOther )
{
	// Is the zone enabled?
	if ( IsDisabled() )
		return;

	// Get the TF player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( pPlayer )
	{
		// Check to see if the player has the capture flag.
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( pPlayer->GetItem() );
			if ( pFlag && !pFlag->IsCaptured() )
			{
				// does this capture point have a team number asssigned?
				if ( GetTeamNumber() != TEAM_UNASSIGNED )
				{
					// TF2V exclusive: We use the enemy's capture zone for Assault CTF.
					bool bIsTeamCapturePoint;
					if ( pFlag->GetGameType() == TF_FLAGTYPE_CTF && tf2v_assault_ctf_rules.GetBool() )
						bIsTeamCapturePoint = pPlayer->GetTeamNumber() == GetTeamNumber();
					else
						bIsTeamCapturePoint = pPlayer->GetTeamNumber() != GetTeamNumber();
					
					// Check to see if the capture zone team matches the player's team.
					if ( pPlayer->GetTeamNumber() != TEAM_UNASSIGNED && bIsTeamCapturePoint )
					{
						if ( pFlag->GetGameType() == TF_FLAGTYPE_CTF )
						{
							// Do this at most once every 5 seconds
							if ( m_flNextTouchingEnemyZoneWarning < gpGlobals->curtime )
							{
								CSingleUserRecipientFilter filter( pPlayer );
								TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP );
								m_flNextTouchingEnemyZoneWarning = gpGlobals->curtime + 5;
							}
						}
						else if ( pFlag->GetGameType() == TF_FLAGTYPE_INVADE )
						{
							TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_Wrong_Goal" );
						}

						return;
					}
				}

				if ( TFGameRules()->FlagsMayBeCapped() )
				{
					pFlag->Capture( pPlayer, m_nCapturePoint );

					// Output.
					m_outputOnCapture.FireOutput( this, this );
					switch ( pPlayer->GetTeamNumber() )
					{
						case TF_TEAM_RED:
							m_OnCapTeam1.FireOutput( this, this );
							break;
						case TF_TEAM_BLUE:
							m_OnCapTeam2.FireOutput( this, this );
							break;
					}

					IGameEvent *event = gameeventmanager->CreateEvent( "ctf_flag_captured" );
					if ( event )
					{
						int iCappingTeam = pPlayer->GetTeamNumber();
						int	iCappingTeamScore = 0;
						CTFTeam* pCappingTeam = pPlayer->GetTFTeam();
						if ( pCappingTeam )
						{
							iCappingTeamScore = pCappingTeam->GetFlagCaptures();
						}

						event->SetInt( "capping_team", iCappingTeam );
						event->SetInt( "capping_team_score", iCappingTeamScore );
						event->SetInt( "capper", pPlayer->GetUserID() );
						event->SetInt( "priority", 9 ); // HLTV priority

						gameeventmanager->FireEvent( event );
					}

					// TODO:
					if ( TFGameRules() )
					{
						if ( TFGameRules()->IsHolidayActive( kHoliday_EOTL ) )
						{
							//TFGameRules()->DropBonusDuck( pPlayer->GetAbsOrigin(), pPlayer, NULL, NULL, false, true );
						}
						else if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
						{
							//TFGameRules()->DropHalloweenSoulPackToTeam( 5, GetAbsOrigin(), pPlayer->GetTeamNumber(), TEAM_SPECTATOR );
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureZone::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureZone::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureZone::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		BaseClass::Disable();
		SetTouch( NULL );
	}
	else
	{
		BaseClass::Enable();
		SetTouch( &CCaptureZone::Touch );
	}
}