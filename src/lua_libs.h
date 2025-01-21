// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2012-2016 by John "JTE" Muniz.
// Copyright (C) 2012-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  lua_libs.h
/// \brief libraries for Lua scripting

extern lua_State *gL;

extern boolean mousegrabbedbylua;
extern boolean ignoregameinputs;

#define MUTABLE_TAGS

#define LREG_VALID "VALID_USERDATA"
#define LREG_EXTVARS "LUA_VARS"
#define LREG_STATEACTION "STATE_ACTION"
#define LREG_ACTIONS "MOBJ_ACTION"
#define LREG_METATABLES "METATABLES"

#define META_STATE "STATE_T*"
#define META_MOBJINFO "MOBJINFO_T*"
#define META_SFXINFO "SFXINFO_T*"
#define META_SKINCOLOR "SKINCOLOR_T*"
#define META_COLORRAMP "SKINCOLOR_T*RAMP"
#define META_SPRITEINFO "SPRITEINFO_T*"
#define META_PIVOTLIST "SPRITEFRAMEPIVOT_T[]"
#define META_FRAMEPIVOT "SPRITEFRAMEPIVOT_T*"

#define META_TAGLIST "TAGLIST"

#define META_MOBJ "MOBJ_T*"
#define META_MAPTHING "MAPTHING_T*"

#define META_PLAYER "PLAYER_T*"
#define META_TICCMD "TICCMD_T*"
#define META_SKIN "SKIN_T*"
#define META_POWERS "PLAYER_T*POWERS"
#define META_SOUNDSID "SKIN_T*SOUNDSID"
#define META_SKINSPRITES "SKIN_T*SKINSPRITES"
#define META_SKINSPRITESLIST "SKIN_T*SKINSPRITES[]"
#define META_SKINSPRITESCOMPAT "SKIN_T*SPRITES" // TODO: 2.3: Delete

#define META_VERTEX "VERTEX_T*"
#define META_LINE "LINE_T*"
#define META_SIDE "SIDE_T*"
#define META_SUBSECTOR "SUBSECTOR_T*"
#define META_SECTOR "SECTOR_T*"
#define META_FFLOOR "FFLOOR_T*"
#ifdef HAVE_LUA_SEGS
#define META_SEG "SEG_T*"
#define META_NODE "NODE_T*"
#endif
#define META_SLOPE "PSLOPE_T*"
#define META_VECTOR2 "VECTOR2_T"
#define META_VECTOR3 "VECTOR3_T"
#define META_MAPHEADER "MAPHEADER_T*"

#define META_POLYOBJ "POLYOBJ_T*"

#define META_CVAR "CONSVAR_T*"

#define META_SECTORLINES "SECTOR_T*LINES"
#ifdef MUTABLE_TAGS
#define META_SECTORTAGLIST "sector_t.taglist"
#endif
#define META_SIDENUM "LINE_T*SIDENUM"
#define META_LINEARGS "LINE_T*ARGS"
#define META_LINESTRINGARGS "LINE_T*STRINGARGS"
#define META_THINGARGS "MAPTHING_T*ARGS"
#define META_THINGSTRINGARGS "MAPTHING_T*STRINGARGS"
#define META_POLYOBJVERTICES "POLYOBJ_T*VERTICES"
#define META_POLYOBJLINES "POLYOBJ_T*LINES"
#ifdef HAVE_LUA_SEGS
#define META_NODEBBOX "NODE_T*BBOX"
#define META_NODECHILDREN "NODE_T*CHILDREN"
#endif

#define META_BBOX "BOUNDING_BOX"

#define META_HUDINFO "HUDINFO_T*"
#define META_PATCH "PATCH_T*"
#define META_COLORMAP "COLORMAP"
#define META_EXTRACOLORMAP "EXTRACOLORMAP_T*"
#define META_CAMERA "CAMERA_T*"

#define META_ACTION "ACTIONF_T*"

#define META_LUABANKS "LUABANKS[]*"

#define META_KEYEVENT "KEYEVENT_T*"
#define META_MOUSE "MOUSE_T*"

boolean luaL_checkboolean(lua_State *L, int narg);

int LUA_EnumLib(lua_State *L);
int LUA_SOCLib(lua_State *L);
int LUA_BaseLib(lua_State *L);
int LUA_MathLib(lua_State *L);
int LUA_HookLib(lua_State *L);
int LUA_ConsoleLib(lua_State *L);
int LUA_InfoLib(lua_State *L);
int LUA_MobjLib(lua_State *L);
int LUA_PlayerLib(lua_State *L);
int LUA_SkinLib(lua_State *L);
int LUA_ThinkerLib(lua_State *L);
int LUA_MapLib(lua_State *L);
int LUA_TagLib(lua_State *L);
int LUA_PolyObjLib(lua_State *L);
int LUA_BlockmapLib(lua_State *L);
int LUA_HudLib(lua_State *L);
int LUA_ColorLib(lua_State *L);
int LUA_InputLib(lua_State *L);
