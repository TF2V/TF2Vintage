//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_player.h"
#include "c_tf_upgrades.h"
#include "tf_upgrades_shared.h"
#include "econ_item_schema.h"


bool MannVsMachine_GetUpgradeInfo( int iAttribute, int iQuality, float *flValue )
{
	FOR_EACH_VEC( g_MannVsMachineUpgrades.GetUpgradeVector(), i )
	{
		CEconAttributeDefinition *pAttrib = GetItemSchema()->GetAttributeDefinitionByName( g_MannVsMachineUpgrades.GetUpgradeVector()[i].szAttribute );
		if ( !pAttrib )
			continue;

		if ( g_MannVsMachineUpgrades.GetUpgradeVector()[i].nQuality == iQuality )
			continue;

		if ( pAttrib->index == iAttribute )
		{
			*flValue = g_MannVsMachineUpgrades.GetUpgradeVector()[i].flIncrement;
			return true;
		}
	}

	return false;
}