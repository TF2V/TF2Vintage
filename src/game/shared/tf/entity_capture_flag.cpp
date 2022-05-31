//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#include "cbase.h"
#include "entity_capture_flag.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "filesystem.h"
#include "tf_logic_robot_destruction.h"

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "hudelement.h"
#include "iclientmode.h"
#include "hud_numericdisplay.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "tf_hud_objectivestatus.h"
#include "view.h"
#include "glow_outline_effect.h"

ConVar cl_flag_return_size( "cl_flag_return_size", "20", FCVAR_CHEAT );

#else
#include "tf_player.h"
#include "tf_team.h"
#include "tf_bot.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "func_respawnroom.h"
#include "datacache/imdlcache.h"
#include "func_respawnflag.h"
#include "func_flagdetectionzone.h"

extern ConVar tf_flag_caps_per_round;
extern ConVar tf_rd_min_points_to_steal;

ConVar cl_flag_return_height( "cl_flag_return_height", "82", FCVAR_CHEAT );
ConVar tf_rd_return_min_time( "tf_rd_return_min_time", "30" );
ConVar tf_rd_return_max_time( "tf_rd_return_max_time", "90" );

#endif

ConVar tf_flag_return_on_touch( "tf_flag_return_on_touch", "0", FCVAR_REPLICATED, "If this is set, your flag must be at base in order to capture the enemy flag. Remote friendly flags return to your base instantly when you touch them" );
#ifdef GAME_DLL
ConVar tf2v_assault_ctf_rules( "tf2v_assault_ctf_rules", "0", FCVAR_NOTIFY, "Uses reversed CTF locations and instant respawning flags. Applied on map switch." );
#endif

#define FLAG_EFFECTS_NONE		0
#define FLAG_EFFECTS_ALL		1
#define FLAG_EFFECTS_PAPERONLY	2
#define FLAG_EFFECTS_COLORONLY	3

#ifdef CLIENT_DLL

static void RecvProxy_IsDisabled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCaptureFlag *pFlag = (CCaptureFlag *) pStruct;
	bool bIsDisabled = ( pData->m_Value.m_Int > 0 );

	if ( pFlag )
	{
		pFlag->SetDisabled( bIsDisabled ); 
	}
}

static void RecvProxy_IsVisibleWhenDisabled( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCaptureFlag *pFlag = (CCaptureFlag *) pStruct;
	bool bVisible = ( pData->m_Value.m_Int > 0 );

	if ( pFlag )
	{
		pFlag->SetVisibleWhenDisabled( bVisible ); 
	}
}

