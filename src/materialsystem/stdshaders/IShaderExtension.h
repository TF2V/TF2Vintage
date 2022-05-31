//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

#ifndef ISHADEREXTENSION_H
#define ISHADEREXTENSION_H
#pragma once

#include "materialsystem/imaterialsystem.h"

struct UberlightState_t
{
	UberlightState_t()
	{
		m_bEnabled = false;
		m_fNearEdge = 2.0f;
		m_fFarEdge = 100.0f;
		m_fCutOn = 10.0f;
		m_fCutOff = 650.0f;
		m_fShearx = 0.0f;
		m_fSheary = 0.0f;
		m_fWidth = 0.3f;
		m_fWedge = 0.05f;
		m_fHeight = 0.3f;
		m_fHedge = 0.05f;
		m_fRoundness = 0.8f;
	}

	bool  m_bEnabled;
	float m_fNearEdge;
	float m_fFarEdge;
	float m_fCutOn;
	float m_fCutOff;
	float m_fShearx;
	float m_fSheary;
	float m_fWidth;
	float m_fWedge;
	float m_fHeight;
	float m_fHedge;
	float m_fRoundness;

	IMPLEMENT_OPERATOR_EQUAL( UberlightState_t );
};

class IShaderExtension
{
public:
	virtual void SetUberlightParamsForFlashlightState( FlashlightState_t&, const UberlightState_t & ) = 0;
	virtual void OnFlashlightStateDestroyed( const FlashlightState_t& ) = 0;

protected:
	virtual ~IShaderExtension() {};
};

#define SHADEREXTENSION_INTERFACE_VERSION "IShaderExtension001"

#endif // ISHADEREXTENSION_H