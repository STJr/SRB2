# - Try to find FluidLite
# Once done this will define
#  FluidLite_FOUND - System has FluidLite
#  FluidLite_INCLUDE_DIRS - The FluidLite include directories
#  FluidLite_LIBRARIES - The libraries needed to use FluidLite

find_path(FluidLite_INCLUDE_DIR "fluidlite.h")
find_library(FluidLite_LIBRARY NAMES fluidlite)

if(FluidLite_INCLUDE_DIR AND FluidLite_LIBRARY)
    if(APPLE)
        find_library(FluidLite_DYNAMIC_LIBRARY NAMES "fluidlite"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(FluidLite_DYNAMIC_LIBRARY NAMES "fluidlite" PATH_SUFFIXES ".dll")
    else()
        find_library(FluidLite_DYNAMIC_LIBRARY NAMES "fluidlite" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FluidLite_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(FluidLite DEFAULT_MSG
                                  FluidLite_LIBRARY FluidLite_INCLUDE_DIR)

mark_as_advanced(FluidLite_INCLUDE_DIR FluidLite_LIBRARY)

set(FluidLite_LIBRARIES ${FluidLite_LIBRARY})
set(FluidLite_INCLUDE_DIRS ${FluidLite_INCLUDE_DIR})

