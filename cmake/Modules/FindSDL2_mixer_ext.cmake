# Locate SDL_MIXER library
#
# This module defines:
#
# ::
#
#   SDLMixerX_LIBRARIES, the name of the library to link against
#   SDLMixerX_INCLUDE_DIRS, where to find the headers
#   SDLMixerX_FOUND, if false, do not try to link against
#   SDLMixerX_VERSION_STRING - human-readable string containing the version of SDL_MIXER
#
#
#
# $SDLDIR is an environment variable that would correspond to the
# ./configure --prefix=$SDLDIR used in building SDL.
#
# Created by Eric Wing.  This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(SDLMixerX_INCLUDE_DIR SDL_mixer.h
        HINTS
        ENV SDL2MixerXDIR
        ENV SDL2MIXERDIR
        ENV SDL2DIR
        # path suffixes to search inside ENV{SDLDIR}
        PATH_SUFFIXES include
        PATHS ${SDLMixerX_SOURCE_DIR}
        )

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(VC_LIB_PATH_SUFFIX lib/x64)
else()
    set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(SDLMixerX_LIBRARY
        NAMES SDL2MixerX
        HINTS
        ENV SDL2MixerXDIR
        ENV SDL2MIXERDIR
        ENV SDL2DIR
        PATH_SUFFIXES lib bin ${VC_LIB_PATH_SUFFIX}
        PATHS ${SDLMixerX_SOURCE_DIR}
        )

if(SDLMixerX_INCLUDE_DIR AND EXISTS "${SDLMixerX_INCLUDE_DIR}/SDL_mixer.h")
    file(STRINGS "${SDLMixerX_INCLUDE_DIR}/SDL_mixer.h" SDL2_MIXER_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+[0-9]+$")
    file(STRINGS "${SDLMixerX_INCLUDE_DIR}/SDL_mixer.h" SDL2_MIXER_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+[0-9]+$")
    file(STRINGS "${SDLMixerX_INCLUDE_DIR}/SDL_mixer.h" SDL2_MIXER_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+[0-9]+$")
    string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_MIXER_VERSION_MAJOR "${SDL2_MIXER_VERSION_MAJOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_MIXER_VERSION_MINOR "${SDL2_MIXER_VERSION_MINOR_LINE}")
    string(REGEX REPLACE "^#define[ \t]+SDL_MIXER_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_MIXER_VERSION_PATCH "${SDL2_MIXER_VERSION_PATCH_LINE}")
    set(SDL2_MIXER_VERSION_STRING ${SDL2_MIXER_VERSION_MAJOR}.${SDL2_MIXER_VERSION_MINOR}.${SDL2_MIXER_VERSION_PATCH})
    unset(SDL2_MIXER_VERSION_MAJOR_LINE)
    unset(SDL2_MIXER_VERSION_MINOR_LINE)
    unset(SDL2_MIXER_VERSION_PATCH_LINE)
    unset(SDL2_MIXER_VERSION_MAJOR)
    unset(SDL2_MIXER_VERSION_MINOR)
    unset(SDL2_MIXER_VERSION_PATCH)
endif()

set(SDLMixerX_LIBRARIES ${SDLMixerX_LIBRARY})
set(SDLMixerX_INCLUDE_DIRS ${SDLMixerX_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer_ext
        REQUIRED_VARS SDLMixerX_LIBRARIES SDLMixerX_INCLUDE_DIRS
        VERSION_VAR SDLMixerX_VERSION_STRING)


mark_as_advanced(SDLMixerX_LIBRARY SDLMixerX_INCLUDE_DIR)