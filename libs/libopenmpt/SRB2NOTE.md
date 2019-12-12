# libopenmpt mingw-w64 binary info

Current built version as of 2019/09/27 is 0.4.7+r12088.pkg

* mingw binaries (.dll): `bin/[x86 or x86_64]/mingw`
* mingw import libraries (.dll.a): `lib/[x86 or x86_64]/mingw`

# Building libopenmpt with mingw-w64

libopenmpt must be built from the Makefile / Android dev package in the
[Downloads page](https://lib.openmpt.org/libopenmpt/download/#all-downloads)

Use the mingw-w64 distributions from
[SourceForge](https://sourceforge.net/projects/mingw-w64/files/#readme).

You can download the appropriate 7-zip archive, extract to a folder of
your choice, remove any existing mingw directories from your PATH,
then call `mingw32-make.exe` from its direct location.

FOR LIBOPENMPT, YOU MUST USE A MINGW PACKAGE THAT SUPPORTS THE POSIX
THREADING MODEL! DO NOT COMPILE WITH A WIN32 THREADING MODEL!

I use GCC 7.3.0:

* [x86_64-posix-seh](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/7.3.0/threads-posix/seh/x86_64-7.3.0-release-posix-seh-rt_v5-rev0.7z)
* [i686-posix-dwarf](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/7.3.0/threads-posix/dwarf/i686-7.3.0-release-posix-dwarf-rt_v5-rev0.7z)

## x86 build instructions

```
set CFLAGS=-march=pentium -static-libgcc
set CXXFLAGS=-march=pentium -static-libgcc -static-libstdc++
set LDFLAGS=-Wl,--out-implib,bin/libopenmpt.dll.a -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,-Bdynamic,--no-whole-archive

cd [libopenmpt-make-src]
[x86-mingw-w64-bin]/mingw32-make.exe CONFIG=mingw64-win32
```

`libopenmpt.dll` and `libopenmpt.dll.a` will be built in the
`bin/` folder.

## x86_64 build instructions

```
set CFLAGS=-march=nocona -static-libgcc
set CXXFLAGS=-march=nocona -static-libgcc -static-libstdc++
set LDFLAGS=-Wl,--out-implib,bin/libopenmpt.dll.a -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,-Bdynamic,--no-whole-archive

cd [libopenmpt-make-src]
[x86_64-mingw-w64-bin]/mingw32-make.exe CONFIG=mingw64-win64
```

`libopenmpt.dll` and `libopenmpt.dll.a` will be built in the
`bin/` folder.