#include "mathlib/mathlib.h"
#include "tier0/platform.h"
#include "tier0/icommandline.h"
#include "vscript/ivscript.h"
#include "vsquirrel.h"

#include <time.h>
#include <Windows.h>
#undef RegisterClass

IScriptVM *g_pScriptVM = NULL;

static ScriptVariant_t TestReturn()
{
	return ScriptVariant_t( "test" );
}

void TestOutput( const char *pszText )
{
	Msg( "%s\n", pszText );
}

bool TestError( ScriptErrorLevel_t eLevel, const char *pszText )
{
	Msg( "%s\n", pszText );

	return true;
}

class CMyClass
{
	ALLOW_SCRIPT_ACCESS();
public:
	CMyClass() : a( 7 ), b( 3.33 ), c( 1, 2, 3 ) {}
	bool Foo( int );
	void Bar( void );
	float FooBar( int, const char * );
	float OverlyTechnicalName( bool );

protected:
	int a;
	float b;
	Vector c;
};

bool CMyClass::Foo( int test )
{
	return true;
}

void CMyClass::Bar( void )
{
}

float CMyClass::FooBar( int test1, const char *test2 )
{
	return 13.37f;
}

float CMyClass::OverlyTechnicalName( bool test )
{
	return M_PI/180;
}

BEGIN_SCRIPTDESC_ROOT_NAMED( CMyClass, "CMyClass", SCRIPT_SINGLETON "" )
	DEFINE_SCRIPTFUNC( Foo, "" )
	DEFINE_SCRIPTFUNC( Bar, "" )
	DEFINE_SCRIPTFUNC( FooBar, "" )
	DEFINE_SCRIPTFUNC_NAMED( OverlyTechnicalName, "SimpleMemberName", "" )
	DEFINE_MEMBERVAR( a, FIELD_INTEGER, "" )
	DEFINE_MEMBERVAR( b, FIELD_FLOAT, "" )
	DEFINE_MEMBERVAR( c, FIELD_VECTOR, "" )
END_SCRIPTDESC();

class CMyDerivedClass : public CMyClass
{
	ALLOW_SCRIPT_ACCESS();
public:
	CMyDerivedClass() : d( true ) {}
	float DerivedFunc() const;

private:
	bool d;
};

BEGIN_SCRIPTDESC( CMyDerivedClass, CMyClass, SCRIPT_SINGLETON "" )
	DEFINE_SCRIPTFUNC( DerivedFunc, "" )
	DEFINE_MEMBERVAR( a, FIELD_INTEGER, "" )
	DEFINE_MEMBERVAR( b, FIELD_FLOAT, "" )
	DEFINE_MEMBERVAR( c, FIELD_VECTOR, "" )
	DEFINE_MEMBERVAR( d, FIELD_BOOLEAN, "" )
END_SCRIPTDESC();

float CMyDerivedClass::DerivedFunc() const
{
	return M_PI*2;
}

CMyDerivedClass derivedInstance;


int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		printf( "No script specified" );
		return 1;
	}

	g_pScriptVM = CreateSquirrelVM();
	Assert( g_pScriptVM != NULL );

	g_pScriptVM->Init();

	g_pScriptVM->SetOutputCallback( TestOutput );
	g_pScriptVM->SetErrorCallback( TestError );

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2, true, true, false, false );

	RandomSeed( time( NULL ) );
	ScriptRegisterFunction( g_pScriptVM, RandomInt, "" );
	ScriptRegisterFunction( g_pScriptVM, RandomFloat, "" );

	ScriptRegisterFunction( g_pScriptVM, TestReturn, "" );

	// Manual class exposure
	g_pScriptVM->RegisterClass( GetScriptDescForClass( CMyClass ) );

	// Auto registration by instance
	g_pScriptVM->RegisterInstance( &derivedInstance, "theInstance" );

	if ( argc == 3 && *argv[2] == 'd' )
	{
		g_pScriptVM->ConnectDebugger();
	}

	do
	{
		const char *pszScript = argv[1];
		FILE *hFile = fopen( pszScript, "rb" );
		if ( !hFile )
		{
			printf( "\"%s\" not found.\n", pszScript );
			return 1;
		}

		fseek( hFile, 0, SEEK_END );
		int nFileLen = ftell( hFile );
		fseek( hFile, 0, SEEK_SET );

		char *pBuf = new char[nFileLen + 1];
		fread( pBuf, 1, nFileLen, hFile );
		pBuf[nFileLen] = 0;
		fclose( hFile );
		
		CScriptScope scope;
		if ( scope.Init( "TestScope" ) )
		{
			printf( "Executing script \"%s\"\n----------------------------------------\n", pszScript );
			HSCRIPT hScript = g_pScriptVM->CompileScript( pBuf, V_GetFileName( pszScript ) );
			if ( hScript )
			{
				if ( scope.Run( hScript ) != SCRIPT_ERROR )
				{
					printf( "----------------------------------------\n" );
					printf( "Script complete.  Press q to exit, enter to run again.\n" );
				}
				else
				{
					printf( "----------------------------------------\n" );
					printf( "Script execution error.  Press q to exit, enter to run again.\n" );
				}
				g_pScriptVM->ReleaseScript( hScript );
			}
			else
			{
				printf( "----------------------------------------\n" );
				printf( "Script failed to compile.  Press q to exit, enter to run again.\n" );
			}
		}
		delete pBuf;
	} while ( getchar() != 'q' );

	g_pScriptVM->DisconnectDebugger();

	g_pScriptVM->Shutdown();
	DestroySquirrelVM( g_pScriptVM );

	return 0;
}