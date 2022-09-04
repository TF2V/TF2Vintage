//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include "tf_hud_pve_winpanel.h"
#include "tf_hud_statpanel.h"
#include "tf_controls.h"
#include "iclientmode.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "tf_mann_vs_machine_stats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPVEWinPanel::CTFPVEWinPanel( IViewPort *pViewPort ) 
	: EditablePanel( NULL, "PVEWinPanel" )
{
	SetScheme( "ClientScheme" );

	ListenForGameEvent( "pve_win_panel" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );

	m_bShouldBeVisible = false;
	m_pRespecContainerPanel = NULL;
	m_pRespecBackground = NULL;
	m_pRespecCountLabel = NULL;
	m_pRespecTextLabel = NULL;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudPVEWinPanel.res" );

	m_pRespecBackground = dynamic_cast<vgui::ScalableImagePanel *>( FindChildByName( "RespecBackground" ) );
	m_pRespecContainerPanel = dynamic_cast<vgui::EditablePanel *>( FindChildByName( "RespecContainer" ) );
	if ( m_pRespecContainerPanel )
	{
		m_pRespecTextLabel = dynamic_cast<vgui::Label *>( m_pRespecContainerPanel->FindChildByName( "RespecTextLabelLoss" ) );
		m_pRespecCountLabel = dynamic_cast<vgui::Label *>( m_pRespecContainerPanel->FindChildByName( "RespecCountLabel" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();

	if ( !V_strcmp( "teamplay_round_start", pszEventName ) || !V_strcmp( "teamplay_game_over", pszEventName ) ||
		 !V_strcmp( "tf_game_over", pszEventName ) || !V_strcmp( "training_complete", pszEventName ) )
	{
		m_bShouldBeVisible = false;
	}
	else if ( !V_strcmp( "pve_win_panel", pszEventName ) )
	{
		if ( !g_PR )
			return;

		int iWinningTeam = event->GetInt( "winning_team" );
		if ( iWinningTeam == TF_TEAM_MVM_BOTS )
		{
			LoadControlSettings( "resource/UI/HudPVEWinPanel.res" );

			InvalidateLayout( false, true );

			SetDialogVariable( "WinningTeamLabel", L"" );

			wchar_t *pwchWinReason = g_pVGuiLocalize->Find( "#Winpanel_PVE_Bomb_Deployed" );
			SetDialogVariable( "WinReasonLabel", pwchWinReason );

			wchar_t *pwchDetails = g_pVGuiLocalize->Find( "#TF_PVE_RestoreToCheckpointDetailed" );
			SetDialogVariable( "DetailsLabel", pwchDetails );

			MoveToFront();

			m_bShouldBeVisible = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::OnTick( void )
{
	if ( m_bShouldBeVisible )
	{
		IViewPortPanel *pScoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
		if ( ( pScoreboard && pScoreboard->IsVisible() ) || IsInFreezeCam() )
		{
			SetVisible( false );
			return;
		}

		CTFStatPanel *pStatPanel = GetStatPanel();
		if ( pStatPanel && pStatPanel->IsVisible() )
		{
			pStatPanel->Hide();
		}

		if ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
		{
			m_bShouldBeVisible = false;
		}

		if ( m_pRespecContainerPanel && m_pRespecBackground && m_pRespecCountLabel && m_pRespecTextLabel )
		{
			CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats == nullptr )
				return;

			uint16 nRespecs = pStats->GetNumRespecsEarnedInWave();
			bool bHasRespecs = nRespecs > 0;

			if ( bHasRespecs && !m_pRespecBackground->IsVisible() )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "RespecEarnedPulseLoss" );

				C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalTFPlayer )
				{
					pLocalTFPlayer->EmitSound( "MVM.RespecAwarded" );
				}
			}

			if ( m_pRespecContainerPanel->IsVisible() != bHasRespecs )
			{
				m_pRespecContainerPanel->SetVisible( bHasRespecs );
			}

			if ( m_pRespecBackground->IsVisible() != bHasRespecs )
			{
				m_pRespecBackground->SetVisible( bHasRespecs );
			}

			if ( m_pRespecCountLabel->IsVisible() != bHasRespecs )
			{
				m_pRespecCountLabel->SetVisible( bHasRespecs );
			}

			if ( m_pRespecTextLabel->IsVisible() != bHasRespecs )
			{
				m_pRespecTextLabel->SetVisible( bHasRespecs );
			}

			if ( bHasRespecs )
			{
				m_pRespecContainerPanel->SetDialogVariable( "respeccount", nRespecs );
			}
		}
	}

	SetVisible( m_bShouldBeVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::Reset( void )
{
	Update();

	m_bShouldBeVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPVEWinPanel::Update( void )
{
}