static void RecvProxy_FlagStatus( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CCaptureFlag *pFlag = (CCaptureFlag *) pStruct;

	if ( pFlag )
	{
		pFlag->SetFlagStatus( pData->m_Value.m_Int );

		IGameEvent *pEvent = gameeventmanager->CreateEvent( "flagstatus_update" );
		if ( pEvent )
		{
			pEvent->SetInt( "entindex", pFlag->entindex() );
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
}


#endif

//=============================================================================
//
// CTF Flag tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlag, DT_CaptureFlag )

BEGIN_NETWORK_TABLE( CCaptureFlag, DT_CaptureFlag )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropBool( SENDINFO( m_bVisibleWhenDisabled ) ),
	SendPropInt( SENDINFO( m_nType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nFlagStatus ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flResetTime ) ),
	SendPropTime( SENDINFO( m_flNeutralTime ) ),
	SendPropTime( SENDINFO( m_flMaxResetTime ) ),
	SendPropEHandle( SENDINFO( m_hPrevOwner ) ),
	SendPropString( SENDINFO( m_szModel ) ),
	SendPropString( SENDINFO( m_szHudIcon ) ),
	SendPropString( SENDINFO( m_szPaperEffect ) ),
	SendPropString( SENDINFO( m_szTrailEffect ) ),
	SendPropInt( SENDINFO( m_nUseTrailEffect ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nPointValue ) ),
	SendPropFloat( SENDINFO( m_flAutoCapTime ) ),
	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropFloat( SENDINFO( m_flTimeToSetPoisonous ) ),

#else
	RecvPropInt( RECVINFO( m_bDisabled ), 0, RecvProxy_IsDisabled ),
	RecvPropInt( RECVINFO( m_bVisibleWhenDisabled ), 0, RecvProxy_IsVisibleWhenDisabled ),
	RecvPropInt( RECVINFO( m_nType ) ),
	RecvPropInt( RECVINFO( m_nFlagStatus ), 0, RecvProxy_FlagStatus ),
	RecvPropTime( RECVINFO( m_flResetTime ) ),
	RecvPropTime( RECVINFO( m_flNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flMaxResetTime ) ),
	RecvPropEHandle( RECVINFO( m_hPrevOwner ) ),
	RecvPropString( RECVINFO( m_szModel ) ),
	RecvPropString( RECVINFO( m_szHudIcon ) ),
	RecvPropString( RECVINFO( m_szPaperEffect ) ),
	RecvPropString( RECVINFO( m_szTrailEffect ) ),
	RecvPropInt( RECVINFO( m_nUseTrailEffect ) ),
	RecvPropInt( RECVINFO( m_nPointValue ) ),
	RecvPropFloat( RECVINFO( m_flAutoCapTime ) ),
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
	RecvPropFloat( RECVINFO( m_flTimeToSetPoisonous ) ),

#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CCaptureFlag )

	// Keyfields.
	DEFINE_KEYFIELD( m_flResetTime, FIELD_INTEGER, "ReturnTime"),
	DEFINE_KEYFIELD( m_nType, FIELD_INTEGER, "GameType" ),
	DEFINE_KEYFIELD( m_nUseTrailEffect, FIELD_INTEGER, "trail_effect" ),
	DEFINE_KEYFIELD( m_bReturnBetweenWaves, FIELD_BOOLEAN, "ReturnBetweenWaves" ),
	DEFINE_KEYFIELD( m_bVisibleWhenDisabled, FIELD_BOOLEAN, "VisibleWhenDisabled" ),
	DEFINE_KEYFIELD( m_nPointValue, FIELD_INTEGER, "PointValue" ),
	
#ifdef GAME_DLL
	DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "flag_model" ),
	DEFINE_KEYFIELD( m_iszHudIcon, FIELD_STRING, "flag_icon" ),
	DEFINE_KEYFIELD( m_iszPaperEffect, FIELD_STRING, "flag_paper" ),
	DEFINE_KEYFIELD( m_iszTrailEffect, FIELD_STRING, "flag_trail" ),
	DEFINE_KEYFIELD( m_iszTags, FIELD_STRING, "tags" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceReset", InputForceReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetSilent", InputForceResetSilent ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetAndDisableSilent", InputForceResetAndDisableSilent ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetReturnTime", InputSetReturnTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ShowTimer", InputShowTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ForceGlowDisabled", InputForceGlowDisabled ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_outputOnPickUp, "OnPickUp" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam1, "OnPickupTeam1" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam2, "OnPickupTeam2" ),
	DEFINE_OUTPUT( m_outputOnPickUpByTeam, "OnPickupByTeam" ),
	DEFINE_OUTPUT( m_outputOnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_outputOnCapTeam1, "OnCapTeam1"),
	DEFINE_OUTPUT( m_outputOnCapTeam2, "OnCapTeam2" ),
	DEFINE_OUTPUT( m_outputOnCapByTeam, "OnCapByTeam" ),
	DEFINE_OUTPUT( m_outputOnTouchSameTeam, "OnTouchSameTeam"),

#endif

END_DATADESC()

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

IMPLEMENT_AUTO_LIST( ICaptureFlagAutoList );

//=============================================================================
//
// CTF Flag functions.
//

CCaptureFlag::CCaptureFlag()
{
#ifdef CLIENT_DLL
	m_pPaperTrailEffect = NULL;
	m_pGlowEffect = NULL;
	m_pCarrierGlowEffect = NULL;
#else
	m_hReturnIcon = NULL;
	m_pGlowTrail = NULL;

	m_iszModel = NULL_STRING;
	m_iszHudIcon = NULL_STRING;
	m_iszPaperEffect = NULL_STRING;
	m_iszTrailEffect = NULL_STRING;

	m_nPointValue = 0;
#endif

	UseClientSideAnimation();

	m_nUseTrailEffect = FLAG_EFFECTS_ALL;
	m_bGlowEnabled = true;
	m_bReturnBetweenWaves = true;
	m_bVisibleWhenDisabled = false;

	m_szModel.GetForModify()[0] = '\0';
	m_szHudIcon.GetForModify()[0] = '\0';
	m_szPaperEffect.GetForModify()[0] = '\0';
	m_szTrailEffect.GetForModify()[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CCaptureFlag::GetItemID( void )
{
	return TF_ITEM_CAPTURE_FLAG;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCaptureFlag::GetFlagModel( void )
{
	if ( m_szModel[0] != '\0' )
	{
		if ( g_pFullFileSystem->FileExists( m_szModel.Get(), "GAME" ) )
		{
			return ( m_szModel.Get() );
		}
	}

	return TF_FLAG_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::GetHudIcon( int nTeam, char *pchName, int nBuffSize )
{
	Assert( nTeam >= 0 && nTeam < TF_TEAM_COUNT );
	V_snprintf( pchName, nBuffSize, "%s_%s", 
				( m_szHudIcon[0] != '\0' ) ? m_szHudIcon.Get() : TF_FLAG_ICON,
				g_aTeamParticleNames[ nTeam ] );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CCaptureFlag::GetPaperEffect( void )
{
	if ( m_szPaperEffect[0] != '\0' )
		return m_szPaperEffect.Get();

	return TF_FLAG_EFFECT;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::GetTrailEffect( int nTeam, char *pchName, int nBuffSize )
{
	Assert( nTeam >= 0 && nTeam < TF_TEAM_COUNT );
	V_snprintf( pchName, nBuffSize, "effects/%s_%s.vmt", 
				( m_szTrailEffect[0] != '\0' ) ? m_szTrailEffect.Get() : TF_FLAG_TRAIL,
				g_aTeamParticleNames[ nTeam ] );
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::Precache( void )
{
	PrecacheModel( GetFlagModel() );

	PrecacheScriptSound( TF_CTF_FLAGSPAWN ); // Should be Resource.Flagspawn
	PrecacheScriptSound( TF_CTF_ENEMY_STOLEN );
	PrecacheScriptSound( TF_CTF_ENEMY_DROPPED );
	PrecacheScriptSound( TF_CTF_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_CTF_ENEMY_RETURNED );
	PrecacheScriptSound( TF_CTF_TEAM_STOLEN );
	PrecacheScriptSound( TF_CTF_TEAM_DROPPED );
	PrecacheScriptSound( TF_CTF_TEAM_CAPTURED );
	PrecacheScriptSound( TF_CTF_TEAM_RETURNED );

	PrecacheScriptSound( TF_AD_CAPTURED_SOUND );
	PrecacheScriptSound( TF_AD_ENEMY_STOLEN );
	PrecacheScriptSound( TF_AD_ENEMY_DROPPED );
	PrecacheScriptSound( TF_AD_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_AD_ENEMY_RETURNED );
	PrecacheScriptSound( TF_AD_TEAM_STOLEN );
	PrecacheScriptSound( TF_AD_TEAM_DROPPED );
	PrecacheScriptSound( TF_AD_TEAM_CAPTURED );
	PrecacheScriptSound( TF_AD_TEAM_RETURNED );

	PrecacheScriptSound( TF_INVADE_ENEMY_STOLEN );
	PrecacheScriptSound( TF_INVADE_ENEMY_DROPPED );
	PrecacheScriptSound( TF_INVADE_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_INVADE_TEAM_STOLEN );
	PrecacheScriptSound( TF_INVADE_TEAM_DROPPED );
	PrecacheScriptSound( TF_INVADE_TEAM_CAPTURED );
	PrecacheScriptSound( TF_INVADE_FLAG_RETURNED );
	
	PrecacheScriptSound( TF_RESOURCE_FLAGSPAWN );
	PrecacheScriptSound( TF_RESOURCE_ENEMY_STOLEN	);
	PrecacheScriptSound( TF_RESOURCE_ENEMY_DROPPED );
	PrecacheScriptSound( TF_RESOURCE_ENEMY_CAPTURED );
	PrecacheScriptSound( TF_RESOURCE_TEAM_STOLEN );
	PrecacheScriptSound( TF_RESOURCE_TEAM_DROPPED );
	PrecacheScriptSound( TF_RESOURCE_TEAM_CAPTURED );
	PrecacheScriptSound( TF_RESOURCE_RETURNED );

	PrecacheParticleSystem( GetPaperEffect() );


	// Team colored trail
	char tempChar[ 512 ];
	for( int i = TF_TEAM_RED; i < TF_TEAM_COUNT; ++i )
	{
		GetTrailEffect( i, tempChar, sizeof( tempChar ) );
		PrecacheModel( tempChar );
	}

}

#ifndef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_nOldFlagStatus = m_nFlagStatus;
	m_bOldGlowEnabled = m_bGlowEnabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::OnDataChanged( DataUpdateType_t updateType )
{
	bool bUpdateGlow = false;

	if ( m_nOldFlagStatus != m_nFlagStatus )
	{
		bUpdateGlow = true;
	}
	else if ( m_hOldOwner.Get() != GetOwnerEntity() )
	{
		bUpdateGlow = true;
		m_hOldOwner = GetOwnerEntity();
	}
	else if ( m_bOldGlowEnabled != m_bGlowEnabled )
	{
		bUpdateGlow = true;
	}

	if ( bUpdateGlow )
	{
		UpdateGlowEffect();
	}

	if ( updateType == DATA_UPDATE_CREATED )
		CreateSiren();
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Spawn( void )
{
#ifdef GAME_DLL
	V_strncpy( m_szModel.GetForModify(), STRING( m_iszModel ), MAX_PATH );
	V_strncpy( m_szHudIcon.GetForModify(), STRING( m_iszHudIcon ), MAX_PATH );
	V_strncpy( m_szPaperEffect.GetForModify(), STRING( m_iszPaperEffect ), MAX_PATH );
	V_strncpy( m_szTrailEffect.GetForModify(), STRING( m_iszTrailEffect ), MAX_PATH );
#endif

	// Precache the model and sounds.  Set the flag model.
	Precache();

	SetModel( GetFlagModel() );
	
	// Set the flag solid and the size for touching.
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetSize( vec3_origin, vec3_origin );

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 24 );

	// use the initial dynamic prop "m_bStartDisabled" setting to set our own m_bDisabled flag
#ifdef GAME_DLL
	m_bDisabled = m_bStartDisabled;
	m_bStartDisabled = false;
	m_flTimeToSetPoisonous = 0.0f;

	// Don't allow the intelligence to fade.
	m_flFadeScale = 0.0f;
#else
	m_bDisabled = false;
#endif

	// Base class spawn.
	BaseClass::Spawn();

	SetCollisionBounds( Vector( -19.5f, -22.5f, -6.5f ), Vector( 19.5f, 22.5f, 6.5f ) );

#ifdef GAME_DLL
	// Save the starting position, so we can reset the flag later if need be.
	m_vecResetPos = GetAbsOrigin();
	m_vecResetAng = GetAbsAngles();

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	m_bCaptured = false;

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 0 );
		TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
		TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
	}

	CSplitString tags( STRING( m_iszTags ), " " );
	for ( int i=0; i < tags.Count(); ++i )
	{
		m_tags.CopyAndAddToTail( tags[i] );
	}
#endif

	SetDisabled( m_bDisabled );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::UpdateOnRemove( void )
{
#ifndef GAME_DLL
	DestroySiren();
#endif

	CTFPlayer *pOwnerPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pOwnerPlayer )
	{
		pOwnerPlayer->SetItem( NULL );
	}

	BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL

// Manage glow effect
void CCaptureFlag::UpdateGlowEffect( void )
{
	if ( !m_pGlowEffect )
	{
		m_pGlowEffect = new CGlowObject( this, Vector( 0.76f, 0.76f, 0.76f ), 1.0, true );
	}

	if ( m_pGlowEffect )
	{
		if ( ShouldHideGlowEffect() )
		{
			m_pGlowEffect->SetEntity( NULL );
		}
		else
		{
			m_pGlowEffect->SetEntity( this );

			Vector vecColor;
			TeamplayRoundBasedRules()->GetTeamGlowColor( GetTeamNumber(), vecColor.x, vecColor.y, vecColor.z );
			m_pGlowEffect->SetColor( vecColor );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCaptureFlag::ShouldHideGlowEffect( void )
{
	if ( !IsGlowEnabled() )
	{
		return true;
	}

	if ( TFGameRules() && !TFGameRules()->AllowGlowOutlinesFlags() )
	{
		return true;
	}

	// If the opposite team stole our intel we need to hide the glow
	bool bIsHiddenTeam = false;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		if ( m_nType == TF_FLAGTYPE_CTF )
		{
			// In CTF the flag is the team of where it was originally sitting
			bIsHiddenTeam = ( pLocalPlayer->GetTeamNumber() == GetTeamNumber() );
		}
		else
		{
			// In non-CTF control the flag changes to the team that's carrying it
			bIsHiddenTeam = ( pLocalPlayer->GetTeamNumber() != TEAM_SPECTATOR && pLocalPlayer->GetTeamNumber() != GetTeamNumber() );
		}
	}

	bool bHide = IsStolen() && bIsHiddenTeam;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		bHide = IsHome();
	}

	return ( IsDisabled() || bHide || IsEffectActive( EF_NODRAW ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::CreateSiren( void )
{
	if ( m_hSirenEffect )
		return;

	int iAttachment = LookupAttachment( "siren" );
	if ( iAttachment > -1 )
	{
		char const *pszEffectName = "cart_flashinglight";
		if ( GetTeamNumber() == TF_TEAM_RED )
		{
			pszEffectName = "cart_flashinglight_red";
		}
		m_hSirenEffect = ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, iAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::DestroySiren( void )
{
	if ( m_hSirenEffect )
	{
		ParticleProp()->StopEmission( m_hSirenEffect );
		m_hSirenEffect = NULL;
	}
}
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::Activate( void )
{
	BaseClass::Activate();

	if ( tf2v_assault_ctf_rules.GetBool() && m_nType == TF_FLAGTYPE_CTF )
	{
		// If we're playing Assault CTF, swap the flags.
		switch (GetTeamNumber())
		{
		case TF_TEAM_RED:
			m_iOriginalTeam = TF_TEAM_BLUE;
			break;
		case TF_TEAM_BLUE:
			m_iOriginalTeam = TF_TEAM_RED;
			break;
		}
	}
	else
		m_iOriginalTeam = GetTeamNumber();

	m_nSkin = GetIntelSkin( GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureFlag *CCaptureFlag::Create( const Vector& vecOrigin, const char *pszModelName, int nFlagType )
{
	CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( CBaseEntity::CreateNoSpawn( "item_teamflag", vecOrigin, vec3_angle, NULL ) );
	pFlag->m_iszModel = MAKE_STRING( pszModelName );
	pFlag->m_nType = nFlagType;

	if ( nFlagType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		pFlag->m_nUseTrailEffect = FLAG_EFFECTS_NONE;
	}

	DispatchSpawn( pFlag );

	return pFlag;
}
#endif

int CCaptureFlag::GetIntelSkin(int iTeamNum, bool bPickupSkin)
{
	switch ( iTeamNum )
	{
	case TF_TEAM_RED:
		return bPickupSkin ? 3 : 0;
	case TF_TEAM_BLUE:
		return bPickupSkin ? 4 : 1;
	case TF_TEAM_GREEN:
		return bPickupSkin ? 8 : 6;
	case TF_TEAM_YELLOW:
		return bPickupSkin ? 9 : 7;
	case TEAM_UNASSIGNED:
	default:
		return bPickupSkin ? 5 : 2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset the flag position state.
//-----------------------------------------------------------------------------
void CCaptureFlag::Reset( void )
{
#ifdef GAME_DLL
	if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION && !IsDisabled() )
	{
		if ( m_nPointValue > 0 )
		{
			// Score points!
			if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
			{
				CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( GetTeamNumber(), 
																				   m_nPointValue.Get(), 
																				   SCORE_REACTOR_RETURNED, 
																				   NULL );

				CTFRobotDestructionLogic::GetRobotDestructionLogic()->FlagDestroyed( GetTeamNumber() );
				m_nPointValue = 0;
			}
		}

		const int nTargetScore = CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetTargetScore( GetTeamNumber() );
		SetDisabled( nTargetScore < tf_rd_min_points_to_steal.GetInt() );
	}
	else if ( m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		UTIL_Remove( this );
		return;
	}

	// Set the flag position.
	SetAbsOrigin( m_vecResetPos );
	SetAbsAngles( m_vecResetAng );

	// No longer dropped, if it was.
	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	if ( m_nType == TF_FLAGTYPE_INVADE || m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		ChangeTeam( m_iOriginalTeam );
	}

	m_nSkin = GetIntelSkin(GetTeamNumber());
	SetMoveType( MOVETYPE_NONE );

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 0 );
		TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
		TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
	}
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::ResetMessage( void )
{
#ifdef GAME_DLL
	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_CTF_ENEMY_RETURNED );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_RETURNED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_CTF_TEAM_RETURNED );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_RETURNED );
			}
		}

		// Returned sound
		CPASAttenuationFilter filter( this, TF_CTF_FLAGSPAWN );
		PlaySound( filter, TF_CTF_FLAGSPAWN );
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_AD_TEAM_RETURNED );
			}
			else
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_FlagReturned" );

				CTeamRecipientFilter filter( iTeam, true );
				if ( TFGameRules()->IsMannVsMachineMode() )
				{
					PlaySound( filter, TF_MVM_AD_ENEMY_RETURNED );
				}
				else
				{
					PlaySound( filter, TF_AD_ENEMY_RETURNED );
				}
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagReturned" );

			CTeamRecipientFilter filter( iTeam, true );
			PlaySound( filter, TF_INVADE_FLAG_RETURNED );
		}
	}
	else if ( m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		const char *pszSound = TF_RESOURCE_RETURNED;
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			//TODO: TFGameRules()->StartSomeTimerForDoomsdayEvent();
			pszSound = TF_RESOURCE_EVENT_RETURNED;
		}
		// This is a neutral flag, so tell everyone.
		TFGameRules()->BroadcastSound( 255, pszSound );
		
		// Returned sound
		CPASAttenuationFilter filter( this, TF_RESOURCE_FLAGSPAWN );
		PlaySound( filter, TF_RESOURCE_FLAGSPAWN );
	}
	else if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam == GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_ENEMY_RETURNED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_TEAM_RETURNED );
			}
		}
	}

	// Output.
	m_outputOnReturn.FireOutput( this, this );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "eventtype", TF_FLAGEVENT_RETURNED );
		event->SetInt( "priority", 8 );
		event->SetInt( "team", GetTeamNumber() );
		gameeventmanager->FireEvent( event );
	}

	DestroyReturnIcon();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	// Is the flag enabled?
	if ( IsDisabled() )
	{
		return;
	}

	// The the touch from a live player.
	if ( !pOther->IsPlayer() || !pOther->IsAlive() )
	{
		return;
	}

