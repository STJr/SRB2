#
# Detect the host system and compiler version.
#

# Previously featured:\
	PANDORA\
	DUMMY\
	DJGPPDOS\
	SOLARIS\
	MACOSX\

all_systems:=\
	LINUX64\
	MINGW64\
	MINGW\
	UNIX\
	LINUX\
	FREEBSD\
	HAIKU\

# check for user specified system
ifeq (,$(filter $(all_systems),$(.VARIABLES)))
ifeq ($(OS),Windows_NT) # all windows are Windows_NT...

_m=Detected a Windows system,\
	compiling for 32-bit MinGW SDL...)
$(call Print,$(_m))

# go for a 32-bit sdl mingw exe by default
MINGW:=1

else # if you on the *nix

system:=$(shell uname -s)

ifeq ($(system),Linux)
new_system:=LINUX
else ifeq ($(system),Haiku)
new_system:=HAIKU
else

$(error \
	Could not automatically detect your system,\
	try specifying a system manually)

endif

ifeq ($(shell getconf LONG_BIT),64)
system+=64-bit
new_system:=$(new_system)64
endif

$(call Print,Detected $(system) ($(new_system))...)
$(new_system):=1

endif
endif

# This must have high to low order.
gcc_versions:=\
	132 131 130\
	123 122 121 120\
	114 113 112 111 110\
	105 104 103 102 101 100\
	95 94 93 92 91 90\
	85 84 83 82 81 80\
	75 74 73 72 71 70\
	64 63 62 61 60\
	55 54 53 52 51 50\
	49 48 47 46 45 44 43 42 41 40

latest_gcc_version:=13.2

# Automatically set version flag, but not if one was
# manually set. And don't bother if this is a clean only
# run.
ifeq (,$(call Wildvar,GCC% destructive))

# can't use $(CC) --version here since that uses argv[0] to display the name
# also gcc outputs the information to stderr, so I had to do 2>&1
# this program really doesn't like identifying itself
shellversion:=$(shell $(CC) -v 2>&1)
# Try to remove "-win32"
version:=$(subst -win32,.0,$(shellversion))

# check if this is in fact GCC
ifneq (,$(findstring gcc version,$(version)))

# in stark contrast to the name, gcc will give me a nicely formatted version number for free
shellversion:=$(shell $(CC) -dumpfullversion)

# Try to remove "-win32"
version:=$(subst -win32,.0,$(shellversion))

# Turn version into words of major, minor
v:=$(subst ., ,$(version))
# concat. major minor
v:=$(word 1,$(v))$(word 2,$(v))

# If this version is not in the list,
# default to the latest supported
ifeq (,$(filter $(v),$(gcc_versions)))
define line =
Your compiler version, GCC $(version), \
is not supported by the Makefile.
The Makefile will assume GCC $(latest_gcc_version).
endef
$(call Print,$(line))
GCC$(subst .,,$(latest_gcc_version)):=1
else
$(call Print,Detected GCC $(version) (GCC$(v)))
GCC$(v):=1
endif

endif
endif
