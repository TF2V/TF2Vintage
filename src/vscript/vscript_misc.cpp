#include "vscript/ivscript.h"
#include "tier1/tier1.h"
#include "tier0/platform.h"
#include "Color.h"
#include "icommandline.h"
#include "worldsize.h"
#include "bspflags.h"

#include "vscript_misc.h"
#include "vscript_math.h"

static void ScriptColorPrint( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s", pszMsg );
}

static void ScriptColorPrintL( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s\n", pszMsg );
}

//=============================================================================
//
// Command Line
// 
//=============================================================================
static class CGlobalSys
{
public:
	const char* ScriptGetCommandLine()
	{
		return CommandLine()->GetCmdLine();
	}

	bool CommandLineCheck(const char* name)
	{
		return !!CommandLine()->FindParm(name);
	}

	const char* CommandLineCheckStr(const char* name)
	{
		return CommandLine()->ParmValue(name);
	}

	float CommandLineCheckFloat(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}

	int CommandLineCheckInt(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}
} g_ScriptGlobalSys;

BEGIN_SCRIPTDESC_ROOT( CGlobalSys, SCRIPT_SINGLETON "GlobalSys" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetCommandLine, "GetCommandLine", "returns the command line" )
	DEFINE_SCRIPTFUNC( CommandLineCheck, "returns true if the command line param was used, otherwise false." )
	DEFINE_SCRIPTFUNC( CommandLineCheckStr, "returns the command line param as a string." )
	DEFINE_SCRIPTFUNC( CommandLineCheckFloat, "returns the command line param as a float." )
	DEFINE_SCRIPTFUNC( CommandLineCheckInt, "returns the command line param as an int." )
END_SCRIPTDESC();