#ifdef GAME_DLL
	// Don't let the person who threw this flag pick it up until it hits the ground.
	// This way we can throw the flag to people, but not touch it as soon as we throw it ourselves
	if(  m_hPrevOwner.Get() && m_hPrevOwner.Get() == pOther && m_bAllowOwnerPickup == false )
	{
		return;
	}
#endif

	if ( pOther->GetTeamNumber() == GetTeamNumber() )
	{
	#ifdef GAME_DLL
		m_outputOnTouchSameTeam.FireOutput( this, this );
	#endif

		// Does my team own this flag? If so, no touch.
		if ( m_nType == TF_FLAGTYPE_CTF || m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
		{
			if ( !tf_flag_return_on_touch.GetBool() )
				return;

			if ( IsHome() || IsStolen() )
				return;
		}
	}

	if ( ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND || m_nType == TF_FLAGTYPE_TERRITORY_CONTROL ) &&
		   pOther->GetTeamNumber() != GetTeamNumber() )
	{
		return;
	}

	// Don't allow us to touch the flag until it's neutral.
	if ( ( m_nType == TF_FLAGTYPE_INVADE || m_nType == TF_FLAGTYPE_RESOURCE_CONTROL ) && GetTeamNumber() != TEAM_UNASSIGNED )
	{
		if ( pOther->GetTeamNumber() != GetTeamNumber() )
		{
			return;
		}
	}

	// Can't pickup flags during WaitingForPlayers
	if ( TFGameRules()->IsInWaitingForPlayers() )
		return;

	// Get the touching player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
	{
		return;
	}

	if ( !pPlayer->IsAllowedToPickUpFlag() )
	{
		return;
	}

