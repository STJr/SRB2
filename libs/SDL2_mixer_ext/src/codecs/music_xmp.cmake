option(USE_XMP         "Build with XMP library (LGPL)" ON)
if(USE_XMP)
    option(USE_XMP_DYNAMIC "Use dynamical loading of XMP library" OFF)
    option(USE_XMP_LITE "Use the lite version of XMP library" OFF)
    if(VITA OR NINTENDO_DS OR NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_SWITCH)
        set(USE_XMP_MEMORY_ONLY_DEFAULT ON)
    else()
        set(USE_XMP_MEMORY_ONLY_DEFAULT OFF)
    endif()
    option(USE_XMP_MEMORY_ONLY "Force the memory-based file loading method (preferred for low-end hardware)" ${USE_XMP_MEMORY_ONLY_DEFAULT})

    if(NOT MIXERX_LGPL AND NOT MIXERX_GPL)
        set(USE_XMP_LITE_ENFORCE TRUE)
    endif()

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        if(USE_XMP_LITE OR USE_XMP_LITE_ENFORCE)
            find_package(XMPLite)
            message("XMP-Lite: [${XMPLITE_FOUND}] ${XMPLITE_INCLUDE_DIRS} ${XMPLITE_LIBRARIES}")
            if(USE_XMP_DYNAMIC)
                list(APPEND SDL_MIXER_DEFINITIONS -DXMP_DYNAMIC=\"${XMPLITE_DYNAMIC_LIBRARY}\")
                message("Dynamic XMP-Lite: ${XMPLITE_DYNAMIC_LIBRARY}")
            endif()
            set(XMP_INCLUDE_DIRS ${XMPLITE_INCLUDE_DIRS})
            set(XMP_LIBRARIES ${XMPLITE_LIBRARIES})
            set(XMP_FOUND ${XMPLITE_FOUND})
        else()
            find_package(XMP)
            message("XMP: [${XMP_FOUND}] ${XMP_INCLUDE_DIRS} ${XMP_LIBRARIES}")
            if(USE_XMP_DYNAMIC)
                list(APPEND SDL_MIXER_DEFINITIONS -DXMP_DYNAMIC=\"${XMP_DYNAMIC_LIBRARY}\")
                message("Dynamic XMP: ${XMP_DYNAMIC_LIBRARY}")
            endif()
        endif()

    elseif(NOT USE_XMP_LITE AND NOT USE_XMP_LITE_ENFORCE)
        if(MSVC)
            set(xmplib libxmp-static)
        else()
            set(xmplib xmp)
        endif()

        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(XMP_LIBRARIES ${xmplib}${MIX_DEBUG_SUFFIX})
        else()
            find_library(XMP_LIBRARIES NAMES ${xmplib} libxmp
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(XMP_FOUND 1)
        set(XMP_INCLUDE_DIRS
            ${AUDIO_CODECS_PATH}/libxmp/include
            ${AUDIO_CODECS_INSTALL_PATH}/include/xmp
        )
    endif()

    if(XMP_FOUND)
        if(NOT USE_XMP_DYNAMIC)
            if(USE_XMP_LITE OR USE_XMP_LITE_ENFORCE)
                message("== using libXMP-Lite (MIT) ==")
                setLicense(LICENSE_MIT)
            else()
                message("== using libXMP (LGPL v2.1+) ==")
                setLicense(LICENSE_LGPL_2_1p)
            endif()
        endif()

        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MOD_XMP)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${XMP_INCLUDE_DIRS})

        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_XMP_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${XMP_LIBRARIES})
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_xmp.c
            ${CMAKE_CURRENT_LIST_DIR}/music_xmp.h
        )

        if(USE_XMP_MEMORY_ONLY)
            list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_XMP_MEMORY_ONLY)
        endif()

        if(WIN32) # Workaround to fix the broken linking of static library, caused by abuse of `dllimport()`
            list(APPEND SDL_MIXER_DEFINITIONS -DBUILDING_STATIC -DLIBXMP_STATIC)
        endif()

        if(USE_XMP_LITE OR USE_XMP_LITE_ENFORCE)
            appendTrackerFormats("MOD;XM;S3M;IT")
        else()
            appendTrackerFormats("MOD;M15;FLX;WOW;S3M;DBM;DIGI;EMOD;MED;MTN;OKT;SFX;DTM;MGT;
                                  669;FAR;FNK;IMF(Imago Orpheus);IT;LIQ;MDL;MTM;PTM;RTM;S3M;STM;ULT;XM;
                                  AMF;GDM;STX;
                                  ABK;AMF;PSM;J2B;MFP;SMP;MMDC;STIM;UMX")
        endif()
    else()
        message("-- skipping XMP --")
    endif()
endif()
