cmake_minimum_required (VERSION 2.8.12...3.5)
project(SDLMixerX C CXX)

set(SDLMixerX_VERSION_MAJOR 2)
set(SDLMixerX_VERSION_MINOR 7)
set(SDLMixerX_VERSION_PATCH 0)
set(SDLMixerX_VERSION_TWEAK 0)
set(SDLMixerX_VERSION      "${SDLMixerX_VERSION_MAJOR}.${SDLMixerX_VERSION_MINOR}.${SDLMixerX_VERSION_PATCH}.${SDLMixerX_VERSION_TWEAK}")

set(SDL_REQUIRED_VERSION 2.0.7)

include(GNUInstallDirs)
include(FindPkgConfig)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

set(CMAKE_MODULE_PATH "${SDLMixerX_SOURCE_DIR}/cmake;${SDLMixerX_SOURCE_DIR}/cmake/find;${SDLMixerX_SOURCE_DIR}/src/codecs;${CMAKE_MODULE_PATH}")

include(mixerx_setup)
include(mixerx_libapi)
include(mixerx_macros)

include(CppNeedCheck)
include(WhichLicense)
include(FormatsSupported)

if(NOT EMSCRIPTEN
   AND NOT VITA
   AND NOT NINTENDO_3DS
   AND NOT NINTENDO_WII
   AND NOT NINTENDO_WIIU)

    set(SDL_MIXER_X_STATIC_ENABLED_BY_DEFAULT ON)
    set(SDL_MIXER_X_SHARED_ENABLED_BY_DEFAULT OFF)

    # When defined, respect CMake's BUILD_SHARED_LIBS setting
    if (BUILD_SHARED_LIBS)
        set(SDL_MIXER_X_SHARED_ENABLED_BY_DEFAULT ON)
        set(SDL_MIXER_X_STATIC_ENABLED_BY_DEFAULT OFF)
    endif()

    option(SDL_MIXER_X_STATIC   "Build static library of SDL Mixer X" ${SDL_MIXER_X_STATIC_ENABLED_BY_DEFAULT})
    option(SDL_MIXER_X_SHARED   "Build shared library of SDL Mixer X" ${SDL_MIXER_X_SHARED_ENABLED_BY_DEFAULT})
else()
    set(SDL_MIXER_X_STATIC ON)
    set(SDL_MIXER_X_SHARED OFF)
endif()

option(DOWNLOAD_AUDIO_CODECS_DEPENDENCY "Downloads and compiles AudioCodecs dependencies from git repository and build it" OFF)
option(AUDIO_CODECS_BUILD_LOCAL_SDL2 "Build SDL2 from local copy of 'AudioCodecs' repository" OFF)
set(AUDIO_CODECS_REPO_PATH "" CACHE PATH "Path to the AudioCodecs repository with sources of dependencies (if not specified, system libraries will be used)")
set(AUDIO_CODECS_INSTALL_PATH "" CACHE PATH "Path to installed AudioCodecs prefix (if not specified, system libraries will be used)")

option(MIXERX_ENABLE_LGPL   "Enable components with LGPL license" OFF)
option(MIXERX_ENABLE_GPL    "Enable using of components with GPL and LGPL licenses" OFF)

option(MIXERX_DISABLE_SIMD "Disable any SIMD optimizations as possible" OFF)

option(ENABLE_ADDRESS_SANITIZER "Enable the Address Sanitizer GCC feature" OFF)

set(MIXERX_ZLIB TRUE)
if(MIXERX_ENABLE_LGPL OR MIXERX_ENABLE_GPL)
    set(MIXERX_LGPL TRUE)
    if(MIXERX_ENABLE_GPL)
        set(MIXERX_GPL TRUE)
    endif()
endif()

option(USE_SYSTEM_SDL2   "Use SDL2 from system" OFF)

