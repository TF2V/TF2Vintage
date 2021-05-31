//========= Copyright � Valve LLC, All rights reserved. =======================
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
#include "sqplus.h"
#include "sqrdbg.h"
#include "sqobject.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqtable.h"
#include "sqclosure.h"
#include "sqfuncproto.h"
#include "sqclass.h"
#include "sqstring.h"
#include "squtils.h"
#include "sqdbgserver.h"

#include "vscript/ivscript.h"
#include "vscript_init_nut.h"

#include "sq_vector.h"
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


typedef struct
{
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	SQObjectPtr m_instanceUniqueId;
} ScriptInstance_t;



static HSQOBJECT const INVALID_HSQOBJECT = { (SQObjectType)-1, (SQTable *)-1 };
inline bool operator==( HSQOBJECT const &lhs, HSQOBJECT const &rhs ) { return lhs._type == rhs._type && _table( lhs ) == _table( rhs ); }
inline bool operator!=( HSQOBJECT const &lhs, HSQOBJECT const &rhs ) { return lhs._type != rhs._type || _table( lhs ) != _table( rhs ); }

static CSquirrelVM *GetVScript( HSQUIRRELVM pVM )
{
	return static_cast<CSquirrelVM *>( _ss( pVM )->_up );
}

//-----------------------------------------------------------------------------
// Purpose: Squirrel scripting engine implementation
//-----------------------------------------------------------------------------
class CSquirrelVM : public IScriptVM
{
	friend struct SQVM;
public:
	CSquirrelVM( void );

	enum
	{
		MAX_FUNCTION_PARAMS = 14
	};

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
	void				DumpState() {}

	bool				ConnectDebugger();
	void				DisconnectDebugger();
	void				SetOutputCallback( ScriptOutputFunc_t pFunc ) {}
	void				SetErrorCallback( ScriptErrorFunc_t pFunc ) {}
	bool				RaiseException( const char *pszExceptionText );

private:
	HSQUIRRELVM GetVM( void )   { return m_pVirtualMachine; }

	void						ConvertToVariant( SQObject const &pValue, ScriptVariant_t *pVariant );
	void						PushVariant( ScriptVariant_t const &pVariant, bool bDuplicate );

	HSQOBJECT					CreateClass( ScriptClassDesc_t *pClassDesc );
	bool						CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease );

	void						RegisterFunctionGuts( ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );
	void						RegisterDocumentation( HSQOBJECT pClosure, ScriptFunctionBinding_t *pFunction, ScriptClassDesc_t *pClassDesc = NULL );

	HSQOBJECT					LookupObject( char const *szName, HSCRIPT hScope = NULL, bool bRefCount = true );

	//---------------------------------------------------------------------------------------------
	// Squirrel Function Callbacks
	//---------------------------------------------------------------------------------------------

	static SQInteger			CallConstructor( HSQUIRRELVM pVM );
	static SQInteger			ReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			ExternalReleaseHook( SQUserPointer data, SQInteger size );
	static SQInteger			GetDeveloper( HSQUIRRELVM pVM );
	static SQInteger			GetFunctionSignature( HSQUIRRELVM pVM );
	static SQInteger			TranslateCall( HSQUIRRELVM pVM );
	static SQInteger			InstanceToString( HSQUIRRELVM pVM );
	static SQInteger			InstanceIsValid( HSQUIRRELVM pVM );
	static void					PrintFunc( HSQUIRRELVM, const SQChar *, ... );
	static int					QueryContinue( HSQUIRRELVM );

	//---------------------------------------------------------------------------------------------

	HSQUIRRELVM m_pVirtualMachine;
	HSQREMOTEDBG m_pDbgServer;

	CUtlHashFast<SQClass *, CUtlHashFastGenericHash> m_ScriptClasses;

	// A reference to our Vector type to compare to
	HSQOBJECT m_VectorClass;

	HSQOBJECT m_CreateScopeClosure;
	HSQOBJECT m_ReleaseScopeClosure;

	SQObjectPtr m_ErrorString;

	ConVarRef developer;

	long long m_nUniqueKeySerial;
	float m_flTimeStartedCall;
};

