cmake_minimum_required (VERSION 2.8.11)
project(SDLMixerX C)

include(FindPkgConfig)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
#include(CheckCSourceRuns)
#include(CheckCCompilerFlag)

if(NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    set(CMAKE_MACOSX_RPATH 0)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("== Using default build configuration which is a Release!")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MIX_DEBUG_SUFFIX "d")
else()
    set(MIX_DEBUG_SUFFIX "")
endif()

if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

if(POLICY CMP0058)
    cmake_policy(SET CMP0058 NEW)
endif()

option(SSEMATH             "Allow GCC to use SSE floating point math" ${OPT_DEF_SSEMATH})
option(MMX                 "Use MMX assembly routines" ${OPT_DEF_ASM})
option(3DNOW               "Use 3Dnow! MMX assembly routines" ${OPT_DEF_ASM})
option(SSE                 "Use SSE assembly routines" ${OPT_DEF_ASM})
option(SSE2                "Use SSE2 assembly routines" ${OPT_DEF_SSEMATH})
option(SSE3                "Use SSE3 assembly routines" ${OPT_DEF_SSEMATH})

# Settings
option(SDL_MIXER_X_STATIC   "Build static library of SDL Mixer X" ON)
option(SDL_MIXER_X_SHARED   "Build shared library of SDL Mixer X" ON)

option(DOWNLOAD_AUDIO_CODECS_DEPENDENCY "Downloads and compiles AudioCodecs dependencies pack directly" OFF)
set(AUDIO_CODECS_REPO_PATH "" CACHE PATH "Path to the AudioCodecs dependencies pack")
set(AUDIO_CODECS_INSTALL_PATH "" CACHE PATH "Path to installed AudioCodecs prefix")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Turn on warnings and legacy C/C++ standards to support more compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c90")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++98")
    # Deny undefined symbols
    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
    endif()
endif()

set(SDL_MIXER_INCLUDE_PATHS)

if(NOT AUDIO_CODECS_REPO_PATH AND NOT AUDIO_CODECS_INSTALL_PATH)
    # Try to resolve sqlite dependency
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        # Download and configure AudioCodecs dependency
        set(SDL2_REPO_PATH ${CMAKE_INSTALL_PREFIX})
        set(SDL2_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include/SDL2)
        set(SDL2_LIBS_PATH ${CMAKE_INSTALL_PREFIX}/lib)
        include(cmake/DownloadAudioCodecs.cmake)
        set(AUDIO_CODECS_REPO_PATH ${AUDIO_CODECS_REPOSITORY_PATH})
        set(AUDIO_CODECS_INSTALL_PATH ${AUDIO_CODECS_INSTALL_DIR})
    endif()
endif()

set(FIND_PREFER_STATIC
    "-static${MIX_DEBUG_SUFFIX}.a"
    "-static${MIX_DEBUG_SUFFIX}.lib"
    "${MIX_DEBUG_SUFFIX}.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    "-static.a"
    "-static.lib"
    ".a"
    ".lib"
    "${MIX_DEBUG_SUFFIX}.dll.a"
    "${MIX_DEBUG_SUFFIX}.dll.lib"
    ".dll.a"
    ".dll.lib"
    "${MIX_DEBUG_SUFFIX}.so"
    "${MIX_DEBUG_SUFFIX}.dylib"
    ".so"
    ".dylib"
)

set(FIND_PREFER_SHARED
    "${MIX_DEBUG_SUFFIX}.dll.a"
    "${MIX_DEBUG_SUFFIX}.dll.lib"
    ".dll.a"
    ".dll.lib"
    "${MIX_DEBUG_SUFFIX}.so"
    "${MIX_DEBUG_SUFFIX}.dylib"
    ".so"
    ".dylib"
    "-static${MIX_DEBUG_SUFFIX}.a"
    "-static${MIX_DEBUG_SUFFIX}.lib"
    "${MIX_DEBUG_SUFFIX}.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    "-static.a"
    "-static.lib"
    ".a"
    ".lib"
)

