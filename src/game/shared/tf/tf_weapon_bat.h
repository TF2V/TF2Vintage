//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_BAT_H
#define TF_WEAPON_BAT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFBat C_TFBat
#endif

//=============================================================================
//
// Bat class.
//
class CTFBat : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBat, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBat();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BAT; }

private:

	CTFBat( const CTFBat & ) {}
};


#if defined CLIENT_DLL
#define CTFBat_Fish C_TFBat_Fish
#endif

class CTFBat_Fish : public CTFBat
{
public:

	DECLARE_CLASS( CTFBat_Fish, CTFBat )
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int GetWeaponID( void ) const { return TF_WEAPON_BAT_FISH; }
};


#endif // TF_WEAPON_BAT_H