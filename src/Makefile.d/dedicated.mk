makedir:=$(makedir)/Dedicated

sources+=$(call List,dedicated/Sourcefile)

opts+=-DDEDICATED

ifdef FREEBSD
# on FreeBSD, we have to link to libpthread explicitly
libs+=-lpthread
endif

ifdef MINGW
libs+=-mconsole
endif

ifndef NOTHREADS
opts+=-DHAVE_THREADS
sources+=dedicated/i_threads.c
endif

NOOPENMPT=1
NOGME=1
NOHW=1
NOUPNP=1
