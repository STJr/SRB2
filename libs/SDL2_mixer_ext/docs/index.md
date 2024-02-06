# MixerX - the extended SDL_mixer
A fork of [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio,
plus a single channel of music, mixed by the popular FLAC, OPUS, XMP, ModPlug,
MikMod MOD, Timidity MIDI, FluidSynth, libADLMIDI, libOPNMIDI, Ogg Vorbis,
stb_vorbis, FLAC, DRFLAC, DRMP3 and MPG123 MP3 libraries.

SDL Mixer X (or shorty MixerX) - It’s an extended fork of the SDL_mixer library
made by Vitaly Novichkov "Wohlstand" in 13 January 2015. SDL_mixer is a quick and
stable high-quality audio library, however, it has own bunch of deffects and lack
of certain abilities such as inability to choose the MIDI backend in runtime,
No support for cross-fade and parallel streams playing (has only one musical stream.
Using of very long Chunks is ineffectively), etc. The goal of this fork is resolving
those issues, providing more extended functionality than was originally, and
providing support for more supported audio formats.

## New features of MixerX in comparison to original SDL_mixer
* Added much more music formats (Such a game music emulators liks NSF, VGM, HES, GBM, etc. playing via GME library, also XMI, MUS, and IMF playing via [ADLMIDI](https://github.com/Wohlstand/libADLMIDI) library)
* Support for multiple parallel music (or atmpsphere) streams using new-added API functions including the cross-fade support
* Better MIDI support:
  * Added ability to choose any of available MIDI backends in runtime
  * Added ability to append custom config path for Timidity synthesizer, then no more limit to default patches set
  * Forked version now has [ADLMIDI](https://github.com/Wohlstand/libADLMIDI) midi sequences together with Native MIDI, Timidity and Fluidsynth. ADLMIDI is [OPL-Synth](http://wohlsoft.ru/pgewiki/FM_Synthesis) Emulation based MIDI player. Unlike to other MIDI synthesizers are using in SDL Mixer X (except of Native MIDI), ADLMIDI is completely standalone software synthesizer which never requires any external sound fonts or banks to work.
  * Also, the [OPNMIDI](https://github.com/Wohlstand/libOPNMIDI) was added which a MIDI player through emulator of YM2612 chip which was widely used on Sega Megadrive/Genesis game console. Also is fully standalone like ADLMIDI.
* Added new API functions
  * Added ADLMIDI Extra functions: Change bank ID, enable/disable high-level tremolo, enable/disable high-level vibrato, enable/disable scalable modulation, Set path to custom bank file (You can use [this editor](https://github.com/Wohlstand/OPL3BankEditor) to create or edit them)
  * Added OPNMIDI Extra function: Set path to custom bank file (You can use [this editor](https://github.com/Wohlstand/OPN2BankEditor) to create or edit them)
  * Music tempo change function: you can change speed of playing music by giving the factor value while music is playing. Works for MIDI (libADLMIDI and libOPNMIDI), GME, and XMP.
  * Added music functions with the `Stream` suffix which can alter individual properties of every opened music and even play them concurrently.
* Added an ability to set individual effects per every `Mix_Music` instance.
* Built-in effects (such as 3D-position, distance, panning, and reverse stereo) now supported per every `Mix_Music` instance
* Added support of [extra arguments](http://wohlsoft.ru/pgewiki/SDL_Mixer_X#Path_arguments) in the tail of the file path, passed into Mix_LoadMUS function.
* Added ability to build shared library in the <u>stdcall</u> mode with static linking of libSDL on Windows to use it as independent audio library with other languages like VB6 or .NET.
* CMake building systems in use
  * Can be used with libraries currently installed in the system.
  * Optionally, you can set the `-DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON` option to automatically download sources of all dependencies and build them in a place.

### Features introduced at MixerX later added into the mainstream SDL_mixer
* Added support of the loop points in the OGG and OPUS files (via <u>LOOPSTART</u> and <u>LOOPEND</u> (or <u>LOOPLENGHT</u>) meta-tags). Backported into SDL_mixer since 12'th of Novenber, 2019.
* In the Modplug module enabled internal loops (tracker musics with internal loops are will be looped correctly). Fixed at SDL_mixer since 31'th of January.
* Added new API functions
  * Ability to redefine Timidity patches path. So, patches folders are can be stored in any place!
  * Added functions to retrieve some meta-tags: Title, Artist, Album, Copyright

### Features introduced at MixerX later removed because of unnecessarity
* Own re-sampling implementation which a workaround to glitches caused with inaccurate re-sampler implementation from SDL2. Recent versions of SDL2 now has much better resampler than was before.


# How to build

## Requirements

### Minimal
To build library, the next conditions are required:
- CMake 2.8.12 and higher (when using AudioCodecs mode, 3.2 and higher)
- SDL2 version 2.0.8 or newer
- git if you want to build the library with the AudioCodecs pack
* Compiler, compatible with C90 (C99 is required when building with OPUS library) or higher and C++98 (to build C++-written parts and libraries) or higher. Build tested and works on next compilers:
  * GCC 4.8 and higher
  * CLang 3.x and higher
  * MSVC 2013, 2015, 2017

### More file formats support
Without installing any codec libraries and without AudioCodecs pack use, these
formats will be supported by the library by default:
- WAV
- AIFF
- VOC (As Mix_Chunk only)
- MP3 via embedded DRMP3
- OGG Vorbis via embedded stb_vorbis
- FLAC via embedded DRFLAC
- MIDI via embedded Timidity or via system-wide MIDI output

If you want to enable support for more file formats, you will need to install
optional libraries (listed below) into the system and, if needed, their use.
You aren't required to install every library into the system to build the
library: any enabled component will be automatically disabled when the required
library is missing from the system.

#### BSD/MIT/PD-licensed libraries
There are libraries licensed under any of permissive license. These libraries
can be linked to any project (including closed-source proprietary projects).
They are always used when detected to be installed in the system.
- [libogg](https://github.com/xiph/ogg) - is required when is need to support Vorbis, FLAC, or Opus.
- [libvorbis](https://github.com/xiph/vorbis) or [Tremor](https://gitlab.xiph.org/xiph/tremor) - to enable the OGG Vorbis support, otherwise the embedded stb_vorbis can be used.
- [libopus](https://github.com/xiph/opus) and [libopusfile](https://github.com/xiph/opusfile) - to enable the Opus support.
- [libFLAC](https://github.com/xiph/flac) - to enable the FLAC support, otherwise the embedded [DRFLAC](https://github.com/mackron/dr_libs) codec can be used.
- [libmodplug](https://github.com/Konstanty/libmodplug) - another library for the tracker music formats support.
- [libfluidsynth](https://github.com/FluidSynth/fluidsynth) or [libfluidlite](https://github.com/divideconcept/FluidLite) - the wavetable MIDI synthesizer library based on SoundFont 2 specifications
- [libEDMIDI](https://github.com/Wohlstand/libEDMIDI) - the MIDI synthesizer library based on OPLL/PSG/SCC synthesis

#### LGPL-licensed libraries
There are libraries that has LGPL license, and you will need to link them
dynamically when your project is not compatible to GPL license. These libraries
will not be used without `-DMIXERX_ENABLE_LGPL=ON` (or `-DMIXERX_ENABLE_GPL=ON`)
CMake argument.
- [libmpg123](https://www.mpg123.de/) - to enable the MP3 support, otherwise the embedded [DRMP3](https://github.com/mackron/dr_libs) codec can be used.
- [libxmp](https://github.com/libxmp/libxmp/) - to enable the support for various tracker music formats such as MOD, IT, XM, S3M, etc.
- [libgme](https://bitbucket.org/mpyne/game-music-emu) - to enable support for chiptune formats such as NSF, VGM, SPC, HES, etc.

#### GPL-licensed libraries
There are libraries that currently has GPL license, and if you will take use of
them, the license of MixerX will be GPLv3. These libraries will not ne used
without `-DMIXERX_ENABLE_GPL=ON` CMake argument.
- [libADLMIDI](https://github.com/Wohlstand/libADLMIDI) - the MIDI synthesizer library based on OPL3 synthesis
- [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI) - the MIDI synthesizer library based on OPN2/OPNA synthesis


## Build the project

### License of library
The MixerX library has 3 different license modes (Zlib (default), LGPLv2.1+,
and GPLv3+) depending on enabled components: you may get the LGPL license if
you statically link LGPL components into the MixerX shared library, or GPL
license when you link any GPL components to the library. By default, LGPL and
GPL licensed modules are disabled. You may use the next CMake options to enable
these modules:

* `-DMIXERX_ENABLE_LGPL=ON` - Enable components with LGPL license. This will
enable part of extra functionality and if you link them statically into MixerX,
build it will be licensed under LGPL, or you can link the MixerX itself statically,
but link other components dynamically, they will stay LGPL while MixerX by itself
will be LGPL.
* `-DMIXERX_ENABLE_GPL=ON` - Enable components with GPL license. This
will enable complete functionality of the MixerX, and its build will be licensed
under GPL. This argument also enables LGPL-licensed libraries.

If you want to statically link the library into GPL-incompatible software,
don't enable these options.


### Build with default configuration
Once all necessary dependencies installed, run the next code in your terminal
at the clonned repository directory:
```bash
# Prepare the build directory
mkdir build
cd build

# Configure the project
cmake -DCMAKE_BUILD_TYPE=Release ..

# Run the build
make -j 4 #where 4 - set number of CPU cores you have

# Install the built library and headers into the system
make install
```
It will enable these components which was detected at the system during CMake step.

The built library will have ZLib license that you can statically link with any
application including closed-source proprietary.

This build will have next components always disabled:
- FluidSynth (LGPLv2.1+)
- GME (LGPLv2.1+ or GPLv2+ with MAME YM2612 enabled)
- MPG123 (LGPLv2.1)
- libADLMIDI (GPLv3+)
- libOPNMIDI (GPLv3+)


### Build with default configuration with LGPL mode
If you want to enable support for FluidSynth, GME, and MPG123, you will need
to use the LGPL mode (adding the `-DMIXERX_ENABLE_LGPL=ON` argument), run the
configure step with the next line:
```
# Configure the project
cmake -DCMAKE_BUILD_TYPE=Release -DMIXERX_ENABLE_LGPL=ON -DSDL_MIXER_X_SHARED=ON -DSDL_MIXER_X_STATIC=OFF ..
```
The resulted build will have LGPLv2.1 license that you can dynamically link with any
application including commercial.

This build will have next components always disabled:
- GME (LGPLv2.1+ or GPLv2+ with MAME YM2612 enabled)
- libADLMIDI (GPLv3+)
- libOPNMIDI (GPLv3+)


### Build with default configuration and GPL license
If you want to enable support for libADLMIDI and libOPNMIDI, you will need
to use the GPL mode (adding the `-DMIXERX_ENABLE_GPL=ON` argument), run the
configure step with the next line:
```
# Configure the project
cmake -DCMAKE_BUILD_TYPE=Release -DMIXERX_ENABLE_GPL=ON ..
```
All supported components will be enabled in condition they are installed at the
system. This version you can link with GPL-only applications. The final license
of the library will be printed out after running CMake command.


### Build with AudioCodecs
If you want to build the library with the complete functionality and you don't
want or can't install any libraries into system, you can enable use of the
[AudioCodecs](https://github.com/WohlSoft/AudioCodecs) repository which is
a collection of audio codec libraries bundled into single portable build project.
This way is very useful on platforms such as Windows, mobile platforms such as
Android where is required to build all dependencies from the source separately,
or homebrew console toolchains with the similar case. You aren't required to
build and install separately, you can enable two CMake options to let CMake do
all hard work instead of you. You will need to add `-DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON`
and `-DAUDIO_CODECS_BUILD_LOCAL_SDL2=ON` options into your CMake command line
to enable use of AudioCodecs external package. This will allow you to build
the library even SDL2 (the required dependency) is not installed at your system
at all, or there is an older version than required minimum.


#### General build on UNIX-like platform and install into the system
The library will be built and installed into system in usual way
```bash
# Prepare the build directory
mkdir build
cd build

# Configure the project
cmake -DCMAKE_BUILD_TYPE=Release -DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON -DAUDIO_CODECS_BUILD_LOCAL_SDL2=ON ..

# Run the build
make -j 4 #where 4 - set number of CPU cores you have

# Install the built library and headers into the system
make install
```

#### Build with installing into custom location
If you don't want install the library into system, you may specify the different path for the
library installation and then, copy it into another location or refer it directly at the project:

```bash
# Prepare the build directory
mkdir build
cd build

# Configure the project
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=output -DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON -DAUDIO_CODECS_BUILD_LOCAL_SDL2=ON ..

# Run the build
cmake --build . --config Release -- j 4 #where 4 - set number of CPU cores you have

# Install the built library and headers into the system
make install
```
After run, the library, all dependencies, and headers will appear at the "output"
sub-directory at the "build" directory inside the repository. You can refer
use its content in your project directly.


#### Use standalone copy of AudioCodecs repository
You may totally avoid any network access during build if you make the full copy of the
[AudioCodecs](https://github.com/WohlSoft/AudioCodecs) repository and when you build it separately.
```bash
# Clonning both repositories (and in the next time, pack them into archive and take them at any time you want to use them)
git clone https://github.com/WohlSoft/AudioCodecs.git
git clone https://github.com/WohlSoft/SDL-Mixer-X.git

# Building AudioCodecs
cd AudioCodecs
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_LOCAL_SDL2=ON -DCMAKE_INSTALL_PREFIX=../../mixerx-install ..
make -j 5
make install
cd ../..

# Building MixerX and re-use libraries built at AudioCodecs
cd SDL-Mixer-X
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../mixerx-install -DAUDIO_CODECS_INSTALL_PATH=../../mixerx-install -DAUDIO_CODECS_REPO_PATH=../../AudioCodecs ..
make -j 5
make install
```

So, the complete build will appear at the "mixerx-install" directory among both AudioCodecs and SDL_Mixer_X repositories.


# How to use library

## Include
The API is backward-compatible with original SDL2_mixer and can be used as a drop-in replacement of original SDL2_mixer with exception of a different header name
```cpp
#include <SDL2/SDL_mixer_ext.h>
```
and different linked library name `SDL2_mixer_ext`.

In addition, there are new API calls which can be used to take full ability of the MixerX fork.


## Linking (Linux)

### Dynamically
```bash
gcc playmus.c -o playmus -I/usr/local/include -L/usr/local/lib -lSDL2_mixer_ext -lSDL2 -lstdc++
```

### Statically
To get it linked you should also link all dependencies of MixerX library itself and also dependencies of SDL2 too
```
gcc playmus.c -o playmus -I/usr/local/include -L/usr/local/lib -Wl,-Bstatic -lSDL2_mixer_ext -lopusfile -lopus -logg -lxmp -lmodplug -lADLMIDI -lOPNMIDI -lEDMIDI -lgme -lzlib -lSDL2 -Wl,-Bdynamic -lpthread -lm -ldl -static-libgcc -lstdc++
```

# Documentation
* [Full API documentation](https://wohlsoft.github.io/SDL-Mixer-X/SDL_mixer_ext.html)
* [Moondust-Wiki description page](https://wohlsoft.ru/pgewiki/SDL_Mixer_X)
