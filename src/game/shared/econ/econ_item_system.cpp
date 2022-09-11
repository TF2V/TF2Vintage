#include "cbase.h"
#include "filesystem.h"
#include "inetchannel.h"
#include "econ_networking.h"
#include "econ_item_system.h"
#include "script_parser.h"
#include "activitylist.h"
#include "vscript_shared.h"
#ifndef NO_STEAM
#include "steam/steamtypes.h"
#include "steam/isteamhttp.h"
#endif
#include "tier0/icommandline.h"
#include "econ_networking_messages.h"
#include "lzma/lzma.h"

#if defined(CLIENT_DLL)
#include "hud_macros.h"
#define UTIL_VarArgs  VarArgs
#endif

const char *g_TeamVisualSections[TF_TEAM_VISUALS_COUNT] =
{
	"visuals",			// TEAM_UNASSIGNED
	"",					// TEAM_SPECTATOR
	"visuals_red",		// TEAM_RED
	"visuals_blu",		// TEAM_BLUE
	"visuals_grn",		// TEAM_GREEN
	"visuals_ylw",		// TEAM_YELLOW
	"visuals_mvm_boss"	// ???
};

const char *g_WearableAnimTypeStrings[NUM_WEARABLEANIM_TYPES] =
{
	"on_spawn",			// WAP_ON_SPAWN,
	"start_building",	// WAP_START_BUILDING,
	"stop_building",	// WAP_STOP_BUILDING,
	"start_taunting",		// WAP_START_TAUNTING,
	"stop_taunting",	// WAP_STOP_TAUNTING,
};

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",
	"value_is_inverted_percentage",
	"value_is_additive",
	"value_is_additive_percentage",
	"value_is_or",
	"value_is_date",
	"value_is_account_id",
	"value_is_particle_index",
	"value_is_killstreakeffect_index",
	"value_is_killstreak_idleeffect_index",
	"value_is_item_def",
	"value_is_from_lookup_table"
};

const char *g_EffectTypes[] =
{
	"unusual",
	"strange",
	"neutral",
	"positive",
	"negative"
};

const char *g_szQualityStrings[] =
{
	"normal",
	"rarity1",
	"rarity2",
	"vintage",
	"rarity3",
	"rarity4",
	"unique",
	"community",
	"developer",
	"selfmade",
	"customized",
	"strange",
	"completed",
	"haunted",
	"collectors",
	"paintkitWeapon",
};

const char *g_szQualityColorStrings[] =
{
	"QualityColorNormal",
	"QualityColorrarity1",
	"QualityColorrarity2",
	"QualityColorVintage",
	"QualityColorrarity3",
	"QualityColorrarity4",
	"QualityColorUnique",
	"QualityColorCommunity",
	"QualityColorDeveloper",
	"QualityColorSelfMade",
	"QualityColorSelfMadeCustomized",
	"QualityColorStrange",
	"QualityColorCompleted",
	"QualityColorHaunted",
	"QualityColorCollectors",
	"QualityColorPaintkitWeapon",
};

const char *g_szQualityLocalizationStrings[] =
{
	"#Normal",
	"#rarity1",
	"#rarity2",
	"#vintage",
	"#rarity3",
	"#rarity4",
	"#unique",
	"#community",
	"#developer",
	"#selfmade",
	"#customized",
	"#strange",
	"#completed",
	"#haunted",
	"#collectors",
	"#paintkitWeapon",
};

const char *g_szDropTypeStrings[] =
{
	"",
	"none",
	"drop",
	"break",
};

int GetTeamVisualsFromString( const char *pszString )
{
	for ( int i = 0; i < ARRAYSIZE( g_TeamVisualSections ); i++ )
	{
		// There's a NULL hidden in g_TeamVisualSections
		if ( g_TeamVisualSections[i] && !V_stricmp( pszString, g_TeamVisualSections[i] ) )
			return i;
	}
	return -1;
}

#define ITEMS_GAME		"scripts/items/items_game.txt"


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static CEconItemSchema g_EconItemSchema;
CEconItemSchema *GetItemSchema()
{
	return &g_EconItemSchema;
}

