
#     GNU Make makefile for SRB2
#############################################################################
# Copyright (C) 1998-2000 by DooM Legacy Team.
# Copyright (C) 2003-2014 by Sonic Team Junior.
#
# This program is free software distributed under the
# terms of the GNU General Public License, version 2.
# See the 'LICENSE' file for more details.
#
#     -DPC_DOS    -> use DOS specific code (eg:textmode stuff)...
#     -DLINUX     -> use for the GNU/Linux specific
#     -D_WINDOWS  -> use for the Win32/DirectX specific
#     -DHAVE_SDL  -> use for the SDL interface
#
# Sets:
#     Compile the DGJPP/DOS version with 'make WATTCP=1'
#     Compile the DirectX/Mingw version with 'make MINGW=1'
#     Compile the SDL/Mingw version with 'make MINGW=1 SDL=1'
#     Compile the SDL/Linux version with 'make LINUX=1'
#     Compile the SDL/Solaris version with 'make SOLARIS=1'
#     Compile the SDL/FreeBSD version with 'gmake FREEBSD=1'
#     Compile the SDL/Cygwin version with 'make CYGWIN32=1'
#     Compile the SDL/other version try with 'make SDL=1'
#
# 'Targets':
#     clean
#       Remove all object files
#     cleandep
#       Remove depend.dep
#     dll
#       compile primary HW render DLL/SO
#     all_dll
#       compile all HW render and 3D sound DLLs for the set
#     opengl_dll
#       Pure Mingw only, compile OpenGL HW render DLL
#     minigl_dll
#       Pure Mingw only, compile MiniGL HW render DLL
#     ds3d_dll
#       Pure Mingw only, compile DirectX DirectSound HW sound DLL
#     fmod_dll
#       Pure Mingw only, compile FMOD HW sound DLL
#     openal_dll
#       Pure Mingw only, compile OpenAL HW sound DLL
#     fmod_so
#       Non-Mingw, compile FMOD HW sound SO
#     openal_so
#       Non-Mingw, compile OpenAL HW sound SO
#
#
# Addon:
#     To Cross-Compile, CC=gcc-version make * PREFIX=<dir>
#     Compile with GCC 2.97 version, add 'GCC29=1'
#     Compile with GCC 4.0x version, add 'GCC40=1'
#     Compile with GCC 4.1x version, add 'GCC41=1'
#     Compile with GCC 4.2x version, add 'GCC42=1'
#     Compile with GCC 4.3x version, add 'GCC43=1'
#     Compile with GCC 4.4x version, add 'GCC44=1'
#     Compile with GCC 4.5x version, add 'GCC45=1'
#     Compile with GCC 4.6x version, add 'GCC46=1'
#     Compile a profile version, add 'PROFILEMODE=1'
#     Compile a debug version, add 'DEBUGMODE=1'
#     Compile with extra warnings, add 'WARNINGMODE=1'
#     Compile without NASM's tmap.nas, add 'NOASM=1'
#     Compile without 3D hardware support, add 'NOHW=1'
#     Compile without 3D sound support, add 'NOHS=1'
#     Compile with GDBstubs, add 'RDB=1'
#     Compile without PNG, add 'NOPNG=1'
#
# Addon for SDL:
#     To Cross-Compile, add 'SDL_CONFIG=/usr/*/bin/sdl-config'
#     Compile without SDL_Mixer, add 'NOMIXER=1'
#     Compile without BSD API, add 'NONET=1'
#     Compile without IPX/SPX, add 'NOIPX=1'
#     Compile Mingw/SDL with S_DS3S, add 'DS3D=1'
#     Compile with S_FMOD3D, add 'FMOD=1' (WIP)
#     Compile with S_OPENAL, add 'OPENAL=1' (WIP)
#     To link with the whole SDL_Image lib to load Icons, add 'SDL_IMAGE=1' but it isn't not realy needed
#     To link with SDLMain to hide console or make on a console-less binary, add 'SDLMAIN=1'
#
#############################################################################

# SRB2 data files
D_DIR?=../bin/Resources
D_FILES=$(D_DIR)/srb2.srb \
	$(D_DIR)/player.dta \
	$(D_DIR)/rings.wpn \
	$(D_DIR)/drill.dta \
	$(D_DIR)/soar.dta \
	$(D_DIR)/zones.dta \
	$(D_DIR)/music.dta \

