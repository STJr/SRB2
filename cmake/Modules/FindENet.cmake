# Find ENet
# Once done, this will define
# 
#  ENET_FOUND - system has SDL2
#  ENET_INCLUDE_DIRS - SDL2 include directories
#  ENET_LIBRARIES - link libraries

include(LibFindMacros)

libfind_pkg_check_modules(ENET_PKGCONF ENET)

# includes
find_path(ENET_INCLUDE_DIR
	NAMES enet.h
	PATHS
		${ENET_PKGCONF_INCLUDE_DIRS}
		"/usr/include/enet"
		"/usr/local/include/enet"
)

# library
find_library(ENET_LIBRARY
	NAMES libenet
	PATHS
		${ENET_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)


# set include dir variables
set(ENET_PROCESS_INCLUDES ENET_INCLUDE_DIR)
set(ENET_PROCESS_LIBS ENET_LIBRARY)
libfind_process(ENET)
