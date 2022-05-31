// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#ifndef LIGHT_VOLUMETRICS_VS30_H
#define LIGHT_VOLUMETRICS_VS30_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class light_volumetrics_vs30_Static_Index
{
public:
	light_volumetrics_vs30_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_light_volumetrics_vs30 1


class light_volumetrics_vs30_Dynamic_Index
{
public:
	light_volumetrics_vs30_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_light_volumetrics_vs30 1


#endif	// LIGHT_VOLUMETRICS_VS30_H