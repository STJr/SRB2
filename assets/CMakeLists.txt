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

set(SRB2_ASSET_HASHED
"srb2.srb;\
player.dta;\
rings.dta;\
zones.dta;\
patch.dta"
	CACHE STRING "Asset filenames to apply MD5 checks. No spaces between entries!"
)

set(SRB2_ASSET_DOCS
"README.txt;\
LICENSE.txt;\
LICENSE-3RD-PARTY.txt"
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
	install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
		DESTINATION "${outname}.app/Contents/Resources"
	)
	install(FILES ${SRB2_ASSET_DOCS}
		DESTINATION .
		OPTIONAL
	)
else()
	install(DIRECTORY "${SRB2_ASSET_DIRECTORY}/"
		DESTINATION .
	)
	# Docs are assumed to be located in SRB2_ASSET_DIRECTORY, so don't install again
	#install(FILES ${SRB2_ASSET_DOCS}
	#	DESTINATION .
	#	OPTIONAL
	#)
endif()
