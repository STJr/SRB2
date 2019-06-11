include(LibFindMacros)

libfind_pkg_check_modules(OPENMPT_PKGCONF OPENMPT)

find_path(OPENMPT_INCLUDE_DIR
	NAMES libopenmpt.h
	PATHS
		${OPENMPT_PKGCONF_INCLUDE_DIRS}
		"/usr/include/libopenmpt"
		"/usr/local/include/libopenmpt"
)

find_library(OPENMPT_LIBRARY
	NAMES openmpt
	PATHS
		${OPENMPT_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

set(OPENMPT_PROCESS_INCLUDES OPENMPT_INCLUDE_DIR)
set(OPENMPT_PROCESS_LIBS OPENMPT_LIBRARY)
libfind_process(OPENMPT)