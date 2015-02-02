# Find SDL2
# Once done, this will define
# 
#  SDL2_MIXER_FOUND - system has SDL2
#  SDL2_MIXER_INCLUDE_DIRS - SDL2 include directories
#  SDL2_MIXER_LIBRARIES - link libraries

include(LibFindMacros)

libfind_pkg_check_modules(SDL2_MIXER_PKGCONF SDL2_mixer)

# includes
find_path(SDL2_MIXER_INCLUDE_DIR
	NAMES SDL_mixer.h
	PATHS
		${SDL2_MIXER_PKGCONF_INCLUDE_DIRS}
		"/usr/include/SDL2"
		"/usr/local/include/SDL2"
)

# library
find_library(SDL2_MIXER_LIBRARY
	NAMES SDL2_mixer
	PATHS
		${SDL2_MIXER_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)


# set include dir variables
set(SDL2_MIXER_PROCESS_INCLUDES SDL2_MIXER_INCLUDE_DIR)
set(SDL2_MIXER_PROCESS_LIBS SDL2_MIXER_LIBRARY)
libfind_process(SDL2_MIXER)
