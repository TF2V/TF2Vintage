//========= Copyright © Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================
#define _HAS_EXCEPTIONS 0
#include <exception>

#include "tier0/platform.h"
#include "tier1/convar.h"
#include "tier1/utlvector.h"
#include "tier1/utlhash.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"

#include "squirrel.h"
#include "sqstdaux.h"
#include "sqstdstring.h"
#include "sqstdmath.h"
#include "sqstdtime.h"
#include "sqrdbg.h"
#include "sqobject.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqclass.h"
#include "sqstring.h"
#include "squtils.h"
#ifdef _WIN32
#include "sqdbgserver.h"
#endif

#include "vscript/ivscript.h"
#include "vscript_init_nut.h"

#include "vsquirrel_math.h"
#include "sq_vmstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// we don't want bad actors being malicious
extern "C" 
{
	SQRESULT sqstd_loadfile(HSQUIRRELVM,const SQChar*,SQBool)
	{
		return SQ_ERROR;
	}

	SQRESULT sqstd_register_iolib(HSQUIRRELVM)
	{
		return SQ_ERROR;
	}
}

static SQObjectPtr const _null_;

typedef struct
{
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	SQObjectPtr m_instanceUniqueId;
} ScriptInstance_t;


static SQObject const INVALID_HSQOBJECT = { (SQObjectType)-1, {(SQTable *)-1} };
inline bool operator==( SQObject const &lhs, SQObject const &rhs ) { return lhs._type == rhs._type && _table( lhs ) == _table( rhs ); }
inline bool operator!=( SQObject const &lhs, SQObject const &rhs ) { return lhs._type != rhs._type || _table( lhs ) != _table( rhs ); }

//-----------------------------------------------------------------------------
// Purpose: Squirrel scripting engine implementation
//-----------------------------------------------------------------------------
class CSquirrelVM : public IScriptVM
{
	friend struct SQVM;
public:
	CSquirrelVM( void );

	bool				Init( void );
	void				Shutdown( void );

	bool				Frame( float simTime );

	ScriptLanguage_t	GetLanguage()           { return SL_SQUIRREL; }
	char const			*GetLanguageName()      { return "Squirrel"; }

	ScriptStatus_t		Run( const char *pszScript, bool bWait = true );

	void				AddSearchPath( const char *pszSearchPath ) {}

	void				RemoveOrphanInstances() {}

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

	enum
	{
		SAVE_VERSION = 3
	};
	void				WriteState( CUtlBuffer *pBuffer );
	void				ReadState( CUtlBuffer *pBuffer );
	void				DumpState();

	bool				ConnectDebugger();
	void				DisconnectDebugger();
	void				SetOutputCallback( ScriptOutputFunc_t pFunc ) { m_OutputFunc = pFunc; }
	void				SetErrorCallback( ScriptErrorFunc_t pFunc ) { m_ErrorFunc = pFunc; }
	bool				RaiseException( const char *pszExceptionText );

private:
	HSQUIRRELVM GetVM( void )   { return m_hVM; }

	static void					ConvertToVariant( HSQUIRRELVM pVM, SQObject const &pValue, ScriptVariant_t *pVariant );
	static void					PushVariant( HSQUIRRELVM pVM, ScriptVariant_t const &pVariant );

	HSQOBJECT					CreateClass( ScriptClassDesc_t *pClassDesc );
	bool						CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease );

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};
	void						RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );
	void						RegisterDocumentation( HSQOBJECT pClosure, ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );

	HSQOBJECT					LookupObject( char const *szName, HSCRIPT hScope = NULL, bool bRefCount = true );

	//---------------------------------------------------------------------------------------------
	// Squirrel Function Callbacks
	//---------------------------------------------------------------------------------------------

	static SQInteger			CallConstructor( HSQUIRRELVM pVM );
	static SQInteger			ReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			ExternalReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			InstanceToString( HSQUIRRELVM pVM );
	static SQInteger			InstanceIsValid( HSQUIRRELVM pVM );
	static SQInteger			InstanceGetStub( HSQUIRRELVM pVM );
	static SQInteger			InstanceSetStub( HSQUIRRELVM pVM );
	static SQInteger			TranslateCall( HSQUIRRELVM pVM );
	static SQInteger			GetDeveloper( HSQUIRRELVM pVM );
	static SQInteger			GetFunctionSignature( HSQUIRRELVM pVM );
	static void					PrintFunc( HSQUIRRELVM, const SQChar *, ... );
	static void					ErrorFunc( HSQUIRRELVM, const SQChar *, ... );
	static int					QueryContinue( HSQUIRRELVM );

	//---------------------------------------------------------------------------------------------

	HSQUIRRELVM m_hVM;
	HSQREMOTEDBG m_hDbgSrv;

	CUtlHashFast<SQClass *, CUtlHashFastGenericHash> m_ScriptClasses;

	// A reference to our Vector type to compare to
	HSQOBJECT m_VectorClass;
	HSQOBJECT m_QuaternionClass;
	HSQOBJECT m_MatrixClass;

	HSQOBJECT m_CreateScopeClosure;
	HSQOBJECT m_ReleaseScopeClosure;

	SQObjectPtr m_ErrorString;

	ConVarRef developer;

	long long m_nUniqueKeySerial;
	float m_flTimeStartedCall;

	ScriptOutputFunc_t m_OutputFunc;
	ScriptErrorFunc_t m_ErrorFunc;

	static SQRegFunction s_ScriptClassDelegates[];
};

inline CSquirrelVM *GetVScript( HSQUIRRELVM pVM )
{
	return static_cast<CSquirrelVM *>( sq_getsharedforeignptr(pVM) );
}

SQRegFunction CSquirrelVM::s_ScriptClassDelegates[] ={
	{ _SC( "constructor" ), CSquirrelVM::CallConstructor,	0, NULL },
	{MM_GET,				CSquirrelVM::InstanceGetStub,	2, ".s"},
	{MM_SET,				CSquirrelVM::InstanceSetStub,	3, ".s."},
	{MM_TOSTRING,			CSquirrelVM::InstanceToString,	1, "."},
	{_SC( "IsValid" ),		CSquirrelVM::InstanceIsValid,	1, "."},
	{NULL,					NULL}
};


CSquirrelVM::CSquirrelVM( void )
	: m_hVM(NULL), developer("developer"), m_nUniqueKeySerial(0), m_hDbgSrv(NULL), m_ErrorFunc(NULL), m_OutputFunc(NULL)
{
	m_VectorClass = _null_;
	m_QuaternionClass = _null_;
	m_MatrixClass = _null_;
	m_CreateScopeClosure = _null_;
	m_ReleaseScopeClosure = _null_;
	m_ErrorString = _null_;
}

