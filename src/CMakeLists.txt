add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32)

if("${CMAKE_COMPILER_IS_GNUCC}" AND "${CMAKE_SYSTEM_NAME}" MATCHES "Windows" AND NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	# On MinGW with internal libraries, link the standard library statically
	target_link_options(SRB2SDL2 PRIVATE "-static")
endif()

# Core sources
target_sourcefile(c)
target_sources(SRB2SDL2 PRIVATE comptime.c md5.c config.h.in)

set(SRB2_ASM_SOURCES vid_copy.s)

set(SRB2_NASM_SOURCES tmap_mmx.nas tmap.nas)

### Configuration
set(SRB2_CONFIG_USEASM OFF CACHE BOOL
	"Enable NASM tmap implementation for software mode speedup.")
set(SRB2_CONFIG_YASM OFF CACHE BOOL
	"Use YASM in place of NASM.")
set(SRB2_CONFIG_DEV_BUILD OFF CACHE BOOL
	"Compile a development build of SRB2.")

add_subdirectory(blua)

# OS macros
if (UNIX)
	target_compile_definitions(SRB2SDL2 PRIVATE -DUNIXCOMMON)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
	find_program(OBJCOPY objcopy)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	target_compile_definitions(SRB2SDL2 PRIVATE -DLINUX)
	if(${SRB2_SYSTEM_BITS} EQUAL 64)
		target_compile_definitions(SRB2SDL2 PRIVATE -DLINUX64)
	endif()
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
	target_compile_definitions(SRB2SDL2 PRIVATE -DMACOSX)
endif()

target_link_libraries(SRB2SDL2 PRIVATE gme)
target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_GME)
if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	# this sucks but gme doesn't use modern cmake to delineate public headers
	target_include_directories(SRB2SDL2 PRIVATE "${libgme_SOURCE_DIR}")
endif()

target_link_libraries(SRB2SDL2 PRIVATE openmpt)
target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_OPENMPT)

target_link_libraries(SRB2SDL2 PRIVATE ZLIB::ZLIB PNG::PNG CURL::libcurl)
target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_ZLIB -DHAVE_PNG -DHAVE_CURL -D_LARGEFILE64_SOURCE)
target_sources(SRB2SDL2 PRIVATE apng.c)

set(SRB2_HAVE_THREADS ON)
target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_THREADS)

if("${SRB2_CONFIG_HWRENDER}")
	target_compile_definitions(SRB2SDL2 PRIVATE -DHWRENDER)
	add_subdirectory(hardware)

	if("${SRB2_CONFIG_STATIC_OPENGL}")
		find_package(OpenGL)
		if(${OPENGL_FOUND})
			target_compile_definitions(SRB2SDL2 PRIVATE -DSTATIC_OPENGL)
		else()
			message(WARNING "You have specified static opengl but opengl was not found. Not setting HWRENDER.")
		endif()
	endif()
endif()

if(${SRB2_CONFIG_USEASM})
	#SRB2_ASM_FLAGS can be used to pass flags to either nasm or yasm.
	if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
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
	target_compile_options(SRB2SDL2 PRIVATE -msse3 -mfpmath=sse)

	target_sources(SRB2SDL2 PRIVATE ${SRB2_ASM_SOURCES}
		${SRB2_NASM_SOURCES})
else()
	set(SRB2_USEASM OFF)
	target_compile_definitions(SRB2SDL2 PRIVATE -DNONX86 -DNORUSEASM)
endif()

# Targets

# If using CCACHE, then force it.
# https://github.com/Cockatrice/Cockatrice/pull/3052/files
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
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
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|x64|amd64|AMD64|em64t|EM64T)")
		target_compile_options(SRB2SDL2 PRIVATE -mno-ms-bitfields)
	endif()
	target_compile_options(SRB2SDL2 PRIVATE -Wno-trigraphs)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(SRB2SDL2 PRIVATE -Wno-absolute-value)
endif()

if(${SRB2_CONFIG_DEV_BUILD})
	target_compile_definitions(SRB2SDL2 PRIVATE -DDEVELOP)
endif()
target_compile_definitions(SRB2SDL2 PRIVATE -DCMAKECONFIG)

add_subdirectory(sdl)

# strip debug symbols into separate file when using gcc.
# to be consistent with Makefile, don't generate for OS X.
if((CMAKE_COMPILER_IS_GNUCC) AND NOT ("${CMAKE_SYSTEM_NAME}" MATCHES Darwin))
	if((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
		if(${CMAKE_BUILD_TYPE} MATCHES Debug)
			set(OBJCOPY_ONLY_KEEP_DEBUG "--only-keep-debug")
		endif()
		message(STATUS "Will make separate debug symbols in *.debug")
		add_custom_command(TARGET SRB2SDL2 POST_BUILD
			COMMAND ${OBJCOPY} ${OBJCOPY_ONLY_KEEP_DEBUG} $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
			COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
			COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
		)
	endif()
endif()
