cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set(CMAKE_BINARY_DIR "${BINARY_DIR}")
set(CMAKE_CURRENT_BINARY_DIR "${BINARY_DIR}")

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(GitUtilities)

git_current_branch(SRB2_COMP_BRANCH)
git_working_tree_dirty(SRB2_COMP_UNCOMMITTED)

git_latest_commit(SRB2_COMP_REVISION)
git_subject(subject)
string(REGEX REPLACE "([\"\\])" "\\\\\\1" SRB2_COMP_NOTE "${subject}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE None)
endif()

# These build types enable optimizations of some kind by default.
set(optimized_build_types "MINSIZEREL;RELEASE;RELWITHDEBINFO")

string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
if("${build_type}" IN_LIST optimized_build_types)
	set(SRB2_COMP_OPTIMIZED TRUE)
else()
	set(SRB2_COMP_OPTIMIZED FALSE)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/config.h")