bool CSquirrelVM::Init( void )
{
	m_hVM = sq_open( 1024 );
	
	sq_setsharedforeignptr( GetVM(), this );
	m_hVM->SetQuerySuspendFn( &CSquirrelVM::QueryContinue );
	
	sq_setprintfunc( GetVM(), &CSquirrelVM::PrintFunc, &CSquirrelVM::ErrorFunc );

	if ( IsDebug() || developer.GetInt() )
		sq_enabledebuginfo( GetVM(), SQTrue );
	{
		// register libraries
		sq_pushroottable( GetVM() );

		sqstd_register_stringlib( GetVM() );
		sqstd_register_mathlib( GetVM() );
		sqstd_register_timelib( GetVM() );

		sqstd_seterrorhandlers( GetVM() );

		// register root functions
		sq_pushstring( GetVM(), "developer", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::GetDeveloper, 0 );
		sq_setnativeclosurename( GetVM(), -1, "developer" );
		sq_createslot( GetVM(), -3 );
		sq_pushstring( GetVM(), "GetFunctionSignature", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::GetFunctionSignature, 0 );
		sq_setnativeclosurename( GetVM(), -1, "GetFunctionSignature" );
		sq_createslot( GetVM(), -3 );

		// pop off root table
		sq_pop( GetVM(), 1 );
	}

	RegisterMathBindings( GetVM() );

	// store a reference to our classes for instancing
	m_VectorClass = LookupObject( "Vector" );
	m_QuaternionClass = LookupObject( "Quaternion" );
	m_MatrixClass = LookupObject( "matrix3x4_t" );

	m_ScriptClasses.Init( 256 );

	Run( (char *)g_Script_init );

	// store a reference to the scope utilities from the init script
	m_CreateScopeClosure = LookupObject( "VSquirrel_OnCreateScope" );
	m_ReleaseScopeClosure = LookupObject( "VSquirrel_OnReleaseScope" );

	return true;
}

void CSquirrelVM::Shutdown( void )
{
	if ( GetVM() )
	{
		sq_collectgarbage( GetVM() );

		// free the root table reference
		sq_pushnull( GetVM() );
		sq_setroottable( GetVM() );

		sq_close( m_hVM );
		m_hVM = NULL;
	}

	DisconnectDebugger();
	m_ScriptClasses.Purge();
}

bool CSquirrelVM::Frame( float simTime )
{
	if ( m_hDbgSrv )
	{
		// process outgoing messages
		sq_rdbg_update( m_hDbgSrv );

		if ( !sq_rdbg_connected( m_hDbgSrv ) )
			DisconnectDebugger();
	}

	return true;
}

ScriptStatus_t CSquirrelVM::Run( const char *pszScript, bool bWait )
{
	HSQOBJECT pObject;
	if ( SQ_FAILED( sq_compilebuffer( GetVM(), pszScript, V_strlen( pszScript ), "unnamed", SQ_CALL_RAISE_ERROR ) ) )
		return SCRIPT_ERROR;

	// a closure is pushed on success
	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );

	// pop it off
	sq_pop( GetVM(), 1 );

	ScriptStatus_t result = ExecuteFunction( (HSCRIPT)&pObject, NULL, 0, NULL, NULL, bWait );

	sq_release( GetVM(), &pObject );

	return result;
}

ScriptStatus_t CSquirrelVM::Run( HSCRIPT hScript, HSCRIPT hScope, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, hScope, bWait );
}

ScriptStatus_t CSquirrelVM::Run( HSCRIPT hScript, bool bWait )
{
	return ExecuteFunction( hScript, NULL, 0, NULL, NULL, bWait  );
}

