# - Try to find libXMP Lite version
# Once done this will define
#  XMPLITE_FOUND - System has libXMP Lite version
#  XMPLITE_INCLUDE_DIRS - The libXMP include directories
#  XMPLITE_LIBRARIES - The libraries needed to use libXMP lite version

find_path(XMPLITE_INCLUDE_DIR "xmp.h" PATH_SUFFIXES xmp)
if(NOT XMPLITE_INCLUDE_DIR)
    find_path(XMPLITE_INCLUDE_DIR "xmp.h")
endif()

find_library(XMPLITE_LIBRARY NAMES xmp-lite)

if(XMPLITE_INCLUDE_DIR AND XMPLITE_LIBRARY)
    if(APPLE)
        find_library(XMPLITE_DYNAMIC_LIBRARY NAMES "xmp-lite"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(XMPLITE_DYNAMIC_LIBRARY NAMES "xmp-lite" PATH_SUFFIXES ".dll")
    else()
        find_library(XMPLITE_DYNAMIC_LIBRARY NAMES "xmp-lite" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set XMP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(XMPLITE  DEFAULT_MSG
                                  XMPLITE_LIBRARY XMPLITE_INCLUDE_DIR)

mark_as_advanced(XMPLITE_INCLUDE_DIR XMPLITE_LIBRARY)

set(XMPLITE_LIBRARIES ${XMPLITE_LIBRARY})
set(XMPLITE_INCLUDE_DIRS ${XMPLITE_INCLUDE_DIR})
