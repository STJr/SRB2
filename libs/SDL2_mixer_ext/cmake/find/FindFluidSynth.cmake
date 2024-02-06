# - Try to find FluidSynth
# Once done this will define
#  FluidSynth_FOUND - System has FluidSynth
#  FluidSynth_INCLUDE_DIRS - The FluidSynth include directories
#  FluidSynth_LIBRARIES - The libraries needed to use FluidSynth

find_path(FluidSynth_INCLUDE_DIR "fluidsynth.h")
find_library(FluidSynth_LIBRARY NAMES fluidsynth)

if(FluidSynth_INCLUDE_DIR AND FluidSynth_LIBRARY)
    if(APPLE)
        find_library(FluidSynth_DYNAMIC_LIBRARY NAMES "fluidsynth"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(FluidSynth_DYNAMIC_LIBRARY NAMES "fluidsynth" PATH_SUFFIXES ".dll")
    else()
        find_library(FluidSynth_DYNAMIC_LIBRARY NAMES "fluidsynth" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FluidSynth_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(FluidSynth DEFAULT_MSG
                                  FluidSynth_LIBRARY FluidSynth_INCLUDE_DIR)

mark_as_advanced(FluidSynth_INCLUDE_DIR FluidSynth_LIBRARY)

set(FluidSynth_LIBRARIES ${FluidSynth_LIBRARY})
set(FluidSynth_INCLUDE_DIRS ${FluidSynth_INCLUDE_DIR})
