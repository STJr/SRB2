# SDL Mixer X

An audio mixer library, a fork of [SDL_mixer](https://github.com/libsdl-org/SDL_mixer) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio,
plus a single channel of music, mixed by the popular FLAC, OPUS, XMP, ModPlug,
Timidity MIDI, FluidSynth, Ogg Vorbis, FLAC, and MPG123 MP3 libraries.

SDL Mixer X - is an extended fork made by Vitaly Novichkov "Wohlstand" in
13 January 2015. SDL_mixer is a quick and stable high-quality audio library,
however, it has own bunch of defects such as incorrect playback of some WAV files,
inability to choose MIDI backend in runtime, inability to customize Timidity patches path,
No support for cross-fade and parallel streams playing (has only one musical stream.
Using of very long Chunks is ineffective), etc. The goal of this fork is to resolve those
issues, providing more extended functionality than was originally,
and providing support for more audio formats.

* [More detailed information](docs/index.md)
* [Read original ReadMe](README.txt)


# License
The library does use many dependencies licensed differently, and the final license
depends on which libraries you'll use and how (statically or dynamically).

The license of the library itself is ZLib as the mainstream SDL_mixer.

# Building
See the [docs/index.md](docs/index.md#How%20to%20build) file for details.


## List of libraries
There are libraries that keeps MixerX library under ZLib:

### Static linking
(BSD, ZLib, "Artistic", MIT, and "public domain" licenses are allows usage in closed-source projects)
* libFLAC
* libModPlug
* libOGG
* libOpus
* libSDL2
* libTimidity-SDL
* libVorbis
* libZlib
* dr_mp3
* dr_flac
* stb-vorbis

### LGPL-license libraries
LGPL allows usage in closed-source projects when LGPL-licensed components are linked dynamically.
Note, once you link those libraries statically, the MixerX library gets LGPLv2.1+ license.
They can be enabled if you set the `-DMIXER_ENABLE_LGPL=ON` CMake option.
* libFluidLite
* libGME when Nuked OPN2 or GEMS emulator is used
* libMPG123
* libXMP

### GPL-license libraries
There are libraries making the MixerX being licensed as GPLv2+ or GPLv3+ with no matter
how you will link them. They can be enabled if you set the `-DMIXER_ENABLE_GPL=ON` CMake option.
* libADLMIDI
* libOPNMIDI
* libGME when MAME YM2612 emulator is used