CSquirrelVM::CSquirrelVM( void )
	: m_pVirtualMachine( NULL ), developer( "developer" ), m_nUniqueKeySerial( 0 ), m_pDbgServer( NULL )
{
	m_VectorClass = _null_;
	m_CreateScopeClosure = _null_;
	m_ReleaseScopeClosure = _null_;
	m_ErrorString = _null_;
}

bool CSquirrelVM::Init( void )
{
	m_pVirtualMachine = sq_open( 1024 );
	
	m_pVirtualMachine->_sharedstate->_up = this;
	m_pVirtualMachine->SetQuerySuspendFn( &CSquirrelVM::QueryContinue );
	
	sq_setprintfunc( GetVM(), &CSquirrelVM::PrintFunc );

	// register libraries
	sq_pushroottable( GetVM() );

	sqstd_register_stringlib( GetVM() );
	sqstd_register_mathlib( GetVM() );
	sqstd_seterrorhandlers( GetVM() );

	sq_pop( GetVM(), 1 );

	if ( IsDebug() || developer.GetInt() )
		sq_enabledebuginfo( GetVM(), SQTrue );

	// register root functions
	sq_pushroottable( GetVM() );

	sq_pushstring( GetVM(), "developer", -1 );
	sq_newclosure( GetVM(), &CSquirrelVM::GetDeveloper, 0 );
	sq_setnativeclosurename( GetVM(), -1, "developer" );
	sq_createslot( GetVM(), -3 );

	sq_pushstring( GetVM(), "GetFunctionSignature", -1 );
	sq_newclosure( GetVM(), &CSquirrelVM::GetFunctionSignature, 0 );
	sq_setnativeclosurename( GetVM(), -1, "GetFunctionSignature" );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	RegisterVector( GetVM() );

	// store a reference to the Vector class for instancing
	sq_pushroottable( GetVM() );

	sq_pushstring( GetVM(), "Vector", -1 );
	if( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) )
	{
		sq_getstackobj( GetVM(), -1, &m_VectorClass );
		sq_addref( GetVM(), &m_VectorClass );

		sq_pop( GetVM(), 1 );
	}

	sq_pop( GetVM(), 1 );

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

		DisconnectDebugger();

		sq_close( m_pVirtualMachine );
		m_pVirtualMachine = NULL;
	}

	m_ScriptClasses.Purge();
}

bool CSquirrelVM::Frame( float simTime )
{
	if ( m_pDbgServer )
	{
		// process outgoing messages
		sq_rdbg_update( m_pDbgServer );

		if ( !m_pDbgServer->IsConnected() )
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
		delete hScript;
	}
}

HSCRIPT CSquirrelVM::CreateScope( const char *pszScope, HSCRIPT hParent )
{
	if ( hParent == NULL )
		hParent = (HSCRIPT)&GetVM()->_roottable;

	HSQOBJECT &pParent = *(HSQOBJECT *)hParent;
	HSQOBJECT pScope ={OT_NULL, NULL};

	// call the utility create function
	sq_pushobject( GetVM(), m_CreateScopeClosure );

	// push parameters
	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pszScope, -1 );
	sq_pushobject( GetVM(), pParent );

	// this pops off the parameters automatically
	if ( SQ_SUCCEEDED( sq_call( GetVM(), 3, SQTrue, SQ_CALL_RAISE_ERROR ) ) )
	{
		sq_getstackobj( GetVM(), -1, &pScope );

		// a result is pushed on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );
	
	// valid return?
	if ( sq_isnull( pScope ) )
		return NULL;
	
	sq_addref( GetVM(), &pScope );

	HSQOBJECT *pObject = new SQObject;
	pObject->_type = pScope._type;
	pObject->_unVal = pScope._unVal;

	return (HSCRIPT)pObject;
}