PKG_CONFIG?=pkg-config

ifdef WIILINUX
LINUX=1
endif

ifdef PANDORA
LINUX=1
endif

ifdef LINUX64
LINUX=1
NONX86=1
endif

ifdef HAIKU
SDL=1
endif

ifdef NDS
# Include this before the main Makefile.cfg
EXENAME?=srb2.elf
include nds/Makefile.cfg
endif

include Makefile.cfg

ifdef DUMMY
NOPNG=1
NONET=1
NOHW=1
NOHS=1
NOASM=1
NOIPX=1
EXENAME?=srb2dummy
OBJS=$(OBJDIR)/i_video.o
LIBS=-lm
endif

ifdef HAIKU
NOIPX=1
NOASM=1
ifndef NONET
LIBS=-lnetwork
endif
CFLAGS+=-DUNIXCOMMON
PNG_CFLAGS?=
PNG_LDFLAGS?=-lpng
endif

ifdef WIILINUX
NONX86=1
NOTERMIOS=1
NOHW=1
CFLAGS+=-DWMINPUT
NOTERMIOS=1
NOPOSTPROCESSING=1
endif

ifdef PANDORA
NONX86=1
NOHW=1
NOHS=1
endif

ifdef WII
NONX86=1
NOHW=1
NOPOSTPROCESSING=1
endif

ifdef PS3N
NONX86=1
NOHW=1
endif

ifdef DJGPPDOS
include djgppdos/Makefile.cfg
endif

ifdef MINGW
include win32/Makefile.cfg
endif #ifdef MINGW

ifdef UNIX
UNIXCOMMON=1
endif

ifdef LINUX
UNIXCOMMON=1
ifndef NOGME
HAVE_LIBGME=1
endif
endif

ifdef SOLARIS
UNIXCOMMON=1
endif

ifdef FREEBSD
UNIXCOMMON=1
endif

ifdef MACOSX
UNIXCOMMON=1
endif

ifdef NDS
NOPNG=1
NONET=1
#NOHW=1
NOHS=1
NOASM=1
NOIPX=1
NONX86=1
OBJS+=$(OBJDIR)/i_video.o
LIBS+=-lm
endif

ifdef SDL
#SDL 2.0
ifndef SDL12
	include sdl/Makefile.cfg
#SDL 1.2
else
	include sdl12/Makefile.cfg
endif #ifndef SDL12
endif #ifdef SDL

ifdef DISTCC
        CC:=distcc $(CC)
endif

ifdef CCACHE
        CC:=ccache $(CC)
endif

MSGFMT?=msgfmt

ifndef ECHO
	NASM:=@$(NASM)
	REMOVE:=@$(REMOVE)
	CC:=@$(CC)
	CXX:=@$(CXX)
	OBJCOPY:=@$(OBJCOPY)
	OBJDUMP:=@$(OBJDUMP)
	STRIP:=@$(STRIP)
	WINDRES:=@$(WINDRES)
	CP:=@$(CP)
	MKDIR:=@$(MKDIR)
	MKISOFS:=@$(MKISOFS)
	DD:=@$(DD)
	NDSTOOL:=@$(NDSTOOL)
	GZIP:=@$(GZIP)
	MSGFMT:=@$(MSGFMT)
	UPX:=@$(UPX)
	UPX_OPTS+=-q
endif

ifdef NONET
	OPTS+=-DNONET
else
ifdef NO_IPV6
	OPTS+=-DNO_IPV6
endif
endif

ifdef NOHW
	OPTS+=-DNOHW
else
ifndef DC
	#Hurdler: not really supported and not tested recently
	#OPTS+=-DUSE_PALETTED_TEXTURE
endif
	OPTS+=-DHWRENDER
	OBJS+=$(OBJDIR)/hw_bsp.o $(OBJDIR)/hw_draw.o $(OBJDIR)/hw_light.o \
		 $(OBJDIR)/hw_main.o $(OBJDIR)/hw_md2.o $(OBJDIR)/hw_cache.o $(OBJDIR)/hw_trick.o