#ifdef GAME_DLL
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// skip all the restrictions and let bots pick up the flag
		PickUp( pPlayer, true );
		return;
	}
#endif

	// Is the touching player about to teleport?
	if ( pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
		return;

	// Don't let invulnerable players pickup flags, except in PD
	if ( pPlayer->m_Shared.IsInvulnerable() && m_nType != TF_FLAGTYPE_PLAYER_DESTRUCTION )
		return;

	// Don't let bonked players pickup flags
	if ( pPlayer->m_Shared.InCond( TF_COND_PHASE ) )
		return;

#ifdef GAME_DLL
	if ( PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() ) )
		return;
#endif

	// Do not allow the player to pick up multiple flags
	if ( pPlayer->HasTheFlag() && !( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION || m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION ) && !tf_flag_return_on_touch.GetBool() )
		return;

	if ( IsDropped() && pOther->GetTeamNumber() == GetTeamNumber() && 
		 m_nType == TF_FLAGTYPE_CTF && tf_flag_return_on_touch.GetBool() )
	{
		Reset();
		ResetMessage();
	}
	else
	{
		// Pick up the flag.
		PickUp( pPlayer, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( pPlayer->HasTheFlag() )
			return;
	}

#ifdef GAME_DLL
	if ( !m_bAllowOwnerPickup )
	{
		if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer ) 
		{
			return;
		}
	}

	if ( TFGameRules()->IsMannVsMachineMode() && pPlayer->IsBot() )
	{
		CTFBot *pBot = assert_cast<CTFBot *>( pPlayer );

		if ( pBot->HasAttribute( CTFBot::AttributeType::IGNOREFLAG ) )
			return;

		pBot->SetFlagTarget( this );
	}

	if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) && pPlayer->GetItem() != this )
		{
			CTFRobotDestructionLogic::GetRobotDestructionLogic()->FlagDestroyed( GetTeamNumber() );

			CCaptureFlag* pOtherFlag = static_cast<CCaptureFlag *>( pPlayer->GetItem() );
			pOtherFlag->AddPointValue( m_nPointValue );
			UTIL_Remove( this );

			return;
		}
	}
	else if ( m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		if ( pPlayer->HasItem() && ( pPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) && pPlayer->GetItem() != this )
		{
			CCaptureFlag* pOtherFlag = static_cast<CCaptureFlag *>( pPlayer->GetItem() );
			pOtherFlag->AddPointValue( m_nPointValue );
			UTIL_Remove( this );

			/*if ( CTFPlayerDestructionLogic::GetPlayerDestructionLogic() )
			{
				CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->CalcTeamLeader( pPlayer->GetTeamNumber() );
				CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->PlayPropPickupSound( pPlayer );
			}*/

			return;
		}
	}
