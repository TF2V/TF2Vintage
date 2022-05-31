//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "tf_spectatorgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf2v_allow_disguiseweapons;
	
ConVar tf2v_enable_disguise_text( "tf2v_enable_disguise_text", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Enables the text notification in chatbox when you are diguised.", true, 0.0f, true, 1.0f );
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDisguiseStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CDisguiseStatus, vgui::EditablePanel);

public:
	CDisguiseStatus(const char *pElementName);
	void			Init(void);
	virtual void	ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void	Paint(void);
	virtual bool	ShouldDraw(void);
	void			HideStatus(void);
	void			ShowAndUpdateStatus(void);
	void			CheckWeapon(void);

private:
	CPanelAnimationVar(vgui::HFont, m_hFont, "TextFont", "TargetID");
	CTFImagePanel		*m_pDisguiseStatusBG;
	vgui::Label			*m_pDisguiseNameLabel;
	vgui::Label			*m_pWeaponNameLabel;
	CTFSpectatorGUIHealth	*m_pTargetHealth;
	//CEmbeddedItemModelPanel *m_pItemModelPanel;

	bool				m_bVisible;
	int					m_iCurrentDisguiseTeam;
};

DECLARE_HUDELEMENT(CDisguiseStatus);

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDisguiseStatus::CDisguiseStatus(const char *pElementName) :
CHudElement(pElementName), BaseClass(NULL, "DisguiseStatus")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pTargetHealth = new CTFSpectatorGUIHealth(this, "SpectatorGUIHealth");

	SetHiddenBits(HIDEHUD_MISCSTATUS);
	m_iCurrentDisguiseTeam = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CDisguiseStatus::Init(void)
{
	HideStatus();
}

