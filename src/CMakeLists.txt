# SRB2 Core

# Core sources
set(SRB2_CORE_SOURCES
	am_map.c
	b_bot.c
	command.c
	comptime.c
	console.c
	d_clisrv.c
	d_main.c
	d_net.c
	d_netcmd.c
	d_netfil.c
	dehacked.c
	f_finale.c
	f_wipe.c
	filesrch.c
	g_game.c
	g_input.c
	hu_stuff.c
	i_tcp.c
	info.c
	lzf.c
	m_anigif.c
	m_argv.c
	m_bbox.c
	m_cheat.c
	m_cond.c
	m_fixed.c
	m_menu.c
	m_misc.c
	m_queue.c
	m_random.c
	md5.c
	mserv.c
	p_ceilng.c
	p_enemy.c
	p_fab.c
	p_floor.c
	p_inter.c
	p_lights.c
	p_map.c
	p_maputl.c
	p_mobj.c
	p_polyobj.c
	p_saveg.c
	p_setup.c
	p_sight.c
	p_spec.c
	p_telept.c
	p_tick.c
	p_user.c
	r_bsp.c
	r_data.c
	r_draw.c
	r_main.c
	r_plane.c
	r_segs.c
	r_sky.c
	r_splats.c
	r_things.c
	s_sound.c
	screen.c
	sounds.c
	st_stuff.c
	#string.c
	tables.c
	v_video.c
	w_wad.c
	y_inter.c
	z_zone.c
)

set(SRB2_CORE_HEADERS
	am_map.h
	b_bot.h
	byteptr.h
	command.h
	comptime.h
	console.h
	d_clisrv.h
	d_event.h
	d_main.h
	d_net.h
	d_netcmd.h
	d_netfil.h
	d_player.h
	d_think.h
	d_ticcmd.h
	dehacked.h
	doomdata.h
	doomdef.h
	doomstat.h
	doomtype.h
	endian.h
	f_finale.h
	fastcmp.h
	filesrch.h
	g_game.h
	g_input.h
	g_state.h
	hu_stuff.h
	i_joy.h
	i_net.h
	i_sound.h
	i_system.h
	i_tcp.h
	i_video.h
	info.h
	keys.h
	lzf.h
	m_anigif.h
	m_argv.h
	m_bbox.h
	m_cheat.h
	m_cond.h
	m_dllist.h
	m_fixed.h
	m_menu.h
	m_misc.h
	m_queue.h
	m_random.h
	m_swap.h
	md5.h
	mserv.h
	p5prof.h
	p_local.h
	p_maputl.h
	p_mobj.h
	p_polyobj.h
	p_pspr.h
	p_saveg.h
	p_setup.h
	p_spec.h
	p_tick.h
	r_bsp.h
	r_data.h
	r_defs.h
	r_draw.h
	r_local.h
	r_main.h
	r_plane.h
	r_segs.h
	r_sky.h
	r_splats.h
	r_state.h
	r_things.h
	s_sound.h
	screen.h
	sounds.h
	st_stuff.h
	tables.h
	v_video.h
	w_wad.h
	y_inter.h
	z_zone.h
)

prepend_sources(SRB2_CORE_SOURCES)
prepend_sources(SRB2_CORE_HEADERS)

