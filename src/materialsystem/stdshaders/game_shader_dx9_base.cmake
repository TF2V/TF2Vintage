# game_shader_dx9_base.cmake

include_guard(GLOBAL)

set(GAME_SHADER_DX9_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(
	GAME_SHADER_DX9_BASE_SOURCE_FILES
	"${GAME_SHADER_DX9_BASE_DIR}/BaseVSShader.cpp"

	"${GAME_SHADER_DX9_BASE_DIR}/example_model_dx9.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/example_model_dx9_helper.cpp"

	"${GAME_SHADER_DX9_BASE_DIR}/Bloom.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/screenspace_general.cpp"
		
	"${GAME_SHADER_DX9_BASE_DIR}/cloak_blended_pass_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/emissive_scroll_blended_pass_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/flesh_interior_blended_pass_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/lightmappedgeneric_dx9.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/lightmappedgeneric_dx9_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/depthwrite.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/refract.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/light_volumetrics.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/refract_dx9_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/shadow.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/shadowbuild_dx9.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/shadowmodel_dx9.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/skin_dx9_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/splinerope.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/teeth.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/vertexlitgeneric_dx9.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/vertexlitgeneric_dx9_helper.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/water.cpp"
	"${GAME_SHADER_DX9_BASE_DIR}/worldvertextransition.cpp"

	# Header Files
	"${GAME_SHADER_DX9_BASE_DIR}/BaseVSShader.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_fxc.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_hlsl_cpp_consts.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_ps_fxc.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_vertexlitgeneric_dx9.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_vs_fxc.h"
	"${GAME_SHADER_DX9_BASE_DIR}/shader_constant_register_map.h"

	"${GAME_SHADER_DX9_BASE_DIR}/example_model_dx9_helper.h"

	# Miscellaneous
	"${GAME_SHADER_DX9_BASE_DIR}/buildsdkshaders.bat"
	"${GAME_SHADER_DX9_BASE_DIR}/buildshaders.bat"

	"${GAME_SHADER_DX9_BASE_DIR}/sdk_shaders.txt"
	"${GAME_SHADER_DX9_BASE_DIR}/shaders.txt"
	"${GAME_SHADER_DX9_BASE_DIR}/light_volumetrics_shaders.txt"
)

function(target_use_game_shader_dx9_base target)
	target_sources(
		${target} PRIVATE
		${GAME_SHADER_DX9_BASE_SOURCE_FILES}
	)

	target_include_directories(
		${target} PRIVATE
		"${GAME_SHADER_DX9_BASE_DIR}/include"
	)

	target_compile_definitions(
		${target} PRIVATE
		STDSHADER_DX9_DLL_EXPORT
		FAST_MATERIALVAR_ACCESS
		GAME_SHADER_DLL
		$<$<NOT:${USE_GL}>:USE_ACTUAL_DX>
	)

	target_link_libraries(
		${target} PRIVATE
		"$<${IS_WINDOWS}:version;winmm>"
		mathlib
		"${LIBPUBLIC}/shaderlib${STATIC_LIB_EXT}"
	)
endfunction()