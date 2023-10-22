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
