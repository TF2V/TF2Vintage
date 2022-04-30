#include "tier0/platform.h"

#include "angelscript.h"

#include "vscript/ivscript.h"
#include "as_vector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CAngelScriptVM : public IScriptVM
{
public:
	CAngelScriptVM();

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage() { return SL_ANGELSCRIPT; }
	char const			*GetLanguageName() { return "AngelScript"; }

	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );

	void				AddSearchPath( const char *pszSearchPath ) {}

	void				RemoveOrphanInstances();

	HSCRIPT				CompileScript( const char *pszScript, const char *pszId = NULL );
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
	void				RegisterConstant( ScriptConstantBinding_t *pScriptConstant );
	void				RegisterEnum( ScriptEnumDesc_t *pEnumDesc );

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

	void				WriteState( CUtlBuffer *pBuffer ) {}
	void				ReadState( CUtlBuffer *pBuffer ) {}
	void				DumpState() {}

	bool				ConnectDebugger() { return false; }
	void				DisconnectDebugger() {}
	void				SetOutputCallback( ScriptOutputFunc_t pFunc ) {}
	void				SetErrorCallback( ScriptErrorFunc_t pFunc ) {}
	bool				RaiseException( const char *pszExceptionText ) { return true; }
};


IScriptVM *CreateAngelScriptVM( void )
{
	return new CAngelScriptVM();
}

void DestroyAngelScriptVM( IScriptVM *pVM )
{
	if ( pVM ) delete assert_cast<CAngelScriptVM *>( pVM );
}