# - Try to find FLAC
# Once done this will define
#  FLAC_FOUND - System has FLAC
#  FLAC_INCLUDE_DIRS - The FLAC include directories
#  FLAC_LIBRARIES - The libraries needed to use FLAC

find_path(FLAC_INCLUDE_DIR "all.h" PATH_SUFFIXES FLAC)
find_path(Ogg_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(FLAC_LIBRARY NAMES FLAC)

if(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
    if(APPLE)
        find_library(FLAC_DYNAMIC_LIBRARY NAMES "FLAC"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(FLAC_DYNAMIC_LIBRARY NAMES "FLAC" PATH_SUFFIXES ".dll")
    else()
        find_library(FLAC_DYNAMIC_LIBRARY NAMES "FLAC" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FLAC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(FLAC  DEFAULT_MSG
                                  FLAC_LIBRARY FLAC_INCLUDE_DIR)

mark_as_advanced(FLAC_INCLUDE_DIR Ogg_INCLUDE_DIR FLAC_LIBRARY)

set(FLAC_LIBRARIES ${FLAC_LIBRARY})
set(FLAC_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${FLAC_INCLUDE_DIR})
