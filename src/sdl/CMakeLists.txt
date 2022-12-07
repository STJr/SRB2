# Declare SDL2 interface sources

target_sources(SRB2SDL2 PRIVATE mixer_sound.c)

target_sourcefile(c)

target_sources(SRB2SDL2 PRIVATE ogl_sdl.c)

target_sources(SRB2SDL2 PRIVATE i_threads.c)

if(${SRB2_USEASM})
	set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES LANGUAGE C)
	set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_sources(SRB2SDL2 PRIVATE
		../win32/win_dbg.c
		../win32/Srb2win.rc)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
	set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	target_sources(SRB2SDL2 PRIVATE
		macosx/mac_alert.c
		macosx/mac_alert.h
		macosx/mac_resources.c
		macosx/mac_resources.h
		macosx/Srb2mac.icns
	)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2win)
elseif("${CMAKE_SYSTEM_NAME}" MATCHES Linux)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME lsdlsrb2)
else()
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	find_library(CORE_FOUNDATION_LIBRARY "CoreFoundation")
	target_link_libraries(SRB2SDL2 PRIVATE
		${CORE_FOUNDATION_LIBRARY}
	)

	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")

	# Configure the app bundle icon and plist properties
	target_sources(SRB2SDL2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Srb2mac.icns")
	set_target_properties(SRB2SDL2 PROPERTIES
		MACOSX_BUNDLE_ICON_FILE "Srb2mac"
		MACOSX_BUNDLE_BUNDLE_NAME "Sonic Robo Blast 2"
		MACOSX_BUNDLE_BUNDLE_VERSION ${SRB2_VERSION}

		RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Srb2mac.icns"
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}" AND NOT "${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}")
	target_link_libraries(SRB2SDL2 PRIVATE SDL2::SDL2-static SDL2_mixer::SDL2_mixer-static)
else()
	target_link_libraries(SRB2SDL2 PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Linux)
	target_link_libraries(SRB2SDL2 PRIVATE m rt)
endif()

if(${SRB2_USEASM})
	if(${SRB2_CONFIG_YASM})
		set(ASM_ASSEMBLER_TEMP ${CMAKE_ASM_YASM_COMPILER})
		set(ASM_ASSEMBLER_OBJFORMAT ${CMAKE_ASM_YASM_OBJECT_FORMAT})
		set_source_files_properties(${SRB2_NASM_SOURCES} LANGUAGE ASM_YASM)
	else()
		set(ASM_ASSEMBLER_TEMP ${CMAKE_ASM_NASM_COMPILER})
		set(ASM_ASSEMBLER_OBJFORMAT ${CMAKE_ASM_NASM_OBJECT_FORMAT})
		set_source_files_properties(${SRB2_NASM_SOURCES} LANGUAGE ASM_NASM)
	endif()
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_link_libraries(SRB2SDL2 PRIVATE
		ws2_32
	)
	target_compile_options(SRB2SDL2 PRIVATE
		-U_WINDOWS
	)
endif()

target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
target_compile_definitions(SRB2SDL2 PRIVATE -DDIRECTFULLSCREEN -DHAVE_SDL)

#### Installation ####
if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	install(TARGETS SRB2SDL2
		BUNDLE DESTINATION .
	)
	set_property(TARGET SRB2SDL2 PROPERTY INSTALL_RPATH_USE_LINK_PATH ON)
else()
	install(TARGETS SRB2SDL2 SRB2SDL2
		RUNTIME DESTINATION .
	)
	if ((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
		set(SRB2_DEBUG_INSTALL OFF CACHE BOOL "Insert *.debug file into the install directory or package.")
		if (${SRB2_DEBUG_INSTALL})
			install(FILES $<TARGET_FILE:SRB2SDL2>.debug
				DESTINATION .
				OPTIONAL
			)
		endif()
	endif()
endif()

# Mac bundle fixup
# HACK: THIS IS IMPORTANT! See the escaped \${CMAKE_INSTALL_PREFIX}? This
# makes it so that var is evaluated LATER during cpack, not right now!
# This fixes the quirk where the bundled libraries don't land in the final package
# https://cmake.org/pipermail/cmake/2011-March/043532.html
#
# HOWEVER: ${CPACK_PACKAGE_DESCRIPTION_SUMMARY} is NOT escaped, because that var
# is only available to us at this step. Read the link: ${CMAKE_INSTALL_PREFIX} at
# this current step points to the CMAKE build folder, NOT the folder that CPACK uses.
# Therefore, it makes sense to escape that var, but not the other.
if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	install(CODE "
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${CPACK_PACKAGE_DESCRIPTION_SUMMARY}.app\"
			\"\"
			/Library/Frameworks
		)"
	)
endif()

set(SRB2_SDL2_AVAILABLE YES PARENT_SCOPE)
