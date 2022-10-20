# protobuf_builder.cmake

#This is a target added in /thirdparty/protobuf-2.3.0
if( IS_LINUX )
    set(PROTO_COMPILER "${SRCDIR}/devtools/bin/linux/protoc")
elseif( IS_WINDOWS )
    set(PROTO_COMPILER "${SRCDIR}/gcsdk/bin/protoc.exe")
elseif( OSXALL )
    set(PROTO_COMPILER "${SRCDIR}/gcsdk/bin/osx32/protoc")
else()
    message(FATAL_ERROR "Platform Unknown!")
endif()

#Built a .proto file and add the resulting C++ to the target.
macro( TargetBuildAndAddProto TARGET_NAME PROTO_FILE PROTO_OUTPUT_FOLDER )
    set(PROTO_FILENAME)
	
	target_include_directories(${TARGET_NAME} PRIVATE ${GENERATED_PROTO_DIR})
	target_include_directories(${TARGET_NAME} PRIVATE "${SRCDIR}/thirdparty/protobuf-2.3.0/src")

	target_compile_definitions(${TARGET_NAME} PRIVATE "PROTOBUF")	

    get_filename_component(PROTO_FILENAME ${PROTO_FILE} NAME_WLE) #name without any extensions

    add_custom_command(
            OUTPUT "${PROTO_OUTPUT_FOLDER}/${PROTO_FILENAME}.pb.cc"
                   "${PROTO_OUTPUT_FOLDER}/${PROTO_FILENAME}.pb.h"
            COMMAND ${PROTO_COMPILER}
            ARGS --proto_path=${SRCDIR}/game/shared/econ --proto_path=${SRCDIR}/thirdparty/protobuf-2.3.0/src --proto_path=${SRCDIR}/gcsdk --cpp_out=. ${PROTO_FILE}
            DEPENDS ${PROTO_FILE} ${PROTO_COMPILER}
            WORKING_DIRECTORY ${PROTO_OUTPUT_FOLDER}
            COMMENT "Running protoc compiler on ${PROTO_FILE} - output (${PROTO_OUTPUT_FOLDER}/${PROTO_FILENAME}.pb.cc)"
            VERBATIM
    )

    #add the output folder in the include path.
    target_include_directories(${TARGET_NAME} PRIVATE ${PROTO_OUTPUT_FOLDER})
    target_sources(${TARGET_NAME} PRIVATE ${PROTO_OUTPUT_FOLDER}/${PROTO_FILENAME}.pb.cc)
endmacro()