#include "mathlib/mathlib.h"
#include "tier0/platform.h"
#include "tier0/icommandline.h"
#include "vscript/ivscript.h"
#include "vsquirrel.h"

#include <time.h>
#include <Windows.h>

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

matrix3x4_t const &ScriptMatrixSetColumn( Vector vecset, int column, matrix3x4_t mat )
{
	static matrix3x4_t result;
	MatrixCopy( mat, result );
	MatrixSetColumn( vecset, column, result );
	return result;
}

QAngle const &ScriptMatrixAngles( matrix3x4_t mat )
{
	static QAngle result;
	MatrixAngles( mat, result );
	return result;
}

Quaternion const &ScriptMatrixQuaternion( matrix3x4_t mat )
{
	static Quaternion result;
	MatrixQuaternion( mat, result );
	return result;
}


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

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixSetColumn, "MatrixSetColumn", "Sets the column of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixAngles, "MatrixAngles", "Gets the angles and position of a matrix." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatrixQuaternion, "MatrixQuaternion", "Converts a matrix to a quaternion." );

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
				ScriptVariant_t	Table;
				g_pScriptVM->CreateTable( Table );
				g_pScriptVM->SetValue( Table, "controlpoint_1_vector", Vector( 23, 4, 15 ) );
				scope.SetValue( "mytable", Table );

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