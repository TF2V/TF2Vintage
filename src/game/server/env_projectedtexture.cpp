//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity to control screen overlays on a player
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ENV_PROJECTEDTEXTURE_STARTON				( 1 << 0 )
#define ENV_PROJECTEDTEXTURE_VOLUMETRICS_START_ON	( 1 << 1 )
#define ENV_PROJECTEDTEXTURE_UBERLIGHT				( 1 << 2 )

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CEnvProjectedTexture : public CPointEntity
{
	DECLARE_CLASS( CEnvProjectedTexture, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvProjectedTexture();
	bool KeyValue( const char *szKeyName, const char *szValue );

	// Always transmit to clients
	virtual int UpdateTransmitState();
	void Spawn( void );
	virtual void Activate( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputSetFOV( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata );
	void InputSetCameraSpace( inputdata_t &inputdata );
	void InputSetLightOnlyTarget( inputdata_t &inputdata );
	void InputSetLightWorld( inputdata_t &inputdata );
	void InputSetEnableShadows( inputdata_t &inputdata );
	void InputSetLightColor( inputdata_t &inputdata );
	void InputSetSpotlightTexture( inputdata_t &inputdata );
	void InputSetAmbient( inputdata_t &inputdata );
	void InputSetEnableVolumetrics( inputdata_t &inputdata );
	void InputEnableUberLight( inputdata_t &inputdata );
	void InputDisableUberLight( inputdata_t &inputdata );

	void InitialThink( void );
	void FlickerThink( void );

	CNetworkHandle( CBaseEntity, m_hTargetEntity );

private:

	CNetworkVar( bool, m_bState );
	CNetworkVar( float, m_flLightFOV );
	CNetworkVar( bool, m_bEnableShadows );
	CNetworkVar( bool, m_bLightOnlyTarget );
	CNetworkVar( bool, m_bLightWorld );
	CNetworkVar( bool, m_bCameraSpace );
	CNetworkVector( m_LinearFloatLightColor );
	Vector m_LinearFloatLightColorCopy;
	CNetworkVar( float, m_flAmbient );
	CNetworkString( m_SpotlightTextureName, MAX_PATH );
	CNetworkVar( int, m_nSpotlightTextureFrame );
	CNetworkVar( float, m_flNearZ );
	CNetworkVar( float, m_flFarZ );
	CNetworkVar( int, m_nShadowQuality );
	CNetworkVar( bool, m_bFlicker );

	CNetworkVar( bool, m_bEnableVolumetrics );
	CNetworkVar( bool, m_bEnableVolumetricsLOD );
	CNetworkVar( float, m_flVolumetricsFadeDistance );
	CNetworkVar( int, m_iVolumetricsQuality );
	CNetworkVar( float, m_flVolumetricsQualityBias );
	CNetworkVar( float, m_flVolumetricsMultiplier );

	CNetworkVar( bool, m_bUberlight );
	CNetworkVar( float, m_fNearEdge );
	CNetworkVar( float, m_fFarEdge );
	CNetworkVar( float, m_fCutOn );
	CNetworkVar( float, m_fCutOff );
	CNetworkVar( float, m_fShearx );
	CNetworkVar( float, m_fSheary );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fWedge );
	CNetworkVar( float, m_fHeight );
	CNetworkVar( float, m_fHedge );
	CNetworkVar( float, m_fRoundness );
};

LINK_ENTITY_TO_CLASS( env_projectedtexture, CEnvProjectedTexture );

BEGIN_DATADESC( CEnvProjectedTexture )
	DEFINE_FIELD( m_hTargetEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bState, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flLightFOV, FIELD_FLOAT, "lightfov" ),
	DEFINE_KEYFIELD( m_bEnableShadows, FIELD_BOOLEAN, "enableshadows" ),
	DEFINE_KEYFIELD( m_bLightOnlyTarget, FIELD_BOOLEAN, "lightonlytarget" ),
	DEFINE_KEYFIELD( m_bLightWorld, FIELD_BOOLEAN, "lightworld" ),
	DEFINE_KEYFIELD( m_bCameraSpace, FIELD_BOOLEAN, "cameraspace" ),
	DEFINE_KEYFIELD( m_flAmbient, FIELD_FLOAT, "ambient" ),
	DEFINE_KEYFIELD( m_bFlicker, FIELD_BOOLEAN, "flicker" ),
	DEFINE_AUTO_ARRAY( m_SpotlightTextureName, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe" ),
	DEFINE_KEYFIELD( m_flNearZ, FIELD_FLOAT, "nearz" ),
	DEFINE_KEYFIELD( m_flFarZ, FIELD_FLOAT, "farz" ),
	DEFINE_KEYFIELD( m_nShadowQuality, FIELD_INTEGER, "shadowquality" ),
	DEFINE_FIELD( m_LinearFloatLightColor, FIELD_VECTOR ),

	DEFINE_FIELD( m_bEnableVolumetrics, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bEnableVolumetricsLOD, FIELD_BOOLEAN, "volumetricslod" ),
	DEFINE_KEYFIELD( m_flVolumetricsFadeDistance, FIELD_FLOAT, "volumetricsfadedistance" ),
	DEFINE_KEYFIELD( m_iVolumetricsQuality, FIELD_INTEGER, "volumetricsquality" ),
	DEFINE_KEYFIELD( m_flVolumetricsQualityBias, FIELD_FLOAT, "volumetricsqualitybias" ),
	DEFINE_KEYFIELD( m_flVolumetricsMultiplier, FIELD_FLOAT, "volumetricsmultiplier" ),

	DEFINE_FIELD( m_bUberlight, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_fNearEdge, FIELD_FLOAT, "uberlight_near" ),
	DEFINE_KEYFIELD( m_fFarEdge, FIELD_FLOAT, "uberlight_far" ),
	DEFINE_KEYFIELD( m_fCutOn, FIELD_FLOAT, "uberlight_cuton" ),
	DEFINE_KEYFIELD( m_fCutOff, FIELD_FLOAT, "uberlight_cutoff" ),
	DEFINE_KEYFIELD( m_fShearx, FIELD_FLOAT, "uberlight_shearx" ),
	DEFINE_KEYFIELD( m_fSheary, FIELD_FLOAT, "uberlight_sheary" ),
	DEFINE_KEYFIELD( m_fWidth, FIELD_FLOAT, "uberlight_width" ),
	DEFINE_KEYFIELD( m_fWedge, FIELD_FLOAT, "uberlight_wedge" ),
	DEFINE_KEYFIELD( m_fHeight, FIELD_FLOAT, "uberlight_height" ),
	DEFINE_KEYFIELD( m_fHedge, FIELD_FLOAT, "uberlight_hedge" ),
	DEFINE_KEYFIELD( m_fRoundness, FIELD_FLOAT, "uberlight_roundness" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "FOV", InputSetFOV ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "Target", InputSetTarget ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "CameraSpace", InputSetCameraSpace ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "LightOnlyTarget", InputSetLightOnlyTarget ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "LightWorld", InputSetLightWorld ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "EnableShadows", InputSetEnableShadows ),
	// this is broken . . need to be able to set color and intensity like light_dynamic
	//	DEFINE_INPUTFUNC( FIELD_COLOR32, "LightColor", InputSetLightColor ),
	DEFINE_INPUTFUNC( FIELD_STRING, "LightColor", InputSetLightColor ),//TE120
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Ambient", InputSetAmbient ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SpotlightTexture", InputSetSpotlightTexture ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "EnableVolumetrics", InputSetEnableVolumetrics ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableUberLight", InputEnableUberLight ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableUberLight", InputDisableUberLight ),
	DEFINE_THINKFUNC( InitialThink ),
	DEFINE_THINKFUNC( FlickerThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CEnvProjectedTexture, DT_EnvProjectedTexture )
	SendPropEHandle( SENDINFO( m_hTargetEntity ) ),
	SendPropBool( SENDINFO( m_bState ) ),
	SendPropFloat( SENDINFO( m_flLightFOV ) ),
	SendPropBool( SENDINFO( m_bEnableShadows ) ),
	SendPropBool( SENDINFO( m_bLightOnlyTarget ) ),
	SendPropBool( SENDINFO( m_bLightWorld ) ),
	SendPropBool( SENDINFO( m_bCameraSpace ) ),
	SendPropFloat( SENDINFO( m_flAmbient ) ),
	SendPropString( SENDINFO( m_SpotlightTextureName ) ),
	SendPropInt( SENDINFO( m_nSpotlightTextureFrame ) ),
	SendPropFloat( SENDINFO( m_flNearZ ), 16, SPROP_ROUNDDOWN, 0.0f, 500.0f ),
	SendPropFloat( SENDINFO( m_flFarZ ), 18, SPROP_ROUNDDOWN, 0.0f, 1500.0f ),
	SendPropInt( SENDINFO( m_nShadowQuality ), 1, SPROP_UNSIGNED ),  // Just one bit for now

	SendPropBool( SENDINFO( m_bEnableVolumetrics ) ),
	SendPropBool( SENDINFO( m_bEnableVolumetricsLOD ) ),
	SendPropFloat( SENDINFO( m_flVolumetricsFadeDistance ) ),
	SendPropInt( SENDINFO( m_iVolumetricsQuality ) ),
	SendPropFloat( SENDINFO( m_flVolumetricsQualityBias ) ),
	SendPropFloat( SENDINFO( m_flVolumetricsMultiplier ) ),

	SendPropBool( SENDINFO( m_bUberlight ) ),
	SendPropFloat( SENDINFO( m_fNearEdge ) ),
	SendPropFloat( SENDINFO( m_fFarEdge ) ),
	SendPropFloat( SENDINFO( m_fCutOn ) ),
	SendPropFloat( SENDINFO( m_fCutOff ) ),
	SendPropFloat( SENDINFO( m_fShearx ) ),
	SendPropFloat( SENDINFO( m_fSheary ) ),
	SendPropFloat( SENDINFO( m_fWidth ) ),
	SendPropFloat( SENDINFO( m_fWedge ) ),
	SendPropFloat( SENDINFO( m_fHeight ) ),
	SendPropFloat( SENDINFO( m_fHedge ) ),
	SendPropFloat( SENDINFO( m_fRoundness ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CEnvProjectedTexture::CEnvProjectedTexture( void )
{
	m_bState = true;
	m_flLightFOV = 45.0f;
	m_bEnableShadows = false;
	m_bLightOnlyTarget = false;
	m_bLightWorld = true;
	m_bCameraSpace = false;
	m_bUberlight = false;

	// if ( g_pHardwareConfig->SupportsBorderColor() )
#if defined( _X360 )
	Q_strcpy( m_SpotlightTextureName.GetForModify(), "effects/flashlight_border" );
#else
	Q_strcpy( m_SpotlightTextureName.GetForModify(), "effects/flashlight001" );
#endif

	m_nSpotlightTextureFrame = 0;
	m_flAmbient = 0.0f;
	m_flNearZ = 4.0f;
	m_flFarZ = 750.0f;
	m_nShadowQuality = 0;
	m_flVolumetricsQualityBias = 3.0f;
	m_fNearEdge = 0.f;
	m_fFarEdge = 0.f;
	m_fCutOn = 0.f;
	m_fCutOff = 0.f;
	m_fShearx = 0.f;
	m_fSheary = 0.f;
	m_fWidth = 0.f;
	m_fWedge = 0.f;
	m_fHeight = 0.f;
	m_fHedge = 0.f;
	m_fRoundness = 0.f;
}

void UTIL_ColorStringToLinearFloatColor( Vector &color, const char *pString )
{
	float tmp[4];
	UTIL_StringToFloatArray( tmp, 4, pString );
	if ( tmp[3] <= 0.0f )
	{
		tmp[3] = 255.0f;
	}
	tmp[3] *= ( 1.0f / 255.0f );
	color.x = GammaToLinear( tmp[0] * ( 1.0f / 255.0f ) ) * tmp[3];
	color.y = GammaToLinear( tmp[1] * ( 1.0f / 255.0f ) ) * tmp[3];
	color.z = GammaToLinear( tmp[2] * ( 1.0f / 255.0f ) ) * tmp[3];
}

bool CEnvProjectedTexture::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "lightcolor" ) )
	{
		Vector tmp;
		UTIL_ColorStringToLinearFloatColor(tmp, szValue);
		m_LinearFloatLightColor = tmp;
	}
	else if ( FStrEq(szKeyName, "texturename" ) )
	{
		Q_strcpy( m_SpotlightTextureName.GetForModify(), szValue );
	}
	else if ( FStrEq( szKeyName, "texturename" ) )
	{
		if ( !szValue || Q_strlen( szValue ) < 1 )
		{
			// If we have no texture defined use a default texture
			Q_strcpy( m_SpotlightTextureName.GetForModify(), "effects/flashlight001" );
		}
		else
		{
			Q_strcpy( m_SpotlightTextureName.GetForModify(), szValue );
		}
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

void CEnvProjectedTexture::InputTurnOn( inputdata_t &inputdata )
{
	m_bState = true;
}

void CEnvProjectedTexture::InputTurnOff( inputdata_t &inputdata )
{
	m_bState = false;
}

void CEnvProjectedTexture::InputSetFOV( inputdata_t &inputdata )
{
	m_flLightFOV = inputdata.value.Float();
}

void CEnvProjectedTexture::InputSetTarget( inputdata_t &inputdata )
{
	m_hTargetEntity = inputdata.value.Entity();
}

void CEnvProjectedTexture::InputSetCameraSpace( inputdata_t &inputdata )
{
	m_bCameraSpace = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetLightOnlyTarget( inputdata_t &inputdata )
{
	m_bLightOnlyTarget = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetLightWorld( inputdata_t &inputdata )
{
	m_bLightWorld = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetEnableShadows( inputdata_t &inputdata )
{
	m_bEnableShadows = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputSetLightColor( inputdata_t &inputdata )
{
	Vector tmp;
	UTIL_ColorStringToLinearFloatColor( tmp, inputdata.value.String() );
	// DevMsg( "vCLR.x: %f\n", tmp.x );
	m_LinearFloatLightColor = tmp;
}

void CEnvProjectedTexture::InputSetAmbient( inputdata_t &inputdata )
{
	m_flAmbient = inputdata.value.Float();
}

void CEnvProjectedTexture::InputSetSpotlightTexture( inputdata_t &inputdata )
{
	Q_strcpy( m_SpotlightTextureName.GetForModify(), inputdata.value.String() );
}

void CEnvProjectedTexture::Spawn( void )
{
	m_bState = HasSpawnFlags( ENV_PROJECTEDTEXTURE_STARTON );
	m_bEnableVolumetrics = HasSpawnFlags( ENV_PROJECTEDTEXTURE_VOLUMETRICS_START_ON );
	m_bUberlight = HasSpawnFlags( ENV_PROJECTEDTEXTURE_UBERLIGHT );
}

void CEnvProjectedTexture::Activate( void )
{
	SetThink( &CEnvProjectedTexture::InitialThink );
	SetNextThink( gpGlobals->curtime + 0.1 );

	BaseClass::Activate();
}

void CEnvProjectedTexture::InitialThink( void )
{
	if ( m_hTargetEntity == NULL && m_target != NULL_STRING )
		m_hTargetEntity = gEntList.FindEntityByName( NULL, m_target );

	if ( m_hTargetEntity == NULL )
		return;

	Vector vecToTarget = ( m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin() );
	QAngle vecAngles;
	VectorAngles( vecToTarget, vecAngles );
	SetAbsAngles( vecAngles );

	if ( m_bFlicker )
	{
		DevMsg( "CEnvProjectedTexture::InitialThink: m_bFlicker..\n" );
		m_LinearFloatLightColorCopy = m_LinearFloatLightColor;
		SetThink( &CEnvProjectedTexture::FlickerThink );
		SetNextThink( gpGlobals->curtime + 0.05 );
	}
	else
		SetNextThink( gpGlobals->curtime + 0.1 );
}

void CEnvProjectedTexture::FlickerThink( void )
{
	float flNoise = 0.75 + ( 0.25 * ( cosf( gpGlobals->curtime * 7.0 ) * sinf( gpGlobals->curtime * 25.0 ) ) );
	m_LinearFloatLightColor = m_LinearFloatLightColorCopy * flNoise;

	if ( m_bFlicker )
	{
		SetThink( &CEnvProjectedTexture::FlickerThink );
		SetNextThink( gpGlobals->curtime + 0.05 );
	}
}

int CEnvProjectedTexture::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CEnvProjectedTexture::InputSetEnableVolumetrics( inputdata_t &inputdata )
{
	m_bEnableVolumetrics = inputdata.value.Bool();
}

void CEnvProjectedTexture::InputEnableUberLight( inputdata_t &inputdata )
{
	m_bUberlight = true;
}

void CEnvProjectedTexture::InputDisableUberLight( inputdata_t &inputdata )
{
	m_bUberlight = false;
}

// Console command for creating env_projectedtexture entities
void CC_CreateFlashlight( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	QAngle angles = pPlayer->EyeAngles();
	Vector origin = pPlayer->EyePosition();

	CEnvProjectedTexture *pFlashlight = dynamic_cast< CEnvProjectedTexture * >( CreateEntityByName( "env_projectedtexture" ) );
	if ( args.ArgC() > 1 )
	{
		pFlashlight->SetName( AllocPooledString( args[1] ) );
	}

	pFlashlight->Teleport( &origin, &angles, NULL );

}
static ConCommand create_flashlight( "create_flashlight", CC_CreateFlashlight, 0, FCVAR_CHEAT );
