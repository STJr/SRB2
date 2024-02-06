# - Try to find libModPlug
# Once done this will define
#  ModPlug_FOUND - System has libModPlug
#  ModPlug_INCLUDE_DIRS - The libModPlug include directories
#  ModPlug_LIBRARIES - The libraries needed to use libModPlug

find_path(ModPlug_INCLUDE_DIR "modplug.h"
          PATH_SUFFIXES libmodplug)
find_library(ModPlug_LIBRARY NAMES modplug)

if(ModPlug_INCLUDE_DIR AND ModPlug_LIBRARY)
    if(APPLE)
        find_library(ModPlug_DYNAMIC_LIBRARY NAMES "modplug"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(ModPlug_DYNAMIC_LIBRARY NAMES "modplug" PATH_SUFFIXES ".dll")
    else()
        find_library(ModPlug_DYNAMIC_LIBRARY NAMES "modplug" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ModPlug_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ModPlug  DEFAULT_MSG
                                  ModPlug_LIBRARY ModPlug_INCLUDE_DIR)

mark_as_advanced(ModPlug_INCLUDE_DIR ModPlug_LIBRARY)

set(ModPlug_LIBRARIES ${ModPlug_LIBRARY} )
set(ModPlug_INCLUDE_DIRS ${ModPlug_INCLUDE_DIR} )

