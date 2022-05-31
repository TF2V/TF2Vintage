//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hudelement.h"
#include "hud_basechat.h"
#include "hud_vote.h"
#include "econ_notifications.h"
#include "vgui_avatarimage.h"
#include "vgui_controls/TextImage.h"
#include "iclientmode.h"
#include "tf_controls.h"
#include "tf_confirm_dialog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_notifications_show_ingame( "cl_notifications_show_ingame", "1", FCVAR_ARCHIVE, "Whether notifications should show up in-game." );


//-----------------------------------------------------------------------------
static void ColorizeText( CEconNotification *pNotification, CExLabel *pLabel, const wchar_t *wszText )
{
	if ( pLabel )
	{
		pLabel->GetTextImage()->ClearColorChangeStream();

		if ( wszText )
		{
			static wchar_t wszStrippedText[2048];

			int nIndex = 0, nReplaceAt = 0;
			bool bContinue = true;
			while ( bContinue )
			{
				switch ( wszText[nIndex] )
				{
					case COLOR_NORMAL:
					{
						pLabel->GetTextImage()->AddColorChange( pLabel->GetFgColor(), nReplaceAt );
						break;
					}
					case COLOR_PLAYERNAME:
					{
						pLabel->GetTextImage()->AddColorChange( g_ColorYellow, nReplaceAt );
						break;
					}
					case COLOR_LOCATION:
					{
						pLabel->GetTextImage()->AddColorChange( g_ColorDarkGreen, nReplaceAt );
						break;
					}
					case COLOR_ACHIEVEMENT:
					{
						Color color = pLabel->GetFgColor();

						vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme( "SourceScheme" ));
						if ( pScheme )
							color = pScheme->GetColor( "SteamLightGreen", pLabel->GetBgColor() );

						pLabel->GetTextImage()->AddColorChange( color, nReplaceAt );
						break;
					}
					case COLOR_CUSTOM:
					{
						KeyValues *pKV = pNotification->GetKeyValues();
						if ( pKV )
						{
							KeyValues *pColor = pKV->FindKey( "custom_color" );
							if ( pColor )
								pLabel->GetTextImage()->AddColorChange( pColor->GetColor(), nReplaceAt );
						}
					}
					case NULL:
						bContinue = false;
					default:
						wszStrippedText[ nReplaceAt++ ] = wszText[ nIndex ];
						break;
				}

				++nIndex;
			}

			pLabel->SetText( wszStrippedText );
		}
		else
		{
			pLabel->SetText( L"" );
		}
	}
}


static class CEconNotificationQueue
{
	friend void cl_trigger_first_notification_cc( CCommand const & );
	friend void cl_decline_first_notification_cc( CCommand const & );
public:
	CEconNotificationQueue() : m_nNextID( 0 ) {}
	virtual ~CEconNotificationQueue() {}

	int			Add( CEconNotification *pNotification );
	CEconNotification *Get( int iID ) const;
	CEconNotification *GetByIndex( int nIndex ) const;
	void		Remove( int iID );
	void		Remove( CEconNotification *pNotification );
	void		Remove( bool (*filterFunc)( CEconNotification * ) );
	void		RemoveAll( void );

	int			Count( void ) const { return m_Notifications.Count(); }
	int			Count( bool (*filterFunc)( CEconNotification * ) ) const;

	void		Visit( CEconNotificationVisitor &visitor );