# ================= Optimizations and setup =======================

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(FLAG_C99 "-std=c99")
    # Turn on warnings and legacy C/C++ standards to support more compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-long-long -Wno-missing-field-initializers")
    if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu90")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++98")
    # Deny undefined symbols
    if(NOT APPLE)
        set(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        set(CMAKE_REQUIRED_FLAGS "-Wl,--no-undefined")
        check_c_compiler_flag("" HAVE_NO_UNDEFINED)
        set(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
        if(HAVE_NO_UNDEFINED)
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
            set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
        endif()
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-undefined,error" )
    endif()

# Supress the std::vector::insert() GCC change warning
    if(VITA)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVITA -DVITA=1 -fcompare-debug-second")
    endif()
    if(NINTENDO_3DS OR NINTENDO_WII OR NINTENDO_WIIU OR NINTENDO_SWITCH)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcompare-debug-second")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcompare-debug-second")
    endif()
endif()

if(MSVC)
    mixer_add_opt_flag("/utf-8" MSVC_UTF8)
endif()

if(MIXERX_DISABLE_SIMD)
    mixer_add_opt_flag("-mno-sse2" NO_SSE2)
    mixer_add_opt_flag("-mno-sse3" NO_SSE3)
    mixer_add_opt_flag("-mno-ssse3" NO_SSSE3)
    mixer_add_opt_flag("-mno-sse4.1" NO_SSE41)
    mixer_add_opt_flag("-mno-sse4.2" NO_SSE42)
    mixer_add_opt_flag("-mno-avx" NO_AVX)
    mixer_add_opt_flag("-mno-avx2" NO_AVX2)
    mixer_add_opt_flag("-mno-avx512f" NO_AVX512F)
endif()

# ================= Audio Codecs =======================

set(SDL_MIXER_DEFINITIONS)
set(SDL_MIXER_INCLUDE_PATHS)

if(NOT AUDIO_CODECS_REPO_PATH AND NOT AUDIO_CODECS_INSTALL_PATH)
    # Try to resolve sqlite dependency
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        # Download and configure AudioCodecs dependency
        set(SDL2_REPO_PATH ${CMAKE_INSTALL_PREFIX})
        set(SDL2_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include/SDL2)
        set(SDL2_LIBS_PATH ${CMAKE_INSTALL_PREFIX}/lib)
        set(BUILD_AUDIO_CODECS_NEEDED TRUE)
        message("== AudioCodecs will be built locally ==")
        include(cmake/DownloadAudioCodecs.cmake)
        set(AUDIO_CODECS_REPO_PATH ${AUDIO_CODECS_REPOSITORY_PATH})
        set(AUDIO_CODECS_INSTALL_PATH ${AUDIO_CODECS_INSTALL_DIR})
    endif()
endif()

if(AUDIO_CODECS_REPO_PATH OR AUDIO_CODECS_INSTALL_PATH)
    set(AUDIO_CODECS_PATH ${AUDIO_CODECS_REPO_PATH})
    set(AUDIO_CODECS_INSTALL_DIR ${AUDIO_CODECS_INSTALL_PATH})
    set(SDL2_REPO_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(SDL2_INCLUDE_PATH ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    link_directories(${AUDIO_CODECS_INSTALL_DIR}/lib)
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include ${SDL2_INCLUDE_PATH})
    message("AudioCodecs will be used. No needed extra libraries installed in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 0)
    set(CMAKE_FIND_ROOT_PATH_OLD ${CMAKE_FIND_ROOT_PATH})
    set(CMAKE_FIND_ROOT_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_STATIC})
else()
    message("AudioCodecs will NOT be used. Libraries will be recognized in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 1)
endif()

list(APPEND SDL_MIXER_DEFINITIONS
    -DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF
    -DPIC -D_REENTRANT -D_USE_MATH_DEFINES
)

if(UNIX AND NOT APPLE AND NOT HAIKU AND NOT EMSCRIPTEN)
    find_library(M_LIBRARY m)
    if(M_LIBRARY) # No need to link it by an absolute path
        set(M_LIBRARY m)
    endif()
    mark_as_advanced(M_LIBRARY)
else()
    set(M_LIBRARY "")
endif()

if(NOT MSVC)
    try_compile(STDCPP_FOUND
        ${CMAKE_BINARY_DIR}/compile_tests
        ${SDLMixerX_SOURCE_DIR}/cmake/tests/dummy.c
        LINK_LIBRARIES stdc++
    )
    try_compile(CPPABI_FOUND
        ${CMAKE_BINARY_DIR}/compile_tests
        ${SDLMixerX_SOURCE_DIR}/cmake/tests/dummy.c
        LINK_LIBRARIES c++abi
    )
    message("C++ Library: stdc++ [${STDCPP_FOUND}], c++abi [${CPPABI_FOUND}]")
    if(STDCPP_FOUND)
        set(STDCPP_LIBRARY stdc++)
    elseif(CPPABI_FOUND)
        set(STDCPP_LIBRARY c++abi)
    endif()
endif()

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

include(music_wav)

include(music_ogg_vorbis)
include(music_opus)
include(music_flac)
include(music_drflac)
include(music_wavpack)

include(music_mpg123)
include(music_drmp3)

# Differences between ModPlug and libXMP:
# - ModPlug-exclusive formats: AMS, DMF, DSM, MT2
# - XMP-exclusive formats: ABK, DIGI, DTM, EMOD, FLX, FNK, GDM, IMF, J2B, LIQ,
#                          M15, MFP, MGT, MMDC, MTN, RTM, SFX, SPM, STIM, STX,
#                          WOW
# - Other formats supported by both ModPlug and libXMP
include(music_modplug)
include(music_xmp)
include(music_gme)
include(music_ffmpeg)
include(music_pxtone)

include(music_cmd)

option(USE_MIDI            "Build with MIDI support" ON)
if(USE_MIDI)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_USE_MIDI)

    include(music_midi_adl)
    include(music_midi_opn)
    include(music_midi_edmidi)
    include(music_timidity)
    include(music_fluidlite)
    include(music_fluidsynth)
    include(music_nativemidi_alt_win32)
    include(music_nativemidi)

    include(music_midi_seq)

    if(NOT USE_MIDI_ADLMIDI AND
       NOT USE_MIDI_OPNMIDI AND
       NOT USE_MIDI_TIMIDITY AND
       NOT USE_MIDI_FLUIDSYNTH AND
       NOT USE_MIDI_NATIVE)
        message(FATAL_ERROR "To have MIDI support you must enable at least one MIDI sequencer library")
    endif()
else()
    message("== !!! MIDI SUPORT HAS BEEN DISABLED !!! ==")
endif()

include(music_ogg)

include(libSDL2)

if(LIBMATH_NEEDED AND NOT MSVC AND M_LIBRARY)
    list(APPEND SDLMixerX_LINK_LIBS ${M_LIBRARY})
endif()

# It's the solid-built DLL of MixerX which is a mix of statically linked SDL2 and codec libraries
if(WIN32 AND NOT EMSCRIPTEN)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        message("== SDL Mixer X will be built as solid module that is ready for MS-VB6 binding! ==")
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/VB6_Wrapper/vb6_sdl_binds.c)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS "${LIBSDL2CUSTOM_LIB}")
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2d)
        message("== VB6 binding: SDL2 name to link: ${AUDIOCODECS_SDL2_LIB_NAME}")
        list(APPEND SDLMixerX_LINK_LIBS
                    ${AUDIOCODECS_SDL2_LIB_NAME}
                    uuid winmm ole32 imm32 version oleaut32 user32 gdi32 setupapi)

        if(NOT MSVC)
            list(APPEND SDLMixerX_LINK_LIBS stdc++ gcc pthread)
            unset(STDCPP_NEEDED) # stdc++ has been handled, no need to link it twice
        endif()
    endif()
