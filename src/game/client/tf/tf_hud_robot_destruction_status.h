//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_HUD_ROBOT_DESTRUCTION_STATUS_H
#define TF_HUD_ROBOT_DESTRUCTION_STATUS_H

#include "tf_controls.h"
#include "tf_logic_robot_destruction.h"
#include "tf_hud_objectivestatus.h"

class CTFHudRobotDestruction_StateImage : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_StateImage, vgui::EditablePanel );
public:
	CTFHudRobotDestruction_StateImage( vgui::Panel *parent, const char *name, const char *pszResFile );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *inResourceData );

	void			SetImageVisible( bool bVisible )	{ m_pImage->SetVisible( bVisible ); }

protected:
	vgui::ImagePanel *m_pImage;
	vgui::ImagePanel *m_pRobotImage;
	const char *m_pszResFile;
};

class CTFHudRobotDestruction_DeadImage : public CTFHudRobotDestruction_StateImage
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_DeadImage, CTFHudRobotDestruction_StateImage );
public:
	CTFHudRobotDestruction_DeadImage( vgui::Panel *parent, const char *name, const char *pszResFile );

	void SetProgress( float flProgress );

private:
	CTFProgressBar *m_pRespawnProgressBar;
};

class CTFHudRobotDestruction_ActiveImage : public CTFHudRobotDestruction_StateImage
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_ActiveImage, CTFHudRobotDestruction_StateImage );
public:
	CTFHudRobotDestruction_ActiveImage( vgui::Panel *parent, const char *name, const char *pszResFile );

	virtual void ApplySettings( KeyValues *inResourceData );
};


class CTFHudRobotDestruction_RobotIndicator : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_RobotIndicator, vgui::EditablePanel );
public:
	CTFHudRobotDestruction_RobotIndicator( vgui::Panel *pParent, const char *pszName, CTFRobotDestruction_RobotGroup *pGroup );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	OnTick();
	virtual void	PerformLayout();

	void			DoUnderAttackBlink();
	int				GetGroupNumber() const;
	int				GetTeamNumber() const;
	void			UpdateState();

	CTFRobotDestruction_RobotGroup *GetGroup( void ) const { return m_hGroup; }

	DHANDLE<CTFHudRobotDestruction_RobotIndicator> m_hRobotIndicator1;
	DHANDLE<CTFHudRobotDestruction_RobotIndicator> m_hRobotIndicator2;

	ERobotState m_eState;

private:
	CHandle<CTFRobotDestruction_RobotGroup> m_hGroup;

	CControlPointIconSwoop *m_pSwoop;
	vgui::EditablePanel *m_pRobotStateContainer;
	CTFHudRobotDestruction_DeadImage *m_pDeadPanel;
	CTFHudRobotDestruction_ActiveImage *m_pActivePanel;
	CTFHudRobotDestruction_StateImage *m_pShieldedPanel;
};


