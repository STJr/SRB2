include(LibFindMacros)

libfind_pkg_check_modules(libminiupnpc_PKGCONF miniupnpc libminiupnpc)

find_path(libminiupnpc_INCLUDE_DIR
	NAMES miniupnpc.h
	PATHS
		${libminiupnpc_PKGCONF_INCLUDE_DIRS}
		"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
		"/usr/include"
		"/usr/local/include"
	PATH_SUFFIXES
		miniupnpc
)

find_library(libminiupnpc_LIBRARY
	NAMES miniupnpc
	PATHS
		${libminiupnpc_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(miniupnpc
    REQUIRED_VARS libminiupnpc_LIBRARY libminiupnpc_INCLUDE_DIR)

if(miniupnpc_FOUND AND NOT TARGET miniupnpc)
	add_library(miniupnpc UNKNOWN IMPORTED)
	set_target_properties(
		miniupnpc
		PROPERTIES
		IMPORTED_LOCATION "${libminiupnpc_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${libminiupnpc_INCLUDE_DIR}"
	)
	add_library(miniupnpc::miniupnpc ALIAS miniupnpc)
endif()

mark_as_advanced(libminiupnpc_LIBRARY libminiupnpc_INCLUDE_DIR)
