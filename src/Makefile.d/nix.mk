#
# sdl/makeNIX.cfg for SRB2/?nix
#

#Valgrind support
ifdef VALGRIND
VALGRIND_PKGCONFIG?=valgrind
VALGRIND_CFLAGS?=$(shell $(PKG_CONFIG) $(VALGRIND_PKGCONFIG) --cflags)
VALGRIND_LDFLAGS?=$(shell $(PKG_CONFIG) $(VALGRIND_PKGCONFIG) --libs)
ZDEBUG=1
LIBS+=$(VALGRIND_LDFLAGS)
ifdef GCC46
WFLAGS+=-Wno-error=unused-but-set-variable
WFLAGS+=-Wno-unused-but-set-variable
endif
endif

#
#here is GNU/Linux and other
#

	OPTS=-DUNIXCOMMON

	#LDFLAGS = -L/usr/local/lib
	LIBS=-lm
ifdef LINUX
	LIBS+=-lrt
ifdef NOTERMIOS
	OPTS+=-DNOTERMIOS
endif
endif

ifdef LINUX64
	OPTS+=-DLINUX64
endif

#
#here is Solaris
#
ifdef SOLARIS
	NOIPX=1
	NOASM=1
	OPTS+=-DSOLARIS -DINADDR_NONE=INADDR_ANY -DBSD_COMP
	OPTS+=-I/usr/local/include -I/opt/sfw/include
	LDFLAGS+=-L/opt/sfw/lib
	LIBS+=-lsocket -lnsl
endif

#
#here is FreeBSD
#
ifdef FREEBSD
	OPTS+=-DLINUX -DFREEBSD -I/usr/X11R6/include
	SDL_CONFIG?=sdl11-config
	LDFLAGS+=-L/usr/X11R6/lib
	LIBS+=-lipx -lkvm
endif

#
#here is Mac OS X
#
ifdef MACOSX
	OBJS+=$(OBJDIR)/mac_resources.o
	OBJS+=$(OBJDIR)/mac_alert.o
	LIBS+=-framework CoreFoundation
endif

ifndef NOHW
	OPTS+=-I/usr/X11R6/include
	LDFLAGS+=-L/usr/X11R6/lib
endif

	# name of the exefile
	EXENAME?=lsdl2srb2