#endif

	// Check whether we have a weapon that's prohibiting us from picking the flag up
	if ( !pPlayer->IsAllowedToPickUpFlag() )
		return;

	// Call into the base class pickup.
	BaseClass::PickUp( pPlayer, false );

	m_nSkin = GetIntelSkin(GetTeamNumber(), true);

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL

	pPlayer->AddGlowEffect();
	
	// Update the parent to set the correct place on the model to attach the flag.
	int iAttachment = pPlayer->LookupAttachment( "flag" );
	if( iAttachment != -1 )
	{
		SetParent( pPlayer, iAttachment );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}

	// Remove the player's disguse if they're a spy
	if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) ||
			pPlayer->m_Shared.InCond( TF_COND_DISGUISING ))
		{
			pPlayer->m_Shared.RemoveDisguise();
		}
	}

	// Remove the touch function.
	SetTouch( NULL );

	m_hPrevOwner = pPlayer;
	m_bAllowOwnerPickup = true;

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_CTF_ENEMY_STOLEN );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_TAKEN );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_CTF_TEAM_STOLEN );

				// exclude the guy who just picked it up
				filter.RemoveRecipient( pPlayer );

				TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_TAKEN );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_TakeFlagToPoint" );

		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				if ( TFGameRules()->IsMannVsMachineMode() )
				{
					PlaySound( filter, TF_MVM_AD_ENEMY_STOLEN );
				}
				else
				{
					PlaySound( filter, TF_AD_ENEMY_STOLEN );
				}
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_AD_TEAM_STOLEN );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerPickup" );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamPickup" );

		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamPickup" );

				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_INVADE_ENEMY_STOLEN );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_INVADE_TEAM_STOLEN );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		// Only do notifications when a neutral flag has been taken.
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
		{
			// Tell our team we picked up the flag.
			TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerPickup" );
			TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamPickup" );

			bool bEventMap = TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY );
			if ( bEventMap )
			{
				//TODO: TFGameRules()->InvalidateSomeTimerForDoomsdayEvent();
			}

			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				const char *pszSound = TF_RESOURCE_ENEMY_STOLEN;

				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					if ( bEventMap )
					{
						pszSound = TF_RESOURCE_EVENT_ENEMY_STOLEN;
					}

					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamPickup" );
				}
				else
				{
					pszSound = TF_RESOURCE_TEAM_STOLEN;
					if ( bEventMap )
					{
						pszSound = TF_RESOURCE_EVENT_TEAM_STOLEN;
					}
				}

				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, pszSound, iTeam );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_ENEMY_STOLEN, iTeam );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_TEAM_STOLEN, iTeam );
			}
		}
	}
	else if ( m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		/*if ( CTFPlayerDestructionLogic::GetPlayerDestructionLogic() )
		{
			CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->CalcTeamLeader( pPlayer->GetTeamNumber() );
			CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->PlayPropPickupSound( pPlayer );
		}*/
	}
	
	// Neutral flags: Set team.
	if ( m_nType == TF_FLAGTYPE_INVADE || m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		// Set the flag to be the carrier's team.
		ChangeTeam( pPlayer->GetTeamNumber() );
		m_nSkin = GetIntelSkin( GetTeamNumber(), true );
	}

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() && m_flTimeToSetPoisonous == 0.f )
	{
		m_flTimeToSetPoisonous = gpGlobals->curtime + 90.f;
	}

	if ( m_nFlagStatus == TF_FLAGINFO_NONE )
	{
		m_flLastResetDuration = GetMaxReturnTime();
	}
	else
	{
		m_flLastResetDuration = m_flResetTime - gpGlobals->curtime;
	}

	// Remember that this is when the item was picked up.
	m_flLastPickupTime = gpGlobals->curtime;

	int nOldFlagStatus = m_nFlagStatus;
	SetFlagStatus( TF_FLAGINFO_STOLEN, pPlayer );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_PICKUP );
		event->SetInt( "priority", 8 );
		event->SetInt( "home", ( nOldFlagStatus == TF_FLAGINFO_NONE ) ? 1 : 0 );
		event->SetInt( "team", GetTeamNumber() );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGPICKUP );

	// Output.
	m_outputOnPickUp.FireOutput( this, this );

	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_outputOnPickUpTeam1.FireOutput( this, this );
			break;

		case TF_TEAM_BLUE:
			m_outputOnPickUpTeam2.FireOutput(this, this);
			break;
	}

	variant_t value{};
	value.SetInt( pPlayer->GetTeamNumber() );
	m_outputOnPickUpByTeam.FireOutput( value, this, this );

	DestroyReturnIcon();

	HandleFlagPickedUpInDetectionZone( pPlayer );

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 0 );
		TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
		TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( nOldFlagStatus == TF_FLAGINFO_NONE )
		{
			/*if ( pPlayer->IsMiniBoss() )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_GIANT_HAS_BOMB, TF_TEAM_MVM_PLAYERS );
			}
			else*/
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_FIRST_BOMB_PICKUP, TF_TEAM_MVM_PLAYERS );
			}
		}
		else
		{
			/*if ( pPlayer->IsMiniBoss() )
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_GIANT_HAS_BOMB, TF_TEAM_MVM_PLAYERS );
			}
			else*/
			{
				TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_PICKUP, TF_TEAM_MVM_PLAYERS );
			}
		}
	}

