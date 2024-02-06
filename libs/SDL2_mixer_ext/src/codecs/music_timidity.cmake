option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
if(USE_MIDI_TIMIDITY)
    message("== using Timidity-SDL (Artistic Public License) ==")

    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(TIMIDITYSDL_LIBRARIES timidity_sdl2${MIX_DEBUG_SUFFIX})
        set(TIMIDITYSDL_FOUND True)
    elseif(NOT USE_SYSTEM_AUDIO_LIBRARIES)
        find_library(TIMIDITYSDL_LIBRARIES NAMES timidity_sdl2
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        if(TIMIDITYSDL_LIBRARIES)
            set(TIMIDITYSDL_FOUND True)
        endif()
    endif()

    if(NOT TIMIDITYSDL_LIBRARIES)
        message("-- !! Timidity-SDL2 library is not found, using local sources to build it")
        set(TIMIDITYSDL_LIBRARIES)
        list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_CONFIG_H=0)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/timidity/common.c     ${CMAKE_CURRENT_LIST_DIR}/timidity/common.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/instrum.c    ${CMAKE_CURRENT_LIST_DIR}/timidity/instrum.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/mix.c        ${CMAKE_CURRENT_LIST_DIR}/timidity/mix.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/output.c     ${CMAKE_CURRENT_LIST_DIR}/timidity/output.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/playmidi.c   ${CMAKE_CURRENT_LIST_DIR}/timidity/playmidi.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/readmidi.c   ${CMAKE_CURRENT_LIST_DIR}/timidity/readmidi.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/resample.c   ${CMAKE_CURRENT_LIST_DIR}/timidity/resample.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/tables.c     ${CMAKE_CURRENT_LIST_DIR}/timidity/tables.h
            ${CMAKE_CURRENT_LIST_DIR}/timidity/timidity.c   ${CMAKE_CURRENT_LIST_DIR}/timidity/timidity.h
        )
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${CMAKE_CURRENT_LIST_DIR}/timidity)
        set(TIMIDITYSDL_FOUND True)
    endif()

    if(TIMIDITYSDL_FOUND)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_TIMIDITY)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libtimidity/include)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${TIMIDITYSDL_LIBRARIES})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_timidity.c
            ${CMAKE_CURRENT_LIST_DIR}/music_timidity.h
        )
        appendMidiFormats("MIDI;RIFF MIDI")
    endif()
endif()
