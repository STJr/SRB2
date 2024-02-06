option(USE_MIDI_EDMIDI    "Build with libEDMIDI OPLL, PSG, and SCC Emulator based MIDI sequencer support" ON)
if(USE_MIDI_EDMIDI)
    option(USE_MIDI_EDMIDI_DYNAMIC "Use dynamical loading of libEDMIDI library" OFF)
    option(USE_SYSTEM_EDMIDI "Use the library from the system even USE_SYSTEM_AUDIO_LIBRARIES is unset" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES OR USE_SYSTEM_EDMIDI)
        find_package(EDMIDI QUIET)
        message("EDMIDI: [${EDMIDI_FOUND}] ${EDMIDI_INCLUDE_DIRS} ${EDMIDI_LIBRARIES}")
        if(USE_MIDI_EDMIDI_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DEDMIDI_DYNAMIC=\"${EDMIDI_DYNAMIC_LIBRARY}\")
            message("Dynamic libEDMIDI: ${EDMIDI_DYNAMIC_LIBRARY}")
        endif()

        if(EDMIDI_FOUND)
            cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/edmidi.c
                ""
                ${EDMIDI_INCLUDE_DIRS}
                "${EDMIDI_LIBRARIES};${M_LIBRARY}"
                STDCPP_NEEDED
            )
        endif()

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(EDMIDI_LIBRARIES EDMIDI${MIX_DEBUG_SUFFIX})
        else()
            find_library(EDMIDI_LIBRARIES NAMES EDMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        if(EDMIDI_LIBRARIES)
            set(EDMIDI_FOUND 1)
            set(STDCPP_NEEDED 1) # Statically linking EDMIDI which is C++ library
        endif()
        set(EDMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libEDMIDI/include")
    endif()

    if(EDMIDI_FOUND)
        message("== using EDMIDI (Zlib) ==")
        setLicense(LICENSE_ZLIB)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_EDMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${EDMIDI_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_EDMIDI_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${EDMIDI_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_edmidi.c
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_edmidi.h
        )
        appendMidiFormats("MIDI;RIFF MIDI;XMI;MUS")
    else()
        message("-- skipping EDMIDI --")
    endif()
endif()
