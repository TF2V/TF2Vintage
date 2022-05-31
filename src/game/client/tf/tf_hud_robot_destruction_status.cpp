#include "cbase.h"
#include "hudelement.h"
#include "tf_hud_robot_destruction_status.h"
#include "c_func_capture_zone.h"
#include "clientmode_shared.h"
#include "tier1/fmtstr.h"

extern bool IsTakingAFreezecamScreenshot( void );

extern ConVar tf_rd_min_points_to_steal;
extern ConVar tf_rd_steal_rate;
extern ConVar tf_rd_points_per_steal;
extern ConVar tf_rd_points_approach_interval;
extern ConVar tf_rd_points_per_approach;

int SortRobotVec( CTFHudRobotDestruction_RobotIndicator *const *p1, CTFHudRobotDestruction_RobotIndicator *const *p2 )
{
	return (*p2)->GetGroupNumber() - (*p1)->GetGroupNumber();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_StateImage::CTFHudRobotDestruction_StateImage( Panel *parent, const char *name, const char *pszResFile )
	: vgui::EditablePanel( parent, name ), m_pszResFile( pszResFile )
{
	m_pImage = new vgui::ImagePanel( this, "Image" );
	m_pRobotImage = new vgui::ImagePanel( this, "RobotImage" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_StateImage::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( m_pszResFile );

	ImagePanel *pGlow = dynamic_cast<ImagePanel *>( FindChildByName( "GlowImage", true ) );
	if ( pGlow )
		pGlow->SetAlpha( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_StateImage::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int nTeam = TF_TEAM_RED;
	CTFHudRobotDestruction_RobotIndicator *pRobotImageParent = dynamic_cast<CTFHudRobotDestruction_RobotIndicator *>( GetParent()->GetParent() );
	if ( pRobotImageParent )
	{
		nTeam = pRobotImageParent->GetTeamNumber();
	}

	const char *pszKeyName = "redimage";
	switch ( nTeam )
	{
		case TF_TEAM_RED:
			pszKeyName = "redimage";
			break;
		case TF_TEAM_BLUE:
			pszKeyName = "blueimage";
			break;
		case TF_TEAM_GREEN:
			pszKeyName = "greenimage";
			break;
		case TF_TEAM_YELLOW:
			pszKeyName = "yellowimage";
			break;
	}

	const char *pszImageName = inResourceData->GetString( pszKeyName );
	if ( pszImageName && pszImageName[0] )
	{
		m_pImage->SetImage( pszImageName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_DeadImage::CTFHudRobotDestruction_DeadImage( Panel *parent, const char *name, const char *pszResFile )
	: CTFHudRobotDestruction_StateImage( parent, name, pszResFile )
{
	m_pRespawnProgressBar = new CTFProgressBar( this, "RespawnProgressBar" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_DeadImage::SetProgress( float flProgress )
{
	m_pRespawnProgressBar->SetPercentage( flProgress );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_ActiveImage::CTFHudRobotDestruction_ActiveImage( Panel *parent, const char *name, const char *pszResFile )
	: CTFHudRobotDestruction_StateImage( parent, name, pszResFile )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_ActiveImage::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	CTFHudRobotDestruction_RobotIndicator *pRobotImageParent = dynamic_cast<CTFHudRobotDestruction_RobotIndicator *>( GetParent()->GetParent() );
	if ( pRobotImageParent && pRobotImageParent->GetGroup() )
	{
		m_pRobotImage->SetImage( pRobotImageParent->GetGroup()->m_pszHudIcon );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_RobotIndicator::CTFHudRobotDestruction_RobotIndicator( vgui::Panel *parent, const char *name, CTFRobotDestruction_RobotGroup *pGroup )
	: vgui::EditablePanel( parent, name ), m_hGroup( pGroup )
{
	m_pSwoop = new CControlPointIconSwoop( this, "Swoop" );
	m_pSwoop->SetVisible( false );
	m_pRobotStateContainer = new EditablePanel( this, "RobotStateContainer" );
	m_pDeadPanel = new CTFHudRobotDestruction_DeadImage( m_pRobotStateContainer, "DeadState", "resource/UI/TFHudRobotDestruction_DeadState.res" );
	m_pActivePanel = new CTFHudRobotDestruction_ActiveImage( m_pRobotStateContainer, "ActiveState", "resource/UI/TFHudRobotDestruction_ActiveState.res" );
	m_pShieldedPanel = new CTFHudRobotDestruction_StateImage( m_pRobotStateContainer, "ShieldedState", "resource/UI/TFHudRobotDestruction_ShieldedState.res" );

	ivgui()->AddTickSignal( GetVPanel(), 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/TFHudRobotDestruction_RobotIndicator.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::OnTick()
{
	if ( !m_hGroup )
	{
		SetVisible( false );
		ivgui()->RemoveTickSignal( GetVPanel() );
	}

	UpdateState();
	DoUnderAttackBlink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pSwoop->SetBounds( 0, 0, GetWide(), GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::DoUnderAttackBlink()
{
	if ( !m_hGroup )
		return;

	if ( gpGlobals->curtime < ( m_hGroup->m_flLastAttackedTime + 2.0f ) && ( GetLocalPlayerTeam() == m_hGroup->GetTeamNumber() ) )
	{
		ImagePanel *pGlow = dynamic_cast<ImagePanel *>( FindChildByName( "GlowImage", true ) );
		if ( pGlow )
		{
			float flAlpha = fabs( sin( gpGlobals->curtime * 10.f ) ) * 255;
			pGlow->SetAlpha( flAlpha );
		}
	}
	else
	{
		ImagePanel *pGlow = dynamic_cast<ImagePanel *>( FindChildByName( "GlowImage", true ) );
		if ( pGlow )
		{
			pGlow->SetAlpha( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFHudRobotDestruction_RobotIndicator::GetGroupNumber() const
{
	if ( !m_hGroup )
		return 0;
	
	return m_hGroup->m_nGroupNumber;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFHudRobotDestruction_RobotIndicator::GetTeamNumber() const
{
	if ( !m_hGroup )
		return 0;

	return m_hGroup->GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::UpdateState()
{
	if ( !m_hGroup )
		return;

	ERobotState eState = (ERobotState)m_hGroup->m_nState.Get();
	float flStartTime = m_hGroup->m_flRespawnStartTime;
	float flEndTime = m_hGroup->m_flRespawnEndTime;

	m_pDeadPanel->SetDialogVariable( "time", CFmtStr( "%0.0f", Max( 0.f, flEndTime - gpGlobals->curtime ) ) );
	m_pDeadPanel->SetProgress( ( flEndTime - flStartTime ) / ( gpGlobals->curtime - flStartTime ) );

	if ( eState != ROBOT_STATE_INACTIVE )
	{
		m_pRobotStateContainer->SetVisible( true );
	}
	else
	{
		m_pRobotStateContainer->SetVisible( false );
	}

	m_eState = eState;

	m_pActivePanel->SetImageVisible( eState != ROBOT_STATE_DEAD );
	m_pDeadPanel->SetVisible( eState == ROBOT_STATE_DEAD );
	m_pActivePanel->SetVisible( eState == ROBOT_STATE_ACTIVE );
	m_pShieldedPanel->SetVisible( eState == ROBOT_STATE_SHIELDED );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHUDRobotDestruction::CTFHUDRobotDestruction( vgui::Panel *parent, const char *name )
	: vgui::EditablePanel( parent, name ), m_pPlayingTo( NULL ), m_pRobotIndicatorSettings( NULL ), m_pTeamLeaderImage( NULL ), m_pCountdownContainer( NULL )
{
	m_pCarriedContainer = new EditablePanel( this, "CarriedContainer" );
	m_pCarriedImage = new ImagePanel( m_pCarriedContainer, "CarriedImage" );
	m_pCarriedFlagProgressBar = new CProgressPanel( m_pCarriedContainer, "CarriedProgressBar" );

	m_pScoreContainer = new EditablePanel( this, "ScoreContainer" );
	m_pBlueScoreValueContainer = new EditablePanel( m_pScoreContainer, "BlueScoreValueContainer" );
	m_pRedScoreValueContainer = new EditablePanel( m_pScoreContainer, "RedScoreValueContainer" );

	m_pBlueStolenContainer = new EditablePanel( m_pScoreContainer, "BlueStolenContainer" );
	m_pBlueDroppedPanel = new EditablePanel( m_pBlueStolenContainer, "DroppedIntelContainer" );
	m_pRedStolenContainer = new EditablePanel( m_pScoreContainer, "RedStolenContainer" );
	m_pRedDroppedPanel = new EditablePanel( m_pRedStolenContainer, "DroppedIntelContainer" );

	m_pProgressBarsContainer = new EditablePanel( m_pScoreContainer, "ProgressBarContainer" );

	m_pBlueVictoryPanel = new EditablePanel( m_pProgressBarsContainer, "BlueVictoryContainer" );
	m_pBlueProgressBar = new CProgressPanel( m_pProgressBarsContainer, "BlueProgressBarFill" );
	m_pBlueProgressBarEscrow = new CProgressPanel( m_pProgressBarsContainer, "BlueProgressBarEscrow" );

	m_pRedVictoryPanel = new EditablePanel( m_pProgressBarsContainer, "RedVictoryContainer" );
	m_pRedProgressBar = new CProgressPanel( m_pProgressBarsContainer, "RedProgressBarFill" );
	m_pRedProgressBarEscrow = new CProgressPanel( m_pProgressBarsContainer, "RedProgressBarEscrow" );

	ivgui()->AddTickSignal( GetVPanel(), 50 );

	m_hBlueFlag = m_hRedFlag = NULL;
	m_bInRobotDestruction = false;

	ListenForGameEvent( "rd_rules_state_changed" );
	ListenForGameEvent( "flagstatus_update" );
	ListenForGameEvent( "rd_team_points_changed" );
	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHUDRobotDestruction::~CTFHUDRobotDestruction()
{
	if ( m_pRobotIndicatorSettings )
	{
		m_pRobotIndicatorSettings->deleteThis();
		m_pRobotIndicatorSettings = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	C_TFRobotDestructionLogic *pLogic = C_TFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( !pLogic )
		return;

	LoadControlSettings( pLogic->GetResFile() );

	m_vecRedRobots.PurgeAndDeleteElements();
	m_vecBlueRobots.PurgeAndDeleteElements();

	FOR_EACH_VEC( IRobotDestructionGroupAutoList::AutoList(), i )
	{
		C_TFRobotDestruction_RobotGroup *pGroup = static_cast<C_TFRobotDestruction_RobotGroup *>( IRobotDestructionGroupAutoList::AutoList()[i] );
		if ( pGroup->IsDormant() )
			continue;

		switch ( pGroup->GetTeamNumber() )
		{
			case TF_TEAM_RED:
				m_vecRedRobots[ m_vecRedRobots.AddToTail() ] = 
					new CTFHudRobotDestruction_RobotIndicator( this, CFmtStr( "red_group_%d", m_vecRedRobots.Count() ), pGroup );
				break;
			case TF_TEAM_BLUE:
				m_vecBlueRobots[ m_vecBlueRobots.AddToTail() ] =
					new CTFHudRobotDestruction_RobotIndicator( this, CFmtStr( "blue_group_%d", m_vecBlueRobots.Count() ), pGroup );
				break;
		}
	}

	m_vecRedRobots.Sort( &SortRobotVec );
	m_vecBlueRobots.Sort( &SortRobotVec );

	m_pCarriedContainer->SetVisible( false );

	m_pCountdownContainer = dynamic_cast<EditablePanel *>( FindChildByName( "CountdownContainer" ) );
	m_pTeamLeaderImage = dynamic_cast<CTFImagePanel *>( m_pCarriedContainer->FindChildByName( "TeamLeaderImage" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::ApplySettings( KeyValues *inResourceData )
{
	KeyValues *pRobotKV = inResourceData->FindKey( "robot_kv" );
	if ( pRobotKV )
	{
		if ( m_pRobotIndicatorSettings )
		{
			m_pRobotIndicatorSettings->deleteThis();
		}
		m_pRobotIndicatorSettings = new KeyValues( "robot_kv" );
		pRobotKV->CopySubkeys( m_pRobotIndicatorSettings );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHUDRobotDestruction::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::FireGameEvent( IGameEvent *pEvent )
{
	C_TFRobotDestructionLogic *pLogic = C_TFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( !pLogic )
		return;

	const char *pszEventName = pEvent->GetName();
	if ( FStrEq( "rd_rules_state_changed", pszEventName ) )
	{
		UpdateRobotElements();
	}
	else if ( FStrEq( pszEventName, "flagstatus_update" ) )
	{
		C_BasePlayer *pNewOwner = ToBasePlayer( ClientEntityList().GetEnt( engine->GetPlayerForUserID( pEvent->GetInt( "userid" ) ) ) );

		C_CaptureFlag *pFlag = dynamic_cast<C_CaptureFlag *>( ClientEntityList().GetEnt( pEvent->GetInt( "entindex" ) ) );
		if ( pFlag )
		{
			UpdateCarriedFlagStatus( pNewOwner, pFlag );
			UpdateStolenFlagStatus( pFlag->GetTeamNumber(), pFlag );
		}
	}
	else if ( FStrEq( pszEventName, "rd_team_points_changed" ) )
	{
		int nTeam = pEvent->GetInt( "team" );
		int nPoints = pEvent->GetInt( "points" );
		ERDScoreMethod eMethod = (ERDScoreMethod)pEvent->GetInt( "method" );

		Panel *pPanel = nTeam == TF_TEAM_RED ? m_pRedScoreValueContainer : m_pBlueScoreValueContainer;
		bool bPositive = ( nTeam == GetLocalPlayerTeam() && nPoints > 0 ) || ( nTeam != GetLocalPlayerTeam() && nPoints < 0 );
		const char *pszAnimName = bPositive ? "RDPositiveScorePulse" : "RDNegativeScorePulse";

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pPanel, pszAnimName );

		CProgressPanel *pProgressBar = nTeam == TF_TEAM_RED ? m_pRedProgressBar : m_pBlueProgressBar;
		pProgressBar->m_flLastScoreUpdate = gpGlobals->curtime;

		if ( eMethod == SCORE_REACTOR_STEAL )
		{
			CProgressPanel *pEscrowBar = nTeam != TF_TEAM_RED ? m_pRedProgressBarEscrow : m_pBlueProgressBarEscrow;
			pEscrowBar->m_flLastScoreUpdate = gpGlobals->curtime;
		}
	}
	else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
	{
		float flApproachSpeed = ( tf_rd_points_per_approach.GetInt() / tf_rd_points_approach_interval.GetFloat() ) / pLogic->GetMaxPoints();
		m_pBlueProgressBar->m_flApproachSpeed = flApproachSpeed;
		m_pBlueProgressBarEscrow->m_flApproachSpeed = flApproachSpeed;
		m_pRedProgressBar->m_flApproachSpeed = flApproachSpeed;
		m_pRedProgressBarEscrow->m_flApproachSpeed = flApproachSpeed;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// TODO: Four Team
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::OnTick()
{
	C_TFRobotDestructionLogic *pLogic = C_TFRobotDestructionLogic::GetRobotDestructionLogic();
	bool bOldInRobotDestruction = m_bInRobotDestruction;
	m_bInRobotDestruction = ( pLogic != NULL );
	if ( bOldInRobotDestruction != m_bInRobotDestruction )
	{
		if ( m_bInRobotDestruction )
			InvalidateLayout( true, true );
	}

	if ( !pLogic )
		return;

	m_pRedScoreValueContainer->SetDialogVariable( "score", pLogic->GetScore( TF_TEAM_RED ) );
	m_pBlueScoreValueContainer->SetDialogVariable( "score", pLogic->GetScore( TF_TEAM_BLUE ) );

	int nBlueEscrow = 0, nRedEscrow = 0;

	if ( pLogic->GetType() == C_TFRobotDestructionLogic::TYPE_PLAYER_DESTRUCTION )
	{
		// TODO
	}
	else
	{
		if ( !m_hRedFlag || !m_hBlueFlag )
		{
			FOR_EACH_VEC( ICaptureFlagAutoList::AutoList(), i )
			{
				C_CaptureFlag *pFlag = static_cast<C_CaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
				switch ( pFlag->GetTeamNumber() )
				{
					case TF_TEAM_RED:
						m_hRedFlag = pFlag;
						break;
					case TF_TEAM_BLUE:
						m_hBlueFlag = pFlag;
						break;
				}
			}
		}

		if ( m_hRedFlag && m_hBlueFlag )
		{
			if ( m_hRedFlag->IsDropped() )
			{
				nRedEscrow += m_hRedFlag->GetPointValue();
				if ( m_hRedFlag->GetReturnProgress() > 0.8f )
					m_pRedDroppedPanel->SetAlpha( (int)gpGlobals->curtime * 10 % 10 < 5 ? 255 : 0 );
			}
			else
			{
				nBlueEscrow += m_hRedFlag->GetPointValue();
				m_pRedDroppedPanel->SetAlpha( 255 );
			}

			if ( m_hBlueFlag->IsDropped() )
			{
				nBlueEscrow += m_hBlueFlag->GetPointValue();
				if ( m_hBlueFlag->GetReturnProgress() > 0.8f )
					m_pBlueDroppedPanel->SetAlpha( (int)gpGlobals->curtime * 10 % 10 < 5 ? 255 : 0 );
			}
			else
			{
				nRedEscrow += m_hBlueFlag->GetPointValue();
				m_pBlueDroppedPanel->SetAlpha( 255 );
			}
		}

		const float flFinaleLength = pLogic->GetFinaleLength();
		const float flMaxPoints = pLogic->GetMaxPoints();

		const float flRedFinaleProgress = clamp( pLogic->GetFinaleWinTime( TF_TEAM_RED ) - gpGlobals->curtime, 0, flFinaleLength );
		m_pRedVictoryPanel->SetVisible( flRedFinaleProgress < flFinaleLength );
		m_pRedScoreValueContainer->SetVisible( flRedFinaleProgress >= flFinaleLength );
		if ( flRedFinaleProgress < flFinaleLength )
		{
			m_pRedVictoryPanel->SetDialogVariable( "victorytime", (int)flRedFinaleProgress );
		}

		const float flBlueFinaleProgress = clamp( pLogic->GetFinaleWinTime( TF_TEAM_BLUE ) - gpGlobals->curtime, 0, flFinaleLength );
		m_pBlueVictoryPanel->SetVisible( flBlueFinaleProgress < flFinaleLength );
		m_pBlueScoreValueContainer->SetVisible( flBlueFinaleProgress >= flFinaleLength );
		if ( flBlueFinaleProgress < flFinaleLength )
		{
			m_pBlueVictoryPanel->SetDialogVariable( "victorytime", (int)flBlueFinaleProgress );
		}

		const int nBluePoints = pLogic->GetTargetScore( TF_TEAM_BLUE );
		m_pBlueProgressBar->SetProgress( nBluePoints / pLogic->GetMaxPoints() );
		m_pBlueProgressBarEscrow->SetProgress( ( nBluePoints + nBlueEscrow ) / flMaxPoints );

		const int nRedPoints = pLogic->GetTargetScore( TF_TEAM_RED );
		m_pRedProgressBar->SetProgress( nRedPoints / pLogic->GetMaxPoints() );
		m_pRedProgressBarEscrow->SetProgress( ( nRedPoints + nRedEscrow ) / flMaxPoints );
	}

	SetPlayingToLabelVisible( true );

	SetDialogVariable( "rounds", pLogic->GetMaxPoints() );

	UpdateCarriedFlagStatus( NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::Paint()
{
	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PaintBackground()
{
	UpdateStolenPoints( TF_TEAM_RED, m_pRedStolenContainer );
	UpdateStolenPoints( TF_TEAM_BLUE, m_pBlueStolenContainer );

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PerformLayout()
{
	BaseClass::PerformLayout();

	m_vecRedRobots.Sort( &SortRobotVec );
	m_vecBlueRobots.Sort( &SortRobotVec );

	PerformRobotLayout( m_vecRedRobots, TF_TEAM_RED );
	PerformRobotLayout( m_vecBlueRobots, TF_TEAM_BLUE );

	int iX, iY;
	m_pProgressBarsContainer->GetPos( iX, iY );
	m_nStealLeftEdge = iX + m_nStealLeftEdgeOffset - ( m_pRedStolenContainer->GetWide() / 2 );
	m_nStealRightEdge = iY + m_pProgressBarsContainer->GetWide() - m_nStealRightEdgeOffset + ( m_pRedStolenContainer->GetWide() / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PaintPDPlayerScore( C_TFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
		return;

	Vector vecPos = pPlayer->GetAbsOrigin();
	vecPos.z += VEC_HULL_MAX_SCALED( pPlayer ).z + 20;

	int iX, iY;
	Vector vecWorld( vecPos.x, vecPos.y, vecPos.z );
	if ( GetVectorInHudSpace( vecWorld, iX, iY ) )
	{
		int iCurrentLeadingPoint = 0;
		if ( pPlayer->HasItem() )
		{
			C_CaptureFlag *pFlag = dynamic_cast<C_CaptureFlag *>( pPlayer->GetItem() );
			if ( pFlag )
			{
				iCurrentLeadingPoint = pFlag->GetPointValue();
			}
		}

		wchar_t wszScore[3];
		V_snwprintf( wszScore, ARRAYSIZE( wszScore ), L"%d", iCurrentLeadingPoint );
		const int nWidth = V_wcslen( wszScore ) * 15;

		// draw the name
		vgui::surface()->DrawSetTextFont( m_hPDPlayerScoreFont );
		vgui::surface()->DrawSetTextPos( iX - ( nWidth / 2 ), iY );
		vgui::surface()->DrawSetTextColor( m_TextColor );


		vgui::surface()->DrawPrintText( wszScore, wcslen( wszScore ), vgui::FONT_DRAW_NONADDITIVE );

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// TODO: Four Team
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PerformRobotLayout( CUtlVector<CTFHudRobotDestruction_RobotIndicator *> const &vecRobots, int nTeam )
{
	int iX = 0, iY = 0, nWide = 0, nTall = 0;
	GetBounds( iX, iY, nWide, nTall );

	int iActiveIndex = 0;
	vgui::Panel *pPrevPanel = NULL;
	FOR_EACH_VEC_BACK( vecRobots, i )
	{
		CTFHudRobotDestruction_RobotIndicator *pRobot = vecRobots[i];
		if ( pRobot )
		{
			pRobot->ApplySettings( m_pRobotIndicatorSettings );
			pRobot->UpdateState();
			pRobot->SetZPos( vecRobots.Count() - i );

			CTFHudRobotDestruction_RobotIndicator *pPrevRobot = dynamic_cast<CTFHudRobotDestruction_RobotIndicator *>( pPrevPanel );
			if ( pPrevRobot )
			{
				pRobot->m_hRobotIndicator1 = pPrevRobot;
				pPrevRobot->m_hRobotIndicator2 = pRobot;
			}

			// Yeesh, refactor later
			const int nXPos = ( iX + nWide * 0.5f ) + ( ( m_iRobotXOffset + pRobot->GetWide() * 0.5f ) * ( nTeam == TF_TEAM_RED ? 1 : -1 ) ) + ( ( nTeam == TF_TEAM_RED ? m_iRobotXStep : -m_iRobotXStep ) * iActiveIndex ) - ( pRobot->GetWide() * 0.5 );
			pRobot->SetPos( nXPos, ( nTall - m_iRobotYOffset ) - pRobot->GetTall() - ( m_iRobotYStep * i ) );

			pRobot->InvalidateLayout( true, true );
			if ( pRobot->m_eState != ROBOT_STATE_INACTIVE )
			{
				pPrevPanel = pRobot;
				++iActiveIndex;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::SetPlayingToLabelVisible( bool bVisible )
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
void CTFHUDRobotDestruction::UpdateCarriedFlagStatus( C_BasePlayer *pNewOwner, C_CaptureFlag *pFlag )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pNewOwner && pNewOwner->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
		return;

	C_CaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		if ( !pNewOwner || pNewOwner == pLocalPlayer )
		{
			pPlayerFlag = dynamic_cast<C_CaptureFlag *>( pLocalPlayer->GetItem() );
		}
	}

	if ( !pPlayerFlag && pLocalPlayer && pLocalPlayer == pNewOwner )
	{
		pPlayerFlag = pFlag;
	}

	if ( pPlayerFlag && !pPlayerFlag->IsMarkedForDeletion() && !pPlayerFlag->IsDormant() )
	{
		m_pCarriedContainer->SetVisible( true );
		m_pCarriedContainer->SetDialogVariable( "flagvalue", pPlayerFlag->GetPointValue() );
		
		if ( m_pCarriedImage && !m_pCarriedImage->IsVisible() )
		{
			int nTeam;
			if ( pPlayerFlag->GetGameType() == TF_FLAGTYPE_ATTACK_DEFEND ||
				 pPlayerFlag->GetGameType() == TF_FLAGTYPE_TERRITORY_CONTROL ||
				 pPlayerFlag->GetGameType() == TF_FLAGTYPE_INVADE ||
				 pPlayerFlag->GetGameType() == TF_FLAGTYPE_RESOURCE_CONTROL )
			{
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}
			else
			{
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_RED ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}

			m_pCarriedImage->SetVisible( true );

			m_pCarriedFlagProgressBar->m_flEndProgress = 0;
			m_pCarriedFlagProgressBar->m_flCurrentProgress = 0;
			m_pCarriedFlagProgressBar->CalculateSize();
			m_pCarriedFlagProgressBar->m_StandardColor = pLocalPlayer->GetTeamNumber() == TF_TEAM_RED ? m_ColorRed : m_ColorBlue;
		}

		C_TFRobotDestructionLogic *pLogic = C_TFRobotDestructionLogic::GetRobotDestructionLogic();
		if ( pLogic )
		{
			int nMinToSteal = tf_rd_min_points_to_steal.GetInt();
			float flProgress = float( pPlayerFlag->GetPointValue() ) / float( pLogic->GetMaxPoints() );
			const float flProgressAtDottedLine = float( nMinToSteal ) / float( pLogic->GetMaxPoints() );
			const float flWhereTheDottedLineIs = 0.25f;

			if ( flProgress <= flProgressAtDottedLine )
			{
				flProgress = RemapValClamped( flProgress, 0.f, flProgressAtDottedLine, 0.f, flWhereTheDottedLineIs );
			}
			else
			{
				flProgress = RemapValClamped( flProgress, flProgressAtDottedLine, 1.f, flWhereTheDottedLineIs, 1.f );
			}
			m_pCarriedFlagProgressBar->SetProgress( flProgress );
		}
	}
	else if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
	{
		m_pCarriedContainer->SetVisible( false );
		m_pCarriedImage->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateRobotElements()
{
	m_vecRedRobots.PurgeAndDeleteElements();
	m_vecBlueRobots.PurgeAndDeleteElements();

	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateStolenFlagStatus( int nTeam, C_CaptureFlag *pFlag )
{
	vgui::Panel *pCarriedImage = NULL;
	vgui::Panel *pDownImage = NULL;
	switch ( nTeam )
	{
		case TF_TEAM_RED:
			pCarriedImage = m_pRedStolenContainer->FindChildByName( "IntelImage" );
			pDownImage = m_pRedStolenContainer->FindChildByName( "DroppedIntelContainer" );
			break;

		case TF_TEAM_BLUE:
			pCarriedImage = m_pBlueStolenContainer->FindChildByName( "IntelImage" );
			pDownImage = m_pBlueStolenContainer->FindChildByName( "DroppedIntelContainer" );
			break;
	}
	if ( !pCarriedImage || !pDownImage )
		return;

	bool bIsDropped = pFlag->IsDropped();
	pCarriedImage->SetVisible( !bIsDropped );
	pDownImage->SetVisible( bIsDropped );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateStolenPoints( int nTeam, vgui::EditablePanel *pContainer )
{
	C_TFRobotDestructionLogic *pRoboLogic = C_TFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( pRoboLogic )
	{
		int nStolenPoints = 0;
		C_CaptureFlag *pTheirFlag = nTeam == TF_TEAM_RED ? m_hRedFlag : m_hBlueFlag;
		if ( pTheirFlag )
		{
			nStolenPoints = pTheirFlag->GetPointValue();
		}
		
		pContainer->SetVisible( nStolenPoints > 0 );
		pContainer->SetDialogVariable( "intelvalue", nStolenPoints );
	}

	C_CaptureFlag *pStolenFlag = nTeam == TF_TEAM_RED ? m_hRedFlag : m_hBlueFlag;
	if ( pStolenFlag && pStolenFlag->IsHome() )
	{
		pStolenFlag = NULL;
	}

	C_CaptureZone *pStartCaptureZone = NULL, *pEndCaptureZone = NULL;
	for ( int i = 0; i < ICaptureZoneAutoList::AutoList().Count(); i++ )
	{
		C_CaptureZone *pCaptureZone = static_cast<C_CaptureZone *>( ICaptureZoneAutoList::AutoList()[i] );
		if ( !pCaptureZone->IsDormant() && !pCaptureZone->IsDisabled() )
		{
			if ( pCaptureZone->GetTeamNumber() == nTeam )
			{
				pStartCaptureZone = pCaptureZone;
			}
			else
			{
				pEndCaptureZone = pCaptureZone;
			}
		}
	}

	if ( pStolenFlag && pStartCaptureZone && pEndCaptureZone )
	{
		Vector vecFlagPos = pStolenFlag->GetMoveParent() ? pStolenFlag->GetMoveParent()->GetAbsOrigin() : pStolenFlag->GetAbsOrigin();
		const float flTotalDist = ( pEndCaptureZone->WorldSpaceCenter() - pStartCaptureZone->WorldSpaceCenter() ).Length() - pEndCaptureZone->BoundingRadius() - pStartCaptureZone->BoundingRadius();
		const float flFlagDist = ( pEndCaptureZone->WorldSpaceCenter() - vecFlagPos ).Length() - pEndCaptureZone->BoundingRadius();
		const float flLerp = clamp( flFlagDist / flTotalDist, 0.f, 1.f );
		const float flProgress = nTeam == TF_TEAM_BLUE ? ( 1.f - flLerp ) : flLerp;

		int nWide = pContainer->GetWide();
		const int nXpos = ( ( m_nStealRightEdge - ( m_nStealLeftEdge + nWide ) ) * flProgress ) + m_nStealLeftEdge;

		int nDummy, nYpos;
		pContainer->GetPos( nDummy, nYpos );
		pContainer->SetPos( nXpos, nYpos );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHUDRobotDestruction::CProgressPanel::CProgressPanel( vgui::Panel *parent, const char *name )
	: ImagePanel( parent, name )
{
	m_nX = m_nY = m_nWide = m_nTall = 0;
	m_flCurrentProgress = m_flEndProgress = 0;
	m_flLastScoreUpdate = m_flLastTick = -1.0f;

	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::CProgressPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int nXpos, nYpos;
	GetPos( nXpos, nYpos );
	SetPos( nXpos + m_nLeftOffset, nYpos );

	GetBounds( m_nX, m_nY, m_nWide, m_nTall );
	if ( GetImage() )
	{
		GetImage()->SetSize( m_nWide, m_nTall );
	}

	CalculateSize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::CProgressPanel::FireGameEvent( IGameEvent *pEvent )
{
	if ( FStrEq( pEvent->GetName(), "teamplay_round_start" ) )
	{
		m_flCurrentProgress = 0.f;
		m_flEndProgress = 0.f;

		CalculateSize();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::CProgressPanel::OnTick()
{
	const float flDelta = gpGlobals->curtime - m_flLastTick;

	m_flLastTick = gpGlobals->curtime;

	m_flCurrentProgress = Approach( m_flEndProgress, m_flCurrentProgress, flDelta * m_flApproachSpeed );

	if ( m_flCurrentProgress == m_flEndProgress )
	{
		// Stop ticking now
		ivgui()->RemoveTickSignal( GetVPanel() );
	}

	CalculateSize();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::CProgressPanel::PaintBackground()
{
	vgui::IImage *pImage = GetImage();
	if ( pImage )
	{
		pImage->SetPos( m_bLeftToRight ? -m_nLeftOffset : -m_flXPos, m_nY );
		pImage->SetSize( m_nWide, m_nTall );
	}

	// This is a doozy
	float flLerp = gpGlobals->curtime > ( m_flLastScoreUpdate + 0.25f ) ? 
		( m_flCurrentProgress >= m_flBlinkThreshold ) ? 
		  ( sin( gpGlobals->curtime * m_flBlinkRate ) * 0.5f ) + 0.5f 
		  : 
		  1.0f
		: 
		( gpGlobals->curtime - m_flLastScoreUpdate ) / 0.25f;
	flLerp = Clamp( flLerp, 0.0f, 1.0f );
	
	Color drawColor( ( 1.0 - flLerp ) * m_BrightColor.r() + flLerp * m_StandardColor.r(),
					 ( 1.0 - flLerp ) * m_BrightColor.g() + flLerp * m_StandardColor.g(),
					 ( 1.0 - flLerp ) * m_BrightColor.b() + flLerp * m_StandardColor.b(),
					 255 );
	SetDrawColor( drawColor );

	BaseClass::PaintBackground();
}