endif()

if(APPLE)
    find_library(APPLE_CORE_AUDIO CoreAudio)
    find_library(APPLE_IOKIT IOKit)
    find_library(APPLE_COCOA Cocoa)
    find_library(APPLE_AUDIOTOOLBOX AudioToolbox)
    find_library(APPLE_CORE_VIDEO CoreVideo)
    find_library(APPLE_CORE_FOUNDATION CoreFoundation)
    find_library(APPLE_CARBON Carbon)
    find_library(APPLE_FORCE_FEEDBACK ForceFeedback)
    check_library_exists(iconv iconv_open "" HAVE_LIBICONV)
    if(HAVE_LIBICONV)
        set(ICONV_LIB iconv)
    endif()

    list(APPEND SDLMixerX_LINK_LIBS
            ${APPLE_CORE_AUDIO}
            ${APPLE_COCOA}
            ${APPLE_CORE_VIDEO}
            ${APPLE_CORE_FOUNDATION}
            ${APPLE_FORCE_FEEDBACK}
            ${APPLE_IOKIT}
            ${APPLE_CARBON}
            ${APPLE_AUDIOTOOLBOX}
            ${ICONV_LIB})
    list(APPEND SDLMixerX_LINK_LIBS objc)
endif()

if(ANDROID AND USE_XMP AND NOT USE_XMP_DYNAMIC)
    list(APPEND SDLMixerX_LINK_LIBS log)
