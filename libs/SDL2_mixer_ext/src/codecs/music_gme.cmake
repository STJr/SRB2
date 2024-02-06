option(USE_GME             "Build with Game Music Emulators library (LGPL)" ON)
if(USE_GME AND MIXERX_LGPL)
    option(USE_GME_DYNAMIC "Use dynamical loading of Game Music Emulators library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(GME QUIET)
        message("GME: [${GME_FOUND}] ${GME_INCLUDE_DIRS} ${GME_LIBRARIES}")
        if(USE_GME_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_DYNAMIC=\"${GME_DYNAMIC_LIBRARY}\")
            message("Dynamic GME: ${GME_DYNAMIC_LIBRARY}")
        endif()

        if(GME_FOUND)
            cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/gme.c
                ""
                ${GME_INCLUDE_DIRS}
                "${GME_LIBRARIES};${M_LIBRARY}"
                STDCPP_NEEDED
            )

            try_compile(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/gme_has_gme_set_autoload_playback_limit.c
                LINK_LIBRARIES ${GME_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${GME_INCLUDE_DIRS}"
                OUTPUT_VARIABLE GME_TEST_RESULT
            )
            message("gme_set_autoload_playback_limit() compile test result: ${GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT}")

            try_compile(GME_HAS_DISABLE_ECHO
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/gme_has_gme_disable_echo.c
                LINK_LIBRARIES ${GME_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${GME_INCLUDE_DIRS}"
                OUTPUT_VARIABLE GME_TEST_RESULT
            )
            message("gme_disable_echo() compile test result: ${GME_HAS_DISABLE_ECHO}")
        endif()

    else()
        option(USE_SYSTEM_ZLIB "Use ZLib from the system" OFF)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme${MIX_DEBUG_SUFFIX})
            if(USE_SYSTEM_ZLIB)
                find_package(ZLIB REQUIRED)
                set(LIBZLIB_LIB ${ZLIB_LIBRARIES})
            else()
                set(LIBZLIB_LIB zlib${MIX_DEBUG_SUFFIX})
            endif()
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            if(USE_SYSTEM_ZLIB)
                find_package(ZLIB REQUIRED)
                set(LIBZLIB_LIB ${ZLIB_LIBRARIES})
            else()
                find_library(LIBZLIB_LIB NAMES zlib
                             HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            endif()
        endif()

        mark_as_advanced(LIBGME_LIB LIBZLIB_LIB)
        set(GME_LIBRARIES ${LIBGME_LIB} ${LIBZLIB_LIB})
        set(GME_FOUND 1)
        set(STDCPP_NEEDED 1) # Statically linking GME which is C++ library
        set(LIBMATH_NEEDED 1)
        set(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT TRUE)
        set(GME_HAS_DISABLE_ECHO TRUE)
        set(GME_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_PATH}/include/gme
            ${AUDIO_CODECS_INSTALL_PATH}/include
            ${AUDIO_CODECS_PATH}/libgme/include
            ${AUDIO_CODECS_PATH}/zlib/include
        )
    endif()

    if(GME_FOUND)
        message("== using GME (LGPLv2.1+ or GPLv2+ if MAME YM2612 emulator was used) ==")
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            setLicense(LICENSE_GPL_2p)  # at AudioCodecs set, the MAME YM2612 emualtor is enabled by default
        else()
            setLicense(LICENSE_LGPL_2_1p)
        endif()

        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_GME)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${GME_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_GME_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${GME_LIBRARIES})
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.c
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.h
        )

        if(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT)
        endif()

        if(GME_HAS_DISABLE_ECHO)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_HAS_DISABLE_ECHO)
        endif()

        appendChiptuneFormats("AY;GBS;GYM;HES;KSS;NSF;NSFE;SAP;SPC;VGM;VGZ")
    else()
        message("-- skipping GME --")
    endif()
endif()