void CSquirrelVM::ReleaseScope( HSCRIPT hScript )
{
	HSQOBJECT &pObject = *(HSQOBJECT *)hScript;

	// call the utility release function
	sq_pushobject( GetVM(), m_ReleaseScopeClosure );

	// push parameters
	sq_pushroottable( GetVM() );
	sq_pushobject( GetVM(), pObject );

	// this pops off the paramaeters automatically
	sq_call( GetVM(), 2, SQFalse, SQ_CALL_RAISE_ERROR );

	// pop off the closure
	sq_pop( GetVM(), 1 );

	if ( hScript )
	{
		sq_release( GetVM(), (HSQOBJECT *)hScript );
		delete hScript;
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
		delete hScript;
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

	if ( m_pDbgServer )
	{
		if ( g_bSqDebugBreak )
		{
			DisconnectDebugger();
			g_bSqDebugBreak = false;
		}
	}

	SQInteger initialTop = GetVM()->_top;
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
			PushVariant( pArgs[i], true );
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
		ConvertToVariant( _return, pReturn );

		// sq_call pushes a result on success, pop it off
		sq_pop( GetVM(), 1 );
	}

	// pop off the closure
	sq_pop( GetVM(), 1 );

	if ( GetVM()->_top != initialTop )
	{
		Warning( "Callstack mismatch in VScript/Squirrel!\n" );
		Assert( GetVM()->_top == initialTop );
	}

	if ( !sq_isnull( m_ErrorString ) )
	{
		if ( sq_isstring( m_ErrorString ) )
			sq_throwerror( GetVM(), _stringval( m_ErrorString ) );
		else
			sq_throwerror( GetVM(), "Internal error" );

		m_ErrorString = _null_;

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
	UtlHashFastHandle_t hndl = m_ScriptClasses.Find( (uintp)pClassDesc );
	if ( hndl != m_ScriptClasses.InvalidHandle() )
		return true;

	sq_pushroottable( GetVM() );
	sq_pushstring( GetVM(), pClassDesc->m_pszScriptName, -1 );

	if ( SQ_SUCCEEDED( sq_get( GetVM(), -2 ) ) )
	{
		sq_pop( GetVM(), 2 );
		return false;
	}

	sq_pop( GetVM(), 1 );

	if ( pClassDesc->m_pBaseDesc )
		RegisterClass( pClassDesc->m_pBaseDesc );

	int nArgs = sq_gettop( GetVM() );

	HSQOBJECT pObject = CreateClass( pClassDesc );
	SQClass *pClass = _class( pObject );

	if ( pObject != INVALID_HSQOBJECT )
	{
		sq_pushobject( GetVM(), pObject );

		// register our constructor if we have one
		if ( pClassDesc->m_pfnConstruct )
		{
			sq_pushstring( GetVM(), "constructor", -1 );

			ScriptClassDesc_t **pClassDescription = (ScriptClassDesc_t **)sq_newuserdata( GetVM(), sizeof( ScriptClassDesc_t * ) );
			*pClassDescription = pClassDesc;

			sq_newclosure( GetVM(), &CSquirrelVM::CallConstructor, 1 );
			sq_createslot( GetVM(), -3 );
		}

		// _tostring is used for printing objects
		sq_pushstring( GetVM(), "_tostring", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::InstanceToString, 0 );
		sq_createslot( GetVM(), -3 );
		// helper to determine we are a VScript instance
		sq_pushstring( GetVM(), "IsValid", -1 );
		sq_newclosure( GetVM(), &CSquirrelVM::InstanceIsValid, 0 );
		sq_createslot( GetVM(), -3 );

		// register member functions
		FOR_EACH_VEC( pClassDesc->m_FunctionBindings, i )
		{
			RegisterFunctionGuts( &pClassDesc->m_FunctionBindings[i], pClassDesc );
		}
	}

	// restore VM state
	sq_settop( GetVM(), nArgs );

	m_ScriptClasses.FastInsert( (uintp)pClassDesc, pClass );
	return true;
}

HSCRIPT CSquirrelVM::RegisterInstance( ScriptClassDesc_t *pDesc, void *pInstance )
{
	if ( !RegisterClass( pDesc ) )
		return NULL;

	ScriptInstance_t *pScriptInstance = new ScriptInstance_t;
	pScriptInstance->m_pClassDesc = pDesc;
	pScriptInstance->m_pInstance = pInstance;
	pScriptInstance->m_instanceUniqueId = _null_;

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
	delete hScript;
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
				pInstance->m_instanceUniqueId = stack_get( GetVM(), -1 );
			}
		}
	}

	PushVariant( value, true );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return true;
}

void CSquirrelVM::CreateTable( ScriptVariant_t &Table )
{
	HSQOBJECT pObject;
	sq_newtable( GetVM() );

	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );

	ConvertToVariant( pObject, &Table );

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

	ConvertToVariant( pKeyObj, pKey );
	ConvertToVariant( pValueObj, pValue );

	int nNexti = 0;
	sq_getinteger( GetVM(), -1, &nNexti );

	sq_pop( GetVM(), 2 );

	return nNexti;
}