endif()

# When statically linking some libraries, it's need to link  also
if(STDCPP_NEEDED AND NOT MSVC)
    message("Note: ${STDCPP_LIBRARY} will be linked!")
    list(APPEND SDLMixerX_LINK_LIBS ${STDCPP_LIBRARY})
endif()

list(APPEND SDLMixerX_SOURCES
    ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer.h
    ${SDLMixerX_SOURCE_DIR}/src/effect_position.c
    ${SDLMixerX_SOURCE_DIR}/src/effects_internal.c ${SDLMixerX_SOURCE_DIR}/src/effects_internal.h
    ${SDLMixerX_SOURCE_DIR}/src/effect_stereoreverse.c
    ${SDLMixerX_SOURCE_DIR}/src/mixer.c ${SDLMixerX_SOURCE_DIR}/src/mixer.h
    ${SDLMixerX_SOURCE_DIR}/src/music.c ${SDLMixerX_SOURCE_DIR}/src/music.h
    ${SDLMixerX_SOURCE_DIR}/src/mixer_x_deprecated.c
    ${SDLMixerX_SOURCE_DIR}/src/utils.c ${SDLMixerX_SOURCE_DIR}/src/utils.h
    ${SDLMixerX_SOURCE_DIR}/src/codecs/mp3utils.c
)

#file(GLOB SDLMixerX_SOURCES ${SDLMixerX_SOURCES})

if(SDL_MIXER_X_STATIC AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_Static STATIC ${SDLMixerX_SOURCES})
    if(CMAKE_STATIC_LIBRARY_PREFIX STREQUAL "" AND CMAKE_STATIC_LIBRARY_SUFFIX STREQUAL ".lib")
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext-static)
    else()
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext)
    endif()

    target_link_libraries(SDL2_mixer_ext_Static PUBLIC ${SDLMixerX_LINK_LIBS})
    target_compile_definitions(SDL2_mixer_ext_Static PRIVATE ${SDL_MIXER_DEFINITIONS})
    target_include_directories(SDL2_mixer_ext_Static PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    target_include_directories(SDL2_mixer_ext_Static PUBLIC
        $<BUILD_INTERFACE:${SDLMixerX_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>
    )

    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext_Static AudioCodecs)
    endif()

    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext_Static
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
        )
    endif()

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(SDL2_mixer_ext_Static PRIVATE -fsanitize=address)
    endif()

    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext_Static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${SDLMixerX_SOURCE_DIR}/include/SDL_mixer.h"
                    "${AUDIO_CODECS_INSTALL_DIR}/include/SDL2/SDL_mixer_ext.h")
    endif()
endif()

if(WIN32)
    list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/version.rc)
endif()

if(SDL_MIXER_X_SHARED AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext SHARED ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext PROPERTIES
        VERSION ${SDLMixerX_VERSION}
        SOVERSION ${SDLMixerX_VERSION_MAJOR}
    )
    target_link_libraries(SDL2_mixer_ext PRIVATE ${SDLMixerX_LINK_LIBS})
    target_compile_definitions(SDL2_mixer_ext PRIVATE ${SDL_MIXER_DEFINITIONS} -DDLL_EXPORT)
    target_include_directories(SDL2_mixer_ext PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    target_include_directories(SDL2_mixer_ext PUBLIC
        $<BUILD_INTERFACE:${SDLMixerX_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SDL2>
    )

    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext AudioCodecs)
    endif()

    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
        )
    endif()

    if(ENABLE_ADDRESS_SANITIZER)
        target_compile_options(SDL2_mixer_ext PRIVATE -fsanitize=address)
        target_link_options(SDL2_mixer_ext PRIVATE -fsanitize=address)
    endif()

    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${SDLMixerX_SOURCE_DIR}/include/SDL_mixer.h"
                    "${AUDIO_CODECS_INSTALL_DIR}/include/SDL2/SDL_mixer_ext.h")
    endif()
