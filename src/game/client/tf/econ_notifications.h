//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ECON_NOTIFICATIONS_H
#define ECON_NOTIFICATIONS_H

#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "vgui_controls/EditablePanel.h"

class CAvatarImagePanel;


class CGenericNotificationToast : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CGenericNotificationToast, vgui::EditablePanel );
public:
	CGenericNotificationToast( vgui::Panel *parent, int iID, bool bMainMenu );
	virtual ~CGenericNotificationToast();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnThink( void ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
protected:
	int 				m_iID;
	vgui::Panel			*m_pAvatarBG;
	CAvatarImagePanel	*m_pAvatar;
	bool				m_bMainMenu;
};

//-----------------------------------------------------------------------------
class CEconNotification
{
	friend class CEconNotificationQueue;
public:
	CEconNotification();
	virtual ~CEconNotification();

	enum class ENotificationType
	{
		General,
		AcceptOrDecline,
		Trigger,
		OnlyTrigger
	};

	virtual void	SetLifetime( float flTime );
	virtual float	GetExpireTime( void ) const { return m_flLifeTime; }
	virtual float	GetInGameLifeTime( void ) const { return m_flLifeTime; }
	virtual bool	BShowInGameElements( void ) const { return true; }
	virtual void	MarkForDeletion( void );
	virtual ENotificationType NotificationType( void ) { return ENotificationType::General; }
	virtual bool	BHighPriority( void ) { return false; }
	virtual void	Trigger( void ) {}
	virtual void	Accept( void ) {}
	virtual void	Decline( void ) {}
	virtual void	Deleted( void ) {}
	virtual void	Expired( void ) {}
	virtual void	UpdateTick( void ) {}
	virtual const char *GetUnlocalizedHelpText( void );
	virtual vgui::EditablePanel *CreateUIElement( bool bMainMenu ) const;

	void	AddStringToken( char const *pszToken, wchar_t const *pwszValue );
	int		GetID() const;
	void	SetIsInUse( bool bSet );
	bool	GetIsInUse( void ) const { return m_bInUse; }
	void	SetSteamID( CSteamID const &steamID );
	CSteamID const &GetSteamID( void ) const { return m_steamID; }
	void	SetText( char const *pszText );
	const wchar_t *GetText( void ) const { return m_wszBuffer; }
	void	SetKeyValues( KeyValues *pKeyValues );
	KeyValues *GetKeyValues( void ) const { return m_pKeyValues; }
	char const *GetSoundFileName( void ) const { return m_pszSoundName; }

protected:
	char const *m_pszText;
	char const *m_pszSoundName;
	float m_flLifeTime;
	KeyValues *m_pKeyValues;
	wchar_t m_wszBuffer[1024];
	CSteamID m_steamID;

private:
	int m_iID;
	bool m_bInUse;
};

class CEconNotificationVisitor
{
public:
	virtual void Visit( CEconNotification *pNotification ) = 0;
};

int NotificationQueue_Add( CEconNotification *pNotification );
int NotificationQueue_Count( bool ( *filterFunc )( CEconNotification * ) );
vgui::EditablePanel *NotificationQueue_CreateMainMenuUIElement( vgui::EditablePanel *pParent, const char *pElementName );
CEconNotification *NotificationQueue_Get( int iID );
CEconNotification *NotificationQueue_GetByIndex( int nIndex );
int NotificationQueue_GetNumNotifications( void );
void NotificationQueue_RemoveAll( void );
void NotificationQueue_Remove( CEconNotification *pNotification );
void NotificationQueue_Remove( bool ( *filterFunc )( CEconNotification * ) );
void NotificationQueue_Remove( int iID );
void NotificationQueue_Update( void );
void NotificationQueue_Visit( CEconNotificationVisitor &visitor );

#endif // ECON_NOTIFICATIONS_H
