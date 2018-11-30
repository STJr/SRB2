## Assets Target Configuration ##

# For prepending the current source path, later
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(SRB2_ASSET_REQUIRED
"srb2.srb;\
player.dta;\
rings.dta;\
zones.dta;\
patch.dta"
	CACHE STRING "Required asset files for packaging. No spaces between entries!"
)

set(SRB2_ASSET_DOCS
"README.txt;\
LICENSE.txt;\
LICENSE-3RD-PARTY.txt"
	CACHE STRING "Documentation files; will not fail if they do not exist. Packaged differently from optional assets. No spaces between entries!"
)

set(SRB2_ASSET_OPTIONAL
"music.dta"
	CACHE STRING "Optional asset files. No spaces between entries!"
)

# MD5 generation - Filename only, we don't append path to this
set(SRB2_ASSET_HASHED
	${SRB2_ASSET_REQUIRED}
)

PREPEND(SRB2_ASSET_REQUIRED ${CMAKE_CURRENT_SOURCE_DIR} ${SRB2_ASSET_REQUIRED})
PREPEND(SRB2_ASSET_DOCS ${CMAKE_CURRENT_SOURCE_DIR} ${SRB2_ASSET_DOCS})
PREPEND(SRB2_ASSET_OPTIONAL ${CMAKE_CURRENT_SOURCE_DIR} ${SRB2_ASSET_OPTIONAL})

foreach(SRB2_ASSET ${SRB2_ASSET_HASHED})
	file(MD5 ${CMAKE_CURRENT_SOURCE_DIR}/${SRB2_ASSET} "SRB2_ASSET_${SRB2_ASSET}_HASH")
	set(SRB2_ASSET_${SRB2_ASSET}_HASH ${SRB2_ASSET_${SRB2_ASSET}_HASH} PARENT_SCOPE)
endforeach()

# Installation

if(${CMAKE_SYSTEM} MATCHES Darwin)
	get_target_property(outname SRB2SDL2 OUTPUT_NAME)
	install(FILES ${SRB2_ASSET_REQUIRED}
		DESTINATION "${outname}.app/Contents/Resources"
	)
	install(FILES ${SRB2_ASSET_OPTIONAL}
		DESTINATION "${outname}.app/Contents/Resources"
		OPTIONAL
	)
	install(FILES ${SRB2_ASSET_DOCS}
		DESTINATION .
		OPTIONAL
	)
else()
	install(FILES ${SRB2_ASSET_REQUIRED}
		DESTINATION .
	)
	install(FILES ${SRB2_ASSET_OPTIONAL}
		DESTINATION .
		OPTIONAL
	)
	install(FILES ${SRB2_ASSET_DOCS}
		DESTINATION .
		OPTIONAL
	)
endif()