endif()

if(BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_VB6 MODULE ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES OUTPUT_NAME SDL2MixerVB)
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES PREFIX "")
    target_compile_definitions(SDL2_mixer_ext_VB6 PRIVATE ${SDL_MIXER_DEFINITIONS} -DFORCE_STDCALLS -DDLL_EXPORT)
    target_link_libraries(SDL2_mixer_ext_VB6 ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )

    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )

    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES LINK_FLAGS
        "-Wl,--add-stdcall-alias -static-libgcc -static-libstdc++ -static -lpthread")

    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext_VB6 AudioCodecs)
    endif()

    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext_VB6
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
        )
    endif()

    add_custom_command(
        TARGET SDL2_mixer_ext_VB6 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/VB6_Wrapper/modSDL2_mixer_ext_vb6.bas"
                "${CMAKE_BINARY_DIR}/sdl-mixer-vb6")
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES AND NOT BUILD_AS_VB6_BINDING)
    add_subdirectory(examples)
endif()

if(SDL_MIXER_X_STATIC AND NOT BUILD_AS_VB6_BINDING)
    install(TARGETS SDL2_mixer_ext_Static
            EXPORT SDL2MixerExtStaticTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    install(EXPORT SDL2MixerExtStaticTargets
            FILE SDL2_mixer_ext-static-targets.cmake
            NAMESPACE SDL2_mixer_ext::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_mixer_ext")
endif()

if(SDL_MIXER_X_SHARED AND NOT BUILD_AS_VB6_BINDING)
    install(TARGETS SDL2_mixer_ext
            EXPORT SDL2MixerExtSharedTargets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

    install(EXPORT SDL2MixerExtSharedTargets
            FILE SDL2_mixer_ext-shared-targets.cmake
            NAMESPACE SDL2_mixer_ext::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_mixer_ext")
endif()


if(BUILD_AS_VB6_BINDING)
    install(TARGETS SDL2_mixer_ext_VB6
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()

if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
    if(EXISTS ${AUDIO_CODECS_INSTALL_DIR}/bin)
        install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/bin
                DESTINATION .)
    endif()
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/lib
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/include
            DESTINATION .)
endif()

install(FILES
    include/SDL_mixer.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2"
    RENAME SDL_mixer_ext.h
)

include(CMakePackageConfigHelpers)
configure_package_config_file(SDL2_mixer_extConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer_extConfig.cmake"
    PATH_VARS CMAKE_INSTALL_PREFIX CMAKE_INSTALL_FULL_BINDIR CMAKE_INSTALL_FULL_INCLUDEDIR CMAKE_INSTALL_FULL_LIBDIR
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_mixer_ext"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SDL2_mixer_extConfig.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SDL2_mixer_ext"
)

if(NOT ANDROID)
    set(SDL_VERSION ${SDL_REQUIRED_VERSION})
    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")
    set(bindir "\${exec_prefix}/bin")
    set(includedir "\${prefix}/include")
    set(VERSION ${SDLMixerX_VERSION})

    configure_file("${SDLMixerX_SOURCE_DIR}/SDL2_mixer_ext.pc.in"
        "${SDLMixerX_BINARY_DIR}/SDL2_mixer_ext.pc" @ONLY)
    install(FILES "${SDLMixerX_BINARY_DIR}/SDL2_mixer_ext.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

# === Unit tests ====
option(WITH_UNIT_TESTS "Enable unit testing" OFF)
if(WITH_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


function(print_sumary _libName _isEnabled _wasFound _whatFound _lic_allow _lic_name)
    if(${_isEnabled})
        if(${_wasFound})
            message("  ${_libName} found:")
            message("  -- ${${_whatFound}}")
        elseif(NOT ${_lic_allow})
            message("  ${_libName} is disabled because it's ${_lic_name}-licensed component")
            message("  -- <disabled>")
        else()
            message("  ${_libName} is missing, will be disabled")
            message("  -- <missing>")
        endif()
    else()
        message("  ${_libName} is disabled")
        message("  -- <disabled>")
    endif()
endfunction()

set(STATS_BUILTIN_LIB "<built-in>")

message("==== SDL Mixer X modules ====")
if(LIBOGG_NEEDED)
    print_sumary("OGG" LIBOGG_NEEDED OGG_FOUND OGG_LIBRARIES MIXERX_ZLIB "-")
endif()
if(USE_OGG_VORBIS_STB)
    print_sumary("stb_vorbis" USE_OGG_VORBIS_STB TRUE STATS_BUILTIN_LIB MIXERX_ZLIB "-")
elseif(USE_OGG_VORBIS_TREMOR)
    print_sumary("Tremor" USE_OGG_VORBIS_TREMOR Tremor_FOUND Tremor_LIBRARIES MIXERX_ZLIB "-")
else()
    print_sumary("Vorbis" USE_OGG_VORBIS Vorbis_FOUND Vorbis_LIBRARIES MIXERX_ZLIB "-")
endif()
print_sumary("OPUS" USE_OPUS Opus_FOUND Opus_LIBRARIES MIXERX_ZLIB "-")
print_sumary("DRFLAC" USE_DRFLAC TRUE STATS_BUILTIN_LIB MIXERX_ZLIB "-")
print_sumary("FLAC" USE_FLAC FLAC_FOUND FLAC_LIBRARIES MIXERX_ZLIB "-")
print_sumary("DRMP3" USE_MP3_DRMP3 TRUE STATS_BUILTIN_LIB MIXERX_ZLIB "-")
print_sumary("MPG123" USE_MP3_MPG123 MPG123_FOUND MPG123_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("WavPack" USE_WAVPACK WavPack_FOUND WavPack_LIBRARIES MIXERX_ZLIB "-")
print_sumary("ModPlug" USE_MODPLUG ModPlug_FOUND ModPlug_LIBRARIES MIXERX_ZLIB "-")
print_sumary("libXMP" USE_XMP XMP_FOUND XMP_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("GME" USE_GME GME_FOUND GME_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("FFMPEG" USE_FFMPEG FFMPEG_FOUND FFMPEG_LINK_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("PXTONE" USE_PXTONE TRUE STATS_BUILTIN_LIB MIXERX_ZLIB "PXTONE")
print_sumary("libADLMIDI" USE_MIDI_ADLMIDI ADLMIDI_FOUND ADLMIDI_LIBRARIES MIXERX_GPL "GPLv3")
print_sumary("libOPNMIDI" USE_MIDI_OPNMIDI OPNMIDI_FOUND OPNMIDI_LIBRARIES MIXERX_GPL "GPLv3")
print_sumary("libEDMIDI" USE_MIDI_EDMIDI EDMIDI_FOUND EDMIDI_LIBRARIES MIXERX_ZLIB "-")
print_sumary("FluidSynth" USE_MIDI_FLUIDSYNTH FluidSynth_FOUND FluidSynth_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("FluidLite" USE_MIDI_FLUIDLITE FluidLite_FOUND FluidLite_LIBRARIES MIXERX_LGPL "LGPLv2.1")
print_sumary("Timidity-SDL" USE_MIDI_TIMIDITY TIMIDITYSDL_FOUND TIMIDITYSDL_LIBRARIES MIXERX_ZLIB "-")

printFormats()

printLicense()
if(NOT MIXERX_LGPL)
    message("LGPL-licensed components are DISABLED. If you want to enable them, set the -DMIXERX_ENABLE_LGPL=ON option")
endif()
if(NOT MIXERX_GPL)
    message("GPL-licensed components are DISABLED. If you want to enable them, set the -DMIXERX_ENABLE_GPL=ON option")
endif()
if(NOT MIXERX_GPL)
    message("===============================================================================")
endif()
