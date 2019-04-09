//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_obj_sentrygun.h"
#include "entity_bossresource.h"
#include "eyeball_boss_behavior.h"
#include "eyeball_boss.h"

ConVar tf_eyeball_boss_debug( "tf_eyeball_boss_debug", "0", FCVAR_CHEAT );
ConVar tf_eyeball_boss_debug_orientation( "tf_eyeball_boss_debug_orientation", "0", FCVAR_CHEAT );
ConVar tf_eyeball_boss_lifetime( "tf_eyeball_boss_lifetime", "120", FCVAR_CHEAT );
ConVar tf_eyeball_boss_lifetime_spell( "tf_eyeball_boss_lifetime_spell", "8", FCVAR_CHEAT );
ConVar tf_eyeball_boss_speed( "tf_eyeball_boss_speed", "250", FCVAR_CHEAT );
ConVar tf_eyeball_boss_hover_height( "tf_eyeball_boss_hover_height", "200", FCVAR_CHEAT );
ConVar tf_eyeball_boss_acceleration( "tf_eyeball_boss_acceleration", "500", FCVAR_CHEAT );
ConVar tf_eyeball_boss_horiz_damping( "tf_eyeball_boss_horiz_damping", "2", FCVAR_CHEAT );
ConVar tf_eyeball_boss_vert_damping( "tf_eyeball_boss_vert_damping", "1", FCVAR_CHEAT );
ConVar tf_eyeball_boss_attack_range( "tf_eyeball_boss_attack_range", "750", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_base( "tf_eyeball_boss_health_base", "8000", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_per_player( "tf_eyeball_boss_health_per_player", "400", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_at_level_2( "tf_eyeball_boss_health_at_level_2", "17000", FCVAR_CHEAT );
ConVar tf_eyeball_boss_health_per_level( "tf_eyeball_boss_health_per_level", "3000", FCVAR_CHEAT );

extern ConVar tf_halloween_bot_min_player_count;

IMPLEMENT_SERVERCLASS_ST( CEyeBallBoss, DT_EyeBallBoss )
SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
SendPropExclude( "DT_BaseEntity", "m_angAbsRotation" ),

SendPropVector( SENDINFO( m_lookAtSpot ) ),
SendPropInt( SENDINFO( m_attitude ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( eyeball_boss, CEyeBallBoss );

int CEyeBallBoss::m_level = 0;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static const float EyeBallBossModifyDamage( CTakeDamageInfo const& info )
{
	const float flDamage = info.GetDamage();

	CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( info.GetInflictor() );
	if ( pSentry )
		return flDamage / 4;

	CTFProjectile_SentryRocket *pRocket = dynamic_cast<CTFProjectile_SentryRocket *>( info.GetInflictor() );
	if ( pRocket )
		return flDamage / 4;

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info.GetWeapon() );
	if ( pWeapon )
	{
		if ( pWeapon->IsWeapon( TF_WEAPON_MINIGUN ) )
			return flDamage / 4;

		if ( pWeapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) )
			return flDamage / 2;
	}

	return flDamage;
}


IMPLEMENT_INTENTION_INTERFACE( CEyeBallBoss, CEyeBallBossBehavior )


CEyeBallBossLocomotion::CEyeBallBossLocomotion( CEyeBallBoss *me )
	: ILocomotion( me )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::Update( void )
{
	CEyeBallBoss *pActor = (CEyeBallBoss *)GetBot()->GetEntity();

	this->MaintainAltitude();

	float flLength = m_localVelocity.NormalizeInPlace();
	m_motionVector = m_localVelocity;

	m_verticalSpeed = m_localVelocity.z * flLength;

	m_localVelocity.x = ( m_wishVelocity.x - ( tf_eyeball_boss_horiz_damping.GetFloat() * m_localVelocity.x ) ) * GetUpdateInterval();
	m_localVelocity.y = ( m_wishVelocity.y - ( tf_eyeball_boss_horiz_damping.GetFloat() * m_localVelocity.y ) ) * GetUpdateInterval();
	m_localVelocity.z = ( m_wishVelocity.z - ( tf_eyeball_boss_vert_damping.GetFloat() * m_localVelocity.z ) ) * GetUpdateInterval();

	pActor->SetAbsVelocity( m_localVelocity );

	CTraceFilterSkipClassname filter( pActor, "eyeball_boss", COLLISION_GROUP_NONE );

	m_wishVelocity = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::Reset( void )
{
	m_desiredSpeed = 0;
	m_desiredAltitude = tf_eyeball_boss_hover_height.GetFloat();
	m_verticalSpeed = 0;
	m_motionVector = vec3_origin;
	m_wishVelocity = vec3_origin;
	m_localVelocity = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossLocomotion::GetStepHeight( void ) const
{
	return 50.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossLocomotion::GetMaxJumpHeight( void ) const
{
	return 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossLocomotion::GetDeathDropHeight( void ) const
{
	return 1000.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::Approach( const Vector& goalPos, float goalWeight )
{
	Vector vecGoal = goalPos - GetBot()->GetEntity()->GetAbsOrigin();
	Vector vecVelocity = vecGoal.Normalized() * tf_eyeball_boss_acceleration.GetFloat();

	m_wishVelocity.x += vecVelocity.x;
	m_wishVelocity.y += vecVelocity.y;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::FaceTowards( const Vector& target )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();

	Vector vecTo = target - pActor->WorldSpaceCenter();
	vecTo.z = 0;

	QAngle vecAng;
	VectorAngles( vecTo, vecAng );

	pActor->SetAbsAngles( vecAng );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CEyeBallBossLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );
	return up;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CEyeBallBossLocomotion::GetFeet( void ) const
{
	static Vector feet;

	CBaseCombatCharacter *pActor = GetBot()->GetEntity();
	CTraceFilterSkipClassname filter( pActor, "eyeball_boss", COLLISION_GROUP_NONE );

	trace_t tr;
	UTIL_TraceLine( pActor->GetAbsOrigin(), pActor->GetAbsOrigin() + Vector( 0, 0, -2000.0f ), MASK_PLAYERSOLID_BRUSHONLY, &filter, &tr );

	feet = tr.startpos;
	feet.z = tr.endpos.z;

	return feet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CEyeBallBossLocomotion::GetVelocity( void ) const
{
	return m_wishVelocity;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossLocomotion::GetDesiredSpeed( void ) const
{
	return m_desiredSpeed;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::SetDesiredSpeed( float fSpeed )
{
	m_desiredSpeed = fSpeed;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::SetDesiredAltitude( float fHeight )
{
	m_desiredAltitude = fHeight;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossLocomotion::MaintainAltitude( void )
{
	CEyeBallBoss *pActor = (CEyeBallBoss *)GetBot()->GetEntity();
	if ( pActor->IsAlive() )
	{
		CTraceFilterSkipClassname filter( pActor, "eyeball_boss", COLLISION_GROUP_NONE );
		Vector vecStart = pActor->GetAbsOrigin();
		Vector vecEnd = pActor->GetAbsOrigin();
		Vector vecMins = pActor->CollisionProp()->OBBMins();
		Vector vecMaxs = pActor->CollisionProp()->OBBMaxs();
		int nMask = pActor->GetBodyInterface()->GetSolidMask();

		trace_t ceiltrace;
		UTIL_TraceHull(
			vecStart,
			vecEnd + Vector( 0, 0, 1000.0f ),
			vecMins,
			vecMaxs,
			nMask,
			&filter,
			&ceiltrace
		);

		Vector vecAdditiveVec;
		if ( this->IsAttemptingToMove() )
		{
			vecAdditiveVec = m_motionVector;
			vecAdditiveVec.NormalizeInPlace();
		}
		else
		{
			vecAdditiveVec = vec3_origin;
		}

		Vector vecOffset = Vector( 0, 0, -2000.0f );
		vecOffset.z -= ceiltrace.endpos.z - vecStart.z;

		trace_t floortrace;
		UTIL_TraceHull(
			vecStart,
			vecEnd - vecOffset + vecAdditiveVec * 50.0f,
			vecMins * 1.25f,
			vecMaxs * 1.25f,
			nMask,
			&filter,
			&floortrace
		);

		m_wishVelocity.z += Clamp( GetDesiredAltitude() - ( vecStart.z - floortrace.endpos.z ),
								   -tf_eyeball_boss_acceleration.GetFloat(),
								   tf_eyeball_boss_acceleration.GetFloat() );
	}
	else
	{
		m_wishVelocity = Vector( 0, 0, -300.0f );
	}
}


CEyeBallBossBody::CEyeBallBossBody( CEyeBallBoss *me )
	: IBody( me )
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossBody::Update( void )
{
	CBaseCombatCharacter *pActor = GetBot()->GetEntity();

	Vector vecTo = m_lookAtSpot - pActor->WorldSpaceCenter();
	vecTo.NormalizeInPlace();

	Vector vecFwd;
	pActor->GetVectors( &vecFwd, NULL, NULL );

	vecTo = ( ( vecTo * 3.0f ) * GetUpdateInterval() ) + vecFwd;
	vecTo.NormalizeInPlace();

	QAngle vecAng;
	VectorAngles( vecTo, vecAng );

	pActor->SetAbsAngles( vecAng );
	if ( tf_eyeball_boss_debug_orientation.GetBool() )
	{
		NDebugOverlay::Line( pActor->WorldSpaceCenter(),
							 pActor->WorldSpaceCenter() + vecTo * 150.0f,
							 255,
							 0,
							 205,
							 false,
							 1.0f );
	}

	pActor->StudioFrameAdvance();
	pActor->DispatchAnimEvents( pActor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CEyeBallBossBody::GetMaxHeadAngularVelocity( void ) const
{
	return 3000.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossBody::AimHeadTowards( const Vector& lookAtPos, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	m_lookAtSpot = lookAtPos;
	CEyeBallBoss *pActor = (CEyeBallBoss *)GetBot()->GetEntity();
	pActor->m_lookAtSpot = m_lookAtSpot;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBossBody::AimHeadTowards( CBaseEntity *subject, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	m_lookAtSpot = subject->EyePosition();
	CEyeBallBoss *pActor = (CEyeBallBoss *)GetBot()->GetEntity();
	pActor->m_lookAtSpot = m_lookAtSpot;
}


CEyeBallBoss::CEyeBallBoss()
{
	m_intention = new CEyeBallBossIntention( this );
	m_body = new CEyeBallBossBody( this );
	m_locomotor = new CEyeBallBossLocomotion( this );

	m_iOldHealth = -1;
	m_iAngerPose = -1;
}

CEyeBallBoss::~CEyeBallBoss()
{
	delete m_intention;
	delete m_body;
	delete m_locomotor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::Precache( void )
{
	BaseClass::Precache();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheEyeBallBoss();

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/props_halloween/halloween_demoeye.mdl" );

	int iHealth;
	if ( m_level <= 1 )
	{
		iHealth = tf_eyeball_boss_health_base.GetInt();
		int iNumPlayers = GetGlobalTFTeam( TF_TEAM_RED )->GetNumPlayers() + GetGlobalTFTeam( TF_TEAM_BLUE )->GetNumPlayers();
		int iMinPlayers = tf_halloween_bot_min_player_count.GetInt();
		if ( iNumPlayers > iMinPlayers )
			iHealth += tf_eyeball_boss_health_per_player.GetInt() * ( iNumPlayers - iMinPlayers );
	}
	else
	{
		int iHealthPerLevel = tf_eyeball_boss_health_per_level.GetInt() * ( m_level - 2 );
		iHealth = tf_eyeball_boss_health_at_level_2.GetInt() + iHealthPerLevel;
	}

	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	const Vector mins( -50.0f, -50.0f, -50.0f ), maxs( 50.0f, 50.0f, 50.0f );
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );
	CollisionProp()->SetCollisionBounds( mins, maxs );

	ChangeTeam( TF_TEAM_BOSS );

	m_vecSpawn = GetAbsOrigin();

	if ( GetTeamNumber() == TF_TEAM_BOSS )
	{
		if ( g_pMonsterResource )
			g_pMonsterResource->SetBossHealthPercentage( 1.0f );

		m_attitude = ATTITUDE_CALM;
	}
	else
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				m_attitude = ATTITUDE_HATEBLUE;
				break;
			case TF_TEAM_BLUE:
				m_attitude = ATTITUDE_HATERED;
				break;
			default:
				break;
		}
	}

	m_lookAtSpot = vec3_origin;

	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "info_target" ) ) != NULL )
	{
		if ( pEntity->NameMatches( "spawn_boss_alt" ) )
			m_hSpawnEnts.AddToTail( pEntity );
	}

	if ( m_hSpawnEnts.IsEmpty() )
		Warning( "No info_target entities named 'spawn_boss_alt' found!\n" );

	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::UpdateOnRemove( void )
{
	if ( g_pMonsterResource )
		g_pMonsterResource->HideBossHealthMeter();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEyeBallBoss::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
	int res = 0;
	CTakeDamageInfo newInfo = info;
	if ( !IsSelf( info.GetAttacker() ) && GetTeamNumber() == TF_TEAM_BOSS )
	{
		int iHealth = GetHealth();
		newInfo.SetDamage( EyeBallBossModifyDamage( info ) );
		res = BaseClass::OnTakeDamage_Alive( newInfo );

		if ( g_pMonsterResource )
		{
			float flHPPercent = (float)GetHealth() / GetMaxHealth();
			if ( flHPPercent <= 0.0f )
				g_pMonsterResource->HideBossHealthMeter();
			else
				g_pMonsterResource->SetBossHealthPercentage( flHPPercent );
		}

		if ( m_iOldHealth >= 0 )
			m_iOldHealth = Max( m_iOldHealth - ( iHealth - GetHealth() ), 0 );
	}

	return res;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CEyeBallBoss::EyePosition( void )
{
	return GetViewOffset() + GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: We'll be off the ground by a few hundred units so search for our last known further away than BaseClass
//-----------------------------------------------------------------------------
void CEyeBallBoss::UpdateLastKnownArea( void )
{
	// find the area we are directly standing in
	CNavArea *area = TheNavMesh->GetNearestNavArea( this, GETNAVAREA_CHECK_LOS, 500.0f );
	if ( !area )
		return;

	// make sure we can actually use this area - if not, consider ourselves off the mesh
	if ( !IsAreaTraversable( area ) )
		return;

	if ( area != m_lastNavArea )
	{
		// player entered a new nav area
		if ( m_lastNavArea )
		{
			m_lastNavArea->DecrementPlayerCount( m_registeredNavTeam, entindex() );
			m_lastNavArea->OnExit( this, area );
		}

		m_registeredNavTeam = GetTeamNumber();
		area->IncrementPlayerCount( m_registeredNavTeam, entindex() );
		area->OnEnter( this, m_lastNavArea );

		OnNavAreaChanged( area, m_lastNavArea );

		m_lastNavArea = area;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::Update( void )
{
	BaseClass::Update();

	if ( m_iAngerPose < 0 )
		m_iAngerPose = LookupPoseParameter( "anger" );

	Assert( m_iAngerPose >= 0 );

	m_attitude = ATTITUDE_CALM;

	if ( GetTeamNumber() == TF_TEAM_BOSS && GetHealth() < 2 * ( GetMaxHealth() / 3 ) && ( !m_attitudeTimer.HasStarted() || m_attitudeTimer.IsElapsed() ) )
	{
		if ( GetHealth() < ( GetMaxHealth() / 3 ) )
		{
			m_attitude = ATTITUDE_ANGRY;
			SetPoseParameter( m_iAngerPose, 0.0f );

			return;
		}

		m_attitude = ATTITUDE_GRUMPY;
		SetPoseParameter( m_iAngerPose, 0.4f );

		return;
	}
	else if ( GetTeamNumber() != TF_TEAM_BOSS )
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				m_attitude = ATTITUDE_HATEBLUE;
				break;
			case TF_TEAM_BLUE:
				m_attitude = ATTITUDE_HATERED;
				break;
			default:
				break;
		}
	}

	SetPoseParameter( m_iAngerPose, 1.0f );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::JarateNearbyPlayer( float range )
{
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, TF_TEAM_RED, true );
	CollectPlayers( &enemies, TF_TEAM_BLUE, true, true );

	for ( int i=0; i<enemies.Count(); ++i )
	{
		CTFPlayer *pPlayer = enemies[i];
		if ( IsRangeLessThan( pPlayer, range ) && IsLineOfSightClear( pPlayer, CBaseCombatCharacter::IGNORE_ACTORS ) )
			pPlayer->m_Shared.AddCond( TF_COND_URINE, 10.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CEyeBallBoss::GetVictim( void ) const
{
	if ( m_hTarget )
	{
		if ( GetTeamNumber() == TF_TEAM_BOSS && m_hTarget->GetAbsOrigin().z < -1152.0f )
			return nullptr;
	}

	return m_hTarget;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CEyeBallBoss::FindNearestVisibleVictim( void )
{
	int iTeam;
	float flMinDist;
	CUtlVector<CTFPlayer *> players;
	CBaseCombatCharacter *pClosest = nullptr;

	if ( GetTeamNumber() == TF_TEAM_BOSS )
	{
		iTeam = TEAM_ANY;
		flMinDist = FLT_MAX;
	}
	else
	{
		switch ( GetTeamNumber() )
		{
			case TF_TEAM_RED:
				iTeam = TF_TEAM_BLUE;
				break;
			case TF_TEAM_BLUE:
				iTeam = TF_TEAM_RED;
				break;
			default:
				iTeam = TEAM_ANY;
				break;
		}

		flMinDist = FLT_MAX;
	}

	CollectPlayers( &players, iTeam, true );
	for ( int i=0; i<players.Count(); ++i )
	{
		CTFPlayer *pPlayer = players[i];
		if ( GetTeamNumber() == TF_TEAM_BOSS && pPlayer->GetAbsOrigin().z < -1152.0f )
			continue;

		if ( !pPlayer->m_Shared.IsStealthed() ||
			 pPlayer->m_Shared.InCond( TF_COND_BURNING ) ||
			 pPlayer->m_Shared.InCond( TF_COND_URINE ) ||
			 pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) ||
			 pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		{
			if ( ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pPlayer->m_Shared.GetDisguiseTeam() != GetTeamNumber() ) && !pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			{
				float flDistance = ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
				if ( flMinDist > flDistance && IsLineOfSightClear( pPlayer, CBaseCombatCharacter::IGNORE_NOTHING ) )
				{
					flMinDist = flDistance;
					pClosest = pPlayer;
				}
			}
		}
	}

	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *pObj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( !pObj || pObj->GetTeamNumber() == GetTeamNumber() || pObj->GetType() != OBJ_SENTRYGUN )
			continue;

		float flDistance = GetRangeSquaredTo( pObj );
		if ( flMinDist > flDistance && IsAbleToSee( pObj, CBaseCombatCharacter::USE_FOV ) )
		{
			flMinDist = flDistance;
			pClosest = pObj;
		}
	}

	return pClosest;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::BecomeEnraged( float duration )
{
	if ( GetTeamNumber() == TF_TEAM_BOSS && GetHealth() >= ( GetMaxHealth() / 3 ) && ( !m_attitudeTimer.HasStarted() || m_attitudeTimer.IsElapsed() ) )
		EmitSound( "Halloween.EyeballBossBecomeEnraged" );

	m_attitudeTimer.Start( duration );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const Vector& CEyeBallBoss::PickNewSpawnSpot( void ) const
{
	static Vector spot = GetAbsOrigin();

	if ( !m_hSpawnEnts.IsEmpty() )
	{
		CBaseEntity *pSpot = m_hSpawnEnts[RandomInt( 0, m_hSpawnEnts.Count() )];
		if ( pSpot )
			spot = pSpot->GetAbsOrigin();
	}

	return spot;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::PrecacheEyeBallBoss( void )
{
	PrecacheModel( "models/props_halloween/halloween_demoeye.mdl" );
	PrecacheModel( "models/props_halloween/eyeball_projectile.mdl" );

	PrecacheScriptSound( "Halloween.EyeballBossIdle" );
	PrecacheScriptSound( "Halloween.EyeballBossBecomeAlert" );
	PrecacheScriptSound( "Halloween.EyeballBossAcquiredVictim" );
	PrecacheScriptSound( "Halloween.EyeballBossStunned" );
	PrecacheScriptSound( "Halloween.EyeballBossStunRecover" );
	PrecacheScriptSound( "Halloween.EyeballBossLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossBigLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossDie" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeSoon" );
	PrecacheScriptSound( "Halloween.EyeballBossEscapeImminent" );
	PrecacheScriptSound( "Halloween.EyeballBossEscaped" );
	PrecacheScriptSound( "Halloween.EyeballBossDie" );
	PrecacheScriptSound( "Halloween.EyeballBossTeleport" );
	PrecacheScriptSound( "Halloween.HeadlessBossSpawnRumble" );
	PrecacheScriptSound( "Halloween.EyeballBossBecomeEnraged" );
	PrecacheScriptSound( "Halloween.EyeballBossRage" );
	PrecacheScriptSound( "Halloween.EyeballBossCalmDown" );
	PrecacheScriptSound( "Halloween.spell_spawn_boss_disappear" );

	PrecacheParticleSystem( "eyeboss_death" );
	PrecacheParticleSystem( "eyeboss_aura_angry" );
	PrecacheParticleSystem( "eyeboss_aura_grumpy" );
	PrecacheParticleSystem( "eyeboss_aura_calm" );
	PrecacheParticleSystem( "eyeboss_aura_stunned" );
	PrecacheParticleSystem( "eyeboss_tp_normal" );
	PrecacheParticleSystem( "eyeboss_tp_escape" );
	PrecacheParticleSystem( "eyeboss_team_red" );
	PrecacheParticleSystem( "eyeboss_team_blue" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEyeBallBoss::LogPlayerInteraction( const char *event, CTFPlayer *pAttacker )
{
}
