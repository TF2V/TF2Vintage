//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef ENTITY_CURRENCYPACK_H
#define ENTITY_CURRENCYPACK_H

#include "tf_powerup.h"
#include "tf_shareddefs.h"

DECLARE_AUTO_LIST( ICurrencyPackAutoList );

class CCurrencyPack : public CTFPowerup, public ICurrencyPackAutoList
{
	DECLARE_CLASS( CCurrencyPack, CTFPowerup );
public:
	DECLARE_SERVERCLASS();

	CCurrencyPack();
	virtual ~CCurrencyPack();

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	UpdateOnRemove( void );
	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool	MyTouch( CBasePlayer *pPlayer );

	virtual bool	AffectedByRadiusCollection() const { return true; }
	virtual CurrencyRewards_t GetPackSize( void ) { return CURRENCY_PACK_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_large.mdl"; }

	void			SetAmount( float flAmount );
	void			DistributedBy( CBasePlayer *pMoneyMaker );

protected:
	virtual void	ComeToRest( void );
	void			BlinkThink( void );

	bool	m_bTouched;
	CNetworkVar( bool, m_bDistributed );
	int		m_nAmount;

	int		m_nBlinkState;
};

class CCurrencyPackMedium : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackMedium, CCurrencyPack );

	virtual CurrencyRewards_t GetPackSize( void ) { return CURRENCY_PACK_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_medium.mdl"; }
};

class CCurrencyPackSmall : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackSmall, CCurrencyPack );

	virtual CurrencyRewards_t GetPackSize( void ) { return CURRENCY_PACK_SMALL; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_small.mdl"; }
};

class CCurrencyPackCustom : public CCurrencyPack
{
public:
	DECLARE_CLASS( CCurrencyPackCustom, CCurrencyPack );

	virtual CurrencyRewards_t GetPackSize( void ) { return CURRENCY_PACK_CUSTOM; }
	virtual const char *GetDefaultPowerupModel( void );
};

#endif