endif

ifdef NOHS
	OPTS+=-DNOHS
else
	OPTS+=-DHW3SOUND
	OBJS+=$(OBJDIR)/hw3sound.o
endif

OPTS += -DCOMPVERSION

ifndef NONX86
ifndef GCC29
	M5=-march=pentium
	M4=-march=i486
else
	M5=-mpentium
	M4=-m486
endif
endif

ifndef NOASM
ifndef NONX86
	OBJS+=$(OBJDIR)/tmap.o $(OBJDIR)/tmap_mmx.o
	OPTS+=-DUSEASM
endif
endif

ifndef NOPNG
OPTS+=-DHAVE_PNG

ifdef PNG_PKGCONFIG
PNG_CFLAGS?=$(shell $(PKG_CONFIG) $(PNG_PKGCONFIG) --cflags)
PNG_LDFLAGS?=$(shell $(PKG_CONFIG) $(PNG_PKGCONFIG) --libs)
else
ifdef PREFIX
PNG_CONFIG?=$(PREFIX)-libpng-config
else
PNG_CONFIG?=libpng-config
endif

ifdef PNG_STATIC
PNG_CFLAGS?=$(shell $(PNG_CONFIG) --static --cflags)
PNG_LDFLAGS?=$(shell $(PNG_CONFIG) --static --ldflags)
else
PNG_CFLAGS?=$(shell $(PNG_CONFIG) --cflags)
PNG_LDFLAGS?=$(shell $(PNG_CONFIG) --ldflags)
endif
endif

ifdef LINUX
PNG_CFLAGS+=-D_LARGEFILE64_SOURCE
endif

LIBS+=$(PNG_LDFLAGS)
CFLAGS+=$(PNG_CFLAGS)
endif

ZLIB_PKGCONFIG?=zlib
ZLIB_CFLAGS?=$(shell $(PKG_CONFIG) $(ZLIB_PKGCONFIG) --cflags)
ZLIB_LDFLAGS?=$(shell $(PKG_CONFIG) $(ZLIB_PKGCONFIG) --libs)

LIBS+=$(ZLIB_LDFLAGS)
CFLAGS+=$(ZLIB_CFLAGS)

ifdef HAVE_LIBGME
OPTS+=-DHAVE_LIBGME

LIBGME_PKGCONFIG?=libgme
LIBGME_CFLAGS?=$(shell $(PKG_CONFIG) $(LIBGME_PKGCONFIG) --cflags)
LIBGME_LDFLAGS?=$(shell $(PKG_CONFIG) $(LIBGME_PKGCONFIG) --libs)

LIBS+=$(LIBGME_LDFLAGS)
CFLAGS+=$(LIBGME_CFLAGS)
endif

ifdef STATIC
LIBS:=-static $(LIBS)
endif

ifdef HAVE_MINIUPNPC
ifdef NONET
HAVE_MINIUPNPC=''
else
LIBS+=-lminiupnpc
ifdef MINGW
LIBS+=-lws2_32 -liphlpapi
endif
CFLAGS+=-DHAVE_MINIUPNPC
endif
endif

ifndef NO_LUA
	include blua/Makefile.cfg
endif

ifdef NOMD5
	OPTS+=-DNOMD5
else
	OBJS:=$(OBJDIR)/md5.o $(OBJS)
endif

ifdef FAKEDC
	OPTS+=-DDC
endif

ifdef FAKEPSP
	OPTS+=-DPSP
endif

ifdef NOPOSTPROCESSING
	OPTS+=-DNOPOSTPROCESSING
endif

	OPTS:=-fno-exceptions $(OPTS)

ifdef MOBJCONSISTANCY
	OPTS+=-DMOBJCONSISTANCY
endif

ifdef PACKETDROP
	OPTS+=-DPACKETDROP
endif

ifdef DEBUGMODE

	# build with debugging information
	WINDRESFLAGS = -D_DEBUG
ifdef GCC48
	CFLAGS+=-Og
else
	CFLAGS+=-O0
endif
	CFLAGS+= -Wall -DPARANOIA -DRANGECHECK -DPACKETDROP -DMOBJCONSISTANCY
else


	# build a normal optimised version
	WINDRESFLAGS = -DNDEBUG
	CFLAGS+=-O3
