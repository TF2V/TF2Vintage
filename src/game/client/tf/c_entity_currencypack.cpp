//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_entity_currencypack.h"

IMPLEMENT_CLIENTCLASS_DT( C_CurrencyPack, DT_CurrencyPack, CCurrencyPack )
	RecvPropBool( RECVINFO( m_bDistributed ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CurrencyPack::C_CurrencyPack()
{
	m_pGlowEffect = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CurrencyPack::~C_CurrencyPack()
{
	DestroyGlowEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::ClientThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::UpdateGlowEffect( void )
{
	if ( m_pGlowEffect )
		DestroyGlowEffect();

	if ( m_bShouldGlow )
	{
		m_pGlowEffect = new CGlowObject( 
			this, 
			m_bDistributed ? Vector( 150, 0, 0 ) : Vector( 0, 150, 0 ),
			1.0,
			true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}