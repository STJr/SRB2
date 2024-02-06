cmake_minimum_required (VERSION 2.8.12)
project(SDLMixerMusPlayer)

include(GNUInstallDirs)
include(NoPie)

# Version
include(version.cmake)

set(CMAKE_CXX_STANDARD 11)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

if(POLICY CMP0043)
    cmake_policy(SET CMP0043 NEW)
endif()

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(USE_OFFICIAL_SDL_MIXER "Compile player in compatibility with official SDL Mixer variant" OFF)

if(USE_OFFICIAL_SDL_MIXER)
    set(SDL_MIXER_X_LIB SDL2_mixer)
else()
    add_definitions(-DUSE_SDL_MIXER_X)
    set(SDL_MIXER_X_LIB libSDLMixerX)
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

if(APPLE)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CARBON_LIBRARY Carbon)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FORCEFEEDBAK_LIBRARY ForceFeedback)
    find_library(METAL_LIBRARY Metal)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    add_definitions(-DQ_OS_MACX) # Workaround for MOC
endif()

# The Qt5Widgets_INCLUDES also includes the include directories for
# dependencies QtCore and QtGui
include_directories(
    ${Qt5Widgets_INCLUDES}
    ${Qt5Network_INCLUDES}
    ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
    ${SDL2_INCLUDE_DIRS}
)

link_directories(
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Network_LIBRARIES}
)

if(APPLE) # To use XCode-built SDL Mixer X
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_lower)
    if(CMAKE_BUILD_TYPE_lower STREQUAL "release")
        link_directories(
            ${USE_SDL_MIXER_X_PREFIX}/lib/Release)
    else()
        link_directories(
            ${USE_SDL_MIXER_X_PREFIX}/lib/Debug)
    endif()
endif()

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
add_definitions(${Qt5Widgets_DEFINITIONS} ${Qt5Network_DEFINITIONS})

# Executables fail to build with Qt 5 in the default configuration
# without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${Qt5Network_EXECUTABLE_COMPILE_FLAGS}")

set(SDLMixer_MusPlay_SRC)
include(sources.cmake)

qt5_add_resources(RESOURCE_ADDED ${SDLMixer_MusPlay_RESOURCE})
qt5_wrap_ui(UIS_HDRS ${SDLMixer_MusPlay_UI})

list(APPEND SDLMixer_MusPlay_SRC
    ${UIS_HDRS}
    ${RESOURCE_ADDED}
)

add_executable(sdlmixer_musplay
    ${SDLMixer_MusPlay_SRC}
)
pge_set_nopie(sdlmixer_musplay)

set(PGE_MUSPLAY_LINK_LIBS)
list(APPEND PGE_MUSPLAY_LINK_LIBS
    ${SDL_MIXER_X_LIB}
)

if(WIN32)
    target_link_libraries(sdlmixer_musplay version ${PGE_MUSPLAY_LINK_LIBS} comctl32)
elseif(APPLE)
    target_link_libraries(sdlmixer_musplay ${PGE_MUSPLAY_LINK_LIBS} iconv
        ${COREAUDIO_LIBRARY} ${COREVIDEO_LIBRARY} ${COCOA_LIBRARY} ${CARBON_LIBRARY}
        ${FORCEFEEDBAK_LIBRARY} ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY} ${AUDIOUNIT_LIBRARY}
        ${METAL_LIBRARY}
    )
else()
    target_link_libraries(sdlmixer_musplay ${PGE_MUSPLAY_LINK_LIBS})
endif()

if(ENABLE_ADDRESS_SANITIZER)
    target_compile_options(sdlmixer_musplay PRIVATE -fsanitize=address)
    target_link_options(sdlmixer_musplay PRIVATE -fsanitize=address)
endif()

qt5_use_modules(sdlmixer_musplay Widgets Network)

install(TARGETS sdlmixer_musplay
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