set(SRB2_CORE_HEADERS ${SRB2_CORE_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(SRB2_HWRENDER_SOURCES
	hardware/hw_bsp.c
	hardware/hw_cache.c
	hardware/hw_draw.c
	hardware/hw_light.c
	hardware/hw_main.c
	hardware/hw_md2.c
	hardware/hw_trick.c
)

set (SRB2_HWRENDER_HEADERS
	hardware/hw_data.h
	hardware/hw_defs.h
	hardware/hw_dll.h
	hardware/hw_drv.h
	hardware/hw_glide.h
	hardware/hw_glob.h
	hardware/hw_light.h
	hardware/hw_main.h
	hardware/hw_md2.h
)

prepend_sources(SRB2_HWRENDER_SOURCES)
prepend_sources(SRB2_HWRENDER_HEADERS)

set(SRB2_R_OPENGL_SOURCES
	hardware/r_opengl/r_opengl.c
)

set(SRB2_R_OPENGL_HEADERS
	hardware/r_opengl/r_opengl.h
)

prepend_sources(SRB2_R_OPENGL_SOURCES)
prepend_sources(SRB2_R_OPENGL_HEADERS)

set(SRB2_NASM_SOURCES
	tamp_mmx.nas
	tmap.nas
)

prepend_sources(SRB2_NASM_SOURCES)

### Configuration
set(SRB2_CONFIG_HAVE_BLUA ON CACHE BOOL
	"Enable Lua interpreter support")
set(SRB2_CONFIG_HAVE_PNG ON CACHE BOOL
	"Enable PNG support. Depends on zlib, so will be disabled if you don't enable that too.")
set(SRB2_CONFIG_HAVE_ZLIB ON CACHE BOOL
	"Enable zlib support")
set(SRB2_CONFIG_HAVE_GME ON CACHE BOOL
	"Enable GME support")
set(SRB2_CONFIG_HWRENDER ON CACHE BOOL
	"Enable hardware rendering through OpenGL")
set(SRB2_CONFIG_USEASM OFF CACHE BOOL
	"Enable NASM tmap implementation for software mode speedup.")
set(SRB2_CONFIG_STATIC_OPENGL OFF CACHE BOOL
	"Use statically linked OpenGL. NOT RECOMMENDED.")

if(${SRB2_CONFIG_HAVE_BLUA})
	add_definitions(-DHAVE_BLUA)
	set(SRB2_LUA_SOURCES
		lua_baselib.c
		lua_consolelib.c
		lua_hooklib.c
		lua_hudlib.c
		lua_infolib.c
		lua_maplib.c
		lua_mathlib.c
		lua_mobjlib.c
		lua_playerlib.c
		lua_script.c
		lua_skinlib.c
		lua_thinkerlib.c
	)
	set(SRB2_LUA_HEADERS
		lua_hook.h
		lua_hud.h
		lua_libs.h
		lua_script.h
	)

	prepend_sources(SRB2_LUA_SOURCES)
	prepend_sources(SRB2_LUA_HEADERS)

	set(SRB2_BLUA_SOURCES
		blua/lapi.c
		blua/lauxlib.c
		blua/lbaselib.c
		blua/lcode.c
		blua/ldebug.c
		blua/ldo.c
		blua/ldump.c
		blua/lfunc.c
		blua/lgc.c
		blua/linit.c
		blua/llex.c
		blua/lmem.c
		blua/lobject.c
		blua/lopcodes.c
		blua/lparser.c
		blua/lstate.c
		blua/lstring.c
		blua/lstrlib.c
		blua/ltable.c
		blua/ltablib.c
		blua/ltm.c
		blua/lundump.c
		blua/lvm.c
		blua/lzio.c
	)
	set(SRB2_BLUA_HEADERS
		blua/lapi.h
		blua/lauxlib.h
		blua/lcode.h
		blua/ldebug.h
		blua/ldo.h
		blua/lfunc.h
		blua/lgc.h
		blua/llex.h
		blua/llimits.h
		blua/lmem.h
		blua/lobject.h
		blua/lopcodes.h
		blua/lparser.h
		blua/lstate.h
		blua/lstring.h
		blua/ltable.h
		blua/ltm.h
		blua/lua.h
		blua/luaconf.h
		blua/lualib.h
		blua/lundump.h
		blua/lvm.h
		blua/lzio.h
	)
	prepend_sources(SRB2_BLUA_SOURCES)
	prepend_sources(SRB2_BLUA_HEADERS)
endif()

if(${SRB2_CONFIG_HAVE_GME})
	find_package(GME)
	if(${GME_FOUND})
		set(SRB2_HAVE_GME ON)
	else()
		message(WARNING "You have specified that GME is available but it was not found.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_ZLIB})
	find_package(ZLIB)
	if(${ZLIB_FOUND})
		set(SRB2_HAVE_ZLIB ON)
	else()
		message(WARNING "You have specified that ZLIB is available but it was not found. SRB2 may not compile correctly.")
	endif()
endif()

if(${SRB2_CONFIG_HAVE_PNG} AND ${SRB2_CONFIG_HAVE_ZLIB})
	if (${ZLIB_FOUND})
		find_package(PNG)
		if(${PNG_FOUND})
			set(SRB2_HAVE_PNG ON)
			add_definitions(-DHAVE_PNG)
			add_definitions(-D_LARGEFILE64_SOURCE)
		else()
			message(WARNING "You have specified that PNG is available but it was not found. SRB2 may not compile correctly.")
		endif()
	endif()
endif()

if(${SRB2_CONFIG_HWRENDER})
	add_definitions(-DHWRENDER)
endif()

if(${SRB2_CONFIG_HWRENDER} AND ${SRB2_CONFIG_STATIC_OPENGL})
	find_package(OpenGL)
	if(${OPENGL_FOUND})
		add_definitions(-DHWRENDER)
		add_definitions(-DSTATIC_OPENGL)
	else()
		message(WARNING "You have specified static opengl but opengl was not found. Not setting HWRENDER.")
	endif()
endif()

if(${SRB2_CONFIG_USEASM})
	enable_language(ASM-NASM)
	set(SRB2_USEASM ON)
	add_definitions(-DUSEASM)
else()
	set(SRB2_USEASM OFF)
	add_definitions(-DNOASM -DNONX86)
endif()

# Targets

if(${CMAKE_SYSTEM} MATCHES Windows)
	add_subdirectory(win32)
endif()

# Compatibility flag with later versions of GCC
# We should really fix our code to not need this
if(NOT CLANG AND NOT MSVC)
	add_compile_options(-mno-ms-bitfields)
endif()

add_definitions(-DCMAKECONFIG)

add_subdirectory(sdl)
