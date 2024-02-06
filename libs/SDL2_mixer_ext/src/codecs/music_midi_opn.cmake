option(USE_MIDI_OPNMIDI    "Build with libOPNMIDI OPN2 Emulator based MIDI sequencer support (GPL)" ON)
if(USE_MIDI_OPNMIDI AND MIXERX_GPL)
    option(USE_MIDI_OPNMIDI_DYNAMIC "Use dynamical loading of libOPNMIDI library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(OPNMIDI QUIET)
        message("OPNMIDI: [${OPNMIDI_FOUND}] ${OPNMIDI_INCLUDE_DIRS} ${OPNMIDI_LIBRARIES}")

        if(USE_MIDI_OPNMIDI_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPNMIDI_DYNAMIC=\"${OPNMIDI_DYNAMIC_LIBRARY}\")
            message("Dynamic libOPNMIDI: ${OPNMIDI_DYNAMIC_LIBRARY}")
        endif()

        if(OPNMIDI_FOUND)
            cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/opnmidi.c
                ""
                ${OPNMIDI_INCLUDE_DIRS}
                "${OPNMIDI_LIBRARIES};${M_LIBRARY}"
                STDCPP_NEEDED
            )

            try_compile(OPNMIDI_HAS_CHANNEL_ALLOC_MODE
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/opnmidi_channel_alloc_mode.c
                LINK_LIBRARIES ${OPNMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${OPNMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE OPNMIDI_HAS_CHANNEL_ALLOC_MODE_RESULT
            )
            message("OPNMIDI_HAS_CHANNEL_ALLOC_MODE compile test result: ${OPNMIDI_HAS_CHANNEL_ALLOC_MODE}")

            try_compile(OPNMIDI_HAS_GET_SONGS_COUNT
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/opnmidi_get_songs_count.c
                LINK_LIBRARIES ${OPNMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${OPNMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE OPNMIDI_HAS_GET_SONGS_COUNT_RESULT
            )

            try_compile(OPNMIDI_HAS_SELECT_SONG_NUM
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/opnmidi_get_songs_count.c
                LINK_LIBRARIES ${OPNMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${OPNMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE OPNMIDI_HAS_SELECT_SONG_NUM_RESULT
            )
        endif()

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(OPNMIDI_LIBRARIES OPNMIDI${MIX_DEBUG_SUFFIX})
        else()
            find_library(OPNMIDI_LIBRARIES NAMES OPNMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()

        if(OPNMIDI_LIBRARIES)
            set(OPNMIDI_FOUND 1)
            set(OPNMIDI_HAS_CHANNEL_ALLOC_MODE TRUE)
            set(OPNMIDI_HAS_GET_SONGS_COUNT TRUE)
            set(OPNMIDI_HAS_SELECT_SONG_NUM TRUE)
            set(STDCPP_NEEDED 1) # Statically linking OPNMIDI which is C++ library
        endif()

        set(OPNMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libOPNMIDI/include")
    endif()

    if(OPNMIDI_FOUND)
        message("== using OPNMIDI (GPLv3+) ==")
        setLicense(LICENSE_GPL_3p)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_OPNMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${OPNMIDI_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_OPNMIDI_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${OPNMIDI_LIBRARIES})
        endif()

        if(OPNMIDI_HAS_CHANNEL_ALLOC_MODE)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPNMIDI_HAS_CHANNEL_ALLOC_MODE)
        endif()

        if(OPNMIDI_HAS_GET_SONGS_COUNT)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPNMIDI_HAS_GET_SONGS_COUNT)
        endif()

        if(OPNMIDI_HAS_SELECT_SONG_NUM)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPNMIDI_HAS_SELECT_SONG_NUM)
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_opn.c
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_opn.h
            ${CMAKE_CURRENT_LIST_DIR}/OPNMIDI/gm_opn_bank.h
        )
        appendMidiFormats("MIDI;RIFF MIDI;XMI;MUS")
    else()
        message("-- skipping OPNMIDI --")
    endif()
endif()
