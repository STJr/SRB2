# Declare SDL2 interface sources

set(SRB2_CONFIG_SDL2_USEMIXER ON CACHE BOOL "Use SDL2_mixer or regular sdl sound")

if(${SRB2_CONFIG_SDL2_USEMIXER})
	find_package(SDL2_mixer)
	if(${SDL2_MIXER_FOUND})
		set(SRB2_HAVE_MIXER ON)
		set(SRB2_SDL2_SOUNDIMPL mixer_sound.c)
	else()
		message(WARNING "You specified that SDL2_mixer is available, but it was not found. Falling back to sdl sound.")
		set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
	endif()
else()
	set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
endif()

set(SRB2_SDL2_SOURCES
	dosstr.c
	endtxt.c
	hwsym_sdl.c
	i_cdmus.c
	i_main.c
	i_net.c
	i_system.c
	i_ttf.c
	i_video.c
	#IMG_xpm.c
	ogl_sdl.c

	${SRB2_SDL2_SOUNDIMPL}
)

set(SRB2_SDL2_HEADERS
	endtxt.h
	hwsym_sdl.h
	i_ttf.h
	ogl_sdl.h
	sdlmain.h
)

source_group("Interface Code" FILES ${SRB2_SDL2_SOURCES} ${SRB2_SDL2_HEADERS})

# Dependency
find_package(SDL2)

