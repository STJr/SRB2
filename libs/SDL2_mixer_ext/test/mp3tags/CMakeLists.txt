
set(CMAKE_CXX_STANDARD 11)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
  ${SDLMixerX_SOURCE_DIR}/include
  ${SDLMixerX_SOURCE_DIR}/src
  ${SDLMixerX_SOURCE_DIR}/src/codecs
)

add_executable(mp3utils_test mp3utils_test.c)
target_include_directories(mp3utils_test PRIVATE ${SDL_MIXER_INCLUDE_PATHS})
target_link_libraries(mp3utils_test PRIVATE SDL2_mixer_ext_Static SDL2_test)

add_test(NAME mp3utils_test
         COMMAND mp3utils_test
         WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data"
)