class CTFHUDRobotDestruction : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHUDRobotDestruction, vgui::EditablePanel );
public:
	CTFHUDRobotDestruction( vgui::Panel *parent, const char *name );
	virtual ~CTFHUDRobotDestruction();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual bool IsVisible( void ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	virtual void Paint() OVERRIDE;
	virtual void PaintBackground() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void Reset();

	void PaintPDPlayerScore( CTFPlayer *pPlayer );
	void PerformRobotLayout( CUtlVector<CTFHudRobotDestruction_RobotIndicator *> const &vecRobots, int nTeam );
	void SetPlayingToLabelVisible( bool bVisible );
	void UpdateCarriedFlagStatus( CBasePlayer *pNewOwner, CCaptureFlag *pFlag );
	void UpdateRobotElements();
	void UpdateStolenFlagStatus( int nTeam, CCaptureFlag *pFlag );
	void UpdateStolenPoints( int nTeam, vgui::EditablePanel *pContainer );

private:

	class CProgressPanel : public vgui::ImagePanel, public CGameEventListener
	{
		DECLARE_CLASS_SIMPLE( CProgressPanel, vgui::ImagePanel );
	public:

		CProgressPanel( vgui::Panel *parent, const char *name );

		virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
		virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
		virtual void OnTick() OVERRIDE;
		virtual void PaintBackground() OVERRIDE;

		void CalculateSize()
		{
			int nProgressWidth = m_nWide - m_nRightOffset - m_nLeftOffset;
			m_flXPos = m_bLeftToRight ? m_nX : ( 1.f - m_flCurrentProgress ) * nProgressWidth + m_nX;
			m_flWidth = m_flCurrentProgress * nProgressWidth;

			SetBounds( m_flXPos, m_nY, m_flWidth, m_nTall );
		}

		void SetProgress( float flProgress )
		{
			if ( m_flEndProgress != flProgress )
			{
				vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
				m_flLastTick = gpGlobals->curtime;
			}

			m_flEndProgress = flProgress;
		}

		int m_nX;
		int m_nY;
		int m_nWide;
		int m_nTall;

		float m_flXPos;
		float m_flWidth;
		float m_flLastScoreUpdate;
		float m_flCurrentProgress;
		float m_flEndProgress;
		float m_flLastTick;

		CPanelAnimationVarAliasType( int, m_nLeftOffset, "left_offset", "25", "proportional_int" );
		CPanelAnimationVarAliasType( int, m_nRightOffset, "right_offset", "25", "proportional_int" );
		CPanelAnimationVar( Color, m_StandardColor, "standard_color", "255 255 255 255" );
		CPanelAnimationVar( Color, m_BrightColor, "bright_color", "255 255 255 255" );
		CPanelAnimationVar( bool, m_bLeftToRight, "left_to_right", "1" );
		CPanelAnimationVar( float, m_flApproachSpeed, "approach_speed", "1.f" );
		CPanelAnimationVar( float, m_flBlinkThreshold, "blink_threshold", "2.f" );
		CPanelAnimationVar( float, m_flBlinkRate, "blink_rate", "3.f" );
	};

	KeyValues *m_pRobotIndicatorSettings;

	CUtlVector<CTFHudRobotDestruction_RobotIndicator *> m_vecRedRobots;
	CUtlVector<CTFHudRobotDestruction_RobotIndicator *> m_vecBlueRobots;

	CHandle<CCaptureFlag> m_hRedFlag;
	CHandle<CCaptureFlag> m_hBlueFlag;

	bool m_bInRobotDestruction;

	CExLabel *m_pPlayingTo;
	vgui::Panel *m_pPlayingToBG;
	vgui::EditablePanel *m_pCarriedContainer;
	vgui::ImagePanel *m_pCarriedImage;
	vgui::EditablePanel *m_pScoreContainer;
	vgui::EditablePanel *m_pProgressBarsContainer;
	vgui::EditablePanel *m_pBlueStolenContainer;
	vgui::EditablePanel *m_pBlueDroppedPanel;
	vgui::EditablePanel *m_pRedStolenContainer;
	vgui::EditablePanel *m_pRedDroppedPanel;
	vgui::EditablePanel *m_pBlueScoreValueContainer;
	vgui::EditablePanel *m_pRedScoreValueContainer;
	vgui::EditablePanel *m_pCountdownContainer;
	CTFImagePanel *m_pTeamLeaderImage;
	CProgressPanel *m_pCarriedFlagProgressBar;
	vgui::EditablePanel *m_pRedVictoryPanel;
	CProgressPanel *m_pRedProgressBar;
	CProgressPanel *m_pRedProgressBarEscrow;
	vgui::EditablePanel *m_pBlueVictoryPanel;
	CProgressPanel *m_pBlueProgressBar;
	CProgressPanel *m_pBlueProgressBarEscrow;

	int	m_nStealLeftEdge;
	int	m_nStealRightEdge;
	CPanelAnimationVarAliasType( int, m_nStealLeftEdgeOffset, "left_steal_edge_offset", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nStealRightEdgeOffset, "right_steal_edge_offset", "100", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotXOffset, "robot_x_offset", "6", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotYOffset, "robot_y_offset", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotXStep, "robot_x_step", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotYStep, "robot_y_step", "0", "proportional_int" );

	CPanelAnimationVar( Color, m_ColorBlue, "color_blue", "0 0 255 255" );
	CPanelAnimationVar( Color, m_ColorRed, "color_red", "255 0 0 255" );

	CPanelAnimationVar( vgui::HFont, m_hPDPlayerScoreFont, "player_name_font", "HudFontSmallBold" );
	CPanelAnimationVar( Color, m_TextColor, "text_color", "255 255 255 255" );
};

#endif