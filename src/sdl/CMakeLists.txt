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

prepend_sources(SRB2_SDL2_SOURCES)
prepend_sources(SRB2_SDL2_HEADERS)

# Dependency
find_package(SDL2)

if(${SDL2_FOUND})
	set(SRB2_SDL2_TOTAL_SOURCES
		${SRB2_CORE_SOURCES}
		${SRB2_CORE_HEADERS}
		${SRB2_SDL2_SOURCES}
		${SRB2_SDL2_HEADERS}
	)

	if(${SRB2_CONFIG_HWRENDER})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_HWRENDER_SOURCES}
			${SRB2_HWRENDER_HEADERS}
			${SRB2_R_OPENGL_SOURCES}
			${SRB2_R_OPENGL_HEADERS}
		)
	endif()

	if(${SRB2_CONFIG_HAVE_BLUA})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_LUA_SOURCES}
			${SRB2_LUA_HEADERS}
			${SRB2_BLUA_SOURCES}
			${SRB2_BLUA_HEADERS}
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${CMAKE_SOURCE_DIR}/src/win32/win_dbg.c
			${CMAKE_SOURCE_DIR}/src/win32/Srb2win.rc
		)
	endif()
	if(NOT CLANG)
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${CMAKE_SOURCE_DIR}/src/string.c
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
		set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			macosx/mac_alert.c
			macosx/mac_alert.h
			macosx/mac_resources.c
			macosx/mac_resources.h
			macosx/Srb2mac.icns
		)
	endif()

	if(CLANG)
		add_executable(${SRB2_SDL2_EXE_NAME} MACOSX_BUNDLE ${SRB2_SDL2_TOTAL_SOURCES})
		add_framework(CoreFoundation ${SRB2_SDL2_EXE_NAME})
		add_framework(SDL2 ${SRB2_SDL2_EXE_NAME})
		add_framework(SDL2_mixer ${SRB2_SDL2_EXE_NAME})
		target_link_libraries(${SRB2_SDL2_EXE_NAME} PRIVATE
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)
		set_target_properties(${SRB2_SDL2_EXE_NAME} PROPERTIES OUTPUT_NAME "Sonic Robo Blast 2")
	else()
		add_executable(${SRB2_SDL2_EXE_NAME} WIN32 ${SRB2_SDL2_TOTAL_SOURCES})

		target_link_libraries(${SRB2_SDL2_EXE_NAME} PRIVATE
			${SDL2_LIBRARIES}
			${SDL2_MIXER_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)

		if(${CMAKE_SYSTEM} MATCHES Linux)
			target_link_libraries(${SRB2_SDL2_EXE_NAME} PRIVATE
				m
				rt
			)
		endif()

	endif()

	set_target_properties(${SRB2_SDL2_EXE_NAME} PROPERTIES VERSION ${SRB2_VERSION})

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_link_libraries(${SRB2_SDL2_EXE_NAME} PRIVATE
			ws2_32
		)
		target_compile_options(${SRB2_SDL2_EXE_NAME} PRIVATE
			-U_WINDOWS
		)
	endif()

	if(MSVC)
		find_package(SDL2_MAIN REQUIRED)
		target_link_libraries(${SRB2_SDL2_EXE_NAME} PRIVATE
			${SDL2_MAIN_LIBRARIES}
		)
		target_compile_options(${SRB2_SDL2_EXE_NAME} PRIVATE
			/Umain
			/D_CRT_SECURE_NO_WARNINGS # something about string functions.
			/D_CRT_NONSTDC_NO_DEPRECATE
			/DSDLMAIN
			/D_WINSOCK_DEPRECATED_NO_WARNINGS # Don't care
		)
	endif()

	target_include_directories(${SRB2_SDL2_EXE_NAME} PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_MIXER_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIRS}
	)

	if(${SRB2_HAVE_MIXER})
		target_compile_definitions(${SRB2_SDL2_EXE_NAME} PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
	endif()

	target_compile_definitions(${SRB2_SDL2_EXE_NAME} PRIVATE
		-DHAVE_SDL
		-DNDEBUG
	)

	#### Installation ####
	if (CLANG)
		install(TARGETS ${SRB2_SDL2_EXE_NAME}
			BUNDLE DESTINATION .
		)
	else()
		install(TARGETS ${SRB2_SDL2_EXE_NAME} ${SRB2_SDL2_EXE_NAME}
			RUNTIME DESTINATION .
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		find_library(SRB2_SDL2_DLL_SDL2 "SDL2.dll")
		if(${SRB2_CONFIG_SDL2_USEMIXER})
			find_library(SRB2_SDL2_DLL_SDL2_mixer "SDL2_mixer.dll")
			find_library(SRB2_SDL2_DLL_libmikmod-2 "libmikmod-2.dll")
			find_library(SRB2_SDL2_DLL_libogg_0 "libogg-0.dll")
			find_library(SRB2_SDL2_DLL_libvorbis_0 "libvorbis-0.dll")
			find_library(SRB2_SDL2_DLL_libvorbisfile_3 "libvorbisfile-3.dll")
			find_library(SRB2_SDL2_DLL_smpeg2 "smpeg2.dll")
		endif()
		if(${SRB2_CONFIG_HAVE_GME})
			find_library(SRB2_SDL2_DLL_libgme "libgme.dll")
		endif()

		install(PROGRAMS
			${SRB2_SDL2_DLL_SDL2}

			${SRB2_SDL2_DLL_SDL2_mixer}
			${SRB2_SDL2_DLL_libmikmod}
			${SRB2_SDL2_DLL_libogg_0}
			${SRB2_SDL2_DLL_libvorbis_0}
			${SRB2_SDL2_DLL_libvorbisfile_3}
			${SRB2_SDL2_DLL_smpeg2}

			${SRB2_SDL2_DLL_libgme}
			DESTINATION .
		)
	endif()

	
	# Mac bundle fixup
	if(CLANG)
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/Sonic Robo Blast 2.app\"
				\"\"
				/Library/Frameworks
			)"
		)
	endif()
else()
	message(WARNING "SDL2 wasn't found, so ${SRB2_SDL2_EXE_NAME} won't be available")
endif()
