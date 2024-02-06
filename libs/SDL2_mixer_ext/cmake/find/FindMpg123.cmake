# - Try to find Mpg123
# Once done this will define
#  MPG123_FOUND - System has Mpg123
#  MPG123_INCLUDE_DIRS - The Mpg123 include directories
#  MPG123_LIBRARIES - The libraries needed to use Mpg123
#  MPG123_DEFINITIONS - Compiler switches required for using Mpg123

find_package(PkgConfig)
pkg_check_modules(PC_MPG123 QUIET libmpg123)
set(MPG123_DEFINITIONS ${PC_MPG123_CFLAGS_OTHER})

find_path(MPG123_INCLUDE_DIR mpg123.h
          HINTS ${PC_LIBXML_INCLUDEDIR} ${PC_LIBXML_INCLUDE_DIRS}
          PATH_SUFFIXES libmpg123 )

find_library(MPG123_LIBRARY NAMES mpg123
             HINTS ${PC_MPG123_LIBDIR} ${PC_MPG123_LIBRARY_DIRS} )

if(MPG123_LIBRARY AND MPG123_INCLUDE_DIR)
    if(APPLE)
        find_library(MPG123_DYNAMIC_LIBRARY NAMES "mpg123"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(MPG123_DYNAMIC_LIBRARY NAMES "mpg123" PATH_SUFFIXES ".dll")
    else()
        find_library(MPG123_DYNAMIC_LIBRARY NAMES "mpg123" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MPG123_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Mpg123  DEFAULT_MSG
                                  MPG123_LIBRARY MPG123_INCLUDE_DIR)

mark_as_advanced(MPG123_INCLUDE_DIR MPG123_LIBRARY )

set(MPG123_LIBRARIES ${MPG123_LIBRARY} )
set(MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR} )