HSCRIPT CSquirrelVM::CompileScript( const char *pszScript, const char *pszId )
{
	if ( !pszScript || !pszScript[0] )
		return NULL;

	HSQOBJECT *pObject = NULL;
	if ( SQ_SUCCEEDED( sq_compilebuffer( GetVM(), pszScript, V_strlen( pszScript ), pszId ? pszId : "unnamed", SQ_CALL_RAISE_ERROR ) ) )
	{
		pObject = new SQObject;

		sq_getstackobj( GetVM(), -1, pObject );
		sq_addref( GetVM(), pObject );

		// a result is pushed on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseScript( HSCRIPT hScript )
{
	if ( hScript )
	{
		sq_release( GetVM(), (HSQOBJECT *)hScript );
		delete (HSQOBJECT *)hScript;
	}
}

HSCRIPT CSquirrelVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	// call the utility create function
	sq_pushobject( GetVM(), m_CreateScopeClosure );
	// push parameters
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pszScope, -1 );
	if ( hParent )
	{
		HSQOBJECT &pTable = *(HSQOBJECT *)hParent;
		Assert( hParent != INVALID_HSCRIPT && sq_istable( pTable ) );
		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	// this pops off the parameters automatically
	HSQOBJECT hScope = _null_;
	if ( SQ_SUCCEEDED( sq_call( GetVM(), 3, SQTrue, SQ_CALL_RAISE_ERROR ) ) )
	{
		sq_getstackobj( GetVM(), -1, &hScope );

		// a result is pushed on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );
	
	// valid return?
	if ( sq_isnull( hScope ) )
		return NULL;
	
	sq_addref( GetVM(), &hScope );

	HSQOBJECT *pObject = new HSQOBJECT;
	pObject->_type = hScope._type;
	pObject->_unVal = hScope._unVal;

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseScope( HSCRIPT hScript )
{
	if ( hScript )
	{
		HSQOBJECT *pObject = (HSQOBJECT *)hScript;

		// call the utility release function
		sq_pushobject( GetVM(), m_ReleaseScopeClosure );

		// push parameters
		sq_pushroottable( GetVM() );
		sq_pushobject( GetVM(), *pObject );

		// this pops off the parameters automatically
		sq_call( GetVM(), 2, SQFalse, SQ_CALL_RAISE_ERROR );

		// pop off the closure
		sq_pop( GetVM(), 1 );

		sq_release( GetVM(), pObject );
		delete (HSQOBJECT *)hScript;
	}
}

HSCRIPT CSquirrelVM::LookupFunction( const char *pszFunction, HSCRIPT hScope )
{
	HSQOBJECT pFunc = LookupObject( pszFunction, hScope );
	// did we find it?
	if ( sq_isnull( pFunc ) )
		return NULL;
	// is it even a function?
	if ( !sq_isclosure( pFunc ) )
	{
		sq_release( GetVM(), &pFunc );
		return NULL;
	}
	
	HSQOBJECT *pObject = new SQObject;
	pObject->_type = OT_CLOSURE;
	pObject->_unVal.pClosure = _closure( pFunc );

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseFunction( HSCRIPT hScript )
{
	if ( hScript )
	{
		sq_release( GetVM(), (HSQOBJECT *)hScript );
		delete (HSQOBJECT *)hScript;
	}
}

ScriptStatus_t CSquirrelVM::ExecuteFunction( HSCRIPT hFunction, ScriptVariant_t *pArgs, int nArgs, ScriptVariant_t *pReturn, HSCRIPT hScope, bool bWait )
{
	if ( hScope == INVALID_HSCRIPT )
	{
		DevWarning( "Invalid scope handed to script VM\n" );
		return SCRIPT_ERROR;
	}

	if ( hFunction == NULL )
	{
		if ( pReturn )
			pReturn->m_type = FIELD_VOID;

		return SCRIPT_ERROR;
	}

	if ( m_hDbgSrv )
	{
		if ( g_bSqDebugBreak )
		{
			DisconnectDebugger();
			g_bSqDebugBreak = false;
		}
	}

	SQInteger initialTop = sq_gettop( GetVM() );
	HSQOBJECT &pClosure = *(HSQOBJECT *)hFunction;
	sq_pushobject( GetVM(), pClosure );

	// push the parent table to call from
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
		{
			sq_pop( GetVM(), 1 );
			return SCRIPT_ERROR;
		}

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
		{
			sq_pop( GetVM(), 1 );
			return SCRIPT_ERROR;
		}

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	if ( pArgs )
	{
		for ( int i=0; i < nArgs; ++i )
			PushVariant( GetVM(), pArgs[i] );
	}

	m_flTimeStartedCall = Plat_FloatTime();
	if ( SQ_FAILED( sq_call( GetVM(), nArgs + 1, pReturn != NULL, SQ_CALL_RAISE_ERROR ) ) )
	{
		// pop off the closure
		sq_pop( GetVM(), 1 );

		if ( pReturn )
			pReturn->m_type = FIELD_VOID;

		m_flTimeStartedCall = 0.0f;

		return SCRIPT_ERROR;
	}

	m_flTimeStartedCall = 0.0f;

	if ( pReturn )
	{
		HSQOBJECT _return;
		sq_getstackobj( GetVM(), -1, &_return );
		ConvertToVariant( GetVM(), _return, pReturn );

		// sq_call pushes a result on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );

	if ( sq_gettop( GetVM() ) != initialTop )
	{
		Warning( "Callstack mismatch in VScript/Squirrel!\n" );
		Assert( sq_gettop( GetVM() ) == initialTop );
	}

	if ( !sq_isnull( m_ErrorString ) )
	{
		sq_pushobject( GetVM(), m_ErrorString );
		m_ErrorString = _null_;
		sq_throwobject( GetVM() );
		return SCRIPT_ERROR;
	}

	return SCRIPT_DONE;
}

void CSquirrelVM::RegisterFunction( ScriptFunctionBinding_t *pScriptFunction )
{
	sq_pushroottable( GetVM() );

	RegisterFunctionGuts( pScriptFunction );

	sq_pop( GetVM(), 1 );
}

bool CSquirrelVM::RegisterClass( ScriptClassDesc_t *pClassDesc )
{
	UtlHashFastHandle_t hndl = m_ScriptClasses.Find( (intp)pClassDesc );
	if ( hndl != m_ScriptClasses.InvalidHandle() )
		return true;

	if ( pClassDesc->m_pBaseDesc )
	{
		RegisterClass( pClassDesc->m_pBaseDesc );
	}

	HSQOBJECT hObject = CreateClass( pClassDesc );
	if ( hObject == INVALID_HSQOBJECT )
		return false;

	sq_pushobject( GetVM(), hObject );

	SQRegFunction *pReg = s_ScriptClassDelegates;
	// register our constructor if we have one
	if ( pClassDesc->m_pfnConstruct )
	{
		sq_pushstring( GetVM(), pReg->name, -1 );
		sq_newclosure( GetVM(), pReg->f, 0 );
		sq_createslot( GetVM(), -3 );
	}

	while ( ( ++pReg)->name != NULL )
	{
		sq_pushstring( GetVM(), pReg->name, -1 );
		sq_newclosure( GetVM(), pReg->f, 0 );
		sq_setnativeclosurename( GetVM(), -1, pReg->name );
		sq_createslot( GetVM(), -3 );
	}

	// register member functions
	FOR_EACH_VEC( pClassDesc->m_FunctionBindings, i )
	{
		RegisterFunctionGuts( &pClassDesc->m_FunctionBindings[i], pClassDesc );
	}

	sq_pop( GetVM(), 1 );


	m_ScriptClasses.FastInsert( (intp)pClassDesc, _class( hObject ) );
	return true;
}

void CSquirrelVM::RegisterConstant( ScriptConstantBinding_t *pScriptConstant )
{
	// register to the const table so users can't change it
	sq_pushconsttable( GetVM() );
	sq_pushstring( GetVM(), pScriptConstant->m_pszScriptName, -1 );
	PushVariant( GetVM(), pScriptConstant->m_data );
	// add to consts
	sq_newslot( GetVM(), -3, SQFalse );
	// pop off const table
	sq_pop( GetVM(), 1 );
}

void CSquirrelVM::RegisterEnum( ScriptEnumDesc_t *pEnumDesc )
{
	// register to the const table
	sq_pushconsttable( GetVM() );
	sq_pushstring( GetVM(), pEnumDesc->m_pszScriptName, -1 );

	// Check if name is already taken
	if ( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) )
	{
		HSQOBJECT hObject = _null_;
		sq_getstackobj( GetVM(), -1, &hObject );
		if ( !sq_isnull( hObject ) )
		{
			sq_pop( GetVM(), 2 );
			return;
		}
	}

	// create a new table to hold the values
	sq_newtable( GetVM() );
	FOR_EACH_VEC( pEnumDesc->m_ConstantBindings, i )
	{
		ScriptConstantBinding_t &constant = pEnumDesc->m_ConstantBindings[i];

		sq_pushstring( GetVM(), constant.m_pszScriptName, -1 );
		PushVariant( GetVM(), constant.m_data );
		// add to table
		sq_newslot( GetVM(), -3, SQFalse );
	}

	// add to consts
	sq_newslot( GetVM(), -3, SQTrue );
	// pop off const table
	sq_pop( GetVM(), 1 );
}

HSCRIPT CSquirrelVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	if ( !RegisterClass( pDesc ) )
		return NULL;

	ScriptInstance_t *pScriptInstance = new ScriptInstance_t;
	pScriptInstance->m_pClassDesc = pDesc;
	pScriptInstance->m_pInstance = pInstance;

	if ( !CreateInstance( pDesc, pScriptInstance, &CSquirrelVM::ExternalReleaseHook ) )
	{
		delete pScriptInstance;
		return NULL;
	}

	HSQOBJECT *pObject = new SQObject;
	sq_getstackobj( GetVM(), -1, pObject );
	sq_addref( GetVM(), pObject );

	sq_pop( GetVM(), 1 );

	return (HSCRIPT)pObject;
}

void CSquirrelVM::SetInstanceUniqeId( HSCRIPT hInstance, const char *pszId )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hInstance;
	if ( !sq_isinstance( pObject ) )
		return;

	ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;
	pInstance->m_instanceUniqueId = SQString::Create( _ss( GetVM() ), pszId, V_strlen( pszId ) );
}

void CSquirrelVM::RemoveInstance( HSCRIPT hScript )
{
	if ( hScript == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hScript;
	if ( sq_isinstance( pObject ) )
		_instance( pObject )->_userpointer = NULL;

	sq_release( GetVM(), &pObject );
	delete (HSQOBJECT *)hScript;
}

void *CSquirrelVM::GetInstanceValue( HSCRIPT hInstance, ScriptClassDesc_t *pExpectedType )
{
	if ( hInstance == NULL )
	{
		ExecuteOnce( DevMsg( "NULL instance passed to vscript!\n" ) );
		return NULL;
	}

	HSQOBJECT &pObject = *(HSQOBJECT *)hInstance;
	if ( !sq_isinstance( pObject ) )
		return NULL;

	ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;

	ScriptClassDesc_t *pDescription = pInstance->m_pClassDesc;
	while ( pDescription )
	{
		if ( pDescription == pExpectedType )
			return pInstance->m_pInstance;

		pDescription = pDescription->m_pBaseDesc;
	}

	return NULL;
}

bool CSquirrelVM::GenerateUniqueKey( const char *pszRoot, char *pBuf, int nBufSize )
{
	if( Q_strlen(pszRoot) + 41 > nBufSize )
	{
		Error( "GenerateUniqueKey: buffer too small\n" );
		if ( nBufSize != 0 )
			*pBuf = '\0';

		return false;
	}

	V_snprintf( pBuf, nBufSize, "%x%x%llx_%s", RandomInt(0, 4095), Plat_MSTime(), ++m_nUniqueKeySerial, pszRoot );

	return true;
}

bool CSquirrelVM::ValueExists( HSCRIPT hScope, const char *pszKey )
{
	return !sq_isnull( LookupObject( pszKey, hScope, false ) );
}

bool CSquirrelVM::SetValue( HSCRIPT hScope, const char *pszKey, const char *pszValue )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );
	sq_pushstring( GetVM(), pszValue, -1 );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return true;
}

bool CSquirrelVM::SetValue( HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );

	if ( value.m_type == FIELD_HSCRIPT && value.m_hScript )
	{
		HSQOBJECT &pObject = *(HSQOBJECT *)value.m_hScript;
		if ( sq_isinstance( pObject ) && _instance( pObject )->_class->_typetag != VECTOR_TYPE_TAG )
		{
			ScriptInstance_t *pInstance = (ScriptInstance_t *)_instance( pObject )->_userpointer;
			if ( sq_isnull( pInstance->m_instanceUniqueId ) )
			{
				// if we haven't been given a unique ID, we'll be assigned the key name
				sq_getstackobj( GetVM(), -1, &pInstance->m_instanceUniqueId );
			}
		}
	}

	PushVariant( GetVM(), value );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return true;
}