#define GET_STRING(copyto, from, name)													\
		if (from->GetString(#name, NULL))												\
			(copyto)->name = from->GetString(#name)

#define GET_STRING_DEFAULT(copyto, from, name, defaultstring) \
		(copyto)->name = from->GetString(#name, defaultstring)

#define GET_BOOL(copyto, from, name) \
		(copyto)->name = from->GetBool(#name, (copyto)->name)

#define GET_FLOAT(copyto, from, name) \
		(copyto)->name = from->GetFloat(#name, (copyto)->name)

#define GET_INT(copyto, from, name) \
		(copyto)->name = from->GetInt(#name, (copyto)->name)

#define FIND_ELEMENT(map, key, val)						\
		unsigned int index = map.Find(key);			\
		if (index != map.InvalidIndex())				\
			val = map.Element(index)				

#define FIND_ELEMENT_STRING(map, key, val)						\
		unsigned int index = map.Find(key);					\
		if (index != map.InvalidIndex())						\
			Q_snprintf(val, sizeof(val), map.Element(index))

#define IF_ELEMENT_FOUND(map, key)						\
		unsigned int index = map.Find(key);			\
		if (index != map.InvalidIndex())			

#define GET_VALUES_FAST_BOOL(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				dict.Element(index) = pKeyData->GetBool();	\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), pKeyData->GetBool());\
			}												\
		}


#define GET_VALUES_FAST_STRING(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())	\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				Q_strncpy((char*)dict.Element(index), pKeyData->GetString(), 196);\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), strdup(pKeyData->GetString()));\
			}												\
		}	

// Recursively inherit keys from parent KeyValues
void InheritKVRec( KeyValues *pFrom, KeyValues *pTo )
{
	for ( KeyValues *pSubData = pFrom->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
	{
		switch ( pSubData->GetDataType() )
		{
			// Identifies the start of a subsection
			case KeyValues::TYPE_NONE:
			{
				KeyValues *pKey = pTo->FindKey( pSubData->GetName() );
				if ( pKey == NULL )
				{
					pKey = pTo->CreateNewKey();
					pKey->SetName( pSubData->GetName() );
				}

				InheritKVRec( pSubData, pKey );
				break;
			}
			// Actual types
			case KeyValues::TYPE_STRING:
			{
				pTo->SetString( pSubData->GetName(), pSubData->GetString() );
				break;
			}
			case KeyValues::TYPE_INT:
			{
				pTo->SetInt( pSubData->GetName(), pSubData->GetInt() );
				break;
			}
			case KeyValues::TYPE_FLOAT:
			{
				pTo->SetFloat( pSubData->GetName(), pSubData->GetFloat() );
				break;
			}
			case KeyValues::TYPE_WSTRING:
			{
				pTo->SetWString( pSubData->GetName(), pSubData->GetWString() );
				break;
			}
			case KeyValues::TYPE_COLOR:
			{
				pTo->SetColor( pSubData->GetName(), pSubData->GetColor() );
				break;
			}
			case KeyValues::TYPE_UINT64:
			{
				pTo->SetUint64( pSubData->GetName(), pSubData->GetUint64() );
				break;
			}
			default:
				break;
		}
	}
}


bool CEconItemDefinition::LoadFromKV( KeyValues *pDefinition )
{
	definition = pDefinition;

	GET_STRING( this, pDefinition, name );
	GET_BOOL( this, pDefinition, show_in_armory );

	GET_STRING( this, pDefinition, item_class );
	GET_STRING( this, pDefinition, item_name );
	GET_STRING( this, pDefinition, item_description );
	GET_STRING( this, pDefinition, item_type_name );

	const char *pszQuality = pDefinition->GetString( "item_quality" );
	if ( pszQuality[0] )
	{
		int iQuality = UTIL_StringFieldToInt( pszQuality, g_szQualityStrings, ARRAYSIZE( g_szQualityStrings ) );
		if ( iQuality != -1 )
		{
			item_quality = iQuality;
		}
	}

	GET_STRING( this, pDefinition, item_logname );
	GET_STRING( this, pDefinition, item_iconname );

	const char *pszLoadoutSlot = pDefinition->GetString( "item_slot" );

	if ( pszLoadoutSlot[0] )
	{
		item_slot = UTIL_StringFieldToInt( pszLoadoutSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
	}

	const char *pszAnimSlot = pDefinition->GetString( "anim_slot" );
	if ( pszAnimSlot[0] )
	{
		if ( V_strcmp( pszAnimSlot, "FORCE_NOT_USED" ) != 0 )
		{
			anim_slot = UTIL_StringFieldToInt( pszAnimSlot, g_AnimSlots, TF_WPN_TYPE_COUNT );
		}
		else
		{
			anim_slot = -2;
		}
	}

	GET_BOOL( this, pDefinition, baseitem );
	GET_INT( this, pDefinition, min_ilevel );
	GET_INT( this, pDefinition, max_ilevel );
	GET_BOOL( this, pDefinition, loadondemand );

	GET_STRING( this, pDefinition, image_inventory );
	GET_INT( this, pDefinition, image_inventory_size_w );
	GET_INT( this, pDefinition, image_inventory_size_h );

	GET_STRING( this, pDefinition, model_player );
	GET_STRING( this, pDefinition, model_vision_filtered );
	GET_STRING( this, pDefinition, model_world );
	GET_STRING( this, pDefinition, extra_wearable );
	GET_STRING( this, pDefinition, extra_wearable_vm );

	GET_INT( this, pDefinition, attach_to_hands );
	GET_INT( this, pDefinition, attach_to_hands_vm_only );
	GET_BOOL( this, pDefinition, act_as_wearable );
	GET_BOOL( this, pDefinition, act_as_weapon );
	GET_INT( this, pDefinition, hide_bodygroups_deployed_only );

	GET_BOOL( this, pDefinition, is_reskin );
	GET_BOOL( this, pDefinition, specialitem );
	GET_BOOL( this, pDefinition, demoknight );
	GET_STRING( this, pDefinition, holiday_restriction );
	GET_INT( this, pDefinition, year );
	GET_BOOL( this, pDefinition, is_custom_content );
	GET_BOOL( this, pDefinition, is_cut_content );
	GET_BOOL( this, pDefinition, is_multiclass_item );

	GET_STRING( this, pDefinition, item_script );

	const char *pszDropType = pDefinition->GetString("drop_type");
	if ( pszDropType[0] )
	{
		int iDropType = UTIL_StringFieldToInt( pszDropType, g_szDropTypeStrings, ARRAYSIZE( g_szDropTypeStrings ) );
		if (iDropType != -1)
		{
			drop_type = iDropType;
		}
	}

	// Initialize all visuals for next section.
	for (int i = 0; i < TF_TEAM_VISUALS_COUNT; i++)
	{
		visual[i] = NULL;
	}

	FOR_EACH_SUBKEY( pDefinition, pSubData )
	{
		if ( !V_stricmp( pSubData->GetName(), "capabilities" ) )
		{
			GET_VALUES_FAST_BOOL( capabilities, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "tags" ) )
		{
			GET_VALUES_FAST_BOOL( tags, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "model_player_per_class" ) )
		{
			FOR_EACH_SUBKEY( pSubData, pClassData )
			{
				const char *pszClass = pClassData->GetName();

				if ( !V_stricmp( pszClass, "basename" ) )
				{
					// Generic item, assign a model for every class.
					for ( int i = TF_FIRST_NORMAL_CLASS; i <= TF_LAST_NORMAL_CLASS; i++ )
					{
						// Add to the player model per class.
						model_player_per_class[i] = UTIL_VarArgs( pClassData->GetString(), g_aRawPlayerClassNamesShort[i], g_aRawPlayerClassNamesShort[i], g_aRawPlayerClassNamesShort[i] );
					}
				}
				else
				{
					// Check the class this item is for and assign it to them.
					int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						// Valid class, assign to it.
						model_player_per_class[iClass] = pClassData->GetString();
					}
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "used_by_classes" ) )
		{
			FOR_EACH_SUBKEY( pSubData, pClassData )
			{
				const char *pszClass = pClassData->GetName();
				int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );

				if ( iClass != -1 )
				{
					used_by_classes |= ( 1 << iClass );
					const char *pszSlotname = pClassData->GetString();

					if ( pszSlotname[0] != '1' )
					{
						int iSlot = UTIL_StringFieldToInt( pszSlotname, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );

						if ( iSlot != -1 )
							item_slot_per_class[iClass] = iSlot;
					}
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "attributes" ) )
		{
			FOR_EACH_TRUE_SUBKEY( pSubData, pAttribData )
			{
				static_attrib_t attribute;
				if ( !attribute.BInitFromKV_MultiLine( pAttribData ) )
					continue;

				attributes.AddToTail( attribute );
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "static_attrs" ) )
		{
			FOR_EACH_SUBKEY( pSubData, pAttribData )
			{
				static_attrib_t attribute;
				if ( !attribute.BInitFromKV_SingleLine( pAttribData ) )
					continue;

				attributes.AddToTail( attribute );
			}
		}
		else if (!V_strnicmp(pSubData->GetName(), "visuals", 7))
		{
			// Figure out what team is this meant for.
			int iVisuals = UTIL_StringFieldToInt(pSubData->GetName(), g_TeamVisualSections, TF_TEAM_VISUALS_COUNT);
			if (iVisuals == TEAM_UNASSIGNED)
			{
				// Hacky: for standard visuals block, assign it to all teams at once.
				for (int team = 0; team < TF_TEAM_VISUALS_COUNT; team++)
				{
					if (team == TEAM_SPECTATOR)
						continue;

					ParseVisuals(pSubData, team);
				}
			}
			else if (iVisuals != -1 && iVisuals != TEAM_SPECTATOR)
			{
				ParseVisuals(pSubData, iVisuals);
			}
		}
	}

	return true;
}

void CEconItemDefinition::ParseVisuals( KeyValues *pKVData, int iIndex )
{

	PerTeamVisuals_t* pVisuals;
	if (visual[iIndex]) // If we have data already in here, load the previous data before adding new stuff.
		pVisuals = visual[iIndex];
	else
		pVisuals = new PerTeamVisuals_t; // Build ourselves a fresh visual file.

	FOR_EACH_SUBKEY( pKVData, pVisualData )
	{
		if ( !V_stricmp( pVisualData->GetName(), "use_visualsblock_as_base" ) )
		{
			const char *pszBlockTeamName = pVisualData->GetString();
			int iTeam = GetTeamVisualsFromString( pszBlockTeamName );
			if ( iTeam != -1 )
			{
				*pVisuals = *GetVisuals( iTeam );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "vm_bodygroup_override" ) )
			{
				Warning( "Unknown visuals block: %s", pszBlockTeamName );
			}
		}
		else if ( !V_stricmp( pVisualData->GetName(), "player_bodygroups" ) )
		{
			GET_VALUES_FAST_BOOL( pVisuals->player_bodygroups, pVisualData );
		}
		else if ( !V_stricmp( pVisualData->GetName(), "attached_models" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pAttachData )
			{
				AttachedModel_t attached_model;
				attached_model.model_display_flags = pAttachData->GetInt( "model_display_flags", AM_VIEWMODEL|AM_WORLDMODEL );
				V_strcpy_safe( attached_model.model, pAttachData->GetString( "model" ) );

				pVisuals->attached_models.AddToTail( attached_model );
			}
		}
		else if ( !V_stricmp( pVisualData->GetName(), "custom_particlesystem" ) )
		{
			pVisuals->custom_particlesystem = pVisualData->GetString( "system" );
		}
		else if ( !V_stricmp( pVisualData->GetName(), "muzzle_flash" ) )
		{
			// Fetching this similar to weapon script file parsing.
			pVisuals->muzzle_flash = pVisualData->GetString();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "tracer_effect" ) )
		{
			// Fetching this similar to weapon script file parsing.
			pVisuals->tracer_effect = pVisualData->GetString();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "animation_replacement" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pAnimData )
			{
				ActivityReplacement_t *override = new ActivityReplacement_t;
				override->pszActivity = pAnimData->GetName();
				override->pszReplacement = pAnimData->GetString();

				pVisuals->animation_replacement.AddToTail( override );
			}
		}
		else if ( !V_stricmp( pVisualData->GetName(), "playback_activity" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pActivityData )
			{
				int iPlaybackType = UTIL_StringFieldToInt( pActivityData->GetName(), g_WearableAnimTypeStrings, NUM_WEARABLEANIM_TYPES );
				if ( iPlaybackType != -1 )
				{
					activity_on_wearable_t activity;
					activity.playback = (wearableanimplayback_t)iPlaybackType;
					activity.activity = kActivityLookup_Unknown;
					activity.activity_name = pActivityData->GetString();

					pVisuals->playback_activity.AddToTail( activity );
				}
			}
		}
		else if ( !V_strnicmp( pVisualData->GetName(), "custom_sound", 12 ) )
		{
			int iSound = 0;
			if ( pVisualData->GetName()[12] )
			{
				iSound = Clamp( V_atoi( pVisualData->GetName() + 12 ), 0, MAX_CUSTOM_WEAPON_SOUNDS - 1 );
			}

			pVisuals->aCustomWeaponSounds[ iSound ] = pVisualData->GetString();
		}
		else if ( !V_strnicmp( pVisualData->GetName(), "sound_", 6 ) )
		{
			// Advancing pointer past sound_ prefix... why couldn't they just make a subsection for sounds?
			int iSound = GetWeaponSoundFromString( pVisualData->GetName() + 6 );

			if ( iSound != -1 )
			{
				pVisuals->aWeaponSounds[ iSound ] = pVisualData->GetString();
			}
		}
		else if ( !V_stricmp( pVisualData->GetName(), "styles" ) )
		{
			FOR_EACH_SUBKEY( pVisualData, pStyleData )
			{
				ItemStyle_t *style = new ItemStyle_t;

				GET_STRING( style, pStyleData, name );
				GET_STRING( style, pStyleData, model_player );
				GET_STRING( style, pStyleData, image_inventory );
				GET_BOOL( style, pStyleData, selectable );
				GET_INT( style, pStyleData, skin_red );
				GET_INT( style, pStyleData, skin_blu );

				FOR_EACH_SUBKEY( pStyleData, pStyleModelData )
				{
					if ( !V_stricmp( pStyleModelData->GetName(), "model_player_per_class" ) )
					{
						GET_VALUES_FAST_STRING( style->model_player_per_class, pStyleModelData );
					}
				}

				pVisuals->styles.AddToTail( style );
			}
		}
		else if ( !V_stricmp( pVisualData->GetName(), "skin" ) )
		{
			pVisuals->skin = pVisualData->GetInt();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "use_per_class_bodygroups" ) )
		{
			pVisuals->use_per_class_bodygroups = pVisualData->GetInt();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "material_override" ) )
		{
			pVisuals->material_override = pVisualData->GetString();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "vm_bodygroup_override" ) )
		{
			pVisuals->vm_bodygroup_override = pVisualData->GetInt();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "vm_bodygroup_state_override" ) )
		{
			pVisuals->vm_bodygroup_state_override = pVisualData->GetInt();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "wm_bodygroup_override" ) )
		{
			pVisuals->wm_bodygroup_override = pVisualData->GetInt();
		}
		else if ( !V_stricmp( pVisualData->GetName(), "wm_bodygroup_state_override" ) )
		{
			pVisuals->wm_bodygroup_state_override = pVisualData->GetInt();
		}
	}

	visual[ iIndex ] = pVisuals;
}