//-----------------------------------------------------------------------------
// Purpose: Hide all elements
//-----------------------------------------------------------------------------
void CDisguiseStatus::HideStatus(void)
{
	if (m_pDisguiseStatusBG)
		m_pDisguiseStatusBG->SetVisible(false);

	if (m_pDisguiseNameLabel)
		m_pDisguiseNameLabel->SetVisible(false);

	if (m_pWeaponNameLabel)
		m_pWeaponNameLabel->SetVisible(false);

	if (m_pTargetHealth)
		m_pTargetHealth->SetVisible(false);

	m_bVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDisguiseStatus::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer)
		return false;

	if (pPlayer->m_Shared.InCond(TF_COND_DISGUISED))
	{
		if ( !m_bVisible || m_iCurrentDisguiseTeam != pPlayer->m_Shared.GetDisguiseTeam() )
			ShowAndUpdateStatus();

		return true;
	}
	else
	{
		HideStatus();
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	LoadControlSettings("resource/UI/DisguiseStatusPanel.res");

	m_pTargetHealth = dynamic_cast< CTFSpectatorGUIHealth *>(FindChildByName("SpectatorGUIHealth"));
	//m_pItemModelPanel = dynamic_cast< CEmbeddedItemModelPanel *>( FindChildByName( "CEmbeddedItemModelPanel" ) );
	m_pDisguiseStatusBG = dynamic_cast< CTFImagePanel * >(FindChildByName("DisguiseStatusBG"));
	m_pDisguiseNameLabel = dynamic_cast< vgui::Label *>(FindChildByName("DisguiseNameLabel"));
	m_pWeaponNameLabel = dynamic_cast< vgui::Label *>(FindChildByName("WeaponNameLabel"));

	SetPaintBackgroundEnabled(false);

	HideStatus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::CheckWeapon(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer)
		return;

	CEconItemDefinition *pItem = pPlayer->m_Shared.GetDisguiseItem()->GetStaticData();
	if (pItem)
		SetDialogVariable( "weaponname", pItem->GenerateLocalizedItemNameNoQuality() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDisguiseStatus::ShowAndUpdateStatus(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer)
		return;

	m_iCurrentDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();

	if (m_pDisguiseStatusBG)
	{
		m_pDisguiseStatusBG->SetVisible(true);

		// This isn't how live tf2 does it, they simply call UpdateBGImage, however I'm not sure what exactly
		// they're doing.
		m_pDisguiseStatusBG->SetBGImage(m_iCurrentDisguiseTeam);
		//m_pDisguiseStatusBG->UpdateBGImage();
	}

	if (m_pDisguiseNameLabel)
	{
		m_pDisguiseNameLabel->SetVisible(true);
		C_BasePlayer *pDisguiseTarget = ToBasePlayer(pPlayer->m_Shared.GetDisguiseTarget());
		if (pDisguiseTarget)
		{ 
			//const char *nameError = PLAYER_ERROR_NAME, *nameUnconnected = PLAYER_UNCONNECTED_NAME;
			//if (pPlayer->GetPlayerName() == nameError || pPlayer->GetPlayerName() == nameUnconnected || pDisguiseTarget->GetTeamNumber() != pPlayer->m_Shared.GetDisguiseTeam()) //check for undefined player names
				//SetDialogVariable("disguisename", pPlayer->GetPlayerName());
			if (pPlayer->m_Shared.GetDisguiseTarget() == NULL)
				SetDialogVariable("disguisename", pPlayer->GetPlayerName());
			else
				SetDialogVariable("disguisename", pDisguiseTarget->GetPlayerName());
		}
	}

	if (m_pWeaponNameLabel)
	{
		// Only show weapon name when we can swap disguise weapons.
		if ( tf2v_allow_disguiseweapons.GetBool() )
			m_pWeaponNameLabel->SetVisible(true);
		CheckWeapon();
	}

	if (m_pTargetHealth)
	{
		m_pTargetHealth->SetVisible(true);
		m_pTargetHealth->SetHealth(pPlayer->m_Shared.GetDisguiseHealth(), pPlayer->m_Shared.GetDisguiseMaxHealth(), pPlayer->m_Shared.GetDisguiseMaxBuffedHealth());
	}

	m_bVisible = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CDisguiseStatus::Paint(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer)
		return;

	// We don't print anything until we're fully disguised
	if (!pPlayer->m_Shared.InCond(TF_COND_DISGUISED))
		return;

	C_BasePlayer *pDisguiseTarget = ToBasePlayer(pPlayer->m_Shared.GetDisguiseTarget());
	if (pDisguiseTarget)
	{
		SetDialogVariable("disguisename", pDisguiseTarget->GetPlayerName());
	}

	CheckWeapon();

	if (m_pTargetHealth)
		m_pTargetHealth->SetHealth(pPlayer->m_Shared.GetDisguiseHealth(), pPlayer->m_Shared.GetDisguiseMaxHealth(), pPlayer->m_Shared.GetDisguiseMaxBuffedHealth());
	
	if (tf2v_enable_disguise_text.GetBool())
	{
		int xpos = 0;
		int ypos = 0;

		#define MAX_DISGUISE_STATUS_LENGTH	128
		wchar_t szStatus[MAX_DISGUISE_STATUS_LENGTH];
		szStatus[0] = '\0';

		wchar_t *pszTemplate = NULL;
		int nDisguiseClass = TF_CLASS_UNDEFINED, nDisguiseTeam = TEAM_UNASSIGNED;
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			pszTemplate = g_pVGuiLocalize->Find( "#TF_Spy_Disguising" );
			nDisguiseClass = pPlayer->m_Shared.GetDesiredDisguiseClass();
			nDisguiseTeam = pPlayer->m_Shared.GetDesiredDisguiseTeam();
		}
		else if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			pszTemplate = g_pVGuiLocalize->Find( "#TF_Spy_Disguised_as" );
			nDisguiseClass = pPlayer->m_Shared.GetDisguiseClass();
			nDisguiseTeam = pPlayer->m_Shared.GetDisguiseTeam();
		}

		wchar_t *pszClassName = g_pVGuiLocalize->Find( GetPlayerClassData( nDisguiseClass )->m_szLocalizableName );

		wchar_t *pszTeamName = NULL;
		if ( nDisguiseTeam == TF_TEAM_RED )
		{
			pszTeamName = g_pVGuiLocalize->Find("#TF_Spy_Disguise_Team_Red");
		}
		else
		{
			pszTeamName = g_pVGuiLocalize->Find("#TF_Spy_Disguise_Team_Blue");
		}

		if ( pszTemplate && pszClassName && pszTeamName )
		{
			wcsncpy( szStatus, pszTemplate, MAX_DISGUISE_STATUS_LENGTH );

			g_pVGuiLocalize->ConstructString( szStatus, MAX_DISGUISE_STATUS_LENGTH*sizeof(wchar_t), pszTemplate,
				2,
				pszTeamName,
				pszClassName );	
		}

		if ( szStatus[0] != '\0' )
		{
			vgui::surface()->DrawSetTextFont( m_hFont );

			Color cPlayerColor;

			if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
			{
				cPlayerColor = g_PR->GetTeamColor( pPlayer->m_Shared.GetDisguiseTeam() );
			}
			else
			{
				cPlayerColor = g_PR->GetTeamColor( pPlayer->GetTeamNumber() );
			}

			// draw a black dropshadow ( the default one looks horrible )
			vgui::surface()->DrawSetTextPos( xpos+1, ypos+1 );
			vgui::surface()->DrawSetTextColor( Color(0,0,0,255) );
			vgui::surface()->DrawPrintText( szStatus, wcslen(szStatus) );		

			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( cPlayerColor );
			vgui::surface()->DrawPrintText( szStatus, wcslen(szStatus) );
		}
	}
}
