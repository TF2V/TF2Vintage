//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_FLAGSTATUS_H
#define TF_HUD_FLAGSTATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "entity_capture_flag.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose:  Draws the rotated arrow panels
//-----------------------------------------------------------------------------
class CTFArrowPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CTFArrowPanel, vgui::Panel );

	CTFArrowPanel( vgui::Panel *parent, const char *name );
	virtual void Paint();
	virtual bool IsVisible( void );
	void SetEntity( EHANDLE hEntity ){ m_hEntity = hEntity; }
	float GetAngleRotation( void );

private:

	EHANDLE				m_hEntity;	

	CMaterialReference	m_RedMaterial;
	CMaterialReference	m_BlueMaterial;
	CMaterialReference	m_GreenMaterial;
	CMaterialReference	m_YellowMaterial;
	CMaterialReference	m_NeutralMaterial;

	CMaterialReference	m_RedMaterialNoArrow;
	CMaterialReference	m_BlueMaterialNoArrow;
	CMaterialReference	m_GreenMaterialNoArrow;
	CMaterialReference	m_YellowMaterialNoArrow;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFFlagStatus : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFFlagStatus, vgui::EditablePanel );

	CTFFlagStatus( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	void UpdateStatus( void );

	void SetEntity( EHANDLE hEntity )
	{ 
		m_hEntity = hEntity;

		if ( m_pArrow )
		{
			m_pArrow->SetEntity( hEntity );
		}
	}

private:

	EHANDLE			m_hEntity;

	CTFArrowPanel	*m_pArrow;
	CTFImagePanel	*m_pStatusIcon;
	CTFImagePanel	*m_pBriefcase;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudFlagObjectives : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudFlagObjectives, vgui::EditablePanel );

public:

	CTFHudFlagObjectives( vgui::Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsVisible( void );
	virtual void Reset();
	void OnTick();

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent *event );

private:
	
	void UpdateStatus( void );
	void SetPlayingToLabelVisible( bool bVisible );

private:

	CTFImagePanel			*m_pCarriedImage;

	CExLabel				*m_pPlayingTo;
	CTFImagePanel			*m_pPlayingToBG;

	CTFFlagStatus			*m_pRedFlag;
	CTFFlagStatus			*m_pBlueFlag;
	CTFFlagStatus			*m_pGreenFlag;
	CTFFlagStatus			*m_pYellowFlag;
	CTFArrowPanel			*m_pCapturePoint;

	bool					m_bFlagAnimationPlayed;
	bool					m_bCarryingFlag;

	vgui::ImagePanel		*m_pSpecCarriedImage;
};

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFFlagCalloutPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFFlagCalloutPanel, vgui::EditablePanel );
	static CUtlVector<CTFFlagCalloutPanel *> sm_FlagCalloutPanels;
public:

	CTFFlagCalloutPanel( const char *name );
	virtual ~CTFFlagCalloutPanel();

	static CTFFlagCalloutPanel *AddFlagCalloutIfNotFound( CCaptureFlag *pFlag, float f, Vector const &v );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	virtual void Paint( void );
	virtual void PaintBackground( void );
	virtual void PerformLayout( void );

	void GetCalloutPosition( const Vector &vecDelta, float flRadius, int *xpos, int *ypos, float *flRotation );
	void ScaleAndPositionCallout( float flScale );
	void SetFlag( CCaptureFlag *pFlag, float f, Vector const &v );
	bool ShouldShowFlagIconToLocalPlayer( void ) const;

private:
	CHandle<CCaptureFlag> m_hFlag;

	CTFImagePanel *m_pFlagCalloutPanel;
	vgui::Label *m_pFlagValueLabel;
	CTFImagePanel *m_pFlagStatusIcon;

	IMaterial *m_pArrowMaterial;

	float m_f1;
	float m_flLastUpdate;
	Vector m_v1;
	int m_i1;
	bool m_bInLOS;

	float m_flPrevScale;
	int m_nPanelWide;
	int m_nPanelTall;
	int m_nLabelWide;
	int m_nLabelTall;
	int m_nIconWide;
	int m_nIconTall;
};
#endif	// TF_HUD_FLAGSTATUS_H
