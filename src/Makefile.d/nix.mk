#
# Makefile options for unices (linux, bsd...)
#

EXENAME?=lsdl2srb2

opts+=-DUNIXCOMMON -DLUA_USE_POSIX
# Use -rdynamic so a backtrace log shows function names
# instead of addresses
libs+=-lm -rdynamic

ifndef nasm_format
nasm_format:=elf -DLINUX
endif

ifndef NOHW
opts+=-I/usr/X11R6/include
libs+=-L/usr/X11R6/lib
endif

SDL?=1

# In common usage.
ifdef LINUX
libs+=-lrt
passthru_opts+=NOTERMIOS
endif

# Tested by Steel, as of release 2.2.8.
ifdef FREEBSD
opts+=-I/usr/X11R6/include -DLINUX -DFREEBSD
libs+=-L/usr/X11R6/lib -lipx -lkvm
endif

# FIXME: UNTESTED
#ifdef SOLARIS
#NOIPX=1
#NOASM=1
#opts+=-I/usr/local/include -I/opt/sfw/include \
#		-DSOLARIS -DINADDR_NONE=INADDR_ANY -DBSD_COMP
#libs+=-L/opt/sfw/lib -lsocket -lnsl
#endif
