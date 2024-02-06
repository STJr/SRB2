option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support (GPL)" ON)
if(USE_MIDI_ADLMIDI AND MIXERX_GPL)
    option(USE_MIDI_ADLMIDI_DYNAMIC "Use dynamical loading of libADLMIDI library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(ADLMIDI QUIET)
        message("ADLMIDI: [${ADLMIDI_FOUND}] ${ADLMIDI_INCLUDE_DIRS} ${ADLMIDI_LIBRARIES}")

        if(USE_MIDI_ADLMIDI_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DADLMIDI_DYNAMIC=\"${ADLMIDI_DYNAMIC_LIBRARY}\")
            message("Dynamic libADLMIDI: ${ADLMIDI_DYNAMIC_LIBRARY}")
        endif()

        if(ADLMIDI_FOUND)
            cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/adlmidi.c
                ""
                ${ADLMIDI_INCLUDE_DIRS}
                "${ADLMIDI_LIBRARIES};${M_LIBRARY}"
                STDCPP_NEEDED
            )

            try_compile(ADLMIDI_HAS_CHANNEL_ALLOC_MODE
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/adlmidi_channel_alloc_mode.c
                LINK_LIBRARIES ${ADLMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ADLMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE ADLMIDI_HAS_CHANNEL_ALLOC_MODE_RESULT
            )
            message("ADLMIDI_HAS_CHANNEL_ALLOC_MODE compile test result: ${ADLMIDI_HAS_CHANNEL_ALLOC_MODE}")

            try_compile(ADLMIDI_HAS_GET_SONGS_COUNT
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/adlmidi_get_songs_count.c
                LINK_LIBRARIES ${ADLMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ADLMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE ADLMIDI_HAS_GET_SONGS_COUNT_RESULT
            )

            try_compile(ADLMIDI_HAS_SELECT_SONG_NUM
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/adlmidi_get_songs_count.c
                LINK_LIBRARIES ${ADLMIDI_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ADLMIDI_INCLUDE_DIRS}"
                OUTPUT_VARIABLE ADLMIDI_HAS_SELECT_SONG_NUM_RESULT
            )
        endif()

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(ADLMIDI_LIBRARIES ADLMIDI${MIX_DEBUG_SUFFIX})
        else()
            find_library(ADLMIDI_LIBRARIES NAMES ADLMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()

        if(ADLMIDI_LIBRARIES)
            set(ADLMIDI_FOUND 1)
            set(ADLMIDI_HAS_CHANNEL_ALLOC_MODE TRUE)
            set(ADLMIDI_HAS_GET_SONGS_COUNT TRUE)
            seT(ADLMIDI_HAS_SELECT_SONG_NUM TRUE)
            set(STDCPP_NEEDED 1) # Statically linking ADLMIDI which is C++ library
        endif()
        set(ADLMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libADLMIDI/include")
    endif()

    if(ADLMIDI_FOUND)
        message("== using ADLMIDI (GPLv3+) ==")
        setLicense(LICENSE_GPL_3p)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_ADLMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${ADLMIDI_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_ADLMIDI_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${ADLMIDI_LIBRARIES})
        endif()

        if(ADLMIDI_HAS_CHANNEL_ALLOC_MODE)
            list(APPEND SDL_MIXER_DEFINITIONS -DADLMIDI_HAS_CHANNEL_ALLOC_MODE)
        endif()

        if(ADLMIDI_HAS_GET_SONGS_COUNT)
            list(APPEND SDL_MIXER_DEFINITIONS -DADLMIDI_HAS_GET_SONGS_COUNT)
        endif()

        if(ADLMIDI_HAS_SELECT_SONG_NUM)
            list(APPEND SDL_MIXER_DEFINITIONS -DADLMIDI_HAS_SELECT_SONG_NUM)
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_adl.c
            ${CMAKE_CURRENT_LIST_DIR}/music_midi_adl.h
        )
        appendMidiFormats("MIDI;RIFF MIDI;XMI;MUS")
        appendChiptuneFormats("IMF(OPL2);CMF")
    else()
        message("-- skipping ADLMIDI --")
    endif()
endif()
