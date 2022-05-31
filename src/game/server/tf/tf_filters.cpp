//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "filters.h"
#include "team_control_point.h"
#include "tf_bot.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// Team Fortress Team Filter
//
class CFilterTFTeam : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFTeam, CBaseFilter );

public:

	void InputRoundSpawn( inputdata_t &inputdata );
	void InputRoundActivate( inputdata_t &inputdata );

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:

	string_t m_iszControlPointName;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFTeam )

DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),

// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( filter_activator_tfteam, CFilterTFTeam );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFilterTFTeam::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	// is the entity we're asking about on the winning 
	// team during the bonus time? (winners pass all filters)
	if (  TFGameRules() &&
		( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && 
		( TFGameRules()->GetWinningTeam() == pEntity->GetTeamNumber() ) )
	{
		// this should open all doors for the winners
		if ( m_bNegated )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return ( pEntity->GetTeamNumber() == GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterTFTeam::InputRoundSpawn( inputdata_t &input )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFilterTFTeam::InputRoundActivate( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		CTeamControlPoint *pControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( pControlPoint )
		{
			ChangeTeam( pControlPoint->GetTeamNumber() );
		}
		else
		{
			Warning( "%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}
}

//=============================================================================
//
// Class filter
//
class CFilterTFClass : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFClass, CBaseFilter );

public:

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

private:

	int	m_iAllowedClass;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFClass )

DEFINE_KEYFIELD( m_iAllowedClass, FIELD_INTEGER, "classfilter" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( filter_activator_tfclass, CFilterTFClass );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFilterTFClass::PassesFilterImpl(CBaseEntity *pCaller, CBaseEntity *pEntity)
{
	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >(pEntity);

	if (!pPlayer)
		return false;

	// is the entity we're asking about on the winning 
	// team during the bonus time? (winners pass all filters)

	if (  TFGameRules() &&
		( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && 
		(TFGameRules()->GetWinningTeam() == pPlayer->GetTeamNumber()))
	{
		// this should open all doors for the winners
		if ( m_bNegated )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return (pPlayer->GetTeamNumber() == GetTeamNumber() && pPlayer->IsPlayerClass(m_iAllowedClass));
}

//=============================================================================
//
// Tagged Team Fortress Bot filter
//
class CFilterTFBotHasTag : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFBotHasTag, CBaseFilter );
	
public:

	inline void Spawn() OVERRIDE
	{
		BaseClass::Spawn();

		const char *tags = STRING( m_iszTags );
		CSplitString splitTags( tags, " " );
		for ( int i=0; i < splitTags.Count(); ++i )
		{
			m_tags.CopyAndAddToTail( splitTags[i] );
		}
	}

	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity ) OVERRIDE
	{
		CTFBot *pBot = ToTFBot( pEntity );

		if ( !pBot )
			return false;

		bool bPasses = false;
		for ( int i=0; i < m_tags.Count(); ++i )
		{
			const char *pszTag = m_tags[i];
			if ( pBot->HasTag( pszTag ) )
			{
				bPasses = true;
				if ( !m_bRequireAllTags )
				{
					break;
				}
			}
			else if ( m_bRequireAllTags )
			{
				return false;
			}
		}

		return bPasses;
	}

private:

	CUtlStringList m_tags;
	string_t m_iszTags;
	bool m_bRequireAllTags;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFBotHasTag )
	DEFINE_KEYFIELD( m_iszTags, FIELD_STRING, "tags" ),
	DEFINE_KEYFIELD( m_bRequireAllTags, FIELD_BOOLEAN, "require_all_tags" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( filter_tf_bot_has_tag, CFilterTFBotHasTag );


//=============================================================================
//
// Team Fortress Condition Filter
//
class CFilterTFCondition : public CBaseFilter
{
	DECLARE_CLASS( CFilterTFCondition, CBaseFilter );

public:
	inline bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity ) OVERRIDE
	{
		if ( !pEntity->IsPlayer() )
			return false;

		CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
		return pTFPlayer && pTFPlayer->m_Shared.InCond( m_nCondition );
	}

private:

	int m_nCondition;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFilterTFCondition )
	DEFINE_KEYFIELD( m_nCondition, FIELD_INTEGER, "condition" ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( filter_tf_condition, CFilterTFCondition );
