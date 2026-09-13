if(TARGET ZLIB::ZLIB)
    return()
endif()

message(STATUS "Third-party: creating target 'ZLIB::ZLIB'")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

if (zlib_USE_THIRDPARTY)
	FetchContent_Declare(
		ZLIB
		GITHUB_REPOSITORY "madler/zlib"
		GIT_TAG v1.3.1
		OVERRIDE_FIND_PACKAGE
		OPTIONS
			"ZLIB_BUILD_EXAMPLES OFF"
	)
else()
	FetchContent_Declare(
		ZLIB
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/zlib/"
		OVERRIDE_FIND_PACKAGE
		OPTIONS
			"ZLIB_BUILD_EXAMPLES OFF"
	)
endif()

FetchContent_MakeAvailable(ZLIB)


add_library(ZLIB::ZLIB ALIAS zlibstatic)

set(ZLIB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/zlib" "${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