	void		Update( void );

private:
	int m_nNextID;
	CUtlVector<CEconNotification *> m_Notifications;
} g_notificationQueue;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconNotificationQueue::Add( CEconNotification *pNotification )
{
	if ( !engine->IsInGame() || ( cl_notifications_show_ingame.GetBool() && pNotification->BShowInGameElements() ) )
		vgui::surface()->PlaySound( pNotification->GetSoundFileName() ? pNotification->GetSoundFileName() : "ui/notification_alert.wav" );

	pNotification->m_iID = ++m_nNextID;
	m_Notifications.AddToTail( pNotification );

	return pNotification->m_iID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconNotification *CEconNotificationQueue::Get( int iID ) const
{
	FOR_EACH_VEC( m_Notifications, i )
	{
		if ( m_Notifications[i]->GetID() == iID )
			return m_Notifications[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconNotification *CEconNotificationQueue::GetByIndex( int nIndex ) const
{
	return m_Notifications[ nIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::Remove( int iID )
{
	FOR_EACH_VEC_BACK( m_Notifications, i )
	{
		CEconNotification *pNotif = m_Notifications[i];
		if ( pNotif->GetID() == iID )
		{
			delete pNotif;
			m_Notifications.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::Remove( CEconNotification *pNotification )
{
	m_Notifications.FindAndRemove( pNotification );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::Remove( bool( *filterFunc )( CEconNotification * ) )
{
	FOR_EACH_VEC( m_Notifications, i )
	{
		if ( filterFunc( m_Notifications[i] ) )
			m_Notifications[i]->MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::RemoveAll( void )
{
	m_Notifications.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconNotificationQueue::Count( bool( *filterFunc )( CEconNotification * ) ) const
{
	int nCount = 0;
	FOR_EACH_VEC( m_Notifications, i )
	{
		if ( filterFunc( m_Notifications[i] ) )
			nCount++;
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::Visit( CEconNotificationVisitor &visitor )
{
	FOR_EACH_VEC( m_Notifications, i )
	{
		visitor.Visit( m_Notifications[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotificationQueue::Update( void )
{
	FOR_EACH_VEC( m_Notifications, i )
	{
		CEconNotification *pNotif = m_Notifications[i];
		if ( !pNotif->GetIsInUse() )
		{
			if ( pNotif->GetExpireTime() >= 0 || pNotif->GetExpireTime() < engine->Time() )
			{
				pNotif->Expired();
				delete pNotif;

				m_Notifications.FastRemove( i );

				continue;
			}
		}

		pNotif->UpdateTick();
	}
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CGenericNotificationToast::CGenericNotificationToast( vgui::Panel *parent, int iID, bool bMainMenu ) 
	: BaseClass(parent, "GenericNotificationToast")
{
	m_iID = iID;
	m_bMainMenu = bMainMenu;
	m_pAvatar = NULL;
	m_pAvatarBG = NULL;
}

CGenericNotificationToast::~CGenericNotificationToast()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGenericNotificationToast::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValuesAD pConditionKVs( "conditions" );
	CEconNotification *pMyNotif = NotificationQueue_Get( m_iID );
	if ( pMyNotif )
	{
		if ( pMyNotif->BHighPriority() )
			pConditionKVs->AddSubKey( new KeyValues( "if_high_priority" ) );
	}

	if ( m_bMainMenu )
	{
		LoadControlSettings( "Resource/UI/Econ/GenericNotificationToastMainMenu.res", NULL, NULL, pConditionKVs );
	}
	else
	{
		LoadControlSettings( "Resource/UI/Econ/GenericNotificationToast.res", NULL, NULL, pConditionKVs );
	}

	m_pAvatar = FindControl<CAvatarImagePanel>( "AvatarImage" );
	m_pAvatarBG = FindChildByName( "AvatarBGPanel" );

	if ( pMyNotif )
	{
		if ( pMyNotif->GetSteamID() == CSteamID() )
		{
			ColorizeText( pMyNotif, FindControl<CExLabel>( "TextLabel" ), pMyNotif->GetText() );
		}
		else
		{
			ColorizeText( pMyNotif, FindControl<CExLabel>( "AvatarTextLabel" ), pMyNotif->GetText() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGenericNotificationToast::OnThink( void )
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGenericNotificationToast::PerformLayout()
{
	BaseClass::PerformLayout();
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconNotification::CEconNotification()
{
	m_pszText = "";
	m_flLifeTime = engine->Time() + 10;
	m_pKeyValues = NULL;
	m_pszSoundName = NULL;
	m_bInUse = false;
}

CEconNotification::~CEconNotification()
{
	Deleted();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::SetText( const char *pszText )
{
	m_pszText = pszText;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconNotification::GetUnlocalizedHelpText()
{
	Assert( NotificationType() >= ENotificationType::General && NotificationType() <= ENotificationType::OnlyTrigger );
	static char const *const s_helpText[] ={
		"#Notification_Remove_Help",
		"#Notification_AcceptOrDecline_Help",
		"#Notification_CanTrigger_Help",
		"#Notification_CanTrigger_Help"
	};

	return s_helpText[ (int)NotificationType() ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::EditablePanel *CEconNotification::CreateUIElement( bool bMainMenu ) const
{
	return new CGenericNotificationToast( NULL, m_iID, bMainMenu );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::AddStringToken( const char *pszToken, const wchar_t *pwszValue )
{
	if ( !m_pKeyValues )
	{
		m_pKeyValues = new KeyValues( "CEconNotification" );
	}
	m_pKeyValues->SetWString( pszToken, pwszValue );

	g_pVGuiLocalize->ConstructString( m_wszBuffer, ARRAYSIZE( m_wszBuffer ), m_pszText, m_pKeyValues );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconNotification::GetID() const
{
	return m_iID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::SetKeyValues( KeyValues *pKeyValues )
{
	if ( m_pKeyValues )
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = pKeyValues->MakeCopy();

	g_pVGuiLocalize->ConstructString( m_wszBuffer, ARRAYSIZE( m_wszBuffer ), m_pszText, m_pKeyValues );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::SetLifetime( float flTime )
{
	m_flLifeTime = flTime + engine->Time();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::SetIsInUse( bool bSet )
{
	m_bInUse = bSet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::SetSteamID( const CSteamID &steamID )
{
	m_steamID = steamID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconNotification::MarkForDeletion()
{
	m_flLifeTime = 0;
}


//-----------------------------------------------------------------------------
int NotificationQueue_Add( CEconNotification *pNotification )
{
	return g_notificationQueue.Add( pNotification );
}

int NotificationQueue_Count( bool( *filterFunc )( CEconNotification * ) )
{
	return g_notificationQueue.Count( filterFunc );
}

CEconNotification *NotificationQueue_Get( int iID )
{
	return g_notificationQueue.Get( iID );
}

CEconNotification *NotificationQueue_GetByIndex( int nIndex )
{
	return g_notificationQueue.GetByIndex( nIndex );
}

int NotificationQueue_GetNumNotifications()
{
	return g_notificationQueue.Count();
}

void NotificationQueue_RemoveAll()
{
	g_notificationQueue.RemoveAll();
}

void NotificationQueue_Remove( CEconNotification *pNotification )
{
	g_notificationQueue.Remove( pNotification );
}

void NotificationQueue_Remove( bool( *filterFunc )( CEconNotification * ) )
{
	g_notificationQueue.Remove( filterFunc );
}

void NotificationQueue_Remove( int iID )
{
	g_notificationQueue.Remove( iID );
}

void NotificationQueue_Update( void )
{
	g_notificationQueue.Update();
}

void NotificationQueue_Visit( CEconNotificationVisitor &visitor )
{
	g_notificationQueue.Visit( visitor );
}

vgui::EditablePanel *NotificationQueue_CreateMainMenuUIElement( vgui::EditablePanel *pParent, const char *pElementName )
{
	// TODO
	return nullptr;
}


//-----------------------------------------------------------------------------
CON_COMMAND_EXTERN( cl_trigger_first_notification, cl_trigger_first_notification_cc, "Tries to accept/trigger the first notification" )
{
	if ( !g_notificationQueue.m_Notifications.IsEmpty() )
	{
		CEconNotification *pNotification = g_notificationQueue.m_Notifications.Head();
		switch ( pNotification->NotificationType() )
		{
			case CEconNotification::ENotificationType::AcceptOrDecline:
				pNotification->Accept();
				break;
			case CEconNotification::ENotificationType::Trigger:
			case CEconNotification::ENotificationType::OnlyTrigger:
				pNotification->Trigger();
				break;
			case CEconNotification::ENotificationType::General:
				break;
			default:
				Assert( false );
				break;
		}
	}
}

CON_COMMAND_EXTERN( cl_decline_first_notification, cl_decline_first_notification_cc, "Tries to decline/remove the first notification" )
{
	if ( !g_notificationQueue.m_Notifications.IsEmpty() )
	{
		CEconNotification *pNotification = g_notificationQueue.m_Notifications.Head();
		switch ( pNotification->NotificationType() )
		{
			case CEconNotification::ENotificationType::AcceptOrDecline:
				pNotification->Decline();
				break;
			case CEconNotification::ENotificationType::Trigger:
			case CEconNotification::ENotificationType::General:
				pNotification->MarkForDeletion();
				break;
			case CEconNotification::ENotificationType::OnlyTrigger:
				break;
			default:
				Assert( false );
				break;
		}
	}
}

