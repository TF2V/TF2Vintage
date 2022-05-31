//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_mann_vs_machine_logic.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "func_flagdetectionzone.h"
#include "tf_population_manager.h"


BEGIN_DATADESC( CMannVsMachineLogic )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_mann_vs_machine, CMannVsMachineLogic );

CHandle<CMannVsMachineLogic> g_hMannVsMachineLogic;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineLogic::CMannVsMachineLogic()
{
	InitPopulationManager();

	m_flNextAlarmCheck = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMannVsMachineLogic::~CMannVsMachineLogic()
{
	g_hMannVsMachineLogic = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineLogic::Spawn( void )
{
	BaseClass::Spawn();

	SetThink( &CMannVsMachineLogic::Update );
	SetNextThink( gpGlobals->curtime );

	g_hMannVsMachineLogic = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineLogic::SetupOnRoundStart( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( m_populationManager )
	{
		m_populationManager->SetupOnRoundStart();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineLogic::Update( void )
{
	VPROF_BUDGET( "CMannVsMachineLogic::Update", VPROF_BUDGETGROUP_GAME );

	SetNextThink( gpGlobals->curtime + 0.05f );

	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( m_populationManager )
	{
		m_populationManager->Update();
	}

	if ( m_flNextAlarmCheck < gpGlobals->curtime )
	{
		m_flNextAlarmCheck = gpGlobals->curtime + 0.1;

		for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
		{
			CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
			if ( pFlag->IsStolen() )
			{
				for ( int j=0; j<IFlagDetectionZoneAutoList::AutoList().Count(); ++j )
				{
					CFlagDetectionZone *pZone = static_cast<CFlagDetectionZone *>( IFlagDetectionZoneAutoList::AutoList()[j] );
					if ( !pZone->IsDisabled() && pZone->IsAlarmZone() && pZone->PointIsWithin( pFlag->GetAbsOrigin() ) )
					{
						// Is the alarm currently off?
						if ( !TFGameRules()->GetMannVsMachineAlarmStatus() )
						{
							IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_alarm_triggered" );
							if ( event )
							{
								//	WEE WOO WEE WOO WEE WOO
								gameeventmanager->FireEvent( event );
							}
						}

						TFGameRules()->SetMannVsMachineAlarmStatus( true );
						return;
					}
				}
			}
		}

		TFGameRules()->SetMannVsMachineAlarmStatus( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMannVsMachineLogic::InitPopulationManager(void)
{
	bool bFound = false;
	char szFileName[MAX_PATH] ={0};

	if ( TFGameRules()->GetNextMvMPopfile()[0] != '\0' )
	{
		Q_snprintf( szFileName, sizeof( szFileName ), "scripts/population/%s.pop", TFGameRules()->GetNextMvMPopfile() );
		if ( g_pFullFileSystem->FileExists( szFileName, "GAME" ) )
		{
			bFound = true;
		}
		else
		{
			// It might need the additional map prefix to find the file
			Q_snprintf( szFileName, sizeof( szFileName ), "scripts/population/%s_%s.pop", STRING( gpGlobals->mapname ), TFGameRules()->GetNextMvMPopfile() );
			if ( g_pFullFileSystem->FileExists( szFileName, "GAME" ) )
				bFound = true;
			
			if ( !bFound )
				Warning( "Population file '%s' not found", TFGameRules()->GetNextMvMPopfile() );
		}
	}

	// Fallback to defaults
	if ( !bFound )
	{
		CUtlVector<CUtlString> defaultPopFileList;
		g_pPopulationManager->FindDefaultPopulationFileShortNames( defaultPopFileList );
		if ( !defaultPopFileList.IsEmpty() )
		{
			CUtlString defaultPopFileName;
			if ( g_pPopulationManager->FindPopulationFileByShortName( defaultPopFileList[0], &defaultPopFileName ) )
			{
				V_strncpy( szFileName, defaultPopFileName.Get(), sizeof( szFileName ) );
				bFound = true;
			}
		}
	}

	if ( !bFound )
		Q_snprintf( szFileName, sizeof( szFileName ), "scripts/population/%s.pop", STRING( gpGlobals->mapname ) );

	// TODO: This is a bit weird, they check that m_populationManager is null and if so, create one
	// but they also use g_pPopulationManager and never validate it
	if ( m_populationManager && !FStrEq( m_populationManager->GetPopulationFilename(), szFileName ) )
	{
		UTIL_RemoveImmediate( m_populationManager );
		m_populationManager = NULL;
	}

	if ( !m_populationManager )
	{
		m_populationManager = (CPopulationManager *)CreateEntityByName( "info_populator" );
		m_populationManager->SetPopulationFilename( szFileName );
	}

	TFGameRules()->SetNextMvMPopfile( "" );
}