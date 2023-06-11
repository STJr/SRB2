macro(export)
endmacro()

if(SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES)
	set(SRB2_INTERNAL_LIBRARY_TYPE SHARED)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES OFF)
else()
	set(SRB2_INTERNAL_LIBRARY_TYPE STATIC)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES ON)
endif()


if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME SDL2
		VERSION 2.24.2
		URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.24.2.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS
			"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_SHARED ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_STATIC ${NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL_TEST OFF"
			"SDL2_DISABLE_SDL2MAIN ON"
			"SDL2_DISABLE_INSTALL ON"
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME SDL2_mixer
		VERSION 2.6.2
		URL "https://github.com/libsdl-org/SDL_mixer/archive/refs/tags/release-2.6.2.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS
			"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
			"SDL2MIXER_INSTALL OFF"
			"SDL2MIXER_DEPS_SHARED OFF"
			"SDL2MIXER_SAMPLES OFF"
			"SDL2MIXER_VENDORED ON"
			"SDL2MIXER_FLAC ON"
			"SDL2MIXER_FLAC_LIBFLAC OFF"
			"SDL2MIXER_FLAC_DRFLAC ON"
			"SDL2MIXER_MOD OFF"
			"SDL2MIXER_MP3 ON"
			"SDL2MIXER_MP3_DRMP3 ON"
			"SDL2MIXER_MIDI ON"
			"SDL2MIXER_OPUS OFF"
			"SDL2MIXER_VORBIS STB"
			"SDL2MIXER_WAVE ON"
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME ZLIB
		VERSION 1.2.13
		URL "https://github.com/madler/zlib/archive/refs/tags/v1.2.13.zip"
		EXCLUDE_FROM_ALL
		DOWNLOAD_ONLY YES
	)
	if(ZLIB_ADDED)
		set(ZLIB_SRCS
			crc32.h
			deflate.h
			gzguts.h
			inffast.h
			inffixed.h
			inflate.h
			inftrees.h
			trees.h
			zutil.h

			adler32.c
			compress.c
			crc32.c
			deflate.c
			gzclose.c
			gzlib.c
			gzread.c
			gzwrite.c
			inflate.c
			infback.c
			inftrees.c
			inffast.c
			trees.c
			uncompr.c
			zutil.c
		)
		list(TRANSFORM ZLIB_SRCS PREPEND "${ZLIB_SOURCE_DIR}/")

		configure_file("${ZLIB_SOURCE_DIR}/zlib.pc.cmakein" "${ZLIB_BINARY_DIR}/zlib.pc" @ONLY)
		configure_file("${ZLIB_SOURCE_DIR}/zconf.h.cmakein" "${ZLIB_BINARY_DIR}/include/zconf.h" @ONLY)
		configure_file("${ZLIB_SOURCE_DIR}/zlib.h" "${ZLIB_BINARY_DIR}/include/zlib.h" @ONLY)

		add_library(ZLIB ${SRB2_INTERNAL_LIBRARY_TYPE} ${ZLIB_SRCS})
		set_target_properties(ZLIB PROPERTIES
			VERSION 1.2.13
			OUTPUT_NAME "z"
		)
		target_include_directories(ZLIB PRIVATE "${ZLIB_SOURCE_DIR}")
		target_include_directories(ZLIB PUBLIC "${ZLIB_BINARY_DIR}/include")
		if(MSVC)
			target_compile_definitions(ZLIB PRIVATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
		endif()
		add_library(ZLIB::ZLIB ALIAS ZLIB)
	endif()
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME png
		VERSION 1.6.38
		URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.38.zip"
		# png cmake build is broken on msys/mingw32
		DOWNLOAD_ONLY YES
	)

	if(png_ADDED)
		# Since png's cmake build is broken, we're going to create a target manually
		set(
			PNG_SOURCES

			png.h
			pngconf.h

			pngpriv.h
			pngdebug.h
			pnginfo.h
			pngstruct.h

			png.c
			pngerror.c
			pngget.c
			pngmem.c
			pngpread.c
			pngread.c
			pngrio.c
			pngrtran.c
			pngrutil.c
			pngset.c
			pngtrans.c
			pngwio.c
			pngwrite.c
			pngwtran.c
			pngwutil.c
		)
		list(TRANSFORM PNG_SOURCES PREPEND "${png_SOURCE_DIR}/")

		add_custom_command(
			OUTPUT "${png_BINARY_DIR}/include/png.h" "${png_BINARY_DIR}/include/pngconf.h"
			COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h" "${png_BINARY_DIR}/include"
			DEPENDS "${png_SOURCE_DIR}/png.h" "${png_SOURCE_DIR}/pngconf.h"
			VERBATIM
		)
		add_custom_command(
			OUTPUT "${png_BINARY_DIR}/include/pnglibconf.h"
			COMMAND ${CMAKE_COMMAND} -E copy "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${png_BINARY_DIR}/include/pnglibconf.h"
			DEPENDS "${png_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt"
			VERBATIM
		)
		list(
			APPEND PNG_SOURCES
			"${png_BINARY_DIR}/include/png.h"
			"${png_BINARY_DIR}/include/pngconf.h"
			"${png_BINARY_DIR}/include/pnglibconf.h"
		)
		add_library(png "${SRB2_INTERNAL_LIBRARY_TYPE}" ${PNG_SOURCES})

		# Disable ARM NEON since having it automatic breaks libpng external build on clang for some reason
		target_compile_definitions(png PRIVATE -DPNG_ARM_NEON_OPT=0)

		# The png includes need to be available to consumers
		target_include_directories(png PUBLIC "${png_BINARY_DIR}/include")

		# ... and these also need to be present only for png build
		target_include_directories(png PRIVATE "${ZLIB_SOURCE_DIR}")
		target_include_directories(png PRIVATE "${ZLIB_BINARY_DIR}")
		target_include_directories(png PRIVATE "${png_BINARY_DIR}")

		target_link_libraries(png PRIVATE ZLIB::ZLIB)
		add_library(PNG::PNG ALIAS png)
	endif()
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	set(
		internal_curl_options

		"BUILD_CURL_EXE OFF"
		"BUILD_SHARED_LIBS ${SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES}"
		"CURL_DISABLE_TESTS ON"
		"HTTP_ONLY ON"
		"CURL_DISABLE_CRYPTO_AUTH ON"
		"CURL_DISABLE_NTLM ON"
		"ENABLE_MANUAL OFF"
		"ENABLE_THREADED_RESOLVER OFF"
		"CURL_USE_LIBPSL OFF"
		"CURL_USE_LIBSSH2 OFF"
		"USE_LIBIDN2 OFF"
		"CURL_ENABLE_EXPORT_TARGET OFF"
	)
	if(${CMAKE_SYSTEM} MATCHES Windows)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
		list(APPEND internal_curl_options "CURL_USE_SCHANNEL ON")
	endif()
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL OFF")
		list(APPEND internal_curl_options "CURL_USE_SECTRANSP ON")
	endif()
	if(${CMAKE_SYSTEM} MATCHES Linux)
		list(APPEND internal_curl_options "CURL_USE_OPENSSL ON")
	endif()

	CPMAddPackage(
		NAME curl
		VERSION 7.86.0
		URL "https://github.com/curl/curl/archive/refs/tags/curl-7_86_0.zip"
		EXCLUDE_FROM_ALL ON
		OPTIONS ${internal_curl_options}
	)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME openmpt
		VERSION 0.4.30
		URL "https://github.com/OpenMPT/openmpt/archive/refs/tags/libopenmpt-0.4.30.zip"
		DOWNLOAD_ONLY ON
	)

	if(openmpt_ADDED)
		set(
			openmpt_SOURCES

			# minimp3
			# -DMPT_WITH_MINIMP3
			include/minimp3/minimp3.c

			common/mptStringParse.cpp
			common/mptLibrary.cpp
			common/Logging.cpp
			common/Profiler.cpp
			common/version.cpp
			common/mptCPU.cpp
			common/ComponentManager.cpp
			common/mptOS.cpp
			common/serialization_utils.cpp
			common/mptStringFormat.cpp
			common/FileReader.cpp
			common/mptWine.cpp
			common/mptPathString.cpp
			common/mptAlloc.cpp
			common/mptUUID.cpp
			common/mptTime.cpp
			common/mptString.cpp
			common/mptFileIO.cpp
			common/mptStringBuffer.cpp
			common/mptRandom.cpp
			common/mptIO.cpp
			common/misc_util.cpp

			common/mptCRC.h
			common/mptLibrary.h
			common/mptIO.h
			common/version.h
			common/stdafx.h
			common/ComponentManager.h
			common/Endianness.h
			common/mptStringFormat.h
			common/mptMutex.h
			common/mptUUID.h
			common/mptExceptionText.h
			common/BuildSettings.h
			common/mptAlloc.h
			common/mptTime.h
			common/FileReaderFwd.h
			common/Logging.h
			common/mptException.h
			common/mptWine.h
			common/mptStringBuffer.h
			common/misc_util.h
			common/mptBaseMacros.h
			common/mptMemory.h
			common/mptFileIO.h
			common/serialization_utils.h
			common/mptSpan.h
			common/mptThread.h
			common/FlagSet.h
			common/mptString.h
			common/mptStringParse.h
			common/mptBaseUtils.h
			common/mptRandom.h
			common/CompilerDetect.h
			common/FileReader.h
			common/mptAssert.h
			common/mptPathString.h
			common/Profiler.h
			common/mptOS.h
			common/mptBaseTypes.h
			common/mptCPU.h
			common/mptBufferIO.h
			common/versionNumber.h

			soundlib/WAVTools.cpp
			soundlib/ITTools.cpp
			soundlib/AudioCriticalSection.cpp
			soundlib/Load_stm.cpp
			soundlib/MixerLoops.cpp
			soundlib/Load_dbm.cpp
			soundlib/ModChannel.cpp
			soundlib/Load_gdm.cpp
			soundlib/Snd_fx.cpp
			soundlib/Load_mid.cpp
			soundlib/mod_specifications.cpp
			soundlib/Snd_flt.cpp
			soundlib/Load_psm.cpp
			soundlib/Load_far.cpp
			soundlib/patternContainer.cpp
			soundlib/Load_med.cpp
			soundlib/Load_dmf.cpp
			soundlib/Paula.cpp
			soundlib/modcommand.cpp
			soundlib/Message.cpp
			soundlib/SoundFilePlayConfig.cpp
			soundlib/Load_uax.cpp
			soundlib/plugins/PlugInterface.cpp
			soundlib/plugins/LFOPlugin.cpp
			soundlib/plugins/PluginManager.cpp
			soundlib/plugins/DigiBoosterEcho.cpp
			soundlib/plugins/dmo/DMOPlugin.cpp
			soundlib/plugins/dmo/Flanger.cpp
			soundlib/plugins/dmo/Distortion.cpp
			soundlib/plugins/dmo/ParamEq.cpp
			soundlib/plugins/dmo/Gargle.cpp
			soundlib/plugins/dmo/I3DL2Reverb.cpp
			soundlib/plugins/dmo/Compressor.cpp
			soundlib/plugins/dmo/WavesReverb.cpp
			soundlib/plugins/dmo/Echo.cpp
			soundlib/plugins/dmo/Chorus.cpp
			soundlib/Load_ams.cpp
			soundlib/tuningbase.cpp
			soundlib/ContainerUMX.cpp
			soundlib/Load_ptm.cpp
			soundlib/ContainerXPK.cpp
			soundlib/SampleFormatMP3.cpp
			soundlib/tuning.cpp
			soundlib/Sndfile.cpp
			soundlib/ContainerMMCMP.cpp
			soundlib/Load_amf.cpp
			soundlib/Load_669.cpp
			soundlib/modsmp_ctrl.cpp
			soundlib/Load_mtm.cpp
			soundlib/OggStream.cpp
			soundlib/Load_plm.cpp
			soundlib/Tables.cpp
			soundlib/Load_c67.cpp
			soundlib/Load_mod.cpp
			soundlib/Load_sfx.cpp
			soundlib/Sndmix.cpp
			soundlib/load_j2b.cpp
			soundlib/ModSequence.cpp
			soundlib/SampleFormatFLAC.cpp
			soundlib/ModInstrument.cpp
			soundlib/Load_mo3.cpp
			soundlib/ModSample.cpp
			soundlib/Dlsbank.cpp
			soundlib/Load_itp.cpp
			soundlib/UpgradeModule.cpp
			soundlib/MIDIMacros.cpp
			soundlib/ContainerPP20.cpp
			soundlib/RowVisitor.cpp
			soundlib/Load_imf.cpp
			soundlib/SampleFormatVorbis.cpp
			soundlib/Load_dsm.cpp
			soundlib/Load_mt2.cpp
			soundlib/MixerSettings.cpp
			soundlib/S3MTools.cpp
			soundlib/Load_xm.cpp
			soundlib/MIDIEvents.cpp
			soundlib/pattern.cpp
			soundlib/Load_digi.cpp
			soundlib/Load_s3m.cpp
			soundlib/tuningCollection.cpp
			soundlib/SampleIO.cpp
			soundlib/Dither.cpp
			soundlib/Load_mdl.cpp
			soundlib/OPL.cpp
			soundlib/WindowedFIR.cpp
			soundlib/SampleFormats.cpp
			soundlib/Load_wav.cpp
			soundlib/Load_it.cpp
			soundlib/UMXTools.cpp
			soundlib/Load_stp.cpp
			soundlib/Load_okt.cpp
			soundlib/Load_ult.cpp
			soundlib/MixFuncTable.cpp
			soundlib/SampleFormatOpus.cpp
			soundlib/Fastmix.cpp
			soundlib/Tagging.cpp
			soundlib/ITCompression.cpp
			soundlib/Load_dtm.cpp
			soundlib/MPEGFrame.cpp
			soundlib/XMTools.cpp
			soundlib/SampleFormatMediaFoundation.cpp
			soundlib/InstrumentExtensions.cpp

			soundlib/MixerInterface.h
			soundlib/SoundFilePlayConfig.h
			soundlib/ModSample.h
			soundlib/MIDIEvents.h
			soundlib/ModSampleCopy.h
			soundlib/patternContainer.h
			soundlib/ChunkReader.h
			soundlib/ITCompression.h
			soundlib/Dither.h
			soundlib/S3MTools.h
			soundlib/MPEGFrame.h
			soundlib/WAVTools.h
			soundlib/mod_specifications.h
			soundlib/ITTools.h
			soundlib/RowVisitor.h
			soundlib/plugins/PluginMixBuffer.h
			soundlib/plugins/PluginStructs.h
			soundlib/plugins/LFOPlugin.h
			soundlib/plugins/PlugInterface.h
			soundlib/plugins/DigiBoosterEcho.h
			soundlib/plugins/OpCodes.h
			soundlib/plugins/dmo/Echo.h
			soundlib/plugins/dmo/I3DL2Reverb.h
			soundlib/plugins/dmo/WavesReverb.h
			soundlib/plugins/dmo/ParamEq.h
			soundlib/plugins/dmo/Gargle.h
			soundlib/plugins/dmo/DMOPlugin.h
			soundlib/plugins/dmo/Chorus.h
			soundlib/plugins/dmo/Compressor.h
			soundlib/plugins/dmo/Distortion.h
			soundlib/plugins/dmo/Flanger.h
			soundlib/plugins/PluginManager.h
			soundlib/SampleIO.h
			soundlib/Container.h
			soundlib/ModSequence.h
			soundlib/UMXTools.h
			soundlib/Message.h
			soundlib/modcommand.h
			soundlib/XMTools.h
			soundlib/Snd_defs.h
			soundlib/MixFuncTable.h
			soundlib/pattern.h
			soundlib/modsmp_ctrl.h
			soundlib/Tagging.h
			soundlib/tuningcollection.h
			soundlib/Mixer.h
			soundlib/FloatMixer.h
			soundlib/AudioCriticalSection.h
			soundlib/Tables.h
			soundlib/tuningbase.h
			soundlib/WindowedFIR.h
			soundlib/Sndfile.h
			soundlib/Paula.h
			soundlib/ModInstrument.h
			soundlib/Dlsbank.h
			soundlib/IntMixer.h
			soundlib/OPL.h
			soundlib/Resampler.h
			soundlib/ModChannel.h
			soundlib/MixerSettings.h
			soundlib/AudioReadTarget.h
			soundlib/MixerLoops.h
			soundlib/tuning.h
			soundlib/MIDIMacros.h
			soundlib/OggStream.h
			soundlib/Loaders.h
			soundlib/BitReader.h
			soundlib/opal.h

			sounddsp/AGC.cpp
			sounddsp/EQ.cpp
			sounddsp/DSP.cpp
			sounddsp/Reverb.cpp
			sounddsp/Reverb.h
			sounddsp/EQ.h
			sounddsp/DSP.h
			sounddsp/AGC.h

			libopenmpt/libopenmpt_c.cpp
			libopenmpt/libopenmpt_cxx.cpp
			libopenmpt/libopenmpt_impl.cpp
			libopenmpt/libopenmpt_ext_impl.cpp
		)
		list(TRANSFORM openmpt_SOURCES PREPEND "${openmpt_SOURCE_DIR}/")

		# -DLIBOPENMPT_BUILD
		configure_file("openmpt_svn_version.h" "svn_version.h")
		add_library(openmpt "${SRB2_INTERNAL_LIBRARY_TYPE}" ${openmpt_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/svn_version.h)
		if("${CMAKE_C_COMPILER_ID}" STREQUAL GNU OR "${CMAKE_C_COMPILER_ID}" STREQUAL Clang OR "${CMAKE_C_COMPILER_ID}" STREQUAL AppleClang)
			target_compile_options(openmpt PRIVATE "-g0")
		endif()
		if("${CMAKE_SYSTEM_NAME}" STREQUAL Windows AND "${CMAKE_C_COMPILER_ID}" STREQUAL MSVC)
			target_link_libraries(openmpt PRIVATE Rpcrt4)
		endif()
		target_compile_features(openmpt PRIVATE cxx_std_11)
		target_compile_definitions(openmpt PRIVATE -DLIBOPENMPT_BUILD)

		target_include_directories(openmpt PRIVATE "${openmpt_SOURCE_DIR}/common")
		target_include_directories(openmpt PRIVATE "${openmpt_SOURCE_DIR}/src")
		target_include_directories(openmpt PRIVATE "${openmpt_SOURCE_DIR}/include")
		target_include_directories(openmpt PRIVATE "${openmpt_SOURCE_DIR}")
		target_include_directories(openmpt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

		# I wish this wasn't necessary, but it is
		target_include_directories(openmpt PUBLIC "${openmpt_SOURCE_DIR}")
	endif()
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
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
	target_compile_features(gme PRIVATE cxx_std_11)
	target_link_libraries(gme PRIVATE ZLIB::ZLIB)
endif()