endif
	CFLAGS+=-g $(OPTS) $(M5) $(WINDRESFLAGS)

ifdef YASM
ifdef STABS
	NASMOPTS?= -g stabs
else
	NASMOPTS?= -g dwarf2
endif
else
	NASMOPTS?= -g
endif

ifdef PROFILEMODE
	# build with profiling information
	CFLAGS+=-pg
	LDFLAGS+=-pg
endif

ifdef ZDEBUG
	CPPFLAGS+=-DZDEBUG
endif

OPTS+=$(CPPFLAGS)

# default EXENAME if all else fails
EXENAME?=srb2
DBGNAME?=$(EXENAME).debug

# $(OBJDIR)/dstrings.o \

# not too sophisticated dependency
OBJS:=$(i_main_o) \
		$(OBJDIR)/comptime.o \
		$(OBJDIR)/string.o   \
		$(OBJDIR)/d_main.o   \
		$(OBJDIR)/d_clisrv.o \
		$(OBJDIR)/d_net.o    \
		$(OBJDIR)/d_netfil.o \
		$(OBJDIR)/d_netcmd.o \
		$(OBJDIR)/dehacked.o \
		$(OBJDIR)/z_zone.o   \
		$(OBJDIR)/f_finale.o \
		$(OBJDIR)/f_wipe.o   \
		$(OBJDIR)/g_game.o   \
		$(OBJDIR)/g_input.o  \
		$(OBJDIR)/am_map.o   \
		$(OBJDIR)/command.o  \
		$(OBJDIR)/console.o  \
		$(OBJDIR)/hu_stuff.o \
		$(OBJDIR)/y_inter.o  \
		$(OBJDIR)/st_stuff.o \
		$(OBJDIR)/m_aatree.o \
		$(OBJDIR)/m_anigif.o \
		$(OBJDIR)/m_argv.o   \
		$(OBJDIR)/m_bbox.o   \
		$(OBJDIR)/m_cheat.o  \
		$(OBJDIR)/m_cond.o   \
		$(OBJDIR)/m_fixed.o  \
		$(OBJDIR)/m_menu.o   \
		$(OBJDIR)/m_misc.o   \
		$(OBJDIR)/m_random.o \
		$(OBJDIR)/m_queue.o  \
		$(OBJDIR)/info.o     \
		$(OBJDIR)/p_ceilng.o \
		$(OBJDIR)/p_enemy.o  \
		$(OBJDIR)/p_floor.o  \
		$(OBJDIR)/p_inter.o  \
		$(OBJDIR)/p_lights.o \
		$(OBJDIR)/p_map.o    \
		$(OBJDIR)/p_maputl.o \
		$(OBJDIR)/p_mobj.o   \
		$(OBJDIR)/p_polyobj.o\
		$(OBJDIR)/p_saveg.o  \
		$(OBJDIR)/p_setup.o  \
		$(OBJDIR)/p_sight.o  \
		$(OBJDIR)/p_spec.o   \
		$(OBJDIR)/p_telept.o \
		$(OBJDIR)/p_tick.o   \
		$(OBJDIR)/p_user.o   \
		$(OBJDIR)/p_slopes.o \
		$(OBJDIR)/tables.o   \
		$(OBJDIR)/r_bsp.o    \
		$(OBJDIR)/r_data.o   \
		$(OBJDIR)/r_draw.o   \
		$(OBJDIR)/r_main.o   \
		$(OBJDIR)/r_plane.o  \
		$(OBJDIR)/r_segs.o   \
		$(OBJDIR)/r_sky.o    \
		$(OBJDIR)/r_splats.o \
		$(OBJDIR)/r_things.o \
		$(OBJDIR)/screen.o   \
		$(OBJDIR)/v_video.o  \
		$(OBJDIR)/s_sound.o  \
		$(OBJDIR)/sounds.o   \
		$(OBJDIR)/w_wad.o    \
		$(OBJDIR)/filesrch.o \
		$(OBJDIR)/mserv.o    \
		$(OBJDIR)/i_tcp.o    \
		$(OBJDIR)/lzf.o	     \
		$(OBJDIR)/vid_copy.o \
		$(OBJDIR)/b_bot.o \
		$(i_cdmus_o)    \
		$(i_net_o)      \
		$(i_system_o)   \
		$(i_sound_o)    \
		$(OBJS)

