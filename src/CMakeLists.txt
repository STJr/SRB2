add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32)

# Core sources
target_sourcefile(c)
target_sources(SRB2SDL2 PRIVATE comptime.c md5.c config.h.in)

set(SRB2_ASM_SOURCES vid_copy.s)

set(SRB2_NASM_SOURCES tmap_mmx.nas tmap.nas)

### Configuration
set(SRB2_CONFIG_HAVE_PNG ON CACHE BOOL
	"Enable PNG support. Depends on zlib, so will be disabled if you don't enable that too.")
set(SRB2_CONFIG_HAVE_ZLIB ON CACHE BOOL
	"Enable zlib support.")
set(SRB2_CONFIG_HAVE_GME ON CACHE BOOL
	"Enable GME support.")
set(SRB2_CONFIG_HAVE_OPENMPT ON CACHE BOOL
	"Enable OpenMPT support.")
set(SRB2_CONFIG_HAVE_CURL ON CACHE BOOL
	"Enable curl support.")
set(SRB2_CONFIG_HAVE_THREADS ON CACHE BOOL
	"Enable multithreading support.")
if(${CMAKE_SYSTEM} MATCHES Windows)
	set(SRB2_CONFIG_HAVE_MIXERX ON CACHE BOOL
		"Enable SDL Mixer X support.")
else()
	set(SRB2_CONFIG_HAVE_MIXERX OFF)
endif()
set(SRB2_CONFIG_HWRENDER ON CACHE BOOL
	"Enable hardware rendering through OpenGL.")
set(SRB2_CONFIG_USEASM OFF CACHE BOOL
	"Enable NASM tmap implementation for software mode speedup.")
set(SRB2_CONFIG_YASM OFF CACHE BOOL
	"Use YASM in place of NASM.")
set(SRB2_CONFIG_STATIC_OPENGL OFF CACHE BOOL
	"Use statically linked OpenGL. NOT RECOMMENDED.")
set(SRB2_CONFIG_DEV_BUILD OFF CACHE BOOL
	"Compile a development build of SRB2.")

### use internal libraries?
if(${CMAKE_SYSTEM} MATCHES "Windows") ###set on Windows only
	set(SRB2_CONFIG_USE_INTERNAL_LIBRARIES OFF CACHE BOOL
	"Use SRB2's internal copies of required dependencies (SDL2, PNG, zlib, GME, OpenMPT).")
endif()

add_subdirectory(blua)

