option(USE_MP3_DRMP3  "Build with MP3 codec using dr_mp3" ON)
option(USE_MP3_MINIMP3  "[compatibility fallback] Build with MP3 codec using dr_mp3" OFF)

mark_as_advanced(USE_MP3_MINIMP3)

if(USE_MP3_DRMP3 OR USE_MP3_MINIMP3)
    message("== using DRMP3 (public domain or MIT-0) ==")
    setLicense(LICENSE_PUBLICDOMAIN)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MP3_DRMP3)

    if(MIXERX_DISABLE_SIMD)
        list(APPEND SDL_MIXER_DEFINITIONS -DDR_MP3_NO_SIMD=1)
    endif()

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_drmp3.c
        ${CMAKE_CURRENT_LIST_DIR}/music_drmp3.h
        ${CMAKE_CURRENT_LIST_DIR}/dr_libs/dr_mp3.h
    )
    appendPcmFormats("MP3")
endif()
