# Declare SDL2 interface sources

target_sources(SRB2SDL2 PRIVATE
	ogl_sdl.c
	i_threads.c
	i_net.c
	i_system.c
	i_main.c
	i_video.c
	dosstr.c
	endtxt.c
	hwsym_sdl.c
)

# Compatibility flag with later versions of GCC
# We should really fix our code to not need this
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(SRB2SDL2 PRIVATE -mno-ms-bitfields)
endif()

# Yes we know we use insecure CRT functions...
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_definitions(SRB2SDL2 PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()

# Compiler warnings configuration
target_compile_options(SRB2SDL2 PRIVATE
    # Using generator expressions to handle per-language compile options

    # C, GNU
    # This is a direct translation from versions.mk
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:GNU>>:
        -Wall
        -Wno-trigraphs
        -W # Was controlled by RELAXWARNINGS
        -Wfloat-equal
        -Wundef
        -Wpointer-arith
        -Wbad-function-cast
        -Wcast-qual
        -Wcast-align # Was controlled by NOCASTALIGNWARN
        -Wwrite-strings
        -Wsign-compare
        -Wmissing-prototypes
        -Wmissing-declarations
        -Wmissing-noreturn
        -Winline
        -Wformat-y2k
        -Wformat-security

        $<$<VERSION_LESS:$<C_COMPILER_VERSION>,2.9.5>:
            -Wno-div-by-zero
            -Wendif-labels
            -Wdisabled-optimization
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,4.0.0>:
            -Wold-style-definition
            -Wmissing-field-initializers
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,4.1.0>:
            -Wshadow
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,4.3.0>:
            -funit-at-a-time
            -Wlogical-op
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,4.5.0>:
            -Wlogical-op
            -Wno-error=array-bounds
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,4.6.0>:
            -Wno-suggest-attribute=noreturn
            -Wno-error=suggest-attribute=noreturn
            -Werror=vla
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,5.4.0>:
            -Wno-logical-op
            -Wno-error=logical-op
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,6.1.0>:
            -Wno-tautological-compare
            -Wno-error=tautological-compare
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7.1.0>:
            -Wno-error=format-overflow=2
            -Wimplicit-fallthrough=4
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,8.1.0>:
            -Wno-error=format-overflow
            -Wno-error=stringop-truncation
            -Wno-error=stringop-overflow
            -Wno-format-overflow
            -Wno-stringop-truncation
            -Wno-stringop-overflow
            -Wno-error=multistatement-macros
        >

        $<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,9.1.0>:
            -Wno-error=address-of-packed-member
        >
    >

    # C, Clang and Apple Clang
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<OR:$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>>>:
        -Wall
        -Wno-absolute-value
        -Wno-trigraphs
        -Wno-error=non-literal-null-conversion
        -Wno-error=constant-conversion
        -Wno-unused-but-set-variable
        -Wno-error=unused-but-set-variable
        -Wno-shorten-64-to-32
    >

    # C, MSVC
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<C_COMPILER_ID:MSVC>>:
        # All warnings at and before Visual Studio 2019 RTM
        # https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-by-compiler-version?view=msvc-170
        /Wv:19.20.27004.0
    >

    # C++, GNU, Clang and Apple Clang
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>>>:
        -Wall
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-private-field
    >

    # C++, MSVC
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<C_COMPILER_ID:MSVC>>:
        /Wv:19.20.27004.0
    >

    # GNU
    $<$<C_COMPILER_ID:GNU>:
        -fmax-errors=5
    >
)
if(SRB2_CONFIG_ERRORMODE)
    target_compile_options(SRB2SDL2 PRIVATE
        $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>>:
            -Werror
        >

        $<$<C_COMPILER_ID:MSVC>:
            /WX
        >
    )
endif()

# Link warnings configuration
target_link_options(SRB2SDL2 PRIVATE
    $<$<C_COMPILER_ID:GNU>:
        # -Wl,--as-needed   - Was controlled by NOLDWARNING
    >
)

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

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_link_libraries(SRB2SDL2 PRIVATE SDL2::SDL2main)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Linux)
	target_link_libraries(SRB2SDL2 PRIVATE m rt)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_link_libraries(SRB2SDL2 PRIVATE
		ws2_32
	)
	target_compile_options(SRB2SDL2 PRIVATE
		-U_WINDOWS
	)
endif()

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2 REQUIRED)
if(TARGET SDL2main)
	target_link_libraries(SRB2SDL2 PRIVATE SDL2main)