if(AUDIO_CODECS_REPO_PATH OR AUDIO_CODECS_INSTALL_PATH)
    set(AUDIO_CODECS_PATH ${AUDIO_CODECS_REPO_PATH})
    set(AUDIO_CODECS_INSTALL_DIR ${AUDIO_CODECS_INSTALL_PATH})
    set(SDL2_REPO_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(SDL2_INCLUDE_PATH ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    link_directories(${AUDIO_CODECS_INSTALL_DIR}/lib)
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include ${SDL2_INCLUDE_PATH})
    message("AudioCodecs will be used. No needed extra libraries installed in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 0)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_STATIC})
else()
    message("AudioCodecs will NOT be used. Libraries will be recognized in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 1)
endif()

add_definitions(-DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF)
add_definitions(-Dmain=SDL_main -DPIC -D_REENTRANT -D_USE_MATH_DEFINES)

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

# Recognize SDL2 library from the system
if(USE_SYSTEM_AUDIO_LIBRARIES)
    find_package(PkgConfig)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${SDL2_INCLUDE_DIRS})
    list(APPEND SDLMixerX_LINK_LIBS ${SDL2_LIBRARIES})
    message("== SDL2: ${SDL2_INCLUDE_DIRS} ${SDL2_LIBRARIES} ==")
endif()

option(USE_WAV             "Build with WAV codec" ON)
if(USE_WAV)
    add_definitions(-DMUSIC_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_aiff.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_voc.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_wav.c)
endif()

option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(vorbisfile ov_open "vorbis/vorbisfile.h" FOUND_VORBIS)
        find_path(LIBVORBIS_INCLUDE_DIR vorbis/codec.h)
        find_library(LIBVORBISFILE_LIB NAMES vorbisfile)
        find_library(LIBVORBIS_LIB NAMES vorbis)
        message("Vorbis: ${LIBVORBIS_INCLUDE_DIR} ${LIBVORBIS_LIB}")
    else()
        set(FOUND_VORBIS 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBVORBISFILE_LIB vorbisfile)
            set(LIBVORBIS_LIB vorbis)
        else()
            find_library(LIBVORBISFILE_LIB NAMES vorbisfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBVORBIS_LIB NAMES vorbis
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_VORBIS)
        message("== using Vorbis ==")
        add_definitions(-DMUSIC_OGG)
        list(APPEND SDL_MIXER_INCLUDE_PATHS
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libvorbis/include
        )
        list(APPEND SDLMixerX_LINK_LIBS ${LIBVORBISFILE_LIB} ${LIBVORBIS_LIB})
        set(LIBOGG_NEEDED ON)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_ogg.c)
    endif()
endif()

option(USE_OPUS      "Build with OPUS codec" ON)
if(USE_OPUS)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(opusfile op_open_file "opus/opusfile.h" FOUND_OPUS)
        find_path(LIBOPUS_INCLUDE_DIR opus/opusfile.h)
        find_library(LIBOPUSFILE_LIB NAMES opusfile)
        find_library(LIBOPUS_LIB NAMES opusfile)
        message("Opus: ${LIBOPUS_INCLUDE_DIR} ${LIBOPUSFILE_LIB} ${LIBOPUS_LIB}")
    else()
        set(FOUND_OPUS 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBOPUSFILE_LIB opusfile)
            set(LIBOPUS_LIB opus)
        else()
            find_library(LIBOPUSFILE_LIB NAMES opusfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBOPUS_LIB NAMES opus
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_OPUS)
        message("== using Opus ==")
        add_definitions(-DMUSIC_OPUS)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS
                ${AUDIO_CODECS_PATH}/libogg/include
                ${AUDIO_CODECS_PATH}/libopus/include
                ${AUDIO_CODECS_PATH}/libopusfile/include
            )
        endif()
        if(AUDIO_CODECS_INSTALL_DIR)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include/opus)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBOPUSFILE_LIB} ${LIBOPUS_LIB})
        set(LIBOGG_NEEDED ON)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_opus.c)
    endif()
endif()

