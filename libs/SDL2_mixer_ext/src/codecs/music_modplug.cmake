option(USE_MODPLUG         "Build with ModPlug library" ON)
if(USE_MODPLUG)
    option(USE_MODPLUG_DYNAMIC "Use dynamical loading of ModPlug" OFF)
    option(USE_MODPLUG_STATIC "Use linking with a static ModPlug" ON)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(ModPlug QUIET)
        message("ModPlug: [${ModPlug_FOUND}] ${ModPlug_INCLUDE_DIRS} ${ModPlug_LIBRARIES}")
        if(USE_MODPLUG_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DMODPLUG_DYNAMIC=\"${ModPlug_DYNAMIC_LIBRARY}\")
            message("Dynamic ModPlug: ${ModPlug_DYNAMIC_LIBRARY}")
        endif()

        if(ModPlug_FOUND)
            cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/modplug.c
                "-DMODPLUG_STATIC"
                ${ModPlug_INCLUDE_DIRS}
                "${ModPlug_LIBRARIES};${M_LIBRARY}"
                STDCPP_NEEDED
            )

            try_compile(MODPLUG_HAS_TELL
                ${CMAKE_BINARY_DIR}/compile_tests
                ${SDLMixerX_SOURCE_DIR}/cmake/tests/modplug_tell.c
                COMPILE_DEFINITIONS "-DMODPLUG_STATIC"
                LINK_LIBRARIES ${ModPlug_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
                CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${ModPlug_INCLUDE_DIRS}"
                OUTPUT_VARIABLE MODPLUG_TEST_RESULT
            )
            message("ModPlug_Tell() compile test result: ${MODPLUG_HAS_TELL}")
        endif()

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(ModPlug_LIBRARIES modplug${MIX_DEBUG_SUFFIX})
        else()
            find_library(ModPlug_LIBRARIES NAMES modplug
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(ModPlug_FOUND 1)
        set(STDCPP_NEEDED 1) # Statically linking ModPlug which is C++ library
        set(MODPLUG_HAS_TELL TRUE)
        set(ModPlug_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libmodplug/include")
    endif()

    if(ModPlug_FOUND)
        message("== using libModPlug (Public Domain) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MOD_MODPLUG -DMODPLUG_STATIC)
        if(USE_MODPLUG_STATIC)
            list(APPEND SDL_MIXER_DEFINITIONS)
        endif()
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MODPLUG_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${ModPlug_LIBRARIES})
        endif()
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${ModPlug_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_modplug.c
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_modplug.h
        )
        if(MODPLUG_HAS_TELL)
            list(APPEND SDL_MIXER_DEFINITIONS -DMODPLUG_HAS_TELL)
        endif()
        appendTrackerFormats("MOD;S3M;XM;IT;669;AMF;AMS;DBM;DMF;DSM;FAR;
                              MDL;MED;MTM;OKT;PTM;STM;ULT;UMX;MT2;PSM")
    else()
        message("-- skipping libModPlug --")
    endif()
endif()
