set(SDLMixer_MusPlay_UI
    MainWindow/player_main.ui
    MainWindow/multi_music_test.ui
    MainWindow/multi_music_item.ui
    MainWindow/multi_sfx_test.ui
    MainWindow/multi_sfx_item.ui
    MainWindow/musicfx.ui
    MainWindow/sfx_tester.ui
    MainWindow/setup_audio.ui
    MainWindow/setup_midi.ui
    MainWindow/track_muter.ui
    MainWindow/echo_tune.ui
    MainWindow/reverb_tune.ui
    AssocFiles/assoc_files.ui
)

set(SDLMixer_MusPlay_RESOURCE ${SDLMixerMusPlayer_SOURCE_DIR}/_resources/musicplayer.qrc)

list(APPEND SDLMixer_MusPlay_SRC
    AssocFiles/assoc_files.cpp AssocFiles/assoc_files.h
    Effects/reverb.cpp Effects/reverb.h
    Effects/spc_echo.cpp Effects/spc_echo.h
    MainWindow/flowlayout.cpp MainWindow/flowlayout.h
    MainWindow/label_marquee.cpp MainWindow/label_marquee.h
    MainWindow/multi_music_test.cpp MainWindow/multi_music_test.h
    MainWindow/multi_music_item.cpp MainWindow/multi_music_item.h
    MainWindow/multi_sfx_test.cpp MainWindow/multi_sfx_test.h
    MainWindow/multi_sfx_item.cpp MainWindow/multi_sfx_item.h
    MainWindow/musplayer_base.cpp MainWindow/musplayer_base.h
    MainWindow/musplayer_qt.cpp MainWindow/musplayer_qt.h
    MainWindow/musicfx.cpp MainWindow/musicfx.h
    MainWindow/seek_bar.cpp MainWindow/seek_bar.h
    MainWindow/setup_audio.cpp MainWindow/setup_audio.h
    MainWindow/setup_midi.cpp MainWindow/setup_midi.h
    MainWindow/sfx_tester.cpp MainWindow/sfx_tester.h
    MainWindow/track_muter.cpp MainWindow/track_muter.h
    MainWindow/echo_tune.cpp MainWindow/echo_tune.h
    MainWindow/reverb_tune.cpp MainWindow/reverb_tune.h
    Player/mus_player.cpp Player/mus_player.h
    SingleApplication/localserver.cpp SingleApplication/localserver.h
    SingleApplication/pge_application.cpp SingleApplication/pge_application.h
    SingleApplication/singleapplication.cpp SingleApplication/singleapplication.h
    main.cpp
    wave_writer.c
    wave_writer.h

    MainWindow/snes_spc/dsp.cpp MainWindow/snes_spc/dsp.h
    MainWindow/snes_spc/SNES_SPC.cpp MainWindow/snes_spc/SNES_SPC.h
    MainWindow/snes_spc/SNES_SPC_misc.cpp
    MainWindow/snes_spc/SNES_SPC_state.cpp
    MainWindow/snes_spc/spc.cpp MainWindow/snes_spc/spc.h
    MainWindow/snes_spc/SPC_DSP.cpp MainWindow/snes_spc/SPC_DSP.h
    MainWindow/snes_spc/SPC_CPU.h
    MainWindow/snes_spc/SPC_Filter.cpp MainWindow/snes_spc/SPC_Filter.h
    MainWindow/snes_spc/blargg_common.h
    MainWindow/snes_spc/blargg_config.h
    MainWindow/snes_spc/blargg_endian.h
)

if(APPLE)
    add_definitions(-DQ_OS_MACX) # Workaround for MOC
    set(PGE_FILE_ICONS "${SDLMixerMusPlayer_SOURCE_DIR}/_resources/file_musplay.icns")
    list(APPEND SDLMixer_MusPlay_SRC
        _resources/cat_musplay.icns
        ${PGE_FILE_ICONS}
    )
endif()

if(WIN32)
    list(APPEND SDLMixer_MusPlay_SRC _resources/musicplayer.rc)
endif()
