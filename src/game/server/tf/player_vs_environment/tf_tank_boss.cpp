//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_tank_boss.h"
#include "particle_parse.h"
#include "eventqueue.h"
#include "tf_gamerules.h"
#include "tf_objective_resource.h"
#include "tf_population_manager.h"
#include "engine/IEngineSound.h"
//#include "tf_mann_vs_machine_stats.h"


#define TANK_DAMAGE_MODEL_COUNT 4
#define TANK_DEFAULT_HEALTH 1000


static const char *s_TankModel[ TANK_DAMAGE_MODEL_COUNT ] =
{
	"models/bots/boss_bot/boss_tank.mdl",
	"models/bots/boss_bot/boss_tank_damage1.mdl",
	"models/bots/boss_bot/boss_tank_damage2.mdl",
	"models/bots/boss_bot/boss_tank_damage3.mdl"
};

static const char *s_TankModelRome[ TANK_DAMAGE_MODEL_COUNT ] =
{
	"models/bots/tw2/boss_bot/boss_tank.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage1.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage2.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage3.mdl"
};


float CTFTankBoss::sm_flLastTankAlert = 0.0f;


class CTFTankDestruction : public CBaseAnimating
{
	DECLARE_CLASS( CTFTankDestruction, CBaseAnimating );
public:
	DECLARE_DATADESC();

	CTFTankDestruction( void );

	virtual void Precache( void );
	virtual void Spawn( void );

	void AnimThink( void );

	float m_flVanishTime;
	bool m_bIsAtCapturePoint;
	int m_nDeathAnimPick;
	char m_szDeathPostfix[8];
};

PRECACHE_REGISTER( tank_destruction );

BEGIN_DATADESC( CTFTankDestruction )
	DEFINE_THINKFUNC( AnimThink ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tank_destruction, CTFTankDestruction );


CTFTankDestruction::CTFTankDestruction( void )
{
	m_szDeathPostfix[0] = '\0';
}

void CTFTankDestruction::Precache( void )
{
	PrecacheModel( "models/bots/boss_bot/boss_tank_part1_destruction.mdl" );
	PrecacheModel( "models/bots/tw2/boss_bot/boss_tank_part1_destruction.mdl" );

	PrecacheParticleSystem( "explosionTrail_seeds_mvm" );
	PrecacheParticleSystem( "fluidSmokeExpl_ring_mvm" );

	PrecacheScriptSound( "MVM.TankExplodes" );

	BaseClass::Precache();
}

void CTFTankDestruction::Spawn( void )
{
	SetModel( "models/bots/boss_bot/boss_tank_part1_destruction.mdl" );
	SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( "models/bots/boss_bot/boss_tank_part1_destruction.mdl" ) );
	SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( "models/bots/tw2/boss_bot/boss_tank_part1_destruction.mdl" ) );

	BaseClass::Spawn();

	int nDestroySequence = -1;
	int nDeathAnimPick = ( m_nDeathAnimPick != 0 ? m_nDeathAnimPick : RandomInt( 1, 3 ) );
	if ( m_bIsAtCapturePoint )
		nDestroySequence = LookupSequence( UTIL_VarArgs( "destroy_%s%i%s", gpGlobals->mapname.ToCStr(), nDeathAnimPick, m_szDeathPostfix ) );
	if ( nDestroySequence == -1 )
		nDestroySequence = LookupSequence( UTIL_VarArgs( "destroy%i", nDeathAnimPick ) );

	if ( nDestroySequence != -1 )
	{
		SetSequence( nDestroySequence );
		SetPlaybackRate( 1.0f );
		SetCycle( 0 );
	}

	DispatchParticleEffect( "explosionTrail_seeds_mvm", GetAbsOrigin(), GetAbsAngles() );
	DispatchParticleEffect( "fluidSmokeExpl_ring_mvm", GetAbsOrigin(), GetAbsAngles() );

	StopSound( "MVM.TankEngineLoop" );

	CBroadcastRecipientFilter filter;
	const Vector vecOrigin = GetAbsOrigin();
	CBaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM.TankExplodes", &vecOrigin );
	CBaseEntity::EmitSound( filter, SOUND_FROM_WORLD, "MVM.TankEnd" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START );

	m_flVanishTime = gpGlobals->curtime + SequenceDuration();

	SetThink( &CTFTankDestruction::AnimThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( nDestroySequence == -1 )
	{
		SetThink( &CTFTankDestruction::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime );
	}
}

