makedir:=$(makedir)/Dedicated

sources+=$(call List,dedicated/Sourcefile)

opts+=-DDEDICATED

ifdef FREEBSD
# on FreeBSD, we have to link to libpthread explicitly
libs+=-lpthread
endif

ifndef NOTHREADS
opts+=-DHAVE_THREADS
sources+=dedicated/i_threads.c
endif

NOOPENMPT=1
NOHW=1
