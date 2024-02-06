SDL Mixer X (aka SDL Mixer 2.0 Modded or SDL_mixer_ext),
by Vitaly Novichkov <Wohlstand>,
forked from SDL Mixer 2.0 by Sam Lantinga <slouken@libsdl.org>

vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
WARNING: The licenses for libADLMIDI, libOPNMIDI, and GME is GPL,
         which means that in order to use it your application must
         also be GPL!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The latest original version of this library is available from:
http://www.libsdl.org/projects/SDL_mixer/

Sources of modified library version is available here:
https://github.com/WohlSoft/SDL-Mixer-X


=============================================================================
Please read the updated ReadMe at the `docs/index.md` and the full documentation
at the docs/SDL_mixer_ext.html
=============================================================================

Due to popular demand, here is a simple multi-channel audio mixer.
It supports 8 channels of 16 bit stereo audio, plus a single channel
of music, mixed by the libXMP, Modplug MOD, Timidity MIDI, libADLMIDI,
libOPNMIDI, FluidSynth, FluidLite, GME, and drmp3 MP3 libraries.

See the header file SDL_mixer_ext.h for documentation on this mixer library.

The mixer can currently load Microsoft WAVE files and Creative Labs VOC
files as audio samples, and can load MIDI files via Timidity, libADLMIDI,
libOPNMIDI, or by FluidSynth, and the following music formats via libXMP,
libModPlug:  .MOD .S3M .IT .XM, and many other formats. It can load
Ogg Vorbis streams as music if built with Ogg Vorbis, Tremor, or stb-vrobis
libraries, load Opus, FLAC, and finally it can load MP3 music
using the drmp3 or libmpg123 libraries.

Tremor decoding is disabled by default; you can enable it by passing
	-DUSE_OGG_VORBIS=ON -DUSE_OGG_VORBIS_TREMOR=ON
to CMake, or you can tell the library to use the internally-included stb-vorbis
by passing
	-DUSE_OGG_VORBIS=ON -DUSE_OGG_VORBIS_STB=ON
to CMake.

All other flags and codecs you can find using the CMake configure tools like the
cmake-gui or ccmake.

vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
WARNING: The license for libADLMIDI, and libOPNMIDI is GPL,
         which means that in order to use them your application must
         also be GPL!
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The process of mixing MIDI files to wave output is very CPU intensive,
so if playing regular WAVE files sound great, but playing MIDI files
sound choppy on slow computers, try using 8-bit audio, mono audio,
or lower frequencies.

To play MIDI files via Timidity, you'll need to get a complete set of GUS patches
from:
http://www.libsdl.org/projects/mixer/timidity/timidity.tar.gz
and unpack them in /usr/local/lib under UNIX, and C:\ under Win32.

This library is under the zlib license, see the file "COPYING.txt" for details.
