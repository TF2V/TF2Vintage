//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_robot_destruction_robot.h"
#include "tf_logic_robot_destruction.h"
#include "particle_parse.h"

#ifdef GAME_DLL
	#include "tf_ammo_pack.h"
	#include "entity_bonuspack.h"
#else
	#include "eventlist.h"
#endif

#ifdef GAME_DLL
extern ConVar tf_obj_gib_velocity_min;
extern ConVar tf_obj_gib_velocity_max;
extern ConVar tf_obj_gib_maxspeed;
#endif

ConVar tf_rd_robot_repair_rate( "tf_rd_robot_repair_rate", "60", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#ifdef GAME_DLL

IMPLEMENT_INTENTION_INTERFACE( CTFRobotDestruction_Robot, CRobotBehavior );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRobotLocomotion::CRobotLocomotion( INextBot *bot )
	: NextBotGroundLocomotion( bot )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CRobotLocomotion::GetRunSpeed( void ) const
{
	CTFRobotDestruction_Robot *me = static_cast<CTFRobotDestruction_Robot *>( GetBot()->GetEntity() );
	return me->IsPanicking() ? 150.f : 80.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CRobotLocomotion::GetGroundSpeed() const
{
	CTFRobotDestruction_Robot *me = static_cast<CTFRobotDestruction_Robot *>( GetBot()->GetEntity() );
	return me->IsPanicking() ? 150.f : 80.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRobotLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	return false;
}


CRobotBody::CRobotBody( INextBot *actor )
	: IBody( actor )
{
	m_iMoveX = -1;
	m_iMoveY = -1;

	m_vecLean = vec3_origin;
	m_vecPrevOrigin = vec3_origin;
	m_vecImpulse = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRobotBody::Update( void )
{
	CBaseCombatCharacter *actor = GetBot()->GetEntity();
	if ( actor == nullptr )
		return;

	if ( m_iMoveX < 0 )
		m_iMoveX = actor->LookupPoseParameter( "move_x" );
	if ( m_iMoveY < 0 )
		m_iMoveY = actor->LookupPoseParameter( "move_y" );

	Vector vecVelocity = m_vecPrevOrigin - actor->GetAbsOrigin();
	m_vecPrevOrigin = actor->GetAbsOrigin();

	Approach( m_vecLean, vecVelocity + m_vecImpulse, 2.0f );
	Approach( m_vecImpulse, vec3_origin, 200.0f );

	Vector vecFwd, vecRight;
	AngleVectors( actor->GetAbsAngles(), &vecFwd, &vecRight, NULL );

	if ( m_iMoveX >= 0 )
		actor->SetPoseParameter( m_iMoveX, vecFwd.Dot( m_vecLean ) );
	if ( m_iMoveY >= 0 )
		actor->SetPoseParameter( m_iMoveY, vecRight.Dot( m_vecLean ) );

	const float flSpeed = GetBot()->GetLocomotionInterface()->GetGroundSpeed();
	if ( actor->m_flGroundSpeed != 0.0f )
		actor->SetPlaybackRate( Clamp( flSpeed / actor->m_flGroundSpeed, -4.0f, 12.0f ) );

	actor->StudioFrameAdvance();
	actor->DispatchAnimEvents( actor );
}

//-----------------------------------------------------------------------------
// Purpose: Begin a specific sequence
//-----------------------------------------------------------------------------
bool CRobotBody::StartActivity( Activity act, unsigned int flags )
{
	CBaseCombatCharacter *actor = GetBot()->GetEntity();

	int iSequence = actor->SelectWeightedSequence( act );
	if ( iSequence )
	{
		m_Activity = act;

		actor->SetSequence( iSequence );
		actor->SetPlaybackRate( 1.0f );
		actor->SetCycle( 0.0f );
		actor->ResetSequenceInfo();

		return true;
	}

	return false;
}
#else

C_RobotBody::C_RobotBody( CBaseCombatCharacter *actor )
	: m_Actor( actor )
{
	m_iMoveX = -1;
	m_iMoveY = -1;

	m_vecLean = vec3_origin;
	m_vecPrevOrigin = vec3_origin;
	m_vecImpulse = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_RobotBody::Update( void )
{
	if ( m_Actor == nullptr )
		return;

	if ( m_iMoveX < 0 )
		m_iMoveX = m_Actor->LookupPoseParameter( "move_x" );
	if ( m_iMoveY < 0 )
		m_iMoveY = m_Actor->LookupPoseParameter( "move_y" );

	Vector vecVelocity = m_vecPrevOrigin - m_Actor->GetAbsOrigin();
	m_vecPrevOrigin = m_Actor->GetAbsOrigin();

	Approach( m_vecLean, vecVelocity + m_vecImpulse, 2.0f );
	Approach( m_vecImpulse, vec3_origin, 200.0f );

	Vector vecFwd, vecRight;
	AngleVectors( m_Actor->GetAbsAngles(), &vecFwd, &vecRight, nullptr );

	if ( m_iMoveX >= 0 )
		m_Actor->SetPoseParameter( m_iMoveX, vecFwd.Dot( m_vecLean ) );
	if ( m_iMoveY >= 0 )
		m_Actor->SetPoseParameter( m_iMoveY, vecRight.Dot( m_vecLean ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRobotBody::Impulse( Vector const &vecImpulse )
{
	m_vecImpulse += vecImpulse * 5;
}

//-----------------------------------------------------------------------------
// Purpose: Shift a vector towards the target at the specified rate over time
//-----------------------------------------------------------------------------
void CRobotBody::Approach( Vector &vecIn, Vector const &vecTarget, float flRate )
{
	Vector const vecApproach = ( vecTarget - vecIn ) * flRate * gpGlobals->frametime;
	if ( vecApproach.LengthSqr() > ( vecIn - vecTarget ).LengthSqr() )
		vecIn = vecTarget;
	else
		vecIn += vecApproach;
}


BEGIN_DATADESC( CRobotDispenser )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( RobotDispenser, DT_RobotDispenser )
BEGIN_NETWORK_TABLE( CRobotDispenser, DT_RobotDispenser  )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( rd_robot_dispenser, CRobotDispenser );

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRobotDispenser::CRobotDispenser()
{
	m_bPlayRefillSound = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRobotDispenser::Spawn( void )
{
	m_fObjectFlags |= OF_DOESNT_HAVE_A_MODEL;
	m_takedamage = DAMAGE_NO;
	m_iUpgradeLevel = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Finished building
//-----------------------------------------------------------------------------
void CRobotDispenser::OnGoActive( void )
{
	BaseClass::OnGoActive();

	if ( m_hTouchTrigger )
	{
		m_hTouchTrigger->SetParent( GetParent() );
	}

	SetModel( "" ); 
}

//-----------------------------------------------------------------------------
// Purpose: Skip base class function call
//-----------------------------------------------------------------------------
void CRobotDispenser::SetModel( const char *pModel )
{
	CBaseObject::SetModel( pModel );
}

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotDestruction_Robot, DT_TFRobotDestruction_Robot )

BEGIN_NETWORK_TABLE( CTFRobotDestruction_Robot, DT_TFRobotDestruction_Robot  )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO(m_iHealth) ),
	RecvPropInt( RECVINFO(m_iMaxHealth) ),
	RecvPropInt( RECVINFO(m_eType) ),
#else
	SendPropInt( SENDINFO(m_iHealth), -1, SPROP_VARINT ),
	SendPropInt( SENDINFO(m_iMaxHealth), -1, SPROP_VARINT|SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_eType), -1, SPROP_VARINT|SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFRobotDestruction_Robot )
#ifdef GAME_DLL
	DEFINE_INPUTFUNC( FIELD_VOID, "StopAndUseComputer", InputStopAndUseComputer ),
#endif
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_robot_destruction_robot, CTFRobotDestruction_Robot );

CTFRobotDestruction_Robot::CTFRobotDestruction_Robot( void )
{
#ifdef GAME_DLL
	ALLOCATE_INTENTION_INTERFACE( CTFRobotDestruction_Robot );
	m_loco = new CRobotLocomotion( this );
#else
	ListenForGameEvent( "rd_robot_impact" );
#endif

	m_body = new CRobotBody( this );

	UseClientSideAnimation();
}

CTFRobotDestruction_Robot::~CTFRobotDestruction_Robot( void )
{
#ifdef GAME_DLL
	if ( m_hRobotSpawn )
		m_hRobotSpawn->ClearRobot();

	if ( m_intention )
		delete m_intention;
	if ( m_loco )
		delete m_loco;
#endif

	if ( m_body )
		delete m_body;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::Precache()
{
	BaseClass::Precache();
	
	PrecacheParticleSystem( "rd_robot_explosion" );
	PrecacheParticleSystem( "bot_radio_waves" );
	PrecacheParticleSystem( "sentrydamage_4" );

	PrecacheScriptSound( "RD.BotDeathExplosion" );

	for ( int i=0; i < ARRAYSIZE( g_RobotData ); ++i )
	{
		g_RobotData[i]->Precache();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::Spawn()
{
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );
	BuildPropList( "spawn", m_aSpawnProps, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	m_takedamage = DAMAGE_YES;
	m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;

#ifdef GAME_DLL
	SetType( m_spawnData.m_eType );

	m_hGoalPath = dynamic_cast<CPathTrack *>( gEntList.FindEntityByName( NULL, m_spawnData.m_pszPathName ) );
	if ( !m_hGoalPath )
	{
		UTIL_Remove( this );
	}

	if ( m_hRobotGroup )
		m_hRobotGroup->UpdateState();

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotCreated( this );

	m_hDispenser = dynamic_cast<CRobotDispenser *>( CreateEntityByName( "rd_robot_dispenser" ) );
	m_hDispenser->SetParent( this );
	m_hDispenser->Spawn();
	m_hDispenser->ChangeTeam( GetTeamNumber() );
	m_hDispenser->OnGoActive();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Dont collide with players
//-----------------------------------------------------------------------------
bool CTFRobotDestruction_Robot::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::HandleAnimEvent( animevent_t *pEvent )
{
	// NOP
}

//-----------------------------------------------------------------------------
// Purpose: Tell the game logic we're gone
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotRemoved( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::Event_Killed( const CTakeDamageInfo &info )
{
	// Let the game logic know that we died
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotRemoved( this );

	PlayDeathEffects();

	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( info.GetAttacker(), info.GetInflictor(), this ) );
	if ( pScorer )
	{
		CTFPlayer *pAssister = NULL;

		CTFPlayer *pHealer = ToTFPlayer( pScorer->m_Shared.GetFirstHealer() );
		if ( pHealer && pHealer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) )
		{
			pAssister = pHealer;
		}

		int iWeaponID;
		const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, NULL, iWeaponID );
		const char *killer_weapon_log_name = killer_weapon_name;

		CTFWeaponBase *pWeapon = pScorer->Weapon_OwnsThisID( iWeaponID );
		if ( pWeapon )
		{
			CEconItemDefinition *pItemDef = pWeapon->GetItem()->GetStaticData();
			if ( pItemDef )
			{
				if ( pItemDef->GetIconName() )
					killer_weapon_name = pItemDef->GetIconName();

				if ( pItemDef->GetLogName() )
					killer_weapon_log_name = pItemDef->GetLogName();
			}
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "rd_robot_killed" );
		if ( event )
		{
			if ( pAssister && ( pAssister != pScorer ) )
			{
				event->SetInt( "assister", pAssister->GetUserID() );
			}

			event->SetInt( "attacker", pScorer->GetUserID() );
			event->SetString( "weapon", killer_weapon_name );
			event->SetString( "weapon_logclassname", killer_weapon_log_name );
			event->SetInt( "weaponid", iWeaponID );
			event->SetInt( "priority", 6 );		// HLTV priority

			gameeventmanager->FireEvent( event );
		}
	}

	if ( m_hRobotSpawn )
		m_hRobotSpawn->OnRobotKilled();

	if ( m_hRobotGroup )
		m_hRobotGroup->OnRobotKilled();

	if ( m_spawnData.m_eType == ROBOT_TYPE_LARGE )
	{
		SetModel( g_RobotData[ m_eType ]->m_pszDamagedModelName );
		ResetSequence( LookupSequence( "idle" ) );
		m_takedamage = DAMAGE_NO;
		SetContextThink( &CTFRobotDestruction_Robot::SpewBarsThink, gpGlobals->curtime, "spew_bars_context" );
		SetContextThink( &CTFRobotDestruction_Robot::SelfDestructThink, gpGlobals->curtime + 5.0f, "self_destruct_think" );

		return;
	}

	SpewBars( m_spawnData.m_nNumGibs );
	SpewGibs();

	CBaseAnimating::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Play our death visual and audio effects
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::PlayDeathEffects()
{
	EmitSound( g_RobotData[ m_eType ]->m_pszDeathSound ); 
	EmitSound( "RD.BotDeathExplosion" );

	DispatchParticleEffect( "rd_robot_explosion", GetAbsOrigin(), vec3_angle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::SpewGibs()
{
	FOR_EACH_VEC( m_aGibs, i )
	{
		CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin() + m_aGibs[i].offset, GetAbsAngles(), this, m_aGibs[i].modelName );
		if ( pAmmoPack )
		{
			pAmmoPack->ActivateWhenAtRest();

			// Calculate the initial impulse on the weapon.
			Vector vecImpulse( random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( 0.75, 1.25 ) );
			VectorNormalize( vecImpulse );

			bool bIsHeadGib = FStrEq( "models/bots/bot_worker/bot_worker_head_gib.mdl", m_aGibs[i].modelName );
			if ( bIsHeadGib ) 
			{	//Shoot straight up
				vecImpulse[2] = 3.0f;
				vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_max.GetFloat() * 0.75, tf_obj_gib_velocity_max.GetFloat() );
			}
			else
			{
				vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_min.GetFloat(), tf_obj_gib_velocity_max.GetFloat() );
			}

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			if ( pAmmoPack->VPhysicsGetObject() )
			{
				AngularImpulse angImpulse( 0.f, random->RandomFloat( 0.f, 100.f ), 0.f );
				if ( bIsHeadGib )
				{	// Speeeeeeeen!
					angImpulse = AngularImpulse( RandomFloat( -60.f, 60.f ), RandomFloat( -60.f, 60.f ), 100000.f );
				}
				pAmmoPack->VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
			}

			pAmmoPack->SetInitialVelocity( vecImpulse );

			pAmmoPack->m_nSkin = Min( 0, GetTeamNumber() - 2 );

			// Give the ammo pack some health, so that trains can destroy it.
			pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pAmmoPack->m_takedamage = DAMAGE_YES;
			pAmmoPack->SetHealth( 900 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::SpewBars( int nNumToSpew )
{
	for ( int i=0; i < nNumToSpew; ++i )
	{
		CBonusPack *pBonusPack = static_cast<CBonusPack *>( CreateEntityByName("item_bonuspack") );
		if ( pBonusPack )
		{
			pBonusPack->ChangeTeam( GetEnemyTeam( GetTeamNumber() ) );
			pBonusPack->SetDisabled( false );
			pBonusPack->SetAbsOrigin( GetAbsOrigin() + Vector( 0, 0, 20 ) );
			pBonusPack->SetAbsAngles( QAngle( 0.f, RandomFloat( 0, 360.f ), 0.f ) );

			// Calculate the initial impulse on the weapon
			Vector vecImpulse( random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( 1.0, 1.25 ) );
			VectorNormalize( vecImpulse );
			vecImpulse *= random->RandomFloat( 125.f, 150.f );

			// Cap the impulse.
			float flSpeed = vecImpulse.Length();
			if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
			{
				VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
			}

			pBonusPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pBonusPack->AddSpawnFlags( SF_NORESPAWN );
			pBonusPack->m_nSkin = Min( 0, GetTeamNumber() - 2 );

			DispatchSpawn( pBonusPack );
			pBonusPack->DropSingleInstance( vecImpulse, NULL, 0, 0 );
			pBonusPack->SetCycle( RandomFloat( 0.f, 1.f ) );
			pBonusPack->SetGravity( 0.2f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFRobotDestruction_Robot::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Check teams
	if ( info.GetAttacker() )
	{
		if ( InSameTeam( info.GetAttacker() ) )
			return 0;

		CBasePlayer *pAttacker = ToBasePlayer( info.GetAttacker() );
		if ( pAttacker )
		{
			pAttacker->SetLastObjectiveTime( gpGlobals->curtime );
		}
	}

	SetContextThink( &CTFRobotDestruction_Robot::RepairSelfThink, gpGlobals->curtime + 5.0f, "RepairSelfThink" );

	if ( m_bShielded )
		return 0;

	CTakeDamageInfo newInfo{info};
	ModifyDamage( &newInfo );

	Vector vecDamagePos = newInfo.GetDamagePosition();
	QAngle vecDamageAngles;
	VectorAngles( -newInfo.GetDamageForce(), vecDamageAngles );
	if ( vecDamagePos == vec3_origin )
	{
		vecDamagePos = WorldSpaceCenter();
	}

	// Play a spark effect
	DispatchParticleEffect( "rd_bot_impact_sparks", vecDamagePos, vecDamageAngles );

	Vector vecImpulse( newInfo.GetDamageForce() );
	m_body->Impulse( vecImpulse.Normalized() * 20.f );

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_robot_impact" );
	if ( event )
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "impulse_x", vecImpulse.x );
		event->SetInt( "impulse_y", vecImpulse.y );
		event->SetInt( "impulse_z", vecImpulse.z );

		gameeventmanager->FireEvent( event );
	}

	if ( m_hRobotGroup )
	{
		m_hRobotGroup->OnRobotAttacked();
	}

	int nResult = BaseClass::OnTakeDamage( newInfo );

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->RobotAttacked( this );

	return nResult;
}

//-----------------------------------------------------------------------------
// Purpose: Scale damage based on attacker
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::ModifyDamage( CTakeDamageInfo *info ) const
{
	CTFPlayer *pAttacker = ToTFPlayer( info->GetAttacker() );
	if ( pAttacker )
	{
		float flScale = 1.0f;
		switch( pAttacker->GetPlayerClass()->GetClassIndex() )
		{
			case TF_CLASS_SCOUT:
				flScale = 1.5f;
				break;
			case TF_CLASS_SNIPER:
				flScale = 2.25f;
				break;
			case TF_CLASS_SPY:
			case TF_CLASS_MEDIC:
				flScale = 2.0f;
				break;
			case TF_CLASS_PYRO:
			case TF_CLASS_HEAVYWEAPONS:
				flScale = 0.75;
				break;
		}

		info->SetDamage( info->GetDamage() * flScale );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Prevent team damage here so blood doesn't appear
	if ( inputInfo.GetAttacker() && InSameTeam( inputInfo.GetAttacker() ) )
		return;

	AddMultiDamage( inputInfo, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::InputStopAndUseComputer( inputdata_t &inputdata )
{
}

void CTFRobotDestruction_Robot::EnableUber()
{
	m_bShielded = true;
	m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 2 : 3;

	if ( m_hRobotGroup )
	{
		m_hRobotGroup->UpdateState();
	}
}

void CTFRobotDestruction_Robot::DisableUber()
{
	m_bShielded = false;
	m_nSkin = GetTeamNumber() == TF_TEAM_RED ? 0 : 1;

	if ( m_hRobotGroup )
	{
		m_hRobotGroup->UpdateState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::ArriveAtPath( void )
{
	variant_t null;
	m_hGoalPath->AcceptInput( "InPass", this, this, null, 0 );
	m_hGoalPath = m_hGoalPath->GetNext();
}

//-----------------------------------------------------------------------------
// Purpose: Repair ourselves!
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::RepairSelfThink()
{
	int nHealth = GetHealth();
	if ( tf_rd_robot_repair_rate.GetFloat() != 0.0f )
	{
		nHealth += GetMaxHealth() / tf_rd_robot_repair_rate.GetFloat();
	}

	nHealth = Min( nHealth, GetMaxHealth() );
	SetHealth( nHealth );

	// Continue to heal if we're still hurt
	if ( GetHealth() != GetMaxHealth() )
	{
		SetContextThink( &CTFRobotDestruction_Robot::RepairSelfThink, gpGlobals->curtime + 1.0f, "RepairSelfThink" );
	}
}

void CTFRobotDestruction_Robot::SpewBarsThink()
{
	++m_nBarsSpewed;
	SpewBars( 1 );

	if ( m_nBarsSpewed >= m_spawnData.m_nNumGibs )
	{
		SelfDestructThink();
	}
	else
	{
		SetContextThink( &CTFRobotDestruction_Robot::SpewBarsThink, gpGlobals->curtime + 0.1f, "spew_bars_context" );
	}
}

void CTFRobotDestruction_Robot::SelfDestructThink()
{
	SpewGibs();
	SpewBars( m_spawnData.m_nNumGibs - m_nBarsSpewed );
	PlayDeathEffects();
	UTIL_Remove( this );
}
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::FireGameEvent( IGameEvent *event )
{
	const char *pszName = event->GetName();
	if ( FStrEq( pszName, "rd_robot_impact" ) )
	{
		const int index_ = event->GetInt( "entindex" );
		if ( index_ == entindex() )
		{
			Vector vecImpulse( event->GetFloat( "impulse_x" ), event->GetFloat( "impulse_y" ), event->GetFloat( "impulse_z" ) );
			m_body->Impulse( vecImpulse );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateDamagedEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *CTFRobotDestruction_Robot::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	BuildPropList( "spawn", m_aSpawnProps, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Play damaged effects
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::UpdateDamagedEffects()
{
	// Start playing our damaged particle if we're damaged
	bool bLowHealth = GetHealth() <= ( GetMaxHealth() * 0.5 );
	if ( bLowHealth && !m_hDamagedEffect )
	{
		m_hDamagedEffect = ParticleProp()->Create( "sentrydamage_4", PATTACH_ABSORIGIN_FOLLOW, 0, Vector( 0, 0, 50.0f ) );
	}
	else if ( !bLowHealth && m_hDamagedEffect )
	{
		ParticleProp()->StopEmission( m_hDamagedEffect );
		m_hDamagedEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_RD_ROBOT_POP_PANELS_OFF )
	{
		CUtlVector<breakmodel_t> vecProp;
		FOR_EACH_VEC( m_aSpawnProps, i )
		{
			char szLowerName[MAX_PATH]{0};
			Q_snprintf( szLowerName, sizeof( szLowerName ), "%s", options );
			Q_strlower( szLowerName );
			if ( Q_strstr( m_aSpawnProps[i].modelName, szLowerName ) )
			{
				vecProp.AddToTail( m_aSpawnProps[i] );
			}
		}

		if ( vecProp.Count() )
		{
			Vector vForward, vRight, vUp;
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );

			Vector vecBreakVelocity = Vector( 0, 0, 200.0f );
			AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
			Vector vecOrigin = GetAbsOrigin() + (vForward * 70.0f) + (vUp * 10.0f);
			QAngle vecAngles = GetAbsAngles();
			breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
			breakParams.impactEnergyScale = 1.0f;
			breakParams.defBurstScale = 3.0f;

			CreateGibsFromList( vecProp, GetModelIndex(), NULL, breakParams, this, -1 , false, true );
		}
	}

	BaseClass::FireEvent( origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestruction_Robot::UpdateClientSideAnimation( void )
{
	m_body->Update();

	BaseClass::UpdateClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: Specify where our healthbars should go over our heads
//-----------------------------------------------------------------------------
float CTFRobotDestruction_Robot::GetHealthBarHeightOffset() const
{
	return g_RobotData[ m_eType ]->m_flHealthBarOffset;
}

#endif
