#
# Makefile options for SDL2 backend.
#

#
# SDL...., *looks at Alam*, THIS IS A MESS!
# 
# ...a little bird flexes its muscles...
#

makedir:=$(makedir)/SDL

sources+=$(call List,sdl/Sourcefile)
opts+=-DDIRECTFULLSCREEN -DHAVE_SDL

# FIXME: UNTESTED
#ifdef PANDORA
#include sdl/SRB2Pandora/Makefile.cfg
#endif #ifdef PANDORA

# FIXME: UNTESTED
#ifdef CYGWIN32
#include sdl/MakeCYG.cfg
#endif #ifdef CYGWIN32

ifndef NOHW
sources+=sdl/ogl_sdl.c
endif

ifdef NOMIXER
sources+=sdl/sdl_sound.c
else
opts+=-DHAVE_MIXER
sources+=sdl/mixer_sound.c

  ifdef HAVE_MIXERX
  opts+=-DHAVE_MIXERX
  libs+=-lSDL2_mixer_ext
  else
  libs+=-lSDL2_mixer
  endif
endif

ifndef NOTHREADS
opts+=-DHAVE_THREADS
sources+=sdl/i_threads.c
endif

ifdef SDL_PKGCONFIG
$(eval $(call Use_pkg_config,SDL))
else
SDL_CONFIG?=$(call Prefix,sdl2-config)
SDL_CFLAGS?=$(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS?=$(shell $(SDL_CONFIG) \
		$(if $(STATIC),--static-libs,--libs))
$(eval $(call Propogate_flags,SDL))
endif

# use the x86 asm code
ifndef CYGWIN32
ifndef NOASM
USEASM=1
endif
endif

ifdef MINGW
ifndef NOSDLMAIN
SDLMAIN=1
endif
endif

ifdef SDLMAIN
opts+=-DSDLMAIN
else
ifdef MINGW
opts+=-Umain
libs+=-mconsole
endif
endif