void CSquirrelVM::CreateTable( ScriptVariant_t &Table )
{
	HSQOBJECT hObject = INVALID_HSQOBJECT;
	sq_newtable( GetVM() );

	sq_getstackobj( GetVM(), -1, &hObject );
	sq_addref( GetVM(), &hObject );

	ConvertToVariant( GetVM(), hObject, &Table );

	sq_pop( GetVM(), 1 );
}

int CSquirrelVM::GetKeyValue( HSCRIPT hScope, int nIterator, ScriptVariant_t *pKey, ScriptVariant_t *pValue )
{
	HSQOBJECT pKeyObj, pValueObj;
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return -1;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return -1;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushinteger( GetVM(), nIterator );
	if ( SQ_FAILED( sq_next( GetVM(), -2 ) ) )
	{
		sq_pop( GetVM(), 2 );
		return -1;
	}

	sq_getstackobj( GetVM(), -2, &pKeyObj );
	sq_getstackobj( GetVM(), -1, &pValueObj );
	sq_addref( GetVM(), &pKeyObj );
	sq_addref( GetVM(), &pValueObj );

	// sq_next pushes 2 objects onto the stack, so pop them off too
	sq_pop( GetVM(), 2 );

	ConvertToVariant( GetVM(), pKeyObj, pKey );
	ConvertToVariant( GetVM(), pValueObj, pValue );

	// The next index is set by reference in sq_next, so retrieve it here
	int nNexti = 0;
	sq_getinteger( GetVM(), -1, &nNexti );

	// Pop index and table
	sq_pop( GetVM(), 2 );

	return nNexti;
}

bool CSquirrelVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	HSQOBJECT pObject = LookupObject( pszKey, hScope );
	ConvertToVariant( GetVM(), pObject, pValue );

	return !sq_isnull( pObject );
}

void CSquirrelVM::ReleaseValue( ScriptVariant_t &value )
{
	if( value.m_type == FIELD_HSCRIPT )
		sq_release( GetVM(), (HSQOBJECT *)value.m_hScript );
	
	value.Free();
	value.m_type = FIELD_VOID;
}

bool CSquirrelVM::ClearValue( HSCRIPT hScope, const char *pszKey )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return false;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return false;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), pszKey, -1 );
	sq_deleteslot( GetVM(), -2, SQFalse );

	sq_pop( GetVM(), 1 );

	return true;
}

int CSquirrelVM::GetNumTableEntries( HSCRIPT hScope )
{
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return 0;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return 0;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	const int nEntries = sq_getsize( GetVM(), -1 );

	sq_pop( GetVM(), 1 );

	return nEntries;
}

void CSquirrelVM::WriteState( CUtlBuffer *pBuffer )
{
#ifndef VSQUIRREL_TEST
	pBuffer->PutInt( SAVE_VERSION );
	pBuffer->PutInt64( m_nUniqueKeySerial );

	WriteSquirrelState( GetVM(), pBuffer );
#endif
}

void CSquirrelVM::ReadState( CUtlBuffer *pBuffer )
{
#ifndef VSQUIRREL_TEST
	if ( pBuffer->GetInt() != SAVE_VERSION )
	{
		DevMsg( "Incompatible script version\n" );
		return;
	}

	int64 serial = pBuffer->GetInt64();
	m_nUniqueKeySerial = Max( m_nUniqueKeySerial, serial );

	ReadSquirrelState( GetVM(), pBuffer );
#endif
}

void CSquirrelVM::DumpState()
{
#ifndef VSQUIRREL_TEST
	DumpSquirrelState( GetVM() );
#endif
}

bool CSquirrelVM::ConnectDebugger()
{
#ifdef _WIN32
	if( developer.GetInt() == 0 )
		return false;

	if ( m_hDbgSrv == NULL )
	{
		const int serverPort = 1234;
		m_hDbgSrv = sq_rdbg_init( GetVM(), serverPort, SQTrue );
	}

	if ( m_hDbgSrv == NULL )
		return false;

	// !WARN!
	// This will hold up the main thread until connection is established
	return SQ_SUCCEEDED( sq_rdbg_waitforconnections( m_hDbgSrv ) );
#else
	return false;
#endif
}

void CSquirrelVM::DisconnectDebugger()
{
#ifdef _WIN32
	if ( m_hDbgSrv )
	{
		sq_rdbg_shutdown( m_hDbgSrv );
		m_hDbgSrv = NULL;
	}
#endif
}

bool CSquirrelVM::RaiseException( const char *pszExceptionText )
{
	m_ErrorString = SQString::Create( _ss( GetVM() ), pszExceptionText );

	return true;
}