bool CSquirrelVM::GetValue( HSCRIPT hScope, const char *pszKey, ScriptVariant_t *pValue )
{
	HSQOBJECT pObject = LookupObject( pszKey, hScope );
	ConvertToVariant( pObject, pValue );

	return !sq_isnull( pObject );
}

void CSquirrelVM::ReleaseValue( ScriptVariant_t &value )
{
	switch ( value.m_type )
	{
		case FIELD_HSCRIPT:
			sq_release( GetVM(), (HSQOBJECT *)value.m_hScript );
		case FIELD_VECTOR:
		case FIELD_CSTRING:
			value.Free();
		default:
			break;
	}

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
	sq_collectgarbage( GetVM() );

	pBuffer->PutInt( SAVE_VERSION );
	pBuffer->PutInt64( m_nUniqueKeySerial );

	WriteSquirrelState( GetVM(), pBuffer );
}

void CSquirrelVM::ReadState( CUtlBuffer *pBuffer )
{
	if ( pBuffer->GetInt() != SAVE_VERSION )
	{
		DevMsg( "Incompatible script version\n" );
		return;
	}

	sq_collectgarbage( GetVM() );

	int64 serial = pBuffer->GetInt64();
	m_nUniqueKeySerial = Max( m_nUniqueKeySerial, serial );

	ReadSquirrelState( GetVM(), pBuffer );
}

bool CSquirrelVM::ConnectDebugger()
{
	if( developer.GetInt() == 0 )
		return false;

	if ( m_pDbgServer == NULL )
	{
		const int serverPort = 1234;
		m_pDbgServer = sq_rdbg_init( GetVM(), serverPort, SQTrue );
	}

	if ( m_pDbgServer == NULL )
		return false;

	return SQ_SUCCEEDED( sq_rdbg_waitforconnections( m_pDbgServer ) );
}

void CSquirrelVM::DisconnectDebugger()
{
	if ( m_pDbgServer )
	{
		sq_rdbg_shutdown( m_pDbgServer );
		m_pDbgServer = NULL;
	}
}

bool CSquirrelVM::RaiseException( const char *pszExceptionText )
{
	m_ErrorString = SQString::Create( _ss( GetVM() ), pszExceptionText );

	return true;
}

