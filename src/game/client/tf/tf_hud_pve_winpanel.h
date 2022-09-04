#ifndef TF_HUD_PVE_WIN_PANEL_H
#define TF_HUD_PVE_WIN_PANEL_H
#pragma once

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "hudelement.h"
#include "tf_shareddefs.h"


class CTFPVEWinPanel : public vgui::EditablePanel, public CGameEventListener, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE( CTFPVEWinPanel, vgui::EditablePanel );
public:
	CTFPVEWinPanel( IViewPort *pViewPort );

	void		ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	void		ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	void		FireGameEvent( IGameEvent *event ) OVERRIDE;
	const char *GetName( void )	OVERRIDE { return PANEL_PVE_WIN; }
	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }
	virtual int	GetRenderGroupPriority() { return 70; }
	vgui::VPANEL GetVPanel( void ) OVERRIDE { return BaseClass::GetVPanel(); }
	bool		HasInputElements( void ) OVERRIDE { return true; }
	bool		IsVisible( void ) OVERRIDE { return BaseClass::IsVisible(); }
	bool		NeedsUpdate( void ) OVERRIDE { return false; }
	void		OnTick( void ) OVERRIDE;
	void		Reset( void ) OVERRIDE;
	void		SetData( KeyValues *pData ) OVERRIDE {}
	void		SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); }
	void		ShowPanel( bool bShow ) OVERRIDE {}
	void		Update( void ) OVERRIDE;

private:
	vgui::ScalableImagePanel *m_pRespecBackground;
	vgui::EditablePanel *m_pRespecContainerPanel;
	vgui::Label *m_pRespecTextLabel;
	vgui::Label *m_pRespecCountLabel;
	bool m_bShouldBeVisible;
};

#endif