void CSquirrelVM::ConvertToVariant( HSQUIRRELVM pVM, HSQOBJECT const &pValue, ScriptVariant_t *pVariant )
{
	switch ( sq_type( pValue ) )
	{
		case OT_INTEGER:
		{
			*pVariant = _integer( pValue );
			break;
		}
		case OT_FLOAT:
		{
			*pVariant = _float( pValue );
			break;
		}
		case OT_BOOL:
		{
			*pVariant = _integer( pValue ) != 0;
			break;
		}
		case OT_STRING:
		{
			const int nLength = _string( pValue )->_len + 1;
			char *pString = new char[ nLength ];

			*pVariant = pString;
			V_memcpy( (void *)pVariant->m_pszString, _stringval( pValue ), nLength );
			pVariant->m_flags |= SV_FREE;

			break;
		}
		case OT_NULL:
		{
			pVariant->m_type = FIELD_VOID;
			break;
		}
		case OT_INSTANCE:
		{
			sq_pushobject( pVM, pValue );

			SQUserPointer pInstance = NULL;

			SQRESULT nResult = sq_getinstanceup( pVM, -1, &pInstance, VECTOR_TYPE_TAG );
			if ( nResult == SQ_OK )
			{
				*pVariant = new Vector();
				V_memcpy( (void *)pVariant->m_pVector, pInstance, sizeof( Vector ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			nResult = sq_getinstanceup( pVM, -1, &pInstance, QUATERNION_TYPE_TAG );
			if ( nResult == SQ_OK )
			{
				*pVariant = new Quaternion();
				V_memcpy( (void *)pVariant->m_pQuat, pInstance, sizeof( Quaternion ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			nResult = sq_getinstanceup( pVM, -1, &pInstance, QUATERNION_TYPE_TAG );
			if ( nResult == SQ_OK )
			{
				*pVariant = new matrix3x4_t();
				V_memcpy( (void *)pVariant->m_pMatrix, pInstance, sizeof( matrix3x4_t ) );
				pVariant->m_flags |= SV_FREE;

				sq_pop( pVM, 1 );
				break;
			}

			// Fall through here
		}
		default:
		{
			HSQOBJECT *pObject = new SQObject;
			pObject->_type = pValue._type;
			pObject->_unVal = pValue._unVal;

			*pVariant = (HSCRIPT)pObject;
			pVariant->m_flags |= SV_FREE;

			break;
		}
	}
}

void CSquirrelVM::PushVariant( HSQUIRRELVM pVM, ScriptVariant_t const &Variant )
{
	switch ( Variant.m_type )
	{
		case FIELD_INTEGER:
		{
			sq_pushinteger( pVM, Variant.m_int );
			break;
		}
		case FIELD_FLOAT:
		{
			sq_pushfloat( pVM, Variant.m_float );
			break;
		}
		case FIELD_BOOLEAN:
		{
			sq_pushbool( pVM, Variant.m_bool );
			break;
		}
		case FIELD_CHARACTER:
		{
			sq_pushstring( pVM, &Variant.m_char, 1 );
			break;
		}
		case FIELD_CSTRING:
		{
			char const *szString = Variant.m_pszString ? Variant : "";
			sq_pushstring( pVM, szString, V_strlen( szString ) );

			break;
		}
		case FIELD_HSCRIPT:
		{
			if ( Variant.m_hScript )
				sq_pushobject( pVM, *(HSQOBJECT *)Variant.m_hScript );
			else
				sq_pushnull( pVM );

			break;
		}
		case FIELD_VECTOR:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_VectorClass );
			sq_createinstance( pVM, -1 );

			Vector *pVector = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pVector, NULL );
			V_memcpy( pVector, Variant.m_pVector, sizeof(Vector) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		case FIELD_QUATERNION:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_QuaternionClass );
			sq_createinstance( pVM, -1 );

			Quaternion *pQuat = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pQuat, NULL );
			V_memcpy( pQuat, Variant.m_pQuat, sizeof(Quaternion) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		case FIELD_MATRIX3X4:
		{
			sq_pushobject( pVM, GetVScript( pVM )->m_MatrixClass );
			sq_createinstance( pVM, -1 );

			matrix3x4_t *pMatrix = NULL;
			sq_getinstanceup( pVM, -1, (SQUserPointer *)&pMatrix, NULL );
			V_memcpy( pMatrix, Variant.m_pMatrix, sizeof(matrix3x4_t) );

			// Remove the class object from stack so we are aligned
			sq_remove( pVM, -2 );

			break;
		}
		default:
		{
			sq_pushnull( pVM );
			break;
		}
	}
}

HSQOBJECT CSquirrelVM::CreateClass( ScriptClassDesc_t *pClassDesc )
{
	int nArgs = sq_gettop( GetVM() );

	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pClassDesc->m_pszScriptName, -1 );

	bool bHasBase = false;

	ScriptClassDesc_t *pBase = pClassDesc->m_pBaseDesc;
	if ( pBase )
	{
		sq_pushstring( GetVM(), pBase->m_pszScriptName, -1 );
		if ( SQ_FAILED( sq_get( GetVM(), -3 ) ) )
		{
			sq_settop( GetVM(), nArgs );
			return INVALID_HSQOBJECT;
		}

		bHasBase = true;
	}

	if ( SQ_FAILED( sq_newclass( GetVM(), bHasBase ) ) )
	{
		sq_settop( GetVM(), nArgs );
		return INVALID_HSQOBJECT;
	}

	sq_settypetag( GetVM(), -1, pClassDesc );

	HSQOBJECT pObject = _null_;
	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );
	
	sq_createslot( GetVM(), -3 );
	sq_pop( GetVM(), 1 );

	return pObject;
}

bool CSquirrelVM::CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease )
{
	UtlHashFastHandle_t nIndex = m_ScriptClasses.Find( (intp)pClassDesc );
	if ( nIndex == m_ScriptClasses.InvalidHandle() )
		return false;

	SQObjectPtr hClass( m_ScriptClasses[ nIndex ] );
	sq_pushobject( GetVM(), hClass );
	if ( SQ_FAILED( sq_createinstance( GetVM(), -1 ) ) )
	{
		sq_pop( GetVM(), 1 );
		return false;
	}

	// remove class object to align stack
	sq_remove( GetVM(), -2 );

	if ( SQ_FAILED( sq_setinstanceup( GetVM(), -1, pInstance ) ) )
		return false;

	sq_setreleasehook( GetVM(), -1, fnRelease );

	return true;
}

void CSquirrelVM::RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc )
{
	if ( pFunction->m_desc.m_Parameters.Count() > MAX_FUNCTION_PARAMS )
	{
		AssertMsg( 0, "Too many agruments provided for script function %s\n", pFunction->m_desc.m_pszFunction );
		return;
	}

	char szParamCheck[ MAX_FUNCTION_PARAMS+1 ]{0};
	szParamCheck[0] = '.';

	char *pCurrent = &szParamCheck[1];
	for ( int i=0; i < pFunction->m_desc.m_Parameters.Count(); ++i )
	{
		switch ( pFunction->m_desc.m_Parameters[i] )
		{
			case FIELD_INTEGER:
			{
				*pCurrent++ = 'n';
				break;
			}
			case FIELD_FLOAT:
			{
				*pCurrent++ = 'n';
				break;
			}
			case FIELD_BOOLEAN:
			{
				*pCurrent++ = 'b';
				break;
			}
			case FIELD_VECTOR:
			case FIELD_QUATERNION:
			case FIELD_MATRIX3X4:
			{
				*pCurrent++ = 'x';
				break;
			}
			case FIELD_CSTRING:
			{
				*pCurrent++ = 's';
				break;
			}
			case FIELD_HSCRIPT:
			{
				*pCurrent++ = '.';
				break;
			}
			default:
			{
				AssertMsg( 0 , "Unsupported type" );
				return;
			}
		}
	}

	// null terminate
	*pCurrent++ = '\0';
	
	sq_pushstring( GetVM(), pFunction->m_desc.m_pszScriptName, -1 );
	sq_pushuserpointer( GetVM(), pFunction );
	sq_newclosure( GetVM(), &CSquirrelVM::TranslateCall, 1 );
	sq_setnativeclosurename( GetVM(), -1, pFunction->m_desc.m_pszScriptName );
	sq_setparamscheck( GetVM(), pFunction->m_desc.m_Parameters.Count() + 1, szParamCheck );

	HSQOBJECT pClosure;
	sq_getstackobj( GetVM(), -1, &pClosure );

	sq_createslot( GetVM(), -3 );

	if ( developer.GetInt() )
	{
		if ( pFunction->m_desc.m_pszDescription && *pFunction->m_desc.m_pszDescription == *SCRIPT_HIDE )
			return;

		RegisterDocumentation( pClosure, pFunction, pClassDesc );
	}
}

