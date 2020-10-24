#
# sdl/makefile.cfg for SRB2/SDL
#

#
#SDL...., *looks at Alam*, THIS IS A MESS!
#

ifdef UNIXCOMMON
include sdl/MakeNIX.cfg
endif

ifdef PANDORA
include sdl/SRB2Pandora/Makefile.cfg
endif #ifdef PANDORA

ifdef CYGWIN32
include sdl/MakeCYG.cfg
endif #ifdef CYGWIN32

ifdef SDL_PKGCONFIG
SDL_CFLAGS?=$(shell $(PKG_CONFIG) $(SDL_PKGCONFIG) --cflags)
SDL_LDFLAGS?=$(shell $(PKG_CONFIG) $(SDL_PKGCONFIG) --libs)
else
ifdef PREFIX
	SDL_CONFIG?=$(PREFIX)-sdl2-config
else
	SDL_CONFIG?=sdl2-config
endif

ifdef STATIC
	SDL_CFLAGS?=$(shell $(SDL_CONFIG) --cflags)
	SDL_LDFLAGS?=$(shell $(SDL_CONFIG) --static-libs)
else
	SDL_CFLAGS?=$(shell $(SDL_CONFIG) --cflags)
	SDL_LDFLAGS?=$(shell $(SDL_CONFIG) --libs)
endif
endif


	#use the x86 asm code
ifndef CYGWIN32
ifndef NOASM
	USEASM=1
endif
endif

	OBJS+=$(OBJDIR)/i_video.o $(OBJDIR)/dosstr.o $(OBJDIR)/endtxt.o $(OBJDIR)/hwsym_sdl.o

	OPTS+=-DDIRECTFULLSCREEN -DHAVE_SDL

ifndef NOHW
	OBJS+=$(OBJDIR)/r_opengl.o $(OBJDIR)/ogl_sdl.o
endif

ifdef NOMIXER
	i_sound_o=$(OBJDIR)/sdl_sound.o
else
	i_sound_o=$(OBJDIR)/mixer_sound.o
	OPTS+=-DHAVE_MIXER
ifdef HAVE_MIXERX
	OPTS+=-DHAVE_MIXERX
	SDL_LDFLAGS+=-lSDL2_mixer_ext
else
	SDL_LDFLAGS+=-lSDL2_mixer
endif
endif

ifndef NOTHREADS
	OPTS+=-DHAVE_THREADS
	OBJS+=$(OBJDIR)/i_threads.o
endif

ifdef SDL_TTF
	OPTS+=-DHAVE_TTF
	SDL_LDFLAGS+=-lSDL2_ttf -lfreetype -lz
	OBJS+=$(OBJDIR)/i_ttf.o
endif

ifdef SDL_IMAGE
	OPTS+=-DHAVE_IMAGE
	SDL_LDFLAGS+=-lSDL2_image
endif

ifdef SDL_NET
	OPTS+=-DHAVE_SDLNET
	SDL_LDFLAGS+=-lSDL2_net
endif

ifdef MINGW
ifndef NOSDLMAIN
	SDLMAIN=1
endif
endif

ifdef SDLMAIN
	OPTS+=-DSDLMAIN
else
ifdef MINGW
	SDL_CFLAGS+=-Umain
	SDL_LDFLAGS+=-mconsole
endif
endif

ifndef NOHW
ifdef OPENAL
ifdef MINGW
	LIBS:=-lopenal32 $(LIBS)
else
	LIBS:=-lopenal $(LIBS)
endif
else
ifdef MINGW
ifdef DS3D
	LIBS:=-ldsound -luuid $(LIBS)
endif
endif
endif
endif

CFLAGS+=$(SDL_CFLAGS)
LIBS:=$(SDL_LDFLAGS) $(LIBS)
ifdef STATIC
	LIBS+=$(shell $(SDL_CONFIG) --static-libs)
endif
