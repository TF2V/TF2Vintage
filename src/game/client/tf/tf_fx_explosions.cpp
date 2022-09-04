//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "econ_item_system.h"

//--------------------------------------------------------------------------------------------------------------
extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback( const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, ClientEntityHandle_t hEntity, int iItemID, int nSoundIndex, int nParticleSystemIndex )
{
	// Get the weapon information.
	CTFWeaponInfo *pWeaponInfo = NULL;
	switch ( iWeaponID )
	{
	case TF_WEAPON_GRENADE_PIPEBOMB:
	case TF_WEAPON_GRENADE_DEMOMAN:
	case TF_WEAPON_GRENADE_PIPEBOMB_BETA:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_PIPEBOMBLAUNCHER );
		break;
	case TF_WEAPON_SENTRY_ROCKET:
	case TF_WEAPON_PUMPKIN_BOMB:
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_ROCKETLAUNCHER );
		break;
	default:
		pWeaponInfo = GetTFWeaponInfo( iWeaponID );
		break;
	}

	bool bIsPlayer = false;
	int iTeamNumber = TEAM_UNASSIGNED;
	if ( hEntity.Get() )
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance( hEntity );
		if ( pEntity )
		{
			iTeamNumber = pEntity->GetTeamNumber();

			if( pEntity->IsPlayer() )
				bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = ( UTIL_PointContents( vecOrigin ) & CONTENTS_WATER );
	bool bInAir = false;
	QAngle angExplosion( 0.0f, 0.0f, 0.0f );

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if ( fabs( vecNormal.x ) < 0.05f && fabs( vecNormal.y ) < 0.05f && fabs( vecNormal.z ) < 0.05f )
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles( vecNormal, angExplosion );
		bInAir = false;
	}

	// Base explosion effect and sound.
	const char *pszEffect = (nParticleSystemIndex == -1) ?
		"ExplosionCore_wall" : GetParticleSystemNameFromIndex( nParticleSystemIndex );
	char pszSound[64] = "BaseExplosionEffect.Sound";

	if ( pWeaponInfo )
	{
		if( nParticleSystemIndex == -1 )
		{
			// Explosions.
			if ( bIsWater )
			{
				if ( Q_strlen( pWeaponInfo->m_szExplosionWaterEffect ) > 0 )
				{
					pszEffect = pWeaponInfo->m_szExplosionWaterEffect;
				}
			}
			else
			{
				if ( bIsPlayer || bInAir )
				{
					if ( Q_strlen( pWeaponInfo->m_szExplosionPlayerEffect ) > 0 )
					{
						pszEffect = pWeaponInfo->m_szExplosionPlayerEffect;
					}
				}
				else
				{
					if ( Q_strlen( pWeaponInfo->m_szExplosionEffect ) > 0 )
					{
						pszEffect = pWeaponInfo->m_szExplosionEffect;
					}
				}
			}
		}

		// Sound.
		if ( Q_strlen( pWeaponInfo->m_szExplosionSound ) > 0 )
		{
			V_strncpy( pszSound, pWeaponInfo->m_szExplosionSound, sizeof( pszSound ) );
		}
	}

	// Allow schema to override explosion sound.
	if ( iItemID >= 0 )
	{
		CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( iItemID );
		if ( pItemDef )
		{
			PerTeamVisuals_t *pVisuals = pItemDef->GetVisuals();
			if( pVisuals && pVisuals->GetWeaponShootSound( nSoundIndex ) )
			{
				V_strncpy( pszSound, pVisuals->GetWeaponShootSound( nSoundIndex ), sizeof( pszSound ) );
			}

			if ( iTeamNumber > TEAM_SPECTATOR )
			{
				pVisuals = pItemDef->GetVisuals( iTeamNumber );
				if( pVisuals && pVisuals->GetWeaponShootSound( nSoundIndex ) )
				{
					V_strncpy( pszSound, pVisuals->GetWeaponShootSound( nSoundIndex ), sizeof( pszSound ) );
				}
			}
		}
	}

	if ( iWeaponID == TF_WEAPON_PUMPKIN_BOMB )
		V_strncpy( pszSound, "Halloween.PumpkinExplode", sizeof( pszSound ) );
	
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, pszSound, &vecOrigin );

	DispatchParticleEffect( pszEffect, vecOrigin, angExplosion );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector		m_vecOrigin;
	Vector		m_vecNormal;
	int			m_iWeaponID;
	int			m_iItemID;
	int			m_nSound;
	int			m_nParticleIndex;
	ClientEntityHandle_t m_hEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_iItemID = -1;
	m_hEntity = INVALID_EHANDLE_INDEX;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity, m_iItemID, m_nSound, m_nParticleIndex );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( RECVINFO( m_iItemID ) ),
	RecvPropInt( RECVINFO( m_nSound ) ),
	RecvPropInt( RECVINFO( m_nParticleIndex ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
END_RECV_TABLE();

