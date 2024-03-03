cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
	message(FATAL_ERROR "In-source builds are blocked. Please build from a separate directory.")
endif()

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(CMakeDependentOption)
include(cmake/CPM.cmake)

file(STRINGS src/version.h SRB2_VERSION)
string(REGEX MATCH "[0-9]+\\.[0-9.]+" SRB2_VERSION ${SRB2_VERSION})

# DO NOT CHANGE THIS SRB2 STRING! Some variable names depend on this string.
# Version change is fine.
project(SRB2
	VERSION ${SRB2_VERSION}
	LANGUAGES C CXX)

##### PACKAGE CONFIGURATION #####

set(SRB2_CPACK_GENERATOR "" CACHE STRING "Generator to use for making a package. E.g., ZIP, TGZ, DragNDrop (OSX only). Leave blank for default generator.")

if("${SRB2_CPACK_GENERATOR}" STREQUAL "")
	if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
		set(SRB2_CPACK_GENERATOR "ZIP")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		set(SRB2_CPACK_GENERATOR "TGZ")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
		set(SRB2_CPACK_GENERATOR "TGZ")
	endif()
endif()

set(CPACK_GENERATOR ${SRB2_CPACK_GENERATOR})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sonic Robo Blast 2" CACHE STRING "Program name for display purposes")
set(CPACK_PACKAGE_VENDOR "Sonic Team Jr." CACHE STRING "Vendor name for display purposes")
#set(CPACK_PACKAGE_DESCRIPTION_FILE )
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR ${SRB2_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${SRB2_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${SRB2_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}")
SET(CPACK_OUTPUT_FILE_PREFIX package)
include(CPack)

# Options

if("${CMAKE_SYSTEM_NAME}" MATCHES Linux)
	set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT ON)
else()
	set(SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT OFF)
endif()

# Clang tidy options will be ignored if CMAKE_<LANG>_CLANG_TIDY are set.
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_C "Enable default clang-tidy check configuration for C" OFF)
option(SRB2_CONFIG_ENABLE_CLANG_TIDY_CXX "Enable default clang-tidy check configuration for C++" OFF)
option(
	SRB2_CONFIG_SYSTEM_LIBRARIES
	"Link dependencies using CMake's find_package and do not use internal builds"
	${SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT}
)
option(SRB2_CONFIG_ENABLE_TESTS "Build the test suite" ON)
# This option isn't recommended for distribution builds and probably won't work (yet).
cmake_dependent_option(
	SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES
	"Use dynamic libraries when compiling internal dependencies"
	OFF "NOT SRB2_CONFIG_SYSTEM_LIBRARIES"
	OFF
)
option(SRB2_CONFIG_HWRENDER "Enable hardware render (OpenGL) support" ON)
option(SRB2_CONFIG_USE_GME "Enable GME playback support" OFF)
option(SRB2_CONFIG_STATIC_OPENGL "Enable static linking GL (do not do this)" OFF)
option(SRB2_CONFIG_ERRORMODE "Compile C code with warnings treated as errors." OFF)
option(SRB2_CONFIG_DEBUGMODE "Compile with PARANOIA, ZDEBUG, RANGECHECK and PACKETDROP defined." OFF)
option(SRB2_CONFIG_MOBJCONSISTANCY "Compile with MOBJCONSISTANCY defined." OFF)
option(SRB2_CONFIG_PACKETDROP "Compile with PACKETDROP defined." OFF)
option(SRB2_CONFIG_EXECINFO "Enable stack trace dump support." ON)
option(SRB2_CONFIG_ZDEBUG "Compile with ZDEBUG defined." OFF)
# SRB2_CONFIG_PROFILEMODE is probably superceded by some CMake setting.
option(SRB2_CONFIG_PROFILEMODE "Compile for profiling (GCC only)." OFF)
set(SRB2_CONFIG_ASSET_DIRECTORY "" CACHE PATH "Path to directory that contains all asset files for the installer. If set, assets will be part of installation and cpack.")

