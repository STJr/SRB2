if(TARGET SDL2-static)
    return()
endif()

message(STATUS "Third-party: creating target 'SDL2::SDL2'")

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)

set(
	internal_sdl2_options

		"SDL_STATIC ON"
		"SDL_SHARED OFF"
		"SDL_TEST OFF"
		"SDL2_DISABLE_INSTALL OFF"
)

if(${CMAKE_SYSTEM} MATCHES Windows)
	list(APPEND internal_sdl2_options "SDL2_DISABLE_SDL2MAIN OFF")
	option(SDL2_DISABLE_SDL2MAIN   "Disable building/installation of SDL2main" OFF)
	set(SDL2_DISABLE_SDL2MAIN OFF CACHE BOOL "" FORCE)
endif()
if(${CMAKE_SYSTEM} MATCHES Darwin)
	list(APPEND internal_sdl2_options "SDL2_DISABLE_SDL2MAIN OFF")
	option(SDL2_DISABLE_SDL2MAIN   "Disable building/installation of SDL2main" OFF)
	set(SDL2_DISABLE_SDL2MAIN OFF CACHE BOOL "" FORCE)
endif()
if(${CMAKE_SYSTEM} MATCHES Linux)
	list(APPEND internal_sdl2_options "SDL2_DISABLE_SDL2MAIN ON")
	option(SDL2_DISABLE_SDL2MAIN   "Disable building/installation of SDL2main" ON)
	set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

if (SDL2_USE_THIRDPARTY)
	FetchContent_Declare(
		SDL2
		VERSION 2.30.0
		GITHUB_REPOSITORY "libsdl-org/SDL"
		GIT_TAG release-2.30.0
		OPTIONS ${internal_sdl2_options}
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(
		SDL2
		SOURCE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/SDL2/"
		OPTIONS ${internal_sdl2_options}
		OVERRIDE_FIND_PACKAGE
	)
endif()

FetchContent_MakeAvailable(SDL2)

set(SDL2_INCLUDE_DIR "${SDL2_BINARY_DIR}/include" CACHE PATH "" FORCE)
set(SDL2_LIBRARY "${SDL2_BINARY_DIR}/SDL2-staticd.lib" CACHE PATH "" FORCE)
set(SDL2_DIR ${SDL2_BINARY_DIR} CACHE PATH "" FORCE)