option(USE_FLAC            "Build with FLAC codec" ON)
if(USE_FLAC)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(FLAC FLAC__format_sample_rate_is_valid "FLAC/format.h" FOUND_FLAC)
        find_path(LIBFLAC_INCLUDE_DIR "FLAC/all.h")
        find_library(LIBFLAC_LIB NAMES FLAC)
        message("FLAC: ${LIBFLAC_INCLUDE_DIR} ${LIBFLAC_LIB}")
    else()
        set(FOUND_FLAC 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBFLAC_LIB FLAC)
        else()
            find_library(LIBFLAC_LIB NAMES FLAC
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_FLAC)
        message("== using FLAC ==")
        add_definitions(-DMUSIC_FLAC -DFLAC__NO_DLL)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS
                ${AUDIO_CODECS_PATH}/libogg/include
                ${AUDIO_CODECS_PATH}/libFLAC/include
            )
        endif()
        set(LIBOGG_NEEDED ON)
        list(APPEND SDLMixerX_LINK_LIBS ${LIBFLAC_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_flac.c)
    endif()
endif()

if(LIBOGG_NEEDED)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_library(LIBOGG_LIB NAMES ogg)
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBOGG_LIB ogg)
        else()
            find_library(LIBOGG_LIB NAMES ogg
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    list(APPEND SDLMixerX_LINK_LIBS ${LIBOGG_LIB})
endif()

option(USE_MP3_ID3TAG   "Build with MP3 Meta tags support provided by libID3Tag library" ON)
if(USE_MP3_ID3TAG AND NOT USE_SYSTEM_AUDIO_LIBRARIES)
    message("== using ID3Tag (custom) ==")
    add_definitions(-DMUSIC_MP3_ID3TAG)
    if(AUDIO_CODECS_REPO_PATH)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libid3tag/include)
    endif()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(LIBID3TAG_LIB id3tag)
    else()
        find_library(LIBID3TAG_LIB NAMES id3tag
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
    endif()
    list(APPEND SDLMixerX_LINK_LIBS ${LIBID3TAG_LIB})
endif()

option(USE_MP3_MAD      "Build with MAD MP3 codec" ON)
option(USE_MP3_MAD_GPL_DITHERING "Enable GPL-Licensed dithering functions for MAD library" ON)
if(USE_MP3_MAD)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(mad mad_frame_init "mad.h" FOUND_MAD)
        find_path(LIBMAD_INCLUDE_DIR "FLAC/all.h")
        find_library(LIBMAD_LIB NAMES mad)
        message("MAD: ${LIBMAD_INCLUDE_DIR} ${LIBMAD_LIB}")
    else()
        set(FOUND_MAD 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBMAD_LIB mad)
        else()
            find_library(LIBMAD_LIB NAMES mad
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_MAD)
        message("== using MAD ==")
        add_definitions(-DMUSIC_MP3_MAD)
        if(USE_MP3_MAD_GPL_DITHERING)
            add_definitions(-DMUSIC_MP3_MAD_GPL_DITHERING)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBMAD_LIB})
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libmad/include)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mad.c)
    endif()
endif()

option(USE_MP3_SMPEG       "Build with SMPEG MP3 codec" OFF)
if(USE_MP3_SMPEG)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(smpeg SMPEG_new "smpeg/smpeg.h" FOUND_SMPEG)
        find_path(LIBSMPEG_INCLUDE_DIR "smpeg/smpeg.h")
        find_library(LIBSMPEG_LIB NAMES smpeg)
        message("SMPEG: ${LIBSMPEG_INCLUDE_DIR} ${LIBSMPEG_LIB}")
    else()
        set(FOUND_SMPEG 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBSMPEG_LIB smpeg)
        else()
            find_library(LIBSMPEG_LIB NAMES smpeg
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_SMPEG)
        message("== using SMPEG ==")
        add_definitions(-DMUSIC_MP3_SMPEG)
        list(APPEND SDLMixerX_LINK_LIBS ${LIBSMPEG_LIB})
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/smpeg/include)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_smpeg.c)
    endif()
endif()

option(USE_MP3_MPG123     "[WIP, DON'T USE IT] Build with MPG123 MP3 codec" OFF)
if(USE_MP3_MPG123)
    add_definitions(-DMUSIC_MP3_MPG123)

    message(WARNING "MPG123 SUPPORT IS WIP, DON'T USE IT IN PRODUCTION!!!")
    # ======= Until AudioCodecs will receive buildable mpg123, detect it externally =======
    include(cmake/FindMpg123.cmake)

    if(NOT MPG123_FOUND)
        message(FATAL_ERROR "MPG123 is not found!")
    else()
        message("MPG123 found in ${MPG123_INCLUDE_DIR} folder")
    endif()

    list(APPEND SDL_MIXER_INCLUDE_PATHS ${MPG123_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${MPG123_LIBRARIES})
    list(APPEND SDLMixerX_LINK_LIBS mpg123)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mpg123.c)
endif()

if(USE_MP3_MAD OR USE_MP3_MPG123 OR USE_MP3_SMPEG)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_id3tag.c)
endif()

