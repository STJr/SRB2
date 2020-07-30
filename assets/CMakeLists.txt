## Assets Target Configuration ##

# For prepending the current source path, later
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(SRB2_ASSET_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/installer"
	CACHE STRING "Path to directory that contains all asset files for the installer.")

set(SRB2_ASSET_INSTALL ON
	CACHE BOOL "Insert asset files into the install directory or package.")

####################
# POST-V2.2 NOTE: Do not forget to add patch.pk3 to the end of this list!
####################

set(SRB2_ASSET_HASHED
"srb2.pk3;\
player.dta;\
zones.pk3;\
patch.pk3"
	CACHE STRING "Asset filenames to apply MD5 checks. No spaces between entries!"
)

set(SRB2_ASSET_DOCS
"README.txt;\
LICENSE.txt;\
LICENSE-3RD-PARTY.txt;\
README-SDL.txt"
	CACHE STRING "Documentation filenames. In OS X, these are packaged separately from other assets. No spaces between entries!"
)

PREPEND(SRB2_ASSET_DOCS ${SRB2_ASSET_DIRECTORY} ${SRB2_ASSET_DOCS})

foreach(SRB2_ASSET ${SRB2_ASSET_HASHED})
	file(MD5 ${SRB2_ASSET_DIRECTORY}/${SRB2_ASSET} "SRB2_ASSET_${SRB2_ASSET}_HASH")
	set(SRB2_ASSET_${SRB2_ASSET}_HASH ${SRB2_ASSET_${SRB2_ASSET}_HASH} PARENT_SCOPE)
endforeach()

# Installation

if(${CMAKE_SYSTEM} MATCHES Darwin)
	get_target_property(outname SRB2SDL2 OUTPUT_NAME)
	if(${SRB2_ASSET_INSTALL})
		install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
			DESTINATION "${outname}.app/Contents/Resources"
		)
	endif()
	# Always install the doc files, even in non-asset packages.
	install(FILES ${SRB2_ASSET_DOCS}
		DESTINATION .
		OPTIONAL
	)
else()
	if(${SRB2_ASSET_INSTALL})
		install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
			DESTINATION .
		)
		# Docs are assumed to be located in SRB2_ASSET_DIRECTORY, so don't install them in their own call.
	else()
		# Always install the doc files, even in non-asset packages.
		install(FILES ${SRB2_ASSET_DOCS}
			DESTINATION .
			OPTIONAL
		)
	endif()
endif()
