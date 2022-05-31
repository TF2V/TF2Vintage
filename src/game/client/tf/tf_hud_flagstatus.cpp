//========= Copyright � 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "teamplay_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_team_objectiveresource.h"
#include "c_func_capture_zone.h"
#include "tf_hud_objectivestatus.h"
#include "tf_hud_flagstatus.h"
#include "tf_spectatorgui.h"
#include "teamplayroundbased_gamerules.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"
#include "tier1/fmtstr.h"
#include "view.h"
#include "prediction.h"
#include "tf_logic_robot_destruction.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFArrowPanel );
DECLARE_BUILD_FACTORY( CTFFlagStatus );

DECLARE_HUDELEMENT( CTFFlagCalloutPanel );

ConVar tf_rd_flag_ui_mode( "tf_rd_flag_ui_mode", "3", FCVAR_DEVELOPMENTONLY, "When flags are stolen and not visible: 0 = Show outlines (glows), 1 = Show most valuable enemy flag (icons), 2 = Show all enemy flags (icons), 3 = Show all flags (icons)." );

extern ConVar tf_flag_caps_per_round;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFArrowPanel::CTFArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI );
	m_GreenMaterial.Init("hud/objectives_flagpanel_compass_green", TEXTURE_GROUP_VGUI);
	m_YellowMaterial.Init("hud/objectives_flagpanel_compass_yellow", TEXTURE_GROUP_VGUI);
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI ); 

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI );
	m_GreenMaterialNoArrow.Init("hud/objectives_flagpanel_compass_green_noArrow", TEXTURE_GROUP_VGUI);
	m_YellowMaterialNoArrow.Init("hud/objectives_flagpanel_compass_yellow_noArrow", TEXTURE_GROUP_VGUI);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();

	if ( pPlayer && pEnt )
	{
		QAngle vangles;
		Vector eyeOrigin;
		float zNear, zFar, fov;

		pPlayer->CalcView( eyeOrigin, vangles, zNear, zFar, fov );

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos( dot );

		dot = DotProduct( vecFlag, right );

		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArrowPanel::Paint()
{
	if ( !m_hEntity.Get() )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	IMaterial *pMaterial = m_NeutralMaterial;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// figure out what material we need to use
	if ( pEnt->GetTeamNumber() == TF_TEAM_RED )
	{
		pMaterial = m_RedMaterial;

		if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
		{
			// is our target a player?
			C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
			if ( pTargetEnt && pTargetEnt->IsPlayer() )
			{
				// does our target have the flag and are they carrying the flag we're currently drawing?
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
				if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
				{
					pMaterial = m_RedMaterialNoArrow;
				}
			}
		}
	}
	else if ( pEnt->GetTeamNumber() == TF_TEAM_BLUE )
	{
		pMaterial = m_BlueMaterial;

		if ( pLocalPlayer && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
		{
			// is our target a player?
			C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
			if ( pTargetEnt && pTargetEnt->IsPlayer() )
			{
				// does our target have the flag and are they carrying the flag we're currently drawing?
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pTargetEnt );
				if ( pTarget->HasTheFlag() && ( pTarget->GetItem() == pEnt ) )
				{
					pMaterial = m_BlueMaterialNoArrow;
				}
			}
		}
	}
	else if (pEnt->GetTeamNumber() == TF_TEAM_GREEN)
	{
		pMaterial = m_GreenMaterial;

		if (pLocalPlayer && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE))
		{
			// is our target a player?
			C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
			if (pTargetEnt && pTargetEnt->IsPlayer())
			{
				// does our target have the flag and are they carrying the flag we're currently drawing?
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >(pTargetEnt);
				if (pTarget->HasTheFlag() && (pTarget->GetItem() == pEnt))
				{
					pMaterial = m_GreenMaterialNoArrow;
				}
			}
		}
	}
	else if (pEnt->GetTeamNumber() == TF_TEAM_YELLOW)
	{
		pMaterial = m_YellowMaterial;

		if (pLocalPlayer && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE))
		{
			// is our target a player?
			C_BaseEntity *pTargetEnt = pLocalPlayer->GetObserverTarget();
			if (pTargetEnt && pTargetEnt->IsPlayer())
			{
				// does our target have the flag and are they carrying the flag we're currently drawing?
				C_TFPlayer *pTarget = static_cast< C_TFPlayer* >(pTargetEnt);
				if (pTarget->HasTheFlag() && (pTarget->GetItem() == pEnt))
				{
					pMaterial = m_YellowMaterialNoArrow;
				}
			}
		}
	}

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );
	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix(); 

	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
