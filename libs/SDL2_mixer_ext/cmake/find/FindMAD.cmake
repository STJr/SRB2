# - Try to find MAD
# Once done this will define
#  MAD_FOUND - System has MAD
#  MAD_INCLUDE_DIRS - The MAD include directories
#  MAD_LIBRARIES - The libraries needed to use MAD

find_path(MAD_INCLUDE_DIR "mad.h")
find_library(MAD_LIBRARY NAMES mad)


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MAD_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MAD DEFAULT_MSG
                                  MAD_LIBRARY MAD_INCLUDE_DIR)

mark_as_advanced(MAD_INCLUDE_DIR MAD_LIBRARY)

set(MAD_LIBRARIES ${MAD_LIBRARY})
set(MAD_INCLUDE_DIRS ${MAD_INCLUDE_DIR})
