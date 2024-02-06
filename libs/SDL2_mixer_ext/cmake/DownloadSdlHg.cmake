
# This file downloads, configures and build SDL2 from official HG dependencies package.
#
# Output Variables:
# SDL2_INSTALL_DIR			    The install directory
# SDL2_REPOSITORY_PATH  		The reposotory directory

# Require ExternalProject!
include(ExternalProject)

# Posttible Input Vars:
# <None>

# SET OUTPUT VARS
# set(SDL2_INSTALL_DIR ${CMAKE_BINARY_DIR}/external/install)
set(SDL2_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
set(SDL2_REPOSITORY_PATH ${SDL2_INSTALL_DIR})

ExternalProject_Add(
    SDL2HG
    PREFIX ${CMAKE_BINARY_DIR}/external/SDL2
    URL https://hg.libsdl.org/SDL/archive/default.tar.bz2
    CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${SDL2_INSTALL_DIR}" -DSNDIO=OFF
)
