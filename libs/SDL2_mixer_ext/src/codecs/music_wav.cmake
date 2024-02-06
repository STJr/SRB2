option(USE_WAV "Build with WAV codec" ON)
if(USE_WAV)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_wav.c
        ${CMAKE_CURRENT_LIST_DIR}/music_wav.h
    )
    appendPcmFormats("WAV(Music);AIFF(Music)")
endif()

list(APPEND SDLMixerX_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/load_aiff.c   ${CMAKE_CURRENT_LIST_DIR}/load_aiff.h
    ${CMAKE_CURRENT_LIST_DIR}/load_voc.c    ${CMAKE_CURRENT_LIST_DIR}/load_voc.h
)
