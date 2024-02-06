if(UNIX AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT HAIKU AND NOT APPLE)
    # CMD Music is not supported on Windows, Haiku, macOS, Android, and Emscripten

    option(USE_CMD "Build with CMD music player support" ON)
    if(USE_CMD)
        message("== using CMD Music (ZLib) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_CMD -D_POSIX_C_SOURCE=1)

        set(fork_found OFF)
        if(NOT fork_found)
            check_symbol_exists(fork unistd.h HAVE_FORK)
            if(HAVE_FORK)
                set(fork_found ON)
                list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_FORK)
            endif()
        endif()

        if(NOT fork_found)
            check_symbol_exists(vfork unistd.h HAVE_VFORK)
            if(HAVE_VFORK)
                set(fork_found ON)
                list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_VFORK)
            endif()
        endif()

        if(fork_found)
            list(APPEND SDLMixerX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/music_cmd.c
                ${CMAKE_CURRENT_LIST_DIR}/music_cmd.h
            )
            appendOtherFormats("CMD Music")
        else()
            message(WARNING "Neither fork() nor vfork() or available on this platform. CMD Music will be disabled.")
            set(USE_CMD OFF)
        endif()
    endif()
endif()
