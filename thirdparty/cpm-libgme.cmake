CPMAddPackage(
	NAME libgme
	VERSION 0.6.3
	URL "https://bitbucket.org/mpyne/game-music-emu/get/e76bdc0cb916e79aa540290e6edd0c445879d3ba.zip"
	EXCLUDE_FROM_ALL ON
	OPTIONS
		"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
		"ENABLE_UBSAN OFF"
		"GME_YM2612_EMU MAME"
)

if(libgme_ADDED)
	target_compile_features(gme PRIVATE cxx_std_11)
	# libgme's CMakeLists.txt already links this
	#target_link_libraries(gme PRIVATE ZLIB::ZLIB)
endif()
