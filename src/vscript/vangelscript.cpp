#include "tier0/platform.h"
#include "tier1/convar.h"

#include "angelscript.h"
#include "scripthandle/scripthandle.h"
#include "weakref/weakref.h"
#include "scriptdictionary/scriptdictionary.h"
#include "contextmgr/contextmgr.h"
#include "scriptarray/scriptarray.h"
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"

#include "vscript/ivscript.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void RegisterVector( asIScriptEngine *engine )
{
}

//-----------------------------------------------------------------------------
// Purpose: AngelScript scripting engine implementation
//-----------------------------------------------------------------------------
class CAngelScriptVM : public IScriptVM
{
public:
	CAngelScriptVM( void );

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage()           { return SL_ANGELSCRIPT; }
	char const			*GetLanguageName()      { return "AngelScript"; }

	void				AddSearchPath( const char *pszSearchPath );

	void				RemoveOrphanInstances();

	HSCRIPT				CompileScript( const char *pszScript, const char *pszId = NULL );
	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true );
	ScriptStatus_t		Run( HSCRIPT hScript, bool bWait );
	void				ReleaseScript( HSCRIPT hScript );

	HSCRIPT				CreateScope( const char *pszScope, HSCRIPT hParent = NULL );
	void				ReleaseScope( HSCRIPT hScript );

	HSCRIPT				LookupFunction( const char *pszFunction, HSCRIPT hScope = NULL );
	void				ReleaseFunction( HSCRIPT hScript );
	ScriptStatus_t		ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait );

	void				RegisterFunction( ScriptFunctionBinding_t *pScriptFunction );
	bool				RegisterClass( ScriptClassDesc_t *pClassDesc );
	HSCRIPT				RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance );
	void				*GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType = NULL );
	void				RemoveInstance( HSCRIPT hScript );

	void				SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId );
	bool				GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize );

	bool				ValueExists( HSCRIPT hScope, const char *pszKey );
	bool				SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue );
	bool				SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value );
	void				CreateTable( ScriptVariant_t &Table );
	int					GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue );
	bool				GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue );
	void				ReleaseValue( ScriptVariant_t &value );
	bool				ClearValue( HSCRIPT hScope, const char *pszKey );
	int					GetNumTableEntries( HSCRIPT hScope );

	enum
	{
		SAVE_VERSION = 2
	};
	void				WriteState( CUtlBuffer *pBuffer );
	void				ReadState( CUtlBuffer *pBuffer );
	void				DumpState();

	bool				ConnectDebugger() { return false; }
	void				DisconnectDebugger() {}
	void				SetOutputCallback( ScriptOutputFunc_t pFunc );
	void				SetErrorCallback( ScriptErrorFunc_t pFunc );
	bool				RaiseException( const char *pszExceptionText );

private:
	asIScriptEngine		*GetVM( void ) { return m_pEngine; }

	//---------------------------------------------------------------------------------------------
	// AngelScript Function Callbacks
	//---------------------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------------------

	asIScriptEngine *m_pEngine;
	CContextMgr m_CtxMgr;

	ConVarRef developer;

	long long m_nUniqueKeySerial;
};


CAngelScriptVM::CAngelScriptVM( void )
	: m_pEngine(NULL), developer( "developer" )
{
	m_nUniqueKeySerial = 0;
}

bool CAngelScriptVM::Init( void )
{
	return true;
}

void CAngelScriptVM::Shutdown( void )
{
}

bool CAngelScriptVM::Frame( float simTime )
{
	return true;
}

void CAngelScriptVM::AddSearchPath( const char *pszSearchPath )
{
}

void CAngelScriptVM::RemoveOrphanInstances()
{
}

HSCRIPT CAngelScriptVM::CompileScript( const char *pszScript, const char *pszId )
{
	return HSCRIPT();
}

ScriptStatus_t CAngelScriptVM::Run( const char *pszScript, bool bWait )
{
	return ScriptStatus_t();
}

ScriptStatus_t CAngelScriptVM::Run( HSCRIPT hScript, HSCRIPT hScope, bool bWait )
{
	return ScriptStatus_t();
}

ScriptStatus_t CAngelScriptVM::Run( HSCRIPT hScript, bool bWait )
{
	return ScriptStatus_t();
}

void CAngelScriptVM::ReleaseScript( HSCRIPT hScript )
{
}

HSCRIPT CAngelScriptVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	return HSCRIPT();
}

void CAngelScriptVM::ReleaseScope( HSCRIPT hScript )
{
}

HSCRIPT CAngelScriptVM::LookupFunction( const char *pszFunction, HSCRIPT hScope )
{
	return HSCRIPT();
}

void CAngelScriptVM::ReleaseFunction( HSCRIPT hScript )
{
}

ScriptStatus_t CAngelScriptVM::ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait )
{
	return ScriptStatus_t();
}

void CAngelScriptVM::RegisterFunction( ScriptFunctionBinding_t *pScriptFunction )
{
}

bool CAngelScriptVM::RegisterClass( ScriptClassDesc_t *pClassDesc )
{
	return true;
}

HSCRIPT CAngelScriptVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	return HSCRIPT();
}

void *CAngelScriptVM::GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType )
{
	return nullptr;
}

void CAngelScriptVM::RemoveInstance( HSCRIPT hScript )
{
}

void CAngelScriptVM::SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId )
{
}

bool CAngelScriptVM::GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize )
{
	return true;
}

bool CAngelScriptVM::ValueExists( HSCRIPT hScope, const char *pszKey )
{
	return false;
}

bool CAngelScriptVM::SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue )
{
	return true;
}

bool CAngelScriptVM::SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value )
{
	return true;
}

void CAngelScriptVM::CreateTable( ScriptVariant_t &Table )
{
}

int CAngelScriptVM::GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	return 0;
}

bool CAngelScriptVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	return true;
}

void CAngelScriptVM::ReleaseValue( ScriptVariant_t &value )
{
}

bool CAngelScriptVM::ClearValue( HSCRIPT hScope, const char *pszKey )
{
	return true;
}

int CAngelScriptVM::GetNumTableEntries( HSCRIPT hScope )
{
	return 0;
}

void CAngelScriptVM::WriteState( CUtlBuffer *pBuffer )
{
}

void CAngelScriptVM::ReadState( CUtlBuffer *pBuffer )
{
}

void CAngelScriptVM::DumpState()
{
}

bool CAngelScriptVM::ConnectDebugger()
{
	return false;
}

void CAngelScriptVM::DisconnectDebugger()
{
}

void CAngelScriptVM::SetOutputCallback( ScriptOutputFunc_t pFunc )
{
}

void CAngelScriptVM::SetErrorCallback( ScriptErrorFunc_t pFunc )
{
}

bool CAngelScriptVM::RaiseException( const char *pszExceptionText )
{
	return true;
}


IScriptVM *CreateAngelScriptVM( void )
{
	return new CAngelScriptVM();
}

void DestroyAngelScriptVM( IScriptVM *pVM )
{
	if ( pVM ) delete pVM;
}