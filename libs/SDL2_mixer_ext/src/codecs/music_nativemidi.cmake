 # Native MIDI correctly works on Windows and macOS only.
if((WIN32 AND NOT EMSCRIPTEN) OR (APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER 10))
    set(NATIVE_MIDI_SUPPORTED ON)
else()
    set(NATIVE_MIDI_SUPPORTED OFF)
endif()

option(USE_MIDI_NATIVE     "Build with operating system native MIDI output support" ${NATIVE_MIDI_SUPPORTED})
if(USE_MIDI_NATIVE AND NOT USE_MIDI_NATIVE_ALT)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_NATIVE)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_nativemidi.c
        ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_common.c
        ${CMAKE_CURRENT_LIST_DIR}/music_nativemidi.h
    )
    if(WIN32)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_win32.c)
        list(APPEND SDLMixerX_LINK_LIBS winmm)
    endif()
    if(APPLE)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_macosx.c)
    endif()
    appendMidiFormats("MIDI;RIFF MIDI")
endif()
