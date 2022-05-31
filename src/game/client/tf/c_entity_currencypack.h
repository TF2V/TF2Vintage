//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef C_ENTITY_CURRENCYPACK_H
#define C_ENTITY_CURRENCYPACK_H

class C_CurrencyPack : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_CurrencyPack, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_CurrencyPack();
	virtual ~C_CurrencyPack();

	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void ClientThink();

private:

	void UpdateGlowEffect( void );
	void DestroyGlowEffect( void );
	CGlowObject *m_pGlowEffect;
	bool m_bShouldGlow;

	bool m_bDistributed;
};

#endif