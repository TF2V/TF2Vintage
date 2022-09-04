//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_UPGRADES_H
#define C_TF_UPGRADES_H

#include "hudelement.h"
#include "vgui_controls/EditablePanel.h"

class CItemModelPanel;
class CEconItemView;
class CImageButton;

namespace vgui
{
	class ImagePanel;
	class Button;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CUpgradeBuyPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CUpgradeBuyPanel, vgui::EditablePanel );
public:
	CUpgradeBuyPanel( vgui::Panel *parent, const char *panelName );
	virtual ~CUpgradeBuyPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;

	void SetNumLevelImages( int nValues );
	void UpdateImages( int nCurrentMoney );
	bool ValidateUpgradeStepData( void );

private:

	CPanelAnimationVarAliasType( int, m_iUpgradeButtonXPos, "upgradebutton_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeButtonYPos, "upgradebutton_ypos", "0", "proportional_int" );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudUpgradePanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudUpgradePanel, vgui::EditablePanel );

public:
	CHudUpgradePanel( const char *pElementName );
	virtual ~CHudUpgradePanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual GameActionSet_t GetPreferredActionSet( void ) { return GAME_ACTION_SET_MENUCONTROLS; }
	virtual int	GetRenderGroupPriority( void ) OVERRIDE { return 35; }
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnTick( void ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void SetActive( bool bActive ) OVERRIDE;
	virtual void SetVisible( bool bVisible ) OVERRIDE;
	virtual bool ShouldDraw( void ) OVERRIDE;
	virtual void SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );

	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );
	MESSAGE_FUNC_PTR( OnItemPanelMousePressed, "ItemPanelMousePressed", panel );

	void			CancelUpgrades( void );
	void			CreateItemModelPanel( int i1 );
	CEconItemView	*GetLocalPlayerBottleFromInventory( void );
	void			PlayerInventoryChanged( C_TFPlayer *pPlayer );
	void			QuickEquipBottle( void );
	void			UpdateButtonStates( int i1, int i2, int i3 );
	void			UpdateHighlights( void );
	void			UpdateModelPanels( void );
	void			UpdateMouseOverHighlight( void );
	void			UpdateUpgradeButtons( void );
	void			UpgradeItemInSlot( int iItemSlot );

private:

	CPanelAnimationVarAliasType( int, m_iItemPanelXPos, "itempanel_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelYPos, "itempanel_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelXDelta, "itempanel_xdelta", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iItemPanelYDelta, "itempanel_ydelta", "0", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelXPos, "upgradebuypanel_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelYPos, "upgradebuypanel_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iUpgradeBuyPanelDelta, "upgradebuypanel_delta", "0", "proportional_int" );
};

bool MannVsMachine_GetUpgradeInfo( int iAttribute, int iQuality, float *flValue );

#endif // C_TF_UPGRADES_H