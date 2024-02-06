# - Try to find OGG Container
# Once done this will define
#  OGG_FOUND - System has OGG Container
#  OGG_INCLUDE_DIRS - The OGG Container include directories
#  OGG_LIBRARIES - The libraries needed to use OGG Container

find_path(OGG_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(OGG_LIBRARY NAMES ogg)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OGG_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OGG DEFAULT_MSG
                                  OGG_LIBRARY OGG_INCLUDE_DIR)

mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARY)

set(OGG_LIBRARIES ${OGG_LIBRARY})
set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})
