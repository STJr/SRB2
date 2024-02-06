option(USE_MIDI_FLUIDLITE "Build with an FluidLite wave table MIDI sequencer support (a light-weight of the FluidSynth) (LGPL)" ON)
if(USE_MIDI_FLUIDLITE AND MIXERX_LGPL)
    option(USE_MIDI_FLUIDLITE_DYNAMIC "Use dynamical loading of FluidLite" OFF)
    option(USE_MIDI_FLUIDLITE_OGG_STB "Don't require the vorbis linking (set this flag if FluidLite build has std_vorbis used)" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(FluidLite QUIET)
        message("FluidLite: [${FluidLite_FOUND}] ${FluidLite_INCLUDE_DIRS} ${FluidLite_LIBRARIES}")
        if(USE_MIDI_FLUIDLITE_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DFLUIDSYNTH_DYNAMIC=\"${FluidLite_DYNAMIC_LIBRARY}\")
            message("Dynamic FluidLite: ${FluidLite_DYNAMIC_LIBRARY}")
        elseif(USE_OGG_VORBIS_STB AND NOT USE_MIDI_FLUIDLITE_OGG_STB)
            find_package(Vorbis QUIET)
            list(APPEND FluidLite_LIBRARIES ${Vorbis_LIBRARIES})
            set(LIBOGG_NEEDED ON)
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            if(USE_OGG_VORBIS_STB AND AUDIOCODECS_BUILD_OGG_VORBIS AND NOT USE_MIDI_FLUIDLITE_OGG_STB)  # Keep the vorbis linking
                set(FluidLite_LIBRARIES fluidlite${MIX_DEBUG_SUFFIX} vorbisfile${MIX_DEBUG_SUFFIX} vorbis${MIX_DEBUG_SUFFIX})
                set(LIBOGG_NEEDED ON)
            else()
                set(FluidLite_LIBRARIES fluidlite${MIX_DEBUG_SUFFIX})
            endif()
        else()
            find_library(FluidLite_LIBRARIES NAMES fluidlite
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            if(USE_OGG_VORBIS_STB AND NOT USE_MIDI_FLUIDLITE_OGG_STB)
                find_library(Vorbis_LIBRARY NAMES vorbis
                            HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
                find_library(VorbisFile_LIBRARY NAMES vorbisfile
                            HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
                list(APPEND FluidLite_LIBRARIES ${VorbisFile_LIBRARY} ${Vorbis_LIBRARY})
                set(LIBOGG_NEEDED ON)
            endif()
        endif()
        set(FluidLite_FOUND 1)
        set(FluidLite_INCLUDE_DIRS ${AUDIO_CODECS_PATH}/FluidLite/include)
    endif()

    if(FluidLite_FOUND)
        message("== using FluidLite (LGPLv2.1+) ==")
        if(NOT USE_MIDI_FLUIDLITE_DYNAMIC)
            setLicense(LICENSE_LGPL_2_1p)
        endif()

        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_FLUIDSYNTH -DMUSIC_MID_FLUIDLITE)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${FluidLite_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_FLUIDLITE_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${FluidLite_LIBRARIES})
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_fluidlite.c
            ${CMAKE_CURRENT_LIST_DIR}/music_fluidsynth.h
        )
        set(CPP_MIDI_SEQUENCER_NEEDED TRUE)
    else()
        message("-- skipping FluidLite --")
    endif()
endif()
