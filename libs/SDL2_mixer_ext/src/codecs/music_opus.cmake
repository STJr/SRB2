option(USE_OPUS      "Build with OPUS codec" ON)
if(USE_OPUS)
    option(USE_OPUS_DYNAMIC "Use dynamical loading of Opus" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(Opus QUIET)
        message("Opus: [${Opus_FOUND}] ${Opus_INCLUDE_DIRS} ${Opus_LIBRARIES} ${LIBOPUS_LIB}")
        if(USE_OPUS_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DOPUS_DYNAMIC=\"${OpusFile_DYNAMIC_LIBRARY}\")
            message("Dynamic Opus: ${OpusFile_DYNAMIC_LIBRARY}")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(Opus_LIBRARIES opusfile${MIX_DEBUG_SUFFIX} opus${MIX_DEBUG_SUFFIX})
        else()
            find_library(LIBOPUSFILE_LIB NAMES opusfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBOPUS_LIB NAMES opus
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            set(Opus_LIBRARIES ${LIBOPUSFILE_LIB} ${LIBOPUS_LIB})
        endif()
        set(Opus_FOUND 1)
        set(Opus_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_DIR}/include/opus
            ${AUDIO_CODECS_INSTALL_DIR}/include/ogg
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libopus/include
            ${AUDIO_CODECS_PATH}/libopusfile/include
        )
    endif()

    if(Opus_FOUND)
        message("== using Opus (BSD 3-Clause) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_OPUS)
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OPUS_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${Opus_LIBRARIES} ${LIBOPUS_LIB})
            set(LIBOGG_NEEDED ON)
            set(LIBMATH_NEEDED ON)
        endif()
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${Opus_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_opus.c
            ${CMAKE_CURRENT_LIST_DIR}/music_opus.h
        )
        appendPcmFormats("Opus")
    else()
        message("-- skipping Opus --")
    endif()
endif()
