# protobuf_include.cmake

target_link_libraries(
	${target} PRIVATE

	$<${IS_WINDOWS}:${LIBPUBLIC}/2015/libprotobuf.lib>
	$<${IS_LINUX}:${LIBPUBLIC}/linux32/libprotobuf.a>
)