if(${SDL2_FOUND})
	set(SRB2_SDL2_TOTAL_SOURCES
		${SRB2_CORE_SOURCES}
		${SRB2_CORE_HEADERS}
		${SRB2_CORE_RENDER_SOURCES}
		${SRB2_CORE_GAME_SOURCES}
		${SRB2_LUA_SOURCES}
		${SRB2_LUA_HEADERS}
		${SRB2_BLUA_SOURCES}
		${SRB2_BLUA_HEADERS}
		${SRB2_SDL2_SOURCES}
		${SRB2_SDL2_HEADERS}
	)

	source_group("Main" FILES ${SRB2_CORE_SOURCES} ${SRB2_CORE_HEADERS})
	source_group("Renderer" FILES ${SRB2_CORE_RENDER_SOURCES})
	source_group("Game" FILES ${SRB2_CORE_GAME_SOURCES})
	source_group("Assembly" FILES ${SRB2_ASM_SOURCES} ${SRB2_NASM_SOURCES})
	source_group("LUA" FILES ${SRB2_LUA_SOURCES} ${SRB2_LUA_HEADERS})
	source_group("LUA\\Interpreter" FILES ${SRB2_BLUA_SOURCES} ${SRB2_BLUA_HEADERS})

	if(${SRB2_CONFIG_HWRENDER})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_HWRENDER_SOURCES}
			${SRB2_HWRENDER_HEADERS}
			${SRB2_R_OPENGL_SOURCES}
			${SRB2_R_OPENGL_HEADERS}
		)

		source_group("Hardware" FILES ${SRB2_HWRENDER_SOURCES} ${SRB2_HWRENDER_HEADERS})
		source_group("Hardware\\OpenGL Renderer" FILES ${SRB2_R_OPENGL_SOURCES} ${SRB2_R_OPENGL_HEADERS})
	endif()

	if(${SRB2_USEASM})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_NASM_SOURCES}
		)
		if(MSVC)
			set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
				${SRB2_NASM_OBJECTS}
			)
			set_source_files_properties(${SRB2_NASM_OBJECTS} PROPERTIES GENERATED ON)
		else()
			list(APPEND SRB2_SDL2_TOTAL_SOURCES ${SRB2_ASM_SOURCES})
			set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES LANGUAGE C)
			set_source_files_properties(${SRB2_ASM_SOURCES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
		endif()
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${CMAKE_SOURCE_DIR}/src/win32/win_dbg.c
			${CMAKE_SOURCE_DIR}/src/win32/Srb2win.rc
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
		set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		set(SRB2_SDL2_MAC_SOURCES
			macosx/mac_alert.c
			macosx/mac_alert.h
			macosx/mac_resources.c
			macosx/mac_resources.h
			macosx/Srb2mac.icns
		)
		source_group("Interface Code\\OSX Compatibility" FILES ${SRB2_SDL2_MAC_SOURCES})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES} ${SRB2_SDL2_MAC_SOURCES})
	endif()

	add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32 ${SRB2_SDL2_TOTAL_SOURCES})
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME ${SRB2_SDL2_EXE_NAME})

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		find_library(CORE_LIB CoreFoundation)
		target_link_libraries(SRB2SDL2 PRIVATE
			${CORE_LIB}
			SDL2
			SDL2_mixer
			${GME_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "Sonic Robo Blast 2")
	else()
		target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_LIBRARIES}
			${SDL2_MIXER_LIBRARIES}
			${GME_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)

		if(${CMAKE_SYSTEM} MATCHES Linux)
			target_link_libraries(SRB2SDL2 PRIVATE
				m
				rt
			)
		endif()
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

		if(MSVC)
			# using assembler with msvc doesn't work, must do it manually
			foreach(ASMFILE ${SRB2_NASM_SOURCES})
				get_filename_component(ASMFILE_NAME ${ASMFILE} NAME_WE)
				set(ASMFILE_NAME ${ASMFILE_NAME}.obj)
				add_custom_command(TARGET SRB2SDL2 PRE_LINK
					COMMAND ${ASM_ASSEMBLER_TEMP} ARGS -f ${ASM_ASSEMBLER_OBJFORMAT} -o ${CMAKE_CURRENT_BINARY_DIR}/${ASMFILE_NAME} ${ASMFILE}
					COMMENT "assemble ${ASMFILE_NAME}."
				)
			endforeach()
		endif()
	endif()

	set_target_properties(SRB2SDL2 PROPERTIES VERSION ${SRB2_VERSION})

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_link_libraries(SRB2SDL2 PRIVATE
			ws2_32
		)
		target_compile_options(SRB2SDL2 PRIVATE
			-U_WINDOWS
		)
	endif()

	if(MSVC)
		find_package(SDL2_MAIN REQUIRED)
		target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_MAIN_LIBRARIES}
		)
		target_compile_options(SRB2SDL2 PRIVATE
			/Umain
			/D_CRT_SECURE_NO_WARNINGS # something about string functions.
			/D_CRT_NONSTDC_NO_DEPRECATE
			/DSDLMAIN
			/D_WINSOCK_DEPRECATED_NO_WARNINGS # Don't care
		)
	endif()

	target_include_directories(SRB2SDL2 PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_MIXER_INCLUDE_DIRS}
		${GME_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIRS}
	)

	if(${SRB2_HAVE_MIXER})
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
	endif()

	target_compile_definitions(SRB2SDL2 PRIVATE
		-DHAVE_SDL
	)

	## strip debug symbols into separate file when using gcc
	if(CMAKE_COMPILER_IS_GNUCC)
		if(${CMAKE_BUILD_TYPE} MATCHES Debug)
			message(STATUS "Will make separate debug symbols in *.debug")
			add_custom_command(TARGET SRB2SDL2 POST_BUILD
				COMMAND ${OBJCOPY} --only-keep-debug $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
				COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
				COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
			)
		endif()
	endif()

	#### Installation ####
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(TARGETS SRB2SDL2
			BUNDLE DESTINATION .
		)
	else()
		install(TARGETS SRB2SDL2 SRB2SDL2
			RUNTIME DESTINATION .
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(win_extra_dll_list "")
		macro(getwinlib dllname defaultname)
			find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}")
			list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
		endmacro()
		getwinlib(SDL2 "SDL2.dll")
		if(${SRB2_CONFIG_SDL2_USEMIXER})
			getwinlib(SDL2_mixer "SDL2_mixer.dll")
			getwinlib(libmikmod-2 "libmikmod-2.dll")
			getwinlib(libogg_0 "libogg-0.dll")
			getwinlib(libvorbis_0 "libvorbis-0.dll")
			getwinlib(libvorbisfile_3 "libvorbisfile-3.dll")
			getwinlib(smpeg2 "smpeg2.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_GME})
			getwinlib(libgme "libgme.dll")
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
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/Sonic Robo Blast 2.app\"
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
