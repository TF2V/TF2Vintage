//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "c_tf_base_boss.h"
#include "teamplayroundbased_gamerules.h"

class C_TFTankBoss : public C_TFBaseBoss
{
public:
	DECLARE_CLASS( C_TFTankBoss, C_TFBaseBoss );
	DECLARE_CLIENTCLASS();

	C_TFTankBoss();

	virtual void GetGlowEffectColor( float *r, float *g, float *b );

	// ITFMvMBossProgressUser
	virtual const char* GetBossProgressImageName() const OVERRIDE { return "tank"; }
};


IMPLEMENT_CLIENTCLASS_DT(C_TFTankBoss, DT_TFTankBoss, CTFTankBoss)
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( tank_boss, C_TFTankBoss );


C_TFTankBoss::C_TFTankBoss()
{
}

void C_TFTankBoss::GetGlowEffectColor( float *r, float *g, float *b )
{
	TeamplayRoundBasedRules()->GetTeamGlowColor( GetTeamNumber(), *r, *g, *b );
}

