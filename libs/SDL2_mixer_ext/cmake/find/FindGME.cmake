# - Try to find libGME
# Once done this will define
#  GME_FOUND - System has libGME
#  GME_INCLUDE_DIRS - The libGME include directories
#  GME_LIBRARIES - The libraries needed to use libGME

find_path(GME_INCLUDE_DIR "gme.h" PATH_SUFFIXES gme)
find_library(GME_LIBRARY NAMES gme)
find_library(ZLib_LIBRARY NAMES z zlib)

if(GME_INCLUDE_DIR AND GME_LIBRARY)
    if(APPLE)
        find_library(GME_DYNAMIC_LIBRARY NAMES "gme"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(GME_DYNAMIC_LIBRARY NAMES "gme" PATH_SUFFIXES ".dll")
    else()
        find_library(GME_DYNAMIC_LIBRARY NAMES "gme" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GME_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(GME  DEFAULT_MSG
                                  GME_LIBRARY GME_INCLUDE_DIR)

mark_as_advanced(GME_INCLUDE_DIR GME_LIBRARY ZLib_LIBRARY)

set(GME_LIBRARIES ${GME_LIBRARY} ${ZLib_LIBRARY})
set(GME_INCLUDE_DIRS ${GME_INCLUDE_DIR})
