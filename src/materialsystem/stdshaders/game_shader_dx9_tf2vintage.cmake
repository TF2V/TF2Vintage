# game_shader_dx9_tf2vintage.cmake

include( "${CMAKE_CURRENT_LIST_DIR}/game_shader_dx9_base.cmake")

set(
	GAME_SHADER_DX9_TF2VINTAGE_SOURCE_FILES
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_dx9.cpp"

	# Header Files
	"${GAME_SHADER_DX9_BASE_DIR}/common_vertexlitgeneric_dx9.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_lightmappedgeneric_fxc.h"
	"${GAME_SHADER_DX9_BASE_DIR}/common_flashlight_fxc.h"
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_common_ps2_3_x.h"
	
	# FXC Sources
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_ps30.fxc"
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_vs30.fxc"
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_ps20b.fxc"
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_vs20b.fxc"

	# Miscellaneous
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_dx9_30.txt"
	"${GAME_SHADER_DX9_BASE_DIR}/pbr_dx9_20b.txt"
)

add_library(game_shader_dx9_tf2vintage MODULE ${GAME_SHADER_DX9_TF2VINTAGE_SOURCE_FILES})

set_target_properties(
	game_shader_dx9_tf2vintage PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY "${GAMEDIR}/tf2vintage/bin"
)

target_use_game_shader_dx9_base(game_shader_dx9_tf2vintage)