option(USE_DRFLAC "Build with FLAC codec using drflac" ON)
if(USE_DRFLAC)
    message("== using FLAC (public domain or MIT-0) ==")
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FLAC_DRFLAC)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_drflac.c
        ${CMAKE_CURRENT_LIST_DIR}/music_drflac.h
        ${CMAKE_CURRENT_LIST_DIR}/dr_libs/dr_flac.h
    )
    appendPcmFormats("FLAC")
endif()