void CSquirrelVM::RegisterDocumentation( HSQOBJECT pClosure, ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc )
{
	char szName[128]{};
	if ( pClassDesc )
	{
		V_strcat_safe( szName, pClassDesc->m_pszScriptName );
		V_strcat_safe( szName, "::" );
	}
	V_strcat_safe( szName, pFunction->m_desc.m_pszScriptName );

	char const *pszReturnType = "";
	switch ( pFunction->m_desc.m_ReturnType )
	{
		case FIELD_VOID:
			pszReturnType = "void";
			break;
		case FIELD_FLOAT:
			pszReturnType = "float";
			break;
		case FIELD_CSTRING:
			pszReturnType = "string";
			break;
		case FIELD_VECTOR:
			pszReturnType = "Vector";
			break;
		case FIELD_QUATERNION:
			pszReturnType = "Quaternion";
			break;
		case FIELD_MATRIX3X4:
			pszReturnType = "matrix3x4_t";
			break;
		case FIELD_INTEGER:
			pszReturnType = "int";
			break;
		case FIELD_BOOLEAN:
			pszReturnType = "bool";
			break;
		case FIELD_CHARACTER:
			pszReturnType = "char";
			break;
		case FIELD_HSCRIPT:
			pszReturnType = "handle";
			break;
		default:
			pszReturnType = "<unknown>";
			break;
	}

	char szSignature[512]{};
	V_strcat_safe( szSignature, pszReturnType );
	V_strcat_safe( szSignature, " " );
	V_strcat_safe( szSignature, szName );
	V_strcat_safe( szSignature, "(" );
	FOR_EACH_VEC( pFunction->m_desc.m_Parameters, i )
	{
		if ( i != 0 )
			V_strcat_safe( szSignature, ", " );

		char const *pszArgumentType = "";
		switch ( pFunction->m_desc.m_Parameters[i] )
		{
			case FIELD_FLOAT:
				pszArgumentType = "float";
				break;
			case FIELD_CSTRING:
				pszArgumentType = "string";
				break;
			case FIELD_VECTOR:
				pszArgumentType = "Vector";
				break;
			case FIELD_QUATERNION:
				pszArgumentType = "Quaternion";
				break;
			case FIELD_MATRIX3X4:
				pszArgumentType = "matrix3x4_t";
				break;
			case FIELD_INTEGER:
				pszArgumentType = "int";
				break;
			case FIELD_BOOLEAN:
				pszArgumentType = "bool";
				break;
			case FIELD_CHARACTER:
				pszArgumentType = "char";
				break;
			case FIELD_HSCRIPT:
				pszArgumentType = "handle";
				break;
			default:
				pszReturnType = "<unknown>";
				break;
		}

		V_strcat_safe( szSignature, pszArgumentType );
	}
	V_strcat_safe( szSignature, ")" );

	HSQOBJECT pRegisterDocumentation = LookupObject( "RegisterFunctionDocumentation", NULL, false );
	sq_pushobject( GetVM(), pRegisterDocumentation );
	// push our parameters
	sq_pushroottable( GetVM() );
	sq_pushobject( GetVM(), pClosure );
	sq_pushstring( GetVM(), szName, -1 );
	sq_pushstring( GetVM(), szSignature, -1 );
	sq_pushstring( GetVM(), pFunction->m_desc.m_pszDescription, -1 );
	// call the function and pop the parameters
	sq_call( GetVM(), 5, SQFalse, SQ_CALL_RAISE_ERROR );

	// pop off the closure
	sq_pop( GetVM(), 1 );
}

HSQOBJECT CSquirrelVM::LookupObject( char const *szName, HSCRIPT hScope, bool bRefCount )
{
	HSQOBJECT pObject = _null_;
	if ( hScope )
	{
		if ( hScope == INVALID_HSCRIPT )
			return _null_;

		HSQOBJECT &pTable = *(HSQOBJECT *)hScope;
		if ( pTable == INVALID_HSQOBJECT || !sq_istable( pTable ) )
			return _null_;

		sq_pushobject( GetVM(), pTable );
	}
	else
	{
		sq_pushroottable( GetVM() );
	}

	sq_pushstring( GetVM(), szName, -1 );
	if ( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) )
	{
		sq_getstackobj( GetVM(), -1, &pObject );
		if ( bRefCount )
			sq_addref( GetVM(), &pObject );

		sq_pop( GetVM(), 1 );
	}

	sq_pop( GetVM(), 1 );

	return pObject;
}

SQInteger CSquirrelVM::GetDeveloper( HSQUIRRELVM pVM )
{
	sq_pushinteger( pVM, GetVScript( pVM )->developer.GetInt() );
	return 1;
}

SQInteger CSquirrelVM::GetFunctionSignature( HSQUIRRELVM pVM )
{
	static char szSignature[512];

	if ( sq_gettop( pVM ) != 3 )
		return 0;

	HSQOBJECT pObject = _null_;
	sq_getstackobj( pVM, 2, &pObject );
	if ( !sq_isclosure( pObject ) )
		return 0;

	V_memset( szSignature, 0, sizeof szSignature );

	char const *pszName = NULL;
	sq_getstring( pVM, 1, &pszName );
	SQClosure *pClosure = _closure( pObject );
	SQFunctionProto *pPrototype = pClosure->_function;

	char const *pszFuncName;
	if ( pszName && *pszName )
	{
		pszFuncName = pszName;
	}
	else
	{
		pszFuncName = sq_isstring( pPrototype->_name ) ? _stringval( pPrototype->_name ) : "<unnamed>";
	}

	V_strncat( szSignature, "function ", sizeof szSignature );
	V_strncat( szSignature, pszFuncName, sizeof szSignature );
	V_strncat( szSignature, "(", sizeof szSignature );

	if ( pPrototype->_nparameters >= 2 )
	{
		for ( int i=1; i < pPrototype->_nparameters; ++i )
		{
			if ( i != 1 )
				V_strncat( szSignature, ", ", sizeof szSignature );

			char const *pszArgument = sq_isstring( pPrototype->_parameters[i] ) ? _stringval( pPrototype->_parameters[i] ) : "arg";
			V_strncat( szSignature, pszArgument, sizeof szSignature );
		}
	}

	V_strncat( szSignature, ")", sizeof szSignature );

	sq_pushstring( pVM, szSignature, -1 );
	return 1;
}

