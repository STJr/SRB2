CPMAddPackage(
	NAME png
	VERSION 1.6.38
	URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.38.zip"
	# png cmake build is broken on msys/mingw32
	DOWNLOAD_ONLY YES
)

if(png_ADDED)
	# Since png's cmake build is broken, we're going to create a target manually
	set(
		PNG_SOURCES
		png.h
		pngconf.h
		pngpriv.h
		pngdebug.h
		pnginfo.h
		pngstruct.h
		png.c
		pngerror.c
		pngget.c
		pngmem.c
		pngpread.c
		pngread.c
		pngrio.c
		pngrtran.c
		pngrutil.c
		pngset.c
		pngtrans.c
		pngwio.c
		pngwrite.c
		pngwtran.c
		pngwutil.c
	)
	list(TRANSFORM PNG_SOURCES PREPEND "${png_SOURCE_DIR}/")

	add_custom_command(
		OUTPUT "${png_BINARY_DIR}/include/png.h" "${png_BINARY_DIR}/include/pngconf.h"
		COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h" "${png_BINARY_DIR}/include"
		DEPENDS "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h"
		VERBATIM
	)
	add_custom_command(
		OUTPUT "${png_BINARY_DIR}/include/pnglibconf.h"
		COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${png_BINARY_DIR}/include/pnglibconf.h"
		DEPENDS "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt"
		VERBATIM
	)
	list(
		APPEND PNG_SOURCES
		"${png_BINARY_DIR}/include/png.h"
		"${png_BINARY_DIR}/include/pngconf.h"
		"${png_BINARY_DIR}/include/pnglibconf.h"
	)
	add_library(png "${SRB2_INTERNAL_LIBRARY_TYPE}" ${PNG_SOURCES})

	# Disable ARM NEON since having it automatic breaks libpng external build on clang for some reason
	target_compile_definitions(png PRIVATE -DPNG_ARM_NEON_OPT=0)

	# The png includes need to be available to consumers
	target_include_directories(png PUBLIC "${png_BINARY_DIR}/include")

	# ... and these also need to be present only for png build
	target_include_directories(png PRIVATE "${ZLIB_SOURCE_DIR}")
	target_include_directories(png PRIVATE "${ZLIB_BINARY_DIR}")
	target_include_directories(png PRIVATE "${png_BINARY_DIR}")
	target_link_libraries(png PRIVATE ZLIB::ZLIB)
	add_library(PNG::PNG ALIAS png)
endif()