void RegisterBaseBindings( IScriptVM *pVM )
{
	ScriptRegisterFunctionNamed( pVM, ScriptColorPrint, "printc", "Version of print() which takes a color before the message." );
	ScriptRegisterFunctionNamed( pVM, ScriptColorPrintL, "printcl", "Version of printl() which takes a color before the message." );

	//-----------------------------------------------------------------------------

	pVM->RegisterInstance( &g_ScriptGlobalSys, "GlobalSys" );

	//-----------------------------------------------------------------------------

	//
	// Math/world
	//
	ScriptRegisterConstant( pVM, MAX_COORD_FLOAT, "Maximum float coordinate." );
	ScriptRegisterConstant( pVM, MAX_TRACE_LENGTH, "Maximum traceable distance (assumes cubic world and trace from one corner to opposite)." );

	// 
	// Trace Contents/Masks
	// 
	ScriptRegisterConstant( pVM, CONTENTS_EMPTY, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_SOLID, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_WINDOW, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_AUX, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_GRATE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_SLIME, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_WATER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_BLOCKLOS, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_OPAQUE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TESTFOGVOLUME, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TEAM1, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TEAM2, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_IGNORE_NODRAW_OPAQUE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MOVEABLE, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_AREAPORTAL, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_PLAYERCLIP, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MONSTERCLIP, "Spatial content flags." );

	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_0, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_90, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_180, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_270, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_UP, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_CURRENT_DOWN, "Spatial content flags." );

	ScriptRegisterConstant( pVM, CONTENTS_ORIGIN, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_MONSTER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_DEBRIS, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_DETAIL, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_TRANSLUCENT, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_LADDER, "Spatial content flags." );
	ScriptRegisterConstant( pVM, CONTENTS_HITBOX, "Spatial content flags." );

	ScriptRegisterConstant( pVM, LAST_VISIBLE_CONTENTS, "Contains last visible spatial content flags." );
	ScriptRegisterConstant( pVM, ALL_VISIBLE_CONTENTS, "Contains all visible spatial content flags." );

	ScriptRegisterConstant( pVM, MASK_SOLID, "Spatial content mask representing solid objects (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_PLAYERSOLID, "Spatial content mask representing objects solid to the player, including player clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCSOLID, "Spatial content mask representing objects solid to NPCs, including NPC clips (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_WATER, "Spatial content mask representing water and slime solids (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME)" );
	ScriptRegisterConstant( pVM, MASK_OPAQUE, "Spatial content mask representing objects which block lighting (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_OPAQUE_AND_NPCS, "Spatial content mask equivalent to MASK_OPAQUE, but also including NPCs (MASK_OPAQUE|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_BLOCKLOS, "Spatial content mask representing objects which block LOS for AI (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)" );
	ScriptRegisterConstant( pVM, MASK_BLOCKLOS_AND_NPCS, "Spatial content mask equivalent to MASK_BLOCKLOS, but also including NPCs (MASK_BLOCKLOS|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_VISIBLE, "Spatial content mask representing objects which block LOS for players (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_VISIBLE_AND_NPCS, "Spatial content mask equivalent to MASK_VISIBLE, but also including NPCs (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE)" );
	ScriptRegisterConstant( pVM, MASK_SHOT, "Spatial content mask representing objects solid to bullets (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)" );
	ScriptRegisterConstant( pVM, MASK_SHOT_HULL, "Spatial content mask representing objects solid to non-raycasted weapons, including grates (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_SHOT_PORTAL, "Spatial content mask equivalent to MASK_SHOT, but excluding debris and not using expensive hitbox calculations (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)" );
	ScriptRegisterConstant( pVM, MASK_SOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_SOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_PLAYERSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_PLAYERSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCSOLID_BRUSHONLY, "Spatial content mask equivalent to MASK_NPCSOLID, but without NPCs (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_NPCWORLDSTATIC, "Spatial content mask representing objects static to NPCs, used for nodegraph rebuilding (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)" );
	ScriptRegisterConstant( pVM, MASK_SPLITAREAPORTAL, "Spatial content mask representing objects which can split areaportals (CONTENTS_WATER|CONTENTS_SLIME)" );

	// 
	// Misc. General
	// 
	ScriptRegisterConstant( pVM, FCVAR_NONE, "Empty convar flag." );
	ScriptRegisterConstant( pVM, FCVAR_UNREGISTERED, "If this convar flag is set, it isn't added to linked list, etc." );
	ScriptRegisterConstant( pVM, FCVAR_DEVELOPMENTONLY, "If this convar flag is set, it's hidden in \"retail\" DLLs." );
	ScriptRegisterConstant( pVM, FCVAR_GAMEDLL, "This convar flag is defined in server DLL convars." );
	ScriptRegisterConstant( pVM, FCVAR_CLIENTDLL, "This convar flag is defined in client DLL convars." );
	ScriptRegisterConstant( pVM, FCVAR_HIDDEN, "If this convar flag is set, it doesn't appear in the console or any searching tools, but it can still be set." );
	ScriptRegisterConstant( pVM, FCVAR_PROTECTED, "This convar flag prevents convars with secure data (e.g. passwords) from sending full data to clients, only sending 1 if non-zero and 0 otherwise." );
	ScriptRegisterConstant( pVM, FCVAR_SPONLY, "If this convar flag is set, it can't be changed by clients connected to a multiplayer server." );
	ScriptRegisterConstant( pVM, FCVAR_ARCHIVE, "If this convar flag is set, its value will be saved when the game is exited." );
	ScriptRegisterConstant( pVM, FCVAR_NOTIFY, "If this convar flag is set, it will notify players when it is changed." );
	ScriptRegisterConstant( pVM, FCVAR_USERINFO, "If this convar flag is set, it will be marked as info which plays a part in how the server identifies a client." );
	ScriptRegisterConstant( pVM, FCVAR_PRINTABLEONLY, "If this convar flag is set, it cannot contain unprintable characters. Used for player name cvars, etc." );
	ScriptRegisterConstant( pVM, FCVAR_UNLOGGED, "If this convar flag is set, it will not log its changes if a log is being created." );
	ScriptRegisterConstant( pVM, FCVAR_NEVER_AS_STRING, "If this convar flag is set, it will never be printed as a string." );
	ScriptRegisterConstant( pVM, FCVAR_REPLICATED, "If this convar flag is set, it will enforce a serverside value on any clientside counterparts. (also known as FCAR_SERVER)" );
	ScriptRegisterConstant( pVM, FCVAR_DEMO, "If this convar flag is set, it will be recorded when starting a demo file." );
	ScriptRegisterConstant( pVM, FCVAR_DONTRECORD, "If this convar flag is set, it will NOT be recorded when starting a demo file." );
	ScriptRegisterConstant( pVM, FCVAR_RELOAD_MATERIALS, "If this convar flag is set, it will force a material reload when it changes." );
	ScriptRegisterConstant( pVM, FCVAR_RELOAD_TEXTURES, "If this convar flag is set, it will force a texture reload when it changes." );
	ScriptRegisterConstant( pVM, FCVAR_NOT_CONNECTED, "If this convar flag is set, it cannot be changed by a client connected to the server." );
	ScriptRegisterConstant( pVM, FCVAR_MATERIAL_SYSTEM_THREAD, "This convar flag indicates it's read from the material system thread." );
	ScriptRegisterConstant( pVM, FCVAR_ARCHIVE_XBOX, "If this convar flag is set, it will be archived on the Xbox config." );
	ScriptRegisterConstant( pVM, FCVAR_ACCESSIBLE_FROM_THREADS, "If this convar flag is set, it will be accessible from the material system thread." );
	ScriptRegisterConstant( pVM, FCVAR_SERVER_CAN_EXECUTE, "If this convar flag is set, the server will be allowed to execute it as a client command." );
	ScriptRegisterConstant( pVM, FCVAR_SERVER_CANNOT_QUERY, "If this convar flag is set, the server will not be allowed to query its value." );
	ScriptRegisterConstant( pVM, FCVAR_CLIENTCMD_CAN_EXECUTE, "If this convar flag is set, any client will be allowed to execute this command." );

	//-----------------------------------------------------------------------------

	RegisterMathBaseBindings( pVM );
}