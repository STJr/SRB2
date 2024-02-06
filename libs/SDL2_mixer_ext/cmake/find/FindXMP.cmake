# - Try to find libXMP
# Once done this will define
#  XMP_FOUND - System has libXMP
#  XMP_INCLUDE_DIRS - The libXMP include directories
#  XMP_LIBRARIES - The libraries needed to use libXMP

find_path(XMP_INCLUDE_DIR "xmp.h" PATH_SUFFIXES xmp)
if(NOT XMP_INCLUDE_DIR)
    find_path(XMP_INCLUDE_DIR "xmp.h")
endif()

find_library(XMP_LIBRARY NAMES xmp libxmp)

if(XMP_INCLUDE_DIR AND XMP_LIBRARY)
    if(APPLE)
        find_library(XMP_DYNAMIC_LIBRARY NAMES "xmp"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(XMP_DYNAMIC_LIBRARY NAMES "xmp" "libxmp" PATH_SUFFIXES ".dll")
    else()
        find_library(XMP_DYNAMIC_LIBRARY NAMES "xmp" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set XMP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(XMP  DEFAULT_MSG
                                  XMP_LIBRARY XMP_INCLUDE_DIR)

mark_as_advanced(XMP_INCLUDE_DIR XMP_LIBRARY)

set(XMP_LIBRARIES ${XMP_LIBRARY})
set(XMP_INCLUDE_DIRS ${XMP_INCLUDE_DIR})