void CTFTankDestruction::AnimThink( void )
{
	if ( m_flVanishTime < gpGlobals->curtime )
	{
		SetThink( &CTFTankDestruction::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime );
		return;
	}

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTankBossBody::CTFTankBossBody( INextBot *bot )
	: IBody( bot )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBossBody::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	// move the animation ahead in time	
	me->StudioFrameAdvance();
	me->DispatchAnimEvents( me );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
unsigned int CTFTankBossBody::GetSolidMask( void ) const
{
	return CONTENTS_SOLID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTankBossBody::StartSequence( const char *name )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	int iSequence = me->LookupSequence( name );
	if ( iSequence )
	{
		me->SetSequence( iSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBossBody::SetSkin( int nSkin )
{
	GetBot()->GetEntity()->m_nSkin = nSkin;
}


IMPLEMENT_SERVERCLASS_ST( CTFTankBoss, DT_TFTankBoss)

END_SEND_TABLE()


BEGIN_DATADESC( CTFTankBoss )

	DEFINE_THINKFUNC( TankBossThink ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "DestroyIfAtCapturePoint", InputDestroyIfAtCapturePoint ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AddCaptureDestroyPostfix", InputAddCaptureDestroyPostfix ),

END_DATADESC()

PRECACHE_REGISTER( tank_boss );
LINK_ENTITY_TO_CLASS( tank_boss, CTFTankBoss );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTankBoss::CTFTankBoss()
{
	m_body = new CTFTankBossBody( this );
	m_iExhaustAttachment = -1;
	m_szDeathPostfix[0] = '\0';
	m_pWaveSpawnPopulator = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTankBoss::~CTFTankBoss()
{
	if( m_body )
		delete m_body;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( "models/bots/boss_bot/bomb_mechanism.mdl" );
	PrecacheModel( "models/bots/boss_bot/tank_track_L.mdl" );
	PrecacheModel( "models/bots/boss_bot/tank_track_R.mdl" );

	PrecacheModel( "models/bots/tw2/boss_bot/bomb_mechanism.mdl" );
	PrecacheModel( "models/bots/tw2/boss_bot/tank_track_L.mdl" );
	PrecacheModel( "models/bots/tw2/boss_bot/tank_track_R.mdl" );

	PrecacheParticleSystem( "smoke_train" );
	PrecacheParticleSystem( "bot_impact_light" );
	PrecacheParticleSystem( "bot_impact_heavy" );

	PrecacheScriptSound( "MVM.TankEngineLoop" );
	PrecacheScriptSound( "MVM.TankPing" );
	PrecacheScriptSound( "MVM.TankDeploy" );
	PrecacheScriptSound( "MVM.TankStart" );
	PrecacheScriptSound( "MVM.TankEnd" );
	PrecacheScriptSound( "MVM.TankSmash" );

	for( int i=0; i<TANK_DAMAGE_MODEL_COUNT; ++i )
	{
		PrecacheModel( s_TankModel[i] );
		PrecacheModel( s_TankModelRome[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::Spawn( void )
{
	if ( ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() ) && GetInitialHealth() == 0 )
	{
		if ( GetHealth() > 0 )
			SetInitialHealth( GetHealth() );
		else
			SetInitialHealth( TANK_DEFAULT_HEALTH );
	}

	BaseClass::Spawn();

	m_vecCollisionMins.Init();
	m_vecCollisionMaxs.Init();

	ChangeTeam( TF_TEAM_MVM_BOTS );
	AddGlowEffect();
	SetBloodColor( DONT_BLEED );

	m_iDamageModelIndex = 0;
	SetModel( s_TankModel[ 0 ] );
	SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( s_TankModel[ 0 ] ) );
	SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( s_TankModelRome[ 0 ] ) );

	m_nLastHealth = GetMaxHealth();
	m_flCreateTime = gpGlobals->curtime;

	m_hLeftTrack = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_hLeftTrack )
	{
		m_hLeftTrack->SetModel( "models/bots/boss_bot/tank_track_L.mdl" );
		m_hLeftTrack->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( "models/bots/boss_bot/tank_track_L.mdl" ) );
		m_hLeftTrack->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( "models/bots/tw2/boss_bot/tank_track_L.mdl" ) );

		m_hLeftTrack->FollowEntity( this, true );

		int iSequence = m_hLeftTrack->LookupSequence( "forward" );
		if ( iSequence )
		{
			m_hLeftTrack->SetSequence( iSequence );
			m_hLeftTrack->SetPlaybackRate( 1.0f );
			m_hLeftTrack->SetCycle( 0 );
			m_hLeftTrack->ResetSequenceInfo();
		}

		m_vecLeftTrackPrevPos = m_hLeftTrack->GetAbsOrigin();
	}		

	m_hRightTrack = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_hRightTrack )
	{
		m_hRightTrack->SetModel( "models/bots/boss_bot/tank_track_R.mdl" );
		m_hRightTrack->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( "models/bots/boss_bot/tank_track_R.mdl" ) );
		m_hRightTrack->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( "models/bots/tw2/boss_bot/tank_track_R.mdl" ) );

		m_hRightTrack->FollowEntity( this, true );

		int iSequence = m_hRightTrack->LookupSequence( "forward" );
		if ( iSequence )
		{
			m_hRightTrack->SetSequence( iSequence );
			m_hRightTrack->SetPlaybackRate( 1.0f );
			m_hRightTrack->SetCycle( 0 );
			m_hRightTrack->ResetSequenceInfo();
		}

		m_vecRightTrackPrevPos = m_hRightTrack->GetAbsOrigin();
	}		

	m_hBomb = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
	if ( m_hBomb )
	{
		m_hBomb->SetModel( "models/bots/boss_bot/bomb_mechanism.mdl" );
		m_hBomb->SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( "models/bots/boss_bot/bomb_mechanism.mdl" ) );
		m_hBomb->SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( "models/bots/tw2/boss_bot/bomb_mechanism.mdl" ) );

		m_hBomb->FollowEntity( this, true );
	}

	m_body->StartSequence( "movement" );

	m_iExhaustAttachment = LookupAttachment( "smoke_attachment" );

	if ( m_hGoalNode == NULL )
	{
		m_hGoalNode = dynamic_cast<CPathTrack *>( gEntList.FindEntityByClassname( NULL, "path_track" ) );

		if ( m_hGoalNode )
		{
			// Find first node of path
			while( m_hGoalNode->GetPrevious() )
			{
				m_hGoalNode = m_hGoalNode->GetPrevious();
			}

			SetAbsOrigin( m_hGoalNode->WorldSpaceCenter() );
		}
	}
	else
	{
		SetAbsOrigin( m_hGoalNode->WorldSpaceCenter() );
	}

	if ( TFGameRules() )
	{
		int nTankCount = 0;

		CBaseEntity *tank = NULL;
		while( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
		{
			nTankCount++;
		}

		if ( nTankCount <= 1 )
		{
			if ( sm_flLastTankAlert + 5.0f < gpGlobals->curtime )
			{
				CWave *pWave = g_pPopulationManager ? g_pPopulationManager->GetCurrentWave() : NULL;
				if ( pWave && pWave->m_nNumTanksSpawned > 1 )
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Another" );
				}
				else
				{
					TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Spawn" );
				}

				sm_flLastTankAlert = gpGlobals->curtime;
			}
		}
		else
		{
			// Don't worry about when the last alert was in this case because 2 tanks can spawn at once
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Alert_Multiple" );
			sm_flLastTankAlert = gpGlobals->curtime;
		}

		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_CALLOUT, TF_TEAM_MVM_PLAYERS );
	}

	m_hEndNode = m_hStartNode = m_hGoalNode;

	if ( m_hGoalNode != NULL )
	{
		CPathTrack *pNextNode = m_hGoalNode->GetNext();
		if ( pNextNode )
		{
			const Vector vecDirection = pNextNode->GetAbsOrigin() - m_hGoalNode->GetAbsOrigin();

			QAngle vecOrientation;
			VectorAngles( vecDirection, vecOrientation );
			SetAbsAngles( vecOrientation );

			CPathTrack *pPrevNode = m_hGoalNode.Get();
			Vector2D vecToTravel ={};
			while ( pNextNode )
			{
				vecToTravel = pNextNode->GetAbsOrigin().AsVector2D() - pPrevNode->GetAbsOrigin().AsVector2D();

				m_flTotalDistance += vecToTravel.Length();
				m_CumulativeDistances.AddToTail( m_flTotalDistance );

				pPrevNode = pNextNode;
				pNextNode = pNextNode->GetNext();
			}
		}
	}

	CBroadcastRecipientFilter filter;
	EmitSound( filter, entindex(), "MVM.TankEngineLoop" );
	EmitSound( "MVM.TankStart" );

	SetThink( &CTFTankBoss::TankBossThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::UpdateOnRemove( void )
{
	StopSound( "MVM.TankEngineLoop" );

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->DecrementMannVsMachineWaveClassCount( MAKE_STRING( "tank" ), MVM_CLASS_FLAG_NORMAL | MVM_CLASS_FLAG_MINIBOSS );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::UpdateCollisionBounds( void )
{
	// Save off starting bounds
	if ( m_vecCollisionMins.IsZero() || m_vecCollisionMaxs.IsZero() )
	{
		m_vecCollisionMins = WorldAlignMins();
		m_vecCollisionMaxs = WorldAlignMaxs();
	}

	const float flAngleCoefficient = 1.0f - fabsf( sin( DEG2RAD( GetAbsAngles().y ) * 2.0f ) ) * 0.4f;

	Vector vecMins = m_vecCollisionMins;
	vecMins.x *= flAngleCoefficient;
	vecMins.y *= flAngleCoefficient;

	Vector vecMaxs = m_vecCollisionMaxs;
	vecMaxs.x *= flAngleCoefficient;
	vecMaxs.y *= flAngleCoefficient;

	VMatrix rot;
	MatrixFromAngles( GetAbsAngles(), rot );

	// Transform collision bounds based on how we're rotated
	TransformAABB( rot.As3x4(), vecMins, vecMaxs, vecMins, vecMaxs );
	CollisionProp()->SetCollisionBounds( vecMins, vecMaxs );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFTankBoss::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( (float)GetHealth() / GetMaxHealth() > 0.3f )
	{
		DispatchParticleEffect( "bot_impact_light", info.GetDamagePosition(), vec3_angle );
	}
	else
	{
		DispatchParticleEffect( "bot_impact_heavy", info.GetDamagePosition(), vec3_angle );
	}

	if ( info.GetAttacker() == nullptr )
		return 0;

	int nResult = BaseClass::OnTakeDamage_Alive( info );
	if ( nResult != 0 )
	{
		// track who damaged us
		CTFPlayer *pTFPlayer = dynamic_cast<CTFPlayer *>( info.GetAttacker() );
		if ( pTFPlayer )
		{
			// Some achievement related logic goes here too

			/*CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats )
			{
				pStats->PlayerEvent_DealtDamageToTanks( pTFPlayer, info.GetDamage() );
			}*/
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::Event_Killed( const CTakeDamageInfo &info )
{
	m_bKilledByPlayers = ( info.GetDamageType() & DMG_CRUSH ) == 0;

	Explode();

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( FStrEq( "mvm_rottenburg", STRING( gpGlobals->mapname ) ) )
		{
			// Achievement stuff goes here
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFTankBoss::GetCurrencyValue( void )
{
	if ( !m_hGoalNode && !m_bKilledByPlayers )
		return 0;

	if ( m_pWaveSpawnPopulator )
		return m_pWaveSpawnPopulator->GetCurrencyAmountPerDeath();

	return BaseClass::GetCurrencyValue();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::TankBossThink( void )
{
	if ( GetHealth() != m_nLastHealth )
	{
		// health changed - potentially change damage model
		m_nLastHealth = GetHealth();

		int healthPerModel = GetMaxHealth() / TANK_DAMAGE_MODEL_COUNT;
		int healthThreshold = GetMaxHealth() - healthPerModel;

		int iDesiredModelIndex;
		for( iDesiredModelIndex = 0; iDesiredModelIndex < TANK_DAMAGE_MODEL_COUNT; ++iDesiredModelIndex )
		{
			if ( GetHealth() > healthThreshold )
			{
				break;
			}

			healthThreshold -= healthPerModel;
		}

		if ( iDesiredModelIndex >= TANK_DAMAGE_MODEL_COUNT )
		{
			iDesiredModelIndex = TANK_DAMAGE_MODEL_COUNT-1;
		}

		if ( iDesiredModelIndex != m_iDamageModelIndex )
		{
			// update model
			const char *pchSequence = GetSequenceName( GetSequence() );
			float fCycle = GetCycle();

			m_iDamageModelIndex = iDesiredModelIndex;

			SetModel( s_TankModel[ iDesiredModelIndex ] );
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( s_TankModel[ iDesiredModelIndex ] ) );
			SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( s_TankModelRome[ iDesiredModelIndex ] ) );

			int iSequence = LookupSequence( pchSequence );
			if ( iSequence > 0 )
			{
				SetSequence( iSequence );
				SetPlaybackRate( 1.0f );
				ResetSequenceInfo();
				SetCycle( fCycle );
			}
			else
			{
				GetBodyInterface()->StartSequence( "movement" );
			}
		}
	}

	Vector vecFwd, vecRight, vecUp;
	GetVectors( &vecFwd, &vecRight, &vecUp );

	const float flTrackMaxSpeed = 80.0f;
	const float flTrackOffset = 56.221f;

	if ( m_hLeftTrack )
	{
		const Vector vecTrackCenter = GetAbsOrigin() - flTrackOffset * vecRight;
		float flSpeed = ( vecTrackCenter - m_vecLeftTrackPrevPos ).Length() / gpGlobals->frametime;

		if ( flSpeed >= flTrackMaxSpeed )
		{
			m_hLeftTrack->SetPlaybackRate( 1.0f );
		}
		else
		{
			m_hLeftTrack->SetPlaybackRate( flSpeed / flTrackMaxSpeed );
		}

		m_vecLeftTrackPrevPos = vecTrackCenter;
	}

	if ( m_hRightTrack )
	{
		const Vector vecTrackCenter = GetAbsOrigin() + flTrackOffset * vecRight;
		float flSpeed = ( vecTrackCenter - m_vecRightTrackPrevPos ).Length() / gpGlobals->frametime;

		if ( flSpeed >= flTrackMaxSpeed )
		{
			m_hRightTrack->SetPlaybackRate( 1.0f );
		}
		else
		{
			m_hRightTrack->SetPlaybackRate( flSpeed / flTrackMaxSpeed );
		}

		m_vecRightTrackPrevPos = vecTrackCenter;
	}

	if ( m_hGoalNode != NULL )
	{
		Vector2D vecToGoal = m_hGoalNode->WorldSpaceCenter().AsVector2D() - GetAbsOrigin().AsVector2D();
		float flDistance = vecToGoal.Length();

		CBaseEntity *pParent = GetParent();
		if ( pParent )
		{
			vecToGoal = m_hGoalNode->WorldSpaceCenter().AsVector2D() - pParent->GetAbsOrigin().AsVector2D();
			flDistance = Min( flDistance, vecToGoal.Length() );
		}

		if ( TFGameRules() )
		{
			if ( m_nNodeNumber > 0 && gpGlobals->curtime > ( sm_flLastTankAlert + 5.0f ) )
			{
				float flDistancePerc = m_CumulativeDistances[m_nNodeNumber] - flDistance / m_flTotalDistance;
				if ( !m_bPlayedNearAlert && flDistancePerc > 0.75f )
				{
					TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Near_Hatch", 5.0f );

					sm_flLastTankAlert = gpGlobals->curtime;
					m_bPlayedNearAlert = true;
				}
				else if ( !m_bPlayedHalfwayAlert && flDistancePerc > 0.5f )
				{
					int nTankCount = 0;
					CBaseEntity *tank = NULL;
					while ( ( tank = gEntList.FindEntityByClassname( tank, "tank_boss" ) ) != NULL )
						nTankCount++;

					if ( nTankCount > 1 )
						TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Halfway_Multiple", 5.0f );
					else
						TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Halfway", 5.0f );

					sm_flLastTankAlert = gpGlobals->curtime;
					m_bPlayedHalfwayAlert = true;
				}
			}

			if ( flDistance < 20.0f )
			{
				// reached node
				inputdata_t dummyData;
				dummyData.pActivator = this;
				dummyData.pCaller = this;
				dummyData.nOutputID = 0;

				m_hGoalNode->InputPass( dummyData );

				m_hGoalNode = m_hGoalNode->GetNext();
				m_nNodeNumber++;

				if ( m_hGoalNode == NULL && m_hBomb )
				{
					int animSequence = m_hBomb->LookupSequence( "deploy" );
					if ( animSequence )
					{
						m_hBomb->SetSequence( animSequence );
						m_hBomb->SetPlaybackRate( 1.0f );
						m_hBomb->SetCycle( 0 );
						m_hBomb->ResetSequenceInfo();
					}

					animSequence = LookupSequence( "deploy" );
					if ( animSequence )
					{
						SetSequence( animSequence );
						SetPlaybackRate( 1.0f );
						SetCycle( 0 );
						ResetSequenceInfo();
					}

					if ( ( sm_flLastTankAlert + 5.0f ) < gpGlobals->curtime )
					{
						TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Tank_Alert_Deploying", 5.0f );

						sm_flLastTankAlert = gpGlobals->curtime;
						m_bPlayedNearAlert = true;
					}

					m_bDroppingBomb = true;
					m_flDroppingStart = gpGlobals->curtime;

					StopSound( "MVM.TankEngineLoop" );

					EmitSound( "MVM.TankDeploy" );

					TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_DEPLOYING, TF_TEAM_MVM_BOTS );
				}
			}
		}
	}

	if ( m_iExhaustAttachment > 0 )
	{
		Vector smokePos;
		GetAttachment( m_iExhaustAttachment, smokePos );

		trace_t result;
		UTIL_TraceLine( smokePos, smokePos + Vector( 0, 0, 300.0f ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &result );

		if ( result.DidHit() )
		{
			if ( m_bSmoking )
			{
				StopParticleEffects( this );
				m_bSmoking = false;
			}
		}
		else if ( !m_bSmoking )
		{
			DispatchParticleEffect( "smoke_train", PATTACH_POINT_FOLLOW, this, m_iExhaustAttachment );
			m_bSmoking = true;
		}
	}

	if ( m_hGoalNode != NULL )
	{
		Vector vecGoal = m_hGoalNode->WorldSpaceCenter();

		GetLocomotionInterface()->SetDesiredSpeed( GetMaxSpeed() );
		GetLocomotionInterface()->Approach( vecGoal );
		GetLocomotionInterface()->FaceTowards( vecGoal );

		if ( m_rumbleTimer.IsElapsed() )
		{
			m_rumbleTimer.Start( 0.25f );

			UTIL_ScreenShake( GetAbsOrigin(), 2.0f, 5.0f, 1.0f, 500.0f, SHAKE_START );
		}
	}

	if ( m_bDroppingBomb && IsSequenceFinished() )
	{
		FirePopFileEvent( &m_onBombDroppedEventInfo );
		TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Tank_Planted" );

		m_bDroppingBomb = false;
	}

	if ( m_crushTimer.IsElapsed() )
	{
		m_crushTimer.Start( 0.5f );
		
		const Vector vecMins = WorldAlignMins() * 0.75f;
		const Vector vecMaxs = WorldAlignMaxs() * 0.75f;

		CBaseEntity *intersectingEntities[64];
		int count = UTIL_EntitiesInBox( intersectingEntities, 64, GetAbsOrigin() + vecMins, GetAbsOrigin() + vecMaxs, FL_CLIENT|FL_OBJECT );
		for ( int i = 0; i < count; ++i )
		{
			CBaseEntity *pVictim = intersectingEntities[i];
			if ( pVictim == NULL || !pVictim->IsAlive() )
				continue;

			int nDamage = MAX( pVictim->GetMaxHealth(), pVictim->GetHealth() );
			pVictim->TakeDamage( CTakeDamageInfo( this, this, 4 * nDamage, DMG_CRUSH, TF_DMG_CUSTOM_NONE ) );
		}
	}

	UpdatePingSound();

	BaseClass::BossThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::InputDestroyIfAtCapturePoint( inputdata_t &inputdata )
{
	m_nDeathAnimPick = inputdata.value.Int();

	if ( m_hGoalNode == NULL )
	{
		CBaseEntity *pWorld = gEntList.FirstEnt();
		TakeDamage( CTakeDamageInfo( pWorld, pWorld, 9999999.9f, DMG_CRUSH, TF_DMG_CUSTOM_NONE ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::InputAddCaptureDestroyPostfix( inputdata_t &inputdata )
{
	V_strncpy( m_szDeathPostfix, inputdata.value.String(), sizeof( m_szDeathPostfix ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::SetStartingPathTrackNode( char *pszName )
{
	m_hGoalNode = dynamic_cast<CPathTrack *>( gEntList.FindEntityByName( NULL, pszName ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::ModifyDamage( CTakeDamageInfo *info )
{
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( info->GetWeapon() );

	if ( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
	{
		// miniguns are too powerful
		info->SetDamage( info->GetDamage() * 0.25 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::Explode( void )
{
	StopSound( "MVM.TankEngineLoop" );
	FirePopFileEvent( &m_onKilledEventInfo );

	CTFTankDestruction *pDestruction = dynamic_cast<CTFTankDestruction *>( CreateEntityByName( "tank_destruction" ) );
	if ( pDestruction )
	{
		// Only do special capture point death if it was force killed by bomb drop
		pDestruction->m_bIsAtCapturePoint = ( m_hGoalNode == NULL && !m_bKilledByPlayers );
		pDestruction->m_nDeathAnimPick = m_nDeathAnimPick;
		V_strncpy( pDestruction->m_szDeathPostfix, m_szDeathPostfix, sizeof pDestruction->m_szDeathPostfix );

		pDestruction->SetAbsOrigin( GetAbsOrigin() );
		pDestruction->SetAbsAngles( GetAbsAngles() );
		DispatchSpawn( pDestruction );
	}

	if ( m_bKilledByPlayers )
	{
		TFGameRules()->BroadcastSound( 255, "Announcer.MVM_General_Destruction" );
		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_TANK_DEAD, TF_TEAM_MVM_BOTS );

		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_tank_destroyed_by_players" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			// Achievement stuff goes here
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::FirePopFileEvent( EventInfo *eventInfo )
{
	if ( eventInfo && eventInfo->m_action.Length() > 0 )
	{
		CBaseEntity *targetEntity = gEntList.FindEntityByName( NULL, eventInfo->m_target );
		if ( !targetEntity )
		{
			Warning( "CTFTankBoss: Can't find target entity '%s' for event\n", eventInfo->m_target.Access() );
		}
		else
		{
			g_EventQueue.AddEvent( targetEntity, eventInfo->m_action, 0.0f, NULL, NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTankBoss::UpdatePingSound( void )
{
	if( gpGlobals->curtime - m_flLastPingTime >= 5.0f )
	{
		m_flLastPingTime = gpGlobals->curtime;
		EmitSound( "MVM.TankPing");
	}
}
