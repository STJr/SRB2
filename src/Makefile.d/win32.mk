#
# Mingw, if you don't know, that's Win32/Win64
#

ifndef MINGW64
EXENAME?=srb2win.exe
else
EXENAME?=srb2win64.exe
endif

# disable dynamicbase if under msys2
ifdef MSYSTEM
libs+=-Wl,--disable-dynamicbase
endif

sources+=win32/Srb2win.rc
opts+=-DSTDC_HEADERS
libs+=-ladvapi32 -lkernel32 -lmsvcrt -luser32

nasm_format:=win32

SDL?=1

ifndef NOHW
opts+=-DUSE_WGL_SWAP
endif

ifdef MINGW64
libs+=-lws2_32
else
ifdef NO_IPV6
libs+=-lwsock32
else
libs+=-lws2_32
endif
endif

ifndef NONET
ifndef MINGW64 # miniupnc is broken with MINGW64
opts+=-I../libs -DSTATIC_MINIUPNPC
libs+=-L../libs/miniupnpc/mingw$(32) -lws2_32 -liphlpapi
endif
endif

ifndef MINGW64
32=32
x86=x86
i686=i686
else
32=64
x86=x86_64
i686=x86_64
endif

mingw:=$(i686)-w64-mingw32

define _set =
$(1)_CFLAGS?=$($(1)_opts)
$(1)_LDFLAGS?=$($(1)_libs)
endef

lib:=../libs/gme
LIBGME_opts:=-I$(lib)/include
LIBGME_libs:=-L$(lib)/win$(32) -lgme
$(eval $(call _set,LIBGME))

lib:=../libs/libopenmpt
LIBOPENMPT_opts:=-I$(lib)/inc
LIBOPENMPT_libs:=-L$(lib)/lib/$(x86)/mingw -lopenmpt
$(eval $(call _set,LIBOPENMPT))

ifndef NOMIXERX
HAVE_MIXERX=1
lib:=../libs/SDLMixerX/$(mingw)
else
lib:=../libs/SDL2_mixer/$(mingw)
endif

ifdef SDL
mixer_opts:=-I$(lib)/include/SDL2
mixer_libs:=-L$(lib)/lib

lib:=../libs/SDL2/$(mingw)
SDL_opts:=-I$(lib)/include/SDL2\
	$(mixer_opts) -Dmain=SDL_main
SDL_libs:=-L$(lib)/lib $(mixer_libs)\
	-lmingw32 -lSDL2main -lSDL2 -mwindows
$(eval $(call _set,SDL))
endif

lib:=../libs/zlib
ZLIB_opts:=-I$(lib)
ZLIB_libs:=-L$(lib)/win32 -lz$(32)
$(eval $(call _set,ZLIB))

ifndef PNG_CONFIG
lib:=../libs/libpng-src
PNG_opts:=-I$(lib)
PNG_libs:=-L$(lib)/projects -lpng$(32)
$(eval $(call _set,PNG))
endif

lib:=../libs/curl
CURL_opts:=-I$(lib)/include
CURL_libs:=-L$(lib)/lib$(32) -lcurl
$(eval $(call _set,CURL))
