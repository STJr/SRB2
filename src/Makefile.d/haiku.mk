#
# Makefile options for Haiku
#

opts+=-DUNIXCOMMON -DLUA_USE_POSIX

ifndef DEDICATED
ifndef DUMMY
SDL?=1
DEDICATED?=0
endif
endif

NOEXECINFO=1

ifeq (${SDL},1)
EXENAME?=srb2haiku
else ifeq (${DEDICATED},1)
EXENAME?=srb2haikud
endif

ifndef NONET
libs+=-lnetwork
endif

define _set =
$(1)_CFLAGS?=$($(1)_opts)
$(1)_LDFLAGS?=$($(1)_libs)
endef

lib:=../libs/gme
LIBGME_opts:=-I$(lib)/include
LIBGME_libs:=-l:libgme.so.0
$(eval $(call _set,LIBGME))

lib:=../libs/libopenmpt
LIBOPENMPT_opts:=-I$(lib)/inc
LIBOPENMPT_libs:=-l:libopenmpt.so.0
$(eval $(call _set,LIBOPENMPT))

ifdef SDL
lib:=../libs/SDL2_mixer
mixer_opts:=-I$(lib)/include
mixer_libs:=-l:libSDL2_mixer-2.0.so.0

lib:=../libs/SDL2
SDL_opts:=-I$(lib)/include $(mixer_opts)
SDL_libs:=$(mixer_libs) -l:libSDL2-2.0.so.0
$(eval $(call _set,SDL))
endif

lib:=../libs/zlib
ZLIB_opts:=-I$(lib)
ZLIB_libs:=-l:libz.so.1
$(eval $(call _set,ZLIB))

ifndef PNG_CONFIG
PNG_opts:=
PNG_libs:=-l:libpng16.so.16
$(eval $(call _set,PNG))
endif

lib:=../libs/curl
CURL_opts:=-I$(lib)/include
CURL_libs:=-l:libcurl.so.4
$(eval $(call _set,CURL))

lib:=../libs/miniupnpc
MINIUPNPC_opts:=-I$(lib)/include
MINIUPNPC_libs:=-l:libminiupnpc.so.17
$(eval $(call _set,MINIUPNPC))