bool CEconAttributeDefinition::LoadFromKV( KeyValues *pDefinition )
{
	definition = pDefinition;

	GET_STRING_DEFAULT( this, pDefinition, name, "( unnamed )" );
	GET_STRING( this, pDefinition, attribute_class );
	GET_STRING( this, pDefinition, description_string );

	string_attribute = ( V_stricmp( pDefinition->GetString( "attribute_type" ), "string" ) == 0 );

	const char *szFormat = pDefinition->GetString( "description_format" );
	description_format = UTIL_StringFieldToInt( szFormat, g_AttributeDescriptionFormats, ARRAYSIZE( g_AttributeDescriptionFormats ) );

	const char *szEffect = pDefinition->GetString( "effect_type" );
	effect_type = UTIL_StringFieldToInt( szEffect, g_EffectTypes, ARRAYSIZE( g_EffectTypes ) );

	const char *szType = pDefinition->GetString( "attribute_type" );
	type = GetItemSchema()->GetAttributeType( szType );

	GET_BOOL( this, pDefinition, hidden );
	GET_BOOL( this, pDefinition, stored_as_integer );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename T>
class ISchemaAttributeTypeBase : public ISchemaAttributeType
{
public:
	virtual void InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			new( &pValue->iVal ) T;
		else if ( sizeof( T ) <= sizeof( void * ) )
			new( &pValue->bVal ) T;
		else
			pValue->bVal = (byte *)new T;
	}

	virtual void UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			reinterpret_cast<T *>( &pValue->iVal )->~T();
		else if ( sizeof( T ) <= sizeof( void * ) )
			reinterpret_cast<T *>( &pValue->bVal )->~T();
		else
			delete pValue->bVal;
	}

	virtual bool OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
	{
		return pIterator->OnIterateAttributeValue( pAttrDef, GetTypedDataFromValue( value ) );
	}

