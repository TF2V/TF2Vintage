# msvc_base.cmake

# If using CMake, we'll require 2015 toolset or greater
if (MSVC_TOOLSET_VERSION LESS 140)
	message(FATAL_ERROR "MSVC must use toolset 140 (2015) or greater")
endif()

if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(FATAL_ERROR "Source SDK 2013 only supports 32-bit generation")
endif()

# No frame pointer optimization
set(NOFPO 1)

set(MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_DEBUG
	/NODEFAULTLIB:libc
	/NODEFAULTLIB:libcd
	/NODEFAULTLIB:libcmt
	/NODEFAULTLIB:libcpmt
	/NODEFAULTLIB:libcpmt1
)
set(MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_RELEASE
	/NODEFAULTLIB:libc
	/NODEFAULTLIB:libcd
	/NODEFAULTLIB:libcmtd
	/NODEFAULTLIB:libcpmtd
	/NODEFAULTLIB:libcpmtd0
	/NODEFAULTLIB:libcpmtd1
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(
	COMPILER_MSVC
	_CRT_SECURE_NO_DEPRECATE
	_CRT_NONSTDC_NO_DEPRECATE
	_ALLOW_RUNTIME_LIBRARY_MISMATCH
	_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH
	_ALLOW_MSC_VER_MISMATCH
	_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
)

# Remove default warning level from CMAKE_CXX_FLAGS (This is stupid I know)
string (REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Disable C++ exceptions by default
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "/Zi" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "/Ob0" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

string(REPLACE "/Ob2" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
string(REPLACE "/Ob2" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Disable some warnings from now
# Remove me once all of them are fixed.
# C4458: declaration of 'X' hides class member
# C5208: unnamed class used in typedef name cannot declare members other than non-static data members, member enumerations, or member classes
# C4459: declaration of 'X' hides global declaration
# C4457: declaration of 'X' hides function parameter
# C4456: declaration of 'X' hides local declaration
# C5105: macro expansion producing 'x' has undefined behavior
# C4463: overflow; assigning 1 to bit-field that can only hold values from -1 to 0
# C26495: Variable 'X' is uninitialized. Always initialize a member variable (type.6).
# C26451: Arithmetic overflow: Using operator 'x' on a 4 byte value and then casting the result to a 8 byte value. Cast the value to the wider type before calling operator 'x' to avoid overflow (io.2).
# C26819: Unannotated fallthrough between switch labels (es.78).
# C4838: conversion from 'x' to 'x' requires a narrowing conversion.
# C5205: delete of an abstract class 'x' that has a non-virtual destructor results in undefined behavior
# C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead.
# C5205: delete of an abstract class 'IHaptics' that has a non-virtual destructor results in undefined behavior
# C6308: 'y' might return null pointer: assigning null pointer to 'X', which is passed as an argument to 'y', will cause the original memory block to be leaked.
# C6385: Reading invalid data from 'X'.
# C4189: 'X': local variable is initialized but not referenced
# C4706: assignment within conditional expression.
# C6269: Possibly incorrect order of operations.
add_definitions( "/wd4458 /wd5208 /wd4459 /wd4457 /wd4456 /wd5105 /wd4463 /wd26495 /wd26451 /wd26819 /wd4838 /wd6255 /wd5205 /wd6308 /wd6385 /wd4189 /wd4706 /wd6269" )

# These are expanded so that we can pass each option individually to the targets
# So they may choose to exclude them
set(
	MSVC_C_AND_CXX_COMPILE_OPTIONS

	# Set warning level
	/W4

	# Treat warnings as errors... someday
	#/WX

	# Don't Omit Frame Pointers
	"$<${NOFPO}:/Oy->"

	/MP # Multi-processor compilation
	/Gw
	/Zc:threadSafeInit-
	/Zc:__cplusplus
	/Zc:preprocessor
	/Zc:inline

	# Turn on some cpu features
	/arch:SSE
	/arch:SSE2
	/arch:AVX
	/arch:AVX2
	/arch:AVX512

	# We'll be permissive for now
#	/permissive-

	/GR # Enable Run-Time Type Information
	/GF # Enable String Pooling
	/fp:fast # Floating Point Model
	/GS- # Buffer Security Check

	$<$<CONFIG:Release>:/Oi> # Enable Intrinsic Functions
	$<$<CONFIG:Release>:/Ot> # Favor Fast Code
	$<$<CONFIG:Release>:/Gy> # Enable Function-Level Linking

	# Inline Function Expansion
	$<$<CONFIG:Release>:/Ob2>
	$<$<CONFIG:Debug>:/Ob0>
)

add_link_options(
	$<$<CONFIG:Debug>:/DEBUG:FASTLINK>
	$<$<CONFIG:Release>:/DEBUG:FULL>
)

list(
	APPEND ADDITIONAL_COMPILE_OPTIONS_EXE
	$<$<CONFIG:Debug>:/ZI>
	$<$<CONFIG:Release>:/Zi>
	"${MSVC_C_AND_CXX_COMPILE_OPTIONS}"
)

list(
	APPEND ADDITIONAL_COMPILE_OPTIONS_DLL
	$<$<CONFIG:Debug>:/ZI>
	$<$<CONFIG:Release>:/Zi>
	"${MSVC_C_AND_CXX_COMPILE_OPTIONS}"
)

list(
	APPEND ADDITIONAL_COMPILE_OPTIONS_LIB
	$<$<CONFIG:Debug>:/ZI>
	$<$<CONFIG:Release>:/Z7>
	"${MSVC_C_AND_CXX_COMPILE_OPTIONS}"
)

list(
	APPEND ADDITIONAL_LINK_OPTIONS_EXE
	$<$<CONFIG:Debug>:${MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_DEBUG}>
	$<$<CONFIG:Release>:${MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_RELEASE}>
	$<$<CONFIG:Release>:/OPT:REF>
	$<$<CONFIG:Release>:/OPT:ICF>
	/MANIFEST:NO
)

list(
	APPEND ADDITIONAL_LINK_OPTIONS_DLL
	$<$<CONFIG:Debug>:${MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_DEBUG}>
	$<$<CONFIG:Release>:${MSVC_LINK_OPTION_IGNORE_DEFAULTLIBS_RELEASE}>
	$<$<CONFIG:Release>:/OPT:REF>
	$<$<CONFIG:Release>:/OPT:ICF>
	/SAFESEH:NO
	/MANIFEST:NO
)

list(
	APPEND ADDITIONAL_COMPILE_DEFINITIONS_EXE
	$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>
)

list(
	APPEND ADDITIONAL_COMPILE_DEFINITIONS_DLL
	_USRDLL
	$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>
)

list(
	APPEND ADDITIONAL_COMPILE_DEFINITIONS_LIB
	_LIB
	$<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>
)

list(
	APPEND ADDITIONAL_LINK_LIBRARIES_DLL
	legacy_stdio_definitions
)