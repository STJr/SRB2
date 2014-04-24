#
# Simple CMakeLists for Sonic Robo Blast 2
#
# Tweaked for FreeBSD, but should be extendable to support other systems
#
PROJECT(SRB2)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#
# Dependencies
#
FIND_PACKAGE(SDL)
FIND_PACKAGE(SDL_mixer)
FIND_PACKAGE(PNG)

#
# Common stuff
#

# not added defines:
# -DUSEASM - unportable assembler not needed
# -DHAVE_PNG - does not build (incorrect use of PNG_EXPORT etc.)
ADD_DEFINITIONS(-DDIRECTFULLSCREEN -DSDL -DHAVE_MIXER -DNOHW -DHW3SOUND -DHAVE_BLUA)

SET(SDL_DIR sdl) # may be set to SDL2 optionally

SET(COMMON_SRCS
	src/${SDL_DIR}/dosstr.c
	src/${SDL_DIR}/endtxt.c
	src/${SDL_DIR}/hwsym_sdl.c
	src/${SDL_DIR}/i_cdmus.c
	src/${SDL_DIR}/i_main.c
	src/${SDL_DIR}/i_net.c
	src/${SDL_DIR}/i_system.c
	src/${SDL_DIR}/i_video.c
	src/${SDL_DIR}/mixer_sound.c
	src/am_map.c
	src/b_bot.c
	src/blua/lapi.c
	src/blua/lauxlib.c
	src/blua/lbaselib.c
	src/blua/lcode.c
	src/blua/ldebug.c
	src/blua/ldo.c
	src/blua/ldump.c
	src/blua/lfunc.c
	src/blua/lgc.c
	src/blua/linit.c
	src/blua/llex.c
	src/blua/lmem.c
	src/blua/lobject.c
	src/blua/lopcodes.c
	src/blua/lparser.c
	src/blua/lstate.c
	src/blua/lstring.c
	src/blua/lstrlib.c
	src/blua/ltable.c
	src/blua/ltablib.c
	src/blua/ltm.c
	src/blua/lundump.c
	src/blua/lvm.c
	src/blua/lzio.c
	src/command.c
	src/comptime.c
	src/console.c
	src/d_clisrv.c
	src/d_main.c
	src/d_net.c
	src/d_netcmd.c
	src/d_netfil.c
	src/dehacked.c
	src/f_finale.c
	src/f_wipe.c
	src/filesrch.c
	src/g_game.c
	src/g_input.c
	src/hardware/hw3sound.c
	src/hu_stuff.c
	src/i_tcp.c
	src/info.c
	src/lua_baselib.c
	src/lua_consolelib.c
	src/lua_hooklib.c
	src/lua_hudlib.c
	src/lua_infolib.c
	src/lua_maplib.c
	src/lua_mathlib.c
	src/lua_mobjlib.c
	src/lua_playerlib.c
	src/lua_script.c
	src/lua_skinlib.c
	src/lua_thinkerlib.c
	src/lzf.c
	src/m_anigif.c
	src/m_argv.c
	src/m_bbox.c
	src/m_cheat.c
	src/m_cond.c
	src/m_fixed.c
	src/m_menu.c
	src/m_misc.c
	src/m_queue.c
	src/m_random.c
	src/md5.c
	src/mserv.c
	src/p_ceilng.c
	src/p_enemy.c
	src/p_fab.c
	src/p_floor.c
	src/p_inter.c
	src/p_lights.c
	src/p_map.c
	src/p_maputl.c
	src/p_mobj.c
	src/p_polyobj.c
	src/p_saveg.c
	src/p_setup.c
	src/p_sight.c
	src/p_spec.c
	src/p_telept.c
	src/p_tick.c
	src/p_user.c
	src/r_bsp.c
	src/r_data.c
	src/r_draw.c
	src/r_main.c
	src/r_plane.c
	src/r_segs.c
	src/r_sky.c
	src/r_splats.c
	src/r_things.c
	src/s_sound.c
	src/screen.c
	src/sounds.c
	src/st_stuff.c
	src/string.c
	src/tables.c
	src/v_video.c
	src/w_wad.c
	src/y_inter.c
	src/z_zone.c
)

#
# Platform-specific stuff
#
IF(${CMAKE_SYSTEM} MATCHES "FreeBSD")
	FIND_LIBRARY(KVM_LIBRARY NAMES kvm)
	IF(KVM_LIBRARY)
		MESSAGE(STATUS "Found libkvm: ${KVM_LIBRARY}")
		SET(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${KVM_LIBRARY})
	ELSE(KVM_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find libkvm!")
	ENDIF(KVM_LIBRARY)
	ADD_DEFINITIONS(-DUNIXCOMMON -DLINUX -DFREEBSD)
ELSE(${CMAKE_SYSTEM} MATCHES "FreeBSD")
	ADD_DEFINITIONS(-DUNIXCOMMON -DLINUX)
	MESSAGE(WARNING "No specific settings for you system, it may be not supported!")
ENDIF(${CMAKE_SYSTEM} MATCHES "FreeBSD")

#
# Targets
#
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIRS})
ADD_EXECUTABLE(SRB2 ${COMMON_SRCS})
TARGET_LINK_LIBRARIES(SRB2 ${SDL_LIBRARY} ${SDL_MIXER_LIBRARIES} ${EXTRA_LIBRARIES})
