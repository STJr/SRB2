option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    option(USE_OGG_VORBIS_DYNAMIC "Use dynamical loading of OGG Vorbis" OFF)
    option(USE_OGG_VORBIS_TREMOR "Use the Tremor - an integer-only OGG Vorbis implementation" OFF)
    option(USE_OGG_VORBIS_STB    "Use the stb_vorbis - the portable minimalistic OGG Vorbis implementation" ON)

    if(NOT USE_OGG_VORBIS_STB)
        if(USE_SYSTEM_AUDIO_LIBRARIES)
            if(USE_OGG_VORBIS_TREMOR)
                find_package(Tremor QUIET)
                message("Tremor: [${Tremor_FOUND}] ${Tremor_INCLUDE_DIRS} ${Tremor_LIBRARIES}")
            else()
                find_package(Vorbis QUIET)
                message("Vorbis: [${Vorbis_FOUND}] ${Vorbis_INCLUDE_DIRS} ${Vorbis_LIBRARIES}")
            endif()

            if(USE_OGG_VORBIS_DYNAMIC)
                if(USE_OGG_VORBIS_TREMOR)
                    list(APPEND SDL_MIXER_DEFINITIONS -DOGG_DYNAMIC=\"${VorbisIDec_DYNAMIC_LIBRARY}\")
                    message("Dynamic tremor: ${VorbisIDec_DYNAMIC_LIBRARY}")
                else()
                    list(APPEND SDL_MIXER_DEFINITIONS -DOGG_DYNAMIC=\"${VorbisFile_DYNAMIC_LIBRARY}\")
                    message("Dynamic vorbis: ${VorbisFile_DYNAMIC_LIBRARY}")
                endif()
            endif()

        else()
            if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
                if(USE_OGG_VORBIS_TREMOR)
                    set(Tremor_LIBRARIES vorbisidec${MIX_DEBUG_SUFFIX})
                else()
                    set(Vorbis_LIBRARIES vorbisfile${MIX_DEBUG_SUFFIX} vorbis${MIX_DEBUG_SUFFIX})
                endif()
            else()
                if(USE_OGG_VORBIS_TREMOR)
                    find_library(LIBVORBISIDEC_LIB NAMES vorbisidec
                                 HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
                    set(Tremor_LIBRARIES ${LIBVORBISIDEC_LIB})
                    mark_as_advanced(LIBVORBISIDEC_LIB)
                else()
                    find_library(LIBVORBISFILE_LIB NAMES vorbisfile
                                 HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
                    find_library(LIBVORBIS_LIB NAMES vorbis
                                 HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
                    set(Vorbis_LIBRARIES ${LIBVORBISFILE_LIB} ${LIBVORBIS_LIB})
                    mark_as_advanced(LIBVORBISFILE_LIB LIBVORBIS_LIB)
                endif()
            endif()

            if(USE_OGG_VORBIS_TREMOR)
                set(Tremor_FOUND 1)
            else()
                set(Vorbis_FOUND 1)
            endif()

            set(Vorbis_INCLUDE_DIRS
                ${AUDIO_CODECS_INSTALL_DIR}/include/ogg
                ${AUDIO_CODECS_PATH}/libogg/include
            )

            if(USE_OGG_VORBIS_TREMOR)
                list(APPEND Vorbis_INCLUDE_DIRS ${AUDIO_CODECS_INSTALL_DIR}/include/tremor)
            else()
                list(APPEND Vorbis_INCLUDE_DIRS
                    ${AUDIO_CODECS_INSTALL_DIR}/include/vorbis
                    ${AUDIO_CODECS_PATH}/libvorbis/include
                )
            endif()
        endif()

        if(USE_OGG_VORBIS_TREMOR AND Tremor_FOUND)
            list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_OGG -DOGG_USE_TREMOR)
            message("== using Tremor (BSD 3-Clause) ==")
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${Tremor_INCLUDE_DIRS})
            if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OGG_VORBIS_DYNAMIC)
                list(APPEND SDLMixerX_LINK_LIBS ${Tremor_LIBRARIES})
                set(LIBOGG_NEEDED ON)
                set(LIBMATH_NEEDED 1)
            endif()
        elseif(Vorbis_FOUND)
            message("== using Vorbis (BSD 3-Clause) ==")
            list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_OGG)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${Vorbis_INCLUDE_DIRS})
            if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OGG_VORBIS_DYNAMIC)
                list(APPEND SDLMixerX_LINK_LIBS ${Vorbis_LIBRARIES})
                set(LIBOGG_NEEDED ON)
                set(LIBMATH_NEEDED 1)
            endif()
        endif()

        if(NOT Vorbis_FOUND AND NOT Tremor_FOUND)
            message("-- skipping Vorbis/Tremor --")
        endif()
    else()
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_OGG -DOGG_USE_STB)
        set(LIBMATH_NEEDED 1)
        message("== using STB-Vorbis (BSD 3-Clause) ==")
    endif()

    if(Vorbis_FOUND OR Tremor_FOUND)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_ogg.c
            ${CMAKE_CURRENT_LIST_DIR}/music_ogg.h
        )
        appendPcmFormats("OGG Vorbis")
    else(USE_OGG_VORBIS_STB)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_ogg_stb.c
            ${CMAKE_CURRENT_LIST_DIR}/stb_vorbis/stb_vorbis.h
            ${CMAKE_CURRENT_LIST_DIR}/music_ogg.h
        )
        if(DEFINED FLAG_C99)
            set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/music_ogg_stb.c"
                COMPILE_FLAGS ${FLAG_C99}
            )
        endif()
        appendPcmFormats("OGG Voribs")
    endif()

endif()
