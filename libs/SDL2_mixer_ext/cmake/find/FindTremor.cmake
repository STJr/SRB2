# - Try to find of Integer-only OGG Vorbis
# Once done this will define
#  Tremor_FOUND - System has Integer-only OGG Vorbis
#  Tremor_INCLUDE_DIRS - The Integer-only OGG Vorbis include directories
#  Tremor_LIBRARIES - The libraries needed to use Integer-only OGG Vorbis

find_path(Tremor_INCLUDE_DIR "ivorbisfile.h" PATH_SUFFIXES tremor)
find_path(Ogg_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(VorbisIDec_LIBRARY NAMES vorbisidec)

if(Tremor_INCLUDE_DIR AND VorbisIDec_LIBRARY)
    if(APPLE)
        find_library(VorbisIDec_DYNAMIC_LIBRARY NAMES "vorbisidec"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(VorbisIDec_DYNAMIC_LIBRARY NAMES "vorbisidec" PATH_SUFFIXES ".dll")
    else()
        find_library(VorbisIDec_DYNAMIC_LIBRARY NAMES "vorbisidec" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Tremor_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Tremor DEFAULT_MSG
                                  VorbisIDec_LIBRARY Tremor_INCLUDE_DIR)

mark_as_advanced(Ogg_INCLUDE_DIR Tremor_INCLUDE_DIR VorbisIDec_LIBRARY)

set(Tremor_LIBRARIES ${VorbisIDec_LIBRARY})
set(Tremor_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${Tremor_INCLUDE_DIR})
