CPMAddPackage(
	NAME ZLIB
	VERSION 1.2.13
	URL "https://github.com/madler/zlib/archive/refs/tags/v1.2.13.zip"
	EXCLUDE_FROM_ALL
	DOWNLOAD_ONLY YES
)

if(ZLIB_ADDED)
	set(ZLIB_SRCS
		crc32.h
		deflate.h
		gzguts.h
		inffast.h
		inffixed.h
		inflate.h
		inftrees.h
		trees.h
		zutil.h
		adler32.c
		compress.c
		crc32.c
		deflate.c
		gzclose.c
		gzlib.c
		gzread.c
		gzwrite.c
		inflate.c
		infback.c
		inftrees.c
		inffast.c
		trees.c
		uncompr.c
		zutil.c
	)
	list(TRANSFORM ZLIB_SRCS PREPEND "${ZLIB_SOURCE_DIR}/")

	configure_file("${ZLIB_SOURCE_DIR}/zlib.pc.cmakein" "${ZLIB_BINARY_DIR}/zlib.pc" @ONLY)
	configure_file("${ZLIB_SOURCE_DIR}/zconf.h.cmakein" "${ZLIB_BINARY_DIR}/include/zconf.h" @ONLY)
	configure_file("${ZLIB_SOURCE_DIR}/zlib.h" "${ZLIB_BINARY_DIR}/include/zlib.h" @ONLY)

	add_library(ZLIB ${SRB2_INTERNAL_LIBRARY_TYPE} ${ZLIB_SRCS})
	set_target_properties(ZLIB PROPERTIES
		VERSION 1.2.13
		OUTPUT_NAME "z"
	)
	target_include_directories(ZLIB PRIVATE "${ZLIB_SOURCE_DIR}")
	target_include_directories(ZLIB PUBLIC "${ZLIB_BINARY_DIR}/include")
	if(MSVC)
		target_compile_definitions(ZLIB PRIVATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
	endif()
	add_library(ZLIB::ZLIB ALIAS ZLIB)
endif()