if(SRB2_CONFIG_ENABLE_TESTS)
	# https://github.com/catchorg/Catch2
	CPMAddPackage(
		NAME Catch2
		VERSION 3.4.0
		GITHUB_REPOSITORY catchorg/Catch2
		OPTIONS
			"CATCH_INSTALL_DOCS OFF"
	)
	list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
	include(CTest)
	include(Catch)
	add_executable(srb2tests)
	# To add tests, use target_sources to add individual test files to the target in subdirs.
	target_link_libraries(srb2tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)
	target_compile_features(srb2tests PRIVATE c_std_11 cxx_std_17)
	catch_discover_tests(srb2tests)
endif()

# Enable CCache
# (Set USE_CCACHE=ON to use, CCACHE_OPTIONS for options)
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL Windows)
	option(USE_CCACHE "Enable ccache support" OFF)

	if(USE_CCACHE)
		find_program(CCACHE_TOOL_PATH ccache)
		if(CCACHE_TOOL_PATH)
			set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_TOOL_PATH} CACHE STRING "" FORCE)
			set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_TOOL_PATH} CACHE STRING "" FORCE)
		else()
			message(WARNING "USE_CCACHE was set but ccache is not found (set CCACHE_TOOL_PATH)")
		endif()
	endif()
else()
	CPMAddPackage(
		NAME Ccache.cmake
		GITHUB_REPOSITORY TheLartians/Ccache.cmake
		VERSION 1.2
	)
endif()

# Dependencies
add_subdirectory(thirdparty)

if("${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	find_package(ZLIB REQUIRED)
	find_package(PNG REQUIRED)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_mixer REQUIRED)
	find_package(CURL REQUIRED)
	find_package(OPENMPT REQUIRED)

	# libgme defaults to "Nuked" YM2612 emulator, which is
	# very SLOW. The system library probably uses the
	# default so just always build it.
	#find_package(GME REQUIRED)
endif()

if(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
	message(FATAL_ERROR "In-source builds will bring you a world of pain. Please make a separate directory to invoke CMake from.")
endif()

if ((${SRB2_USE_CCACHE}) AND (${CMAKE_C_COMPILER} MATCHES "clang"))
	message(WARNING "Using clang and CCache: You may want to set environment variable CCACHE_CPP2=yes to prevent include errors during compile.")
endif()

# bitness check
set(SRB2_SYSTEM_BITS 0)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Target is 64-bit")
	set(SRB2_SYSTEM_BITS 64)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(STATUS "Target is 32-bit")
	set(SRB2_SYSTEM_BITS 32)
endif()
if(${SRB2_SYSTEM_BITS} EQUAL 0)
	message(STATUS "Target bitness is unknown")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Set EXE names so the assets CMakeLists can refer to its target
set(SRB2_SDL2_EXE_NAME "" CACHE STRING "Override executable binary output name")
set(SRB2_SDL2_EXE_SUFFIX "" CACHE STRING "Optional executable suffix, separated by an underscore")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(assets)


set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")
include(GitUtilities)

if("${SRB2_SDL2_EXE_NAME}" STREQUAL "")
	# cause a reconfigure if the branch changes
	get_git_dir(SRB2_GIT_DIR)
	configure_file("${SRB2_GIT_DIR}/HEAD" HEAD COPYONLY)

	git_current_branch(SRB2_GIT_REVISION)

	if("${SRB2_GIT_REVISION}" STREQUAL "")
		# use abbreviated commit hash if on detached HEAD
		git_latest_commit(SRB2_GIT_REVISION)
	endif()

	if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
		list(APPEND EXE_NAME_PARTS "srb2win")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		list(APPEND EXE_NAME_PARTS "lsdlsrb2")
	else()
		list(APPEND EXE_NAME_PARTS "srb2")
	endif()

	if(NOT "${SRB2_GIT_REVISION}" STREQUAL "master")
		list(APPEND EXE_NAME_PARTS ${SRB2_GIT_REVISION})
	endif()
else()
	list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_NAME})
endif()

list(APPEND EXE_NAME_PARTS ${SRB2_SDL2_EXE_SUFFIX})

list(JOIN EXE_NAME_PARTS "_" EXE_NAME)
set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME ${EXE_NAME})