option(USE_MODPLUG         "Build with ModPlug library" ON)
if(USE_MODPLUG)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(modplug ModPlug_Load "libmodplug/modplug.h" FOUND_MODPLUG)
        find_path(LIBMODPLUG_INCLUDE_DIR libmodplug/modplug.h)
        find_library(LIBMODPLUG_LIB NAMES modplug)
        message("ModPlug: ${LIBMODPLUG_INCLUDE_DIR} ${LIBMODPLUG_LIB}")
    else()
        set(FOUND_MODPLUG 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBMODPLUG_LIB modplug)
        else()
            find_library(LIBMODPLUG_LIB NAMES modplug
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_MODPLUG)
        message("== using libModPlug ==")
        add_definitions(-DMUSIC_MOD_MODPLUG -DMODPLUG_STATIC)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libmodplug/include)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBMODPLUG_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_modplug.c)
    endif()
endif()

option(USE_MIKMOD          "Build with MikMod library" OFF)
if(USE_MIKMOD)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(mikmod Player_Load "mikmod.h" FOUND_MIKMOD)
        find_path(LIBMIKMOD_INCLUDE_DIR "mikmod.h")
        find_library(LIBMIKMOD_LIB NAMES mikmod)
        message("MikMod: ${LIBMIKMOD_INCLUDE_DIR} ${LIBMIKMOD_LIB}")
    else()
        set(FOUND_MIKMOD 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBMIKMOD_LIB mikmod)
        else()
            find_library(LIBMIKMOD_LIB NAMES mikmod
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_MIKMOD)
        message("== using MikMod ==")
        add_definitions(-DMUSIC_MOD_MIKMOD)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libmikmod/include)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBMIKMOD_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mikmod.c)
    endif()
endif()

option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        check_library_exists(gme gme_load_file "gme.h" FOUND_GME)
        find_path(LIBGME_INCLUDE_DIR "gme.h")
        find_library(LIBGME_LIB NAMES gme)
        find_library(LIBZLIB_LIB NAMES z zlib)
        message("GME: ${LIBGME_INCLUDE_DIR} ${LIBGME_LIB}")
    else()
        set(FOUND_GME 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme)
            set(LIBZLIB_LIB zlib)
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBZLIB_LIB NAMES zlib z
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_GME)
        message("== using GME ==")
        add_definitions(-DMUSIC_GME)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libgme/include)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/zlib/include)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBGME_LIB} ${LIBZLIB_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_gme.c)
    endif()
endif()

if(NOT WIN32) # CMD Music is not supported on Windows
    option(USE_CMD             "Build with CMD music player support" ON)
    if(USE_CMD)
        message("== using CMD Music ==")
        add_definitions(-DMUSIC_CMD -D_POSIX_C_SOURCE=1)
        CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
        if(HAVE_FORK)
            add_definitions(-DHAVE_FORK)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_cmd.c)
    endif()
endif()

