# - Try to find libEDMIDI
# Once done this will define
#  EDMIDI_FOUND - System has libEDMIDI
#  EDMIDI_INCLUDE_DIRS - The libEDMIDI include directories
#  EDMIDI_LIBRARIES - The libraries needed to use libEDMIDI

find_path(EDMIDI_INCLUDE_DIR "emu_de_midi.h")
find_library(EDMIDI_LIBRARY NAMES EDMIDI)

if(EDMIDI_INCLUDE_DIR AND EDMIDI_LIBRARY)
    if(APPLE)
        find_library(EDMIDI_DYNAMIC_LIBRARY NAMES "EDMIDI"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(EDMIDI_DYNAMIC_LIBRARY NAMES "EDMIDI" PATH_SUFFIXES ".dll")
    else()
        find_library(EDMIDI_DYNAMIC_LIBRARY NAMES "EDMIDI" PATH_SUFFIXES ".so")
    endif()
endif()


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EDMIDI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(EDMIDI  DEFAULT_MSG
                                  EDMIDI_LIBRARY EDMIDI_INCLUDE_DIR)

mark_as_advanced(EDMIDI_INCLUDE_DIR EDMIDI_LIBRARY EDMIDI_DYNAMIC_LIBRARY)

set(EDMIDI_LIBRARIES ${EDMIDI_LIBRARY})
set(EDMIDI_INCLUDE_DIRS ${EDMIDI_INCLUDE_DIR})
