option(USE_FFMPEG             "Build with FFMPEG library (for AAC and WMA formats) (LGPL)" OFF)
if(USE_FFMPEG AND MIXERX_LGPL)
    option(USE_FFMPEG_DYNAMIC "Use dynamical loading of FFMPEG" ON)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(FFMPEG QUIET)
        message("FFMPEG: [${FFMPEG_avcodec_FOUND}] ${FFMPEG_INCLUDE_DIRS} ${FFMPEG_swresample_LIBRARY} ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY}")

        if(USE_FFMPEG_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS
                -DFFMPEG_DYNAMIC=ON
                -DFFMPEG_DYNAMIC_AVUTIL=\"${FFMPEG_avutil_DYNAMIC_LIBRARY}\"
                -DFFMPEG_DYNAMIC_AVCODEC=\"${FFMPEG_avcodec_DYNAMIC_LIBRARY}\"
                -DFFMPEG_DYNAMIC_AVFORMAT=\"${FFMPEG_avformat_DYNAMIC_LIBRARY}\"
                -DFFMPEG_DYNAMIC_SWRESAMPLE=\"${FFMPEG_swresample_DYNAMIC_LIBRARY}\"
            )
            message("Dynamic FFMPEG: ${FFMPEG_avutil_DYNAMIC_LIBRARY} ${FFMPEG_avcodec_DYNAMIC_LIBRARY} ${FFMPEG_avformat_DYNAMIC_LIBRARY} ${FFMPEG_swresample_DYNAMIC_LIBRARY}")
        endif()

        set(FFMPEG_LINK_LIBRARIES
            ${FFMPEG_swresample_LIBRARY}
            ${FFMPEG_avformat_LIBRARY}
            ${FFMPEG_avcodec_LIBRARY}
            ${FFMPEG_avutil_LIBRARY}
        )
    else()
        message(WARNING "FFMPEG libraries are not a part of AudioCodecs yet. Using any available from the system.")
        set(FFMPEG_INCLUDE_DIRS "" CACHE PATH "")
        set(FFMPEG_swresample_LIBRARY swresample CACHE PATH "")
        set(FFMPEG_avformat_LIBRARY avformat CACHE PATH "")
        set(FFMPEG_avcodec_LIBRARY avcodec CACHE PATH "")
        set(FFMPEG_avutil_LIBRARY avutil CACHE PATH "")
        mark_as_advanced(FFMPEG_INCLUDE_DIRS FFMPEG_swresample_LIBRARY FFMPEG_avformat_LIBRARY FFMPEG_avcodec_LIBRARY FFMPEG_avutil_LIBRARY)
        set(FFMPEG_LINK_LIBRARIES
            ${FFMPEG_swresample_LIBRARY}
            ${FFMPEG_avformat_LIBRARY}
            ${FFMPEG_avcodec_LIBRARY}
            ${FFMPEG_avutil_LIBRARY}
        )
        set(FFMPEG_avcodec_FOUND 1)
        set(FFMPEG_avformat_FOUND 1)
        set(FFMPEG_avutil_FOUND 1)
        set(FFMPEG_swresample_FOUND 1)
    endif()

    if(FFMPEG_avcodec_FOUND AND FFMPEG_avformat_FOUND AND FFMPEG_avutil_FOUND AND FFMPEG_swresample_FOUND)
        set(FFMPEG_FOUND 1)
    endif()

    if(FFMPEG_FOUND)
        message("== using FFMPEG (LGPL v2.1) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FFMPEG)

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_FFMPEG_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${FFMPEG_LINK_LIBRARIES})
        endif()

        list(APPEND SDL_MIXER_INCLUDE_PATHS ${FFMPEG_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c
            ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.h
        )

        if(DEFINED FLAG_C99)
            set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c"
                COMPILE_FLAGS ${FLAG_C99}
            )
        endif()

        appendPcmFormats("WMA;AAC;OPUS")
    else()
        message("-- skipping FFMPEG --")
    endif()
endif()
