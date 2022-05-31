//====== Copyright © 1996-2013, Valve Corporation, All rights reserved. ========//
//
// Purpose: Flare used by the flaregun.
//
//=============================================================================//
#include "cbase.h"
#include "tf_projectile_energy_ring.h"
// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "particles_new.h"
#include "iefx.h"
#include "dlight.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "effect_dispatch_data.h"
#include "collisionutils.h"
#include "tf_team.h"
#include "props.h"
#include "tf_weapon_compound_bow.h"
#endif

#ifdef CLIENT_DLL
extern ConVar tf2v_muzzlelight;
#endif

#define TF_WEAPON_ENERGYRING_MODEL	"models/weapons/w_models/w_drg_ball.mdl"
#define TF_WEAPON_ENERGYRING_INTERVAL	0.15f


ConVar tf2v_use_new_bison_damage( "tf2v_use_new_bison_damage", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Changes Bison's damage mechanics.", true, 0, true, 2 );
#ifdef GAME_DLL
ConVar tf2v_use_new_bison_speed( "tf2v_use_new_bison_speed", "0", FCVAR_NOTIFY, "Decreases Bison speed by 30%." );
#endif

//=============================================================================
//
// Dragon's Fury Projectile
//

BEGIN_DATADESC( CTFProjectile_EnergyRing )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_energy_ring, CTFProjectile_EnergyRing );
PRECACHE_REGISTER( tf_projectile_energy_ring );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )
BEGIN_NETWORK_TABLE( CTFProjectile_EnergyRing, DT_TFProjectile_EnergyRing )
#ifdef GAME_DLL

#else