private:
	T const &GetTypedDataFromValue( const attrib_data_union_t &value ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			return *reinterpret_cast<const T *>( &value.iVal );

		if ( sizeof( T ) <= sizeof( void* ) )
			return *reinterpret_cast<const T *>( &value.bVal );

		Assert( value.bVal );
		return *reinterpret_cast<const T *>( value.bVal );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Default : public ISchemaAttributeTypeBase<unsigned int>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		if ( bUnk )
		{
			double val = 0.0;
			if ( pString && pString[0] )
				val = strtod( pString, NULL );

			pValue->flVal = val;
		}
		else if ( pAttrDef->stored_as_integer )
		{
			pValue->iVal = V_atoi64( pString );
		}
		else
		{
			pValue->flVal = V_atof( pString );
		}

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		if ( pAttrDef->stored_as_integer )
		{
			V_strcpy( pString, UTIL_VarArgs( "%u", value.iVal ) );
		}
		else
		{
			V_strcpy( pString, UTIL_VarArgs( "%f", value.flVal ) );
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_UInt64 : public ISchemaAttributeTypeBase<unsigned long long>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		*(pValue->lVal) = V_atoui64( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		V_strcpy( pString, UTIL_VarArgs( "%llu", *(value.lVal) ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Float : public ISchemaAttributeTypeBase<float>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		pValue->flVal = V_atof( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		V_strcpy( pString, UTIL_VarArgs( "%f", value.flVal ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_String : public ISchemaAttributeTypeBase<CAttribute_String>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		*(pValue->sVal) = pString;
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		if ( pAttrDef->string_attribute )
		{
			V_strcpy( pString, *(value.sVal) );
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema() :
	m_Items( DefLessFunc(item_def_index_t) ),
	m_Attributes( DefLessFunc(attrib_def_index_t) ),
	m_PrefabsValues( StringLessThan ),
	m_bInited( false )
{
}

CEconItemSchema::~CEconItemSchema()
{
	m_pSchema->deleteThis();

	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();

	FOR_EACH_DICT_FAST( m_PrefabsValues, i )
	{
		m_PrefabsValues[i]->deleteThis();
	}

	FOR_EACH_VEC( m_AttributeTypes, i )
	{
		delete m_AttributeTypes[i].pType;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CEconItemSchema::Init( void )
{
	Reset();

	KeyValuesAD schema("KVDataFile");
	if ( !schema->LoadFromFile( filesystem, ITEMS_GAME ) )
		return false;

	InitAttributeTypes();

	float flStartTime = engine->Time();

	ParseSchema( schema );

	float flEndTime = engine->Time();
	Msg( "Processing item schema took %.02fms. Parsed %d items and %d attributes.\n", ( flEndTime - flStartTime ) * 1000.0f, m_Items.Count(), m_Attributes.Count() );

	if ( !m_bScriptInit )
	{
		RegisterScriptFunctions();
		m_bScriptInit = true;
	}

	return true;
}

void CEconItemSchema::InitAttributeTypes( void )
{
	FOR_EACH_VEC( m_AttributeTypes, i )
	{
		delete m_AttributeTypes[i].pType;
	}
	m_AttributeTypes.Purge();

	attr_type_t defaultType;
	defaultType.szName = NULL;
	defaultType.pType = new CSchemaAttributeType_Default;
	m_AttributeTypes.AddToTail( defaultType );

	attr_type_t longType;
	longType.szName = "uint64";
	longType.pType = new CSchemaAttributeType_UInt64;
	m_AttributeTypes.AddToTail( longType );

	attr_type_t floatType;
	floatType.szName = "float";
	floatType.pType = new CSchemaAttributeType_Float;
	m_AttributeTypes.AddToTail( floatType );

	attr_type_t stringType;
	stringType.szName = "string";
	stringType.pType = new CSchemaAttributeType_String;
	m_AttributeTypes.AddToTail( stringType );
}

void CEconItemSchema::MergeDefinitionPrefabs( KeyValues *pDefinition, KeyValues *pSchemeData )
{
	char prefab[64]{0};
	V_strcpy_safe( prefab, pSchemeData->GetString( "prefab" ) );

	if ( prefab[0] != '\0' )
	{
		//check if there's prefab for prefab.. PREFABSEPTION
		CUtlStringList strings;
		V_SplitString( prefab, " ", strings );

		// traverse backwards in a adjective > noun style
		FOR_EACH_VEC_BACK( strings, i )
		{
			KeyValues *pPrefabValues = NULL;
			FIND_ELEMENT( m_PrefabsValues, strings[i], pPrefabValues );
			AssertMsg( pPrefabValues, "Unable to find prefab \"%s\".", strings[i] );

			if ( pPrefabValues )
				MergeDefinitionPrefabs( pDefinition, pPrefabValues );
		}
	}

	InheritKVRec( pSchemeData, pDefinition );
}

bool CEconItemSchema::LoadFromFile( void )
{
	KeyValuesAD schema("KVDataFile");
	if ( !schema->LoadFromFile( g_pFullFileSystem, ITEMS_GAME ) )
		return false;

	Reset();
	ParseSchema( schema );

	IGameEvent *event = gameeventmanager->CreateEvent( "inventory_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}

	return true;
}

bool CEconItemSchema::LoadFromBuffer( CUtlBuffer &buf, bool bAsText )
{
	KeyValuesAD schema("KVDataFile");
	bool bDidInit = bAsText ? schema->LoadFromBuffer( ITEMS_GAME, buf ) : schema->ReadAsBinary( buf );
	if ( !bDidInit ) return false;

	Reset();
	ParseSchema( schema );

	IGameEvent *event = gameeventmanager->CreateEvent( "inventory_updated" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}

	return true;
}

bool CEconItemSchema::SaveToBuffer( CUtlBuffer &buf )
{
	return m_pSchema->WriteAsBinary( buf );
}

void CEconItemSchema::Reset( void )
{
	m_pSchema->deleteThis();

	m_GameInfo.Purge();
	m_Colors.Purge();
	m_Qualities.Purge();

	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();

	FOR_EACH_MAP_FAST( m_PrefabsValues, i )
	{
		m_PrefabsValues[i]->deleteThis();
	}
	m_PrefabsValues.Purge();

	m_unSchemaResetCount++;
}

void CEconItemSchema::Precache( void )
{
	static CSchemaAttributeHandle pAttribDef_CustomProjectile( "custom projectile model" );

	// Precache everything from schema.
	FOR_EACH_MAP_FAST( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];
		
		// Precache models.
		if ( pItem->GetWorldModel() )
			CBaseEntity::PrecacheModel( pItem->GetWorldModel() );

		if ( pItem->GetPlayerModel() )
			CBaseEntity::PrecacheModel( pItem->GetPlayerModel() );

		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			if ( pItem->GetPerClassModel( iClass ) )
				CBaseEntity::PrecacheModel( pItem->GetPerClassModel( iClass ) );
		}

		if ( pItem->GetExtraWearableModel() )
			CBaseEntity::PrecacheModel( pItem->GetExtraWearableModel() );
		
		// Precache visuals.
		for ( int i = TEAM_UNASSIGNED; i < TF_TEAM_VISUALS_COUNT; i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			PerTeamVisuals_t *pVisuals = pItem->GetVisuals( i );
			if ( pVisuals == NULL )
				continue;

			// Precache sounds.
			for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
			{
				if ( pVisuals->GetWeaponShootSound( i ) )
					CBaseEntity::PrecacheScriptSound( pVisuals->GetWeaponShootSound( i ) );
			}
			for ( int i = 0; i < MAX_CUSTOM_WEAPON_SOUNDS; i++ )
			{
				if ( pVisuals->GetCustomWeaponSound( i ) )
					CBaseEntity::PrecacheScriptSound( pVisuals->GetCustomWeaponSound( i ) );
			}

			// Precache attachments.
			for ( int i = 0; i < pVisuals->attached_models.Count(); i++ )
			{
				const char *pszModel = pVisuals->attached_models[i].model;
				if ( pszModel && pszModel[0] != '\0' )
					CBaseEntity::PrecacheModel( pszModel );
			}

			// Precache particles
			// Custom Particles
			if ( pVisuals->GetCustomParticleSystem() )
			{
				PrecacheParticleSystem( pVisuals->GetCustomParticleSystem() );
			}
			// Muzzle Flash
			if ( pVisuals->GetMuzzleFlash() )
			{
				PrecacheParticleSystem( pVisuals->GetMuzzleFlash() );
			}
			// Tracer Effect
			if ( pVisuals->GetTracerFX() )
			{
				PrecacheParticleSystem( pVisuals->GetTracerFX() );
				
				//Since these get adjusted we need to do each one manually as a char.
				char pTracerEffect[128];
				char pTracerEffectCrit[128];
				Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_red", pVisuals->GetTracerFX() );
				Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_red_crit", pVisuals->GetTracerFX() );
				PrecacheParticleSystem( pTracerEffect );
				PrecacheParticleSystem( pTracerEffectCrit );

				Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_blue", pVisuals->GetTracerFX() );
				Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_blue_crit", pVisuals->GetTracerFX() );
				PrecacheParticleSystem( pTracerEffect );
				PrecacheParticleSystem( pTracerEffectCrit );
			}

		}

		// Cache all attrbute names.
		FOR_EACH_VEC( pItem->attributes, i )
		{
			static_attrib_t const &attrib = pItem->attributes[i];
			const CEconAttributeDefinition *pAttribute = attrib.GetStaticData();

			// Special case for custom_projectile_model attribute.
			if ( pAttribute == pAttribDef_CustomProjectile )
			{
				CBaseEntity::PrecacheModel( attrib.value );
			}
		}
	}
}

void CEconItemSchema::ParseSchema( KeyValues *pKVData )
{
	m_pSchema = pKVData->MakeCopy();

	KeyValues *pPrefabs = pKVData->FindKey( "prefabs" );
	if ( pPrefabs )
	{
		ParsePrefabs( pPrefabs );
	}

	KeyValues *pGameInfo = pKVData->FindKey( "game_info" );
	if ( pGameInfo )
	{
		ParseGameInfo( pGameInfo );
	}

	KeyValues *pQualities = pKVData->FindKey( "qualities" );
	if ( pQualities )
	{
		ParseQualities( pQualities );
	}

	KeyValues *pColors = pKVData->FindKey( "colors" );
	if ( pColors )
	{
		ParseColors( pColors );
	}

	KeyValues *pAttributes = pKVData->FindKey( "attributes" );
	if ( pAttributes )
	{
		ParseAttributes( pAttributes );
	}

	KeyValues *pItems = pKVData->FindKey( "items" );
	if ( pItems )
	{
		ParseItems( pItems );
	}

#if defined( TF_VINTAGE ) || defined( TF_VINTAGE_CLIENT )
	// TF2V exclusive lists start here.
	// None of these are necessary but they help in organizing the item schema.

	// Stockweapons is just for cataloging stock content.
	KeyValues *pStockItems = pKVData->FindKey( "stockitems" );
	if ( pStockItems )
	{
		ParseItems( pStockItems );
	}

	// Base Unlocks catalogs the standard unlock weapons.
	KeyValues *pUnlockItems = pKVData->FindKey( "unlockitems" );
	if ( pUnlockItems )
	{
		ParseItems( pUnlockItems );
	}

#if defined(CLIENT_DLL)
	if ( !CommandLine()->CheckParm( "-hidecosmetics" ) )
#endif
	{
	// Stock Cosmetics are for the typical cosmetics.
		KeyValues *pCosmeticItems = pKVData->FindKey( "cosmeticitems" );
		if ( pCosmeticItems )
		{
			ParseItems( pCosmeticItems );
		}
	}
	
#if defined(CLIENT_DLL)
	if ( !CommandLine()->CheckParm( "-hidereskins" ) )
#endif
	{
	// Reskins is for reskin weapons.
		KeyValues *pReskinItems = pKVData->FindKey( "reskinitems" );
		if ( pReskinItems )
		{
			ParseItems( pReskinItems );
		}
	}
	
	// Everything below should be largely static and not change much.

	// Special is for special top secret items.
	KeyValues *pSpecialItems = pKVData->FindKey( "specialitems" );
	if ( pSpecialItems )
	{
		ParseItems( pSpecialItems );
	}
	
	// Medals is well, player medals.
	KeyValues *pMedals = pKVData->FindKey( "medals" );
	if ( pMedals )
	{
		ParseItems( pMedals );
	}
	
	// Holiday is for content relating to events, like Halloween or Christmas.
	KeyValues *pHoliday = pKVData->FindKey( "holiday" );
	if ( pHoliday )
	{
		ParseItems( pHoliday );
	}
#endif
}

void CEconItemSchema::ParseGameInfo( KeyValues *pKVData )
{
	FOR_EACH_SUBKEY( pKVData, pSubData )
	{
		m_GameInfo.Insert( pSubData->GetName(), pSubData->GetFloat() );
	}
}

void CEconItemSchema::ParseQualities( KeyValues *pKVData )
{
	FOR_EACH_SUBKEY( pKVData, pSubData )
	{
		EconQuality Quality;
		GET_INT( &Quality , pSubData, value );
		m_Qualities.Insert( pSubData->GetName(), Quality );
	}
}

void CEconItemSchema::ParseColors( KeyValues *pKVData )
{
	FOR_EACH_SUBKEY( pKVData, pSubData )
	{
		EconColor ColorDesc;
		GET_STRING( &ColorDesc, pSubData, color_name );
		m_Colors.Insert( pSubData->GetName(), ColorDesc );
	}
}

void CEconItemSchema::ParsePrefabs( KeyValues *pKVData )
{
	FOR_EACH_TRUE_SUBKEY( pKVData, pSubData )
	{
		if ( GetItemSchema()->m_PrefabsValues.IsValidIndex( GetItemSchema()->m_PrefabsValues.Find( pSubData->GetName() ) ) )
		{
			Warning( "Duplicate prefab name (%s)\n", pSubData->GetName() );
			continue;
		}

		KeyValues *Values = pSubData->MakeCopy();
		m_PrefabsValues.Insert( pSubData->GetName(), Values );
	}
}

void CEconItemSchema::ParseItems( KeyValues *pKVData )
{
	FOR_EACH_TRUE_SUBKEY( pKVData, pSubData )
	{
		// Skip over default item, not sure why it's there.
		if ( V_stricmp( pSubData->GetName(), "default" ) == 0 )
			continue;

		CEconItemDefinition *pItem = new CEconItemDefinition;
		int index = V_atoi( pSubData->GetName() );
		pItem->index = index;

		KeyValues *pDefinition = new KeyValues( pSubData->GetName() );
		MergeDefinitionPrefabs( pDefinition, pSubData );
		pItem->LoadFromKV( pDefinition );

		m_Items.Insert( index, pItem );
	}
}

void CEconItemSchema::ParseAttributes( KeyValues *pKVData )
{
	FOR_EACH_TRUE_SUBKEY( pKVData, pSubData )
	{
		CEconAttributeDefinition *pAttribute = new CEconAttributeDefinition;
		pAttribute->index = V_atoi( pSubData->GetName() );

		pAttribute->LoadFromKV( pSubData->MakeCopy() );

		m_Attributes.Insert( pAttribute->index, pAttribute );
	}
}

void CEconItemSchema::ClientConnected( edict_t *pClient )
{
#if defined( GAME_DLL )
	if ( !pClient || pClient->IsFree() )
		return;

	CSteamID const *playerID = engine->GetClientSteamID( pClient );
	if ( playerID == NULL )
		return;

	INetChannel *pNetChan = dynamic_cast<INetChannel *>( engine->GetPlayerNetInfo( ENTINDEX( pClient ) ) );
	if( pNetChan )
		pNetChan->RegisterMessage( new CEconNetMsg() ); // This is safe to do multiple times

	g_pNetworking->OnClientConnected( *playerID );

	CUtlBuffer buf;
	SaveToBuffer( buf );

	size_t nCompressedLength = 0;
	byte *pCompressedSchema = LZMA_Compress( (byte *)buf.Base(), buf.TellPut(), &nCompressedLength );
	uint32 unSchemaCRC32 = CRC32_ProcessSingleBuffer( pCompressedSchema, nCompressedLength );

	CProtobufMsg<CUpdateItemSchemaMsg> msg;
	msg->set_items_data( pCompressedSchema, nCompressedLength );
	msg->set_items_game_hash( UTIL_VarArgs( "%ud", unSchemaCRC32 ) );
	msg->set_items_game_url( "https://raw.githubusercontent.com/TF2V/TF2Vintage/3.6/game/tf2vintage/assets/base/tf2v_core/scripts/items/items_game.txt" );

	free( pCompressedSchema );

	const int nLength = msg->ByteSize();
	CArrayAutoPtr<byte> array( new byte[ nLength ]() );
	msg->SerializeWithCachedSizesToArray( array.Get() );

	g_pNetworking->SendMessage( *playerID, k_EUpdateItemSchemaMsg, array.Get(), nLength );
#endif
}

void CEconItemSchema::ClientDisconnected( edict_t *pClient )
{
#if defined( GAME_DLL )
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

	CSteamID playerID{};
	pPlayer->GetSteamID( &playerID );
	g_pNetworking->OnClientDisconnected( playerID );
#endif
}

CEconItemDefinition *CEconItemSchema::GetItemDefinition( int id )
{
	CEconItemDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Items, id, itemdef );
	return itemdef;
}

CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Items, i )
	{
		if ( !V_stricmp( m_Items[i]->GetName(), name ) )
		{
			return m_Items[i];
		}
	}

	return NULL;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int id )
{
	CEconAttributeDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Attributes, id, itemdef );
	return itemdef;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->GetName(), name ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByClass( const char *classname )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->GetClassName(), classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

ISchemaAttributeType *CEconItemSchema::GetAttributeType( const char *name ) const
{
	FOR_EACH_VEC( m_AttributeTypes, i )
	{
		if ( m_AttributeTypes[i].szName == name )
			return m_AttributeTypes[i].pType;
	}

	return NULL;
}

bool CEconItemSchema::RegisterScriptFunctions( void )
{
	m_bScriptInit = true;


	return true;
}

#if defined( CLIENT_DLL )
class CUpdateEconItemSchema : public IMessageHandler
{
public:
	bool ProcessMessage( CNetPacket *pPacket ) OVERRIDE
	{
		CProtobufMsg<CUpdateItemSchemaMsg> msg( pPacket );

		std::string data = msg->items_data();
		size_t nUncompressedSize = LZMA_GetActualSize( const_cast<byte *>( reinterpret_cast<const byte *>( data.data() ) ) );
		byte *pUncompressedSchema = reinterpret_cast<byte *>( calloc( nUncompressedSize, sizeof( byte ) ) );
		LZMA_Uncompress( const_cast<byte *>( reinterpret_cast<const byte *>( data.data() ) ), &pUncompressedSchema, &nUncompressedSize );

		CUtlBuffer buf( pUncompressedSchema, nUncompressedSize, CUtlBuffer::READ_ONLY );

		// Ensure consistency
		uint32 unSchemaCRC = CRC32_ProcessSingleBuffer( reinterpret_cast<const void *>( data.data() ), data.length() );
		uint32 unRecvSchemaCRC = V_atoi64( msg->items_game_hash().c_str() );
		if ( unSchemaCRC != unRecvSchemaCRC )
		{
			Assert( unSchemaCRC == unRecvSchemaCRC );
			free( pUncompressedSchema );

			if ( !msg->use_online_backup() )
				return true;

			std::string url = msg->items_game_url();
			ISteamHTTP *pHTTP = SteamHTTP();
			if ( pHTTP )
			{
				m_bHTTPRequestComplete = false;
				HTTPRequestHandle hndl = pHTTP->CreateHTTPRequest( k_EHTTPMethodGET, url.c_str() );
				if ( hndl == INVALID_HTTPREQUEST_HANDLE )
					return true;

				pHTTP->SetHTTPRequestNetworkActivityTimeout( hndl, 10 );

				SteamAPICall_t call;
				pHTTP->SendHTTPRequest( hndl, &call );
				m_callback.Set( call, this, &CUpdateEconItemSchema::OnHTTPRequestCompleted );

				while ( !m_bHTTPRequestComplete )
					ThreadSleep( 15 );
			}

			return true;
		}

		if ( !GetItemSchema()->LoadFromBuffer( buf ) )
		{
			ConColorMsg( COLOR_RED, "****************************************************************\n" );
			ConColorMsg( COLOR_RED, "Unable to load Econ Item Schema from server, loading local file.\n" );
			ConColorMsg( COLOR_RED, "****************************************************************\n" );

			GetItemSchema()->LoadFromFile();
		}

		free( pUncompressedSchema );
		return true;
	}

private:
	CCallResult<CUpdateEconItemSchema, HTTPRequestCompleted_t> m_callback;
	bool m_bHTTPRequestComplete;

	void OnHTTPRequestCompleted( HTTPRequestCompleted_t *pRequest, bool bFailed )
	{
		m_bHTTPRequestComplete = true;

		ISteamHTTP *pHTTP = SteamHTTP();
		if ( !pHTTP )
		{
			Assert( false );
			return;
		}

		if ( pRequest->m_eStatusCode != k_EHTTPStatusCode200OK )
		{
			Warning( "Failed to update item schema: HTTP status %d\n", pRequest->m_eStatusCode );
		}
		else
		{
			if ( !pRequest->m_bRequestSuccessful )
				bFailed = true;

			if ( !bFailed )
			{
				KeyValuesAD pKeyValues( "items_game" );

				CUtlBuffer buf( 0, pRequest->m_unBodySize, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::CONTAINS_CRLF );
				bFailed = pHTTP->GetHTTPResponseBodyData( pRequest->m_hRequest, (uint8 *)buf.Base(), buf.TellPut() );
				if ( !bFailed )
				{
					bFailed = GetItemSchema()->LoadFromBuffer( buf, true );
				}
			}
			
			if( bFailed )
			{
				ConColorMsg( COLOR_RED, "****************************************************************\n" );
				ConColorMsg( COLOR_RED, "Unable to load Econ Item Schema from server, loading local file.\n" );
				ConColorMsg( COLOR_RED, "****************************************************************\n" );

				GetItemSchema()->LoadFromFile();
			}
		}

		pHTTP->ReleaseHTTPRequest( pRequest->m_hRequest );
	}
};
REG_ECON_MSG_HANDLER( CUpdateEconItemSchema, k_EUpdateItemSchemaMsg, UpdateItemSchemaMsg );
#endif