# List of languages to compile.
# For reference, this is the command I use to build a srb2.pot file from the source code.
# (The listed source files are the ones containing translated strings).
# FILES=""; for file in `find ./ | grep "\.c" | grep -v svn`; do [ "`grep "M_GetText(" $file`" ] && FILES="$FILES $file"; done; xgettext -d srb2 -o locale/srb2.pot -kM_GetText -F --no-wrap $FILES
ifdef GETTEXT
POS:=$(BIN)/en.mo

OPTS+=-DGETTEXT
endif

ifdef DJGPPDOS
all:	 pre-build $(BIN)/$(EXENAME)
endif

ifdef XBOX
all:	 pre-build $(BIN)/$(BINNAME)
endif

ifdef PS3N
all:	 pre-build $(BIN)/$(PKGNAME)
endif

ifdef WII
all:	pre-build $(BIN)/$(DOLNAME)
endif

ifdef PANDORA
all:	pre-build $(BIN)/$(PNDNAME)
endif

ifdef PSP
all:	 pre-build $(BIN)/$(BINNAME) post-build
endif

ifdef DC
all:	 pre-build $(BIN)/$(BINNAME) post-build
endif

ifndef DC
ifndef PSP
ifndef XBOX
ifdef MINGW
ifndef SDL
all:	 pre-build $(BIN)/$(EXENAME) dll
endif
endif
endif
endif

ifdef SDL
all:	 pre-build $(BIN)/$(EXENAME)
endif
endif

ifdef NDS
all:	$(BIN)/$(EXENAME:.elf=.nds)
endif

ifdef DUMMY
all:	$(BIN)/$(EXENAME)
endif

cleandep:
	$(REMOVE) $(OBJDIR)/depend.dep
	$(REMOVE) comptime.h

pre-build:
ifdef WINDOWSHELL
	-..\comptime.bat .
else
	-@../comptime.sh .
endif

