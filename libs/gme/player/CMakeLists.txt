find_package(SDL QUIET)

set(player_SRCS Audio_Scope.cpp
                Audio_Scope.h
                Music_Player.cpp
                Music_Player.h
                player.cpp)

if(SDL_FOUND)
    message(" ** SDL library located, player demo is available to be built in the /player directory")

    include_directories(${SDL_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_HOME_DIRECTORY}" "${CMAKE_HOME_DIRECTORY}/gme"
        "${CMAKE_BINARY_DIR}/gme")

    add_executable(gme_player ${player_SRCS})
    target_link_libraries(gme_player ${SDL_LIBRARY} gme)

    # Is not to be installed though
else()
    message("SDL library not found, disabling player demo build")
endif()
