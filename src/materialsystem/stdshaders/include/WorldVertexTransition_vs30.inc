// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#ifndef WORLDVERTEXTRANSITION_VS30_H
#define WORLDVERTEXTRANSITION_VS30_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class WorldVertexTransition_vs30_Static_Index
{
public:
	WorldVertexTransition_vs30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_WorldVertexTransition_vs30 1


class WorldVertexTransition_vs30_Dynamic_Index
{
	unsigned int m_nDOWATERFOG : 2;
#ifdef _DEBUG
	bool m_bDOWATERFOG : 1;
#endif	// _DEBUG
public:
	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif	// _DEBUG
	}

	WorldVertexTransition_vs30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
		m_nDOWATERFOG = 0;
#ifdef _DEBUG
		m_bDOWATERFOG = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bDOWATERFOG );
		return ( 1 * m_nDOWATERFOG ) + 0;
	}
};

#define shaderDynamicTest_WorldVertexTransition_vs30 vsh_forgot_to_set_dynamic_DOWATERFOG


#endif	// WORLDVERTEXTRANSITION_VS30_H