# Declare SDL2 interface sources

if(NOT ${SRB2_CONFIG_HAVE_MIXERX})
	set(SRB2_CONFIG_SDL2_USEMIXER ON CACHE BOOL "Use SDL2_mixer or regular sdl sound")
else()
	set(SRB2_CONFIG_SDL2_USEMIXER OFF)
endif()

if(${SRB2_CONFIG_SDL2_USEMIXER})
	if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
		set(SDL2_MIXER_FOUND ON)
		if(${SRB2_SYSTEM_BITS} EQUAL 64)
			set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/include/SDL2)
			set(SDL2_MIXER_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/lib -lSDL2_mixer")
		else() # 32-bit
			set(SDL2_MIXER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/include/SDL2)
			set(SDL2_MIXER_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/lib -lSDL2_mixer")
		endif()
	else()
		find_package(SDL2_mixer)
	endif()
	if(${SDL2_MIXER_FOUND})
		set(SRB2_HAVE_MIXER ON)
		target_sources(SRB2SDL2 PRIVATE mixer_sound.c)
	else()
		message(WARNING "You specified that SDL2_mixer is available, but it was not found. Falling back to sdl sound.")
		target_sources(SRB2SDL2 PRIVATE sdl_sound.c)
	endif()
elseif(${MIXERX_FOUND})
	target_sources(SRB2SDL2 PRIVATE mixer_sound.c)
else()
	target_sources(SRB2SDL2 PRIVATE sdl_sound.c)
endif()

target_sourcefile(c)

target_sources(SRB2SDL2 PRIVATE ogl_sdl.c)

if(${SRB2_CONFIG_HAVE_THREADS})
	target_sources(SRB2SDL2 PRIVATE i_threads.c)
endif()

# Dependency
if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
	set(SDL2_FOUND ON)
	if(${SRB2_SYSTEM_BITS} EQUAL 64)
		set(SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/include/SDL2)
		set(SDL2_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/lib -lSDL2")
	else() # 32-bit
		set(SDL2_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/include/SDL2)
		set(SDL2_LIBRARIES "-L${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/lib -lSDL2")
	endif()
else()
	find_package(SDL2)
endif()

if(${SDL2_FOUND})
	if(${SRB2_USEASM})
		set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES LANGUAGE C)
		set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_sources(SRB2SDL2 PRIVATE
			../win32/win_dbg.c
			../win32/Srb2win.rc)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
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

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2win)
	elseif(${CMAKE_SYSTEM} MATCHES Linux)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME lsdlsrb2)
	else()
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
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

	target_link_libraries(SRB2SDL2 PRIVATE
		${SDL2_LIBRARIES}
		${SDL2_MIXER_LIBRARIES}
		${GME_LIBRARIES}
		${OPENMPT_LIBRARIES}
		${MIXERX_LIBRARIES}
		${PNG_LIBRARIES}
		${ZLIB_LIBRARIES}
		${OPENGL_LIBRARIES}
		${CURL_LIBRARIES}
	)

	if(${CMAKE_SYSTEM} MATCHES Linux)
		target_link_libraries(SRB2SDL2 PRIVATE
			m
			rt
		)
	endif()

	#target_link_libraries(SRB2SDL2 PRIVATE SRB2Core)

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

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_link_libraries(SRB2SDL2 PRIVATE
			ws2_32
		)
		target_compile_options(SRB2SDL2 PRIVATE
			-U_WINDOWS
		)
	endif()

	target_include_directories(SRB2SDL2 PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_MIXER_INCLUDE_DIRS}
		${GME_INCLUDE_DIRS}
		${OPENMPT_INCLUDE_DIRS}
		${MIXERX_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIRS}
		${CURL_INCLUDE_DIRS}
	)

	if((${SRB2_HAVE_MIXER}) OR (${SRB2_HAVE_MIXERX}))
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
	endif()

	target_compile_definitions(SRB2SDL2 PRIVATE
		-DDIRECTFULLSCREEN -DHAVE_SDL
	)

	## strip debug symbols into separate file when using gcc.
	## to be consistent with Makefile, don't generate for OS X.
	if((CMAKE_COMPILER_IS_GNUCC) AND NOT (${CMAKE_SYSTEM} MATCHES Darwin))
		if((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
			if(${CMAKE_BUILD_TYPE} MATCHES Debug)
				set(OBJCOPY_ONLY_KEEP_DEBUG "--only-keep-debug")
			endif()
			message(STATUS "Will make separate debug symbols in *.debug")
			add_custom_command(TARGET SRB2SDL2 POST_BUILD
				COMMAND ${OBJCOPY} ${OBJCOPY_ONLY_KEEP_DEBUG} $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
				COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
				COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
			)
		endif()
	endif()

	#### Installation ####
	if(${CMAKE_SYSTEM} MATCHES Darwin)
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

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(win_extra_dll_list "")
		macro(getwinlib dllname defaultname)
			if(${SRB2_CONFIG_USE_INTERNAL_LIBRARIES})
				if (${CMAKE_GENERATOR} STREQUAL "MinGW Makefiles")
					if(${SRB2_SYSTEM_BITS} EQUAL 64)
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/x86_64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/x86_64-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/x86_64-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86_64/mingw
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDLMixerX/x86_64-w64-mingw32/bin
						)
					else()
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/i686
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/i686-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/i686-w64-mingw32/bin
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86/mingw
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDLMixerX/i686-w64-mingw32/bin
						)
					endif()
				else()
					if(${SRB2_SYSTEM_BITS} EQUAL 64)
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/x86_64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/lib/x64
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/lib/x64
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86_64/mingw
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDLMixerX/x86_64-w64-mingw32/bin
						)
					else()
						find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}"
							HINTS ${CMAKE_SOURCE_DIR}/libs/dll-binaries/i686
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2/lib/x86
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDL2_mixer/lib/x86
							HINTS ${CMAKE_SOURCE_DIR}/libs/libopenmpt/bin/x86/mingw
							HINTS ${CMAKE_SOURCE_DIR}/libs/SDLMixerX/i686-w64-mingw32/bin
						)
					endif()
				endif()

				list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
			else()
				find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}")
				list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
	endif()
		endmacro()
		getwinlib(SDL2 "SDL2.dll")
		if(${SRB2_CONFIG_SDL2_USEMIXER})
			getwinlib(SDL2_mixer "SDL2_mixer.dll")
			getwinlib(libogg_0 "libogg-0.dll")
			getwinlib(libvorbis_0 "libvorbis-0.dll")
			getwinlib(libvorbisfile_3 "libvorbisfile-3.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_GME})
			getwinlib(libgme "libgme.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_OPENMPT})
			getwinlib(libopenmpt "libopenmpt.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_MIXERX})
			getwinlib(SDL2_mixer_ext "SDL2_mixer_ext.dll")
			getwinlib(libfluidsynth-2 "libfluidsynth-2.dll")
			getwinlib(libgcc_s_sjlj-1 "libgcc_s_sjlj-1.dll")
			getwinlib(libstdc++-6 "libstdc++-6.dll")
		endif()

		install(PROGRAMS
			${win_extra_dll_list}
			DESTINATION .
		)

		# We also want to copy those DLLs to build directories on MSVC.
		# So we'll add a post_build step.
		copy_files_to_build_dir(SRB2SDL2 win_extra_dll_list)
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
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${CPACK_PACKAGE_DESCRIPTION_SUMMARY}.app\"
				\"\"
				/Library/Frameworks
			)"
		)
	endif()

	set(SRB2_SDL2_AVAILABLE YES PARENT_SCOPE)
else()
	message(WARNING "SDL2 was not found, so the SDL2 target will not be available.")
	set(SRB2_SDL2_AVAILABLE NO PARENT_SCOPE)
endif()