if(${SRB2_CONFIG_HAVE_GME})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(GME_FOUND ON)
		set(GME_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/gme/include)
        if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(GME_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/gme/win64 -lgme")
		else() # 32-bit
			set(GME_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/gme/win32 -lgme")
		endif()
	else()
		find_package(GME)
	endif()
	if(${GME_FOUND})
		set(SRB2_HAVE_GME ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_GME)
	else()
		message(WARNING "You have specified that GME is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_OPENMPT})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(OPENMPT_FOUND ON)
		set(OPENMPT_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/inc)
        if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(OPENMPT_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libopenmpt/lib/x86_64/mingw -lopenmpt")
		else() # 32-bit
			set(OPENMPT_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libopenmpt/lib/x86/mingw -lopenmpt")
		endif()
	else()
		find_package(OPENMPT)
	endif()
	if(${OPENMPT_FOUND})
		set(SRB2_HAVE_OPENMPT ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_OPENMPT)
	else()
		message(WARNING "You have specified that OpenMPT is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_MIXERX})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(MIXERX_FOUND ON)
		set(MIXERX_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDLMixerX/i686-w64-mingw32/include/SDL2)
        if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(MIXERX_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDLMixerX/x86_64-w64-mingw32/lib -lSDL2_mixer_ext")
		else() # 32-bit
			set(MIXERX_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDLMixerX/i686-w64-mingw32/lib -lSDL2_mixer_ext")
		endif()
	else()
		# No support for non-Windows (yet?)
		#find_package(MIXERX)
		message(WARNING "SDL Mixer X is not supported as an external library.")
		set(MIXERX_FOUND OFF)
	endif()
	if(${MIXERX_FOUND})
		set(SRB2_HAVE_MIXERX ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXERX)
	else()
		message(WARNING "You have specified that SDL Mixer X is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_ZLIB})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(ZLIB_FOUND ON)
		set(ZLIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/zlib)
		if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(ZLIB_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/zlib/win32 -lz64")
		else() # 32-bit
			set(ZLIB_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/zlib/win32 -lz32")
		endif()
	else()
		find_package(ZLIB)
	endif()
	if(${ZLIB_FOUND})
		set(SRB2_HAVE_ZLIB ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_ZLIB)
	else()
		message(WARNING "You have specified that ZLIB is available but it was not found. SRB2 may not compile correctly.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_PNG} AND ${SRB2_CONFIG_HAVE_ZLIB})
	if (${ZLIB_FOUND})
		if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
			set(PNG_FOUND ON)
			set(PNG_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/libpng-src)
			if(${SRB2_SYSTEM_BITS} EQUAL 64)
				set(PNG_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libpng-src/projects -lpng64")
			else() # 32-bit
				set(PNG_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/libpng-src/projects -lpng32")
			endif()
		else()
			find_package(PNG)
		endif()
		if(${PNG_FOUND})
			set(SRB2_HAVE_PNG ON)
			target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_PNG)
			target_compile_definitions(SRB2SDL2 PRIVATE -D_LARGEFILE64_SOURCE)
			target_sources(SRB2SDL2 PRIVATE apng.c)
		else()
			message(WARNING "You have specified that PNG is available but it was not found. SRB2 may not compile correctly.")
		endif()
	endif()
endif()

if(${SRB2_CONFIG_HAVE_CURL})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(CURL_FOUND ON)
		set(CURL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/curl/include)
		if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(CURL_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/curl/lib64 -lcurl")
		else() # 32-bit
			set(CURL_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/curl/lib32 -lcurl")
		endif()
	else()
		find_package(CURL)
	endif()
	if(${CURL_FOUND})
		set(SRB2_HAVE_CURL ON)
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_CURL)
	else()
		message(WARNING "You have specified that CURL is available but it was not found. SRB2 may not compile correctly.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_THREADS})
	set(SRB2_HAVE_THREADS ON)
	target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_THREADS)
endif()

if(${SRB2_CONFIG_HWRENDER})
	target_compile_definitions(SRB2SDL2 PRIVATE -DHWRENDER)
	add_subdirectory(hardware)
endif()

if(${SRB2_CONFIG_HWRENDER} AND ${SRB2_CONFIG_STATIC_OPENGL})
	find_package(OpenGL)
	if(${OPENGL_FOUND})
		target_compile_definitions(SRB2SDL2 PRIVATE -DHWRENDER)
		target_compile_definitions(SRB2SDL2 PRIVATE -DSTATIC_OPENGL)
	else()
		message(WARNING "You have specified static opengl but opengl was not found. Not setting HWRENDER.")
	endif()
endif()

if(${SRB2_CONFIG_USEASM})
	#SRB2_ASM_FLAGS can be used to pass flags to either nasm or yasm.
	if(${CMAKE_SYSTEM} MATCHES "Linux")
		set(SRB2_ASM_FLAGS "-DLINUX ${SRB2_ASM_FLAGS}")
	endif()

	if(${SRB2_CONFIG_YASM})
		set(CMAKE_ASM_YASM_SOURCE_FILE_EXTENSIONS ${CMAKE_ASM_YASM_SOURCE_FILE_EXTENSIONS} nas)
		set(CMAKE_ASM_YASM_FLAGS "${SRB2_ASM_FLAGS}" CACHE STRING "Flags used by the assembler during all build types.")
		enable_language(ASM_YASM)
	else()
		set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS ${CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS} nas)
		set(CMAKE_ASM_NASM_FLAGS "${SRB2_ASM_FLAGS}" CACHE STRING "Flags used by the assembler during all build types.")
		enable_language(ASM_NASM)
	endif()

	set(SRB2_USEASM ON)
	target_compile_definitions(SRB2SDL2 PRIVATE -DUSEASM)
	target_compile_options(SRB2SDL2 PRIVATE "-msse3" "-mfpmath=sse")

	target_sources(SRB2SDL2 PRIVATE ${SRB2_ASM_SOURCES}
		${SRB2_NASM_SOURCES})
else()
	set(SRB2_USEASM OFF)
	target_compile_definitions(SRB2SDL2 PRIVATE -DNONX86 -DNORUSEASM)
endif()

# Targets

# If using CCACHE, then force it.
# https://github.com/Cockatrice/Cockatrice/pull/3052/files
if (${CMAKE_SYSTEM} MATCHES "Darwin")
	get_property(RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
	if(RULE_LAUNCH_COMPILE)
		MESSAGE(STATUS "Force enabling CCache usage under macOS")
		# Set up wrapper scripts
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/launch-c.in   ${CMAKE_BINARY_DIR}/launch-c)
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/launch-cxx.in ${CMAKE_BINARY_DIR}/launch-cxx)
		execute_process(COMMAND chmod a+rx
			"${CMAKE_BINARY_DIR}/launch-c"
			"${CMAKE_BINARY_DIR}/launch-cxx")

		# Set Xcode project attributes to route compilation through our scripts
		set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c")
		set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx")
		set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c")
		set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx")
	endif()
endif()

# Compatibility flag with later versions of GCC
# We should really fix our code to not need this
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(SRB2SDL2 PRIVATE "-mno-ms-bitfields" "-Wno-trigraphs")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(SRB2SDL2 PRIVATE "-Wno-absolute-value")
endif()

if(${SRB2_CONFIG_DEV_BUILD})
	target_compile_definitions(SRB2SDL2 PRIVATE -DDEVELOP)
endif()

target_compile_definitions(SRB2SDL2 PRIVATE -DCMAKECONFIG)

add_subdirectory(sdl)

if(NOT ${SRB2_SDL2_AVAILABLE})
	message(FATAL_ERROR "There are no targets available to build an SRB2 executable. :(")
endif()
