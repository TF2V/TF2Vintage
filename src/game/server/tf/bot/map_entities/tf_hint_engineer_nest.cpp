//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hint_engineer_nest.h"
#include "tf_hint_teleexit.h"
#include "tf_obj.h"


CTFBotHintEngineerNest::CTFBotHintEngineerNest()
{
	m_bHasActiveTeleporter = false;
}

CTFBotHintEngineerNest::~CTFBotHintEngineerNest()
{
}


int CTFBotHintEngineerNest::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}


void CTFBotHintEngineerNest::Spawn()
{
	CPointEntity::Spawn();

	SetThink( &CTFBotHintEngineerNest::HintThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


CBaseTFBotHintEntity::HintType CTFBotHintEngineerNest::GetHintType() const
{
	return CBaseTFBotHintEntity::HintType::ENGINEER_NEST;
}


void CTFBotHintEngineerNest::DetonateStaleNest()
{
	this->DetonateObjectsFromHints( this->m_SentryHints );
	this->DetonateObjectsFromHints( this->m_TeleHints );
}


CTFBotHintSentrygun *CTFBotHintEngineerNest::GetSentryHint() const
{
	return static_cast<CTFBotHintSentrygun *>( GetHint( this->m_SentryHints ) );
}

CTFBotHintTeleporterExit *CTFBotHintEngineerNest::GetTeleporterHint() const
{
	return static_cast<CTFBotHintTeleporterExit *>( GetHint( this->m_TeleHints ) );
}


bool CTFBotHintEngineerNest::IsStaleNest() const
{
	FOR_EACH_VEC( this->m_SentryHints, i )
	{
		if ( this->m_SentryHints[i]->OwnerObjectHasNoOwner() )
			return true;
	}

	FOR_EACH_VEC( this->m_TeleHints, i )
	{
		if ( this->m_TeleHints[i]->OwnerObjectHasNoOwner() )
			return true;
	}

	return false;
}


void CTFBotHintEngineerNest::HintThink()
{
	for ( int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i )
	{
		CBaseTFBotHintEntity *hint = static_cast<CBaseTFBotHintEntity *>(
			ITFBotHintEntityAutoList::AutoList()[i] );

		/* warning: string pointer comparison */
		if ( hint->GetHintType() == CBaseTFBotHintEntity::HintType::SENTRY_GUN &&
			 hint->GetEntityName() == this->GetEntityName() )
		{
			this->m_SentryHints.AddToTail( hint );
		}
		else if ( hint->GetHintType() == CBaseTFBotHintEntity::HintType::TELEPORTER_EXIT &&
				  hint->GetEntityName() == this->GetEntityName() )
		{
			this->m_TeleHints.AddToTail( hint );
		}
	}

	if ( this->m_SentryHints.IsEmpty() && this->m_TeleHints.IsEmpty() )
	{
		Warning( "Must have a teleporter and/or a sentry hint with the same name.\n" );
	}

	SetThink( &CTFBotHintEngineerNest::HintTeleporterThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CTFBotHintEngineerNest::HintTeleporterThink()
{
	bool now_has_active_tele = false;
	FOR_EACH_VEC( this->m_TeleHints, i )
	{
		CTFBotHintTeleporterExit *hint_tele = (CTFBotHintTeleporterExit *)this->m_TeleHints[i].Get();
		if ( hint_tele == nullptr ) continue;

		CBaseObject *tele = dynamic_cast<CBaseObject *> ( hint_tele->GetOwnerEntity() );
		if ( tele == nullptr ) continue;

		now_has_active_tele |= !tele->IsBuilding();
	}
	m_bHasActiveTeleporter = now_has_active_tele;

	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTFBotHintEngineerNest::DetonateObjectsFromHints( const CUtlVector< CHandle<CBaseTFBotHintEntity> > &hints )
{
	FOR_EACH_VEC( hints, i )
	{
		if ( !hints[i]->OwnerObjectHasNoOwner() )
			continue;

		CBaseEntity *owner = hints[i]->GetOwnerEntity();
		if ( owner != nullptr )
		{
			CBaseObject *obj = static_cast<CBaseObject *>( owner );
			obj->DetonateObject();
		}
	}
}

CBaseTFBotHintEntity *CTFBotHintEngineerNest::GetHint( const CUtlVector< CHandle<CBaseTFBotHintEntity> > &hints ) const
{
	if ( hints.IsEmpty() )
		return nullptr;

	FOR_EACH_VEC( hints, i )
	{
		if ( !hints[i]->OwnerObjectHasNoOwner() )
			continue;

		return hints[i];
	}

	return hints.Random();
}
