# - Find WavPack
# Once done this will define
#   WAVPACK_FOUND        - System has wavpack
#   WAVPACK_INCLUDE_DIRS - The WavPack include directories
#   WAVPACK_LIBRARIES    - The libraries needed to use WavPack

find_path(WavPack_INCLUDE_DIR NAMES "wavpack.h" "wavpack/wavpack.h")
find_library(WavPack_LIBRARY NAMES wavpack)

if(WavPack_INCLUDE_DIR AND WavPack_LIBRARY)
    if(APPLE)
        find_library(WavPack_DYNAMIC_LIBRARY NAMES "wavpack"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(WavPack_DYNAMIC_LIBRARY NAMES "wavpack" PATH_SUFFIXES ".dll")
    else()
        find_library(WavPack_DYNAMIC_LIBRARY NAMES "wavpack" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set wavpack_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(WavPack DEFAULT_MSG
                                  WavPack_LIBRARY WavPack_INCLUDE_DIR)

mark_as_advanced(WavPack_INCLUDE_DIR WavPack_LIBRARY)

set(WavPack_LIBRARIES ${WavPack_LIBRARY})
set(WavPack_INCLUDE_DIRS ${WavPack_INCLUDE_DIR})
