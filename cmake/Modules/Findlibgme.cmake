include(LibFindMacros)

libfind_pkg_check_modules(libgme_PKGCONF gme libgme)

find_path(libgme_INCLUDE_DIR
	NAMES gme.h
	PATHS
		${libgme_PKGCONF_INCLUDE_DIRS}
		"/usr/include"
		"/usr/local/include"
	PATH_SUFFIXES
		gme
)

find_library(libgme_LIBRARY
	NAMES gme
	PATHS
		${libgme_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libgme
    REQUIRED_VARS libgme_LIBRARY libgme_INCLUDE_DIR)

if(libgme_FOUND AND NOT TARGET gme)
	add_library(gme UNKNOWN IMPORTED)
	set_target_properties(
		gme
		PROPERTIES
		IMPORTED_LOCATION "${libgme_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${libgme_INCLUDE_DIR}"
	)
	add_library(gme::gme ALIAS gme)
endif()

mark_as_advanced(libgme_LIBRARY libgme_INCLUDE_DIR)