//	MatrixRotate( panelRotation, Vector( 1, 0, 0 ), 5 );
	panelRotation.SetTranslation( Vector( x + nWidth/2, y + nHeight/2, 0 ) );
	pRenderContext->LoadMatrix( panelRotation );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3f( -nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3f( nWidth/2, -nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3f( nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3f( -nWidth/2, nHeight/2, 0 );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFArrowPanel::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagStatus::CTFFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = NULL;
	m_pStatusIcon = NULL;
	m_pBriefcase = NULL;
	m_hEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res" );

	m_pArrow = dynamic_cast<CTFArrowPanel *>( FindChildByName( "Arrow" ) );
	m_pStatusIcon = dynamic_cast<CTFImagePanel *>( FindChildByName( "StatusIcon" ) );
	m_pBriefcase = dynamic_cast<CTFImagePanel *>( FindChildByName( "Briefcase" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagStatus::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );
	
		if ( pFlag )
		{
			const char *pszImage = "../hud/objectives_flagpanel_ico_flag_home";

			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudFlagObjectives::CTFHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "flagstatus_update" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudFlagObjectives::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( TFGameRules() && TFGameRules()->IsInHybridCTF_CPMode() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_hybrid" );
	}
	else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_mvm" );
	}
	else if ( TFGameRules() && TFGameRules()->IsInSpecialDeliveryMode() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_specialdelivery" );
	}

	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );

	m_pCarriedImage = dynamic_cast<CTFImagePanel *>( FindChildByName( "CarriedImage" ) );
	m_pPlayingTo = dynamic_cast<CExLabel *>( FindChildByName( "PlayingTo" ) );
	m_pPlayingToBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "PlayingToBG" ) );
	m_pRedFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "RedFlag" ) );
	m_pBlueFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "BlueFlag" ) );
	m_pGreenFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "GreenFlag" ) );
	m_pYellowFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "YellowFlag" ) );
	
	m_pCapturePoint = dynamic_cast<CTFArrowPanel *>( FindChildByName( "CaptureFlag" ) );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "SpecCarriedImage" ) );

	// outline is always on, so we need to init the alpha to 0
	CTFImagePanel *pOutline = dynamic_cast<CTFImagePanel *>( FindChildByName( "OutlineImage" ) );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	UpdateStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
	{
		m_pCarriedImage->SetVisible( false );
	}

	if ( m_pBlueFlag && !m_pBlueFlag->IsVisible() )
	{
		m_pBlueFlag->SetVisible( true );
	}

	if ( m_pRedFlag && !m_pRedFlag->IsVisible() )
	{
		m_pRedFlag->SetVisible( true );
	}
	
	if (m_pGreenFlag && !m_pGreenFlag->IsVisible())
	{
		m_pGreenFlag->SetVisible(true);
	}

	if (m_pYellowFlag && !m_pYellowFlag->IsVisible())
	{
		m_pYellowFlag->SetVisible(true);
	}

	if ( m_pSpecCarriedImage && m_pSpecCarriedImage->IsVisible() )
	{
		m_pSpecCarriedImage->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		if ( m_pPlayingTo->IsVisible() != bVisible )
		{
			m_pPlayingTo->SetVisible( bVisible );
		}

		if ( m_pPlayingToBG->IsVisible() != bVisible )
		{
			m_pPlayingToBG->SetVisible( bVisible );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::OnTick()
{
	// iterate through the flags to set their position in our HUD
	for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); i++ )
	{
		CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );

		if ( pFlag )
		{
			if (!pFlag->IsDisabled())
			{
				if (m_pRedFlag && pFlag->GetTeamNumber() == TF_TEAM_RED)
				{
					m_pRedFlag->SetEntity(pFlag);
				}
				else if (m_pBlueFlag && pFlag->GetTeamNumber() == TF_TEAM_BLUE)
				{
					m_pBlueFlag->SetEntity(pFlag);
				}
				else if (m_pGreenFlag && pFlag->GetTeamNumber() == TF_TEAM_GREEN)
				{
					m_pGreenFlag->SetEntity(pFlag);
				}
				else if (m_pYellowFlag && pFlag->GetTeamNumber() == TF_TEAM_YELLOW)
				{
					m_pYellowFlag->SetEntity(pFlag);
				}
			}
		}

		if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() && CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetType() == CTFRobotDestructionLogic::TYPE_ROBOT_DESTRUCTION )
		{
			if ( tf_rd_flag_ui_mode.GetInt() && !pFlag->IsDisabled() && !pFlag->IsHome() )
			{
				Vector vecLocation = pFlag->GetAbsOrigin() + Vector( 0.f, 0.f, 18.f );
				CTFFlagCalloutPanel::AddFlagCalloutIfNotFound( pFlag, FLT_MAX, vecLocation );
			}
		}
	}

	// Hide the capture counter on Special Delivery.
	if (!TFGameRules() || (TFGameRules() && !TFGameRules()->IsInSpecialDeliveryMode()))
	{
		// are we playing captures for rounds?
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->GetFlagCaptures() );
			}
			
			pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
			if ( pTeam )
			{
				SetDialogVariable( "greenscore", pTeam->GetFlagCaptures() );
			}
			
			pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
			if ( pTeam )
			{
				SetDialogVariable( "yellowscore", pTeam->GetFlagCaptures() );
			}

			SetPlayingToLabelVisible( true );
			SetDialogVariable( "rounds", tf_flag_caps_per_round.GetInt() );
		}
		else // we're just playing straight score
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->Get_Score() );
			}
			
			pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
			if ( pTeam )
			{
				SetDialogVariable( "greenscore", pTeam->Get_Score() );
			}
			
			pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
			if ( pTeam )
			{
				SetDialogVariable( "yellowscore", pTeam->Get_Score() );
			}

			SetPlayingToLabelVisible( false );
		}
	}

	// check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag
	bool bSpecCarriedImage = false;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
	{
		// does our target have the flag?
		C_BaseEntity *pEnt = pPlayer->GetObserverTarget();
		if ( pEnt && pEnt->IsPlayer() )
		{
			C_TFPlayer *pTarget = static_cast< C_TFPlayer* >( pEnt );
			if ( pTarget->HasTheFlag() )
			{
				bSpecCarriedImage = true;

				CCaptureFlag *pPlayerFlag = dynamic_cast<CCaptureFlag*>(pTarget->GetItem());

				if ( m_pSpecCarriedImage )
				{
					char szHudIcon[ MAX_PATH ];
					pPlayerFlag->GetHudIcon( pPlayerFlag->GetTeamNumber(), szHudIcon, sizeof( szHudIcon ) );
					m_pSpecCarriedImage->SetImage(szHudIcon);
				}
			}
		}
	}

	if ( bSpecCarriedImage )
	{
		if ( m_pSpecCarriedImage && !m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( true );
		}
	}
	else
	{
		if ( m_pSpecCarriedImage && m_pSpecCarriedImage->IsVisible() )
		{
			m_pSpecCarriedImage->SetVisible( false );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::UpdateStatus( void )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && ( pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG ) )
	{
		pPlayerFlag = dynamic_cast<CCaptureFlag*>( pLocalPlayer->GetItem() );
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( m_pCarriedImage && !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;

			// Set the correct flag image depending on the flag we're holding
			switch (pPlayerFlag->GetTeamNumber())
			{
				case TF_TEAM_RED:
					m_pCarriedImage->SetImage("../hud/objectives_flagpanel_carried_red");
					break;	
				case TF_TEAM_BLUE:
					m_pCarriedImage->SetImage("../hud/objectives_flagpanel_carried_blue");
					break;
				case TF_TEAM_GREEN:
					m_pCarriedImage->SetImage("../hud/objectives_flagpanel_carried_green");
					break;
				case TF_TEAM_YELLOW:
					m_pCarriedImage->SetImage("../hud/objectives_flagpanel_carried_yellow");
					break;
			}

			if (m_pRedFlag && m_pRedFlag->IsVisible())
			{
				m_pRedFlag->SetVisible(false);
			}

			if ( m_pBlueFlag && m_pBlueFlag->IsVisible() )
			{
				m_pBlueFlag->SetVisible( false );
			}
			
			if (m_pGreenFlag && m_pGreenFlag->IsVisible())
			{
				m_pGreenFlag->SetVisible(false);
			}

			if (m_pYellowFlag && m_pYellowFlag->IsVisible())
			{
				m_pYellowFlag->SetVisible(false);
			}

			if ( !m_pCarriedImage->IsVisible() )
			{
				m_pCarriedImage->SetVisible( true );
			}

			ConVarRef cl_hud_console( "cl_hud_console" );
			if ( cl_hud_console.IsValid() && cl_hud_console.GetBool() )
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineConsole" );
			else
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );

			if ( m_pCapturePoint )
			{
				if ( !m_pCapturePoint->IsVisible() )
				{
					m_pCapturePoint->SetVisible( true );
				}

				if ( pLocalPlayer )
				{
					// go through all the capture zones and find ours
					for ( int i = 0; i < ICaptureZoneAutoList::AutoList().Count(); i++ )
					{
						C_CaptureZone *pZone = static_cast<C_CaptureZone *>( ICaptureZoneAutoList::AutoList()[i] );
						if ( !pZone->IsDormant() )
						{
							if ( pZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() )
							{
								m_pCapturePoint->SetEntity( pZone );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		// were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;

			ConVarRef cl_hud_console( "cl_hud_console" );
			if ( cl_hud_console.IsValid() && cl_hud_console.GetBool() )
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineConsole" );
			else
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		m_bFlagAnimationPlayed = false;

		if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint && m_pCapturePoint->IsVisible() )
		{
			m_pCapturePoint->SetVisible( false );
		}

		if ( m_pBlueFlag )
		{
			if ( !m_pBlueFlag->IsVisible() )
			{
				m_pBlueFlag->SetVisible( true );
			}
			
			m_pBlueFlag->UpdateStatus();
		}

		if ( m_pRedFlag )
		{
			if ( !m_pRedFlag->IsVisible() )
			{
				m_pRedFlag->SetVisible( true );
			}

			m_pRedFlag->UpdateStatus();
		}
		
		if (m_pGreenFlag)
		{
			if (!m_pGreenFlag->IsVisible())
			{
				m_pGreenFlag->SetVisible(true);
			}

			m_pGreenFlag->UpdateStatus();
		}

		if (m_pYellowFlag)
		{
			if (!m_pYellowFlag->IsVisible())
			{
				m_pYellowFlag->SetVisible(true);
			}

			m_pYellowFlag->UpdateStatus();
		}
		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "flagstatus_update" ) )
	{
		UpdateStatus();
	}
}


CUtlVector<CTFFlagCalloutPanel *> CTFFlagCalloutPanel::sm_FlagCalloutPanels;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel::CTFFlagCalloutPanel( const char *name )
	: CHudElement( name ), BaseClass( NULL, name )
{
	sm_FlagCalloutPanels.AddToTail( this );

	SetParent( g_pClientMode->GetViewport() );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pFlagCalloutPanel = new CTFImagePanel( this, "FlagCalloutPanel" );
	m_pFlagValueLabel = new Label( this, "FlagValueLabel", "" );
	m_pFlagStatusIcon = new CTFImagePanel( this, "StatusIcon" );

	m_f1 = 1.0f;
	m_flLastUpdate = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel::~CTFFlagCalloutPanel( void )
{
	FOR_EACH_VEC_BACK( sm_FlagCalloutPanels, i )
	{
		if ( sm_FlagCalloutPanels[i] == this )
		{
			sm_FlagCalloutPanels.Remove( i );
			break;
		}
	}

	if ( m_pArrowMaterial )
	{
		m_pArrowMaterial->DecrementReferenceCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlagCalloutPanel *CTFFlagCalloutPanel::AddFlagCalloutIfNotFound( CCaptureFlag *pFlag, float f, Vector const &v )
{
	FOR_EACH_VEC( sm_FlagCalloutPanels, i )
	{
		if ( sm_FlagCalloutPanels[i]->m_hFlag == pFlag )
		{
			return nullptr;
		}
	}

	CTFFlagCalloutPanel *pCallout = new CTFFlagCalloutPanel( "FlagCalloutHUD" );
	if ( pCallout )
	{
		pCallout->SetFlag( pFlag, f, v );
	}
	return pCallout;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/FlagCalloutPanel.res" );

	if ( m_pArrowMaterial )
		m_pArrowMaterial->DecrementReferenceCount();

	m_pArrowMaterial = materials->FindMaterial( "HUD/medic_arrow", TEXTURE_GROUP_VGUI );
	m_pArrowMaterial->IncrementReferenceCount();

	if ( !m_pFlagCalloutPanel || !m_pFlagValueLabel || !m_pFlagStatusIcon )
		return;

	m_pFlagCalloutPanel->GetSize( m_nPanelWide, m_nPanelTall );
	m_pFlagValueLabel->GetSize( m_nLabelWide, m_nLabelTall );
	m_pFlagStatusIcon->GetSize( m_nIconWide, m_nIconTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::OnTick( void )
{
	const int nUIMode = tf_rd_flag_ui_mode.GetInt();

	CTFPlayer *pLocalPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !m_hFlag || m_hFlag->IsHome() || m_hFlag->IsDisabled() || nUIMode == 0 )
	{
		MarkForDeletion();
		return;
	}

	bool bShouldDraw = ShouldShowFlagIconToLocalPlayer();
	if ( nUIMode == 1 )
	{
		int nHighestValue = 0;
		CCaptureFlag *pHighestValueFlag = NULL;
		for ( int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag && pFlag->GetPointValue() > nHighestValue )
			{
				if ( pFlag->IsDisabled() || pFlag->IsHome() || pFlag->InSameTeam( pLocalPlayer ) )
					continue;

				if ( nHighestValue < pFlag->GetPointValue() )
				{
					nHighestValue = pFlag->GetPointValue();
					pHighestValueFlag = pFlag;
				}
			}
		}

		if ( pHighestValueFlag != m_hFlag )
			bShouldDraw = false;
	}

	if ( IsVisible() != bShouldDraw )
	{
		if ( !IsVisible() )
		{
			m_flLastUpdate = gpGlobals->curtime;
			m_flPrevScale = 0;
		}

		SetVisible( bShouldDraw );
	}
	if ( IsEnabled() != bShouldDraw )
	{
		SetEnabled( bShouldDraw );
	}

	if ( !bShouldDraw )
		return;

	if ( !m_hFlag->IsDropped() && m_hFlag->GetPrevOwner() && !prediction->IsFirstTimePredicted() )
		return;

	Vector vecToFlag = m_hFlag->GetAbsOrigin() - pLocalPlayer->GetAbsOrigin();
	ScaleAndPositionCallout( RemapValClamped( vecToFlag.LengthSqr(), Sqr( 1000 ), Sqr( 4000 ), 1.0f, 0.6f ) );

	Vector vecTargetPos = m_hFlag->GetAbsOrigin();
	if ( !m_hFlag->IsDropped() && m_hFlag->GetPrevOwner() )
		vecTargetPos = m_hFlag->GetPrevOwner()->GetAbsOrigin();

	Vector vecToTarget = vecTargetPos - MainViewOrigin();
	int nHalfWidth = GetWide() / 2;

	int iX = 0, iY = 0;
	bool bInHudSpace = GetVectorInHudSpace( vecToTarget, iX, iY );

	if ( !bInHudSpace || iX < nHalfWidth || ( ScreenWidth() - nHalfWidth ) < iX )
	{
		if ( TFGameRules() && TFGameRules()->IsInRobotDestructionMode() && ( m_flLastUpdate + 5.0 ) < gpGlobals->curtime )
		{
			m_i1 = 0;
			SetAlpha( 0 );
		}
		else
		{
			vecToTarget.NormalizeInPlace();

			float flRotation;
			GetCalloutPosition( vecToTarget, YRES( 100 ), &iX, &iY, &flRotation );

			Vector vecFlagCenter = m_hFlag->WorldSpaceCenter();
			m_i1 = ( DotProduct( MainViewRight(), vecFlagCenter - MainViewOrigin() ) > 0 ) ? 2 : 1;

			SetPos( iX - nHalfWidth, iY - ( GetTall() / 2 ) );
			SetAlpha( 128 );
		}
	}
	else
	{
		trace_t	tr;
		UTIL_TraceLine( vecToTarget, MainViewOrigin(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.f )
		{
			m_bInLOS = true;
			SetAlpha( 0 );
			return;
		}
		else
		{
			m_i1 = 0;
			SetAlpha( 128 );
			SetPos( iX - nHalfWidth, iY - ( GetTall() / 2 ) );
		}
	}
	
	m_bInLOS = false;

	if ( !m_pFlagCalloutPanel || !m_pFlagValueLabel || !m_pFlagStatusIcon )
		return;

	const char *pszCalloutImage = NULL;
	switch ( m_hFlag->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			pszCalloutImage = "../hud/obj_briefcase_red";
			break;
		case TF_TEAM_BLUE:
			pszCalloutImage = "../hud/obj_briefcase_blue";
			break;
		case TF_TEAM_GREEN:
			pszCalloutImage = "../hud/obj_briefcase_green";
			break;
		case TF_TEAM_YELLOW:
			pszCalloutImage = "../hud/obj_briefcase_yellow";
			break;
		default:
			return;
	}
	m_pFlagCalloutPanel->SetImage( pszCalloutImage );

	m_pFlagValueLabel->SetText( CFmtStr( "%i", m_hFlag->GetPointValue() ) );

	const char *pszIconImage = "../hud/objectives_flagpanel_ico_flag_home";
	if ( m_hFlag->IsDropped() )
	{
		pszIconImage = "../hud/objectives_flagpanel_ico_flag_dropped";
	}
	else if ( m_hFlag->IsStolen() )
	{
		pszIconImage = "../hud/objectives_flagpanel_ico_flag_moving";
	}
	m_pFlagStatusIcon->SetImage( pszIconImage );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::Paint( void )
{
	if ( m_bInLOS )
		return;

	BaseClass::Paint();

	if ( m_i1 == 0 )
		return;

	int iX = 0, iY = 0;
	GetPos( iX, iY );

	float f1, f2;
	if ( m_i1 == 1 )
	{
		f1 = 0;
		f2 = 1.0;
		iX -= XRES( 8 );
	}
	else
	{
		f1 = 1.0;
		f2 = 0;
		iX += m_pFlagCalloutPanel->GetWide();;
	}

	iY += ( ScreenHeight() - YRES( 10 ) + GetTall() ) * 0.5;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pArrowMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( iX, iY, 0.0f );
	meshBuilder.TexCoord2f( 0, f1, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( iX + XRES( 8 ), iY, 0.0f );
	meshBuilder.TexCoord2f( 0, f2, 0.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( iX + XRES( 8 ), iY + YRES( 10 ), 0.0f );
	meshBuilder.TexCoord2f( 0, f2, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( iX, iY + YRES( 10 ), 0.0f );
	meshBuilder.TexCoord2f( 0, f1, 1.0f );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::PaintBackground( void )
{
	CTFPlayer *pLocalPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	if ( !m_hFlag )
	{
		SetAlpha( 0 );
		return;
	}

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::GetCalloutPosition( const Vector &vecToTarget, float flRadius, int *xpos, int *ypos, float *flRotation )
{
	QAngle viewportAngles = MainViewAngles();

	Vector vecForward;
	AngleVectors( viewportAngles, &vecForward );
	vecForward.NormalizeInPlace();
	vecForward.z = 0;

	float flForward = vecToTarget.Dot( vecForward );
	float flSide = (vecToTarget.x * vecForward.y - vecToTarget.y * vecForward.x) * flRadius;
	*flRotation = RAD2DEG( atan2( -flSide * flRadius, -flForward * flRadius ) + M_PI );

	float cos = 0, sin = 0;
	SinCos( DEG2RAD( -(*flRotation) ), &cos, &sin );
	*xpos = (int)( ( (float)ScreenWidth() / 2 ) + ( flRadius * sin ) );
	*ypos = (int)( ( (float)ScreenHeight() / 2 ) - ( flRadius * cos ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::ScaleAndPositionCallout( float flScale )
{
	if ( flScale == m_flPrevScale )
		return;

	SetSize( ( XRES(30) * flScale ), ( YRES(30) * flScale ) );

	if ( !m_pFlagCalloutPanel || !m_pFlagValueLabel || !m_pFlagStatusIcon )
		return;

	m_pFlagCalloutPanel->SetSize( ( m_nPanelWide * flScale ), ( m_nPanelTall * flScale ) );
	m_pFlagCalloutPanel->SetPos( 0, 0 );

	m_pFlagValueLabel->SetSize( ( m_nLabelWide * flScale ), ( m_nLabelTall * flScale ) );
	const float flLabelX = ( m_pFlagCalloutPanel->GetWide() - m_pFlagValueLabel->GetWide() ) * 0.5f;
	const float flLabelY = ( m_pFlagCalloutPanel->GetWide() - m_pFlagValueLabel->GetTall() ) * 0.65f;
	m_pFlagValueLabel->SetPos( flLabelX, flLabelY );

	m_pFlagStatusIcon->SetSize( ( m_nIconWide * flScale ), ( m_nIconTall * flScale ) );
	const float flIconX = ( m_pFlagCalloutPanel->GetWide() - m_pFlagStatusIcon->GetWide() ) * 1.05f;
	const float flIconY = ( m_pFlagCalloutPanel->GetWide() - m_pFlagStatusIcon->GetTall() ) * 0.85f;
	m_pFlagStatusIcon->SetPos( flIconX, flIconY );

	m_flPrevScale = flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlagCalloutPanel::SetFlag( CCaptureFlag *pFlag, float f, Vector const &v )
{
	m_hFlag = pFlag;
	m_f1 = gpGlobals->curtime + f;
	m_v1 = v;
	m_flLastUpdate = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlagCalloutPanel::ShouldShowFlagIconToLocalPlayer( void ) const
{
	CTFPlayer *pLocalPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	const int nUIMode = tf_rd_flag_ui_mode.GetInt();
	if ( m_hFlag->IsStolen() && m_hFlag->InSameTeam( pLocalPlayer ) && nUIMode == 3 )
		return false;

	if ( m_hFlag->InSameTeam( pLocalPlayer ) && nUIMode < 3 )
		return false;

	// We shouldn't be notified of what we're carrying
	if ( m_hFlag->IsStolen() && pLocalPlayer == m_hFlag->GetPrevOwner() )
		return false;

	return true;
}
