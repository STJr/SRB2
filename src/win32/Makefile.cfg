#
# win32/Makefile.cfg for SRB2/Minwgw
#

#
#Mingw, if you don't know, that's Win32/Win64
#

ifdef MINGW64
	HAVE_LIBGME=1
	LIBGME_CFLAGS=-I../libs/gme/include
	LIBGME_LDFLAGS=-L../libs/gme/win64 -lgme
ifdef HAVE_OPENMPT
	LIBOPENMPT_CFLAGS?=-I../libs/libopenmpt/inc
	LIBOPENMPT_LDFLAGS?=-L../libs/libopenmpt/lib/x86_64/mingw -lopenmpt
endif
ifndef NOMIXERX
	HAVE_MIXERX=1
	SDL_CFLAGS?=-I../libs/SDL2/x86_64-w64-mingw32/include/SDL2 -I../libs/SDLMixerX/x86_64-w64-mingw32/include/SDL2 -Dmain=SDL_main
	SDL_LDFLAGS?=-L../libs/SDL2/x86_64-w64-mingw32/lib -L../libs/SDLMixerX/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
else
	SDL_CFLAGS?=-I../libs/SDL2/x86_64-w64-mingw32/include/SDL2 -I../libs/SDL2_mixer/x86_64-w64-mingw32/include/SDL2 -Dmain=SDL_main
	SDL_LDFLAGS?=-L../libs/SDL2/x86_64-w64-mingw32/lib -L../libs/SDL2_mixer/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
endif
else
	HAVE_LIBGME=1
	LIBGME_CFLAGS=-I../libs/gme/include
	LIBGME_LDFLAGS=-L../libs/gme/win32 -lgme
ifdef HAVE_OPENMPT
	LIBOPENMPT_CFLAGS?=-I../libs/libopenmpt/inc
	LIBOPENMPT_LDFLAGS?=-L../libs/libopenmpt/lib/x86/mingw -lopenmpt
endif
ifndef NOMIXERX
	HAVE_MIXERX=1
	SDL_CFLAGS?=-I../libs/SDL2/i686-w64-mingw32/include/SDL2 -I../libs/SDLMixerX/i686-w64-mingw32/include/SDL2 -Dmain=SDL_main
	SDL_LDFLAGS?=-L../libs/SDL2/i686-w64-mingw32/lib -L../libs/SDLMixerX/i686-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
else
	SDL_CFLAGS?=-I../libs/SDL2/i686-w64-mingw32/include/SDL2 -I../libs/SDL2_mixer/i686-w64-mingw32/include/SDL2 -Dmain=SDL_main
	SDL_LDFLAGS?=-L../libs/SDL2/i686-w64-mingw32/lib -L../libs/SDL2_mixer/i686-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
endif
endif

ifndef NOASM
	USEASM=1
endif

ifndef NONET
ifndef MINGW64 #miniupnc is broken with MINGW64
	HAVE_MINIUPNPC=1
endif
endif

	OPTS=-DSTDC_HEADERS

ifndef GCC44
	#OPTS+=-mms-bitfields
endif

	LIBS+=-ladvapi32 -lkernel32 -lmsvcrt -luser32
ifdef MINGW64
	LIBS+=-lws2_32
else
ifdef NO_IPV6
	LIBS+=-lwsock32
else
	LIBS+=-lws2_32
endif
endif

	# name of the exefile
	EXENAME?=srb2win.exe

ifdef SDL
	i_system_o+=$(OBJDIR)/SRB2.res
	#i_main_o+=$(OBJDIR)/win_dbg.o
ifndef NOHW
	OPTS+=-DUSE_WGL_SWAP
endif
endif


ZLIB_CFLAGS?=-I../libs/zlib
ifdef MINGW64
ZLIB_LDFLAGS?=-L../libs/zlib/win32 -lz64
else
ZLIB_LDFLAGS?=-L../libs/zlib/win32 -lz32
endif

ifndef NOPNG
ifndef PNG_CONFIG
	PNG_CFLAGS?=-I../libs/libpng-src
ifdef MINGW64
	PNG_LDFLAGS?=-L../libs/libpng-src/projects -lpng64
else
	PNG_LDFLAGS?=-L../libs/libpng-src/projects -lpng32
endif #MINGW64
endif #PNG_CONFIG
endif #NOPNG

ifdef GETTEXT
ifndef CCBS
	MSGFMT?=../libs/gettext/bin32/msgfmt.exe
endif
ifdef MINGW64
	CPPFLAGS+=-I../libs/gettext/include64
	LDFLAGS+=-L../libs/gettext/lib64
	LIBS+=-lmingwex
else
	CPPFLAGS+=-I../libs/gettext/include32
	LDFLAGS+=-L../libs/gettext/lib32
	STATIC_GETTEXT=1
endif #MINGW64
ifdef STATIC_GETTEXT
	LIBS+=-lasprintf -lintl
else
	LIBS+=-lintl.dll
endif #STATIC_GETTEXT
endif #GETTEXT

ifdef HAVE_MINIUPNPC
	CPPFLAGS+=-I../libs/ -DSTATIC_MINIUPNPC
ifdef MINGW64
	LDFLAGS+=-L../libs/miniupnpc/mingw64
else
	LDFLAGS+=-L../libs/miniupnpc/mingw32
endif #MINGW64
endif

ifndef NOCURL
	CURL_CFLAGS+=-I../libs/curl/include
ifdef MINGW64
	CURL_LDFLAGS+=-L../libs/curl/lib64 -lcurl
else
	CURL_LDFLAGS+=-L../libs/curl/lib32 -lcurl
endif #MINGW64
endif