void CSquirrelVM::ConvertToVariant( HSQOBJECT const &pValue, ScriptVariant_t *pVariant )
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
			sq_pushobject( GetVM(), pValue );

			SQUserPointer pInstance = NULL;
			SQRESULT nResult = sq_getinstanceup( GetVM(), -1, &pInstance, VECTOR_TYPE_TAG );

			sq_pop( GetVM(), 1 );

			if ( nResult == SQ_OK )
			{
				*pVariant = new Vector();
				V_memcpy( (void *)pVariant->m_pVector, pInstance, sizeof( Vector ) );
				pVariant->m_flags |= SV_FREE;

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

void CSquirrelVM::PushVariant( ScriptVariant_t const &Variant, bool bDuplicate )
{
	switch ( Variant.m_type )
	{
		case FIELD_VOID:
		{
			sq_pushnull( GetVM() );
			break;
		}
		case FIELD_INTEGER:
		{
			sq_pushinteger( GetVM(), Variant.m_int );
			break;
		}
		case FIELD_FLOAT:
		{
			sq_pushfloat( GetVM(), Variant.m_float );
			break;
		}
		case FIELD_BOOLEAN:
		{
			sq_pushbool( GetVM(), Variant.m_bool );
			break;
		}
		case FIELD_CHARACTER:
		{
			sq_pushstring( GetVM(), &Variant.m_char, 1 );
			break;
		}
		case FIELD_CSTRING:
		{
			char const *szString = Variant.m_pszString ? Variant : "";
			sq_pushstring( GetVM(), szString, V_strlen( szString ) );

			break;
		}
		case FIELD_HSCRIPT:
		{
			if ( Variant.m_hScript )
				sq_pushobject( GetVM(), *(HSQOBJECT *)Variant.m_hScript );
			else
				sq_pushnull( GetVM() );

			break;
		}
		case FIELD_VECTOR:
		{
			sq_pushobject( GetVM(), m_VectorClass );
			sq_createinstance( GetVM(), -1 );

			if ( bDuplicate )
			{
				Vector *pVector = new Vector( Variant );
				sq_setinstanceup( GetVM(), -1, (SQUserPointer)pVector );
				sq_setreleasehook( GetVM(), -1, &VectorRelease );
			}
			else
			{
				sq_setinstanceup( GetVM(), -1, (SQUserPointer)Variant.m_pVector );
			}

			// Remove the class object from stack so we are aligned
			sq_remove( GetVM(), -2 );

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

	HSQOBJECT pObject ={OT_NULL, NULL};
	sq_getstackobj( GetVM(), -1, &pObject );
	sq_addref( GetVM(), &pObject );
	sq_settypetag( GetVM(), -1, pClassDesc );
	sq_createslot( GetVM(), -3 );

	sq_pop( GetVM(), 1 );

	return pObject;
}

bool CSquirrelVM::CreateInstance( ScriptClassDesc_t *pClassDesc, ScriptInstance_t *pInstance, SQRELEASEHOOK fnRelease )
{
	UtlHashFastHandle_t index = m_ScriptClasses.Find( (uintp)pClassDesc );
	if ( index == m_ScriptClasses.InvalidHandle() )
		return false;

	SQObjectPtr pClass( m_ScriptClasses[ index ] );
	sq_pushobject( GetVM(), pClass );
	if ( SQ_FAILED( sq_createinstance( GetVM(), -1 ) ) )
	{
		sq_pop( GetVM(), 1 );
		return false;
	}

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

	char szParamCheck[ MAX_FUNCTION_PARAMS+1 ];
	szParamCheck[0] = '.';

	char *pCurrent = &szParamCheck[1];
	for ( int i=0; i < pFunction->m_desc.m_Parameters.Count(); ++i, pCurrent++ )
	{
		switch ( pFunction->m_desc.m_Parameters[i] )
		{
			case FIELD_INTEGER:
			{
				*pCurrent = 'n';
				break;
			}
			case FIELD_FLOAT:
			{
				*pCurrent = 'n';
				break;
			}
			case FIELD_BOOLEAN:
			{
				*pCurrent = 'b';
				break;
			}
			case FIELD_VECTOR:
			{
				*pCurrent = 'x';
				break;
			}
			case FIELD_CSTRING:
			{
				*pCurrent = 's';
				break;
			}
			case FIELD_HSCRIPT:
			{
				*pCurrent = '.';
				break;
			}
			default:
			{
				*pCurrent = '\0';
				AssertMsg( 0 , "Unsupported type" );
				break;
			}
		}
	}

	// null terminate
	*pCurrent = '\0';
	
	sq_pushstring( GetVM(), pFunction->m_desc.m_pszScriptName, -1 );
	
	ScriptFunctionBinding_t **pFunctionBinding = (ScriptFunctionBinding_t **)sq_newuserdata( GetVM(), sizeof( ScriptFunctionBinding_t * ) );
	*pFunctionBinding = pFunction;

	HSQOBJECT pClosure;
	sq_newclosure( GetVM(), &CSquirrelVM::TranslateCall, 1 );
	sq_getstackobj( GetVM(), -1, &pClosure );
	sq_setnativeclosurename( GetVM(), -1, pFunction->m_desc.m_pszScriptName );
	sq_setparamscheck( GetVM(), pFunction->m_desc.m_Parameters.Count() + 1, szParamCheck );

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

	char const *pszReturnType = "void";
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

		char const *pszArgumentType = "int";
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
	// this pops off the number of parameters automatically
	sq_call( GetVM(), 5, SQFalse, SQ_CALL_RAISE_ERROR );

	// pop off the closure
	sq_pop( GetVM(), 1 );
}

SQInteger CSquirrelVM::CallConstructor( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );

	SQUserPointer pData = hndl.GetUserData( hndl.GetParamCount() );
	ScriptClassDesc_t *pClassDesc = ( *(ScriptClassDesc_t **)pData );

	ScriptInstance_t *pInstance = new ScriptInstance_t;
	pInstance->m_pClassDesc = pClassDesc;
	pInstance->m_pInstance = pClassDesc->m_pfnConstruct();

	sq_setinstanceup( pVM, 1, pInstance );
	sq_setreleasehook( pVM, 1, CSquirrelVM::ReleaseHook );

	return SQ_OK;
}

SQInteger CSquirrelVM::ReleaseHook( SQUserPointer data, SQInteger size )
{
	ScriptInstance_t *pObject = (ScriptInstance_t *)data;
	if( pObject )
	{
		pObject->m_pClassDesc->m_pfnDestruct( pObject->m_pInstance );
		delete pObject;
	}

	return SQ_OK;
}

SQInteger CSquirrelVM::ExternalReleaseHook( SQUserPointer data, SQInteger size )
{
	ScriptInstance_t *pObject = (ScriptInstance_t *)data;
	if( pObject )
		delete pObject;

	return SQ_OK;
}

SQInteger CSquirrelVM::GetDeveloper( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	return hndl.Return( GetVScript( pVM )->developer.GetInt() );
}

SQInteger CSquirrelVM::GetFunctionSignature( HSQUIRRELVM pVM )
{
	static char szSignature[512];

	StackHandler hndl( pVM );
	if ( hndl.GetParamCount() != 3 )
		return hndl.Return();

	HSQOBJECT pObject = hndl.GetObjectHandle( 2 );
	if ( !sq_isclosure( pObject ) )
		return hndl.Return();

	V_memset( szSignature, 0, sizeof szSignature );

	char const *pszName = hndl.GetString( 3 );
	SQClosure *pClosure = _closure( pObject );
	SQFunctionProto *pPrototype = _funcproto( pClosure->_function );

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

	return hndl.Return( szSignature );
}

SQInteger CSquirrelVM::TranslateCall( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	CUtlVectorFixed<ScriptVariant_t, MAX_FUNCTION_PARAMS> parameters;

	SQUserPointer pData = hndl.GetUserData( hndl.GetParamCount() );
	ScriptFunctionBinding_t *pFuncBinding = ( *(ScriptFunctionBinding_t **)pData );
	CUtlVector<ScriptDataType_t> const &fnParams = pFuncBinding->m_desc.m_Parameters;

	parameters.SetCount( fnParams.Count() );

	const int nArguments = Max( fnParams.Count(), hndl.GetParamCount() );
	for ( int i=0; i < nArguments; ++i )
	{
		switch ( fnParams.Element( i ) )
		{
			case FIELD_INTEGER:
			{
				parameters[i] = hndl.GetInt( i+2 );
				break;
			}
			case FIELD_FLOAT:
			{
				parameters[i] = hndl.GetFloat( i+2 );
				break;
			}
			case FIELD_BOOLEAN:
			{
				parameters[i] = hndl.GetBool( i+2 ) == SQTrue;
				break;
			}
			case FIELD_CHARACTER:
			{
				char const *pChar = hndl.GetString( i+2 );
				if ( pChar == NULL )
					pChar = "\0";

				parameters[i] = *pChar;
				break;
			}
			case FIELD_CSTRING:
			{
				parameters[i] = hndl.GetString( i+2 );
				break;
			}
			case FIELD_VECTOR:
			{
				SQUserPointer pInstance = hndl.GetInstanceUp( i+2, VECTOR_TYPE_TAG );
				if ( pInstance == NULL )
					return hndl.ThrowError( "Vector argument expected" );

				parameters[i] = (Vector *)pInstance;
				break;
			}
			case FIELD_HSCRIPT:
			{
				HSQOBJECT pObject = hndl.GetObjectHandle( i+2 );
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
				break;
		}
	}

	SQUserPointer pContext = NULL;
	if ( pFuncBinding->m_flags & SF_MEMBER_FUNC )
	{
		ScriptInstance_t *pInstance = (ScriptInstance_t *)hndl.GetInstanceUp( 1, NULL );
		if ( pInstance == NULL || pInstance->m_pInstance == NULL )
			return hndl.ThrowError( "Accessed null instance" );

		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			pContext = pHelper->GetProxied( pInstance->m_pInstance );
			if ( pContext == NULL )
				return hndl.ThrowError( "Accessed null instance" );
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

	if ( bHasReturn )
	{
		switch ( pFuncBinding->m_desc.m_ReturnType )
		{
			case FIELD_INTEGER:
			{
				sq_pushinteger( pVM, returnValue.m_int );
				break;
			}
			case FIELD_FLOAT:
			{
				sq_pushfloat( pVM, returnValue.m_int );
				break;
			}
			case FIELD_BOOLEAN:
			{
				sq_pushbool( pVM, returnValue.m_bool );
				break;
			}
			case FIELD_CSTRING:
			{
				char const *pString = "";
				if ( returnValue.m_pszString )
					pString = returnValue;

				sq_pushstring( pVM, pString, -1 );
				break;
			}
			case FIELD_VECTOR:
			{
				sq_pushobject( pVM, GetVScript( pVM )->m_VectorClass );

				sq_createinstance( pVM, -1 );
				sq_setinstanceup( pVM, -1, (SQUserPointer)returnValue.m_pVector );
				sq_setreleasehook( pVM, -1, &VectorRelease );

				// Remove the class object from stack so we are aligned
				sq_remove( pVM, -2 );

				break;
			}
			case FIELD_HSCRIPT:
			{
				if ( returnValue.m_hScript )
					sq_pushobject( pVM, *(HSQOBJECT *)returnValue.m_hScript );
				else
					sq_pushnull( pVM );

				break;
			}
			default:
			{
				sq_pushnull( pVM );
				break;
			}
		}
	}

	for ( int i=0; i < parameters.Count(); ++i )
	{
		parameters[i].Free();
	}

	HSQOBJECT pErrorString = GetVScript( pVM )->m_ErrorString;
	if ( !sq_isnull( pErrorString ) )
	{
		if ( sq_isstring( pErrorString ) )
			sq_throwerror( pVM, _stringval( pErrorString ) );
		else
			sq_throwerror( pVM, "Internal error" );

		GetVScript( pVM )->m_ErrorString = _null_;
		return SQ_ERROR;
	}

	return pFuncBinding->m_desc.m_ReturnType != FIELD_VOID;
}

SQInteger CSquirrelVM::InstanceToString( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );

	ScriptInstance_t *pInstance = (ScriptInstance_t *)hndl.GetInstanceUp( 1, NULL );
	if ( pInstance && pInstance->m_pInstance )
	{
		IScriptInstanceHelper *pHelper = pInstance->m_pClassDesc->pHelper;
		if ( pHelper )
		{
			char szInstance[64];
			if ( pHelper->ToString( pInstance->m_pInstance, szInstance, sizeof( szInstance ) ) )
				return hndl.Return( szInstance );
		}
	}

	HSQOBJECT pObject = hndl.GetObjectHandle( 1 );
	return hndl.Return( CFmtStr( "(instance : 0x%p)", pObject._unVal.pInstance ) );
}

SQInteger CSquirrelVM::InstanceIsValid( HSQUIRRELVM pVM )
{
	StackHandler hndl( pVM );
	ScriptInstance_t *pInstance = (ScriptInstance_t *)hndl.GetInstanceUp( 1, NULL );
	return hndl.Return( pInstance && pInstance->m_pInstance );
}

void CSquirrelVM::PrintFunc( HSQUIRRELVM pVM, const SQChar *fmt, ... )
{
	static char szMessage[2048]{};

	va_list va;
	va_start( va, szMessage );
	V_vsnprintf( szMessage, sizeof( szMessage ), fmt, va );
	va_end( va );

	Msg( "%s", szMessage );
}

int CSquirrelVM::QueryContinue( HSQUIRRELVM pVM )
{
	CSquirrelVM *pVScript = GetVScript( pVM );
	const float flStartTime = pVScript->m_flTimeStartedCall;
	if ( !pVScript->m_pDbgServer && flStartTime != 0.0f )
	{
		const float flTimeDelta = Plat_FloatTime() - flStartTime;
		if ( flTimeDelta > 0.03f )
		{
			scprintf(_sqT("Script running too long, terminating\n"));
			return SQ_QUERY_BREAK;
		}
	}
	return SQ_QUERY_CONTINUE;
}

HSQOBJECT CSquirrelVM::LookupObject( char const *szName, HSCRIPT hScope, bool bRefCount )
{
	HSQOBJECT pObject ={OT_NULL, NULL};
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
	if( pVM ) delete pVM;
}
