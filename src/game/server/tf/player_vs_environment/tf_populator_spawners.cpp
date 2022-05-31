//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_population_manager.h"
#include "tf_populator_spawners.h"
#include "tf_obj_sentrygun.h"
#include "tf_tank_boss.h"
#include "tf_bot_squad.h"
#include "tf_team.h"
#include "bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.h"

extern ConVar tf_populator_debug;

ConVar tf_debug_placement_failure( "tf_debug_placement_failure", "0", FCVAR_CHEAT );

extern EventInfo *ParseEvent( KeyValues *data );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IPopulationSpawner *IPopulationSpawner::ParseSpawner( IPopulator *pPopulator, KeyValues *data )
{
	const char *pszKey = data->GetName();
	if ( !V_stricmp( pszKey, "TFBot" ) )
	{
		CTFBotSpawner *botSpawner = new CTFBotSpawner( pPopulator );
		if ( !botSpawner->Parse( data ) )
		{
			Warning( "Warning reading TFBot spawner definition\n" );
			delete botSpawner;
			return NULL;
		}

		return botSpawner;
	}
	else if ( !V_stricmp( pszKey, "Tank" ) )
	{
		CTankSpawner *tankSpawner = new CTankSpawner( pPopulator );
		if ( !tankSpawner->Parse( data ) )
		{
			Warning( "Warning reading Tank spawner definition\n" );
			delete tankSpawner;
			return NULL;
		}

		return tankSpawner;
	}
	else if ( !V_stricmp( pszKey, "SentryGun" ) )
	{
		CSentryGunSpawner *sentrySpawner = new CSentryGunSpawner( pPopulator );
		if ( !sentrySpawner->Parse( data ) )
		{
			Warning( "Warning reading SentryGun spawner definition\n" );
			delete sentrySpawner;
			return NULL;
		}

		return sentrySpawner;
	}
	else if ( !V_stricmp( pszKey, "Squad" ) )
	{
		CSquadSpawner *squadSpawner = new CSquadSpawner( pPopulator );
		if ( !squadSpawner->Parse( data ) )
		{
			Warning( "Warning reading Squad spawner definition\n" );
			delete squadSpawner;
			return NULL;
		}

		return squadSpawner;
	}
	else if ( !V_stricmp( pszKey, "Mob" ) )
	{
		CMobSpawner *mobSpawner = new CMobSpawner( pPopulator );
		if ( !mobSpawner->Parse( data ) )
		{
			Warning( "Warning reading Mob spawner definition\n" );
			delete mobSpawner;
			return NULL;
		}

		return mobSpawner;
	}
	else if ( !V_stricmp( pszKey, "RandomChoice" ) )
	{
		CRandomChoiceSpawner *randomSpawner = new CRandomChoiceSpawner( pPopulator );
		if ( !randomSpawner->Parse( data ) )
		{
			Warning( "Warning reading RandomChoice spawner definition\n" );
			delete randomSpawner;
			return NULL;
		}

		return randomSpawner;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ParseItemAttributes( CTFBot::EventChangeAttributes_t *event, KeyValues *data )
{
	char const *pszItemName = NULL;
	CUtlVector<static_attrib_t> parsedAttribs;
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		if ( !V_stricmp( pSubKey->GetName(), "ItemName" ) )
		{
			if ( pszItemName && pszItemName[0] )
				Warning( "TFBotSpawner: \"ItemName\" field specified multiple times ('%s' / '%s').\n", pszItemName, pSubKey->GetName() );

			pszItemName = pSubKey->GetName();
		}
		else
		{
			static_attrib_t attribute;
			if ( !attribute.BInitFromKV_SingleLine( pSubKey ) )
			{
				Warning( "TFBotSpawner: attribute error at '%s'\n", pSubKey->GetName() );
				continue;
			}

			parsedAttribs.AddToTail( attribute );
		}
	}

	if ( !pszItemName || pszItemName[0] == '\0' )
	{
		Warning( "TFBotSpawner: need to specify ItemName in ItemAttributes.\n" );
		return;
	}

	FOR_EACH_VEC( event->m_ItemAttrs, i )
	{
		if ( !V_stricmp( event->m_ItemAttrs[i].m_strItemName, pszItemName ) )
		{
			FOR_EACH_VEC( parsedAttribs, j )
			{
				FOR_EACH_VEC( event->m_ItemAttrs[i].m_Attrs, k )
				{
					if ( event->m_ItemAttrs[i].m_Attrs[k].iAttribIndex == parsedAttribs[j].iAttribIndex )
					{
						event->m_ItemAttrs[i].m_Attrs[k].value = parsedAttribs[j].value;

						break;
					}
				}

				event->m_ItemAttrs[i].m_Attrs.AddToTail( parsedAttribs[j] );
			}

			return;
		}
	}

	CTFBot::EventChangeAttributes_t::item_attributes_t itemAttributes;
	itemAttributes.m_strItemName = pszItemName;
	itemAttributes.m_Attrs.AddVectorToTail( parsedAttribs );

	event->m_ItemAttrs.AddToTail( itemAttributes );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void ParseCharacterAttributes( CTFBot::EventChangeAttributes_t *event, KeyValues *data )
{
	CUtlVector<static_attrib_t> parsedAttribs;
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		static_attrib_t attribute;
		if ( !attribute.BInitFromKV_SingleLine( pSubKey ) )
		{
			Warning( "TFBotSpawner: attribute error at '%s'\n", pSubKey->GetName() );
			continue;
		}

		parsedAttribs.AddToTail( attribute );
	}

	FOR_EACH_VEC( parsedAttribs, i )
	{
		bool bFoundExisting = false;

		FOR_EACH_VEC( event->m_CharAttrs, j )
		{
			if ( event->m_CharAttrs[j].iAttribIndex == parsedAttribs[i].iAttribIndex )
			{
				event->m_CharAttrs[j].value = parsedAttribs[i].value;

				bFoundExisting = true;
				break;
			}
		}

		if ( !bFoundExisting )
			event->m_CharAttrs.AddToTail( parsedAttribs[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool ParseDynamicAttributes( CTFBot::EventChangeAttributes_t *event, KeyValues *data )
{
	const char *pszKey = data->GetName();
	const char *pszValue = data->GetString();

	if ( !V_stricmp( pszKey, "Skill" ) )
	{
		if ( !V_stricmp( pszValue, "Easy" ) )
			event->m_iSkill = CTFBot::DifficultyType::EASY;
		else if ( !V_stricmp( pszValue, "Normal" ) )
			event->m_iSkill = CTFBot::DifficultyType::NORMAL;
		else if ( !V_stricmp( pszValue, "Hard" ) )
			event->m_iSkill = CTFBot::DifficultyType::HARD;
		else if ( !V_stricmp( pszValue, "Expert" ) )
			event->m_iSkill = CTFBot::DifficultyType::EXPERT;
		else
		{
			Warning( "TFBotSpawner: Invalid skill '%s'\n", pszValue );
			return false;
		}
	}
	else if ( !V_stricmp( pszKey, "WeaponRestriction" ) )
	{
		if ( !V_stricmp( pszValue, "MeleeOnly" ) )
			event->m_nWeaponRestrict = CTFBot::WeaponRestrictionType::MELEEONLY;
		else if ( !V_stricmp( pszValue, "PrimaryOnly" ) )
			event->m_nWeaponRestrict = CTFBot::WeaponRestrictionType::PRIMARYONLY;
		else if ( !V_stricmp( pszValue, "SecondaryOnly" ) )
			event->m_nWeaponRestrict = CTFBot::WeaponRestrictionType::SECONDARYONLY;
		else
		{
			Warning( "TFBotSpawner: Invalid weapon restriction '%s'\n", pszValue );
			return false;
		}
	}
	else if ( !V_stricmp( pszKey, "BehaviorModifiers" ) )
	{
		if ( !V_stricmp( pszValue, "Mobber" ) || !V_stricmp( pszValue, "Push" ) )
		{
			event->m_nBotAttrs |= CTFBot::AttributeType::AGGRESSIVE;
		}
		else
		{
			Warning( "TFBotSpawner: Invalid behavior modifier '%s'\n", pszValue );
			return false;
		}
	}
	else if ( !V_stricmp( pszKey, "Attributes" ) )
	{
		if ( !V_stricmp( pszValue, "RemoveOnDeath" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::REMOVEONDEATH;
		else if ( !V_stricmp( pszValue, "Aggressive" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::AGGRESSIVE;
		else if ( !V_stricmp( pszValue, "SuppressFire" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::SUPPRESSFIRE;
		else if ( !V_stricmp( pszValue, "DisableDodge" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::DISABLEDODGE;
		else if ( !V_stricmp( pszValue, "BecomeSpectatorOnDeath" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::BECOMESPECTATORONDEATH;
		else if ( !V_stricmp( pszValue, "RetainBuildings" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::RETAINBUILDINGS;
		else if ( !V_stricmp( pszValue, "SpawnWithFullCharge" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::SPAWNWITHFULLCHARGE;
		else if ( !V_stricmp( pszValue, "AlwaysCrit" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::ALWAYSCRIT;
		else if ( !V_stricmp( pszValue, "IgnoreEnemies" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::IGNOREENEMIES;
		else if ( !V_stricmp( pszValue, "HoldFireUntilFullReload" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::HOLDFIREUNTILFULLRELOAD;
		else if ( !V_stricmp( pszValue, "AlwaysFireWeapon" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::ALWAYSFIREWEAPON;
		else if ( !V_stricmp( pszValue, "TeleportToHint" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::TELEPORTTOHINT;
		else if ( !V_stricmp( pszValue, "MiniBoss" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::MINIBOSS;
		else if ( !V_stricmp( pszValue, "UseBossHealthBar" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::USEBOSSHEALTHBAR;
		else if ( !V_stricmp( pszValue, "IgnoreFlag" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::IGNOREFLAG;
		else if ( !V_stricmp( pszValue, "AutoJump" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::AUTOJUMP;
		else if ( !V_stricmp( pszValue, "AirChargeOnly" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::AIRCHARGEONLY;
		else if ( !V_stricmp( pszValue, "VaccinatorBullets" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::VACCINATORBULLETS;
		else if ( !V_stricmp( pszValue, "VaccinatorBlast" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::VACCINATORBLAST;
		else if ( !V_stricmp( pszValue, "VaccinatorFire" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::VACCINATORFIRE;
		else if ( !V_stricmp( pszValue, "BulletImmune" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::BULLETIMMUNE;
		else if ( !V_stricmp( pszValue, "BlastImmune" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::BLASTIMMUNE;
		else if ( !V_stricmp( pszValue, "FireImmune" ) )
			event->m_nBotAttrs |= CTFBot::AttributeType::FIREIMMUNE;
		else
		{
			Warning( "TFBotSpawner: Invalid attribute '%s'", pszValue );
			return false;
		}
	}
	else if ( !V_stricmp( pszKey, "MaxVisionRange" ) )
	{
		event->m_flVisionRange = data->GetFloat();
	}
	else if ( !V_stricmp( pszKey, "Item" ) )
	{
		event->m_ItemNames.CopyAndAddToTail( pszValue );
	}
	else if ( !V_stricmp( pszKey, "ItemAttributes" ) )
	{
		ParseItemAttributes( event, data );
	}
	else if ( !V_stricmp( pszKey, "CharacterAttributes" ) )
	{
		ParseCharacterAttributes( event, data );
	}
	else if ( !V_stricmp( pszKey, "Tag" ) )
	{
		event->m_Tags.CopyAndAddToTail( pszValue );
	}
	else
	{
		Warning( "TFBotSpawner: Unknown field '%s'\n", pszKey );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBotSpawner::CTFBotSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
	m_iHealth = -1;
	m_iszClassIcon = NULL_STRING;
	m_flScale = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::Parse( KeyValues *data )
{
	m_iClass = TF_CLASS_UNDEFINED;
	m_iszClassIcon = NULL_STRING;
	m_iHealth = -1;
	m_flScale = -1.0f;
	m_flAutoJumpMin = 0;
	m_flAutoJumpMax = 0;

	m_defaultAttributes.Reset();
	m_EventChangeAttributes.RemoveAll();

	KeyValues *pTemplte = data->FindKey( "Template" );
	if ( pTemplte )
	{
		KeyValues *pTemplateKV = m_pPopulator->GetManager()->GetTemplate( pTemplte->GetString() );
		if ( pTemplateKV && !Parse( pTemplateKV ) )
			return false;
		else
			Warning( "Unknown Template '%s' in TFBotSpawner definition\n", pTemplte->GetString() );
	}

	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetString();
		if ( !V_stricmp( pszKey, "Template" ) )
		{
		}
		else if ( !V_stricmp( pszKey, "Class" ) )
		{
			const char *pszValue = pSubKey->GetString();
			m_iClass = GetClassIndexFromString( pszValue );
			if ( m_iClass == TF_CLASS_UNDEFINED )
			{
				Warning( "TFBotSpawner: Invalid class %s", pszValue );
				return false;
			}

			if ( m_name.IsEmpty() )
				m_name = pszValue;
		}
		else if ( !V_stricmp( pszKey, "ClassIcon" ) )
		{
			m_iszClassIcon = AllocPooledString( pSubKey->GetString() );
		}
		else if ( !V_stricmp( pszKey, "Health" ) )
		{
			m_iHealth = pSubKey->GetInt();
		}
		else if ( !V_stricmp( pszKey, "Scale" ) )
		{
			m_flScale = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "Name" ) )
		{
			m_name = pSubKey->GetString();
		}
		else if ( !V_stricmp( pszKey, "TeleportWhere" ) )
		{
			m_TeleportWhere.CopyAndAddToTail( pSubKey->GetString() );
		}
		else if ( !V_stricmp( pszKey, "AutoJumpMin" ) )
		{
			m_flAutoJumpMin = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "AutoJumpMax" ) )
		{
			m_flAutoJumpMax = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "EventChangeAttributes" ) )
		{
			if ( !ParseEventChangeAttributes( pSubKey ) )
			{
				Warning( "TFBotSpawner: Failed to parse EventChangeAttributes\n" );
				return false;
			}
		}
		else if ( ParseDynamicAttributes( &m_defaultAttributes, pSubKey ) )
		{
		}
		else
		{
			Warning( "TFBotSpawner: Unknown field '%s'\n", pszKey );
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CTFNavArea *pArea = assert_cast<CTFNavArea *>( TheNavMesh->GetNavArea( vecPos ) );
	if ( pArea && pArea->HasTFAttributes( TF_NAV_NO_SPAWNING ) )
	{
		if ( tf_populator_debug.GetBool() )
			DevMsg( "CTFBotSpawner: %3.2f: *** Tried to spawn in a NO_SPAWNING area at (%f, %f, %f)\n", gpGlobals->curtime, VectorExpand(vecPos) );
		
		return false;
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
			return false;
	}

	float flZLevel = 0;
	Vector vecNewPos = vecPos;
	for ( ; flZLevel < StepHeight; flZLevel += 4.0 )
	{
		vecNewPos.z += flZLevel;
		if ( IsSpaceToSpawnHere( vecNewPos ) )
			break;
	}

	if ( flZLevel >= StepHeight )
	{
		if ( tf_populator_debug.GetBool() )
			DevMsg( "CTFBotSpawner: %3.2f: *** No space to spawn at (%f, %f, %f)\n", gpGlobals->curtime, VectorExpand(vecPos) );

		return false;
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_iClass == TF_CLASS_ENGINEER && ( m_defaultAttributes.m_nBotAttrs & CTFBot::AttributeType::TELEPORTTOHINT ) != CTFBot::AttributeType::NONE )
		{
			if ( !CTFBotMvMEngineerHintFinder::FindHint( true, false ) )
			{
				if ( tf_populator_debug.GetBool() )
					DevMsg( "CTFBotSpawner: %3.2f: *** No teleporter hint for engineer\n", gpGlobals->curtime );

				return false;
			}
		}
	}

	CTFBot *pSpawned = nullptr;

	CTeam *pSpec = GetGlobalTeam( TEAM_SPECTATOR );
	for ( int i = 0; i < pSpec->GetNumPlayers(); ++i )
	{
		if ( pSpec->GetPlayer( i )->IsBot() )
		{
			pSpawned = ToTFBot( pSpec->GetPlayer( i ) );
			pSpawned->ClearAllAttributes();

			break;
		}
	}

	if ( pSpawned == nullptr )
	{
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CUtlVector<CTFPlayer *> bots;
			CPopulationManager::CollectMvMBots( &bots );

			if ( bots.Count() >= k_nMvMBotTeamSize )
			{
				if( tf_populator_debug.GetBool() )
					DevMsg( "CTFBotSpawner: %3.2f: *** Can't spawn. Max number invaders already spawned.\n", gpGlobals->curtime );

				if ( bots.Count() != k_nMvMBotTeamSize )
				{
					int nNumToRemove = bots.Count() - k_nMvMBotTeamSize;
					CUtlVector<CTFPlayer *> kickCandidates;

					// TODO: What is this loop?
					for ( int i = 0; i < 2; ++i )
					{
						FOR_EACH_VEC( bots, j )
						{
							if ( kickCandidates.Count() >= nNumToRemove )
								break;

							if ( bots[j]->GetTeamNumber() == ( i == 1 ? TEAM_SPECTATOR : TF_TEAM_BLUE ) )
								kickCandidates.AddToTail( bots[j] );
						}
					}

					FOR_EACH_VEC( kickCandidates, i )
					{
						engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", kickCandidates[i]->GetUserID() ) );
					}
				}

				return false;
			}
		}
		else
		{

		}

		pSpawned = NextBotCreatePlayerBot<CTFBot>( "TFBot", false );
	}

	if ( pSpawned )
	{
		if ( !g_internalSpawnPoint )
		{
			g_internalSpawnPoint = (CPopulatorInternalSpawnPoint *)CreateEntityByName( "populator_internal_spawn_point" );
			g_internalSpawnPoint->Spawn();
		}

		g_internalSpawnPoint->SetAbsOrigin( vecNewPos );
		g_internalSpawnPoint->SetAbsAngles( vec3_angle );

		pSpawned->GetAttributeList()->RemoveAllAttributes();
		pSpawned->SetSpawnPoint( g_internalSpawnPoint );
		
		FOR_EACH_VEC( m_TeleportWhere, i )
		{
			pSpawned->m_TeleportWhere.CopyAndAddToTail( m_TeleportWhere[i] );
		}

		pSpawned->ChangeTeam( TFGameRules()->IsMannVsMachineMode() ? TF_TEAM_MVM_BOTS : TF_TEAM_RED, false, true );
		pSpawned->AllowInstantSpawn();

		pSpawned->HandleCommand_JoinClass( GetPlayerClassData( m_iClass )->m_szClassName );
		pSpawned->GetPlayerClass()->SetClassIconName( GetClassIcon() );

		engine->SetFakeClientConVarValue( pSpawned->edict(), "name", m_name.IsEmpty() ? "TFBot" : m_name.Get() );

		pSpawned->ClearEventChangeAttributes();
		FOR_EACH_VEC( m_EventChangeAttributes, i )
		{
			pSpawned->AddEventChangeAttributes( &m_EventChangeAttributes[i] );
		}

		if ( g_pPopulationManager->IsInEndlessWaves() )
		{
			g_pPopulationManager->EndlessSetAttributesForBot( pSpawned );
		}

		pSpawned->SetIsMiniBoss( HasAttribute( CTFBot::AttributeType::MINIBOSS ) );
		pSpawned->SetUseBossHealthBar( HasAttribute( CTFBot::AttributeType::USEBOSSHEALTHBAR ) );

		pSpawned->StartIdleSound();

		if ( HasAttribute( CTFBot::AttributeType::AUTOJUMP ) )
			pSpawned->SetAutoJumpIntervals( m_flAutoJumpMin, m_flAutoJumpMax );
		if ( HasAttribute( CTFBot::AttributeType::BULLETIMMUNE ) )
			pSpawned->m_Shared.AddCond( TF_COND_BULLET_IMMUNE );
		if ( HasAttribute( CTFBot::AttributeType::BLASTIMMUNE ) )
			pSpawned->m_Shared.AddCond( TF_COND_BLAST_IMMUNE );
		if ( HasAttribute( CTFBot::AttributeType::FIREIMMUNE ) )
			pSpawned->m_Shared.AddCond( TF_COND_FIRE_IMMUNE );

		pSpawned->m_flModelScaleOverride = m_flScale;
		pSpawned->SetModelScale( m_flScale > 0.0 ? m_flScale : 1.0f );

		int nHealth = m_iHealth;
		if ( nHealth <= 0 )
			nHealth = pSpawned->GetMaxHealth();

		nHealth *= g_pPopulationManager->GetHealthMultiplier( false );
		pSpawned->ModifyMaxHealth( nHealth );

		if ( TFGameRules()->IsMannVsMachineMode() && pSpawned->GetTeamNumber() == TF_TEAM_MVM_BOTS )
		{
			CMissionPopulator *pMission = dynamic_cast<CMissionPopulator *>( m_pPopulator );
			if ( pMission && ( pMission->m_eMission == CTFBot::MissionType::DESTROY_SENTRIES ) )
			{
				pSpawned->AddItem( "tw_sentrybuster" );
			}
			else
			{
				pSpawned->AddItem( g_szRomePromoItems_Hat[ m_iClass ] );
				pSpawned->AddItem( g_szRomePromoItems_Misc[ m_iClass ] );
			}
		}

		const CTFBot::EventChangeAttributes_t *pEventChangeAttributes = pSpawned->GetEventChangeAttributes( g_pPopulationManager->m_szDefaultEventChangeAttributesName );
		pSpawned->OnEventChangeAttributes( pEventChangeAttributes ? pEventChangeAttributes : &m_defaultAttributes );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBotSpawner::GetClass( int nSpawnNum )
{
	return m_iClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CTFBotSpawner::GetClassIcon( int nSpawnNum )
{
	if ( m_iszClassIcon != NULL_STRING )
		return m_iszClassIcon;

	return AllocPooledString( g_aRawPlayerClassNamesShort[m_iClass] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBotSpawner::GetHealth( int nSpawnNum )
{
	return m_iHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::IsMiniBoss( int nSpawnNum )
{
	return HasAttribute( CTFBot::AttributeType::MINIBOSS, nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum )
{
	return ( m_defaultAttributes.m_nBotAttrs & type ) != CTFBot::AttributeType::NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::HasEventChangeAttributes( const char *pszEventName ) const
{
	FOR_EACH_VEC( m_EventChangeAttributes, i )
	{
		if ( FStrEq( pszEventName, m_EventChangeAttributes[i].m_strName ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBotSpawner::ParseEventChangeAttributes( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();

		CTFBot::EventChangeAttributes_t &event = m_EventChangeAttributes[m_EventChangeAttributes.AddToTail()];
		event.m_strName = pszKey;

		FOR_EACH_SUBKEY( pSubKey, pEventKey )
		{
			if ( !ParseDynamicAttributes( &event, pEventKey ) )
			{
				Warning( "TFBotSpawner EventChangeAttributes: Failed to parse event '%s' with unknown attribute '%s'\n", pSubKey->GetName(), pEventKey->GetName() );
				return false;
			}
		}

		if ( !V_stricmp( pszKey, "default" ) )
			m_defaultAttributes = event;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTankSpawner::CTankSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
	m_iHealth = 50000;
	m_flSpeed = 75.0f;
	m_szName = "Tank";
	m_onKilledEvent = NULL;
	m_onBombDroppedEvent = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTankSpawner::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "Health" ) )
			m_iHealth = pSubKey->GetInt();
		else if ( !V_stricmp( pszKey, "Speed" ) )
			m_flSpeed = pSubKey->GetFloat();
		else if ( !V_stricmp( pszKey, "Name" ) )
			m_szName = pSubKey->GetString();
		else if ( !V_stricmp( pszKey, "Skin" ) )
			m_nSkin = pSubKey->GetInt();
		else if ( !V_stricmp( pszKey, "StartingPathTrackNode" ) )
			m_szStartingPathTrackNodeName = pSubKey->GetName();
		else if ( !V_stricmp( pszKey, "OnKilledOutput" ) )
			m_onKilledEvent = ParseEvent( pSubKey );
		else if ( !V_stricmp( pszKey, "OnBombDroppedOutput" ) )
			m_onBombDroppedEvent = ParseEvent( pSubKey );
		else
		{
			Warning( "Invalid attribute '%s' in Tank definition\n", pszKey );
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTankSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	CTFTankBoss *pTank = (CTFTankBoss *)CreateEntityByName( "tank_boss" );
	if ( pTank == NULL )
		return false;

	pTank->SetAbsOrigin( vecPos );
	pTank->SetAbsAngles( vec3_angle );
	pTank->SetInitialHealth( m_iHealth * g_pPopulationManager->GetHealthMultiplier( true ) );
	pTank->SetMaxSpeed( m_flSpeed );
	pTank->SetName( MAKE_STRING( m_szName ) );
	pTank->SetSkin( m_nSkin );
	pTank->SetStartingPathTrackNode( m_szStartingPathTrackNodeName.GetForModify() );
	pTank->Spawn();
	pTank->SetOnKilledEvent( m_onKilledEvent );
	pTank->SetOnBombDroppedEvent( m_onBombDroppedEvent );

	if ( pOutVec )
		pOutVec->AddToTail( pTank );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSentryGunSpawner::CSentryGunSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSentryGunSpawner::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "Level" ) )
		{
			m_nLevel = pSubKey->GetInt();
		}
		else
		{
			Warning( "Invalid attribute '%s' in SentryGun definition\n", pszKey );
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSentryGunSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	CObjectSentrygun *pSentry = (CObjectSentrygun *)CreateEntityByName( "obj_sentrygun" );
	if ( pSentry == NULL )
	{
		if ( tf_populator_debug.GetBool() )
			DevMsg( "CSentryGunSpawner: %3.2f: Failed to create obj_sentrygun\n", gpGlobals->curtime );

		return false;
	}

	pSentry->SetAbsOrigin( vecPos );
	pSentry->SetAbsAngles( vec3_angle );
	pSentry->Spawn();
	pSentry->ChangeTeam( TF_TEAM_RED );
	pSentry->SetDefaultUpgradeLevel( m_nLevel + 1 );
	pSentry->InitializeMapPlacedObject();

	if ( pOutVec )
		pOutVec->AddToTail( pSentry );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSquadSpawner::CSquadSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
	m_flFormationSize = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSquadSpawner::~CSquadSpawner()
{
	m_SquadSpawners.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSquadSpawner::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		char const *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "FormationSize" ) )
		{
			m_flFormationSize = pSubKey->GetFloat();
		}
		else if ( !V_stricmp( pszKey, "ShouldPreserveSquad" ) )
		{
			m_bShouldPreserveSquad = pSubKey->GetBool();
		}
		else
		{
			IPopulationSpawner *pSpawner = IPopulationSpawner::ParseSpawner( m_pPopulator, pSubKey );
			if ( pSpawner )
			{
				m_SquadSpawners.AddToTail( pSpawner );
			}
			else
			{
				Warning( "Unknown attribute '%s' in Mob definition.\n", pszKey );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSquadSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "CSquadSpawner: %3.2f: <<<< Spawning Squad >>>>\n", gpGlobals->curtime );
	}

	bool bFullySpawned = false;
	CUtlVector<CHandle<CBaseEntity>> spawnedMembers;
	FOR_EACH_VEC( m_SquadSpawners, i )
	{
		if ( !m_SquadSpawners[i]->Spawn( vecPos, &spawnedMembers ) )
		{
			bFullySpawned = false;
			break;
		}
	}

	if ( !bFullySpawned )
	{
		if ( tf_populator_debug.GetBool() )
		{
			DevMsg( "%3.2f: CSquadSpawner: Unable to spawn entire squad\n", gpGlobals->curtime );
		}

		FOR_EACH_VEC( spawnedMembers, i )
		{
			UTIL_Remove( spawnedMembers[i] );
		}

		return false;
	}

	CTFBotSquad *pSquad = new CTFBotSquad();
	if ( pSquad )
	{
		pSquad->SetFormationSize( m_flFormationSize );
		pSquad->SetShouldPreserveSquad( m_bShouldPreserveSquad );

		FOR_EACH_VEC( spawnedMembers, i )
		{
			CTFBot *pBot = ToTFBot( spawnedMembers[i] );
			if ( pBot )
			{
				pBot->JoinSquad( pSquad );
			}
		}
	}

	if ( pOutVec )
		pOutVec->AddVectorToTail( spawnedMembers );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSquadSpawner::GetClass( int nSpawnNum )
{
	if ( nSpawnNum < 0 || m_SquadSpawners.IsEmpty() )
		return TF_CLASS_UNDEFINED;

	int nWhich = nSpawnNum % m_SquadSpawners.Count();
	if ( m_SquadSpawners[nWhich]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return TF_CLASS_UNDEFINED;
	}

	return m_SquadSpawners[nWhich]->GetClass();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CSquadSpawner::GetClassIcon( int nSpawnNum )
{
	if ( nSpawnNum < 0 || m_SquadSpawners.IsEmpty() )
		return NULL_STRING;

	int nWhich = nSpawnNum % m_SquadSpawners.Count();
	if ( m_SquadSpawners[nWhich]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return NULL_STRING;
	}

	return m_SquadSpawners[nWhich]->GetClassIcon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSquadSpawner::GetHealth( int nSpawnNum )
{
	if ( nSpawnNum < 0 || m_SquadSpawners.IsEmpty() )
		return 0;

	int nWhich = nSpawnNum % m_SquadSpawners.Count();
	if ( m_SquadSpawners[nWhich]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return 0;
	}

	return m_SquadSpawners[nWhich]->GetHealth();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSquadSpawner::IsMiniBoss( int nSpawnNum )
{
	if ( nSpawnNum < 0 || m_SquadSpawners.IsEmpty() )
		return false;

	int nWhich = nSpawnNum % m_SquadSpawners.Count();
	if ( m_SquadSpawners[nWhich]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}

	return m_SquadSpawners[nWhich]->IsMiniBoss();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSquadSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum )
{
	if ( nSpawnNum < 0 || m_SquadSpawners.IsEmpty() )
		return false;

	int nWhich = nSpawnNum % m_SquadSpawners.Count();
	if ( m_SquadSpawners[nWhich]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}

	return m_SquadSpawners[nWhich]->HasAttribute( type );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSquadSpawner::HasEventChangeAttributes( const char *pszEventName ) const
{
	bool bHasEventChangeAttributes = false;
	FOR_EACH_VEC( m_SquadSpawners, i )
	{
		bHasEventChangeAttributes |= m_SquadSpawners[i]->HasEventChangeAttributes( pszEventName );
	}

	return bHasEventChangeAttributes;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMobSpawner::CMobSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
	m_pSpawner = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMobSpawner::~CMobSpawner()
{
	if ( m_pSpawner )
	{
		delete m_pSpawner;
		m_pSpawner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMobSpawner::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		char const *pszKey = pSubKey->GetName();
		if ( !V_stricmp( pszKey, "Count" ) )
		{
			m_nSpawnCount = pSubKey->GetInt();
		}
		else
		{
			IPopulationSpawner *pSpawner = IPopulationSpawner::ParseSpawner( m_pPopulator, pSubKey );
			if ( pSpawner )
			{
				if ( m_pSpawner == NULL )
					m_pSpawner = pSpawner;
				else
					Warning( "CMobSpawner: Duplicate spawner encountered - discarding!\n" );
			}
			else
			{
				Warning( "Unknown attribute '%s' in Mob definition.\n", pszKey );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMobSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	if ( !m_pSpawner )
		return false;

	int nNumSpawned = 0;
	if ( m_nSpawnCount > 0 )
	{
		while ( m_pSpawner->Spawn( vecPos, pOutVec ) )
		{
			nNumSpawned++;

			if ( m_nSpawnCount <= nNumSpawned )
				return true;
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CMobSpawner::HasEventChangeAttributes( const char *pszEventName ) const
{
	if ( m_pSpawner )
		return m_pSpawner->HasEventChangeAttributes( pszEventName );

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRandomChoiceSpawner::CRandomChoiceSpawner( IPopulator *pPopulator )
	: m_pPopulator( pPopulator )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRandomChoiceSpawner::~CRandomChoiceSpawner()
{
	m_Spawners.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomChoiceSpawner::Parse( KeyValues *data )
{
	FOR_EACH_SUBKEY( data, pSubKey )
	{
		const char *pszKey = pSubKey->GetName();
		IPopulationSpawner *pSawner = IPopulationSpawner::ParseSpawner( m_pPopulator, pSubKey );
		if ( pSawner )
			m_Spawners.AddToTail( pSawner );
		else
			Warning( "Unknown attribute '%s' in RandomChoice definition.\n", pszKey );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomChoiceSpawner::Spawn( const Vector &vecPos, CUtlVector<CHandle<CBaseEntity>> *pOutVec )
{
	if ( m_Spawners.IsEmpty() )
		return false;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( m_nNumSpawned + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < m_nNumSpawned + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	return m_Spawners[ m_RandomPicks[ m_nNumSpawned++ ] ]->Spawn( vecPos, pOutVec );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRandomChoiceSpawner::GetClass( int nSpawnNum )
{
	if ( m_Spawners.IsEmpty() )
		return TF_CLASS_UNDEFINED;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	if ( m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return TF_CLASS_UNDEFINED;
	}

	return m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->GetClass( nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CRandomChoiceSpawner::GetClassIcon( int nSpawnNum )
{
	if ( m_Spawners.IsEmpty() )
		return NULL_STRING;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	if ( m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return NULL_STRING;
	}

	return m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->GetClassIcon( nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRandomChoiceSpawner::GetHealth( int nSpawnNum )
{
	if ( m_Spawners.IsEmpty() )
		return 0;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	if ( m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return 0;
	}

	return m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->GetHealth( nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomChoiceSpawner::IsMiniBoss( int nSpawnNum )
{
	if ( m_Spawners.IsEmpty() )
		return false;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	if ( m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}

	return m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsMiniBoss( nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomChoiceSpawner::HasAttribute( CTFBot::AttributeType type, int nSpawnNum )
{
	if ( m_Spawners.IsEmpty() )
		return false;

	int nCurrentCount = m_RandomPicks.Count();
	int nNumToAdd = ( nSpawnNum + 1 ) - nCurrentCount;

	if ( nNumToAdd > 0 )
	{
		m_RandomPicks.AddMultipleToTail( nNumToAdd );

		for ( int i = nCurrentCount; i < nSpawnNum + 1; ++i )
		{
			m_RandomPicks[i] = RandomInt( 0, m_Spawners.Count() - 1 );
		}
	}

	if ( m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->IsVarious() )
	{
		DevWarning( "Nested complex spawner types... need a method for counting these." );
		return false;
	}

	return m_Spawners[ m_RandomPicks[ nSpawnNum ] ]->HasAttribute( type, nSpawnNum );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRandomChoiceSpawner::HasEventChangeAttributes( const char *pszEventName ) const
{
	bool bHasEventChangeAttributes = false;
	FOR_EACH_VEC( m_Spawners, i )
	{
		bHasEventChangeAttributes |= m_Spawners[i]->HasEventChangeAttributes( pszEventName );
	}

	return bHasEventChangeAttributes;
}