#endif
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::CreateReturnIcon( void )
{
	m_hReturnIcon = CBaseEntity::Create( "item_teamflag_return_icon", GetAbsOrigin() + Vector(0,0,cl_flag_return_height.GetFloat()), vec3_angle, this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::DestroyReturnIcon( void )
{
	if ( m_hReturnIcon.Get() )
	{
		UTIL_Remove( m_hReturnIcon );
		m_hReturnIcon = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::AddFollower( CTFBot *pBot )
{
	if ( !m_hFollowers.HasElement( pBot ) )
	{
		m_hFollowers.AddToTail( pBot );
		for ( int i=0; i < m_tags.Count(); ++i )
		{
			//pBot->AddTag( m_tags[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::RemoveFollower( CTFBot *pBot )
{
	int index = m_hFollowers.Find( pBot );
	if ( index != m_hFollowers.InvalidIndex() )
	{
		m_hFollowers.Remove( index );
		for ( int i=0; i < m_tags.Count(); ++i )
		{
			//pBot->RemoveTag( m_tags[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CCaptureFlag::GetMaxReturnTime( void )
{
	float flReturnTime = m_flMaxResetTime.Get();
	if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		int nMaxPoints = 300;
		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		{
			nMaxPoints = CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetMaxPoints();
		}
		flReturnTime = RemapValClamped( m_nPointValue, 0.0f, (float)nMaxPoints / 3, tf_rd_return_min_time.GetFloat(), tf_rd_return_max_time.GetFloat() );
	}

	return flReturnTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::AddPointValue( int nPoints )
{ 
	m_nPointValue += nPoints;

	if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION || m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "flagstatus_update" );
		if ( pEvent )
		{
			pEvent->SetInt( "userid", m_hPrevOwner && m_hPrevOwner->IsPlayer() ? ToBasePlayer( m_hPrevOwner )->GetUserID() : -1 );
			pEvent->SetInt( "entindex", entindex() );
			gameeventmanager->FireEvent( pEvent );

			// The return time is determined by how many points are in the flag, so update that. 
			m_flLastResetDuration = GetMaxReturnTime();
		}

		if ( nPoints > 0 )
		{
			pEvent = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( pEvent )
			{
				pEvent->SetInt( "player", m_hPrevOwner && m_hPrevOwner->IsPlayer() ? ToBasePlayer( m_hPrevOwner )->entindex() : -1 );
				pEvent->SetInt( "eventtype", TF_FLAGEVENT_PICKUP );
				gameeventmanager->FireEvent( pEvent );
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Capture( CTFPlayer *pPlayer, int nCapturePoint )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

#ifdef GAME_DLL

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_CTF_ENEMY_CAPTURED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
					{
						PlaySound( filter, TF_RUNE_INTEL_CAPTURED );
					}
					else
					{	
						PlaySound( filter, TF_CTF_TEAM_CAPTURED );
					}

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_CAPTURED );
				}
			}
		}

		// Give temp crit boost to capper's team.
		if ( TFGameRules() )
		{
			TFGameRules()->HandleCTFCaptureBonus( pPlayer->GetTeamNumber() );
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_CTF_CAPTURED_TEAM_FRAGS );
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		char szNumber[64];
		Q_snprintf( szNumber, sizeof(szNumber), "%d", nCapturePoint );

		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_AD_YouSecuredPoint", szNumber );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "TF_AD_AttackersSecuredPoint", szNumber );
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_AD_AttackersSecuredPoint" );

				CTeamRecipientFilter filter( iTeam, true );
				if ( TFGameRules()->IsMannVsMachineMode() )
				{
					PlaySound( filter, TF_MVM_AD_ENEMY_CAPTURED );
				}
				else
				{
					PlaySound( filter, TF_AD_ENEMY_CAPTURED );
				}
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_AD_TEAM_CAPTURED );
			}
		}

		// Capture sound
		CBroadcastRecipientFilter filter;
		PlaySound( filter, TF_AD_CAPTURED_SOUND );

		// Reward the player
		pPlayer->IncrementFragCount( TF_AD_CAPTURED_FRAGS );

		// TFTODO:: Reward the team	
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerCapture" );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamCapture" );

		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamCapture" );

				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_INVADE_ENEMY_CAPTURED );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_INVADE_TEAM_CAPTURED );
			}
		}

		// Reward the player
		pPlayer->IncrementFragCount( TF_INVADE_CAPTURED_FRAGS );

		// Reward the team
		TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );

		// This was added by request for the "push" map -danielmm8888
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );
		}
	}
	else if ( m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		// Pretty much the Invasion code with some tweaks.
		
		// Handle messages to the screen.
		TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerCapture" );
		TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_PlayerTeamCapture" );

		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			TFGameRules()->BroadcastSound( 255, ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) ? TF_RESOURCE_EVENT_RED_CAPPED : TF_RESOURCE_EVENT_BLUE_CAPPED );
		}
		else
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_OtherTeamCapture" );

					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_RESOURCE_ENEMY_CAPTURED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_RESOURCE_TEAM_CAPTURED );
				}
			}
		}

		// Reward the player
		pPlayer->IncrementFragCount( TF_INVADE_CAPTURED_FRAGS );

		// Reward the team
		TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );

		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_INVADE_CAPTURED_TEAM_FRAGS );
		}
	}
	else if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_ENEMY_CAPTURED, iTeam );
			}
			else
			{
				CTeamRecipientFilter filter( iTeam, true );
				PlaySound( filter, TF_RD_TEAM_CAPTURED, iTeam );
			}
		}

		// Score points!
		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		{
			CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( pPlayer->GetTeamNumber(), m_nPointValue.Get(), SCORE_REACTOR_CAPTURED, pPlayer );
			CTFRobotDestructionLogic::GetRobotDestructionLogic()->FlagDestroyed( GetTeamNumber() );
			m_nPointValue = 0;
		}
	}
	
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_CAPTURE );
		event->SetInt( "priority", 9 );
		event->SetInt( "team", GetTeamNumber() );
		gameeventmanager->FireEvent( event );
	}

	if ( IsPoisonous() )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_MARKEDFORDEATH );
	}

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	HandleFlagCapturedInDetectionZone( pPlayer );		
	HandleFlagDroppedInDetectionZone( pPlayer );

	// Reset the flag.
	BaseClass::Drop( pPlayer, true );

	Reset();

	pPlayer->TeamFortress_SetSpeed();
	pPlayer->RemoveGlowEffect();
	if ( !TFGameRules() || !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
	{
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );
	}
	
	// Output.
	m_outputOnCapture.FireOutput( this, this );

	switch (pPlayer->GetTeamNumber())
	{
		case TF_TEAM_RED:
			m_outputOnCapTeam1.FireOutput(this, this);
			break;

		case TF_TEAM_BLUE:
			m_outputOnCapTeam2.FireOutput(this, this);
			break;
	}

	variant_t value{};
	value.SetInt( pPlayer->GetTeamNumber() );
	m_outputOnCapByTeam.FireOutput( value, this, this );

	m_bCaptured = true;
	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );

	if ( TFGameRules()->InStalemate() )
	{
		// whoever capped the flag is the winner, give them enough caps to win
		CTFTeam *pTeam = pPlayer->GetTFTeam();
		if ( !pTeam )
			return;

		// if we still need more caps to trigger a win, give them to us
		if ( pTeam->GetFlagCaptures() < tf_flag_caps_per_round.GetInt() )
		{
			pTeam->SetFlagCaptures( tf_flag_caps_per_round.GetInt() );
		}	
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: A player drops the flag.
//-----------------------------------------------------------------------------
void CCaptureFlag::Drop( CTFPlayer *pPlayer, bool bVisible,  bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	// Call into the base class drop.
	BaseClass::Drop( pPlayer, bVisible );

	pPlayer->TeamFortress_SetSpeed();

#ifdef GAME_DLL

	pPlayer->RemoveGlowEffect();

	if ( bThrown )
	{
		m_bAllowOwnerPickup = false;
		m_flOwnerPickupTime = gpGlobals->curtime + TF_FLAG_OWNER_PICKUP_TIME;
	}

	Vector vecStart = GetAbsOrigin();
	Vector vecEnd = vecStart;
	vecEnd.z -= 8000.0f;
	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trace );
	SetAbsOrigin( trace.endpos );

	if ( m_nType == TF_FLAGTYPE_CTF )
	{
		if ( bMessage  )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_CTF_ENEMY_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_YOUR_FLAG_DROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_CTF_TEAM_DROPPED );

					TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_ENEMY_FLAG_DROPPED );
				}
			}
		}

		if (tf2v_assault_ctf_rules.GetBool())
			SetFlagReturnIn( TF_ACTF_RESET_TIME );
		else
			SetFlagReturnIn( TF_CTF_RESET_TIME );
	}
	else if ( m_nType == TF_FLAGTYPE_INVADE )
	{
		if ( bMessage  )
		{
			// Handle messages to the screen.
			TFTeamMgr()->PlayerCenterPrint( pPlayer, "#TF_Invade_PlayerFlagDrop" );
			TFTeamMgr()->PlayerTeamCenterPrint( pPlayer, "#TF_Invade_FlagDrop" );

			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagDrop" );

					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_INVADE_ENEMY_DROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_INVADE_TEAM_DROPPED );
				}
			}
		}

		SetFlagReturnIn( TF_INVADE_RESET_TIME );
		SetFlagNeutralIn( TF_INVADE_NEUTRAL_TIME );
	}
	else if ( m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		if ( bMessage  )
		{
			const char *pszSound = TF_RESOURCE_TEAM_DROPPED;
			if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
			{
				pszSound = TF_RESOURCE_EVENT_TEAM_DROPPED;
			}

			// We only care about our own team dropping it
			int iTeam = pPlayer->GetTeamNumber();

			CTeamRecipientFilter filter( iTeam, true );
			PlaySound( filter, pszSound, iTeam );
		}

		SetFlagReturnIn( TF_RESOURCE_RESET_TIME );
		SetFlagNeutralIn( TF_RESOURCE_NEUTRAL_TIME );
	}
	else if ( m_nType == TF_FLAGTYPE_ATTACK_DEFEND )
	{
		if ( bMessage  )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					if ( TFGameRules()->IsMannVsMachineMode() )
					{
						PlaySound( filter, TF_MVM_AD_ENEMY_DROPPED );
					}
					else
					{
						PlaySound( filter, TF_AD_ENEMY_DROPPED );
					}
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_AD_TEAM_DROPPED );
				}
			}
		}

		SetFlagReturnIn( TF_AD_RESET_TIME );
	}
	else if ( m_nType == TF_FLAGTYPE_ROBOT_DESTRUCTION )
	{
		// If a player dropped this flag but the flag has less than the min to steal
		// we just return the flag rather than have it exist on the ground
		if ( GetPointValue() < tf_rd_min_points_to_steal.GetInt() )
		{
			Reset();
			ResetMessage();
			return;
		}
		else if ( bMessage )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
			{
				// We only care about our own team dropping it in Special Delivery
				if ( iTeam == pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_RD_TEAM_DROPPED, iTeam );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					PlaySound( filter, TF_RD_ENEMY_DROPPED, iTeam );
				}
			}
		}

		SetFlagReturnIn( GetMaxReturnTime() );
	}
	else if ( m_nType == TF_FLAGTYPE_PLAYER_DESTRUCTION )
	{
		/*if ( CTFPlayerDestructionLogic::GetPlayerDestructionLogic() )
		{
			CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->CalcTeamLeader( pPlayer->GetTeamNumber() );
			CTFPlayerDestructionLogic::GetPlayerDestructionLogic()->PlayPropDropSound( pPlayer );
		}*/
	}

	if ( IsPoisonous() )
	{
		pPlayer->m_Shared.RemoveCond( TF_COND_MARKEDFORDEATH );
	}

	m_nSkin = GetIntelSkin( GetTeamNumber() );

	// Reset the flag's angles.
	SetAbsAngles( m_vecResetAng );

	// Reset the touch function.
	SetTouch( &CCaptureFlag::FlagTouch );

	SetFlagStatus( TF_FLAGINFO_DROPPED );

	// Output.
	m_outputOnDrop.FireOutput( this, this );

	if ( !TFGameRules()->IsMannVsMachineMode() || ( GetMaxReturnTime() < 600 ) )
	{
		CreateReturnIcon();
	}

	HandleFlagDroppedInDetectionZone( pPlayer );	

	if ( PointInRespawnFlagZone( GetAbsOrigin() ) )
	{
		Reset();
		ResetMessage();
	}

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 0 );
		TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
		TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_DROPPED, TF_TEAM_MVM_PLAYERS );
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDropped( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_DROPPED ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsHome( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_NONE ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsStolen( void )
{ 
	return ( m_nFlagStatus == TF_FLAGINFO_STOLEN ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CCaptureFlag::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		if ( m_bVisibleWhenDisabled )
		{
			SetRenderMode( kRenderTransAlpha );
			SetRenderColorA( 180 );
			RemoveEffects( EF_NODRAW );
		}
		else
		{
			AddEffects( EF_NODRAW );
		}

		SetTouch( NULL );
		SetThink( NULL );
	}
	else
	{
		RemoveEffects( EF_NODRAW );
		SetRenderMode( kRenderNormal );
		SetRenderColorA( 255 );

		SetTouch( &CCaptureFlag::FlagTouch );
		SetThink( &CCaptureFlag::Think );
		SetNextThink( gpGlobals->curtime );

	#ifdef GAME_DLL
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) && ( GetTeamNumber() == TEAM_UNASSIGNED ) )
		{
			//TODO: TFGameRules()->StartSomeTimerForDoomsdayEvent();
		}
	#endif
	}

#ifdef CLIENT_DLL
	UpdateGlowEffect();
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::SetVisibleWhenDisabled( bool bVisible )
{
	m_bVisibleWhenDisabled = bVisible;
	SetDisabled( IsDisabled() ); 
}

//-----------------------------------------------------------------------------
// Purpose: Sets the flag status
//-----------------------------------------------------------------------------
void CCaptureFlag::SetFlagStatus( int iStatus, CBasePlayer *pNewOwner /*= NULL*/ )
{
#ifdef GAME_DLL
	MDLCACHE_CRITICAL_SECTION();
#endif

	if ( m_nFlagStatus != iStatus )
	{
		m_nFlagStatus = iStatus;
	#ifdef GAME_DLL
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "flagstatus_update" );
		if ( pEvent )
		{
			pEvent->SetInt( "userid", pNewOwner ? pNewOwner->GetUserID() : -1 );
			pEvent->SetInt( "entindex", entindex() );
			gameeventmanager->FireEvent( pEvent );
		}
	#endif
	}

#ifdef CLIENT_DLL
	UpdateGlowEffect();
#endif

#ifdef GAME_DLL
	switch ( m_nFlagStatus )
	{
		case TF_FLAGINFO_NONE:
		case TF_FLAGINFO_DROPPED:
			ResetSequence( LookupSequence( "spin" ) );	// set spin animation if it's not being held
			break;
		case TF_FLAGINFO_STOLEN:
			ResetSequence( LookupSequence( "idle" ) );	// set idle animation if it is being held
			break;
		default:
			AssertOnce( false );	// invalid stats
			break;
	}
#endif
}

