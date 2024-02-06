option(USE_MIDI_FLUIDSYNTH "Build with FluidSynth wave table MIDI sequencer support (LGPL)" OFF)
if(USE_MIDI_FLUIDSYNTH AND NOT USE_MIDI_FLUIDLITE AND MIXERX_LGPL)
    option(USE_MIDI_FLUIDSYNTH_DYNAMIC "Use dynamical loading of FluidSynth" OFF)

    if(NOT USE_SYSTEM_AUDIO_LIBRARIES)
        message(WARNING "AudioCodecs doesn't ship FluidSynth, it will be recognized from a system!!!")
    endif()

    find_package(FluidSynth QUIET)
    message("FluidSynth: [${FluidSynth_FOUND}] ${FluidSynth_INCLUDE_DIRS} ${FluidSynth_LIBRARIES}")
    if(USE_MIDI_FLUIDSYNTH_DYNAMIC)
        list(APPEND SDL_MIXER_DEFINITIONS -DFLUIDSYNTH_DYNAMIC=\"${FluidSynth_DYNAMIC_LIBRARY}\")
        message("Dynamic FluidSynth: ${FluidSynth_DYNAMIC_LIBRARY}")
    endif()

    if(FluidSynth_FOUND)
        message("== using FluidSynth (LGPLv2.1+) ==")
        if(NOT USE_MIDI_FLUIDSYNTH_DYNAMIC)
            setLicense(LICENSE_LGPL_2_1p)
        endif()

        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_FLUIDSYNTH)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${FluidSynth_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_FLUIDSYNTH_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${FluidSynth_LIBRARIES})
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_fluidsynth.c
            ${CMAKE_CURRENT_LIST_DIR}/music_fluidsynth.h
        )
        appendMidiFormats("MIDI;RIFF MIDI")
    else()
        message("-- skipping FluidSynth --")
    endif()
endif()

if(USE_MIDI_FLUIDSYNTH AND USE_MIDI_FLUIDLITE)
    message("-- skipping FluidSynth because FluidLite is used instead --")
endif()