#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing::CTFProjectile_EnergyRing()
{
#ifdef CLIENT_DLL
	m_pRing = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing::~CTFProjectile_EnergyRing()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmissionAndDestroyImmediately( m_pRing );
	m_pRing = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Is used for differentiating between Bison (true) and Pomson (false) shots.
//-----------------------------------------------------------------------------
bool CTFProjectile_EnergyRing::UsePenetratingBeam() const
{
	int nPenetrate = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, nPenetrate, energy_weapon_penetration );

	return nPenetrate != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char*	CTFProjectile_EnergyRing::GetTrailParticleName() const
{
	if ( UsePenetratingBeam() )	// Righteous Bison
	{
		return IsCritical() ? "drg_bison_projectile_crit" : "drg_bison_projectile_crit";
	}
	else // Pomson
	{
		return IsCritical() ? "drg_pomson_projectile_crit" : "drg_pomson_projectile";
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_EnergyRing::GetProjectileModelName( void )
{
	return TF_WEAPON_ENERGYRING_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_EnergyRing::GetDamage( void )
{
	if ( !UsePenetratingBeam() )
		return 60.0f;

	if ( tf2v_use_new_bison_damage.GetInt() != 1 )
		return 20.0f;
	
	return 45.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Precache()
{
	PrecacheParticleSystem( "drg_bison_projectile" );
	PrecacheParticleSystem( "drg_bison_projectile_crit" );
	PrecacheParticleSystem( "drg_bison_impact" );

	PrecacheScriptSound( "Weapon_Bison.ProjectileImpactWorld" );
	PrecacheScriptSound( "Weapon_Bison.ProjectileImpactFlesh" );

	PrecacheParticleSystem( "drg_pomson_projectile" );
	PrecacheParticleSystem( "drg_pomson_projectile_crit" );
	PrecacheParticleSystem( "drg_pomson_impact" );
	PrecacheParticleSystem( "drg_pomson_impact_drain" );

	PrecacheScriptSound( "Weapon_Pomson.DrainedVictim" );
	PrecacheScriptSound( "Weapon_Pomson.ProjectileImpactWorld" );
	PrecacheScriptSound( "Weapon_Pomson.ProjectileImpactFlesh" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::Spawn()
{
	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	SetRenderMode( kRenderNone	);
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::ProjectileTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) || pOther->IsSolidFlagSet( FSOLID_NOT_SOLID ) || pOther->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
		return;

	CBaseEntity *pOwner = GetOwnerEntity();
	// Don't shoot ourselves
	if ( pOwner == pOther )
		return;

	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	// Pass through ladders
	if( pTrace->surface.flags & CONTENTS_LADDER )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		// if the entity is on our team check if it's a player carrying a bow
		if ( pOther->GetTeamNumber() == GetTeamNumber() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pOther );
			if( pPlayer )
			{
				CTFCompoundBow *pBow = dynamic_cast<CTFCompoundBow *>( pPlayer->GetActiveTFWeapon() );
				if ( pBow )
				{
					// Light the bow on fire.
					pBow->LightArrow();
				}
			}

			if ( UsePenetratingBeam() )
				return;
		}

		bool bShouldDamage = true;
		if ( UsePenetratingBeam() )
		{
			// Check the players hit.
			FOR_EACH_VEC( m_aHitEnemies, i )
			{
				// Is our victim, don't damage them.
				if ( m_aHitEnemies[i].hEntity == pOther && 
					 ( tf2v_use_new_bison_damage.GetInt() == 1 || (m_aHitEnemies[i].flLastHitTime + TF_WEAPON_ENERGYRING_INTERVAL) > gpGlobals->curtime ) )
				{
					bShouldDamage = false;
					break;
				}
			}
		}

		if ( bShouldDamage )
		{
			// Damage.
			float flDamage = GetDamage();
			if ( UsePenetratingBeam() )
			{
				// Damage done with Bison beams depends on era.
				if ( tf2v_use_new_bison_damage.GetInt() == 1 )
					flDamage *= pow( 0.75, m_aHitEnemies.Count() );
			}

			int iDamageType = GetDamageType();

			CTakeDamageInfo info( this, pOwner, m_hLauncher, flDamage, iDamageType | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_PLASMA );
			info.SetReportedPosition( pOwner->GetAbsOrigin() );

			// We collided with pOther, so try to find a place on their surface to show blood
			trace_t trace;
			UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), /*MASK_SOLID*/ MASK_SHOT | CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &trace );

			pOther->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

			ApplyMultiDamage();

			if ( UsePenetratingBeam() )
			{
				bool bExists = false;
				FOR_EACH_VEC( m_aHitEnemies, i )
				{
					if ( m_aHitEnemies[i].hEntity == pOther )
					{
						m_aHitEnemies[i].flLastHitTime = gpGlobals->curtime;
						bExists = true;
					}
				}

				// Save this entity so we don't double dip damage on it.
				if ( !bExists )
					m_aHitEnemies.AddToTail( {pOther, gpGlobals->curtime} );
			}

			Vector vecDelta = pOther->GetAbsOrigin() - GetAbsOrigin();
			Vector vecNormalVel = GetAbsVelocity().Normalized();
			Vector vecNewPos = ( DotProduct( vecDelta, vecNormalVel ) * vecNormalVel ) + GetAbsOrigin();

			PlayImpactEffects( vecNewPos, pOther->IsPlayer() );
		}

		// Non Penetrating: Delete the beam on the first thing we hit.
		if ( !UsePenetratingBeam() )
			UTIL_Remove( this );

		// Penetrating: Delete the beam after hitting the 4th target, when on mid era.
		if ( m_aHitEnemies.Count() >= 4 && tf2v_use_new_bison_damage.GetInt() == 1 )
			UTIL_Remove( this );
	}
	else
	{
		if ( pOther->IsWorld() )
		{
			SetAbsVelocity( vec3_origin	);
			AddSolidFlags( FSOLID_NOT_SOLID );
		}

		PlayImpactEffects( pTrace->endpos, false );

		UTIL_Remove( this );
	}
}

void CTFProjectile_EnergyRing::PlayImpactEffects( const Vector& vecPos, bool bHitFlesh )
{
	CTFWeaponBaseGun *pWeapon = (CTFWeaponBaseGun *)m_hLauncher.Get();
	if ( pWeapon )
	{
		DispatchParticleEffect( "drg_pomson_impact", vecPos, GetAbsAngles(), pWeapon->GetEnergyWeaponColor( false ), 
								pWeapon->GetEnergyWeaponColor( true ), true, NULL, PATTACH_ABSORIGIN );

		const char *pszSound = NULL;
		if ( UsePenetratingBeam() )
		{
			pszSound = bHitFlesh ? "Weapon_Bison.ProjectileImpactFlesh" : "Weapon_Bison.ProjectileImpactWorld";
		}
		else
		{
			pszSound = bHitFlesh ? "Weapon_Pomson.ProjectileImpactFlesh" : "Weapon_Pomson.ProjectileImpactWorld";
		}
		EmitSound( pszSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_EnergyRing *CTFProjectile_EnergyRing::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_EnergyRing *pRing = static_cast<CTFProjectile_EnergyRing *>( CBaseEntity::CreateNoSpawn( "tf_projectile_energy_ring", vecOrigin, vecAngles, pOwner ) );
	if ( pRing )
	{
		// Set team.
		pRing->ChangeTeam( pOwner->GetTeamNumber() );

		// Set scorer.
		pRing->SetScorer( pScorer );

		// Set firing weapon.
		pRing->SetLauncher( pWeapon );

		// Spawn.
		DispatchSpawn( pRing );

		// Setup the initial velocity.
		Vector vecForward, vecRight, vecUp;
		AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );


		float flVelocity = 1200.0f;
		if ( pRing->UsePenetratingBeam() && tf2v_use_new_bison_speed.GetBool() ) // New Bison speed is much slower.
			flVelocity *= 0.7;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flVelocity, mult_projectile_speed );

		Vector vecVelocity = vecForward * flVelocity;
		pRing->SetAbsVelocity( vecVelocity );
		pRing->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		// Setup the initial angles.
		QAngle angles;
		VectorAngles( vecVelocity, angles );
		pRing->SetAbsAngles( angles );

		float flGravity = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mod_rocket_gravity );
		if ( flGravity )
		{
			pRing->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
			pRing->SetGravity( flGravity );
		}

		return pRing;
	}

	return pRing;
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFProjectile_EnergyRing::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CreateTrails();
		CreateLightEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::CreateTrails( void )
{
	if ( IsDormant() )
		return;

	m_pRing = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );

	CTFWeaponBaseGun *pWeapon = (CTFWeaponBaseGun *)m_hLauncher.Get();
	if ( pWeapon )
	{
		m_pRing->SetControlPoint( CUSTOM_COLOR_CP1, pWeapon->GetEnergyWeaponColor( false ) );
		m_pRing->SetControlPoint( CUSTOM_COLOR_CP2, pWeapon->GetEnergyWeaponColor( true ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_EnergyRing::CreateLightEffects( void )
{
	// Handle the dynamic light
	if ( tf2v_muzzlelight.GetBool() )
	{
		AddEffects( EF_DIMLIGHT );

		dlight_t *dl;
		if ( IsEffectActive( EF_DIMLIGHT ) )
		{
			dl = effects->CL_AllocDlight( LIGHT_INDEX_TE_DYNAMIC + index );
			dl->origin = GetAbsOrigin();
			switch ( GetTeamNumber() )
			{
				case TF_TEAM_RED:
					if ( !IsCritical() )
					{
						dl->color.r = 255; dl->color.g = 30; dl->color.b = 10;
					}
					else
					{
						dl->color.r = 255; dl->color.g = 10; dl->color.b = 10;
					}
					break;

				case TF_TEAM_BLUE:
					if ( !IsCritical() )
					{
						dl->color.r = 10; dl->color.g = 30; dl->color.b = 255;
					}
					else
					{
						dl->color.r = 10; dl->color.g = 10; dl->color.b = 255;
					}
					break;
			}
			dl->radius = 256.0f;
			dl->die = gpGlobals->curtime + 0.1;

			tempents->RocketFlare( GetAbsOrigin() );
		}
	}
}
#endif