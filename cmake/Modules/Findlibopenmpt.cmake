include(LibFindMacros)

libfind_pkg_check_modules(libopenmpt_PKGCONF openmpt libopenmpt)

find_path(libopenmpt_INCLUDE_DIR
	NAMES libopenmpt.h
	PATHS
		${libopenmpt_PKGCONF_INCLUDE_DIRS}
		"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
		"/usr/include"
		"/usr/local/include"
	PATH_SUFFIXES
+		libopenmpt
)

find_library(libopenmpt_LIBRARY
	NAMES openmpt
	PATHS
		${libopenmpt_PKGCONF_LIBRARY_DIRS}
		"/usr/lib"
		"/usr/local/lib"
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libopenmpt
    REQUIRED_VARS libopenmpt_LIBRARY libopenmpt_INCLUDE_DIR)

if(libopenmpt_FOUND AND NOT TARGET openmpt)
	add_library(openmpt UNKNOWN IMPORTED)
	set_target_properties(
		openmpt
		PROPERTIES
		IMPORTED_LOCATION "${libopenmpt_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${libopenmpt_INCLUDE_DIR}"
	)
	add_library(libopenmpt::libopenmpt ALIAS openmpt)
endif()

mark_as_advanced(libopenmpt_LIBRARY libopenmpt_INCLUDE_DIR)
