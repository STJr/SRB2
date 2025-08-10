makedir:=$(makedir)/Dedicated

sources+=$(call List,dedicated/Sourcefile)

opts+=-DDEDICATED

ifdef LINUX
# necessary for older distributions
libs+=-lpthread
endif

ifdef FREEBSD
# on FreeBSD, we have to link to libpthread explicitly
libs+=-lpthread
endif

ifdef MINGW
libs+=-mconsole
endif

sources+=dedicated/i_threads.c

NOOPENMPT=1
NOGME=1
NOHW=1
NOUPNP=1
