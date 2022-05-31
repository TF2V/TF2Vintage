//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_TF_OBJECTIVE_RESOURCE_H
#define C_TF_OBJECTIVE_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>
#include "c_team_objectiveresource.h"

class C_TFObjectiveResource : public C_BaseTeamObjectiveResource
{
	DECLARE_CLASS( C_TFObjectiveResource, C_BaseTeamObjectiveResource );
public:
	DECLARE_CLIENTCLASS();

					C_TFObjectiveResource();
	virtual			~C_TFObjectiveResource();

	const char		*GetGameSpecificCPCappingSwipe( int index, int iCappingTeam );
	const char		*GetGameSpecificCPBarFG( int index, int iOwningTeam );
	const char		*GetGameSpecificCPBarBG( int index, int iCappingTeam );
	void			SetCappingTeam( int index, int team );

	int				GetMannVsMachineMaxWaveCount( void ) { return m_nMannVsMachineMaxWaveCount; }
	int				GetMannVsMachineWaveCount( void ) { return m_nMannVsMachineWaveCount; }
	int				GetMannVsMachineWaveEnemyCount( void ) { return m_nMannVsMachineWaveEnemyCount; }
	int				GetMvMInWorldMoney( void ) { return m_nMvMWorldMoney; }

	float			GetMannVsMachineNextWaveTime( void ) { return m_flMannVsMachineNextWaveTime; }
	bool			GetMannVsMachineIsBetweenWaves( void ) { return m_bMannVsMachineBetweenWaves; }

	int				GetMannVsMachineWaveClassCount( int nIndex ) { return m_nMannVsMachineWaveClassCounts[ nIndex ]; }
	const char		*GetMannVsMachineWaveClassName( int nIndex ) { return m_iszMannVsMachineWaveClassNames[ nIndex ]; }
	unsigned int	GetMannVsMachineWaveClassFlags( int nIndex ) { return m_nMannVsMachineWaveClassFlags[ nIndex ]; }
	bool			GetMannVsMachineWaveClassActive( int nIndex ) { return m_bMannVsMachineWaveClassActive[ nIndex ]; }

	int				GetFlagCarrierUpgradeLevel( void ) { return m_nFlagCarrierUpgradeLevel; }
	float			GetBaseMvMBombUpgradeTime( void ) { return m_flMvMBaseBombUpgradeTime; }
	float			GetNextMvMBombUpgradeTime( void ) { return m_flMvMNextBombUpgradeTime; }

	int				GetMvMChallengeIndex( void ) { return m_iChallengeIndex; }
	char const		*GetMvMPopFileName( void ) const { return m_iszMvMPopfileName; }
	int				GetMvMEventPopfileType( void ) { return m_nMvMEventPopfileType; }

private:
	int m_nMannVsMachineMaxWaveCount;
	int m_nMannVsMachineWaveCount;
	int m_nMannVsMachineWaveEnemyCount;

	int m_nMvMWorldMoney;

	float m_flMannVsMachineNextWaveTime;
	bool m_bMannVsMachineBetweenWaves;

	int m_nMannVsMachineWaveClassCounts[ MVM_CLASS_TYPES_PER_WAVE_MAX ];
	char m_iszMannVsMachineWaveClassNames[ MVM_CLASS_TYPES_PER_WAVE_MAX ][64];
	unsigned int m_nMannVsMachineWaveClassFlags[ MVM_CLASS_TYPES_PER_WAVE_MAX ];
	bool m_bMannVsMachineWaveClassActive[ MVM_CLASS_TYPES_PER_WAVE_MAX ];

	int m_nFlagCarrierUpgradeLevel;
	float m_flMvMBaseBombUpgradeTime;
	float m_flMvMNextBombUpgradeTime;

	int m_iChallengeIndex;
	char m_iszMvMPopfileName[ MAX_PATH ];
	int m_nMvMEventPopfileType;
};

inline C_TFObjectiveResource *TFObjectiveResource()
{
	return static_cast<C_TFObjectiveResource*>(g_pObjectiveResource);
}

#endif // C_TF_OBJECTIVE_RESOURCE_H
