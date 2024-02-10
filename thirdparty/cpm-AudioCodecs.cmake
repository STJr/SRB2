CPMAddPackage(
	NAME AudioCodecs
	GITHUB_REPOSITORY WohlSoft/AudioCodecs
	GIT_TAG master
	OPTIONS
		"BUILD_LIBXMP OFF"
		"BUILD_SDL2_STATIC ON"
		"BUILD_TIMIDITYSDL OFF"
		"BUILD_WAVPACK OFF"
		"USE_LOCAL_SDL2 ON"
)
    message(STATUS "Cmake added local AudioCodecs: ${CPM_PACKAGE_AudioCodecs_BINARY_DIR}")
