#!/bin/bash
if [[ "$1" != "" ]]; then
    cp "$1/libSDL_mixer_ext.so" .
else
    cp ../build/libSDL_mixer_ext.so .
fi

g++ -O0 -g -o playwave -DTEST_MIX_DISTANCE -Wl,-rpath='$ORIGIN' -I../include/SDL_mixer_ext playwave.c -L. -L../build -lSDL2 -lSDL_mixer_ext
g++ -O0 -g -o playmus -DHAVE_SIGNAL_H -Wl,-rpath='$ORIGIN' -I../include/SDL_mixer_ext playmus.c -L. -L../build -lSDL2 -lSDL_mixer_ext