SQInteger CSquirrelVM::TranslateCall( HSQUIRRELVM pVM )
{
	CUtlVectorFixed<ScriptVariant_t, MAX_FUNCTION_PARAMS> parameters;

	ScriptFunctionBinding_t *pFuncBinding = NULL;
	sq_getuserpointer( pVM, sq_gettop( pVM ), (SQUserPointer *)&pFuncBinding );
	auto const &fnParams = pFuncBinding->m_desc.m_Parameters;

	parameters.SetCount( fnParams.Count() );

	const int nArguments = Min( fnParams.Count(), sq_gettop( pVM ) );
	for ( int i=0; i < nArguments; ++i )
	{
		switch ( fnParams.Element( i ) )
		{
			case FIELD_INTEGER:
			{
				SQInteger n = 0;
				if ( SQ_FAILED( sq_getinteger( pVM, i+2, &n ) ) )
					return sqstd_throwerrorf( pVM, "Integer argument expected at argument %d", i );

				parameters[i] = n;
				break;
			}
			case FIELD_FLOAT:
			{
				SQFloat f = 0.0;
				if ( SQ_FAILED( sq_getfloat( pVM, i+2, &f ) ) )
					return sqstd_throwerrorf( pVM, "Float argument expected at argument %d", i );

				parameters[i] = f;
				break;
			}
			case FIELD_BOOLEAN:
			{
				SQBool b = SQFalse;
				if ( SQ_FAILED( sq_getbool( pVM, i+2, &b ) ) )
					return sqstd_throwerrorf( pVM, "Bool argument expected at argument %d", i );

				parameters[i] = b == SQTrue;
				break;
			}
			case FIELD_CHARACTER:
			{
				char const *pChar = NULL;
				if ( SQ_FAILED( sq_getstring( pVM, i+2, &pChar ) ) )
					return sqstd_throwerrorf( pVM, "String argument expected at argument %d", i );
				if ( pChar == NULL )
					pChar = "\0";

				parameters[i] = *pChar;
				break;
			}
			case FIELD_CSTRING:
			{
				char const *pszString = NULL;
				if ( SQ_FAILED( sq_getstring( pVM, i+2, &pszString ) ) )
					return sqstd_throwerrorf( pVM, "String argument expected at argument %d", i );

				parameters[i] = pszString;
				break;
			}
			case FIELD_VECTOR:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, VECTOR_TYPE_TAG );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Vector argument expected at argument %d", i );

				parameters[i] = (Vector *)pInstance;
				break;
			}
			case FIELD_QUATERNION:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, QUATERNION_TYPE_TAG );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Quaternion argument expected at argument %d", i );

				parameters[i] = (Quaternion *)pInstance;
				break;
			}
			case FIELD_MATRIX3X4:
			{
				SQUserPointer pInstance = NULL;
				sq_getinstanceup( pVM, i+2, &pInstance, MATRIX_TYPE_TAG );
				if ( pInstance == NULL )
					return sqstd_throwerrorf( pVM, "Matrix argument expected at argument %d", i );

				parameters[i] = (matrix3x4_t *)pInstance;
				break;
			}
			case FIELD_HSCRIPT:
			{
				HSQOBJECT pObject = _null_;
				if ( SQ_FAILED( sq_getstackobj( pVM, i+2, &pObject ) ) )
					return sqstd_throwerrorf( pVM, "Handle argument expected at argument %d", i );

				if ( sq_isnull( pObject ) )
				{
					parameters[i] = (HSCRIPT)NULL;
				}
				else
				{
					HSQOBJECT *pScript = new SQObject;
					pScript->_type = pObject._type;
					pScript->_unVal = pObject._unVal;

					parameters[i] = (HSCRIPT)pScript;
					parameters[i].m_flags |= SV_FREE;
				}

				break;
			}
			default:
			{
				AssertMsg( 0, "Unsupported type" );
				return false;
			}
		}
	}

	SQUserPointer pContext = NULL;
	if ( pFuncBinding->m_flags & SF_MEMBER_FUNC )
	{
		ScriptInstance_t *pInstance = NULL;
		sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL );
		if ( pInstance == NULL || pInstance->m_pInstance == NULL )
			return sq_throwerror( pVM, "Accessed null instance" );

		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			pContext = pHelper->GetProxied( pInstance->m_pInstance );
			if ( pContext == NULL )
				return sq_throwerror( pVM, "Accessed null instance" );
		}
		else
		{
			pContext = pInstance->m_pInstance;
		}
	}

	const bool bHasReturn = pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;

	ScriptVariant_t returnValue;
	pFuncBinding->m_pfnBinding( pFuncBinding->m_pFunction, 
								pContext, 
								parameters.Base(), 
								parameters.Count(), 
								bHasReturn ? &returnValue : NULL );

	for ( int i=0; i < parameters.Count(); ++i )
	{
		parameters[i].Free();
	}

	HSQOBJECT &hErrorString = GetVScript( pVM )->m_ErrorString;
	if ( !sq_isnull( hErrorString ) )
	{
		sq_pushobject( pVM, hErrorString );
		GetVScript( pVM )->m_ErrorString = _null_;
		return sq_throwobject( pVM );
	}

	PushVariant( pVM, returnValue );

	return (SQBool)bHasReturn;
}

SQInteger CSquirrelVM::CallConstructor( HSQUIRRELVM pVM )
{
	ScriptClassDesc_t *pClassDesc = NULL;
	sq_gettypetag( pVM, 1, (SQUserPointer *)&pClassDesc );
	if ( !pClassDesc->m_pfnConstruct )
	{
		return sqstd_throwerrorf( pVM, "Unable to construct instances of %s.", pClassDesc->m_pszScriptName );
	}

	void *pvInstance = pClassDesc->m_pfnConstruct();

	HSQOBJECT &hErrorString = GetVScript( pVM )->m_ErrorString;
	if ( !sq_isnull( hErrorString ) )
	{
		sq_pushobject( pVM, hErrorString );
		GetVScript( pVM )->m_ErrorString = _null_;
		return sq_throwobject( pVM );
	}

	ScriptInstance_t *pInstance = new ScriptInstance_t;
	pInstance->m_pClassDesc = pClassDesc;
	pInstance->m_pInstance = pvInstance;

	sq_setinstanceup( pVM, 1, pInstance );
	sq_setreleasehook( pVM, 1, &CSquirrelVM::ReleaseHook );

	return SQ_OK;
}

SQInteger CSquirrelVM::ReleaseHook( SQUserPointer data, SQInteger size )
{
	ScriptInstance_t *pObject = (ScriptInstance_t *)data;
	if ( pObject->m_pClassDesc->m_pfnDestruct )
	{
		pObject->m_pClassDesc->m_pfnDestruct( pObject->m_pInstance );
		delete pObject;
	}

	return SQ_OK;
}

SQInteger CSquirrelVM::ExternalReleaseHook( SQUserPointer data, SQInteger size )
{
	delete (ScriptInstance_t *)data;
	return SQ_OK;
}

SQInteger CSquirrelVM::InstanceToString( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL );
	if ( pInstance && pInstance->m_pInstance )
	{
		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			char szInstance[128] = "";
			if ( pHelper->ToString( pInstance->m_pInstance, szInstance, sizeof( szInstance ) ) )
			{
				sq_pushstring( pVM, szInstance, -1 );
				return 1;
			}
		}

		sqstd_pushstringf( pVM, "(%s : 0x%p)", pInstance->m_pClassDesc->m_pszScriptName, pInstance->m_pInstance );
		return 1;
	}

	SQObjectPtr hObject;
	sq_getstackobj( pVM, 1, &hObject );
	sqstd_pushstringf( pVM, "(%s : 0x%p)", GetTypeName( hObject ), _instance( hObject ) );
	return 1;
}

SQInteger CSquirrelVM::InstanceIsValid( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL );
	sq_pushbool( pVM, pInstance && pInstance->m_pInstance );
	return 1;
}