option(USE_MIDI            "Build with MIDI support" ON)
if(USE_MIDI)

    option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_ADLMIDI)
        if(USE_SYSTEM_AUDIO_LIBRARIES)
            check_library_exists(ADLMIDI adl_init "adlmidi.h" FOUND_ADLMIDI)
            find_path(LIBADLMIDI_INCLUDE_DIR "adlmidi.h")
            find_library(LIBADLMIDI_LIB NAMES ADLMIDI)
            message("ADLMIDI: ${LIBADLMIDI_INCLUDE_DIR} ${LIBADLMIDI_LIB}")
        else()
            set(FOUND_ADLMIDI 1)
            if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
                set(LIBADLMIDI_LIB ADLMIDI)
            else()
                find_library(LIBADLMIDI_LIB NAMES ADLMIDI
                             HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            endif()
        endif()
        if(FOUND_ADLMIDI)
            message("== using ADLMIDI ==")
            add_definitions(-DMUSIC_MID_ADLMIDI)
            if(AUDIO_CODECS_REPO_PATH)
                list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libADLMIDI/include)
            endif()
            set(LIBMATH_NEEDED 1)
            list(APPEND SDLMixerX_LINK_LIBS ${LIBADLMIDI_LIB})
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_adl.c)
        endif()
    endif()

    option(USE_MIDI_OPNMIDI    "Build with libOPNMIDI OPN2 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_OPNMIDI)
        if(USE_SYSTEM_AUDIO_LIBRARIES)
            check_library_exists(OPNMIDI opn_init "opnmidi.h" FOUND_OPNMIDI)
            find_path(LIBOPNMIDI_INCLUDE_DIR "opnmidi.h")
            find_library(LIBOPNMIDI_LIB NAMES OPNMIDI)
            message("OPNMIDI: ${LIBOPNMIDI_INCLUDE_DIR} ${LIBOPNMIDI_LIB}")
        else()
            set(FOUND_OPNMIDI 1)
            if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
                set(LIBOPNMIDI_LIB OPNMIDI)
            else()
                find_library(LIBOPNMIDI_LIB NAMES OPNMIDI
                             HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            endif()
        endif()
        if(FOUND_OPNMIDI)
            message("== using OPNMIDI ==")
            add_definitions(-DMUSIC_MID_OPNMIDI)
            if(AUDIO_CODECS_REPO_PATH)
                list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libOPNMIDI/include)
            endif()
            set(LIBMATH_NEEDED 1)
            list(APPEND SDLMixerX_LINK_LIBS ${LIBOPNMIDI_LIB})
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_opn.c)
        endif()
    endif()

    option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
    if(USE_MIDI_TIMIDITY AND NOT USE_SYSTEM_AUDIO_LIBRARIES)
        message("== using Timidity (custom) ==")
        add_definitions(-DMUSIC_MID_TIMIDITY)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libtimidity/include)
        endif()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBTIMIDITY_LIB timidity)
        else()
            find_library(LIBTIMIDITY_LIB NAMES timidity
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBTIMIDITY_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_timidity.c)
    endif()

    option(USE_MIDI_FLUIDSYNTH "Build with FluidSynth wave table MIDI sequencer support" OFF)
    if(USE_MIDI_FLUIDSYNTH)
        if(USE_SYSTEM_AUDIO_LIBRARIES)
            check_library_exists(fluidsynth new_fluid_midi_router "fluidsynth.h" FOUND_FLUIDSYNTH)
            find_path(LIBFLUIDSYNTH_INCLUDE_DIR "fluidsynth.h")
            find_library(LIBFLUIDSYNTH_LIB NAMES fluidsynth)
            message("FluidSynth: ${LIBFLUIDSYNTH_INCLUDE_DIR} ${LIBFLUIDSYNTH_LIB}")
        else()
            set(FOUND_FLUIDSYNTH 1)
            if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
                set(LIBFLUIDSYNTH_LIB fluidsynth)
            else()
                find_library(LIBFLUIDSYNTH_LIB NAMES fluidsynth
                             HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            endif()
        endif()
        if(FOUND_FLUIDSYNTH)
            message("== using FluidSynth ==")
            add_definitions(-DMUSIC_MID_FLUIDSYNTH)
            if(AUDIO_CODECS_REPO_PATH)
                list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/FluidLite/include)
            endif()
            list(APPEND SDLMixerX_LINK_LIBS ${LIBFLUIDSYNTH_LIB})
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/music_fluidsynth.c)
        endif()
    endif()

     # Native MIDI correctly works on Windows and macOS only.
    if(WIN32 OR APPLE)
        set(NATIVE_MIDI_SUPPORTED ON)
    else()
        set(NATIVE_MIDI_SUPPORTED OFF)
    endif()

    option(USE_MIDI_NATIVE     "Build with operating system native MIDI output support" ${NATIVE_MIDI_SUPPORTED})
    if(USE_MIDI_NATIVE)
        add_definitions(-DMUSIC_MID_NATIVE)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_nativemidi.c
            ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_common.c)
        if(WIN32)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_win32.c)
            list(APPEND SDLMixerX_LINK_LIBS winmm)
        endif()
        if(APPLE)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_macosx.c)
        endif()
    endif()
    if(NOT USE_MIDI_ADLMIDI AND
       NOT USE_MIDI_OPNMIDI AND
       NOT USE_MIDI_TIMIDITY AND
       NOT USE_MIDI_FLUIDSYNTH AND
       NOT USE_MIDI_NATIVE)
        message_error("To have MIDI support you must enable at least one MIDI sequencer library")
    endif()

endif()

if(USE_SYSTEM_AUDIO_LIBRARIES)
    find_library(LIBSDL2CUSTOM_LIB NAMES SDL2)
