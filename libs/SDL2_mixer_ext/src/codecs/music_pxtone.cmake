option(USE_PXTONE   "Build with PXTone support" ON)
if(USE_PXTONE)
    message("== using PXTone (Own liberal license) ==")

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnData.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnDelay.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnError.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnEvelist.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnMaster.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnMem.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnOverDrive.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Frequency.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Noise.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_NoiseBuilder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Oggv.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Oscillator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_PCM.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnService.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnService_moo.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnText.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnUnit.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnWoice.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnWoice_io.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnWoicePTV.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtoneNoise.cpp

        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtn.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnData.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnDelay.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnError.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnEvelist.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnMaster.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnMax.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnMem.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnOverDrive.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Frequency.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Noise.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_NoiseBuilder.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Oggv.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_Oscillator.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnPulse_PCM.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnService.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnText.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnUnit.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtnWoice.h
        ${CMAKE_CURRENT_LIST_DIR}/pxtone/pxtoneNoise.h
    )
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${CMAKE_CURRENT_LIST_DIR}/pxtone)

    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_PXTONE)

    if(USE_OGG_VORBIS)
        list(APPEND SDL_MIXER_DEFINITIONS -DpxUseSDL -DpxINCLUDE_OGGVORBIS)
        if(USE_OGG_VORBIS_TREMOR)
            list(APPEND SDL_MIXER_DEFINITIONS -DpxINCLUDE_OGGVORBIS_TREMOR)
        elseif(USE_OGG_VORBIS_STB)
            list(APPEND SDL_MIXER_DEFINITIONS -DpxINCLUDE_OGGVORBIS_STB)
        endif()
    endif()

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_pxtone.cpp
        ${CMAKE_CURRENT_LIST_DIR}/music_pxtone.h
    )
    appendChiptuneFormats("PTTUNE;PTCOP")
endif()
