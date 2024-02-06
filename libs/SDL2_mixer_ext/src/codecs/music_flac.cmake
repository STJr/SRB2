option(USE_FLAC            "Build with FLAC codec using libFLAC" OFF)
if(USE_FLAC)
    option(USE_FLAC_DYNAMIC "Use dynamical loading of FLAC" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(FLAC QUIET)
        message("FLAC: [${FLAC_FOUND}] ${FLAC_INCLUDE_DIRS} ${FLAC_LIBRARIES}")
        if(USE_FLAC_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DFLAC_DYNAMIC=\"${FLAC_DYNAMIC_LIBRARY}\")
            message("Dynamic FLAC: ${FLAC_DYNAMIC_LIBRARY}")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(FLAC_LIBRARIES FLAC${MIX_DEBUG_SUFFIX})
        else()
            find_library(FLAC_LIBRARIES NAMES FLAC
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()

        set(FLAC_FOUND 1)
        set(FLAC_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_DIR}/include/FLAC
            ${AUDIO_CODECS_INSTALL_DIR}/include/ogg
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libFLAC/include
        )
    endif()

    if(FLAC_FOUND)
        message("== using FLAC (BSD 3-clause) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FLAC_LIBFLAC -DFLAC__NO_DLL)

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_FLAC_DYNAMIC)
            set(LIBOGG_NEEDED ON)
            set(LIBMATH_NEEDED ON)
            list(APPEND SDLMixerX_LINK_LIBS ${FLAC_LIBRARIES})
        endif()

        list(APPEND SDL_MIXER_INCLUDE_PATHS ${FLAC_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_flac.c
            ${CMAKE_CURRENT_LIST_DIR}/music_flac.h
        )
        appendPcmFormats("FLAC")
    else()
        message("-- skipping FLAC --")
    endif()
endif()