else()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(LIBSDL2CUSTOM_LIB SDL2$<$<CONFIG:Debug>:d>)
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_SHARED})
        find_library(LIBSDL2CUSTOM_LIB NAMES SDL2
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
    endif()
endif()
list(APPEND SDLMixerX_LINK_LIBS ${LIBSDL2CUSTOM_LIB})

if(LIBMATH_NEEDED AND NOT MSVC)
    list(APPEND SDLMixerX_LINK_LIBS m)
endif()

if(WIN32)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        message("== SDL Mixer X will be built as solid module that is ready for MS-VB6 binding! ==")
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/src/vb6_binding/vb6_sdl_binds.c)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2$<$<CONFIG:Debug>:d>)
        list(APPEND SDLMixerX_LINK_LIBS
                    SDL2-static
                    uuid winmm ole32 imm32 version oleaut32 user32 gdi32)
        if(NOT MSVC)
            list(APPEND SDLMixerX_LINK_LIBS stdc++ gcc pthread)
        endif()
    endif()
endif()

if(APPLE)
    find_library(APPLE_CORE_AUDIO CoreAudio)
    find_library(APPLE_IOKIT IOKit)
    find_library(APPLE_COCOA Cocoa)
    find_library(APPLE_AUDIOTOOLBOX AudioToolbox)
    find_library(APPLE_CORE_VIDEO CoreVideo)
    find_library(APPLE_CORE_FOUNDATION CoreFoundation)
    find_library(APPLE_CARBON Carbon)
    find_library(APPLE_FORCE_FEEDBACK ForceFeedback)
    find_library(ICONV_LIB iconv)
    list(APPEND SDLMixerX_LINK_LIBS
            ${APPLE_CORE_AUDIO}
            ${APPLE_COCOA}
            ${APPLE_CORE_VIDEO}
            ${APPLE_CORE_FOUNDATION}
            ${APPLE_FORCE_FEEDBACK}
            ${APPLE_IOKIT}
            ${APPLE_CARBON}
            ${APPLE_AUDIOTOOLBOX}
            ${ICONV_LIB})
    list(APPEND SDLMixerX_LINK_LIBS objc)
endif()

if(NOT MSVC)
    list(APPEND SDLMixerX_LINK_LIBS stdc++)
endif()

list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/effect_position.c
        ${SDLMixerX_SOURCE_DIR}/src/effects_internal.c
        ${SDLMixerX_SOURCE_DIR}/src/effect_stereoreverse.c
        ${SDLMixerX_SOURCE_DIR}/src/mixer.c
        ${SDLMixerX_SOURCE_DIR}/src/music.c
        ${SDLMixerX_SOURCE_DIR}/src/mixer_x_deprecated.c
    )

file(GLOB SDLMixerX_SOURCES ${SDLMixerX_SOURCES})

set(SDLMixerX_INSTALLS)

if(SDL_MIXER_X_STATIC AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_Static STATIC ${SDLMixerX_SOURCES})
    if(WIN32)
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext-static)
    else()
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext)
    endif()
    target_include_directories(SDL2_mixer_ext_Static PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )
    target_include_directories(SDL2_mixer_ext_Static PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext_Static AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_Static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext_Static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer_ext/SDL_mixer_ext.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_Static)
endif()

if(WIN32)
    list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/version.rc)
endif()

if(SDL_MIXER_X_SHARED AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext SHARED ${SDLMixerX_SOURCES})
    target_link_libraries(SDL2_mixer_ext ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )
    target_include_directories(SDL2_mixer_ext PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer_ext/SDL_mixer_ext.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext)
endif()

if(BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_VB6 MODULE ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES OUTPUT_NAME SDL2MixerVB)
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES PREFIX "")
    target_compile_definitions(SDL2_mixer_ext_VB6 PRIVATE -DFORCE_STDCALLS)
    target_link_libraries(SDL2_mixer_ext_VB6 ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES LINK_FLAGS
        "-Wl,--add-stdcall-alias -static-libgcc -static-libstdc++ -static -lpthread")
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext_VB6 AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_VB6
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
    )
    add_custom_command(
        TARGET SDL2_mixer_ext_VB6 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/VB6_Wrapper/modSDL2_mixer_ext_vb6.bas
                ${CMAKE_BINARY_DIR}/sdl-mixer-vb6)
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_VB6)
endif()

install(TARGETS ${SDLMixerX_INSTALLS}
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        FRAMEWORK DESTINATION "lib"
        INCLUDES DESTINATION "include")

if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/bin
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/lib
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/include
            DESTINATION .)
endif()

install(FILES
        include/SDL_mixer_ext/begin_code.h
        include/SDL_mixer_ext/close_code.h
        include/SDL_mixer_ext/SDL_mixer_ext.h
        DESTINATION include/SDL2)

