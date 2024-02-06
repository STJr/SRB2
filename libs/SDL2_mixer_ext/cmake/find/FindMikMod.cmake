# - Try to find libMikMod
# Once done this will define
#  MikMod_FOUND - System has libMikMod
#  MikMod_INCLUDE_DIRS - The libMikMod include directories
#  MikMod_LIBRARIES - The libraries needed to use libMikMod

find_path(MikMod_INCLUDE_DIR "mikmod.h")
find_library(MikMod_LIBRARY NAMES mikmod)

if(MikMod_INCLUDE_DIR AND MikMod_LIBRARY)
    if(APPLE)
        find_library(MikMod_DYNAMIC_LIBRARY NAMES "mikmod"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(MikMod_DYNAMIC_LIBRARY NAMES "mikmod" PATH_SUFFIXES ".dll")
    else()
        find_library(MikMod_DYNAMIC_LIBRARY NAMES "mikmod" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MikMod_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MikMod  DEFAULT_MSG
                                  MikMod_LIBRARY MikMod_INCLUDE_DIR)

mark_as_advanced(MikMod_INCLUDE_DIR MikMod_LIBRARY)

set(MikMod_LIBRARIES ${MikMod_LIBRARY} )
set(MikMod_INCLUDE_DIRS ${MikMod_INCLUDE_DIR} )