endif()
target_compile_definitions(SRB2SDL2 PRIVATE -DDIRECTFULLSCREEN -DHAVE_SDL)
target_include_directories(SRB2SDL2 PRIVATE ${SDL2_INCLUDE_DIR})

find_package(SDL2_mixer_ext CONFIG QUIET)
find_package(SDL2_mixer_ext QUIET)
if(TARGET SDL2_mixer_ext::SDL2_mixer_ext OR TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static OR TARGET SDL2_mixer_ext OR TARGET SDL2_mixer_ext_Static)
	if(NOT TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static AND TARGET SDL2_mixer_ext_Static)
	add_library(SDL2_mixer_ext::SDL2_mixer_ext_Static ALIAS SDL2_mixer_ext_Static)
	endif()
	if(NOT TARGET SDL2_mixer_ext::SDL2_mixer_ext AND TARGET SDL2_mixer_ext)
	add_library(SDL2_mixer_ext::SDL2_mixer_ext ALIAS SDL2_mixer_ext)
	endif()
	if(TARGET SDL2_mixer_ext::SDL2_mixer_ext OR TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static)
		if(TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static)
			target_link_libraries(SRB2SDL2 PRIVATE SDL2_mixer_ext::SDL2_mixer_ext_Static)
		else()
			target_link_libraries(SRB2SDL2 PRIVATE SDL2_mixer_ext::SDL2_mixer_ext)
		endif()
	target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DHAVE_MIXERX -DSOUND=SOUND_MIXER)
	endif()
endif()

if(TARGET SDL2_mixer_ext::SDL2_mixer_ext OR TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static)
	message(STATUS "SDL2_mixer_ext found, skipping SDL2_mixer")
else()
	message(STATUS "SDL2_mixer_ext not found, going to try SDL2_mixer")
	find_package(SDL2_mixer CONFIG QUIET)
	find_package(SDL2_mixer QUIET)
	if(TARGET SDL2_mixer::SDL2_mixer OR TARGET SDL2_mixer::SDL2_mixer-static OR TARGET SDL2_mixer OR TARGET SDL2_mixer_Static)
		if(NOT TARGET SDL2_mixer::SDL2_mixer-static AND TARGET SDL2_mixer_Static)
		add_library(SDL2_mixer::SDL2_mixer-static ALIAS SDL2_mixer_Static)
		endif()
		if(NOT TARGET SDL2_mixer::SDL2_mixer AND TARGET SDL2_mixer)
		add_library(SDL2_mixer::SDL2_mixer ALIAS SDL2_mixer)
		endif()
		if(TARGET SDL2_mixer::SDL2_mixer OR TARGET SDL2_mixer::SDL2_mixer-static)
			if(TARGET SDL2_mixer::SDL2_mixer-static)
				target_link_libraries(SRB2SDL2 PRIVATE SDL2_mixer::SDL2_mixer-static)
			else()
				target_link_libraries(SRB2SDL2 PRIVATE SDL2_mixer::SDL2_mixer)
			endif()
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
		endif()
	endif()
endif()

if(TARGET SDL2_mixer_ext::SDL2_mixer_ext OR TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static OR TARGET SDL2_mixer::SDL2_mixer OR TARGET SDL2_mixer::SDL2_mixer-static)
	target_sources(SRB2SDL2 PRIVATE mixer_sound.c)
	target_link_libraries(SRB2SDL2 PRIVATE SDL2::SDL2)
else()
	target_sources(SRB2SDL2 PRIVATE sdl_sound.c)
	target_link_libraries(SRB2SDL2 PRIVATE SDL2::SDL2)
endif()

if(TARGET SDL2::SDL2)
	message(STATUS "SDL2 Found")
else()
	message(STATUS "no SDL2 Found")
endif()

if(TARGET SDL2::SDL2main)
	message(STATUS "SDL2main Found")
else()
	message(STATUS "No SDL2main Found")
endif()

if(TARGET SDL2_mixer_ext::SDL2_mixer_ext OR TARGET SDL2_mixer_ext::SDL2_mixer_ext_Static)
	message(STATUS "SDL2_mixer_ext Found")
elseif(TARGET SDL2_mixer::SDL2_mixer OR TARGET SDL2_mixer::SDL2_mixer-static)
	message(STATUS "SDL2_mixer found")
else()
	message(STATUS "no SDL2_mixer_ext or SDL2_mixer Found")
endif()

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
