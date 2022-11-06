# Find Valgrind
# Once done, this will define
#
#  VALGRIND_FOUND - system has Valgrind
#  VALGRIND_INCLUDE_DIRS - Valgrind include directories
#  VALGRIND_LIBRARIES - link libraries

include(LibFindMacros)

libfind_pkg_check_modules(VALGRIND_PKGCONF valgrind)

# includes
find_path(VALGRIND_INCLUDE_DIR
	NAMES valgrind.h
	PATHS
		${VALGRIND_PKGCONF_INCLUDE_DIRS}
		"/usr/include/valgrind"
		"/usr/local/include/valgrind"
)

# library
find_library(COREGRIND_LIBRARY
	NAMES coregrind
	PATHS
		${VALGRIND_PKGCONF_LIBRARY_DIRS}
		"/usr/lib/valgrind"
		"/usr/local/lib/valgrind"
)
find_library(VEX_LIBRARY
	NAMES vex
	PATHS
		${VALGRIND_PKGCONF_LIBRARY_DIRS}
		"/usr/lib/valgrind"
		"/usr/local/lib/valgrind"
)

# set include dir variables
set(VALGRIND_PROCESS_INCLUDES VALGRIND_INCLUDE_DIR)
set(VALGRIND_PROCESS_LIBS COREGRIND_LIBRARY VEX_LIBRARY)
libfind_process(VALGRIND)

if(VALGRIND_FOUND AND NOT TARGET Valgrind::Valgrind)
	add_library(Valgrind::Coregrind UNKNOWN IMPORTED)
	set_target_properties(Valgrind::Coregrind PROPERTIES
		IMPORTED_LOCATION "${COREGRIND_LIBRARY}"
	)
	add_library(Valgrind::Vex UNKNOWN IMPORTED)
	set_target_properties(Valgrind::Vex PROPERTIES
		IMPORTED_LOCATION "${VEX_LIBRARY}"
	)
	add_library(Valgrind::Valgrind UNKNOWN IMPORTED)
	set_target_properties(Valgrind::Valgrind PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${VALGRIND_INCLUDE_DIR}"
	)
	target_link_libraries(Valgrind::Valgrind PUBLIC Valgrind::Coregrind Valgrind::Vex)
endif()