clean:
	$(REMOVE) *~ *.flc
	$(REMOVE) $(OBJDIR)/*.o

ifdef MINGW
	$(REMOVE) $(OBJDIR)/*.res
endif

ifdef CYGWIN32
	$(REMOVE) $(OBJDIR)/*.res
endif

#make a big srb2.s that is the disasm of the exe (dos only ?)
asm:
	$(CC) $(LDFLAGS) $(OBJS) -o $(OBJDIR)/tmp.exe $(LIBS)
	$(OBJDUMP) -d $(OBJDIR)/tmp.exe --no-show-raw-insn > srb2.s
	$(REMOVE) $(OBJDIR)/tmp.exe

# executable
# NOTE: DJGPP's objcopy do not have --add-gnu-debuglink

$(BIN)/$(EXENAME): $(POS) $(OBJS)
	-$(MKDIR) $(BIN)
	@echo Linking $(EXENAME)...
	$(LD) $(LDFLAGS) $(OBJS) -o $(BIN)/$(EXENAME) $(LIBS)
ifndef VALGRIND
ifndef NOOBJDUMP
	@echo Dumping debugging info
	$(OBJDUMP) $(OBJDUMP_OPTS) $(BIN)/$(EXENAME) > $(BIN)/$(DBGNAME).txt
	-$(GZIP) $(GZIP_OPTS) $(BIN)/$(DBGNAME).txt
ifndef WINDOWSHELL
	-$(GZIP) $(GZIP_OPT2) $(BIN)/$(DBGNAME).txt
endif
endif

# mac os x lsdlsrb2 does not like objcopy
ifndef MACOSX
ifndef PSP
	$(OBJCOPY) $(BIN)/$(EXENAME) $(BIN)/$(DBGNAME)
	$(OBJCOPY) --strip-debug $(BIN)/$(EXENAME)
	-$(OBJCOPY) --add-gnu-debuglink=$(BIN)/$(DBGNAME) $(BIN)/$(EXENAME)
endif
endif
ifndef NOUPX
	-$(UPX) $(UPX_OPTS) $(BIN)/$(EXENAME)
endif
endif
	@echo Build is done, please look for $(EXENAME) in $(BIN), \(checking for post steps\)

reobjdump:
	@echo Redumping debugging info
	$(OBJDUMP) $(OBJDUMP_OPTS) $(BIN)/$(DBGNAME) > $(BIN)/$(DBGNAME).txt
	-$(GZIP) $(GZIP_OPTS) $(BIN)/$(DBGNAME).txt
ifndef WINDOWSHELL
	-$(GZIP) $(GZIP_OPT2) $(BIN)/$(DBGNAME).txt
endif

$(OBJDIR):
	-$(MKDIR) $(OBJDIR)

ifndef SDL
ifdef NOHW
dll :
else
dll : opengl_dll
endif
ifdef MINGW
all_dll: opengl_dll minigl_dll ds3d_dll fmod_dll openal_dll

opengl_dll: $(BIN)/r_opengl.dll
$(BIN)/r_opengl.dll: $(OBJDIR)/ogl_win.o $(OBJDIR)/r_opengl.o
	-$(MKDIR) $(BIN)
	@echo Linking R_OpenGL.dll...
	$(CC) --shared  $^ -o $@ -g -Wl,--add-stdcall-alias -lgdi32 -static-libgcc
ifndef NOUPX
	-$(UPX) $(UPX_OPTS) $@
endif

minigl_dll: $(BIN)/r_minigl.dll
$(BIN)/r_minigl.dll: $(OBJDIR)/r_minigl.o
	-$(MKDIR) $(BIN)
	@echo Linking R_MiniGL.dll...
	$(CC) --shared  $^ -o $@ -g -Wl,--add-stdcall-alias -lgdi32

ds3d_dll: $(BIN)/s_ds3d.dll
$(BIN)/s_ds3d.dll: $(OBJDIR)/s_ds3d.o
	@echo Linking S_DS3d.dll...
	$(CC) --shared  $^ -o $@ -g -Wl,--add-stdcall-alias -ldsound -luuid

fmod_dll: $(BIN)/s_fmod.dll
$(BIN)/s_fmod.dll: $(OBJDIR)/s_fmod.o
	-$(MKDIR) $(BIN)
	@echo Linking S_FMOD.dll...
	$(CC) --shared  $^ -o $@ -g -Wl,--add-stdcall-alias -lfmod

openal_dll: $(BIN)/s_openal.dll
$(BIN)/s_openal.dll: $(OBJDIR)/s_openal.o
	-$(MKDIR) $(BIN)
	@echo Linking S_OpenAL.dll...
	$(CC) --shared  $^ -o $@ -g -Wl,--add-stdcall-alias -lopenal32
else
all_dll: fmod_so openal_so

fmod_so: $(BIN)/s_fmod.so
$(BIN)/s_fmod.so: $(OBJDIR)/s_fmod.o
	-$(MKDIR) $(BIN)
	@echo Linking S_FMOD.so...
	$(CC) --shared $^ -o $@ -g --nostartfiles -lm -lfmod

openal_so: $(BIN)/s_openal.so
$(BIN)/s_openal.so: $(OBJDIR)/s_openal.o
	-$(MKDIR) $(BIN)
	@echo Linking S_OpenAL.so...
	$(CC) --shared $^ -o $@ -g --nostartfiles -lm -lopenal

endif

else
ifdef SDL
ifdef MINGW
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
else
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -I/usr/X11R6/include -c $< -o $@
endif
endif

endif

#dependecy made by gcc itself !
$(OBJS):
ifndef DUMMY
-include $(OBJDIR)/depend.dep
endif

$(OBJDIR)/depend.dep:
	@echo "Creating dependency file, depend.dep"
	@echo > comptime.h
	-$(MKDIR) $(OBJDIR)
	$(CC) $(CFLAGS) -MM *.c > $(OBJDIR)/depend.ped
	$(CC) $(CFLAGS) -MM $(INTERFACE)/*.c >> $(OBJDIR)/depend.ped
ifndef NOHW
	$(CC) $(CFLAGS) -MM hardware/*.c >> $(OBJDIR)/depend.ped
endif
ifndef NO_LUA
	$(CC) $(CFLAGS) -MM blua/*.c >> $(OBJDIR)/depend.ped
endif
	@sed -e 's,\(.*\)\.o: ,$(subst /,\/,$(OBJDIR))\/&,g' < $(OBJDIR)/depend.ped > $(OBJDIR)/depend.dep
	$(REMOVE) $(OBJDIR)/depend.ped
	@echo "Created dependency file, depend.dep"

ifdef DC
$(OBJDIR)/v_video.o: v_video.c doomdef.h doomtype.h g_state.h m_swap.h r_local.h \
  tables.h m_fixed.h screen.h command.h m_bbox.h r_main.h d_player.h \
  p_pspr.h info.h d_think.h sounds.h p_mobj.h doomdata.h d_ticcmd.h \
  r_data.h r_defs.h r_state.h r_bsp.h r_segs.h r_plane.h r_sky.h \
  r_things.h r_draw.h v_video.h hu_stuff.h d_event.h w_wad.h console.h \
  i_video.h z_zone.h doomstat.h d_clisrv.h d_netcmd.h
	$(CC) $(CFLAGS) -fno-omit-frame-pointer $(WFLAGS) -c $< -o $@
endif

ifdef VALGRIND
$(OBJDIR)/z_zone.o: z_zone.c
	$(CC) $(CFLAGS) $(WFLAGS) -DHAVE_VALGRIND $(VALGRIND_CFLAGS) -c $< -o $@
endif

$(OBJDIR)/comptime.o: comptime.c pre-build
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(BIN)/%.mo: locale/%.po
	-$(MKDIR) $(BIN)
	$(MSGFMT) -f -o $@ $<

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(INTERFACE)/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

ifdef MACOSX
$(OBJDIR)/%.o: sdl/macosx/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif

$(OBJDIR)/%.o: hardware/%.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: blua/%.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.nas
	$(NASM) $(NASMOPTS) -o $@ -f $(NASMFORMAT) $<

$(OBJDIR)/vid_copy.o: vid_copy.s asm_defs.inc
	$(CC) $(OPTS) $(ASFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/%.o: %.s
	$(CC) $(OPTS) -x assembler-with-cpp -c $< -o $@

$(OBJDIR)/SRB2.res: win32/Srb2win.rc win32/afxres.h win32/resource.h
	$(WINDRES) -i $< -O rc $(WINDRESFLAGS) --include-dir=win32 -o $@ -O coff

ifdef DC
$(OBJDIR)/romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d ../data -v

$(OBJDIR)/romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

$(OBJDIR)/dchelp.o: $(INTERFACE)/SRB2DC/dchelp.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/i_udp.o: $(INTERFACE)/SRB2DC/i_udp.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(BIN)/IP.BIN: $(INTERFACE)/SRB2DC/IP.BIN
	$(CP) $< $@

$(BIN)/SRB2DC.cdi.pass1: $(INTERFACE)/SRB2DC/SELFBOOT.BIN
	$(CP) $< $@

$(BIN)/$(BINNAME): $(BIN)/$(EXENAME)
	$(KOS_OBJCOPY) -R .stack -O binary $< $@

$(BIN)/1ST_READ.BIN: $(BIN)/$(BINNAME) $(BIN)/scramble
	$(BIN)/scramble $< $@

$(BIN)/scramble: $(INTERFACE)/SRB2DC/scramble.c
	-$(MKDIR) $(BIN)
	$(HOSTCC) $< -o $@

iso: $(BIN)/SRB2DC.iso
cdi: $(BIN)/SRB2DC.cdi

$(BIN)/SRB2DC.iso.pass1: $(BIN)/1ST_READ.BIN $(BIN)/IP.BIN
	-$(MKDIR) $(BIN)/cdrom
	$(CP) $(BIN)/1ST_READ.BIN $(D_FILES) $(BIN)/cdrom
	$(MKISOFS) -l -r -o $@ $(BIN)/cdrom

$(BIN)/SRB2DC.iso.pass2: $(BIN)/SRB2DC.iso.pass1
	$(DD) if=$< of=$@ bs=2048 skip=16 status=noxfer

$(BIN)/SRB2DC.iso: $(BIN)/SRB2DC.iso.pass2 $(BIN)/IP.BIN
	@cat $(BIN)/IP.BIN $(BIN)/SRB2DC.iso.pass2 > $@

$(BIN)/SRB2DC.cdi: $(BIN)/SRB2DC.iso.pass2 $(BIN)/SRB2DC.cdi.pass1 $(BIN)/IP.BIN
	@cat $(BIN)/SRB2DC.cdi.pass1 $(BIN)/IP.BIN $(BIN)/SRB2DC.iso.pass2 > $@

post-build: $(BIN)/1ST_READ.BIN
endif

ifdef XBOX
$(OBJDIR)/xboxhelp.o: $(INTERFACE)/SRB2XBOX/xboxhelp.c
		$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(BIN)/$(BINNAME): $(BIN)/$(EXENAME)
	$(CXBE) -OUT:"$@" -DUMPINFO:"$(BIN)/SRB2XBOX.cxbe"  -TITLE:"Sonic Robo Blast 2" $<
endif

ifdef NDS
$(BIN)/$(EXENAME:.elf=.nds): $(BIN)/$(EXENAME:.elf=.arm9)
	$(NDSTOOL) -c $@ -9 $(BIN)/$(EXENAME:.elf=.arm9)

%.arm9: %.elf
	$(OBJCOPY) -O binary $< $@
endif

ifdef MINGW
ifndef SDL
ifndef NOHW
$(OBJDIR)/r_opengl.o: hardware/r_opengl/r_opengl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@

$(OBJDIR)/ogl_win.o: hardware/r_opengl/ogl_win.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@

$(OBJDIR)/r_minigl.o: hardware/r_minigl/r_minigl.c hardware/r_opengl/r_opengl.h \
 doomdef.h doomtype.h g_state.h m_swap.h hardware/hw_drv.h screen.h \
 command.h hardware/hw_data.h hardware/hw_glide.h hardware/hw_defs.h \
 hardware/hw_md2.h hardware/hw_glob.h hardware/hw_main.h am_map.h \
 d_event.h d_player.h p_pspr.h m_fixed.h tables.h info.h d_think.h \
 p_mobj.h doomdata.h d_ticcmd.h r_defs.h hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -D_WINDOWS -mwindows -c $< -o $@
endif

ifndef NOHS
$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_ds3d.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_ds3d/s_ds3d.c

$(OBJDIR)/s_fmod.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o $(WFLAGS) -D_WINDOWS -mwindows -c hardware/s_openal/s_openal.c
endif
endif
endif

ifdef SDL

ifdef MINGW
$(OBJDIR)/win_dbg.o: win32/win_dbg.c
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif

ifdef STATICHS
$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

ifdef MINGW
$(OBJDIR)/s_ds3d.o: hardware/s_ds3d/s_ds3d.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif
else

$(OBJDIR)/s_fmod.o: hardware/s_fmod/s_fmod.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_fmod.o -DHW3SOUND -DUNIXCOMMON -shared -nostartfiles -c hardware/s_fmod/s_fmod.c

$(OBJDIR)/s_openal.o: hardware/s_openal/s_openal.c hardware/hw3dsdrv.h \
 hardware/hw_dll.h
	$(CC) $(M5) -Os -o $(OBJDIR)/s_openal.o -DHW3SOUND -DUNIXCOMMON -shared -nostartfiles -c hardware/s_openal/s_openal.c
endif

ifdef FILTERS
$(OBJDIR)/%.o: $(INTERFACE)/filter/%.c
	@echo $< needs deps
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/filters.o: $(INTERFACE)/filter/filters.c $(INTERFACE)/filter/filters.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/hq2x.o: $(INTERFACE)/filter/hq2x.c $(INTERFACE)/filter/hq2x.c \
 $(INTERFACE)/filter/filters.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@

$(OBJDIR)/lq2x.o: $(INTERFACE)/filter/lq2x.c $(INTERFACE)/filter/lq2x.c \
 $(INTERFACE)/filter/filters.h $(INTERFACE)/filter/interp.h \
 $(INTERFACE)/filter/hq2x.h $(INTERFACE)/filter/lq2x.h
	$(CC) $(CFLAGS) $(WFLAGS) -c $< -o $@
endif
endif

#############################################################
#
#############################################################
