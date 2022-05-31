//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#ifndef TF_MANN_VS_MACHINE_LOGIC_H
#define TF_MANN_VS_MACHINE_LOGIC_H

#include "tf_population_manager.h"

class CMannVsMachineLogic : public CPointEntity
{
	DECLARE_CLASS( CMannVsMachineLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CMannVsMachineLogic();
	virtual ~CMannVsMachineLogic();

	virtual void Spawn( void );
	void Update( void );

	void SetupOnRoundStart( void );

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

private:
	void InitPopulationManager( void );

	CHandle<CPopulationManager> m_populationManager;
	float m_flNextAlarmCheck;
};

extern CHandle<CMannVsMachineLogic> g_hMannVsMachineLogic;

#endif
