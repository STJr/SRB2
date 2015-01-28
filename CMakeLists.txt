cmake_minimum_required(VERSION 3.0)
project(SRB2
	VERSION 2.1.14)

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

### Useful functions

# Prepend sources with current source directory
function(prepend_sources SOURCE_FILES)
	foreach(SOURCE_FILE ${${SOURCE_FILES}})
		set(MODIFIED ${MODIFIED} ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE})
	endforeach()
	set(${SOURCE_FILES} ${MODIFIED} PARENT_SCOPE)
endfunction()

# 64-bit check
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(STATUS "Target is 64-bit")
	set(SRB2_SYSTEM_BITS 64)
else()
	set(SRB2_SYSTEM_BITS 32)
endif()

# DO NOT ALLOW MSVC!
if(MSVC)
	message(FATAL_ERROR "MSVC is not supported!")
endif()

# OS macros
if (UNIX)
	add_definitions(-DUNIXCOMMON)
endif()
if(${CMAKE_SYSTEM} MATCHES "Linux")
	add_definitions(-DLINUX)
	if(${SRB2_SYSTEM_BITS} EQUAL 64)
		add_definitions(-DLINUX64)
	endif()
endif()
# TODO support MacOSX builds

add_subdirectory(src/)
