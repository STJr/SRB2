Here it is! SRB2 v2.0 source code!


Win32 with Visual C (6SP6+Processor Pack OR 7)
~~~

2 VC++ 6.0 project files are included:

Win32/DirectX/FMOD
src\win32\wLegacy.dsw
You'll need FMOD to compile this version (www.fmod.org)
or
Win32/SDL/SDL_mixer
src\sdl\Win32SDL.dsp
You'll need SDL and SDL_mixer for this version (www.libsdl.org)

Both needs NASM (http://sourceforge.net/projects/nasm)
For PNG screenshot, libPNG, and Zlib (from http://gnuwin32.sourceforge.net/)

No warranty, support, etc. of any kind is offered,
just plain old as is.
Some bits of code are still really scary.
Go nuts!


Win32 with Dev-C++ (http://bloodshed.net/ free!)
~~~
2 Dev-C++ project files are included:

Win32/DirectX/FMOD
src\win32\SRB2.dev
or
Win32/SDL/SDL_mixer
src\sdl\Win32SDL.dev
You'll need SDL and SDL_mixer for this version (www.libsdl.org)
libPNG and Zlib (from http://gnuwin32.sourceforge.net/)
Note there are precompiled libpng.a and libz.a for Mingw

you will need NASM for both SDL/SDL_mixer and DirectX/FMOD
and you need DirectX 6 (or up) Dev-Paks to compile DirectX version

GNU/Linux
~~~

Dependencies:
  SDL 1.2.7 or better (from libsdl.org)
  SDL_Mixer 1.2.2(.7 for file-less music playback) (from libsdl.org)
  Nasm (use NOASM=1 if you don't have it or have an non-i386 system, I think)
  libPNG 1.2.7
  Zlib 1.2.3
  The Xiph.org libogg and libvorbis libraries
  The OpenGL headers (from Mesa, usually shipped with your X.org or XFree
    installation, so you needn't worry, most likely)
  GCC 3.x toolchain and binutils
  GNU Make

Build instructions:

make -C src LINUX=1

Build instructions to build for Wii Linux/SRB2Wii on a PowerPC system,
follow cross-compiling instructions for cross-compiling on a x86 system:

make -C src LINUX=1 WIILINUX=1

Build instructions to build for Pandora (Linux) on a ARM system,
follow cross-compiling instructions for cross-compiling on a x86 system:

make -C src PANDORA=1

Solaris
~~~

Dependencies:
  SDL 1.2.5 or better (from libsdl.org)
  SDL_Mixer 1.2.2(.7 for file-less music playback) (from libsdl.org)
  libPNG 1.2.7
  Zlib 1.2.3
  The Xiph.org libogg and libvorbis libraries
  The OpenGL headers (from Mesa, usually shipped with your X.org or XFree
    installation, so you needn't worry, most likely)
  GCC 3.x toolchain and binutils
  GNU Make

  You can get all these programs/libraries from the Companion CD (except SDL_mixer and OpenGL)

Build instructions:

gmake -C src SOLARIS=1

FreeBSD
~~~

Dependencies:
  SDL 1.2.7 or better (from libsdl.org)
  SDL_Mixer 1.2.2(.7 for file-less music playback) (from libsdl.org)
  Nasm (use NOASM=1 if you don't have it or have an non-i386 system, I think)
  libPNG 1.2.7
  Zlib 1.2.3
  The Xiph.org libogg and libvorbis libraries
  The OpenGL headers (from Mesa, usually shipped with your X.org or XFree
    installation, so you needn't worry, most likely)
  GCC 3.x toolchain and binutils
  GNU Make

Build instructions:

gmake -C src FREEBSD=1

DJGPP/DOS
~~~

Dependencies:
  Allegro 3.12 game programming library, (from
  http://alleg.sourceforge.net/index.html)
  Nasm (use NOASM=1 if you don't have it)
  libsocket (from http://homepages.nildram.co.uk/~phekda/richdawe/lsck/) or
  Watt-32 (from http://www.bgnett.no/~giva/)
  GCC 3.x toolchain and binutils
  GNU Make

Build instructions:

make -C src # to link with Watt-32, add WATTCP=1
      # for remote debugging over the COM port, add RDB=1

Notes:
 use tools\djgpp\all313.diff to update Allegro to a "more usable" version ;)
 Example: E:\djgpp\allegro>patch -p# < D:\SRB2Code\1.1\srb2\tools\djgpp\all313.diff

Windows CE
~~~

Dependencies:
  SDL 1.27

Build instructions:

use src\SDL\WinCE\SRB2CE.vcw

-------------------------------------------------------------------------------

binaries will turn in up in bin/

note: read the src/makefile for more options

- Sonic Team Junior
http://www.srb2.org
