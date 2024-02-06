# - Try to find libOPNMIDI
# Once done this will define
#  OPNMIDI_FOUND - System has libOPNMIDI
#  OPNMIDI_INCLUDE_DIRS - The libOPNMIDI include directories
#  OPNMIDI_LIBRARIES - The libraries needed to use libOPNMIDI

find_path(OPNMIDI_INCLUDE_DIR "opnmidi.h")
find_library(OPNMIDI_LIBRARY NAMES OPNMIDI)

if(OPNMIDI_INCLUDE_DIR AND OPNMIDI_LIBRARY)
    if(APPLE)
        find_library(OPNMIDI_DYNAMIC_LIBRARY NAMES "OPNMIDI"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(OPNMIDI_DYNAMIC_LIBRARY NAMES "OPNMIDI" PATH_SUFFIXES ".dll")
    else()
        find_library(OPNMIDI_DYNAMIC_LIBRARY NAMES "OPNMIDI" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OPNMIDI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OPNMIDI  DEFAULT_MSG
                                  OPNMIDI_LIBRARY OPNMIDI_INCLUDE_DIR)

mark_as_advanced(OPNMIDI_INCLUDE_DIR OPNMIDI_LIBRARY OPNMIDI_DYNAMIC_LIBRARY)

set(OPNMIDI_LIBRARIES ${OPNMIDI_LIBRARY} )
set(OPNMIDI_INCLUDE_DIRS ${OPNMIDI_INCLUDE_DIR} )

