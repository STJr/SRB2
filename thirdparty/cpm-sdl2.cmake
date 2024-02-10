CPMAddPackage(
	NAME SDL2
	GITHUB_REPOSITORY libsdl-org/SDL
	VERSION 2.30.0
	GIT_TAG release-2.30.0
	OPTIONS
		"BUILD_SHARED_LIBS OFF"
		"SDL_SHARED OFF"
		"SDL_STATIC ON"
		"SDL_TEST OFF"
		"SDL2_DISABLE_SDL2MAIN ON"
		"SDL2_DISABLE_INSTALL ON"
		"SDL_STATIC_PIC ON"
		"SDL_WERROR OFF"
		"TEST_STATIC OFF"
)
find_package(SDL2 REQUIRED)

#if(SDL2_ADDED)
	file(GLOB SDL2_HEADERS "${SDL2_SOURCE_DIR}/include/*.h")

	# Create a target that copies headers at build time, when they change
	add_custom_target(sdl_copy_headers_in_build_dir
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${SDL2_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/SDLHeaders/SDL2"
			DEPENDS ${SDL2_HEADERS})

	# Make SDL depend from it
	add_dependencies(SDL2-static sdl_copy_headers_in_build_dir)

	# And add the directory where headers have been copied as an interface include dir
	target_include_directories(SDL2-static INTERFACE "${CMAKE_BINARY_DIR}/SDLHeaders")

	set (SDL2_INCLUDE_DIR ${SDL2_SOURCE_DIR}/include)

include_directories(${SDL2_INCLUDE_DIR})
#endif()
