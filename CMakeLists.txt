cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

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

option(
	SRB2_CONFIG_SYSTEM_LIBRARIES
	"Link dependencies using CMake's find_package and do not use internal builds"
	${SRB2_CONFIG_SYSTEM_LIBRARIES_DEFAULT}
)
# This option isn't recommended for distribution builds and probably won't work (yet).
cmake_dependent_option(
	SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES
	"Use dynamic libraries when compiling internal dependencies"
	OFF "NOT SRB2_CONFIG_SYSTEM_LIBRARIES"
	OFF
)
option(SRB2_CONFIG_HWRENDER "Enable hardware render (OpenGL) support" ON)
option(SRB2_CONFIG_STATIC_OPENGL "Enable static linking GL (do not do this)" OFF)

# Enable CCache early
set(SRB2_USE_CCACHE OFF CACHE BOOL "Use CCache")
if (${SRB2_USE_CCACHE})
	find_program(CCACHE_PROGRAM ccache)
	if(CCACHE_PROGRAM)
		message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
		set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
	else()
		message(WARNING "You have specified to use CCACHE but it was not found. Object files will not be cached.")
	endif()
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
	find_package(GME REQUIRED)
endif()

if(${PROJECT_SOURCE_DIR} MATCHES ${PROJECT_BINARY_DIR})
	message(FATAL_ERROR "In-source builds will bring you a world of pain. Please make a separate directory to invoke CMake from.")
endif()

if ((${SRB2_USE_CCACHE}) AND (${CMAKE_C_COMPILER} MATCHES "clang"))
	message(WARNING "Using clang and CCache: You may want to set environment variable CCACHE_CPP2=yes to prevent include errors during compile.")
endif()

# Add sources from Sourcefile
function(target_sourcefile type)
	file(STRINGS Sourcefile list
		REGEX "[-0-9A-Za-z_]+\.${type}")
	target_sources(SRB2SDL2 PRIVATE ${list})
endfunction()

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
set(SRB2_SDL2_EXE_NAME srb2 CACHE STRING "Executable binary output name")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_subdirectory(src)
add_subdirectory(assets)


## config.h generation
set(GIT_EXECUTABLE "git" CACHE FILEPATH "Path to git binary")
include(GitUtilities)
git_latest_commit(SRB2_COMP_COMMIT "${CMAKE_SOURCE_DIR}")
git_current_branch(SRB2_GIT_BRANCH "${CMAKE_SOURCE_DIR}")
set(SRB2_COMP_BRANCH "${SRB2_GIT_BRANCH}")
set(SRB2_COMP_REVISION "${SRB2_COMP_COMMIT}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