SQInteger CSquirrelVM::InstanceGetStub( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL );

	const SQChar *pString = NULL;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pString ) ) )
		return sq_throwerror( pVM, "Expected _get( string )" );

	auto const &members = pInstance->m_pClassDesc->m_MemberBindings;
	auto pvInstance = pInstance->m_pInstance;
	if( pvInstance == NULL )
		return sq_throwerror( pVM, "Null instance." );

	FOR_EACH_VEC( members, i )
	{
		if ( V_strcmp( members[i].m_pszScriptName, pString ) == 0 )
		{
			ptrdiff_t const nOffset = members[i].m_unMemberOffs;
			size_t const nSize = members[i].m_unMemberSize;
			switch ( members[i].m_nMemberType )
			{
				case FIELD_INTEGER:
				{
					sq_pushinteger( pVM, *(int *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_FLOAT:
				{
					sq_pushfloat( pVM, *(float *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_BOOLEAN:
				{
					sq_pushbool( pVM, *(bool *)( (uintp)pvInstance + nOffset ) );
					return 1;
				}
				case FIELD_CSTRING:
				{
					SQChar sString[1024];
					V_memcpy( sString, (void *)( (uintp)pvInstance + nOffset ), nSize );
					sq_pushstring( pVM, sString, sizeof( sString ) );
					return 1;
				}
				case FIELD_VECTOR:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_VectorClass );
					sq_createinstance( pVM, -1 );

					Vector *pVector = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pVector, NULL );
					V_memcpy( pVector, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				case FIELD_QUATERNION:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_QuaternionClass );
					sq_createinstance( pVM, -1 );

					Quaternion *pQuat = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pQuat, NULL );
					V_memcpy( pQuat, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				case FIELD_MATRIX3X4:
				{
					sq_pushobject( pVM, GetVScript( pVM )->m_MatrixClass );
					sq_createinstance( pVM, -1 );

					matrix3x4_t *pMatrix = NULL;
					sq_getinstanceup( pVM, -1, (SQUserPointer *)&pMatrix, NULL );
					V_memcpy( pMatrix, (void *)( (uintp)pvInstance + nOffset ), nSize );

					// Remove the class object from stack so we are aligned
					sq_remove( pVM, -2 );
					return 1;
				}
				default:
					return sqstd_throwerrorf( pVM, "Unsupported data type (%s).", ScriptFieldTypeName( members[i].m_nMemberType ) );
			}
		}
	}

	return 0;
}

SQInteger CSquirrelVM::InstanceSetStub( HSQUIRRELVM pVM )
{
	ScriptInstance_t *pInstance = NULL;
	sq_getinstanceup( pVM, 1, (SQUserPointer *)&pInstance, NULL );

	const SQChar *pString = NULL;
	if ( SQ_FAILED( sq_getstring( pVM, 2, &pString ) ) )
		return sq_throwerror( pVM, "Expected _set( string, value )" );

	auto const &members = pInstance->m_pClassDesc->m_MemberBindings;
	auto pvInstance = pInstance->m_pInstance;
	if ( pvInstance == NULL )
		return sq_throwerror( pVM, "Null instance." );

	FOR_EACH_VEC( members, i )
	{
		if ( V_strcmp( members[i].m_pszScriptName, pString ) == 0 )
		{
			ptrdiff_t nOffset = members[i].m_unMemberOffs;
			size_t nSize = members[i].m_unMemberSize;
			switch ( members[i].m_nMemberType )
			{
				case FIELD_INTEGER:
				{
					SQInteger n = 0;
					if ( SQ_FAILED( sq_getinteger( pVM, 3, &n ) ) )
						return sq_throwerror( pVM, "Expected _set( string, integer )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &n, nSize );
					break;
				}
				case FIELD_FLOAT:
				{
					SQFloat f = 0.0;
					if ( SQ_FAILED( sq_getfloat( pVM, 3, &f ) ) )
						return sq_throwerror( pVM, "Expected _set( string, float )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &f, nSize );
					break;
				}
				case FIELD_BOOLEAN:
				{
					SQBool b = SQFalse;
					if ( SQ_FAILED( sq_getbool( pVM, 3, &b ) ) )
						return sq_throwerror( pVM, "Expected _set( string, boolean )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &b, nSize );
					break;
				}
				case FIELD_CHARACTER:
				{
					char const *pChar = NULL;
					if ( SQ_FAILED( sq_getstring( pVM, 3, &pChar ) ) )
						return sq_throwerror( pVM, "Expected _set( string, string )" );
					if ( pChar == NULL )
						pChar = "\0";

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), pChar, nSize );
					break;
				}
				case FIELD_CSTRING:
				{
					char const *pszString = "";
					if ( SQ_FAILED( sq_getstring( pVM, 3, &pszString ) ) )
						return sq_throwerror( pVM, "Expected _set( string, string )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), pszString, nSize );
					break;
				}
				case FIELD_VECTOR:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, VECTOR_TYPE_TAG );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, Vector )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_QUATERNION:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, QUATERNION_TYPE_TAG );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, Quaternion )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_MATRIX3X4:
				{
					SQUserPointer p = NULL;
					sq_getinstanceup( pVM, 3, &p, MATRIX_TYPE_TAG );
					if ( p == NULL )
						return sq_throwerror( pVM, "Expected _set( string, matrix3x4_t )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), p, nSize );
					break;
				}
				case FIELD_HSCRIPT:
				{
					HSQOBJECT hObject = _null_;
					if ( SQ_FAILED( sq_getstackobj( pVM, 3, &hObject ) ) )
						return sq_throwerror( pVM, "Expected _set( string, Handle )" );

					V_memcpy( (void *)( (uintp)pvInstance + nOffset ), &_rawval( hObject ), nSize );
					break;
				}
				default:
					return sqstd_throwerrorf( pVM, "Unsupported data type (%s).", ScriptFieldTypeName( members[i].m_nMemberType ) );
			}
		}
	}

	return SQ_OK;
}

void CSquirrelVM::PrintFunc( HSQUIRRELVM pVM, const SQChar *fmt, ... )
{
	static char szMessage[2048]{};

	va_list va;
	va_start( va, fmt );
	V_vsprintf_safe( szMessage, fmt, va );
	va_end( va );

	ScriptOutputFunc_t fnOutput = GetVScript( pVM )->m_OutputFunc;
	if ( fnOutput )
	{
		fnOutput( szMessage );
	}
	else
	{
		Msg( "%s\n", szMessage );
	}
}

void CSquirrelVM::ErrorFunc( HSQUIRRELVM pVM, const SQChar *fmt, ... )
{
	static char szMessage[2048]{};

	va_list va;
	va_start( va, fmt );
	V_vsprintf_safe( szMessage, fmt, va );
	va_end( va );

	ScriptErrorFunc_t fnError = GetVScript( pVM )->m_ErrorFunc;
	if ( fnError )
	{
		fnError( SCRIPT_LEVEL_ERROR, szMessage );
	}
	else
	{
		Warning( "%s\n", szMessage );
	}
}

int CSquirrelVM::QueryContinue( HSQUIRRELVM pVM )
{
	CSquirrelVM *pVScript = GetVScript( pVM );
	const float flStartTime = pVScript->m_flTimeStartedCall;
	if ( !pVScript->m_hDbgSrv && flStartTime != 0.0f )
	{
		const float flTimeDelta = Plat_FloatTime() - flStartTime;
		if ( flTimeDelta > 0.03f )
		{
			scprintf(_SC("Script running too long, terminating\n"));
			return SQ_QUERY_BREAK;
		}
	}
	return SQ_QUERY_CONTINUE;
}


//-----------------------------------------------------------------------------
// Purpose: Return a new instace of CSquirrelVM
//-----------------------------------------------------------------------------
IScriptVM *CreateSquirrelVM( void )
{
	return new CSquirrelVM();
}

//-----------------------------------------------------------------------------
// Purpose: Delete CSquirrelVM isntance
//-----------------------------------------------------------------------------
void DestroySquirrelVM( IScriptVM *pVM )
{
	if( pVM ) delete assert_cast<CSquirrelVM *>( pVM );
}