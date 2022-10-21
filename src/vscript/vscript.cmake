# vscript.cmake

set(VSCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(
	VSCRIPT_SOURCE_FILES

	# Source Files
	"${VSCRIPT_DIR}/vscript.cpp"
	"${VSCRIPT_DIR}/vscript_math.cpp"
	"${VSCRIPT_DIR}/vscript_misc.cpp"
	"${VSCRIPT_DIR}/languages/squirrel/vsquirrel.cpp"
	"${VSCRIPT_DIR}/languages/squirrel/vsquirrel_math.cpp"
	"${VSCRIPT_DIR}/languages/squirrel/sq_vmstate.cpp"
	"${VSCRIPT_DIR}/languages/lua/lua_vm.cpp"
	"${VSCRIPT_DIR}/languages/lua/lua_vector.cpp"
	# "${VSCRIPT_DIR}/languages/angelscript/vangelscript.cpp"
	# "${VSCRIPT_DIR}/languages/angelscript/as_vector.cpp"

	# Public Header Files
	"${SRCDIR}/public/vscript/ivscript.h"	
	"${SRCDIR}/public/vscript/vscript_templates.h"

	# Header Files
	"${VSCRIPT_DIR}/vscript_math.h"
	"${VSCRIPT_DIR}/vscript_misc.h"
	"${VSCRIPT_DIR}/languages/squirrel/vsquirrel.h"
	"${VSCRIPT_DIR}/languages/squirrel/vsquirrel_math.h"
	"${VSCRIPT_DIR}/languages/squirrel/sq_vmstate.h"
	"${VSCRIPT_DIR}/languages/lua/lua_vm.h"
	"${VSCRIPT_DIR}/languages/lua/lua_vector.h"
	# "${VSCRIPT_DIR}/languages/angelscript/vangelscript.h"
	# "${VSCRIPT_DIR}/languages/angelscript/as_vector.h"

	# Squirrel Lang
	# Source Files
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqapi.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqbaselib.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqclass.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqcompiler.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqdbg/sqdbgserver.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqdebug.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqfuncstate.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqlexer.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqmem.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqobject.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqdbg/sqrdbg.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqstate.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqtable.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqvm.cpp"

	# Header Files
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqarray.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqclass.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqclosure.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqcompiler.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqfuncproto.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqfuncstate.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqlexer.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqobject.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqopcodes.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqpcheader.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqstate.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqstring.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqtable.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/squserdata.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/squtils.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel/sqvm.h"

	# SQSTDLib
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdblob.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdmath.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdrex.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdstring.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdstream.cpp"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqstdlib/sqstdaux.cpp"

	# Public Header Files
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqconfig.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqstdaux.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqstdio.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqstdmath.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqstdstring.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqstdsystem.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/squirrel.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqrdbg.h"
	"${SRCDIR}/thirdparty/SQUIRREL3/include/sqdbgserver.h"

	# LuaJIT
	# Header Files
	"${SRCDIR}/thirdparty/luajit/src/lauxlib.h"
	"${SRCDIR}/thirdparty/luajit/src/lualib.h"
	"${SRCDIR}/thirdparty/luajit/src/lua.h"
	"${SRCDIR}/thirdparty/luajit/src/luaconf.h"
	"${SRCDIR}/thirdparty/luajit/src/luajit.h"

	# AngelScript
	# Source Files
	# "$<${IS_LINUX}:${VSCRIPT_DIR}/languages/angelscript/virtual_asm_linux.cpp>"
	# "$<${IS_WINDOWS}:${VSCRIPT_DIR}/languages/angelscript/virtual_asm_windows.cpp>"
	# "${VSCRIPT_DIR}/languages/angelscript/virtual_asm_x86.cpp"
	# "${VSCRIPT_DIR}/languages/angelscript/as_jit.cpp"

	# Header Files
	# "${VSCRIPT_DIR}/languages/angelscript/as_jit.h"
	# "${VSCRIPT_DIR}/languages/angelscript/virtual_asm.h"

	# Public Header Files
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/angelscript/include/angelscript.h"

    # Add-Ons
	# Source Files
    # "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/contextmgr/contextmgr.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/debugger/debugger.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptany/scriptany.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptarray/scriptarray.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptdictionary/scriptdictionary.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptgrid/scriptgrid.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthandle/scripthandle.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthelper/scripthelper.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmath.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmathcomplex.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/serializer/serializer.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/weakref/weakref.cpp"

	# Header Files
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/contextmgr/contextmgr.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/debugger/debugger.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptany/scriptany.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptarray/scriptarray.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptbuilder/scriptbuilder.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptdictionary/scriptdictionary.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptgrid/scriptgrid.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthandle/scripthandle.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthelper/scripthelper.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmath.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmathcomplex.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptstdstring/scriptstdstring.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/serializer/serializer.h"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/weakref/weakref.h"
)

# set_source_files_properties(
	# "${VSCRIPT_DIR}/languages/angelscript/virtual_asm_x86.cpp"
	# "${VSCRIPT_DIR}/languages/angelscript/as_jit.cpp"
    # "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/contextmgr/contextmgr.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/debugger/debugger.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptany/scriptany.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptarray/scriptarray.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptbuilder/scriptbuilder.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptdictionary/scriptdictionary.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptgrid/scriptgrid.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthandle/scripthandle.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scripthelper/scripthelper.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmath.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptmath/scriptmathcomplex.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptstdstring/scriptstdstring.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/serializer/serializer.cpp"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on/weakref/weakref.cpp"
	# PROPERTIES COMPILE_FLAGS "/EHa"
# )

add_library(vscript MODULE ${VSCRIPT_SOURCE_FILES})

set_target_properties(
	vscript PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${GAMEDIR}/tf2vintage/bin"
)

target_include_directories(
	vscript PRIVATE
	"${SRCDIR}/public/vscript"
	"${SRCDIR}/thirdparty/SQUIRREL3/include"
	"${SRCDIR}/thirdparty/SQUIRREL3/squirrel"
	"${SRCDIR}/thirdparty/SQUIRREL3/sqplus"
	"${SRCDIR}/thirdparty/luajit/src"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/angelscript/include"
	# "${SRCDIR}/thirdparty/angelscript_2.32.0/sdk/add_on"

)

# Nut
add_custom_command(
	TARGET vscript
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} "${SRCDIR}/devtools/bin/texttoarray.py" --file "${VSCRIPT_DIR}/languages/squirrel/vscript_init.nut" --name "g_Script_init" --out "${VSCRIPT_DIR}/languages/squirrel/vscript_init_nut.h"
	COMMENT "vscript_init.nut produces vscript_init_nut.h"
)

target_compile_definitions(
	vscript PRIVATE
	VSCRIPT_DLL_EXPORT
	CROSS_PLATFORM_VERSION=1
)

target_link_libraries(
	vscript PRIVATE

	mathlib
	tier1
	"$<${IS_WINDOWS}:${LIBCOMMON}/lua51.lib>"
	"$<${IS_LINUX}:${LIBCOMMON}/linux32/libluajit.a>"
	# "$<${IS_LINUX}:${LIBCOMMON}/linux32/angelscript.a>"
	# "$<${IS_WINDOWS}:${LIBCOMMON}/angelscript.lib>"
)