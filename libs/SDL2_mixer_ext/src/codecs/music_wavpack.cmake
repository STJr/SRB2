option(USE_WAVPACK      "Build with WAVPACK codec" ON)
if(USE_WAVPACK)
    option(USE_WAVPACK_DYNAMIC "Use dynamical loading of WAVPACK" OFF)
    option(USE_WAVPACK_DSD "Enable WavPack DSD music support" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(WavPack QUIET)
        message("WavPack: [${WavPack_FOUND}] ${WavPack_INCLUDE_DIRS} ${WavPack_LIBRARIES}")
        if(USE_WAVPACK_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DWAVPACK_DYNAMIC=\"${WavPack_DYNAMIC_LIBRARY}\")
            message("Dynamic WavPack: ${WavPack_DYNAMIC_LIBRARY}")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(WavPack_LIBRARIES wavpack${MIX_DEBUG_SUFFIX})
        else()
            find_library(LIBWAVPACK_LIB NAMES wavpack
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            set(WavPack_LIBRARIES ${LIBWAVPACK_LIB})
        endif()
        set(WavPack_FOUND 1)
        set(WavPack_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_DIR}/include/wavpack
            ${AUDIO_CODECS_PATH}/WavPack/include
        )
    endif()

    if(WavPack_FOUND)
        message("== using WavPack (BSD 3-Clause) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_WAVPACK)
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_WAVPACK_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${WavPack_LIBRARIES} ${LIBWAVPACK_LIB})
            set(LIBMATH_NEEDED ON)
        endif()
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${WavPack_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_wavpack.c
            ${CMAKE_CURRENT_LIST_DIR}/music_wavpack.h
        )
        if(DEFINED FLAG_C99)
            set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/music_wavpack.c"
                COMPILE_FLAGS ${FLAG_C99}
            )
        endif()
        if(USE_WAVPACK_DSD)
            list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_WAVPACK_DSD)
        endif()
        appendPcmFormats("WV")
    else()
        message("-- skipping WavPack --")
    endif()
endif()
