# C++-coded alternative MIDI-sequencer
if(CPP_MIDI_SEQUENCER_NEEDED)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/mix_midi_seq.cpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/mix_midi_seq.h
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/midi_sequencer.h
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/midi_sequencer.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/midi_sequencer_impl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/fraction.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/file_reader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/cvt_xmi2mid.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/cvt_mus2mid.hpp
    )

    set(STDCPP_NEEDED TRUE)
    if(NOT MIXERX_LGPL)
        # Disable MUS and XMI formats as they LGPL-licensed
        message("-- !!! MIDI Sequencer will don't support MUS and XMI formats !!!")
        list(APPEND SDL_MIXER_DEFINITIONS -DBWMIDI_DISABLE_MUS_SUPPORT -DBWMIDI_DISABLE_XMI_SUPPORT)
        setLicense(LICENSE_MIT)
        appendMidiFormats("MIDI;RIFF MIDI")
    else()
        # LGPL license because of XMI/MUS support modules. Disabling them will give MIT
        setLicense(LICENSE_LGPL_2_1p)
        appendMidiFormats("MIDI;RIFF MIDI;MUS;XMI")
    endif()
endif()