//-----------------------------------------------------------------------------------------------
// GAME DLL Functions
//-----------------------------------------------------------------------------------------------
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::Think( void )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
	{
		SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
		return;
	}

	if ( m_bCaptured )
	{
		m_bCaptured = false;
		SetTouch( &CCaptureFlag::FlagTouch );
	}

	ManageSpriteTrail();

	if ( IsDropped() )
	{
		if ( !m_bAllowOwnerPickup )
		{
			if ( m_flOwnerPickupTime && gpGlobals->curtime > m_flOwnerPickupTime )
			{
				m_bAllowOwnerPickup = true;
			}
		}

		if ( TFGameRules()->IsMannVsMachineMode() && m_bReturnBetweenWaves )
		{
			if ( TFGameRules()->InSetup() || ( TFObjectiveResource() && TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() ) )
			{
				Reset();
			}
		}

		if ( m_nType == TF_FLAGTYPE_INVADE || m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
			else if ( m_flNeutralTime && gpGlobals->curtime > m_flNeutralTime )
			{
				for ( int iTeam = TF_TEAM_RED; iTeam < TF_TEAM_COUNT; ++iTeam )
				{
					TFTeamMgr()->TeamCenterPrint( iTeam, "#TF_Invade_FlagNeutral" );
				}

				// reset the team to the original team setting (when it spawned)
				ChangeTeam( m_iOriginalTeam );
				m_nSkin = GetIntelSkin( GetTeamNumber() );

				ResetFlagNeutralTime();
			}
		}
		else
		{
			if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
			{
				Reset();
				ResetMessage();
			}
		}
		m_nSkin = GetIntelSkin( GetTeamNumber() );
	}

	if ( m_nType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			// TODO
		}
	}

	CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );
	if ( IsStolen() && TFGameRules() && TFGameRules()->IsPowerupMode() && IsPoisonous() && !pPlayer->m_Shared.InCond( TF_COND_MARKEDFORDEATH ) )
	{
		pPlayer->m_Shared.AddCond( TF_COND_MARKEDFORDEATH );
	}

	SetNextThink( gpGlobals->curtime + TF_FLAG_THINK_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Handles the team colored trail sprites
//-----------------------------------------------------------------------------
void CCaptureFlag::ManageSpriteTrail( void )
{
	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN )
	{
		if ( m_nUseTrailEffect == FLAG_EFFECTS_ALL || m_nUseTrailEffect == FLAG_EFFECTS_COLORONLY )
		{
			if ( GetPrevOwner() /*&& GetPrevOwner() != CBasePlayer::GetLocalPlayer() */ )
			{
				char pEffectName[512];
				CTFPlayer *pPlayer = ToTFPlayer(GetPrevOwner());
				if ( pPlayer )
				{
					if ( pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
					{
						if ( m_pGlowTrail == NULL )
						{
							GetTrailEffect( pPlayer->GetTeamNumber(), pEffectName, sizeof( pEffectName ) );
							m_pGlowTrail = CSpriteTrail::SpriteTrailCreate( pEffectName, GetLocalOrigin(), false );
							m_pGlowTrail->FollowEntity( this );
							m_pGlowTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 96, 0 );
							m_pGlowTrail->SetBrightness( 96 );
							m_pGlowTrail->SetStartWidth( 32.0 );
							m_pGlowTrail->SetTextureResolution( 0.010416667 );
							m_pGlowTrail->SetLifeTime( 0.69999999 );
							m_pGlowTrail->SetTransmit( false );
							m_pGlowTrail->SetParent( this );
						}
					}
					else
					{
						if ( m_pGlowTrail )
						{
							m_pGlowTrail->Remove();
							m_pGlowTrail = NULL;
						}
					}
				}
			}
		}
	}
	else
	{
		if ( m_pGlowTrail )
		{
			m_pGlowTrail->Remove();
			m_pGlowTrail = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCaptureFlag::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceDrop( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		pPlayer->DropFlag();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputRoundActivate( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}

void CCaptureFlag::InputForceReset( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		pPlayer->DropFlag();
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceResetSilent( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		pPlayer->DropFlag( true );
	}

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceResetAndDisableSilent( inputdata_t &inputdata )
{
	InputForceResetSilent( inputdata );
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputSetReturnTime( inputdata_t &inputdata )
{
	int nReturnTime = inputdata.value.Int();
	nReturnTime = Max( 0, nReturnTime );

	if ( IsDropped() )
	{
		// do we currently have a neutral time?
		if ( m_flNeutralTime > 0 )
		{
			// if our return time is less than the neutral time, we don't need a neutral time
			if ( TF_INVADE_NEUTRAL_TIME < nReturnTime )
			{
				SetFlagNeutralIn( TF_INVADE_NEUTRAL_TIME );
			}
		}

		SetFlagReturnIn( nReturnTime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputShowTimer( inputdata_t &inputdata )
{
	int nReturnTime = inputdata.value.Int();
	SetFlagReturnIn( Max( 0, nReturnTime ) );

	CreateReturnIcon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::InputForceGlowDisabled( inputdata_t &inputdata )
{
	int nState = inputdata.value.Int();
	SetGlowEnabled( nState == 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureFlag::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCaptureFlag::PlaySound( IRecipientFilter& filter, char const *pszString, int iTeam /*= TEAM_ANY */ )
{
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// Don't play bomb announcements unless we're at least 5 seconds into a wave in MVM
		if ( !( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING && gpGlobals->curtime - TFGameRules()->GetLastRoundStateChangeTime() >= 5.0f ) )
		{
			return;
		}

		if ( !V_strcmp( pszString, TF_MVM_AD_ENEMY_RETURNED ) )
		{
			EmitSound( filter, entindex(), pszString );
		}
	}
	else if ( TFGameRules()->IsInSpecialDeliveryMode() && ( !V_strcmp( pszString, TF_RESOURCE_TEAM_DROPPED ) || !V_strcmp( pszString, TF_RESOURCE_EVENT_TEAM_DROPPED ) ) )
	{
		// Rate limit certain flag sounds in Special Delivery
		if ( iTeam == TEAM_ANY || gpGlobals->curtime >= m_flNextTeamSoundTime[ iTeam ]  )
		{
			EmitSound( filter, entindex(), pszString );

			if ( iTeam != TEAM_ANY )
			{
				m_flNextTeamSoundTime[ iTeam ] = gpGlobals->curtime + 20.0f;
			}
		}
	}
	else
	{
		EmitSound( filter, entindex(), pszString );
	}
}

#else

float CCaptureFlag::GetReturnProgress()
{
	float flEventTime = max( m_flResetTime.m_Value, m_flNeutralTime.m_Value );

	return ( 1.0 - ( ( flEventTime - gpGlobals->curtime ) / m_flMaxResetTime ) );
}


void CCaptureFlag::Simulate( void )
{
	BaseClass::Simulate();
	ManageTrailEffects();
}

void CCaptureFlag::ManageTrailEffects( void )
{
	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN )
	{
		if ( m_nUseTrailEffect == FLAG_EFFECTS_NONE || m_nUseTrailEffect == FLAG_EFFECTS_COLORONLY )
			return;

		if ( GetPrevOwner() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );

			if ( pPlayer )
			{
				if ( pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
				{
					if ( m_pPaperTrailEffect == NULL )
					{
						m_pPaperTrailEffect = ParticleProp()->Create( m_szPaperEffect, PATTACH_ABSORIGIN_FOLLOW );
					}
				}
				else
				{
					if ( m_pPaperTrailEffect )
					{
						ParticleProp()->StopEmission( m_pPaperTrailEffect );
						m_pPaperTrailEffect = NULL;
					}
				}
			}
		}

	}
	else
	{
		if ( m_pPaperTrailEffect )
		{
			ParticleProp()->StopEmission( m_pPaperTrailEffect );
			m_pPaperTrailEffect = NULL;
		}
	}
}


#endif


LINK_ENTITY_TO_CLASS( item_teamflag_return_icon, CCaptureFlagReturnIcon );

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )

BEGIN_NETWORK_TABLE( CCaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
END_NETWORK_TABLE()

CCaptureFlagReturnIcon::CCaptureFlagReturnIcon()
{
#ifdef CLIENT_DLL
	m_pReturnProgressMaterial_Empty = NULL;
	m_pReturnProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCaptureFlagReturnIcon::Spawn( void )
{
	BaseClass::Spawn();

	UTIL_SetSize( this, Vector(-8,-8,-8), Vector(8,8,8) );

	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}

int CCaptureFlagReturnIcon::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_segment_t;


// This defines the properties of the 8 circle segments
// in the circular progress bar.
progress_segment_t Segments[8] = 
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25,	 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50,	 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75,	 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0,	 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderGroup_t CCaptureFlagReturnIcon::GetRenderGroup( void ) 
{	
	return RENDER_GROUP_TRANSLUCENT_ENTITY;	
}

void CCaptureFlagReturnIcon::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init(  20,  20,  20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCaptureFlagReturnIcon::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );
	
	DrawReturnProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCaptureFlagReturnIcon::DrawReturnProgressBar( void )
{
	CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * > ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetMaxResetTime() <= 0 )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "VGUI/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "VGUI/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;

	switch( pFlag->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		ubColor[0] = 255;
		ubColor[1] = 0;
		ubColor[2] = 0;
		break;
	case TF_TEAM_BLUE:
		ubColor[0] = 0;
		ubColor[1] = 0;
		ubColor[2] = 255;
		break;
	case TF_TEAM_GREEN:
		ubColor[0] = 0;
		ubColor[1] = 255;
		ubColor[2] = 0;
		break;
	case TF_TEAM_YELLOW:
		ubColor[0] = 255;
		ubColor[1] = 255;
		ubColor[2] = 0;
		break;
	default:
		ubColor[0] = 100;
		ubColor[1] = 100;
		ubColor[2] = 100;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	float flProgress = pFlag->GetReturnProgress();

	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i=0;i<8;i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( swipe_x - 0.5 ) ) + (vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( Segments[i].vert2x - 0.5 ) ) + (vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif
